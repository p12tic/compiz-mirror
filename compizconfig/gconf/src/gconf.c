/**
 *
 * GConf libccs backend
 *
 * gconf.c
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
 **/

#define CCS_LOG_DOMAIN "gconf"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <string.h>
#include <dirent.h>

#include <ccs.h>
#include <ccs-backend.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>

#include "ccs_gnome_integration.h"

#define CompAltMask        (1 << 16)
#define CompMetaMask       (1 << 17)
#define CompSuperMask      (1 << 18)
#define CompHyperMask      (1 << 19)
#define CompModeSwitchMask (1 << 20)
#define CompNumLockMask    (1 << 21)
#define CompScrollLockMask (1 << 22)

#define METACITY     "/apps/metacity"
#define COMPIZ       "/apps/compiz-1"
#define COMPIZCONFIG "/apps/compizconfig-1"
#define PROFILEPATH  COMPIZCONFIG "/profiles"
#define DEFAULTPROF "Default"
#define CORE_NAME   "core"

#define BUFSIZE 512

#define KEYNAME(sn)     char keyName[BUFSIZE]; \
                    snprintf (keyName, BUFSIZE, "screen%i", sn);

#define PATHNAME    char pathName[BUFSIZE]; \
		    if (!ccsPluginGetName (ccsSettingGetParent (setting)) || \
			strcmp (ccsPluginGetName (ccsSettingGetParent (setting)), "core") == 0) \
                        snprintf (pathName, BUFSIZE, \
				 "%s/general/%s/options/%s", COMPIZ, \
				 keyName, ccsSettingGetName (setting)); \
                    else \
			snprintf(pathName, BUFSIZE, \
				 "%s/plugins/%s/%s/options/%s", COMPIZ, \
				 ccsPluginGetName (ccsSettingGetParent (setting)), keyName, ccsSettingGetName (setting));

static const char* watchedGnomeDirectories[] = {
    METACITY,
    "/desktop/gnome/applications/terminal",
    "/apps/panel/applets/window_list/prefs"
};
#define NUM_WATCHED_DIRS 3

static GConfClient *client = NULL;
static GConfEngine *conf = NULL;
static guint compizNotifyId;
static guint gnomeNotifyIds[NUM_WATCHED_DIRS];
static char *currentProfile = NULL;
static CCSIntegration *integration = NULL;

/* some forward declarations */
static Bool readInit (CCSBackend *backend, CCSContext * context);
static void readSetting (CCSBackend *backend, CCSContext * context, CCSSetting * setting);
static Bool readOption (CCSSetting * setting);
static Bool writeInit (CCSBackend *backend, CCSContext * context);
static void writeIntegratedOption (CCSContext *context, CCSSetting *setting,
				   int        index);

typedef enum {
    OptionInt,
    OptionBool,
    OptionKey,
    OptionBell,
    OptionString,
    OptionSpecial,
} SpecialOptionType;

typedef struct _SpecialOption {
    const char*       settingName;
    const char*       pluginName;
    Bool	      screen;
    const char*       gnomeName;
    SpecialOptionType type;
} SpecialOption;

