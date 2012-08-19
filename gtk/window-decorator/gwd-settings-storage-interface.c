#include "gwd-settings-storage-interface.h"

static void gwd_settings_storage_default_init (GWDSettingsStorageInterface *settings_interface);

G_DEFINE_INTERFACE (GWDSettingsStorage, gwd_settings_storage, G_TYPE_OBJECT);

static void gwd_settings_storage_default_init (GWDSettingsStorageInterface *settings_interface)
{
}

gboolean
gwd_settings_storage_update_all (GWDSettingsStorage *storage)
{
    GWDSettingsStorageInterface *iface = GWD_SETTINGS_STORAGE_GET_INTERFACE (storage);
    return (*iface->update_all) (storage);
}
