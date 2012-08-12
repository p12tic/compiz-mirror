/**
 *
 * compizconfig gnome integration backend
 *
 * gnome-integration.c
 *
 * Copyright (c) 2011 Canonical Ltd
 *
 * Based on the original compizconfig-backend-gconf
 *
 * Copyright (c) 2007 Danny Baumann <maniac@opencompositing.org>
 *
 * Parts of this code are taken from libberylsettings 
 * gconf backend, written by:
 *
 * Copyright (c) 2006 Robert Carr <racarr@opencompositing.org>
 * Copyright (c) 2007 Dennis Kasprzyk <onestone@opencompositing.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authored By:
 *	Sam Spilsbury <sam.spilsbury@canonical.com>
 *
 **/
#ifdef USE_GCONF

#define METACITY "/apps/metacity"
#define NUM_WATCHED_DIRS 3

#include <stdlib.h>
#include <string.h>
#include <ccs.h>
#include <ccs-backend.h>
#include <ccs-object.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>
#include "ccs_gnome_integration.h"

typedef struct _SpecialOptionGConf {
    const char*       settingName;
    const char*       pluginName;
    Bool	      screen;
    const char*       gnomeName;
    SpecialOptionType type;
} SpecialOptionGConf;

static const SpecialOptionGConf specialOptions[] = {
    {"run_key", "gnomecompat", FALSE,
     METACITY "/global_keybindings/panel_run_dialog", OptionKey},
    {"main_menu_key", "gnomecompat", FALSE,
     METACITY "/global_keybindings/panel_main_menu", OptionKey},
    {"run_command_screenshot_key", "gnomecompat", FALSE,
     METACITY "/global_keybindings/run_command_screenshot", OptionKey},
    {"run_command_window_screenshot_key", "gnomecompat", FALSE,
     METACITY "/global_keybindings/run_command_window_screenshot", OptionKey},
    {"run_command_terminal_key", "gnomecompat", FALSE,
     METACITY "/global_keybindings/run_command_terminal", OptionKey},

    {"toggle_window_maximized_key", "core", FALSE,
     METACITY "/window_keybindings/toggle_maximized", OptionKey},
    {"minimize_window_key", "core", FALSE,
     METACITY "/window_keybindings/minimize", OptionKey},
    {"maximize_window_key", "core", FALSE,
     METACITY "/window_keybindings/maximize", OptionKey},
    {"unmaximize_window_key", "core", FALSE,
     METACITY "/window_keybindings/unmaximize", OptionKey},
    {"maximize_window_horizontally_key", "core", FALSE,
     METACITY "/window_keybindings/maximize_horizontally", OptionKey},
    {"maximize_window_vertically_key", "core", FALSE,
     METACITY "/window_keybindings/maximize_vertically", OptionKey},
    {"raise_window_key", "core", FALSE,
     METACITY "/window_keybindings/raise", OptionKey},
    {"lower_window_key", "core", FALSE,
     METACITY "/window_keybindings/lower", OptionKey},
    {"close_window_key", "core", FALSE,
     METACITY "/window_keybindings/close", OptionKey},
    {"toggle_window_shaded_key", "core", FALSE,
     METACITY "/window_keybindings/toggle_shaded", OptionKey},

    {"show_desktop_key", "core", FALSE,
     METACITY "/global_keybindings/show_desktop", OptionKey},

    {"initiate_key", "move", FALSE,
     METACITY "/window_keybindings/begin_move", OptionKey},
    {"initiate_key", "resize", FALSE,
     METACITY "/window_keybindings/begin_resize", OptionKey},
    {"window_menu_key", "core", FALSE,
     METACITY "/window_keybindings/activate_window_menu", OptionKey},

    /* integration of Metacity's mouse_button_modifier option */
    {"initiate_button", "move", FALSE,
     METACITY "/window_keybindings/begin_move", OptionSpecial},
    {"initiate_button", "resize", FALSE,
     METACITY "/window_keybindings/begin_resize", OptionSpecial},
    {"window_menu_button", "core", FALSE,
     METACITY "/window_keybindings/activate_window_menu", OptionSpecial},
    {"mouse_button_modifier", "__special", FALSE,
     METACITY "/general/mouse_button_modifier", OptionSpecial},
    /* integration of the Metacity's option to swap mouse buttons */
    {"resize_with_right_button", "__special", FALSE,
     METACITY "/general/resize_with_right_button", OptionSpecial},

    {"visual_bell", "fade", TRUE,
     METACITY "/general/visual_bell", OptionBool},
    {"fullscreen_visual_bell", "fade", TRUE,
     METACITY "/general/visual_bell_type", OptionSpecial},

    {"next_key", "staticswitcher", FALSE,
     METACITY "/global_keybindings/switch_windows", OptionKey},
    {"prev_key", "staticswitcher", FALSE,
     METACITY "/global_keybindings/switch_windows_backward", OptionKey},

    {"toggle_sticky_key", "extrawm", FALSE,
     METACITY "/window_keybindings/toggle_on_all_workspaces", OptionKey},
    {"toggle_fullscreen_key", "extrawm", FALSE,
     METACITY "/window_keybindings/toggle_fullscreen", OptionKey},

    {"command0", "commands", FALSE,
     METACITY "/keybinding_commands/command_1", OptionString},
    {"command1", "commands", FALSE,
     METACITY "/keybinding_commands/command_2", OptionString},
    {"command2", "commands", FALSE,
     METACITY "/keybinding_commands/command_3", OptionString},
    {"command3", "commands", FALSE,
     METACITY "/keybinding_commands/command_4", OptionString},
    {"command4", "commands", FALSE,
     METACITY "/keybinding_commands/command_5", OptionString},
    {"command5", "commands", FALSE,
     METACITY "/keybinding_commands/command_6", OptionString},
    {"command6", "commands", FALSE,
     METACITY "/keybinding_commands/command_7", OptionString},
    {"command7", "commands", FALSE,
     METACITY "/keybinding_commands/command_8", OptionString},
    {"command8", "commands", FALSE,
     METACITY "/keybinding_commands/command_9", OptionString},
    {"command9", "commands", FALSE,
     METACITY "/keybinding_commands/command_10", OptionString},
    {"command10", "commands", FALSE,
     METACITY "/keybinding_commands/command_11", OptionString},
    {"command11", "commands", FALSE,
     METACITY "/keybinding_commands/command_12", OptionString},

    {"run_command0_key", "commands", FALSE,
     METACITY "/global_keybindings/run_command_1", OptionKey},
    {"run_command1_key", "commands", FALSE,
     METACITY "/global_keybindings/run_command_2", OptionKey},
    {"run_command2_key", "commands", FALSE,
     METACITY "/global_keybindings/run_command_3", OptionKey},
    {"run_command3_key", "commands", FALSE,
     METACITY "/global_keybindings/run_command_4", OptionKey},
    {"run_command4_key", "commands", FALSE,
     METACITY "/global_keybindings/run_command_5", OptionKey},
    {"run_command5_key", "commands", FALSE,
     METACITY "/global_keybindings/run_command_6", OptionKey},
    {"run_command6_key", "commands", FALSE,
     METACITY "/global_keybindings/run_command_7", OptionKey},
    {"run_command7_key", "commands", FALSE,
     METACITY "/global_keybindings/run_command_8", OptionKey},
    {"run_command8_key", "commands", FALSE,
     METACITY "/global_keybindings/run_command_9", OptionKey},
    {"run_command9_key", "commands", FALSE,
     METACITY "/global_keybindings/run_command_10", OptionKey},
    {"run_command10_key", "commands", FALSE,
     METACITY "/global_keybindings/run_command_11", OptionKey},
    {"run_command11_key", "commands", FALSE,
     METACITY "/global_keybindings/run_command_12", OptionKey},

    {"rotate_to_1_key", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_1", OptionKey},
    {"rotate_to_2_key", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_2", OptionKey},
    {"rotate_to_3_key", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_3", OptionKey},
    {"rotate_to_4_key", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_4", OptionKey},
    {"rotate_to_5_key", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_5", OptionKey},
    {"rotate_to_6_key", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_6", OptionKey},
    {"rotate_to_7_key", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_7", OptionKey},
    {"rotate_to_8_key", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_8", OptionKey},
    {"rotate_to_9_key", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_9", OptionKey},
    {"rotate_to_10_key", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_10", OptionKey},
    {"rotate_to_11_key", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_11", OptionKey},
    {"rotate_to_12_key", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_12", OptionKey},

    {"rotate_left_key", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_left", OptionKey},
    {"rotate_right_key", "rotate", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_right", OptionKey},

    {"switch_to_1_key", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_1", OptionKey},
    {"switch_to_2_key", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_2", OptionKey},
    {"switch_to_3_key", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_3", OptionKey},
    {"switch_to_4_key", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_4", OptionKey},
    {"switch_to_5_key", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_5", OptionKey},
    {"switch_to_6_key", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_6", OptionKey},
    {"switch_to_7_key", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_7", OptionKey},
    {"switch_to_8_key", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_8", OptionKey},
    {"switch_to_9_key", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_9", OptionKey},
    {"switch_to_10_key", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_10", OptionKey},
    {"switch_to_11_key", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_11", OptionKey},
    {"switch_to_12_key", "vpswitch", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_12", OptionKey},

    {"up_key", "wall", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_up", OptionKey},
    {"down_key", "wall", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_down", OptionKey},
    {"left_key", "wall", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_left", OptionKey},
    {"right_key", "wall", FALSE,
     METACITY "/global_keybindings/switch_to_workspace_right", OptionKey},
    {"left_window_key", "wall", FALSE,
     METACITY "/window_keybindings/move_to_workspace_left", OptionKey},
    {"right_window_key", "wall", FALSE,
     METACITY "/window_keybindings/move_to_workspace_right", OptionKey},
    {"up_window_key", "wall", FALSE,
     METACITY "/window_keybindings/move_to_workspace_up", OptionKey},
    {"down_window_key", "wall", FALSE,
     METACITY "/window_keybindings/move_to_workspace_down", OptionKey},

    {"put_topleft_key", "put", FALSE,
     METACITY "/window_keybindings/move_to_corner_nw", OptionKey},
    {"put_topright_key", "put", FALSE,
     METACITY "/window_keybindings/move_to_corner_ne", OptionKey},
    {"put_bottomleft_key", "put", FALSE,
     METACITY "/window_keybindings/move_to_corner_sw", OptionKey},
    {"put_bottomright_key", "put", FALSE,
     METACITY "/window_keybindings/move_to_corner_se", OptionKey},
    {"put_left_key", "put", FALSE,
     METACITY "/window_keybindings/move_to_side_w", OptionKey},
    {"put_right_key", "put", FALSE,
     METACITY "/window_keybindings/move_to_side_e", OptionKey},
    {"put_top_key", "put", FALSE,
     METACITY "/window_keybindings/move_to_side_n", OptionKey},
    {"put_bottom_key", "put", FALSE,
     METACITY "/window_keybindings/move_to_side_s", OptionKey},

    {"rotate_to_1_window_key", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_1", OptionKey},
    {"rotate_to_2_window_key", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_2", OptionKey},
    {"rotate_to_3_window_key", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_3", OptionKey},
    {"rotate_to_4_window_key", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_4", OptionKey},
    {"rotate_to_5_window_key", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_5", OptionKey},
    {"rotate_to_6_window_key", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_6", OptionKey},
    {"rotate_to_7_window_key", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_7", OptionKey},
    {"rotate_to_8_window_key", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_8", OptionKey},
    {"rotate_to_9_window_key", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_9", OptionKey},
    {"rotate_to_10_window_key", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_10", OptionKey},
    {"rotate_to_11_window_key", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_11", OptionKey},
    {"rotate_to_12_window_key", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_12", OptionKey},

    {"rotate_left_window_key", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_left", OptionKey},
    {"rotate_right_window_key", "rotate", FALSE,
     METACITY "/window_keybindings/move_to_workspace_right", OptionKey},

    {"command_screenshot", "gnomecompat", FALSE,
     METACITY "/keybinding_commands/command_screenshot", OptionString},
    {"command_window_screenshot", "gnomecompat", FALSE,
     METACITY "/keybinding_commands/command_window_screenshot", OptionString},
    {"command_terminal", "gnomecompat", FALSE,
     "/desktop/gnome/applications/terminal/exec", OptionString},

    {"current_viewport", "thumbnail", TRUE,
     "/apps/panel/applets/window_list/prefs/display_all_workspaces",
     OptionSpecial},

    {"autoraise", "core", FALSE,
     METACITY "/general/auto_raise", OptionBool},
    {"autoraise_delay", "core", FALSE,
     METACITY "/general/auto_raise_delay", OptionInt},
    {"raise_on_click", "core", FALSE,
     METACITY "/general/raise_on_click", OptionBool},
    {"click_to_focus", "core", FALSE,
     METACITY "/general/focus_mode", OptionSpecial},

    {"audible_bell", "core", FALSE,
     METACITY "/general/audible_bell", OptionBool},
    /*{"hsize", "core", TRUE,
     METACITY "/general/num_workspaces", OptionInt},*/
};

static const char* watchedGConfGnomeDirectories[] = {
    METACITY,
    "/desktop/gnome/applications/terminal",
    "/apps/panel/applets/window_list/prefs"
};

const CCSGNOMEIntegratedSettingNames ccsGNOMEIntegratedSettingNames =
{
    { "audible_bell", "audible_bell" },
    { "click_to_focus", "focus_mode" },
    { "raise_on_click", "raise_on_click" },
    { "autoraise_delay", "auto_raise_delay" },
    { "autoraise", "auto_raise" },
    { "current_viewport", "panel" },
    { "command_terminal", "gnome" },
    { "command_window_screenshot", "command_window_screenshot" },
    { "command_screenshot", "command_screenshot" },
    { "rotate_right_window_key", "move_to_workspace_right" },
    { "rotate_left_window_key", "move_to_workspace_left" },
    { "rotate_to_12_window_key", "move_to_workspace_12" },
    { "rotate_to_11_window_key", "move_to_workspace_11" },
    { "rotate_to_10_window_key", "move_to_workspace_10" },
    { "rotate_to_9_window_key", "move_to_workspace_9" },
    { "rotate_to_8_window_key", "move_to_workspace_8" },
    { "rotate_to_7_window_key", "move_to_workspace_7" },
    { "rotate_to_6_window_key", "move_to_workspace_6" },
    { "rotate_to_5_window_key", "move_to_workspace_5" },
    { "rotate_to_4_window_key", "move_to_workspace_4" },
    { "rotate_to_3_window_key", "move_to_workspace_3" },
    { "rotate_to_2_window_key", "move_to_workspace_2" },
    { "rotate_to_1_window_key", "move_to_workspace_1" },
    { "put_bottom_key", "move_to_side_s" },
    { "put_top_key", "move_to_side_n" },
    { "put_right_key", "move_to_side_e" },
    { "put_left_key", "move_to_side_w" },
    { "put_bottomright_key", "move_to_corner_se" },
    { "put_bottomleft_key", "move_to_corner_sw" },
    { "put_topright_key", "move_to_corner_ne" },
    { "put_topleft_key", "move_to_corner_nw" },
    { "down_window_key", "move_to_workspace_down" },
    { "up_window_key", "move_to_workspace_up" },
    { "right_window_key", "move_to_workspace_right" },
    { "left_window_key", "move_to_workspace_left" },
    { "right_key", "switch_to_workspace_right" },
    { "left_key", "switch_to_workspace_left" },
    { "down_key", "switch_to_workspace_down" },
    { "up_key", "switch_to_workspace_up" },
    { "switch_to_12_key", "switch_to_workspace_12" },
    { "switch_to_11_key", "switch_to_workspace_11" },
    { "switch_to_10_key", "switch_to_workspace_10" },
    { "switch_to_9_key", "switch_to_workspace_9" },
    { "switch_to_8_key", "switch_to_workspace_8" },
    { "switch_to_7_key", "switch_to_workspace_7" },
    { "switch_to_6_key", "switch_to_workspace_6" },
    { "switch_to_5_key", "switch_to_workspace_5" },
    { "switch_to_4_key", "switch_to_workspace_4" },
    { "switch_to_3_key", "switch_to_workspace_3" },
    { "switch_to_2_key", "switch_to_workspace_2" },
    { "switch_to_1_key", "switch_to_workspace_1" },
    { "rotate_right_key", "switch_to_workspace_right" },
    { "rotate_left_key", "switch_to_workspace_left" },
    { "rotate_to_12_key", "switch_to_workspace_12" },
    { "rotate_to_11_key", "switch_to_workspace_11" },
    { "rotate_to_10_key", "switch_to_workspace_10" },
    { "rotate_to_9_key", "switch_to_workspace_9" },
    { "rotate_to_8_key", "switch_to_workspace_8" },
    { "rotate_to_7_key", "switch_to_workspace_7" },
    { "rotate_to_6_key", "switch_to_workspace_6" },
    { "rotate_to_5_key", "switch_to_workspace_5" },
    { "rotate_to_4_key", "switch_to_workspace_4" },
    { "rotate_to_3_key", "switch_to_workspace_3" },
    { "rotate_to_2_key", "switch_to_workspace_2" },
    { "rotate_to_1_key", "switch_to_workspace_1" },
    { "run_command11_key", "run_command_12" },
    { "run_command10_key", "run_command_11" },
    { "run_command9_key", "run_command_10" },
    { "run_command8_key", "run_command_9" },
    { "run_command7_key", "run_command_8" },
    { "run_command6_key", "run_command_7" },
    { "run_command5_key", "run_command_6" },
    { "run_command4_key", "run_command_5" },
    { "run_command3_key", "run_command_4" },
    { "run_command2_key", "run_command_3" },
    { "run_command1_key", "run_command_2" },
    { "run_command0_key", "run_command_1" },
    { "command11", "command_12" },
    { "command10", "command_11" },
    { "command9", "command_10" },
    { "command8", "command_9" },
    { "command7", "command_8" },
    { "command6", "command_7" },
    { "command5", "command_6" },
    { "command4", "command_5" },
    { "command3", "command_4" },
    { "command2", "command_3" },
    { "command1", "command_2" },
    { "command0", "command_1" },
    { "toggle_fullscreen_key", "toggle_fullscreen" },
    { "toggle_sticky_key", "toggle_on_all_workspaces" },
    { "prev_key", "switch_windows_backward" },
    { "next_key", "switch_windows" },
    { "fullscreen_visual_bell", "visual_bell_type" },
    { "visual_bell", "visual_bell" },
    { "resize_with_right_button", "resize_with_right_button" },
    { "mouse_button_modifier", "mouse_button_modifier" },
    { "window_menu_button", "activate_window_menu" },
    { "initiate_button", "begin_resize" },
    { "initiate_button", "begin_move" },
    { "window_menu_key", "activate_window_menu" },
    { "initiate_key", "begin_resize" },
    { "initiate_key", "begin_move" },
    { "show_desktop_key", "show_desktop" },
    { "toggle_window_shaded_key", "toggle_shaded" },
    { "close_window_key", "close" },
    { "lower_window_key", "lower" },
    { "raise_window_key", "raise" },
    { "maximize_window_vertically_key", "maximize_vertically" },
    { "maximize_window_horizontally_key", "maximize_horizontally" },
    { "unmaximize_window_key", "unmaximize" },
    { "maximize_window_key", "maximize" },
    { "minimize_window_key", "minimize" },
    { "toggle_window_maximized_key", "toggle_maximized" },
    { "run_command_terminal_key", "run_command_terminal" },
    { "run_command_window_screenshot_key", "run_command_window_screenshot" },
    { "run_command_screenshot_key", "run_command_screenshot" },
    { "main_menu_key", "panel_main_menu" },
    { "run_key", "panel_run_dialog" }
};

const CCSGConfIntegrationCategories ccsGConfIntegrationCategories =
{
    METACITY "/general/",
    "/apps/panel/applets/window_list/prefs/",
    "/desktop/gnome/applications/terminal/",
    METACITY "/keybinding_commands/",
    METACITY "/window_keybindings/",
    METACITY "/global_keybindings/"
};

const CCSGNOMEIntegratedPluginNames ccsGNOMEIntegratedPluginNames =
{
    "core",
    "thumbnail",
    "gnomecompat",
    "rotate",
    "put",
    "wall",
    "vpswitch",
    "commands",
    "extrawm",
    "resize",
    "move",
    "staticswitcher",
    "fade",
    "__special"
};


#define N_SOPTIONS (sizeof (specialOptions) / sizeof (struct _SpecialOptionGConf))

static GHashTable * populateCategoriesHashTables ();
static GHashTable * populateSpecialTypesHashTables ();


typedef struct _CCSGConfIntegrationBackendPrivate CCSGConfIntegrationBackendPrivate;

struct _CCSGConfIntegrationBackendPrivate
{
    CCSBackend *backend;
    CCSContext *context;
    GConfClient *client;
    guint       gnomeGConfNotifyIds[NUM_WATCHED_DIRS];
    CCSIntegratedSettingFactory  *factory;
    CCSIntegratedSettingsStorage *storage;
};

INTERFACE_TYPE (CCSGNOMEIntegratedSettingInterface);

SpecialOptionType
ccsGNOMEIntegratedSettingGetSpecialOptionType (CCSGNOMEIntegratedSetting *setting)
{
    return (*(GET_INTERFACE (CCSGNOMEIntegratedSettingInterface, setting))->getSpecialOptionType) (setting);
}

const char *
ccsGNOMEIntegratedSettingGetGNOMEName (CCSGNOMEIntegratedSetting *setting)
{
    return (*(GET_INTERFACE (CCSGNOMEIntegratedSettingInterface, setting))->getGNOMEName) (setting);
}

/* CCSGNOMEIntegratedSettingDefaultImpl implementation */

typedef struct _CCSGNOMEIntegratedSettingDefaultImplPrivate CCSGNOMEIntegratedSettingDefaultImplPrivate;

struct _CCSGNOMEIntegratedSettingDefaultImplPrivate
{
    SpecialOptionType type;
    const char	      *gnomeName;
    CCSIntegratedSetting *sharedIntegratedSetting;
};

SpecialOptionType
ccsGNOMEIntegratedSettingGetSpecialOptionDefault (CCSGNOMEIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return priv->type;
}

const char *
ccsGNOMEIntegratedSettingGetGNOMENameDefault (CCSGNOMEIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return priv->gnomeName;
}

CCSSettingValue
ccsGNOMEIntegratedSettingReadValue (CCSIntegratedSetting *setting, CCSSettingType type)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingReadValue (priv->sharedIntegratedSetting, type);
}

void
ccsGNOMEIntegratedSettingWriteValue (CCSIntegratedSetting *setting, CCSSettingValue value, CCSSettingType type)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    ccsIntegratedSettingWriteValue (priv->sharedIntegratedSetting, value, type);
}

const char *
ccsGNOMEIntegratedSettingPluginName (CCSIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingPluginName (priv->sharedIntegratedSetting);
}

const char *
ccsGNOMEIntegratedSettingSettingName (CCSIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingSettingName (priv->sharedIntegratedSetting);
}

CCSSettingType
ccsGNOMEIntegratedSettingGetType (CCSIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingGetType (priv->sharedIntegratedSetting);
}

void
ccsGNOMEIntegratedSettingFree (CCSIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    ccsIntegratedSettingUnref (priv->sharedIntegratedSetting);
    ccsObjectFinalize (setting);

    (*setting->object.object_allocation->free_) (setting->object.object_allocation->allocator, setting);
}

CCSGNOMEIntegratedSettingInterface ccsGNOMEIntegratedSettingDefaultImplInterface =
{
    ccsGNOMEIntegratedSettingGetSpecialOptionDefault,
    ccsGNOMEIntegratedSettingGetGNOMENameDefault
};

const CCSIntegratedSettingInterface ccsGNOMEIntegratedSettingInterface =
{
    ccsGNOMEIntegratedSettingReadValue,
    ccsGNOMEIntegratedSettingWriteValue,
    ccsGNOMEIntegratedSettingPluginName,
    ccsGNOMEIntegratedSettingSettingName,
    ccsGNOMEIntegratedSettingGetType,
    ccsGNOMEIntegratedSettingFree
};

CCSGNOMEIntegratedSetting *
ccsGNOMEIntegratedSettingNew (CCSIntegratedSetting *base,
			      SpecialOptionType    type,
			      const char	   *gnomeName,
			      CCSObjectAllocationInterface *ai)
{
    CCSGNOMEIntegratedSetting *setting = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGNOMEIntegratedSetting));

    if (!setting)
	return NULL;

    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGNOMEIntegratedSettingDefaultImplPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, setting);
	return NULL;
    }

    priv->sharedIntegratedSetting = base;
    priv->gnomeName = gnomeName;
    priv->type = type;

    ccsObjectInit (setting, ai);
    ccsObjectSetPrivate (setting, (CCSPrivate *) priv);
    ccsObjectAddInterface (setting, (const CCSInterface *) &ccsGNOMEIntegratedSettingInterface, GET_INTERFACE_TYPE (CCSIntegratedSettingInterface));
    ccsObjectAddInterface (setting, (const CCSInterface *) &ccsGNOMEIntegratedSettingDefaultImplInterface, GET_INTERFACE_TYPE (CCSGNOMEIntegratedSettingInterface));
    ccsIntegratedSettingRef ((CCSIntegratedSetting *) setting);

    return setting;
}

/* CCSGConfIntegratedSetting implementation */
typedef struct _CCSGConfIntegratedSettingPrivate CCSGConfIntegratedSettingPrivate;

struct _CCSGConfIntegratedSettingPrivate
{
    CCSGNOMEIntegratedSetting *gnomeIntegratedSetting;
    GConfClient		      *client;
    const char		      *sectionName;
};

SpecialOptionType
ccsGConfIntegratedSettingGetSpecialOptionType (CCSGNOMEIntegratedSetting *setting)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsGNOMEIntegratedSettingGetSpecialOptionType (priv->gnomeIntegratedSetting);
}

const char *
ccsGConfIntegratedSettingGetGNOMEName (CCSGNOMEIntegratedSetting *setting)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsGNOMEIntegratedSettingGetGNOMEName (priv->gnomeIntegratedSetting);
}

CCSSettingValue
ccsGConfIntegratedSettingReadValue (CCSIntegratedSetting *setting, CCSSettingType type)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);
    CCSSettingValue		     value;

    memset (&value, 0, sizeof (CCSSettingValue));

    GConfValue *gconfValue;
    GError     *err = NULL;

    gconfValue = gconf_client_get (priv->client,
				   ccsGNOMEIntegratedSettingGetGNOMEName (priv->gnomeIntegratedSetting),
				   &err);

    if (err)
    {
	g_error_free (err);
	ccsError ("error encountered while reading GConf setting");
	return value;
    }

    if (!gconfValue)
    {
	ccsError ("NULL encountered while reading GConf setting");
	return value;
    }

    switch (type)
    {
	case TypeInt:
	    if (gconfValue->type != GCONF_VALUE_INT)
	    {
		ccsError ("Expected string value");
		break;
	    }

	    value.value.asInt = gconf_value_get_int (gconfValue);
	    break;
	case TypeBool:
	    if (gconfValue->type != GCONF_VALUE_BOOL)
	    {
		ccsError ("Expected string value");
		break;
	    }

	    value.value.asBool = gconf_value_get_bool (gconfValue) ? TRUE : FALSE;
	    break;
	case TypeString:
	    if (gconfValue->type != GCONF_VALUE_STRING)
	    {
		ccsError ("Expected string value");
		break;
	    }

	    const char *str = gconf_value_get_string (gconfValue);

	    value.value.asString = strdup (str ? str : "");
	    break;
	default:
	    ccsError ("unreachable case hit?");
    }

    gconf_value_free (gconfValue);

    ccsInfo ("called ccsGConfIntegratedSettingReadValue!\n");

    return value;
}

void
ccsGConfIntegratedSettingWriteValue (CCSIntegratedSetting *setting, CCSSettingValue value, CCSSettingType type)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    // XXX

    ccsInfo ("called ccsGConfIntegratedSettingWriteValue!\n");

    return ccsIntegratedSettingWriteValue ((CCSIntegratedSetting *) priv->gnomeIntegratedSetting, value, type);
}

const char *
ccsGConfIntegratedSettingPluginName (CCSIntegratedSetting *setting)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingPluginName ((CCSIntegratedSetting *) priv->gnomeIntegratedSetting);
}

const char *
ccsGConfIntegratedSettingSettingName (CCSIntegratedSetting *setting)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingSettingName ((CCSIntegratedSetting *) priv->gnomeIntegratedSetting);
}

CCSSettingType
ccsGConfIntegratedSettingGetType (CCSIntegratedSetting *setting)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingGetType ((CCSIntegratedSetting *) priv->gnomeIntegratedSetting);
}

void
ccsGConfIntegratedSettingFree (CCSIntegratedSetting *setting)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    if (priv->client)
	g_object_unref (priv->client);

    ccsIntegratedSettingUnref ((CCSIntegratedSetting *) priv->gnomeIntegratedSetting);
    ccsObjectFinalize (setting);

    (*setting->object.object_allocation->free_) (setting->object.object_allocation->allocator, setting);
}

const CCSGNOMEIntegratedSettingInterface ccsGConfGNOMEIntegratedSettingInterface =
{
    ccsGConfIntegratedSettingGetSpecialOptionType,
    ccsGConfIntegratedSettingGetGNOMEName
};

const CCSIntegratedSettingInterface ccsGConfIntegratedSettingInterface =
{
    ccsGConfIntegratedSettingReadValue,
    ccsGConfIntegratedSettingWriteValue,
    ccsGConfIntegratedSettingPluginName,
    ccsGConfIntegratedSettingSettingName,
    ccsGConfIntegratedSettingGetType,
    ccsGConfIntegratedSettingFree
};

CCSIntegratedSetting *
ccsGConfIntegratedSettingNew (CCSGNOMEIntegratedSetting *base,
			      GConfClient		*client,
			      const char		*section,
			      CCSObjectAllocationInterface *ai)
{
    CCSIntegratedSetting *setting = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSIntegratedSetting));

    if (!setting)
	return NULL;

    CCSGConfIntegratedSettingPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGConfIntegratedSettingPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, priv);
	return NULL;
    }

    priv->gnomeIntegratedSetting = base;
    priv->client = (GConfClient *) g_object_ref (client);
    priv->sectionName = section;

    ccsObjectInit (setting, ai);
    ccsObjectSetPrivate (setting, (CCSPrivate *) priv);
    ccsObjectAddInterface (setting, (const CCSInterface *) &ccsGConfIntegratedSettingInterface, GET_INTERFACE_TYPE (CCSIntegratedSettingInterface));
    ccsObjectAddInterface (setting, (const CCSInterface *) &ccsGConfGNOMEIntegratedSettingInterface, GET_INTERFACE_TYPE (CCSGNOMEIntegratedSettingInterface));
    ccsIntegratedSettingRef (setting);

    return setting;
}

typedef struct _CCSGConfIntegratedSettingFactoryPrivate CCSGConfIntegratedSettingFactoryPrivate;

struct _CCSGConfIntegratedSettingFactoryPrivate
{
    GConfClient *client;
    GHashTable  *pluginsToSettingsSectionsHashTable;
    GHashTable  *pluginsToSettingsSpecialTypesHashTable;
};

static CCSIntegratedSetting *
createNewGConfIntegratedSetting (GConfClient *client,
				 const char  *sectionName,
				 const char  *gnomeName,
				 const char  *pluginName,
				 const char  *settingName,
				 CCSSettingType type,
				 SpecialOptionType specialOptionType,
				 CCSObjectAllocationInterface *ai)
{
    CCSIntegratedSetting *sharedIntegratedSetting = ccsSharedIntegratedSettingNew (pluginName, settingName, type, ai);

    if (!sharedIntegratedSetting)
	return NULL;

    CCSGNOMEIntegratedSetting *gnomeIntegratedSetting = ccsGNOMEIntegratedSettingNew (sharedIntegratedSetting, specialOptionType, gnomeName, ai);

    if (!gnomeIntegratedSetting)
    {
	ccsIntegratedSettingUnref (sharedIntegratedSetting);
	return NULL;
    }

    CCSIntegratedSetting *gconfIntegratedSetting = ccsGConfIntegratedSettingNew (gnomeIntegratedSetting, client, sectionName, ai);

    if (!gconfIntegratedSetting)
    {
	ccsIntegratedSettingUnref ((CCSIntegratedSetting *) gnomeIntegratedSetting);
	return NULL;
    }

    return gconfIntegratedSetting;
}

CCSIntegratedSetting *
ccsGConfIntegratedSettingFactoryCreateIntegratedSettingForCCSSettingNameAndType (CCSIntegratedSettingFactory *factory,
										 const char		     *integratedName,
										 const char		     *pluginName,
										 const char		     *settingName,
										 CCSSettingType		     type)
{
    CCSGConfIntegratedSettingFactoryPrivate *priv = (CCSGConfIntegratedSettingFactoryPrivate *) ccsObjectGetPrivate (factory);
    GHashTable                              *settingsSectionsHashTable = g_hash_table_lookup (priv->pluginsToSettingsSectionsHashTable, pluginName);
    GHashTable                              *settingsSpecialTypesHashTable = g_hash_table_lookup (priv->pluginsToSettingsSpecialTypesHashTable, pluginName);

    if (settingsSectionsHashTable &&
	settingsSpecialTypesHashTable)
    {
	const gchar *sectionName = g_hash_table_lookup (settingsSectionsHashTable, settingName);
	SpecialOptionType specialType = (SpecialOptionType) GPOINTER_TO_INT (g_hash_table_lookup (settingsSpecialTypesHashTable, settingName));

	return createNewGConfIntegratedSetting (priv->client,
						sectionName,
						integratedName,
						pluginName,
						settingName,
						type,
						specialType,
						factory->object.object_allocation);
    }


    return NULL;
}

const CCSIntegratedSettingFactoryInterface ccsGConfIntegratedSettingFactoryInterface =
{
    ccsGConfIntegratedSettingFactoryCreateIntegratedSettingForCCSSettingNameAndType
};

CCSIntegratedSettingFactory *
ccsGConfIntegratedSettingFactoryNew (GConfClient		  *client,
				     CCSObjectAllocationInterface *ai)
{
    CCSIntegratedSettingFactory *factory = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSIntegratedSettingFactory));

    if (!factory)
	return NULL;

    CCSGConfIntegratedSettingFactoryPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGConfIntegratedSettingFactoryPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, factory);
	return NULL;
    }

    priv->client = (GConfClient *) g_object_ref (client);
    priv->pluginsToSettingsSectionsHashTable = populateCategoriesHashTables ();
    priv->pluginsToSettingsSpecialTypesHashTable = populateSpecialTypesHashTables ();

    ccsObjectInit (factory, ai);
    ccsObjectSetPrivate (factory, (CCSPrivate *) priv);
    ccsObjectAddInterface (factory, (const CCSInterface *) &ccsGConfIntegratedSettingFactoryInterface, GET_INTERFACE_TYPE (CCSIntegratedSettingFactoryInterface));

    return factory;
}

void
ccsGConfIntegratedSettingFactoryFree (CCSIntegratedSettingFactory *factory)
{
    CCSGConfIntegratedSettingFactoryPrivate *priv = (CCSGConfIntegratedSettingFactoryPrivate *) ccsObjectGetPrivate (factory);

    if (priv->client)
	g_object_unref (priv->client);

    if (priv->pluginsToSettingsSectionsHashTable)
	g_hash_table_unref (priv->pluginsToSettingsSectionsHashTable);

    if (priv->pluginsToSettingsSpecialTypesHashTable)
	g_hash_table_unref (priv->pluginsToSettingsSpecialTypesHashTable);

    ccsObjectFinalize (factory);
    (*factory->object.object_allocation->free_) (factory->object.object_allocation->allocator, factory);
}

static CCSSetting *
findDisplaySettingForPlugin (CCSContext *context,
			     const char *plugin,
			     const char *setting)
{
    CCSPlugin  *p;
    CCSSetting *s;

    p = ccsFindPlugin (context, plugin);
    if (!p)
	return NULL;

    s = ccsFindSetting (p, setting);
    if (!s)
	return NULL;

    return s;
}

static void destroyHashTableInternal (void *ht)
{
    g_hash_table_unref ((GHashTable *) ht);
}

static GHashTable *
populateCategoriesHashTables ()
{
    GHashTable *masterHashTable = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, destroyHashTableInternal);
    GHashTable *coreHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *thumbnailHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *gnomecompatHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *rotateHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *putHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *wallHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *vpswitchHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *commandsHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *extrawmHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *resizeHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *moveHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *staticswitcherHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *fadeHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *specialHashTable = g_hash_table_new (g_str_hash, g_str_equal);

    const CCSGNOMEIntegratedSettingNames *names = &ccsGNOMEIntegratedSettingNames;
    const CCSGConfIntegrationCategories  *categories = &ccsGConfIntegrationCategories;
    const CCSGNOMEIntegratedPluginNames  *plugins = &ccsGNOMEIntegratedPluginNames;

    g_hash_table_insert (masterHashTable, (gpointer) plugins->CORE, coreHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->THUMBNAIL, thumbnailHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->GNOMECOMPAT, gnomecompatHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->ROTATE, rotateHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->PUT, putHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->WALL, wallHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->VPSWITCH, vpswitchHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->COMMANDS, commandsHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->EXTRAWM, extrawmHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->STATICSWITCHER, staticswitcherHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->FADE, fadeHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->RESIZE, resizeHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->MOVE, moveHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->SPECIAL, specialHashTable);

    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_AUDIBLE_BELL.compizName, (gpointer) categories->GENERAL);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_CLICK_TO_FOCUS.compizName, (gpointer) categories->GENERAL);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_RAISE_ON_CLICK.compizName, (gpointer) categories->GENERAL);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_AUTORAISE_DELAY.compizName, (gpointer) categories->GENERAL);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_AUTORAISE.compizName, (gpointer) categories->GENERAL);
    g_hash_table_insert (thumbnailHashTable, (gpointer) names->THUMBNAIL_CURRENT_VIEWPORT.compizName, (gpointer) categories->APPS);
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_COMMAND_TERMINAL.compizName, (gpointer) categories->DESKTOP);
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_COMMAND_WINDOW_SCREENSHOT.compizName, (gpointer) categories->KEYBINDING_COMMANDS);
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_COMMAND_SCREENSHOT.compizName, (gpointer) categories->KEYBINDING_COMMANDS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_RIGHT_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_LEFT_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_12_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_11_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_10_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_9_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_8_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_7_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_6_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_5_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_4_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_3_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_2_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_1_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_BOTTOM_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_TOP_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_RIGHT_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_LEFT_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_BOTTOMRIGHT_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_BOTTOMLEFT_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_TOPRIGHT_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_TOPLEFT_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_DOWN_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_UP_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_RIGHT_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_LEFT_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_RIGHT_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_LEFT_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_DOWN_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_UP_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_12_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_11_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_10_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_9_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_8_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_7_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_6_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_5_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_4_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_3_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_2_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_1_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_RIGHT_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_LEFT_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_12_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_11_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_10_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_9_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_8_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_7_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_6_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_5_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_4_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_3_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_2_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_1_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND11_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND10_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND9_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND8_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND7_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND6_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND5_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND4_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND3_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND2_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND1_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND0_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND11.compizName, (gpointer) categories->KEYBINDING_COMMANDS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND10.compizName, (gpointer) categories->KEYBINDING_COMMANDS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND9.compizName, (gpointer) categories->KEYBINDING_COMMANDS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND8.compizName, (gpointer) categories->KEYBINDING_COMMANDS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND7.compizName, (gpointer) categories->KEYBINDING_COMMANDS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND6.compizName, (gpointer) categories->KEYBINDING_COMMANDS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND5.compizName, (gpointer) categories->KEYBINDING_COMMANDS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND4.compizName, (gpointer) categories->KEYBINDING_COMMANDS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND3.compizName, (gpointer) categories->KEYBINDING_COMMANDS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND2.compizName, (gpointer) categories->KEYBINDING_COMMANDS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND1.compizName, (gpointer) categories->KEYBINDING_COMMANDS);
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND0.compizName, (gpointer) categories->KEYBINDING_COMMANDS);
    g_hash_table_insert (extrawmHashTable, (gpointer) names->EXTRAWM_TOGGLE_FULLSCREEN_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (extrawmHashTable, (gpointer) names->EXTRAWM_TOGGLE_STICKY_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (staticswitcherHashTable, (gpointer) names->STATICSWITCHER_PREV_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (staticswitcherHashTable, (gpointer) names->STATICSWITCHER_NEXT_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (fadeHashTable, (gpointer) names->FADE_FULLSCREEN_VISUAL_BELL.compizName, (gpointer) categories->GENERAL);
    g_hash_table_insert (fadeHashTable, (gpointer) names->FADE_VISUAL_BELL.compizName, (gpointer) categories->GENERAL);
    g_hash_table_insert (specialHashTable, (gpointer) names->NULL_RESIZE_WITH_RIGHT_BUTTON.compizName, (gpointer) categories->GENERAL);
    g_hash_table_insert (specialHashTable, (gpointer) names->NULL_MOUSE_BUTTON_MODIFIER.compizName, (gpointer) categories->GENERAL);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_WINDOW_MENU_BUTTON.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (resizeHashTable, (gpointer) names->RESIZE_INITIATE_BUTTON.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (moveHashTable, (gpointer) names->MOVE_INITIATE_BUTTON.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_WINDOW_MENU_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (resizeHashTable, (gpointer) names->RESIZE_INITIATE_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (moveHashTable, (gpointer) names->MOVE_INITIATE_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_SHOW_DESKTOP_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_TOGGLE_WINDOW_SHADED_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_CLOSE_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_LOWER_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_RAISE_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_MAXIMIZE_WINDOW_VERTICALLY_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_MAXIMIZE_WINDOW_HORIZONTALLY_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_UNMAXIMIZE_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_MAXIMIZE_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_MINIMIZE_WINDOW_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_TOGGLE_WINDOW_MAXIMIZED_KEY.compizName, (gpointer) categories->WINDOW_KEYBINDINGS);
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_RUN_COMMAND_TERMINAL_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_RUN_COMMAND_WINDOW_SCREENSHOT_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_RUN_COMMAND_SCREENSHOT_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_MAIN_MENU_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_RUN_KEY.compizName, (gpointer) categories->GLOBAL_KEYBINDINGS);

    return masterHashTable;
}


static GHashTable *
populateSpecialTypesHashTables ()
{
    GHashTable *masterHashTable = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, destroyHashTableInternal);
    GHashTable *coreHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *thumbnailHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *gnomecompatHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *rotateHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *putHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *wallHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *vpswitchHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *commandsHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *extrawmHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *resizeHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *moveHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *staticswitcherHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *fadeHashTable = g_hash_table_new (g_str_hash, g_str_equal);
    GHashTable *specialHashTable = g_hash_table_new (g_str_hash, g_str_equal);

    const CCSGNOMEIntegratedSettingNames *names = &ccsGNOMEIntegratedSettingNames;
    const CCSGNOMEIntegratedPluginNames  *plugins = &ccsGNOMEIntegratedPluginNames;

    g_hash_table_insert (masterHashTable, (gpointer) plugins->CORE, coreHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->THUMBNAIL, thumbnailHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->GNOMECOMPAT, gnomecompatHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->ROTATE, rotateHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->PUT, putHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->WALL, wallHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->VPSWITCH, vpswitchHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->COMMANDS, commandsHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->EXTRAWM, extrawmHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->STATICSWITCHER, staticswitcherHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->FADE, fadeHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->RESIZE, resizeHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->MOVE, moveHashTable);
    g_hash_table_insert (masterHashTable, (gpointer) plugins->SPECIAL, specialHashTable);

    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_AUDIBLE_BELL.compizName, GINT_TO_POINTER (OptionBool));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_CLICK_TO_FOCUS.compizName, GINT_TO_POINTER (OptionSpecial));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_RAISE_ON_CLICK.compizName, GINT_TO_POINTER (OptionBool));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_AUTORAISE_DELAY.compizName, GINT_TO_POINTER (OptionInt));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_AUTORAISE.compizName, GINT_TO_POINTER (OptionBool));
    g_hash_table_insert (thumbnailHashTable, (gpointer) names->THUMBNAIL_CURRENT_VIEWPORT.compizName, GINT_TO_POINTER (OptionSpecial));
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_COMMAND_TERMINAL.compizName, GINT_TO_POINTER (OptionString));
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_COMMAND_WINDOW_SCREENSHOT.compizName, GINT_TO_POINTER (OptionString));
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_COMMAND_SCREENSHOT.compizName, GINT_TO_POINTER (OptionString));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_RIGHT_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_LEFT_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_12_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_11_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_10_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_9_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_8_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_7_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_6_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_5_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_4_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_3_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_2_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_1_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_BOTTOM_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_TOP_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_RIGHT_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_LEFT_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_BOTTOMRIGHT_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_BOTTOMLEFT_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_TOPRIGHT_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (putHashTable, (gpointer) names->PUT_PUT_TOPLEFT_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_DOWN_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_UP_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_RIGHT_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_LEFT_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_RIGHT_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_LEFT_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_DOWN_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (wallHashTable, (gpointer) names->WALL_UP_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_12_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_11_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_10_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_9_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_8_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_7_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_6_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_5_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_4_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_3_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_2_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (vpswitchHashTable, (gpointer) names->VPSWITCH_SWITCH_TO_1_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_RIGHT_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_LEFT_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_12_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_11_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_10_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_9_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_8_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_7_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_6_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_5_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_4_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_3_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_2_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (rotateHashTable, (gpointer) names->ROTATE_ROTATE_TO_1_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND11_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND10_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND9_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND8_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND7_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND6_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND5_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND4_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND3_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND2_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND1_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_RUN_COMMAND0_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND11.compizName, GINT_TO_POINTER (OptionString));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND10.compizName, GINT_TO_POINTER (OptionString));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND9.compizName, GINT_TO_POINTER (OptionString));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND8.compizName, GINT_TO_POINTER (OptionString));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND7.compizName, GINT_TO_POINTER (OptionString));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND6.compizName, GINT_TO_POINTER (OptionString));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND5.compizName, GINT_TO_POINTER (OptionString));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND4.compizName, GINT_TO_POINTER (OptionString));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND3.compizName, GINT_TO_POINTER (OptionString));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND2.compizName, GINT_TO_POINTER (OptionString));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND1.compizName, GINT_TO_POINTER (OptionString));
    g_hash_table_insert (commandsHashTable, (gpointer) names->COMMANDS_COMMAND0.compizName, GINT_TO_POINTER (OptionString));
    g_hash_table_insert (extrawmHashTable, (gpointer) names->EXTRAWM_TOGGLE_FULLSCREEN_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (extrawmHashTable, (gpointer) names->EXTRAWM_TOGGLE_STICKY_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (staticswitcherHashTable, (gpointer) names->STATICSWITCHER_PREV_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (staticswitcherHashTable, (gpointer) names->STATICSWITCHER_NEXT_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (fadeHashTable, (gpointer) names->FADE_FULLSCREEN_VISUAL_BELL.compizName, GINT_TO_POINTER (OptionSpecial));
    g_hash_table_insert (fadeHashTable, (gpointer) names->FADE_VISUAL_BELL.compizName, GINT_TO_POINTER (OptionBool));
    g_hash_table_insert (specialHashTable, (gpointer) names->NULL_RESIZE_WITH_RIGHT_BUTTON.compizName, GINT_TO_POINTER (OptionSpecial));
    g_hash_table_insert (specialHashTable, (gpointer) names->NULL_MOUSE_BUTTON_MODIFIER.compizName, GINT_TO_POINTER (OptionSpecial));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_WINDOW_MENU_BUTTON.compizName, GINT_TO_POINTER (OptionSpecial));
    g_hash_table_insert (resizeHashTable, (gpointer) names->RESIZE_INITIATE_BUTTON.compizName, GINT_TO_POINTER (OptionSpecial));
    g_hash_table_insert (moveHashTable, (gpointer) names->MOVE_INITIATE_BUTTON.compizName, GINT_TO_POINTER (OptionSpecial));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_WINDOW_MENU_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (resizeHashTable, (gpointer) names->RESIZE_INITIATE_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (moveHashTable, (gpointer) names->MOVE_INITIATE_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_SHOW_DESKTOP_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_TOGGLE_WINDOW_SHADED_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_CLOSE_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_LOWER_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_RAISE_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_MAXIMIZE_WINDOW_VERTICALLY_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_MAXIMIZE_WINDOW_HORIZONTALLY_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_UNMAXIMIZE_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_MAXIMIZE_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_MINIMIZE_WINDOW_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (coreHashTable, (gpointer) names->CORE_TOGGLE_WINDOW_MAXIMIZED_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_RUN_COMMAND_TERMINAL_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_RUN_COMMAND_WINDOW_SCREENSHOT_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_RUN_COMMAND_SCREENSHOT_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_MAIN_MENU_KEY.compizName, GINT_TO_POINTER (OptionKey));
    g_hash_table_insert (gnomecompatHashTable, (gpointer) names->GNOMECOMPAT_RUN_KEY.compizName, GINT_TO_POINTER (OptionKey));

    return masterHashTable;
}


static CCSIntegratedSetting *
ccsGConfIntegrationBackendGetIntegratedOptionIndex (CCSIntegration *integration,
						    const char		  *pluginName,
						    const char		  *settingName,
						    int			  *index)
{
    unsigned int i;

    CCSGConfIntegrationBackendPrivate *priv = (CCSGConfIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);
    CCSIntegratedSettingList integratedSettings = ccsIntegratedSettingsStorageFindMatchingSettings (priv->storage,
												    pluginName,
												    settingName);

    for (i = 0; i < N_SOPTIONS; i++)
    {
	const SpecialOptionGConf *opt = &specialOptions[i];

	if (strcmp (settingName, opt->settingName) != 0)
	    continue;

	if (pluginName)
	{
	    if (!opt->pluginName)
		continue;
	    if (strcmp (pluginName, opt->pluginName) != 0)
		continue;
	}
	else
	{
	    if (opt->pluginName)
		continue;
	}

	CCSIntegratedSettingList iter = integratedSettings;

	while (iter)
	{
	    g_print ("would update %p\n", ccsIntegratedSettingSettingName (iter->data));
	    iter = iter->next;
	}

	if (index)
	    *index = i;

	return integratedSettings->data;
    }

    if (index)
	*index = -1;

    return NULL;
}

static void
gnomeGConfValueChanged (GConfClient *client,
			guint       cnxn_id,
			GConfEntry  *entry,
			gpointer    user_data)
{
    CCSIntegration *integration = (CCSIntegration *)user_data;
    CCSGConfIntegrationBackendPrivate *priv = (CCSGConfIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);
    char       *keyName = (char*) gconf_entry_get_key (entry);
    int        i, last = 0, num = 0;
    Bool       needInit = TRUE;

    /* We don't care if integration is not enabled */
    if (!ccsGetIntegrationEnabled (priv->context))
	return;

    /* we have to loop multiple times here, because one Gnome
       option may be integrated with multiple Compiz options */
    while (1)
    {
	for (i = last, num = -1; i < N_SOPTIONS; i++)
	{
	    if (strcmp (specialOptions[i].gnomeName, keyName) == 0)
	    {
		num = i;
		last = i + 1;
		break;
	    }
	}

	if (num < 0)
	    break;

	if ((strcmp (specialOptions[num].settingName,
		     "mouse_button_modifier") == 0) ||
	    (strcmp (specialOptions[num].settingName,
		    "resize_with_right_button") == 0))
	{
	    CCSSetting *s;

	    if (needInit)
	    {
		ccsBackendReadInit (priv->backend, priv->context);
		needInit = FALSE;
	    }

	    s = findDisplaySettingForPlugin (priv->context, "core",
					     "window_menu_button");
	    if (s)
		ccsBackendReadSetting (priv->backend, priv->context, s);

	    s = findDisplaySettingForPlugin (priv->context, "move",
					     "initiate_button");
	    if (s)
		ccsBackendReadSetting (priv->backend, priv->context, s);

	    s = findDisplaySettingForPlugin (priv->context, "resize",
					     "initiate_button");
	    if (s)
		ccsBackendReadSetting (priv->backend, priv->context, s);
	}
	else
	{
	    CCSPlugin     *plugin = NULL;
	    CCSSetting    *setting;
	    SpecialOptionGConf *opt = (SpecialOptionGConf *) &specialOptions[num];

	    plugin = ccsFindPlugin (priv->context, (char*) opt->pluginName);
	    if (plugin)
	    {
		for (i = 0; i < 1; i++)
		{
		    setting = ccsFindSetting (plugin, (char*) opt->settingName);

		    if (setting)
		    {
			if (needInit)
			{
			    ccsBackendReadInit (priv->backend, priv->context);
			    needInit = FALSE;
			}
			ccsBackendReadSetting (priv->backend, priv->context, setting);
		    }

		    /* do not read display settings multiple
		       times for multiscreen environments */
		}
	    }
	}
    }
}

static void
finiGConfClient (CCSIntegration *integration)
{
    int i;

    CCSGConfIntegrationBackendPrivate *priv = (CCSGConfIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);

    if (priv->client)
    {
	gconf_client_clear_cache (priv->client);

	for (i = 0; i < NUM_WATCHED_DIRS; i++)
	{
	    if (priv->gnomeGConfNotifyIds[i])
	    {
		gconf_client_notify_remove (priv->client, priv->gnomeGConfNotifyIds[0]);
		priv->gnomeGConfNotifyIds[i] = 0;
	    }
	    gconf_client_remove_dir (priv->client, watchedGConfGnomeDirectories[i], NULL);
	}
	gconf_client_suggest_sync (priv->client, NULL);

	g_object_unref (priv->client);
	priv->client = NULL;
    }
}

static void
registerGConfClient (CCSIntegration *integration,
		     GConfClient    *client)
{
    int i;

    CCSGConfIntegrationBackendPrivate *priv = (CCSGConfIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);

    for (i = 0; i < NUM_WATCHED_DIRS; i++)
    {
	priv->gnomeGConfNotifyIds[i] = gconf_client_notify_add (client,
								watchedGConfGnomeDirectories[i],
								gnomeGConfValueChanged, integration,
								NULL, NULL);
	gconf_client_add_dir (client, watchedGConfGnomeDirectories[i],
			      GCONF_CLIENT_PRELOAD_NONE, NULL);
    }
}

static void
initGConfClient (CCSIntegration *integration)
{
    CCSGConfIntegrationBackendPrivate *priv = (CCSGConfIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);

    if (priv->client)
	finiGConfClient (integration);

    priv->client = gconf_client_get_default ();

    registerGConfClient (integration, priv->client);
}

static unsigned int
getGnomeMouseButtonModifier (CCSIntegratedSetting *mouseButtonModifierSetting)
{
    unsigned int modMask = 0;
    CCSSettingValue v = ccsIntegratedSettingReadValue (mouseButtonModifierSetting, TypeString);

    g_print ("got: %s\n", v.value.asString);

    modMask = ccsStringToModifiers (v.value.asString);

    free (v.value.asString);

    return modMask;
}

static unsigned int
getButtonBindingForSetting (CCSContext   *context,
			    const char   *plugin,
			    const char   *setting)
{
    CCSSetting *s;

    s = findDisplaySettingForPlugin (context, plugin, setting);
    if (!s)
	return 0;

    if (ccsSettingGetType (s) != TypeButton)
	return 0;

    return ccsSettingGetValue (s)->value.asButton.button;
}

static Bool
ccsGConfIntegrationBackendReadOptionIntoSetting (CCSIntegration *integration,
						 CCSContext	       *context,
						 CCSSetting	       *setting,
						 CCSIntegratedSetting   *integratedSetting,
						 int		       index)
{
    Bool       ret = FALSE;

    CCSGConfIntegrationBackendPrivate *priv = (CCSGConfIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);
    
    if (!priv->client)
	initGConfClient (integration);

    ret = ccsSettingIsReadableByBackend (setting);

    if (!ret)
	return FALSE;

    switch (ccsGNOMEIntegratedSettingGetSpecialOptionType ((CCSGNOMEIntegratedSetting *) integratedSetting)) {
    case OptionInt:
	{
	    ccsSetInt (setting, ccsIntegratedSettingReadValue (integratedSetting, TypeInt).value.asInt, TRUE);
	    ret = TRUE;
	}
	break;
    case OptionBool:
	{
	    ccsSetBool (setting, ccsIntegratedSettingReadValue (integratedSetting, TypeBool).value.asBool, TRUE);
	    ret = TRUE;
	}
	break;
    case OptionString:
	{
	    char *str = ccsIntegratedSettingReadValue (integratedSetting, TypeString).value.asString;

	    ccsSetString (setting, str, TRUE);
	    ret = TRUE;
	}
	break;
    case OptionKey:
	{
	    CCSSettingValue v = ccsIntegratedSettingReadValue (integratedSetting, TypeString);

	    if (v.value.asString)
	    {
		CCSSettingKeyValue key;

		memset (&key, 0, sizeof (CCSSettingKeyValue));
		ccsGetKey (setting, &key);
		if (ccsStringToKeyBinding (v.value.asString, &key))
		{
		    ccsSetKey (setting, key, TRUE);
		    ret = TRUE;
		}

		free (v.value.asString);
	    }
	}
	break;
    case OptionSpecial:
	{
	    const char *settingName = ccsSettingGetName (setting);
	    const char *pluginName  = ccsPluginGetName (ccsSettingGetParent (setting));

	    if (strcmp (settingName, "current_viewport") == 0)
	    {
		CCSSettingValue v = ccsIntegratedSettingReadValue (integratedSetting, TypeBool);

		Bool showAll = v.value.asBool;
		ccsSetBool (setting, !showAll, TRUE);
		ret = TRUE;
	    }
	    else if (strcmp (settingName, "fullscreen_visual_bell") == 0)
	    {
		CCSSettingValue v = ccsIntegratedSettingReadValue (integratedSetting, TypeBool);

		g_print ("read: %p\n", &v);

		const char *value = v.value.asString;
		if (value)
		{
		    Bool fullscreen;

		    fullscreen = strcmp (value, "fullscreen") == 0;
		    ccsSetBool (setting, fullscreen, TRUE);
		    ret = TRUE;
		}

		free (v.value.asString);
	    }
	    else if (strcmp (settingName, "click_to_focus") == 0)
	    {
		CCSSettingValue v = ccsIntegratedSettingReadValue (integratedSetting, TypeString);

		g_print ("read: %p\n", &v);

		const char *focusMode = v.value.asString;

		if (focusMode)
		{
		    Bool clickToFocus = (strcmp (focusMode, "click") == 0);
		    ccsSetBool (setting, clickToFocus, TRUE);
		    ret = TRUE;
		}

		free (v.value.asString);
	    }
	    else if (((strcmp (settingName, "initiate_button") == 0) &&
		      ((strcmp (pluginName, "move") == 0) ||
		       (strcmp (pluginName, "resize") == 0))) ||
		      ((strcmp (settingName, "window_menu_button") == 0) &&
		       (strcmp (pluginName, "core") == 0)))
	    {
		gboolean              resizeWithRightButton;
		CCSSettingButtonValue button;

		memset (&button, 0, sizeof (CCSSettingButtonValue));
		ccsGetButton (setting, &button);

		CCSIntegratedSettingList integratedSettingsMBM = ccsIntegratedSettingsStorageFindMatchingSettings (priv->storage,
														ccsGNOMEIntegratedPluginNames.SPECIAL,
														ccsGNOMEIntegratedSettingNames.NULL_MOUSE_BUTTON_MODIFIER.compizName);

		button.buttonModMask = getGnomeMouseButtonModifier (integratedSettingsMBM->data);

		CCSIntegratedSettingList integratedSettings = ccsIntegratedSettingsStorageFindMatchingSettings (priv->storage,
														ccsGNOMEIntegratedPluginNames.SPECIAL,
														ccsGNOMEIntegratedSettingNames.NULL_RESIZE_WITH_RIGHT_BUTTON.compizName);

		g_print ("found integrated setting: %p\n", integratedSettings->data);

		CCSSettingValue v = ccsIntegratedSettingReadValue (integratedSettings->data, TypeBool);

		g_print ("read: %p\n", &v);
		
		resizeWithRightButton =
		   v.value.asBool;

		if (strcmp (settingName, "window_menu_button") == 0)
		    button.button = resizeWithRightButton ? 2 : 3;
		else if (strcmp (pluginName, "resize") == 0)
		    button.button = resizeWithRightButton ? 3 : 2;
		else
		    button.button = 1;

		ccsSetButton (setting, button, TRUE);
		ret = TRUE;
	    }

	}
     	break;
    default:
	break;
    }

    return ret;
}

static Bool
setGnomeMouseButtonModifier (GConfClient *client,
			     unsigned int modMask)
{
    char   *modifiers, *currentValue;
    GError *err = NULL;

    modifiers = ccsModifiersToString (modMask);
    if (!modifiers)
	return FALSE;

    currentValue = gconf_client_get_string (client,
					    METACITY
					    "/general/mouse_button_modifier",
					    &err);
    if (err)
    {
	free (modifiers);
	g_error_free (err);
	return FALSE;
    }

    if (!currentValue || (strcmp (currentValue, modifiers) != 0))
	gconf_client_set_string (client,
				 METACITY "/general/mouse_button_modifier",
				 modifiers, NULL);
    if (currentValue)
	g_free (currentValue);

    free (modifiers);

    return TRUE;
}

static void
setButtonBindingForSetting (CCSContext   *context,
			    const char   *plugin,
			    const char   *setting,
			    unsigned int button,
			    unsigned int buttonModMask)
{
    CCSSetting            *s;
    CCSSettingButtonValue value;

    s = findDisplaySettingForPlugin (context, plugin, setting);
    if (!s)
	return;

    if (ccsSettingGetType (s) != TypeButton)
	return;

    value = ccsSettingGetValue (s)->value.asButton;

    if ((value.button != button) || (value.buttonModMask != buttonModMask))
    {
	value.button = button;
	value.buttonModMask = buttonModMask;

	ccsSetButton (s, value, TRUE);
    }
}

static void
ccsGConfIntegrationBackendWriteOptionFromSetting (CCSIntegration *integration,
						  CCSContext		 *context,
						  CCSSetting		 *setting,
						  CCSIntegratedSetting   *integratedSetting,
						  int			 index)
{
    GError     *err = NULL;
    const char *optionName = specialOptions[index].gnomeName;

    CCSGConfIntegrationBackendPrivate *priv = (CCSGConfIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);

    if (!priv->client)
	initGConfClient (integration);

    switch (ccsGNOMEIntegratedSettingGetSpecialOptionType ((CCSGNOMEIntegratedSetting *) integratedSetting))
    {
    case OptionInt:
	{
	    int newValue, currentValue;
	    if (!ccsGetInt (setting, &newValue))
		break;
	    currentValue = gconf_client_get_int (priv->client, optionName, &err);

	    if (!err && (currentValue != newValue))
		gconf_client_set_int(priv->client, specialOptions[index].gnomeName,
				     newValue, NULL);
	}
	break;
    case OptionBool:
	{
	    Bool     newValue;
	    gboolean currentValue;
	    if (!ccsGetBool (setting, &newValue))
		break;
	    currentValue = gconf_client_get_bool (priv->client, optionName, &err);

	    if (!err && ((currentValue && !newValue) ||
			 (!currentValue && newValue)))
		gconf_client_set_bool (priv->client, specialOptions[index].gnomeName,
				       newValue, NULL);
	}
	break;
    case OptionString:
	{
	    char  *newValue;
	    gchar *currentValue;
	    if (!ccsGetString (setting, &newValue))
		break;
	    currentValue = gconf_client_get_string (priv->client, optionName, &err);

    	    if (!err && currentValue)
	    {
		if (strcmp (currentValue, newValue) != 0)
		    gconf_client_set_string (priv->client, optionName,
		     			     newValue, NULL);
		g_free (currentValue);
	    }
	}
	break;
    case OptionKey:
	{
	    char  *newValue;
	    gchar *currentValue;

	    newValue = ccsKeyBindingToString (&(ccsSettingGetValue (setting)->value.asKey));
	    if (newValue)
    	    {
		if (strcmp (newValue, "Disabled") == 0)
		{
		    /* Metacity doesn't like "Disabled", it wants "disabled" */
		    newValue[0] = 'd';
		}

		currentValue = gconf_client_get_string (priv->client,
							optionName, &err);

		if (!err && currentValue)
		{
		    if (strcmp (currentValue, newValue) != 0)
			gconf_client_set_string (priv->client, optionName,
				 		 newValue, NULL);
    		    g_free (currentValue);
		}
		free (newValue);
	    }
	}
	break;
    case OptionSpecial:
	{
	    const char *settingName = specialOptions[index].settingName;
	    const char *pluginName  = specialOptions[index].pluginName;

	    if (strcmp (settingName, "current_viewport") == 0)
	    {
		Bool currentViewport;

		if (!ccsGetBool (setting, &currentViewport))
		    break;

		gconf_client_set_bool (priv->client, optionName,
				       !currentViewport, NULL);
	    }
	    else if (strcmp (settingName, "fullscreen_visual_bell") == 0)
	    {
		Bool  fullscreen;
		gchar *currentValue, *newValue;
		if (!ccsGetBool (setting, &fullscreen))
		    break;

		newValue = fullscreen ? "fullscreen" : "frame_flash";
		currentValue = gconf_client_get_string (priv->client,
							optionName, &err);
		if (!err && currentValue)
		{
		    if (strcmp (currentValue, newValue) != 0)
			gconf_client_set_string (priv->client, optionName,
						 newValue, NULL);
		    g_free (currentValue);
		}
	    }
	    else if (strcmp (settingName, "click_to_focus") == 0)
	    {
		Bool  clickToFocus;
		gchar *newValue, *currentValue;
		if (!ccsGetBool (setting, &clickToFocus))
		    break;

		newValue = clickToFocus ? "click" : "sloppy";
		currentValue = gconf_client_get_string (priv->client,
							optionName, &err);

		if (!err && currentValue)
		{
		    if (strcmp (currentValue, newValue) != 0)
			gconf_client_set_string (priv->client, optionName,
						 newValue, NULL);
		    g_free (currentValue);
		}
	    }
	    else if (((strcmp (settingName, "initiate_button") == 0) &&
		      ((strcmp (pluginName, "move") == 0) ||
		       (strcmp (pluginName, "resize") == 0))) ||
		      ((strcmp (settingName, "window_menu_button") == 0) &&
		       (strcmp (pluginName, "core") == 0)))
	    {
		unsigned int modMask;
		Bool         resizeWithRightButton = FALSE;
		gboolean     currentValue;

		if ((getButtonBindingForSetting (context, "resize",
						 "initiate_button") == 3) ||
		    (getButtonBindingForSetting (context, "core",
						 "window_menu_button") == 2))
		{
		     resizeWithRightButton = TRUE;
		}

		currentValue =
		    gconf_client_get_bool (priv->client, METACITY
					   "/general/resize_with_right_button",
					   &err);

		if (!err && ((currentValue && !resizeWithRightButton) ||
			     (!currentValue && resizeWithRightButton)))
		{
		    gconf_client_set_bool (priv->client,
					   METACITY
					   "/general/resize_with_right_button",
					   resizeWithRightButton, NULL);
		}

		modMask = ccsSettingGetValue (setting)->value.asButton.buttonModMask;
		if (setGnomeMouseButtonModifier (priv->client, modMask))
		{
		    setButtonBindingForSetting (context, "move",
						"initiate_button", 1, modMask);
		    setButtonBindingForSetting (context, "resize",
						"initiate_button", 
						resizeWithRightButton ? 3 : 2,
						modMask);
		    setButtonBindingForSetting (context, "core",
						"window_menu_button",
						resizeWithRightButton ? 2 : 3,
						modMask);
		}
	    }
	}
     	break;
    }

    if (err)
	g_error_free (err);
}

static void
ccsGConfIntegrationBackendFree (CCSIntegration *integration)
{
    CCSGConfIntegrationBackendPrivate *priv = (CCSGConfIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);

    finiGConfClient (integration);

    priv->backend = NULL;

    ccsObjectFinalize (integration);
    free (integration);
}

const CCSIntegrationInterface ccsGConfIntegrationBackendInterface =
{
    ccsGConfIntegrationBackendGetIntegratedOptionIndex,
    ccsGConfIntegrationBackendReadOptionIntoSetting,
    ccsGConfIntegrationBackendWriteOptionFromSetting,
    ccsGConfIntegrationBackendFree
};

static CCSGConfIntegrationBackendPrivate *
addPrivate (CCSIntegration *backend,
	    CCSObjectAllocationInterface *ai)
{
    CCSGConfIntegrationBackendPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGConfIntegrationBackendPrivate));

    if (!priv)
    {
	ccsObjectFinalize (backend);
	free (backend);
    }

    ccsObjectSetPrivate (backend, (CCSPrivate *) priv);

    return priv;
}

static CCSIntegration *
ccsGConfIntegrationBackendNewCommon (CCSBackend *backend,
				     CCSContext *context,
				     CCSObjectAllocationInterface *ai)
{
    CCSIntegration *integration = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSIntegration));

    if (!integration)
	return NULL;

    ccsObjectInit (integration, ai);

    CCSGConfIntegrationBackendPrivate *priv = addPrivate (integration, ai);
    priv->backend = backend;
    priv->context = context;

    ccsObjectAddInterface (integration,
			   (const CCSInterface *) &ccsGConfIntegrationBackendInterface,
			   GET_INTERFACE_TYPE (CCSIntegrationInterface));

    ccsIntegrationRef (integration);

    return integration;
}