const SpecialOption specialOptions[] = {
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
    {"mouse_button_modifier", NULL, FALSE,
     METACITY "/general/mouse_button_modifier", OptionSpecial},
    /* integration of the Metacity's option to swap mouse buttons */
    {"resize_with_right_button", NULL, FALSE,
     METACITY "/general/resize_with_right_button", OptionSpecial},

    {"visual_bell", "fade", TRUE,
     METACITY "/general/visual_bell", OptionBell},
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

#define N_SOPTIONS (sizeof (specialOptions) / sizeof (struct _SpecialOption))

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

static Bool
isIntegratedOption (CCSSetting *setting,
		    int        *index)
{
    CCSPlugin    *plugin = ccsSettingGetParent (setting);
    const char   *pluginName = ccsPluginGetName (plugin);
    const char   *settingName = ccsSettingGetName (setting);
    unsigned int i = ccsIntegrationGetIntegratedOptionIndex (integration, pluginName, settingName);

    if (index)
	*index = i;

    return i != -1;
}

static void
updateSetting (CCSBackend *backend, CCSContext *context, CCSPlugin *plugin, CCSSetting *setting)
{
    int          index;
    readInit (backend, context);
    if (!readOption (setting))
	ccsResetToDefault (setting, TRUE);

    if (ccsGetIntegrationEnabled (context) &&
	isIntegratedOption (setting, &index))
    {
	writeInit (backend, context);
	writeIntegratedOption (context, setting, index);
    }
}

static void
valueChanged (GConfClient *client,
	      guint       cnxn_id,
	      GConfEntry  *entry,
	      gpointer    user_data)
{
    CCSContext   *context = (CCSContext *)user_data;
    char         *keyName = (char*) gconf_entry_get_key (entry);
    char         *pluginName;
    char         *token;
    unsigned int screenNum;
    CCSPlugin    *plugin;
    CCSSetting   *setting;

    keyName += strlen (COMPIZ) + 1;

    token = strsep (&keyName, "/"); /* plugin */
    if (!token)
	return;

    if (strcmp (token, "general") == 0)
    {
	pluginName = "core";
    }
    else
    {
	token = strsep (&keyName, "/");
	if (!token)
	    return;
	pluginName = token;
    }

    plugin = ccsFindPlugin (context, pluginName);
    if (!plugin)
	return;

    token = strsep (&keyName, "/");
    if (!token)
	return;

    sscanf (token, "screen%d", &screenNum);

    token = strsep (&keyName, "/"); /* 'options' */
    if (!token)
	return;

    token = strsep (&keyName, "/");
    if (!token)
	return;

    setting = ccsFindSetting (plugin, token);
    if (!setting)
	return;

    /* Passing null here is not optimal, but we are not
     * maintaining gconf actively here */
    updateSetting (NULL, context, plugin, setting);
}

static void
gnomeValueChanged (GConfClient *client,
		   guint       cnxn_id,
		   GConfEntry  *entry,
		   gpointer    user_data)
{
    CCSContext *context = (CCSContext *)user_data;
    char       *keyName = (char*) gconf_entry_get_key (entry);
    int        i, last = 0, num = 0;
    Bool       needInit = TRUE;

    if (!ccsGetIntegrationEnabled (context))
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
		readInit (NULL, context);
		needInit = FALSE;
	    }

	    s = findDisplaySettingForPlugin (context, "core",
					     "window_menu_button");
	    if (s)
		readSetting (NULL, context, s);

	    s = findDisplaySettingForPlugin (context, "move",
					     "initiate_button");
	    if (s)
		readSetting (NULL, context, s);

	    s = findDisplaySettingForPlugin (context, "resize",
					     "initiate_button");
	    if (s)
		readSetting (NULL, context, s);
	}
	else
	{
	    CCSPlugin     *plugin = NULL;
	    CCSSetting    *setting;
	    SpecialOption *opt = (SpecialOption *) &specialOptions[num];

	    plugin = ccsFindPlugin (context, (char*) opt->pluginName);
	    if (plugin)
	    {
		for (i = 0; i < 1; i++)
		{
		    setting = ccsFindSetting (plugin, (char*) opt->settingName);

		    if (setting)
		    {
			if (needInit)
			{
			    readInit (NULL, context);
			    needInit = FALSE;
			}
			readSetting (NULL, context, setting);
		    }

		    /* do not read display settings multiple
		       times for multiscreen environments */
		}
	    }
	}
    }
}

static void
initClient (CCSBackend *backend, CCSContext *context)
{
    int i;

    client = gconf_client_get_for_engine (conf);
    integration = ccsGConfIntegrationBackendNewWithClient (backend,
							   context,
							   &ccsDefaultObjectAllocator,
							   client);

    compizNotifyId = gconf_client_notify_add (client, COMPIZ, valueChanged,
					      context, NULL, NULL);
    gconf_client_add_dir (client, COMPIZ, GCONF_CLIENT_PRELOAD_NONE, NULL);

    for (i = 0; i < NUM_WATCHED_DIRS; i++)
    {
	gnomeNotifyIds[i] = gconf_client_notify_add (client,
						     watchedGnomeDirectories[i],
						     gnomeValueChanged, context,
						     NULL, NULL);
	gconf_client_add_dir (client, watchedGnomeDirectories[i],
			      GCONF_CLIENT_PRELOAD_NONE, NULL);
    }
}

