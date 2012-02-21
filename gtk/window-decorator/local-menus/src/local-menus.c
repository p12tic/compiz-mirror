/*
 * Copyright © 2006 Novell, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: David Reveman <davidr@novell.com>
 *
 * 2D Mode: Copyright © 2010 Sam Spilsbury <smspillaz@gmail.com>
 * Frames Management: Copright © 2011 Canonical Ltd.
 *        Authored By: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include <string.h>
#include "local-menus.h"
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#define GLOBAL 0
#define LOCAL 1

gint menu_mode = GLOBAL;

static void
gwd_menu_mode_changed (GSettings *settings,
		       gchar     *key,
		       gpointer  user_data)
{
    menu_mode = g_settings_get_enum (settings, "menu-mode");
}

active_local_menu *active_menu;
pending_local_menu *pending_menu;

gboolean
gwd_window_should_have_local_menu (WnckWindow *win)
{
#ifdef META_HAS_LOCAL_MENUS
    const gchar * const *schemas = g_settings_list_schemas ();
    static GSettings *lim_settings = NULL;
    while (*schemas != NULL && !lim_settings)
    {
	if (g_str_equal (*schemas, "com.canonical.indicator.appmenu"))
	{
	    lim_settings = g_settings_new ("com.canonical.indicator.appmenu");
	    menu_mode = g_settings_get_enum (lim_settings, "menu-mode");
	    g_signal_connect (lim_settings, "changed::menu-mode", G_CALLBACK (gwd_menu_mode_changed), NULL);
	    break;
	}
	++schemas;
    }

    if (lim_settings)
	return menu_mode;
#endif

    return FALSE;
}

gchar *
gwd_get_entry_id_from_sync_variant (GVariant *args)
{
    /* We need to get the indicator data once we've called show */

    GVariantIter* iter            = NULL;
    gchar*        name_hint       = NULL;
    gchar*        indicator_id    = NULL;
    gchar*        entry_id        = NULL;
    gchar*        label           = NULL;
    gboolean      label_sensitive = FALSE;
    gboolean      label_visible   = FALSE;
    guint32       image_type      = 0;
    gchar*        image_data      = NULL;
    gboolean      image_sensitive = FALSE;
    gboolean      image_visible   = FALSE;
    gint32        priority        = -1;

    g_variant_get (args, "(a(ssssbbusbbi))", &iter);
    while (g_variant_iter_loop (iter, "(ssssbbusbbi)",
			    &indicator_id,
			    &entry_id,
			    &name_hint,
			    &label,
			    &label_sensitive,
			    &label_visible,
			    &image_type,
			    &image_data,
			    &image_sensitive,
			    &image_visible,
			    &priority))
    {
	g_variant_unref (args);
	return g_strdup (entry_id);
    }

    g_variant_unref (args);
    g_assert_not_reached ();
    return NULL;
}

static void
on_local_menu_activated (GDBusProxy *proxy,
			 gchar      *sender_name,
			 gchar      *signal_name,
			 GVariant   *parameters,
			 gpointer   user_data)
{
#ifdef META_HAS_LOCAL_MENUS
    if (g_strcmp0 (signal_name, "EntryActivated") == 0)
    {
	show_local_menu_data *d = (show_local_menu_data *) user_data;
	gchar *entry_id = NULL;
	gint            x_out, y_out;
	guint           width, height;

	g_variant_get (parameters, "(s(iiuu))", &entry_id, &x_out, &y_out, &width, &height, NULL);

	if (!d->local_menu_entry_id)
	{
	    GError   *error = NULL;
	    GVariant *params = g_variant_new ("s", "libappmenu.so", NULL);
	    GVariant *args = g_dbus_proxy_call_sync (proxy, "SyncOne", params, 0, 500, NULL, &error);

	    g_assert_no_error (error);
	    d->local_menu_entry_id = gwd_get_entry_id_from_sync_variant (args);
	}

	if (g_strcmp0 (entry_id, d->local_menu_entry_id) == 0)
	{
	    d->rect->x = x_out - d->dx;
	    d->rect->y = y_out - d->dy;
	    d->rect->width = width;
	    d->rect->height = height;
	}
	else if (g_strcmp0 (entry_id, "") == 0)
	{
	    memset (d->rect, 0, sizeof (GdkRectangle));
	    (*d->cb) (d->user_data);

	    g_signal_handlers_disconnect_by_func (d->proxy, on_local_menu_activated, d);

	    g_object_unref (d->proxy);
	    g_object_unref (d->conn);

	    if (active_menu)
	    {
		g_free (active_menu);
		active_menu = NULL;
	    }

	    g_free (d->local_menu_entry_id);
	    g_free (d);
	}
    }
#endif
}

gboolean
gwd_move_window_instead (gpointer user_data)
{
    (*pending_menu->cb) (pending_menu->user_data);
    g_free (pending_menu->user_data);
    g_free (pending_menu);
    pending_menu = NULL;
    return FALSE;
}

