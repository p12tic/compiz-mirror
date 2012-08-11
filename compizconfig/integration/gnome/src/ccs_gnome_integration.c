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
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>
#include "ccs_gnome_integration.h"

typedef enum {
    OptionInt,
    OptionBool,
    OptionKey,
    OptionString,
    OptionSpecial,
} SpecialOptionType;

typedef struct _SpecialOptionGConf {
    const char*       settingName;
    const char*       pluginName;
    Bool	      screen;
    const char*       gnomeName;
    SpecialOptionType type;
} SpecialOptionGConf;

const SpecialOptionGConf specialOptions[] = {
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

#define N_SOPTIONS (sizeof (specialOptions) / sizeof (struct _SpecialOptionGConf))

typedef struct _CCSGConfIntegrationBackendPrivate CCSGConfIntegrationBackendPrivate;

struct _CCSGConfIntegrationBackendPrivate
{
    CCSBackend *backend;
    CCSContext *context;
    GConfClient *client;
    guint       *gnomeGConfNotifyIds;
};

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

static int
ccsGConfIntegrationBackendGetIntegratedOptionIndex (CCSIntegration *integration,
						    const char		  *settingName,
						    const char		  *pluginName)
{
    unsigned int i;

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

	return i;
    }

    return -1;
}

static void
gnomeGConfValueChanged (GConfClient *client,
			guint       cnxn_id,
			GConfEntry  *entry,
			gpointer    user_data)
{
    CCSIntegration *integration = (CCSIntegration *)user_data;
    CCSGConfIntegrationBackendPrivate *priv = (CCSGConfIntegrationBackendPrivate *) integration;
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
initGConfClient (CCSIntegration *integration)
{
    int i;

    CCSGConfIntegrationBackendPrivate *priv = (CCSGConfIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);

    if (priv->client)
	finiGConfClient (integration);

    priv->client = gconf_client_get_default ();

    for (i = 0; i < NUM_WATCHED_DIRS; i++)
    {
	priv->gnomeGConfNotifyIds[i] = gconf_client_notify_add (priv->client,
								watchedGConfGnomeDirectories[i],
								gnomeGConfValueChanged, integration,
								NULL, NULL);
	gconf_client_add_dir (priv->client, watchedGConfGnomeDirectories[i],
			      GCONF_CLIENT_PRELOAD_NONE, NULL);
    }
}

static unsigned int
getGnomeMouseButtonModifier (GConfClient *client)
{
    unsigned int modMask = 0;
    GError       *err = NULL;
    char         *value;

    value = gconf_client_get_string (client,
				     METACITY "/general/mouse_button_modifier",
				     &err);

    if (err)
    {
	g_error_free (err);
	return 0;
    }

    if (!value)
	return 0;

    modMask = ccsStringToModifiers (value);
    g_free (value);

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
						 int		       index)
{
    GConfValue *gconfValue;
    GError     *err = NULL;
    Bool       ret = FALSE;

    CCSGConfIntegrationBackendPrivate *priv = (CCSGConfIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);
    
    if (!priv->client)
	initGConfClient (integration);

    ret = ccsSettingIsReadableByBackend (setting);

    if (!ret)
	return FALSE;

    gconfValue = gconf_client_get (priv->client,
				   specialOptions[index].gnomeName,
				   &err);

    if (err)
    {
	g_error_free (err);
	return FALSE;
    }

    if (!gconfValue)
	return FALSE;

    switch (specialOptions[index].type) {
    case OptionInt:
	if (gconfValue->type == GCONF_VALUE_INT)
	{
	    guint value;

	    value = gconf_value_get_int (gconfValue);
	    ccsSetInt (setting, value, TRUE);
	    ret = TRUE;
	}
	break;
    case OptionBool:
	if (gconfValue->type == GCONF_VALUE_BOOL)
	{
	    gboolean value;

	    value = gconf_value_get_bool (gconfValue);
	    ccsSetBool (setting, value ? TRUE : FALSE, TRUE);
	    ret = TRUE;
	}
	break;
    case OptionString:
	if (gconfValue->type == GCONF_VALUE_STRING)
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
    case OptionKey:
	if (gconfValue->type == GCONF_VALUE_STRING)
	{
	    const char *value;

	    value = gconf_value_get_string (gconfValue);
	    if (value)
	    {
		CCSSettingKeyValue key;

		memset (&key, 0, sizeof (CCSSettingKeyValue));
		ccsGetKey (setting, &key);
		if (ccsStringToKeyBinding (value, &key))
		{
		    ccsSetKey (setting, key, TRUE);
		    ret = TRUE;
		}
	    }
	}
	break;
    case OptionSpecial:
	{
	    const char *settingName = specialOptions[index].settingName;
	    const char *pluginName  = specialOptions[index].pluginName;

	    if (strcmp (settingName, "current_viewport") == 0)
	    {
		if (gconfValue->type == GCONF_VALUE_BOOL)
		{
		    gboolean showAll;

		    showAll = gconf_value_get_bool (gconfValue);
		    ccsSetBool (setting, !showAll, TRUE);
		    ret = TRUE;
		}
	    }
	    else if (strcmp (settingName, "fullscreen_visual_bell") == 0)
	    {
		if (gconfValue->type == GCONF_VALUE_STRING)
		{
		    const char *value;

		    value = gconf_value_get_string (gconfValue);
		    if (value)
		    {
			Bool fullscreen;

			fullscreen = strcmp (value, "fullscreen") == 0;
			ccsSetBool (setting, fullscreen, TRUE);
			ret = TRUE;
		    }
		}
	    }
	    else if (strcmp (settingName, "click_to_focus") == 0)
	    {
		if (gconfValue->type == GCONF_VALUE_STRING)
		{
		    const char *focusMode;

		    focusMode = gconf_value_get_string (gconfValue);

		    if (focusMode)
		    {
			Bool clickToFocus = (strcmp (focusMode, "click") == 0);
			ccsSetBool (setting, clickToFocus, TRUE);
			ret = TRUE;
		    }
		}
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

		button.buttonModMask = getGnomeMouseButtonModifier (priv->client);
		
		resizeWithRightButton =
		    gconf_client_get_bool (priv->client, METACITY
					   "/general/resize_with_right_button",
					   &err);

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

    gconf_value_free (gconfValue);

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
						  int			 index)
{
    GError     *err = NULL;
    const char *optionName = specialOptions[index].gnomeName;

    CCSGConfIntegrationBackendPrivate *priv = (CCSGConfIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);

    if (!priv->client)
	initGConfClient (integration);

    switch (specialOptions[index].type)
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
    CCSGConfIntegrationBackendPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSIntegration));

    if (!priv)
    {
	ccsObjectFinalize (backend);
	free (backend);
    }

    ccsObjectSetPrivate (backend, (CCSPrivate *) priv);

    return priv;
}

CCSIntegration *
ccsGConfIntegrationBackendNew (CCSBackend *backend,
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


#endif