static void
finiClient (void)
{
    int i;

    if (compizNotifyId)
    {
	gconf_client_notify_remove (client, compizNotifyId);
	compizNotifyId = 0;
    }
    gconf_client_remove_dir (client, COMPIZ, NULL);

    for (i = 0; i < NUM_WATCHED_DIRS; i++)
    {
	if (gnomeNotifyIds[i])
	{
	    gconf_client_notify_remove (client, gnomeNotifyIds[0]);
	    gnomeNotifyIds[i] = 0;
	}
	gconf_client_remove_dir (client, watchedGnomeDirectories[i], NULL);
    }

    gconf_client_suggest_sync (client, NULL);

    g_object_unref (client);
    client = NULL;

    ccsIntegrationUnref (integration);
    integration = NULL;
}

static void
copyGconfValues (GConfEngine *conf,
		 const gchar *from,
		 const gchar *to,
		 Bool        associate,
		 const gchar *schemaPath)
{
    GSList *values, *tmp;
    GError *err = NULL;

    values = gconf_engine_all_entries (conf, from, &err);
    tmp = values;

    while (tmp)
    {
	GConfEntry *entry = tmp->data;
	GConfValue *value;
	const char *key = gconf_entry_get_key (entry);
	char       *name, *newKey, *newSchema = NULL;

	name = strrchr (key, '/');
	if (!name)
	    continue;

	if (to)
	{
	    if (asprintf (&newKey, "%s/%s", to, name + 1) == -1)
		newKey = NULL;

	    if (associate && schemaPath)
		if (asprintf (&newSchema, "%s/%s", schemaPath, name + 1) == -1)
		    newSchema = NULL;

	    if (newKey && newSchema)
		gconf_engine_associate_schema (conf, newKey, newSchema, NULL);

	    if (newKey)
	    {
		value = gconf_engine_get_without_default (conf, key, NULL);
		if (value)
		{
		    gconf_engine_set (conf, newKey, value, NULL);
		    gconf_value_free (value);
		}
	    }

	    if (newSchema)
		free (newSchema);
	    if (newKey)
		free (newKey);
	}
	else
	{
	    if (associate)
		gconf_engine_associate_schema (conf, key, NULL, NULL);
	    gconf_engine_unset (conf, key, NULL);
	}

	gconf_entry_unref (entry);
	tmp = g_slist_next (tmp);
    }

    if (values)
	g_slist_free (values);
}

static void
copyGconfRecursively (GConfEngine *conf,
		      GSList      *subdirs,
		      const gchar *to,
		      Bool        associate,
		      const gchar *schemaPath)
{
    GSList* tmp;

    tmp = subdirs;

    while (tmp)
    {
 	gchar *path = tmp->data;
	char  *newKey, *newSchema = NULL, *name;

	name = strrchr (path, '/');
	if (name)
	{
	    if (!(to && asprintf (&newKey, "%s/%s", to, name + 1) != -1))
		newKey = NULL;

	    if (associate && schemaPath)
		if (asprintf (&newSchema, "%s/%s", schemaPath, name + 1) == -1)
		    newSchema = NULL;

	    copyGconfValues (conf, path, newKey, associate, newSchema);
	    copyGconfRecursively (conf,
				  gconf_engine_all_dirs (conf, path, NULL),
				  newKey, associate, newSchema);

	    if (newSchema)
		free (newSchema);

	    if (newKey)
		free (newKey);

	    if (!to)
		gconf_engine_remove_dir (conf, path, NULL);
	}

	g_free (path);
	tmp = g_slist_next (tmp);
    }

    if (subdirs)
	g_slist_free (subdirs);
}

static void
copyGconfTree (CCSBackend  *backend,
	       CCSContext  *context,
	       const gchar *from,
	       const gchar *to,
	       Bool        associate,
	       const gchar *schemaPath)
{
    GSList* subdirs;

    /* we aren't allowed to have an open GConfClient object while
       using GConfEngine, so shut it down and open it again afterwards */
    finiClient ();

    subdirs = gconf_engine_all_dirs (conf, from, NULL);
    gconf_engine_suggest_sync (conf, NULL);

    copyGconfRecursively (conf, subdirs, to, associate, schemaPath);

    gconf_engine_suggest_sync (conf, NULL);

    initClient (backend, context);
}

