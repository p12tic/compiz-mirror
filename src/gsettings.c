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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <string.h>
#include <dirent.h>

#include <ccs.h>
#include <ccs-backend.h>

#include <gio/gio.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>

#define CompAltMask        (1 << 16)
#define CompMetaMask       (1 << 17)
#define CompSuperMask      (1 << 18)
#define CompHyperMask      (1 << 19)
#define CompModeSwitchMask (1 << 20)
#define CompNumLockMask    (1 << 21)
#define CompScrollLockMask (1 << 22)

#define SCHEMA_ID   "org.freedesktop.compiz"
#define METACITY     "/apps/metacity"
#define COMPIZ       "/apps/compiz-1"
#define COMPIZCONFIG "/apps/compizconfig-1"
#define PROFILEPATH  COMPIZCONFIG "/profiles"
#define DEFAULTPROF "Default"
#define CORE_NAME   "core"

#define BUFSIZE 512

#define NUM_WATCHED_DIRS 3

static GConfClient *client = NULL;
static GList	   *settings_list = NULL;
static GSettings   *compizconfig_settings = NULL;
static GSettings   *current_profile_settings = NULL;
static GConfEngine *conf = NULL;
static guint compizNotifyId;
static guint gnomeNotifyIds[NUM_WATCHED_DIRS];
static char *currentProfile = NULL;

static gchar *
gsettings_get_schema_name (const char *plugin)
{
    gchar       *schema_name =  NULL;

    schema_name = g_strconcat (SCHEMA_ID, ".", plugin, NULL);

    return schema_name;
}

static inline gchar *
gsettings_backend_clean (char *gsettingName)
{
    gchar *clean        = NULL;
    gchar **delimited   = NULL;
    guint i		= 0;

    /* Replace underscores with dashes */
    delimited = g_strsplit (gsettingName, "_", 0);

    clean = g_strjoinv ("-", delimited);

    /* It can't be more than 32 chars, warn if that's the case */
    gchar *ret = g_strndup (clean, 31);

    if (strlen (clean) > 31)
	printf ("GSettings Backend: Warning: key name %s is not valid in GSettings, it was changed to %s, this may cause problems!\n", clean, ret);

    /* Everything must be lowercase */
    for (; i < strlen (ret); i++)
	ret[i] = g_ascii_tolower (ret[i]);

    g_free (clean);
    g_strfreev (delimited);

    return ret;
}

#define CLEAN_SETTING_NAME 	char *cleanSettingName = gsettings_backend_clean (setting->name)

#define KEYNAME(sn)     char keyName[BUFSIZE]; \
                    snprintf (keyName, BUFSIZE, "screen%i", sn);

#define PATHNAME    char pathName[BUFSIZE]; \
                    if (!setting->parent->name || \
			strcmp (setting->parent->name, "core") == 0) \
                        snprintf (pathName, BUFSIZE, \
				 "%s/%s/plugins/%s/%s/options/", COMPIZ, currentProfile, \
				 setting->parent->name, keyName); \
                    else \
			snprintf(pathName, BUFSIZE, \
				 "%s/%s/plugins/%s/%s/options/", COMPIZ, currentProfile, \
				 setting->parent->name, keyName);

#define CLEANUP_CLEAN_SETTING_NAME free (cleanSettingName);

static GSettings *
gsettings_object_for_plugin_path (const char *plugin, const char *path)
{
    GSettings *settings_obj = NULL;
    GList *l = settings_list;
    gchar *schema_name = gsettings_get_schema_name (plugin);
    GVariant        *written_plugins;
    char	    *plug;
    GVariant        *new_written_plugins;
    GVariantBuilder *new_written_plugins_builder;
    GVariantIter    *iter;
    gboolean	    found = FALSE;

    while (l)
    {
	settings_obj = (GSettings *) l->data;
	gchar	  *name = NULL;

	g_object_get (G_OBJECT (settings_obj),
		      "schema",
		      &name, NULL);
	if (g_strcmp0 (name, schema_name) == 0)
	{
	    g_free (name);
	    g_free (schema_name);

	    g_object_ref (settings_obj);
	    return settings_obj;
	}

	l = g_list_next (l);
    }

    /* No existing settings object found for this schema, create one */
    
    settings_obj = g_settings_new_with_path (schema_name, path);

    g_object_ref (settings_obj);
    settings_list = g_list_append (settings_list, (void *) settings_obj);

    /* Also write the plugin name to the list of modified plugins so
     * that when we delete the profile the keys for that profile are also
     * unset FIXME: This could be a little more efficient, like we could
     * store keys that have changed from their defaults ... though
     * gsettings doesn't seem to give you a way to get all of the schemas */

    written_plugins = g_settings_get_value (current_profile_settings, "plugins-with-set-keys");

    new_written_plugins_builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));

    iter = g_variant_iter_new (written_plugins);
    while (g_variant_iter_loop (iter, "s", &plug))
    {
	g_variant_builder_add (new_written_plugins_builder, "s", plug);

	if (!found)
	    found = (g_strcmp0 (plug, plugin) == 0);
    }

    if (!found)
	g_variant_builder_add (new_written_plugins_builder, "s", plugin);

    new_written_plugins = g_variant_new ("as", new_written_plugins_builder);
    g_settings_set_value (current_profile_settings, "plugins-with-set-keys", new_written_plugins);

    g_variant_iter_free (iter);
    g_variant_unref (new_written_plugins);
    g_variant_builder_unref (new_written_plugins_builder);

    return settings_obj;
}