void
gwd_prepare_show_local_menu (start_move_window_cb start_move_window,
			     gpointer user_data_start_move_window)
{
    if (pending_menu)
    {
	g_source_remove (pending_menu->move_timeout_id);
	g_free (pending_menu->user_data);
	g_free (pending_menu);
	pending_menu = NULL;
    }

    pending_menu = g_new0 (pending_local_menu, 1);
    pending_menu->cb = start_move_window;
    pending_menu->user_data = user_data_start_move_window;
    pending_menu->move_timeout_id = g_timeout_add (120, gwd_move_window_instead, pending_menu);
}

void
gwd_show_local_menu (Display *xdisplay,
		     Window  frame_xwindow,
		     int      x,
		     int      y,
		     int      x_win,
		     int      y_win,
		     int      button,
		     guint32  timestamp,
		     show_window_menu_hidden_cb cb,
		     gpointer user_data_show_window_menu)
{
    if (pending_menu)
    {
	g_source_remove (pending_menu->move_timeout_id);
	g_free (pending_menu->user_data);
	g_free (pending_menu);
	pending_menu = NULL;
    }

#ifdef META_HAS_LOCAL_MENUS
    GError          *error = NULL;
    GDBusConnection *conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);


    XUngrabPointer (gdk_x11_display_get_xdisplay (gdk_display_get_default ()), CurrentTime);
    XUngrabKeyboard (gdk_x11_display_get_xdisplay (gdk_display_get_default ()), CurrentTime);
    XSync (gdk_x11_display_get_xdisplay (gdk_display_get_default ()), FALSE);

    if (!conn)
    {
	g_print ("error getting connection: %s\n", error->message);
	return;
    }

    GDBusProxy      *proxy = g_dbus_proxy_new_sync (conn, 0, NULL, "com.canonical.Unity.Panel.Service",
					   "/com/canonical/Unity/Panel/Service",
					   "com.canonical.Unity.Panel.Service",
					   NULL, &error);

    if (proxy)
    {
	GVariant *message = g_variant_new ("(uiiu)", frame_xwindow, x, y, time);
	g_dbus_proxy_call_sync (proxy, "ShowAppMenu", message, 0, 500, NULL, &error);
	if (error)
	{
	    g_print ("error calling ShowAppMenu: %s\n", error->message);
	    g_object_unref (proxy);
	    g_object_unref (conn);
	    return;
	}

	show_local_menu_data *data = g_new0 (show_local_menu_data, 1);

	if (active_menu)
	    g_free (active_menu);

	active_menu = g_new0 (active_local_menu, 1);

	data->conn = g_object_ref (conn);
	data->proxy = g_object_ref (proxy);
	data->cb = cb;
	data->user_data = user_data_show_window_menu;
	data->rect = &active_menu->rect;
	data->dx = x - x_win;
	data->dy = y - y_win;
	data->local_menu_entry_id = NULL;  

	g_signal_connect (proxy, "g-signal", G_CALLBACK (on_local_menu_activated), data);

	g_object_unref (conn);
	g_object_unref (proxy);

	return;
    }
    else
    {
	g_print ("error getting proxy: %s\n", error->message);
    }

    g_object_unref (conn);
#endif
}

void
force_local_menus_on (WnckWindow       *win,
		      MetaButtonLayout *button_layout)
{
#ifdef META_HAS_LOCAL_MENUS
    if (gwd_window_should_have_local_menu (win))
    {
	if (button_layout->left_buttons[0] != META_BUTTON_FUNCTION_LAST)
	{
	    unsigned int i = 0;
	    for (; i < MAX_BUTTONS_PER_CORNER; i++)
	    {
		if (button_layout->left_buttons[i] == META_BUTTON_FUNCTION_WINDOW_MENU)
		    break;
		else if (button_layout->left_buttons[i] == META_BUTTON_FUNCTION_LAST)
		{
		    if ((i + 1) < MAX_BUTTONS_PER_CORNER)
		    {
			button_layout->left_buttons[i + 1] = META_BUTTON_FUNCTION_LAST;
			button_layout->left_buttons[i] = META_BUTTON_FUNCTION_WINDOW_MENU;
			break;
		    }
		}
	    }
	}
	if (button_layout->right_buttons[0] != META_BUTTON_FUNCTION_LAST)
	{
	    unsigned int i = 0;
	    for (; i < MAX_BUTTONS_PER_CORNER; i++)
	    {
		if (button_layout->right_buttons[i] == META_BUTTON_FUNCTION_WINDOW_MENU)
		    break;
		else if (button_layout->right_buttons[i] == META_BUTTON_FUNCTION_LAST)
		{
		    if ((i + 1) < MAX_BUTTONS_PER_CORNER)
		    {
			button_layout->right_buttons[i + 1] = META_BUTTON_FUNCTION_LAST;
			button_layout->right_buttons[i] = META_BUTTON_FUNCTION_WINDOW_MENU;
			break;
		    }
		}
	    }
	}
    }
#endif
}