static Bool
readListValue (CCSSetting *setting,
	       GConfValue *gconfValue)
{
    GConfValueType      valueType;
    unsigned int        nItems, i = 0;
    CCSSettingValueList list = NULL;
    GSList              *valueList = NULL;

    switch (ccsSettingGetInfo (setting)->forList.listType)
    {
    case TypeString:
    case TypeMatch:
    case TypeColor:
	valueType = GCONF_VALUE_STRING;
	break;
    case TypeBool:
	valueType = GCONF_VALUE_BOOL;
	break;
    case TypeInt:
	valueType = GCONF_VALUE_INT;
	break;
    case TypeFloat:
	valueType = GCONF_VALUE_FLOAT;
	break;
    default:
	valueType = GCONF_VALUE_INVALID;
	break;
    }

    if (valueType == GCONF_VALUE_INVALID)
	return FALSE;

    if (valueType != gconf_value_get_list_type (gconfValue))
	return FALSE;

    valueList = gconf_value_get_list (gconfValue);
    if (!valueList)
    {
	ccsSetList (setting, NULL, TRUE);
	return TRUE;
    }

    nItems = g_slist_length (valueList);

    switch (ccsSettingGetInfo (setting)->forList.listType)
    {
    case TypeBool:
	{
	    Bool *array = malloc (nItems * sizeof (Bool));
	    if (!array)
		break;

	    for (; valueList; valueList = valueList->next, i++)
		array[i] =
		    gconf_value_get_bool (valueList->data) ? TRUE : FALSE;
	    list = ccsGetValueListFromBoolArray (array, nItems, setting);
	    free (array);
	}
	break;
    case TypeInt:
	{
	    int *array = malloc (nItems * sizeof (int));
	    if (!array)
		break;

	    for (; valueList; valueList = valueList->next, i++)
		array[i] = gconf_value_get_int (valueList->data);
	    list = ccsGetValueListFromIntArray (array, nItems, setting);
	    free (array);
	}
	break;
    case TypeFloat:
	{
	    float *array = malloc (nItems * sizeof (float));
	    if (!array)
		break;

	    for (; valueList; valueList = valueList->next, i++)
		array[i] = gconf_value_get_float (valueList->data);
	    list = ccsGetValueListFromFloatArray (array, nItems, setting);
	    free (array);
	}
	break;
    case TypeString:
    case TypeMatch:
	{
	    gchar **array = malloc (nItems * sizeof (char*));
	    if (!array)
		break;

	    for (; valueList; valueList = valueList->next, i++)
		array[i] = strdup (gconf_value_get_string (valueList->data));
	    list = ccsGetValueListFromStringArray ((const char **) array, nItems, setting);
	    g_strfreev (array);
	}
	break;
    case TypeColor:
	{
	    CCSSettingColorValue *array;
	    array = malloc (nItems * sizeof (CCSSettingColorValue));
	    if (!array)
		break;

	    for (; valueList; valueList = valueList->next, i++)
    	    {
		memset (&array[i], 0, sizeof (CCSSettingColorValue));
		ccsStringToColor (gconf_value_get_string (valueList->data),
				  &array[i]);
	    }
	    list = ccsGetValueListFromColorArray (array, nItems, setting);
	    free (array);
	}
	break;
    default:
	break;
    }

    if (list)
    {
	ccsSetList (setting, list, TRUE);
	ccsSettingValueListFree (list, TRUE);
	return TRUE;
    }

    return FALSE;
}


static Bool
readIntegratedOption (CCSContext *context,
		      CCSSetting *setting,
		      int        index)
{
    return ccsIntegrationReadOptionIntoSetting (integration, context, setting, index);
}

