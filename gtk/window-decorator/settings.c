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
 */

#include "gtk-window-decorator.h"
#include "gwd-settings-writable-interface.h"
#include "gwd-settings-storage-interface.h"
#include "gwd-settings-storage-gconf.h"

GWDSettingsStorage *storage = NULL;

gboolean
shadow_property_changed (WnckScreen *s)
{
    GdkDisplay *display = gdk_display_get_default ();
    Display    *xdisplay = GDK_DISPLAY_XDISPLAY (display);
    GdkScreen  *screen = gdk_display_get_default_screen (display);
    Window     root = GDK_WINDOW_XWINDOW (gdk_screen_get_root_window (screen));
    Atom actual;
    int  result, format;
    unsigned long n, left;
    unsigned char *prop_data;
    XTextProperty shadow_color_xtp;

    gdouble aradius;
    gdouble aopacity;
    gint ax_off;
    gint ay_off;
    char *active_shadow_color;

    gdouble iradius;
    gdouble iopacity;
    gint ix_off;
    gint iy_off;
    char *inactive_shadow_color;

    result = XGetWindowProperty (xdisplay, root, compiz_shadow_info_atom,
				 0, 32768, 0, XA_INTEGER, &actual,
				 &format, &n, &left, &prop_data);

    if (result != Success)
	return FALSE;

    if (n == 8)
    {
	long *data      = (long *) prop_data;
	aradius  = data[0];
	aopacity = data[1];
	ax_off      = data[2];
	ay_off      = data[3];

	iradius  = data[4];
	iopacity = data[5];
	ix_off      = data[6];
	iy_off      = data[7];
	/* Radius and Opacity are multiplied by 1000 to keep precision,
	 * divide by that much to get our real radius and opacity
	 */
	aradius /= 1000;
	aopacity /= 1000;
	iradius /= 1000;
	iopacity /= 1000;

	XFree (prop_data);
    }
    else
    {
	XFree (prop_data);
	return FALSE;
    }

    result = XGetTextProperty (xdisplay, root, &shadow_color_xtp,
			       compiz_shadow_color_atom);

    if (shadow_color_xtp.value)
    {
	int  ret_count = 0;
	char **t_data = NULL;
	
	XTextPropertyToStringList (&shadow_color_xtp, &t_data, &ret_count);
	
	if (ret_count == 2)
	{
	    active_shadow_color = strdup (t_data[0]);
	    inactive_shadow_color = strdup (t_data[1]);

	    XFree (shadow_color_xtp.value);
	    if (t_data)
		XFreeStringList (t_data);
	}
	else
	{
	    XFree (shadow_color_xtp.value);
	    return FALSE;
	}
    }

    return gwd_settings_writable_shadow_property_changed (writable,
							  (gdouble) MAX (0.0, MIN (aradius, 48.0)),
							  (gdouble) MAX (0.0, MIN (aopacity, 6.0)),
							  (gdouble) MAX (-16, MIN (ax_off, 16)),
							  (gdouble) MAX (-16, MIN (ay_off, 16)),
							  active_shadow_color,
							  (gdouble) MAX (0.0, MIN (iradius, 48.0)),
							  (gdouble) MAX (0.0, MIN (iopacity, 6.0)),
							  (gdouble) MAX (-16, MIN (ix_off, 16)),
							  (gdouble) MAX (-16, MIN (iy_off, 16)),
							  inactive_shadow_color);
}

gboolean
init_settings (GWDSettingsWritable *writable,
	       WnckScreen	    *screen)
{
#ifdef USE_GCONF
    storage = gwd_settings_storage_gconf_new (writable);

    shadow_property_changed (screen);
    gwd_settings_storage_update_metacity_theme (storage);
    gwd_settings_storage_update_opacity (storage);
    gwd_settings_storage_update_button_layout (storage);
    gwd_settings_storage_update_font (storage);
    gwd_settings_storage_update_titlebar_actions (storage);
    gwd_settings_storage_update_blur (storage);
    gwd_settings_storage_update_draggable_border_width (storage);
    gwd_settings_storage_update_attach_modal_dialogs (storage);
    gwd_settings_storage_update_use_tooltips (storage);
#endif

    shadow_property_changed (screen);

    return TRUE;
}

void
fini_settings ()
{
    if (storage)
	g_object_unref (storage);
}
