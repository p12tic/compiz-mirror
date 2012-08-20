#include "gwd-settings-interface.h"

static void gwd_settings_interface_default_init (GWDSettingsInterface *settings_interface);

G_DEFINE_INTERFACE (GWDSettings, gwd_settings_interface, G_TYPE_OBJECT);

static void gwd_settings_interface_default_init (GWDSettingsInterface *settings_interface)
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
    g_object_interface_install_property (settings_interface,
					 g_param_spec_string ("metacity-button-layout",
							      "Metacity Button Layout",
							      "Metacity Button Layout",
							      "",
							      G_PARAM_READABLE));
    g_object_interface_install_property (settings_interface,
					 g_param_spec_int ("titlebar-double-click-action",
							   "Titlebar Action Double Click",
							   "Titlebar Action Double Click",
							   CLICK_ACTION_NONE,
							   CLICK_ACTION_MENU,
							   CLICK_ACTION_MAXIMIZE,
							   G_PARAM_READABLE));
    g_object_interface_install_property (settings_interface,
					 g_param_spec_int ("titlebar-middle-click-action",
							   "Titlebar Action Middle Click",
							   "Titlebar Action Middle Click",
							   CLICK_ACTION_NONE,
							   CLICK_ACTION_MENU,
							   CLICK_ACTION_LOWER,
							   G_PARAM_READABLE));
    g_object_interface_install_property (settings_interface,
					 g_param_spec_int ("titlebar-right-click-action",
							   "Titlebar Action Right Click",
							   "Titlebar Action Right Click",
							   CLICK_ACTION_NONE,
							   CLICK_ACTION_MENU,
							   CLICK_ACTION_MENU,
							   G_PARAM_READABLE));
    g_object_interface_install_property (settings_interface,
					 g_param_spec_int ("mouse-wheel-action",
							   "Mouse Wheel Action",
							   "Mouse Wheel Action",
							   WHEEL_ACTION_NONE,
							   WHEEL_ACTION_SHADE,
							   WHEEL_ACTION_SHADE,
							   G_PARAM_READABLE));
    g_object_interface_install_property (settings_interface,
					 g_param_spec_string ("titlebar-font",
							      "Titlebar Font",
							      "Titlebar Font",
							      "Sans 12",
							      G_PARAM_READABLE));
}