static Bool
readOption (CCSSetting * setting)
{
    GConfValue *gconfValue = NULL;
    GError     *err = NULL;
    Bool       ret = FALSE;
    Bool       valid = TRUE;

    KEYNAME (ccsContextGetScreenNum (ccsPluginGetContext (ccsSettingGetParent (setting))));
    PATHNAME;

    /* first check if the key is set */
    gconfValue = gconf_client_get_without_default (client, pathName, &err);
    if (err)
    {
	g_error_free (err);
	return FALSE;
    }
    if (!gconfValue)
	/* value is not set */
	return FALSE;

    /* setting type sanity check */
    switch (ccsSettingGetType (setting))
    {
    case TypeString:
    case TypeMatch:
    case TypeColor:
    case TypeKey:
    case TypeButton:
    case TypeEdge:
	valid = (gconfValue->type == GCONF_VALUE_STRING);
	break;
    case TypeInt:
	valid = (gconfValue->type == GCONF_VALUE_INT);
	break;
    case TypeBool:
    case TypeBell:
	valid = (gconfValue->type == GCONF_VALUE_BOOL);
	break;
    case TypeFloat:
	valid = (gconfValue->type == GCONF_VALUE_FLOAT);
	break;
    case TypeList:
	valid = (gconfValue->type == GCONF_VALUE_LIST);
	break;
    default:
	break;
    }
    if (!valid)
    {
	ccsWarning ("There is an unsupported value at path %s. "
		"Settings from this path won't be read. Try to remove "
		"that value so that operation can continue properly.",
		pathName);
	return FALSE;
    }

    switch (ccsSettingGetType (setting))
    {
    case TypeString:
	{
	    const char *value;
	    value = gconf_value_get_string (gconfValue);
	    if (value)
	    {
		ccsSetString (setting, value, TRUE);
		ret = TRUE;
	    }
	}
	break;
    case TypeMatch:
	{
	    const char * value;
	    value = gconf_value_get_string (gconfValue);
	    if (value)
	    {
		ccsSetMatch (setting, value, TRUE);
		ret = TRUE;
	    }
	}
	break;
    case TypeInt:
	{
	    int value;
	    value = gconf_value_get_int (gconfValue);

	    ccsSetInt (setting, value, TRUE);
	    ret = TRUE;
	}
	break;
    case TypeBool:
	{
	    gboolean value;
	    value = gconf_value_get_bool (gconfValue);

	    ccsSetBool (setting, value ? TRUE : FALSE, TRUE);
	    ret = TRUE;
	}
	break;
    case TypeFloat:
	{
	    double value;
	    value = gconf_value_get_float (gconfValue);

	    ccsSetFloat (setting, (float)value, TRUE);
    	    ret = TRUE;
	}
	break;
    case TypeColor:
	{
	    const char           *value;
	    CCSSettingColorValue color;
	    value = gconf_value_get_string (gconfValue);

	    if (value && ccsStringToColor (value, &color))
	    {
		ccsSetColor (setting, color, TRUE);
		ret = TRUE;
	    }
	}
	break;
    case TypeKey:
	{
	    const char         *value;
	    CCSSettingKeyValue key;
	    value = gconf_value_get_string (gconfValue);

	    if (value && ccsStringToKeyBinding (value, &key))
	    {
		ccsSetKey (setting, key, TRUE);
		ret = TRUE;
	    }
	}
	break;
    case TypeButton:
	{
	    const char            *value;
	    CCSSettingButtonValue button;
	    value = gconf_value_get_string (gconfValue);

	    if (value && ccsStringToButtonBinding (value, &button))
	    {
		ccsSetButton (setting, button, TRUE);
		ret = TRUE;
	    }
	}
	break;
    case TypeEdge:
	{
	    const char   *value;
	    value = gconf_value_get_string (gconfValue);

	    if (value)
	    {
		unsigned int edges;
		edges = ccsStringToEdges (value);
		ccsSetEdge (setting, edges, TRUE);
		ret = TRUE;
	    }
	}
	break;
    case TypeBell:
	{
	    gboolean value;
	    value = gconf_value_get_bool (gconfValue);

	    ccsSetBell (setting, value ? TRUE : FALSE, TRUE);
	    ret = TRUE;
	}
	break;
    case TypeList:
	ret = readListValue (setting, gconfValue);
	break;
    default:
	ccsWarning ("Attempt to read unsupported setting type %d from path %s!",
	       ccsSettingGetType (setting), pathName);
	break;
    }

    if (gconfValue)
	gconf_value_free (gconfValue);

    return ret;
}

