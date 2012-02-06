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

#ifdef __cplusplus
extern "C"
{
#endif

#include <glib.h>
#include <gio/gio.h>
#ifndef WNCK_I_KNOW_THIS_IS_UNSTABLE
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#endif
#include <libwnck/libwnck.h>
#include <libwnck/window-action-menu.h>
#include <metacity-private/theme.h>

typedef void (*show_window_menu_hidden_cb) (gpointer);
typedef void (*start_move_window_cb) (gpointer);

typedef struct _pending_local_menu
{
    gint         move_timeout_id;
    gpointer     user_data;
    start_move_window_cb cb;
} pending_local_menu;

typedef struct _active_local_menu
{
    GdkRectangle rect;
} active_local_menu;

extern active_local_menu *active_menu;

typedef struct _show_local_menu_data
{
    GDBusConnection *conn;
    GDBusProxy      *proxy;
    show_window_menu_hidden_cb cb;
    gpointer        user_data;
} show_local_menu_data;

gboolean
gwd_window_should_have_local_menu (WnckWindow *win);

void
force_local_menus_on (WnckWindow       *win,
		      MetaButtonLayout *layout);

/* Button Down */
void
gwd_prepare_show_local_menu (start_move_window_cb start_move_window,
			     gpointer user_data_start_move_window);

/* Button Up */
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
		     gpointer user_data_show_window_menu);
#ifdef __cplusplus
}
#endif
