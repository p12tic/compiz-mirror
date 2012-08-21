#include "gwd-settings-storage-interface.h"

static void gwd_settings_storage_default_init (GWDSettingsStorageInterface *settings_interface);

G_DEFINE_INTERFACE (GWDSettingsStorage, gwd_settings_storage, G_TYPE_OBJECT);

static void gwd_settings_storage_default_init (GWDSettingsStorageInterface *settings_interface)
{
}

gboolean gwd_settings_storage_update_use_tooltips (GWDSettingsStorage *settings)
{
    GWDSettingsStorageInterface *interface = GWD_SETTINGS_STORAGE_GET_INTERFACE (settings);
    return (*interface->update_use_tooltips) (settings);
}

gboolean gwd_settings_storage_update_draggable_border_width (GWDSettingsStorage *settings)
{
    GWDSettingsStorageInterface *interface = GWD_SETTINGS_STORAGE_GET_INTERFACE (settings);
    return (*interface->update_draggable_border_width) (settings);
}

gboolean gwd_settings_storage_update_attach_modal_dialogs (GWDSettingsStorage *settings)
{
    GWDSettingsStorageInterface *interface = GWD_SETTINGS_STORAGE_GET_INTERFACE (settings);
    return (*interface->update_attach_modal_dialogs) (settings);
}

gboolean gwd_settings_storage_update_blur (GWDSettingsStorage *settings)
{
    GWDSettingsStorageInterface *interface = GWD_SETTINGS_STORAGE_GET_INTERFACE (settings);
    return (*interface->update_blur) (settings);
}

gboolean gwd_settings_storage_update_metacity_theme (GWDSettingsStorage *settings)
{
    GWDSettingsStorageInterface *interface = GWD_SETTINGS_STORAGE_GET_INTERFACE (settings);
    return (*interface->update_metacity_theme) (settings);
}

gboolean gwd_settings_storage_update_opacity (GWDSettingsStorage *settings)
{
    GWDSettingsStorageInterface *interface = GWD_SETTINGS_STORAGE_GET_INTERFACE (settings);
    return (*interface->update_opacity) (settings);
}

gboolean gwd_settings_storage_update_button_layout (GWDSettingsStorage *settings)
{
    GWDSettingsStorageInterface *interface = GWD_SETTINGS_STORAGE_GET_INTERFACE (settings);
    return (*interface->update_button_layout) (settings);
}

gboolean gwd_settings_storage_update_font (GWDSettingsStorage *settings)
{
    GWDSettingsStorageInterface *interface = GWD_SETTINGS_STORAGE_GET_INTERFACE (settings);
    return (*interface->update_font) (settings);
}

gboolean gwd_settings_storage_update_titlebar_actions (GWDSettingsStorage *settings)
{
    GWDSettingsStorageInterface *interface = GWD_SETTINGS_STORAGE_GET_INTERFACE (settings);
    return (*interface->update_titlebar_actions) (settings);
}