static void
writeListValue (CCSSetting *setting,
		char       *pathName)
{
    GSList              *valueList = NULL;
    GConfValueType      valueType;
    Bool                freeItems = FALSE;
    CCSSettingValueList list;
    gpointer            data;

    if (!ccsGetList (setting, &list))
	return;

    switch (ccsSettingGetInfo (setting)->forList.listType)
    {
    case TypeBool:
	{
	    while (list)
	    {
		data = GINT_TO_POINTER (list->data->value.asBool);
		valueList = g_slist_append (valueList, data);
		list = list->next;
	    }
	    valueType = GCONF_VALUE_BOOL;
	}
	break;
    case TypeInt:
	{
	    while (list)
	    {
		data = GINT_TO_POINTER (list->data->value.asInt);
		valueList = g_slist_append(valueList, data);
		list = list->next;
    	    }
	    valueType = GCONF_VALUE_INT;
	}
	break;
    case TypeFloat:
	{
	    gdouble *item;
	    while (list)
	    {
		item = malloc (sizeof (gdouble));
		if (item)
		{
		    *item = list->data->value.asFloat;
		    valueList = g_slist_append (valueList, item);
		}
		list = list->next;
	    }
	    freeItems = TRUE;
	    valueType = GCONF_VALUE_FLOAT;
	}
	break;
    case TypeString:
	{
	    while (list)
	    {
		valueList = g_slist_append(valueList,
		   			   list->data->value.asString);
		list = list->next;
	    }
	    valueType = GCONF_VALUE_STRING;
	}
	break;
    case TypeMatch:
	{
	    while (list)
	    {
		valueList = g_slist_append(valueList,
		   			   list->data->value.asMatch);
		list = list->next;
	    }
	    valueType = GCONF_VALUE_STRING;
	}
	break;
    case TypeColor:
	{
	    char *item;
	    while (list)
	    {
		item = ccsColorToString (&list->data->value.asColor);
		valueList = g_slist_append (valueList, item);
		list = list->next;
	    }
	    freeItems = TRUE;
	    valueType = GCONF_VALUE_STRING;
	}
	break;
    default:
	ccsWarning ("Attempt to write unsupported list type %d at path %s!",
	       ccsSettingGetInfo (setting)->forList.listType, pathName);
	valueType = GCONF_VALUE_INVALID;
	break;
    }

    if (valueType != GCONF_VALUE_INVALID)
    {
	gconf_client_set_list (client, pathName, valueType, valueList, NULL);

	if (freeItems)
	{
	    GSList *tmpList = valueList;
	    for (; tmpList; tmpList = tmpList->next)
		if (tmpList->data)
		    free (tmpList->data);
	}
    }
    if (valueList)
	g_slist_free (valueList);
}

static void
writeIntegratedOption (CCSContext *context,
		       CCSSetting *setting,
		       int        index)
{
    ccsIntegrationWriteSettingIntoOption (integration, context, setting, index);
}

static void
resetOptionToDefault (CCSSetting * setting)
{
    KEYNAME (ccsContextGetScreenNum (ccsPluginGetContext (ccsSettingGetParent (setting))));
    PATHNAME;

    gconf_client_recursive_unset (client, pathName, 0, NULL);
    gconf_client_suggest_sync (client, NULL);
}

static void
writeOption (CCSSetting * setting)
{
    KEYNAME (ccsContextGetScreenNum (ccsPluginGetContext (ccsSettingGetParent (setting))));
    PATHNAME;

    switch (ccsSettingGetType (setting))
    {
    case TypeString:
	{
	    char *value;
	    if (ccsGetString (setting, &value))
		gconf_client_set_string (client, pathName, value, NULL);
	}
	break;
    case TypeMatch:
	{
	    char *value;
	    if (ccsGetMatch (setting, &value))
		gconf_client_set_string (client, pathName, value, NULL);
	}
    case TypeFloat:
	{
	    float value;
	    if (ccsGetFloat (setting, &value))
		gconf_client_set_float (client, pathName, value, NULL);
	}
	break;
    case TypeInt:
	{
	    int value;
	    if (ccsGetInt (setting, &value))
		gconf_client_set_int (client, pathName, value, NULL);
	}
	break;
    case TypeBool:
	{
	    Bool value;
	    if (ccsGetBool (setting, &value))
		gconf_client_set_bool (client, pathName, value, NULL);
	}
	break;
    case TypeColor:
	{
	    CCSSettingColorValue value;
	    char                 *colString;

	    if (!ccsGetColor (setting, &value))
		break;

	    colString = ccsColorToString (&value);
	    if (!colString)
		break;

	    gconf_client_set_string (client, pathName, colString, NULL);
	    free (colString);
	}
	break;
    case TypeKey:
	{
	    CCSSettingKeyValue key;
	    char               *keyString;

	    if (!ccsGetKey (setting, &key))
		break;

	    keyString = ccsKeyBindingToString (&key);
	    if (!keyString)
		break;

	    gconf_client_set_string (client, pathName, keyString, NULL);
	    free (keyString);
	}
	break;
    case TypeButton:
	{
	    CCSSettingButtonValue button;
	    char                  *buttonString;

	    if (!ccsGetButton (setting, &button))
		break;

	    buttonString = ccsButtonBindingToString (&button);
	    if (!buttonString)
		break;

	    gconf_client_set_string (client, pathName, buttonString, NULL);
	    free (buttonString);
	}
	break;
    case TypeEdge:
	{
	    unsigned int edges;
	    char         *edgeString;

	    if (!ccsGetEdge (setting, &edges))
		break;

	    edgeString = ccsEdgesToString (edges);
	    if (!edgeString)
		break;

	    gconf_client_set_string (client, pathName, edgeString, NULL);
	    free (edgeString);
	}
	break;
    case TypeBell:
	{
	    Bool value;
	    if (ccsGetBell (setting, &value))
		gconf_client_set_bool (client, pathName, value, NULL);
	}
	break;
    case TypeList:
	writeListValue (setting, pathName);
	break;
    default:
	ccsWarning ("Attempt to write unsupported setting type %d",
	       ccsSettingGetType (setting));
	break;
    }
}

