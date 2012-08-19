#include "gwd-settings-xproperty-interface.h"

static void gwd_settings_xproperty_storage_default_init (GWDSettingsXPropertyStorageInterface *settings_interface);

G_DEFINE_INTERFACE (GWDSettingsXPropertyStorage, gwd_settings_xproperty_storage, G_TYPE_OBJECT);

static void gwd_settings_xproperty_storage_default_init (GWDSettingsXPropertyStorageInterface *settings_interface)
{
}

gboolean
gwd_settings_xproperty_storage_update_all (GWDSettingsXPropertyStorage *storage)
{
    GWDSettingsXPropertyStorageInterface *iface = GWD_SETTINGS_XPROPERTY_STORAGE_GET_INTERFACE (storage);
    return (*iface->update_all) (storage);
}