static GSettings *
gsettings_object_for_setting (CCSSetting *setting)
{
    KEYNAME(setting->parent->context->screenNum);
    PATHNAME;

    return gsettings_object_for_plugin_path (setting->parent->name, pathName);
}

static const char* watchedGnomeDirectories[] = {
    METACITY,
    "/desktop/gnome/applications/terminal",
    "/apps/panel/applets/window_list/prefs"
};

/* some forward declarations */
static Bool readInit (CCSContext * context);
static void readSetting (CCSContext * context, CCSSetting * setting);
static Bool readOption (CCSSetting * setting);
static Bool writeInit (CCSContext * context);
static void writeOption (CCSSetting *setting);
static void writeIntegratedOption (CCSContext *context, CCSSetting *setting,
				   int        index);

typedef enum {
    OptionInt,
    OptionBool,
    OptionKey,
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
    unsigned int i;

    CLEAN_SETTING_NAME;

    for (i = 0; i < N_SOPTIONS; i++)
    {
	const SpecialOption *opt = &specialOptions[i];

	if (strcmp (cleanSettingName, opt->settingName) != 0)
	    continue;

	if (setting->parent->name)
	{
	    if (!opt->pluginName)
		continue;
	    if (strcmp (setting->parent->name, opt->pluginName) != 0)
		continue;
	}
	else
	{
	    if (opt->pluginName)
		continue;
	}

	if (index)
	    *index = i;

	return TRUE;
    }

    CLEANUP_CLEAN_SETTING_NAME;

    return FALSE;
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
    int          index;
    Bool         isScreen;
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

    isScreen = TRUE;
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

    readInit (context);
    if (!readOption (setting))
	ccsResetToDefault (setting);

    if (ccsGetIntegrationEnabled (context) &&
	isIntegratedOption (setting, &index))
    {
	writeInit (context);
	writeIntegratedOption (context, setting, index);
    }
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
		readInit (context);
		needInit = FALSE;
	    }

	    s = findDisplaySettingForPlugin (context, "core",
					     "window_menu_button");
	    if (s)
		readSetting (context, s);

	    s = findDisplaySettingForPlugin (context, "move",
					     "initiate_button");
	    if (s)
		readSetting (context, s);

	    s = findDisplaySettingForPlugin (context, "resize",
					     "initiate_button");
	    if (s)
		readSetting (context, s);
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
		    unsigned int screen;

		    screen = 0;

		    setting = ccsFindSetting (plugin, (char*) opt->settingName);

		    if (setting)
		    {
			if (needInit)
			{
			    readInit (context);
			    needInit = FALSE;
			}
			readSetting (context, setting);
		    }

		    /* do not read display settings multiple
		       times for multiscreen environments */
		}
	    }
	}
    }
}

static void
initClient (CCSContext *context)
{
    int i;

    client = gconf_client_get_for_engine (conf);
/*
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
*/
}

static void
finiClient (void)
{
    int i;
/*
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
*/
    gconf_client_suggest_sync (client, NULL);

    g_object_unref (client);
    client = NULL;
}

