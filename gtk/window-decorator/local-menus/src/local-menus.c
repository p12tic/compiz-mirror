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

#include "local-menus.h"
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

gboolean
gwd_window_should_have_local_menu (WnckWindow *win)
{
#ifdef META_HAS_LOCAL_MENUS
    const gchar * const *schemas = g_settings_list_schemas ();
    static GSettings *lim_settings = NULL;
    while (*schemas != NULL && !lim_settings)
    {
	if (g_str_equal (*schemas, "com.canonical.Unity.Menus"))
	{
	    lim_settings = g_settings_new ("com.canonical.Unity.Menus");
	    break;
	}
	++schemas;
    }

    if (lim_settings)
    {
	if (g_settings_get_boolean (lim_settings, "force-local-menus"))
	{
	    return TRUE;
	}
    }
#endif

    return FALSE;
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
	gchar *entry_id = NULL;

	g_variant_get (parameters, "(s)", &entry_id, NULL);

	gboolean empty = g_strcmp0 (entry_id, "")  == 0;

	if (empty)
	{
	    show_local_menu_data *d = (show_local_menu_data *) user_data;

	    (*d->cb) (d->user_data);

	    g_signal_handlers_disconnect_by_func (d->proxy, on_local_menu_activated, d);

	    g_object_unref (d->proxy);
	    g_object_unref (d->conn);
	}
    }
#endif
}

void
gwd_show_local_menu (Display *xdisplay,
		     Window  frame_xwindow,
		     int      x,
		     int      y,
		     int      button,
		     guint32  timestamp,
		     show_window_menu_hidden_cb cb,
		     gpointer user_data)
{
#ifdef META_HAS_LOCAL_MENUS
    GError          *error = NULL;
    GDBusConnection *conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
    gint            x_out, y_out;
    guint           width, height;

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
	GVariant *reply = g_dbus_proxy_call_sync (proxy, "ShowAppMenu", message, 0, 500, NULL, &error);
	if (error)
	{
	    g_print ("error calling ShowAppMenu: %s\n", error->message);
	    g_object_unref (proxy);
	    g_object_unref (conn);
	    return;
	}

	show_local_menu_data *data = g_new0 (show_local_menu_data, 1);
	g_variant_get (reply, "(iiuu)", &x_out, &y_out, &width, &height, NULL);

	data->conn = g_object_ref (conn);
	data->proxy = g_object_ref (proxy);
	data->cb = cb;
	data->user_data = user_data;

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
		    }
		}
	    }
	}
    }
#endif
}


