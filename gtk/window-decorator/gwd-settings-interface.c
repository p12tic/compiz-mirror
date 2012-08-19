#include "gwd-settings-interface.h"

static void gwd_settings_default_init (GWDSettingsInterface *settings_interface);

G_DEFINE_INTERFACE (GWDSettings, gwd_settings, G_TYPE_OBJECT);

static void gwd_settings_default_init (GWDSettingsInterface *settings_interface)
{
    g_object_interface_install_property (settings_interface,
					 g_param_spec_pointer ("active-shadow",
							       "Active Shadow",
							       "Active Shadow Settings",
							       G_PARAM_READABLE));
    g_object_interface_install_property (settings_interface,
					 g_param_spec_pointer ("inactive-shadow",
							       "Inactive Shadow",
							       "Inactive Shadow",
							       G_PARAM_READABLE));
    g_object_interface_install_property (settings_interface,
					 g_param_spec_boolean ("use-tooltips",
							       "Use Tooltips",
							       "Use Tooltips Setting",
							       FALSE,
							       G_PARAM_READABLE));
    g_object_interface_install_property (settings_interface,
					 g_param_spec_int ("draggable-border-width",
							   "Draggable Border Width",
							   "Draggable Border Width Setting",
							   0,
							   64,
							   7,
							   G_PARAM_READABLE));
    g_object_interface_install_property (settings_interface,
					 g_param_spec_boolean ("attach-modal-dialogs",
							       "Attach modal dialogs",
							       "Attach modal dialogs setting",
							       FALSE,
							       G_PARAM_READABLE));
    g_object_interface_install_property (settings_interface,
					 g_param_spec_int ("blur",
							   "Blur Type",
							   "Blur type property",
							   0,
							   2,
							   0,
							   G_PARAM_READABLE));
    g_object_interface_install_property (settings_interface,
					 g_param_spec_string ("metacity-theme",
							      "Metacity Theme",
							      "Metacity Theme Setting",
							      "",
							      G_PARAM_READABLE));
    g_object_interface_install_property (settings_interface,
					 g_param_spec_double ("metacity-active-opacity",
							      "Metacity Active Opacity",
							      "Metacity Active Opacity",
							      0.0,
							      1.0,
							      1.0,
							      G_PARAM_READABLE));
    g_object_interface_install_property (settings_interface,
					 g_param_spec_double ("metacity-inactive-opacity",
							      "Metacity Inactive Opacity",
							      "Metacity Inactive Opacity",
							      0.0,
							      1.0,
							      1.0,
							      G_PARAM_READABLE));
    g_object_interface_install_property (settings_interface,
					 g_param_spec_boolean ("metacity-active-shade-opacity",
							      "Metacity Active Shade Opacity",
							      "Metacity Active Shade Opacity",
							      FALSE,
							      G_PARAM_READABLE));
    g_object_interface_install_property (settings_interface,
					 g_param_spec_boolean ("metacity-inactive-shade-opacity",
							      "Metacity Inactive Shade Opacity",
							      "Metacity Inactive Shade Opacity",
							      FALSE,
							      G_PARAM_READABLE));
}