static Bool
readListValue (CCSSetting *setting,
	       GConfValue *gconfValue)
{
    GSettings		*settings = gsettings_object_for_setting (setting);
    gchar		*variantType;
    unsigned int        nItems, i = 0;
    CCSSettingValueList list = NULL;
    GSList              *valueList = NULL;
    GVariant		*value;
    GVariantIter	*iter;

    CLEAN_SETTING_NAME;
    
    switch (setting->info.forList.listType)
    {
    case TypeString:
    case TypeMatch:
    case TypeColor:
	variantType = g_strdup ("s");
	break;
    case TypeBool:
	variantType = g_strdup ("b");
	break;
    case TypeInt:
	variantType = g_strdup ("i");
	break;
    case TypeFloat:
	variantType = g_strdup ("d");
	break;
    default:
	variantType = NULL;
	break;
    }

    if (!variantType)
	return FALSE;

    value = g_settings_get_value (settings, cleanSettingName);
    if (!value)
    {
	ccsSetList (setting, NULL);
	return TRUE;
    }

    iter = g_variant_iter_new (value);
    nItems = g_variant_iter_n_children (iter);

    switch (setting->info.forList.listType)
    {
    case TypeBool:
	{
	    Bool *array = malloc (nItems * sizeof (Bool));
	    Bool *array_counter = array;

	    if (!array)
		break;
	    
	    /* Reads each item from the variant into the position pointed
	     * at by array_counter */
	    while (g_variant_iter_loop (iter, variantType, array_counter))
		array_counter++;

	    list = ccsGetValueListFromBoolArray (array, nItems, setting);
	    free (array);
	}
	break;
    case TypeInt:
	{
	    int *array = malloc (nItems * sizeof (int));
	    int *array_counter = array;

	    if (!array)
		break;
	    
	    /* Reads each item from the variant into the position pointed
	     * at by array_counter */
	    while (g_variant_iter_loop (iter, variantType, array_counter))
		array_counter++;

	    list = ccsGetValueListFromIntArray (array, nItems, setting);
	    free (array);
	}
	break;
    case TypeFloat:
	{
	    double *array = malloc (nItems * sizeof (double));
	    double *array_counter = array;

	    if (!array)
		break;
	    
	    /* Reads each item from the variant into the position pointed
	     * at by array_counter */
	    while (g_variant_iter_loop (iter, variantType, array_counter))
		array_counter++;

	    list = ccsGetValueListFromFloatArray ((float *) array, nItems, setting);
	    free (array);
	}
	break;
    case TypeString:
    case TypeMatch:
	{
	    char **array = calloc (1, (nItems + 1) * sizeof (char *));
	    char **array_counter = array;

	    if (!array)
	    {
		break;
	    }
	    
	    /* Reads each item from the variant into the position pointed
	     * at by array_counter */
	    while (g_variant_iter_loop (iter, variantType, array_counter))
		array_counter++;

	    list = ccsGetValueListFromStringArray (array, nItems, setting);
	    for (i = 0; i < nItems; i++)
		if (array[i])
		    free (array[i]);
	    free (array);
	}
	break;
    case TypeColor:
	{
	    CCSSettingColorValue *array;
	    char		 *colorValue;
	    array = malloc (nItems * sizeof (CCSSettingColorValue));
	    if (!array)
		break;

	    while (g_variant_iter_loop (iter, variantType, &colorValue))
    	    {
		memset (&array[i], 0, sizeof (CCSSettingColorValue));
		ccsStringToColor (colorValue,
				  &array[i]);
	    }
	    list = ccsGetValueListFromColorArray (array, nItems, setting);
	    free (array);
	}
	break;
    default:
	break;
    }

    CLEANUP_CLEAN_SETTING_NAME;
    g_object_unref (settings);
    free (variantType);

    if (list)
    {
	ccsSetList (setting, list);
	ccsSettingValueListFree (list, TRUE);
	return TRUE;
    }

    return FALSE;
}

static unsigned int
getGnomeMouseButtonModifier(void)
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

    if (s->type != TypeButton)
	return 0;

    return s->value->value.asButton.button;
}


static Bool
readIntegratedOption (CCSContext *context,
		      CCSSetting *setting,
		      int        index)
{
    GConfValue *gconfValue;
    GError     *err = NULL;
    Bool       ret = FALSE;

    g_debug ("Attempted to read integrated option %s which is not a supported operation yet", setting->name);
    
    ret = readOption (setting);
#if 0
    gconfValue = gconf_client_get (client,
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
	    ccsSetInt (setting, value);
	    ret = TRUE;
	}
	break;
    case OptionBool:
	if (gconfValue->type == GCONF_VALUE_BOOL)
	{
	    gboolean value;

	    value = gconf_value_get_bool (gconfValue);
	    ccsSetBool (setting, value ? TRUE : FALSE);
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
		ccsSetString (setting, value);
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
		    ccsSetKey (setting, key);
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
		    ccsSetBool (setting, !showAll);
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
			ccsSetBool (setting, fullscreen);
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
			ccsSetBool (setting, clickToFocus);
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

		button.buttonModMask = getGnomeMouseButtonModifier ();
		
		resizeWithRightButton =
		    gconf_client_get_bool (client, METACITY
					   "/general/resize_with_right_button",
					   &err);

		if (strcmp (settingName, "window_menu_button") == 0)
		    button.button = resizeWithRightButton ? 2 : 3;
		else if (strcmp (pluginName, "resize") == 0)
		    button.button = resizeWithRightButton ? 3 : 2;
		else
		    button.button = 1;

		ccsSetButton (setting, button);
		ret = TRUE;
	    }
	}
     	break;
    default:
	break;
    }

    gconf_value_free (gconfValue);
