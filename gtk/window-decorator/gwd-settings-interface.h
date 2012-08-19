/*
 * Copyright © 2010 Canonical Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _COMPIZ_GWD_SETTINGS_INTERFACE_H
#define _COMPIZ_GWD_SETTINGS_INTERFACE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GWD_SETTINGS_INTERFACE(obj) (G_TYPE_CHECK_INSTANCE_CAST (obj))
#define GWD_SETTINGS_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE(obj, \
								       GWD_TYPE_SETTINGS_INTERFACE, \
								       GWDSettingsInterface))
#define GWD_TYPE_SETTINGS_INTERFACE (gwd_settings_interface_get_type ())

typedef struct _GWDSettings GWDSettings;
typedef struct _GWDSettingsInterface GWDSettingsInterface;

struct _GWDSettingsInterface
{
    GTypeInterface parent;
};

enum
{
    BLUR_TYPE_NONE = 0,
    BLUR_TYPE_TITLEBAR = 1,
    BLUR_TYPE_ALL = 2
};

enum
{
    GWD_SETTINGS_PROPERTY_ACTIVE_SHADOW = 1,
    GWD_SETTINGS_PROPERTY_INACTIVE_SHADOW = 2,
    GWD_SETTINGS_PROPERTY_USE_TOOLTIPS = 3,
    GWD_SETTINGS_PROPERTY_DRAGGABLE_BORDER_WIDTH = 4,
    GWD_SETTINGS_PROPERTY_ATTACH_MODAL_DIALOGS = 5,
    GWD_SETTINGS_PROPERTY_BLUR_CHANGED = 6,
    GWD_SETTINGS_PROPERTY_METACITY_THEME = 7,
    GWD_SETTINGS_PROPERTY_ACTIVE_OPACITY = 8,
    GWD_SETTINGS_PROPERTY_INACTIVE_OPACITY = 9,
    GWD_SETTINGS_PROPERTY_ACTIVE_SHADE_OPACITY = 10,
    GWD_SETTINGS_PROPERTY_INACTIVE_SHADE_OPACITY = 11,
    GWD_SETTINGS_PROPERTY_BUTTON_LAYOUT = 12
};

GType gwd_settings_get_type (void);

G_END_DECLS

#endif
