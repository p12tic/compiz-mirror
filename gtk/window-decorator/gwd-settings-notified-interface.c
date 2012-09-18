#include "gwd-settings-notified-interface.h"

static void gwd_settings_notified_interface_default_init (GWDSettingsNotifiedInterface *settings_interface);

G_DEFINE_INTERFACE (GWDSettingsNotified, gwd_settings_notified_interface, G_TYPE_OBJECT);

static void gwd_settings_notified_interface_default_init (GWDSettingsNotifiedInterface *settings_interface)
{
}

gboolean
gwd_settings_notified_update_decorations (GWDSettingsNotified *notified)
{
    GWDSettingsNotifiedInterface *iface = GWD_SETTINGS_NOTIFIED_GET_INTERFACE (notified);
    return (*iface->update_decorations) (notified);
}

gboolean
gwd_settings_notified_update_frames (GWDSettingsNotified *notified)
{
    GWDSettingsNotifiedInterface *iface = GWD_SETTINGS_NOTIFIED_GET_INTERFACE (notified);
    return (*iface->update_frames) (notified);
}

gboolean
gwd_settings_notified_update_metacity_theme (GWDSettingsNotified *notified)
{
    GWDSettingsNotifiedInterface *iface = GWD_SETTINGS_NOTIFIED_GET_INTERFACE (notified);
    return (*iface->update_metacity_theme) (notified);
}

gboolean
gwd_settings_notified_metacity_button_layout (GWDSettingsNotified *notified)
{
    GWDSettingsNotifiedInterface *iface = GWD_SETTINGS_NOTIFIED_GET_INTERFACE (notified);
    return (*iface->update_metacity_button_layout) (notified);
}