#endif
    return ret;
}

static Bool
readOption (CCSSetting * setting)
{
    GSettings  *settings = gsettings_object_for_setting (setting);
    GVariant   *gsettingsValue = NULL;
    GConfValue *gconfValue = NULL;
    GError     *err = NULL;
    Bool       ret = FALSE;
    Bool       valid = TRUE;

    /* It is impossible for certain settings to have a schema,
     * such as actions and read only settings, so in that case
     * just return FALSE since compizconfig doesn't expect us
     * to read them anyways */

    if (setting->type == TypeAction ||
	ccsSettingIsReadOnly (setting))
    {
	g_object_unref (settings);
	return FALSE;
    }

    CLEAN_SETTING_NAME;
    KEYNAME(setting->parent->context->screenNum);
    PATHNAME;

    /* first check if the key is set */
    gsettingsValue = g_settings_get_value (settings, cleanSettingName);
    if (err)
    {
	g_error_free (err);
	CLEANUP_CLEAN_SETTING_NAME;
	g_object_unref (settings);
	printf ("GSettings Backend: Warning: key name %s for path %s is not set!\n", cleanSettingName, pathName);
	return FALSE;
    }

    switch (setting->type)
    {
    case TypeString:
    case TypeMatch:
    case TypeColor:
    case TypeKey:
    case TypeButton:
    case TypeEdge:
	valid = (g_variant_type_is_subtype_of (G_VARIANT_TYPE_STRING, g_variant_get_type (gsettingsValue)));
	break;
    case TypeInt:
	valid = (g_variant_type_is_subtype_of (G_VARIANT_TYPE_INT32, g_variant_get_type (gsettingsValue)));
	break;
    case TypeBool:
    case TypeBell:
	valid = (g_variant_type_is_subtype_of (G_VARIANT_TYPE_BOOLEAN, g_variant_get_type (gsettingsValue)));
	break;
    case TypeFloat:
	valid = (g_variant_type_is_subtype_of (G_VARIANT_TYPE_DOUBLE, g_variant_get_type (gsettingsValue)));
	break;
    case TypeList:
	valid = (g_variant_type_is_array (g_variant_get_type (gsettingsValue)));
	break;
    default:
	break;
    }

    if (!valid)
    {
	printf ("GSettings backend: There is an unsupported value at path %s. "
		"Settings from this path won't be read. Try to remove "
		"that value so that operation can continue properly.\n",
		pathName);
	CLEANUP_CLEAN_SETTING_NAME;
	g_object_unref (settings);
	g_variant_unref (gsettingsValue);
	return FALSE;
    }

    switch (setting->type)
    {
    case TypeString:
	{
	    const char *value;
	    value = g_variant_get_string (gsettingsValue, NULL);
	    if (value)
	    {
		ccsSetString (setting, value);
		ret = TRUE;
	    }
	}
	break;
    case TypeMatch:
	{
	    const char * value;
	    value = g_variant_get_string (gsettingsValue, NULL);
	    if (value)
	    {
		ccsSetMatch (setting, value);
		ret = TRUE;
	    }
	}
	break;
    case TypeInt:
	{
	    int value;
	    value = g_variant_get_int32 (gsettingsValue);

	    ccsSetInt (setting, value);
	    ret = TRUE;
	}
	break;
    case TypeBool:
	{
	    gboolean value;
	    value = g_variant_get_boolean (gsettingsValue);

	    ccsSetBool (setting, value ? TRUE : FALSE);
	    ret = TRUE;
	}
	break;
    case TypeFloat:
	{
	    double value;
	    value = g_variant_get_double (gsettingsValue);

	    ccsSetFloat (setting, (float)value);
    	    ret = TRUE;
	}
	break;
    case TypeColor:
	{
	    const char           *value;
	    CCSSettingColorValue color;
	    value = g_variant_get_string (gsettingsValue, NULL);

	    if (value && ccsStringToColor (value, &color))
	    {
		ccsSetColor (setting, color);
		ret = TRUE;
	    }
	}
	break;
    case TypeKey:
	{
	    const char         *value;
	    CCSSettingKeyValue key;
	    value = g_variant_get_string (gsettingsValue, NULL);

	    if (value && ccsStringToKeyBinding (value, &key))
	    {
		ccsSetKey (setting, key);
		ret = TRUE;
	    }
	}
	break;
    case TypeButton:
	{
	    const char            *value;
	    CCSSettingButtonValue button;
	    value = g_variant_get_string (gsettingsValue, NULL);

	    if (value && ccsStringToButtonBinding (value, &button))
	    {
		ccsSetButton (setting, button);
		ret = TRUE;
	    }
	}
	break;
    case TypeEdge:
	{
	    const char   *value;
	    value = g_variant_get_string (gsettingsValue, NULL);

	    if (value)
	    {
		unsigned int edges;
		edges = ccsStringToEdges (value);
		ccsSetEdge (setting, edges);
		ret = TRUE;
	    }
	}
	break;
    case TypeBell:
	{
	    gboolean value;
	    value = g_variant_get_boolean (gsettingsValue);

	    ccsSetBell (setting, value ? TRUE : FALSE);
	    ret = TRUE;
	}
	break;
    case TypeList:
	ret = readListValue (setting, NULL);
	break;
    default:
	printf("GSettings backend: attempt to read unsupported setting type %d!\n",
	       setting->type);
	break;
    }

    CLEANUP_CLEAN_SETTING_NAME;
    g_object_unref (settings);
    g_variant_unref (gsettingsValue);

    return ret;
}