CCSIntegration *
ccsGConfIntegrationBackendNew (CCSBackend *backend,
			       CCSContext *context,
			       CCSObjectAllocationInterface *ai)
{
    return ccsGConfIntegrationBackendNewCommon (backend, context, ai);
}

CCSIntegration *
ccsGConfIntegrationBackendNewWithClient (CCSBackend *backend,
					 CCSContext *context,
					 CCSObjectAllocationInterface *ai,
					 GConfClient *client)
{
    CCSIntegration *integration = ccsGConfIntegrationBackendNewCommon (backend, context, ai);
    CCSGConfIntegrationBackendPrivate *priv = (CCSGConfIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);

    priv->client = client;
    priv->factory = ccsGConfIntegratedSettingFactoryNew (priv->client, ai);
    priv->storage = ccsIntegratedSettingsStorageDefaultImplNew (ai);

    unsigned int i = 0;

    for (; i < N_SOPTIONS; i++)
    {
	CCSIntegratedSetting *setting = ccsIntegratedSettingFactoryCreateIntegratedSettingForCCSSettingNameAndType (priv->factory,
														    specialOptions[i].gnomeName,
														    specialOptions[i].pluginName,
														    specialOptions[i].settingName,
														    TypeInt);

	ccsIntegratedSettingsStorageAddSetting (priv->storage, setting);
    }

    registerGConfClient (integration, client);

    return integration;
}


#endif