static void
updateCurrentProfileName (char *profile)
{
    GConfSchema *schema;
    GConfValue  *value;
    
    schema = gconf_schema_new ();
    if (!schema)
	return;

    value = gconf_value_new (GCONF_VALUE_STRING);
    if (!value)
    {
	gconf_schema_free (schema);
	return;
    }

    gconf_schema_set_type (schema, GCONF_VALUE_STRING);
    gconf_schema_set_locale (schema, "C");
    gconf_schema_set_short_desc (schema, "Current profile");
    gconf_schema_set_long_desc (schema, "Current profile of gconf backend");
    gconf_schema_set_owner (schema, "compizconfig-1");
    gconf_value_set_string (value, profile);
    gconf_schema_set_default_value (schema, value);

    gconf_client_set_schema (client, COMPIZCONFIG "/current_profile",
			     schema, NULL);

    gconf_schema_free (schema);
    gconf_value_free (value);
}

static char*
getCurrentProfileName (void)
{
    GConfSchema *schema = NULL;

    schema = gconf_client_get_schema (client,
    				      COMPIZCONFIG "/current_profile", NULL);

    if (schema)
    {
	GConfValue *value;
	char       *ret = NULL;

	value = gconf_schema_get_default_value (schema);
	if (value)
	    ret = strdup (gconf_value_get_string (value));
	gconf_schema_free (schema);

	return ret;
    }

    return NULL;
}

static Bool
checkProfile (CCSBackend *backend,
	      CCSContext *context)
{
    const char *profileCCS;
    char *lastProfile;

    lastProfile = currentProfile;

    profileCCS = ccsGetProfile (context);
    if (!profileCCS || !strlen (profileCCS))
	currentProfile = strdup (DEFAULTPROF);
    else
	currentProfile = strdup (profileCCS);

    if (!lastProfile || strcmp (lastProfile, currentProfile) != 0)
    {
	char *pathName;

	if (lastProfile)
	{
	    /* copy /apps/compiz-1 tree to profile path */
	    if (asprintf (&pathName, "%s/%s", PROFILEPATH, lastProfile) == -1)
		pathName = NULL;

	    if (pathName)
	    {
		copyGconfTree (backend,
			       context, COMPIZ, pathName,
			       TRUE, "/schemas" COMPIZ);
		free (pathName);
	    }
	}

	/* reset /apps/compiz-1 tree */
	gconf_client_recursive_unset (client, COMPIZ, 0, NULL);

	/* copy new profile tree to /apps/compiz-1 */
	if (asprintf (&pathName, "%s/%s", PROFILEPATH, currentProfile) == -1)
	    pathName = NULL;

	if (pathName)
	{
	    copyGconfTree (backend, context, pathName, COMPIZ, FALSE, NULL);

    	    /* delete the new profile tree in /apps/compizconfig-1
    	       to avoid user modification in the wrong tree */
	    copyGconfTree (backend, context, pathName, NULL, TRUE, NULL);
    	    free (pathName);
	}

	/* update current profile name */
	updateCurrentProfileName (currentProfile);
    }

    if (lastProfile)
	free (lastProfile);

    return TRUE;
}

static void
processEvents (CCSBackend *backend, unsigned int flags)
{
    if (!(flags & ProcessEventsNoGlibMainLoopMask))
    {
	while (g_main_context_pending(NULL))
	    g_main_context_iteration(NULL, FALSE);
    }
}

static Bool
initBackend (CCSBackend *backend, CCSContext * context)
{
    g_type_init ();

    conf = gconf_engine_get_default ();
    initClient (backend, context);

    currentProfile = getCurrentProfileName ();

    return TRUE;
}