static void
writeListValue (CCSSetting *setting,
		char       *pathName)
{
    GSettings  		*settings = gsettings_object_for_setting (setting);
    GVariant 		*value;
    GSList              *valueList = NULL;
    GConfValueType      valueType;
    Bool                freeItems = FALSE;
    gchar		*variantType = NULL;
    CCSSettingValueList list;
    gpointer            data;

    CLEAN_SETTING_NAME;
    
    if (!ccsGetList (setting, &list))
	return;
    
    switch (setting->info.forList.listType)
    {
    case TypeBool:
	{
	    variantType = "ab";
	    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("ab"));
	    while (list)
	    {
		g_variant_builder_add (builder, "b", list->data->value.asBool);
		list = list->next;
	    }
	    value = g_variant_new ("ab", builder);
	    g_variant_builder_unref (builder);
	    valueType = GCONF_VALUE_BOOL;
	}
	break;
    case TypeInt:
	{
	    variantType = "ai";
	    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("ai"));
	    while (list)
	    {
		g_variant_builder_add (builder, "i", list->data->value.asInt);
		list = list->next;
    	    }
    	    value = g_variant_new ("ai", builder);
	    g_variant_builder_unref (builder);
	    valueType = GCONF_VALUE_INT;
	}
	break;
    case TypeFloat:
	{
	    variantType = "ad";
	    gdouble *item;
	    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("ad"));
	    while (list)
	    {
		g_variant_builder_add (builder, "d", (gdouble) list->data->value.asFloat);
		list = list->next;
	    }
	    value = g_variant_new ("ad", builder);
	    g_variant_builder_unref (builder);
	    valueType = GCONF_VALUE_FLOAT;
	}
	break;
    case TypeString:
	{
	    variantType = "as";
	    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
	    while (list)
	    {
		g_variant_builder_add (builder, "s", list->data->value.asString);
		list = list->next;
	    }
	    value = g_variant_new ("as", builder);
	    g_variant_builder_unref (builder);
	    valueType = GCONF_VALUE_STRING;
	}
	break;
    case TypeMatch:
	{
	    variantType = "as";
	    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
	    while (list)
	    {
		g_variant_builder_add (builder, "s", list->data->value.asMatch);
		list = list->next;
	    }
	    value = g_variant_new ("as", builder);
	    g_variant_builder_unref (builder);
	    valueType = GCONF_VALUE_STRING;
	}
	break;
    case TypeColor:
	{
	    variantType = "as";
	    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
	    char *item;
	    while (list)
	    {
		item = ccsColorToString (&list->data->value.asColor);
		g_variant_builder_add (builder, "s", list->data->value.asColor);
		list = list->next;
	    }
	    value = g_variant_new ("as", builder);
	    g_variant_builder_unref (builder);
	    freeItems = TRUE;
	    valueType = GCONF_VALUE_STRING;
	}
	break;
    default:
	printf("GSettings backend: attempt to write unsupported list type %d!\n",
	       setting->info.forList.listType);
	valueType = GCONF_VALUE_INVALID;
	variantType = NULL;
	break;
    }

    if (valueType != GCONF_VALUE_INVALID &&
	variantType != NULL)
    {
	g_settings_set_value (settings, cleanSettingName, value);
	g_variant_unref (value);
    }
    if (valueList)
	g_slist_free (valueList);
    
    CLEANUP_CLEAN_SETTING_NAME;
    
    g_object_unref (settings);
}

static Bool
setGnomeMouseButtonModifier (unsigned int modMask)
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

    if (s->type != TypeButton)
	return;

    value = s->value->value.asButton;

    if ((value.button != button) || (value.buttonModMask != buttonModMask))
    {
	value.button = button;
	value.buttonModMask = buttonModMask;

	ccsSetButton (s, value);
    }
}

