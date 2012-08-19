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

#ifndef _COMPIZ_GWD_SETTINGS_STORAGE_INTERFACE_H
#define _COMPIZ_GWD_SETTINGS_STORAGE_INTERFACE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GWD_SETTINGS_STORAGE_INTERFACE(obj) (G_TYPE_CHECK_INSTANCE_CAST (obj))
#define GWD_SETTINGS_STORAGE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE(obj, \
										GWD_TYPE_WRITABLE_SETTINGS_INTERFACE, \
										GWDSettingsStorageInterface))
#define GWD_TYPE_WRITABLE_SETTINGS_INTERFACE (gwd_settings_storage_interface_get_type ())

typedef struct _GWDSettingsStorage GWDSettingsStorage;
typedef struct _GWDSettingsStorageInterface GWDSettingsStorageInterface;

struct _GWDSettingsStorageInterface
{
    GTypeInterface parent;

    gboolean (*update_all) (GWDSettingsStorage *storage);
};

gboolean
gwd_settings_storage_update_all (GWDSettingsStorage *storage);

GType gwd_settings_storage_interface_get_type (void);

G_END_DECLS

#endif