static Bool
finiBackend (CCSBackend *backend)
{
    gconf_client_clear_cache (client);
    finiClient ();

    if (currentProfile)
    {
	free (currentProfile);
	currentProfile = NULL;
    }

    gconf_engine_unref (conf);
    conf = NULL;

    return TRUE;
}

static Bool
readInit (CCSBackend *backend, CCSContext * context)
{
    return checkProfile (backend, context);
}

static void
readSetting (CCSBackend *backend,
	     CCSContext *context,
	     CCSSetting *setting)
{
    Bool status;
    int  index;

    if (ccsGetIntegrationEnabled (context) &&
	isIntegratedOption (setting, &index))
    {
	status = readIntegratedOption (context, setting, index);
    }
    else
	status = readOption (setting);

    if (!status)
	ccsResetToDefault (setting, TRUE);
}

static Bool
writeInit (CCSBackend *backend, CCSContext * context)
{
    return checkProfile (backend, context);
}

static void
writeSetting (CCSBackend *backend,
	      CCSContext *context,
	      CCSSetting *setting)
{
    int index;

    if (ccsGetIntegrationEnabled (context) &&
	isIntegratedOption (setting, &index))
    {
	writeIntegratedOption (context, setting, index);
    }
    else if (ccsSettingGetIsDefault (setting))
    {
	resetOptionToDefault (setting);
    }
    else
	writeOption (setting);

}

static Bool
getSettingIsIntegrated (CCSBackend *backend, CCSSetting * setting)
{
    if (!ccsGetIntegrationEnabled (ccsPluginGetContext (ccsSettingGetParent (setting))))
	return FALSE;

    if (!isIntegratedOption (setting, NULL))
	return FALSE;

    return TRUE;
}

static Bool
getSettingIsReadOnly (CCSBackend *backend, CCSSetting * setting)
{
    /* FIXME */
    return FALSE;
}

static CCSStringList
getExistingProfiles (CCSBackend *backend, CCSContext *context)
{
    GSList        *data, *tmp;
    CCSStringList ret = NULL;
    char          *name;

    gconf_client_suggest_sync (client, NULL);
    data = gconf_client_all_dirs (client, PROFILEPATH, NULL);

    for (tmp = data; tmp; tmp = g_slist_next (tmp))
    {
	name = strrchr (tmp->data, '/');
	if (name && (strcmp (name + 1, DEFAULTPROF) != 0))
	{
	    CCSString *str = calloc (1, sizeof (CCSString));
	    str->value = strdup (name + 1);
	    ret = ccsStringListAppend (ret, str);
	}

	g_free (tmp->data);
    }

    g_slist_free (data);

    name = getCurrentProfileName ();
    if (name && strcmp (name, DEFAULTPROF) != 0)
    {
	CCSString *str = calloc (1, sizeof (CCSString));
	str->value = name;
	ret = ccsStringListAppend (ret, str);
    }
    else if (name)
	free (name);

    return ret;
}

static Bool
deleteProfile (CCSBackend *backend,
	       CCSContext *context,
	       char       *profile)
{
    char     path[BUFSIZE];
    gboolean status = FALSE;

    checkProfile (backend, context);

    snprintf (path, BUFSIZE, "%s/%s", PROFILEPATH, profile);

    if (gconf_client_dir_exists (client, path, NULL))
    {
	status =
	    gconf_client_recursive_unset (client, path,
	   				  GCONF_UNSET_INCLUDING_SCHEMA_NAMES,
					  NULL);
	gconf_client_suggest_sync (client, NULL);
    }

    return status;
}

const CCSBackendInfo gconfBackendInfo =
{
    "gconf",
    "GConf Configuration Backend",
    "GConf Configuration Backend for libccs",
    TRUE,
    TRUE,
    1
};

static const CCSBackendInfo *
getInfo (CCSBackend *backend)
{
    return &gconfBackendInfo;
}

static CCSBackendInterface gconfVTable = {
    getInfo,
    processEvents,
    initBackend,
    finiBackend,
    readInit,
    readSetting,
    0,
    writeInit,
    writeSetting,
    0,
    updateSetting,
    getSettingIsIntegrated,
    getSettingIsReadOnly,
    getExistingProfiles,
    deleteProfile
};

CCSBackendInterface *
getBackendInfo (void)
{
    return &gconfVTable;
}