static void
writeIntegratedOption (CCSContext *context,
		       CCSSetting *setting,
		       int        index)
{
    GError     *err = NULL;
    const char *optionName = specialOptions[index].gnomeName;

    g_debug ("Attempted to write integrated option %s which is not supported option yet", setting->name);
    
    writeOption (setting);
#if 0
    
    switch (specialOptions[index].type)
    {
    case OptionInt:
	{
	    int newValue, currentValue;
	    if (!ccsGetInt (setting, &newValue))
		break;
	    currentValue = gconf_client_get_int (client, optionName, &err);

	    if (!err && (currentValue != newValue))
		gconf_client_set_int(client, specialOptions[index].gnomeName,
				     newValue, NULL);
	}
	break;
    case OptionBool:
	{
	    Bool     newValue;
	    gboolean currentValue;
	    if (!ccsGetBool (setting, &newValue))
		break;
    	    currentValue = gconf_client_get_bool (client, optionName, &err);

	    if (!err && ((currentValue && !newValue) ||
			 (!currentValue && newValue)))
		gconf_client_set_bool (client, specialOptions[index].gnomeName,
				       newValue, NULL);
	}
	break;
    case OptionString:
	{
	    char  *newValue;
	    gchar *currentValue;
	    if (!ccsGetString (setting, &newValue))
		break;
	    currentValue = gconf_client_get_string (client, optionName, &err);

    	    if (!err && currentValue)
	    {
		if (strcmp (currentValue, newValue) != 0)
		    gconf_client_set_string (client, optionName,
		     			     newValue, NULL);
		g_free (currentValue);
	    }
	}
	break;
    case OptionKey:
	{
	    char  *newValue;
	    gchar *currentValue;

	    newValue = ccsKeyBindingToString (&setting->value->value.asKey);
	    if (newValue)
    	    {
		if (strcmp (newValue, "Disabled") == 0)
		{
		    /* Metacity doesn't like "Disabled", it wants "disabled" */
		    newValue[0] = 'd';
		}

		currentValue = gconf_client_get_string (client,
							optionName, &err);

		if (!err && currentValue)
		{
		    if (strcmp (currentValue, newValue) != 0)
			gconf_client_set_string (client, optionName,
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

		gconf_client_set_bool (client, optionName,
				       !currentViewport, NULL);
	    }
	    else if (strcmp (settingName, "fullscreen_visual_bell") == 0)
	    {
		Bool  fullscreen;
		gchar *currentValue, *newValue;
		if (!ccsGetBool (setting, &fullscreen))
		    break;

		newValue = fullscreen ? "fullscreen" : "frame_flash";
		currentValue = gconf_client_get_string (client,
							optionName, &err);
		if (!err && currentValue)
		{
		    if (strcmp (currentValue, newValue) != 0)
			gconf_client_set_string (client, optionName,
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
		currentValue = gconf_client_get_string (client,
							optionName, &err);

		if (!err && currentValue)
		{
		    if (strcmp (currentValue, newValue) != 0)
			gconf_client_set_string (client, optionName,
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
		    gconf_client_get_bool (client, METACITY
					   "/general/resize_with_right_button",
					   &err);

		if (!err && ((currentValue && !resizeWithRightButton) ||
			     (!currentValue && resizeWithRightButton)))
		{
		    gconf_client_set_bool (client,
					   METACITY
					   "/general/resize_with_right_button",
					   resizeWithRightButton, NULL);
		}

		modMask = setting->value->value.asButton.buttonModMask;
		if (setGnomeMouseButtonModifier (modMask))
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
#endif
    if (err)
	g_error_free (err);
}

static void
resetOptionToDefault (CCSSetting * setting)
{
    GSettings  *settings = gsettings_object_for_setting (setting);
  
    CLEAN_SETTING_NAME;
    KEYNAME (setting->parent->context->screenNum);
    PATHNAME;

    gconf_client_recursive_unset (client, pathName, 0, NULL);
    gconf_client_suggest_sync (client, NULL);

    g_object_unref (settings);
    CLEANUP_CLEAN_SETTING_NAME;
}

static void
writeOption (CCSSetting * setting)
{
    GSettings  *settings = gsettings_object_for_setting (setting);
    CLEAN_SETTING_NAME;
    KEYNAME (setting->parent->context->screenNum);
    PATHNAME;

    switch (setting->type)
    {
    case TypeString:
	{
	    char *value;
	    if (ccsGetString (setting, &value))
	    {
		g_settings_set (settings, cleanSettingName, "s", value, NULL);
	    }
	}
	break;
    case TypeMatch:
	{
	    char *value;
	    if (ccsGetMatch (setting, &value))
	    {
		g_settings_set (settings, cleanSettingName, "s", value, NULL);
	    }
	}
    case TypeFloat:
	{
	    float value;
	    if (ccsGetFloat (setting, &value))
	    {
		g_settings_set (settings, cleanSettingName, "d", (double) value, NULL);
	    }
	}
	break;
    case TypeInt:
	{
	    int value;
	    if (ccsGetInt (setting, &value))
	    {
		g_settings_set (settings, cleanSettingName, "i", value, NULL);
	    }
	}
	break;
    case TypeBool:
	{
	    Bool value;
	    if (ccsGetBool (setting, &value))
	    {
		g_settings_set (settings, cleanSettingName, "b", value, NULL);
	    }
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

	    g_settings_set (settings, cleanSettingName, "s", value, NULL);
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

	    g_settings_set (settings, cleanSettingName, "s", keyString, NULL);
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

	    g_settings_set (settings, cleanSettingName, "s", buttonString, NULL);
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

	    g_settings_set (settings, cleanSettingName, "s", edgeString, NULL);
	    free (edgeString);
	}
	break;
    case TypeBell:
	{
	    Bool value;
	    if (ccsGetBell (setting, &value))
	    {
		g_settings_set (settings, cleanSettingName, "s", value, NULL);
	    }
	}
	break;
    case TypeList:
	writeListValue (setting, pathName);
	break;
    default:
	printf("GSettings backend: attempt to write unsupported setting type %d\n",
	       setting->type);
	break;
    }

    g_object_unref (settings);
    CLEANUP_CLEAN_SETTING_NAME;
}

static void
updateCurrentProfileName (char *profile)
{
    GVariant        *profiles;
    char	    *prof;
    char	    *profilePath = "/org/freedesktop/compizconfig/profile/";
    char	    *currentProfilePath;
    GVariant        *new_profiles;
    GVariantBuilder *new_profiles_builder;
    GVariantIter    *iter;
    gboolean        found = FALSE;

    profiles = g_settings_get_value (compizconfig_settings, "existing-profiles");

    new_profiles_builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));

    iter = g_variant_iter_new (profiles);
    while (g_variant_iter_loop (iter, "s", &prof))
    {
	g_variant_builder_add (new_profiles_builder, "s", prof);

	if (!found)
	    found = (g_strcmp0 (prof, profile) == 0);
    }

    if (!found)
	g_variant_builder_add (new_profiles_builder, "s", profile);

    new_profiles = g_variant_new ("as", new_profiles_builder);
    g_settings_set_value (compizconfig_settings, "existing-profiles", new_profiles);

    g_variant_iter_free (iter);
    g_variant_unref (new_profiles);
    g_variant_builder_unref (new_profiles_builder);

    /* Change the current profile and current profile settings */
    free (currentProfile);
    g_object_unref (current_profile_settings);

    currentProfile = strdup (profile);
    currentProfilePath = g_strconcat (profilePath, profile, "/", NULL);
    current_profile_settings = g_settings_new_with_path ("org.freedesktop.compizconfig.profile", profilePath);

    g_free (currentProfilePath);

    g_settings_set (compizconfig_settings, "current-profile", "s", profile, NULL);
}

static char*
getCurrentProfileName (void)
{
    GVariant *value;
    char     *ret = NULL;

    value = g_settings_get_value (compizconfig_settings, "current-profile");

    if (value)
	ret = strdup (g_variant_get_string (value, NULL));
    else
	ret = strdup (DEFAULTPROF);

    return ret;
}

static void
processEvents (unsigned int flags)
{
    if (!(flags & ProcessEventsNoGlibMainLoopMask))
    {
	while (g_main_context_pending(NULL))
	    g_main_context_iteration(NULL, FALSE);
    }
}

static Bool
initBackend (CCSContext * context)
{
    const char *profilePath = "/org/freedesktop/compizconfig/profile/";
    char       *currentProfilePath;
    g_type_init ();

    compizconfig_settings = g_settings_new ("org.freedesktop.compizconfig");

    conf = gconf_engine_get_default ();
    initClient (context);

    currentProfile = getCurrentProfileName ();
    currentProfilePath = g_strconcat (profilePath, currentProfile, "/", NULL);
    current_profile_settings = g_settings_new_with_path ("org.freedesktop.compizconfig.profile", currentProfilePath);

    g_free (currentProfilePath);

    return TRUE;
}

static Bool
finiBackend (CCSContext * context)
{
    GList *l = settings_list;

    processEvents (0);

    gconf_client_clear_cache (client);
    finiClient ();

    if (currentProfile)
    {
	free (currentProfile);
	currentProfile = NULL;
    }

    gconf_engine_unref (conf);
    conf = NULL;

    while (l)
    {
	g_object_unref (G_OBJECT (l->data));
	l = g_list_next (l);
    }

    g_object_unref (G_OBJECT (compizconfig_settings));

    processEvents (0);
    return TRUE;
}

static Bool
readInit (CCSContext * context)
{
    return TRUE;
}

static void
readSetting (CCSContext *context,
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
	ccsResetToDefault (setting);
}

static Bool
writeInit (CCSContext * context)
{
    return TRUE;
}

static void
writeSetting (CCSContext *context,
	      CCSSetting *setting)
{
    int index;

    if (ccsGetIntegrationEnabled (context) &&
	isIntegratedOption (setting, &index))
    {
	writeIntegratedOption (context, setting, index);
    }
    else if (setting->isDefault)
    {
	resetOptionToDefault (setting);
    }
    else
	writeOption (setting);

}

static Bool
getSettingIsIntegrated (CCSSetting * setting)
{
    if (!ccsGetIntegrationEnabled (setting->parent->context))
	return FALSE;

    if (!isIntegratedOption (setting, NULL))
	return FALSE;

    return TRUE;
}

static Bool
getSettingIsReadOnly (CCSSetting * setting)
{
    /* FIXME */
    return FALSE;
}

static CCSStringList
getExistingProfiles (CCSContext *context)
{
    GVariant      *value;
    GVariant      *profile;
    GVariantIter  iter;
    CCSStringList ret = NULL;

    value = g_settings_get_value (compizconfig_settings,  "existing-profiles");
    g_variant_iter_init (&iter, value);
    while (g_variant_iter_loop (&iter, "s", &profile))
	ret = ccsStringListAppend (ret, strdup (g_variant_get_string (profile, NULL)));

    g_variant_unref (value);

    return ret;
}

static Bool
deleteProfile (CCSContext *context,
	       char       *profile)
{
    GVariant        *plugins;
    GVariant        *profiles;
    GVariant        *new_profiles;
    GVariantBuilder *new_profiles_builder;
    char            *plugin, *prof;
    GVariantIter    *iter;

    plugins = g_settings_get_value (current_profile_settings, "plugins-with-set-values");
    profiles = g_settings_get_value (compizconfig_settings, "existing-profiles");

    g_variant_iter_new (plugins);
    while (g_variant_iter_loop (iter, "s", &plugin))
    {
	GSettings *settings;
	char      pathName[BUFSIZE]; 


	/* FIXME: We should not be hardcoding to screen0 in this case */
        if (!plugin || 
	    strcmp (plugin, "core") == 0) \
            snprintf (pathName, BUFSIZE, 
		      "%s/%s/plugins/%s/%s/options/", COMPIZ, profile, plugin,
		      "screen0"); 
	else 
	    snprintf (pathName, BUFSIZE, 
		      "%s/%s/plugins/%s/%s/options/", COMPIZ, profile, plugin,
		      "screen0");

	settings = gsettings_object_for_plugin_path (plugin, pathName);

	/* The GSettings documentation says not to use this API
	 * because we should know our own schema ... though really
	 * we don't because we autogenerate schemas ... */
	if (settings)
	{
	    char **keys = g_settings_list_keys (settings);
	    char **key_ptr;

	    /* Unset all the keys */
	    for (key_ptr = keys; key_ptr; key_ptr++)
		g_settings_reset (settings, *key_ptr);

	    g_strfreev (keys);

	    g_object_unref (settings);
	}
    }

    /* Remove the profile from existing-profiles */
    g_variant_iter_free (iter);
    g_settings_reset (current_profile_settings, "plugins-with-set-values");

    iter = g_variant_iter_new (profiles);
    new_profiles_builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));

    while (g_variant_iter_loop (iter, "s", &prof))
    {
	if (g_strcmp0 (prof, profile))
	    g_variant_builder_add (new_profiles_builder, "s", prof);
    }

    new_profiles = g_variant_new ("as", new_profiles_builder);
    g_settings_set_value (compizconfig_settings, "existing-profiles", new_profiles);

    g_variant_unref (new_profiles);
    g_variant_builder_unref (new_profiles_builder);

    return TRUE;
}

static CCSBackendVTable gconfVTable = {
    "gsettings",
    "GSettings Configuration Backend",
    "GSettings Configuration Backend for libccs",
    TRUE,
    TRUE,
    processEvents,
    initBackend,
    finiBackend,
    readInit,
    readSetting,
    0,
    writeInit,
    writeSetting,
    0,
    getSettingIsIntegrated,
    getSettingIsReadOnly,
    0,
    0
};

CCSBackendVTable *
getBackendInfo (void)
{
    return &gconfVTable;
}

