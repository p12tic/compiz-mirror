/**
 *
 * GConf libccs backend
 *
 * gconf.c
 *
 * Copyright (c) 2007 Danny Baumann <maniac@beryl-project.org>
 *
 * Parts of this code are taken from libberylsettings 
 * gconf backend, written by:
 *
 * Copyright (c) 2006 Robert Carr <racarr@beryl-project.org>
 * Copyright (c) 2007 Dennis Kasprzyk <onestone@beryl-project.org>
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

#include <ccs.h>

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

#define METACITY    "/apps/metacity"
#define COMPIZ_CCS   "/apps/compiz/ccs"
#define DEFAULTPROF "Default"
#define CORE_NAME   "core"

#define BUFSIZE 512

#define KEYNAME     char keyName[BUFSIZE]; \
                    if (setting->isScreen) \
                        snprintf(keyName, BUFSIZE, "screen%d/%s", setting->screenNum, setting->name); \
                    else \
                        snprintf(keyName, BUFSIZE, "allscreens/%s", setting->name);

#define PATHNAME    char pathName[BUFSIZE]; \
					if (!setting->parent->name || strcmp(setting->parent->name, "core") == 0) \
						snprintf(pathName, BUFSIZE, "%s/%s/general/%s", COMPIZ_CCS, \
							 currentProfile, keyName); \
					else \
						snprintf(pathName, BUFSIZE, "%s/%s/plugins/%s/%s", COMPIZ_CCS, \
							 currentProfile, setting->parent->name, keyName);

GConfClient *client = NULL;

static guint backendNotifyId = 0;
static guint gnomeNotifyId = 0;
static char *currentProfile = NULL;

/* some forward declarations */
static Bool readInit(CCSContext * context);
static void readSetting(CCSContext * context, CCSSetting * setting);
static void readDone(CCSContext * context);

typedef enum {
	OptionInt,
	OptionBool,
	OptionKey,
	OptionString,
	OptionSpecial,
} SpecialOptionType;

struct _SpecialOption {
	const char* settingName;
	const char* pluginName;
	Bool		screen;
	const char* gnomeName;
	SpecialOptionType type;
} const specialOptions[] = {
	{"run", "core", FALSE, METACITY "/global_keybindings/panel_run_dialog", OptionKey},
	{"main_menu", "core", FALSE, METACITY "/global_keybindings/panel_main_menu", OptionKey},
	{"window_menu", "core", FALSE, METACITY "/window_keybindings/activate_window_menu", OptionKey},
	{"run_command_screenshot", "core", FALSE, METACITY "/global_keybindings/run_command_screenshot", OptionKey},

	{"toggle_window_maximized", "core", FALSE, METACITY "/window_keybindings/toggle_maximized", OptionKey},
	{"minimize_window", "core", FALSE, METACITY "/window_keybindings/minimize", OptionKey},
	{"maximize_window", "core", FALSE, METACITY "/window_keybindings/maximize", OptionKey},
	{"unmaximize_window", "core", FALSE, METACITY "/window_keybindings/unmaximize", OptionKey},
	{"toggle_window_maximized_horizontally", "core", FALSE, METACITY "/window_keybindings/maximize_horizontally", OptionKey},
	{"toggle_window_maximized_vertically", "core", FALSE, METACITY "/window_keybindings/maximize_vertically", OptionKey},
	{"raise_window", "core", FALSE, METACITY "/window_keybindings/raise", OptionKey},
	{"lower_window", "core", FALSE, METACITY "/window_keybindings/lower", OptionKey},
	{"close_window", "core", FALSE, METACITY "/window_keybindings/close", OptionKey},
	{"toggle_window_shaded", "core", FALSE, METACITY "/window_keybindings/toggle_shaded", OptionKey},

	{"show_desktop", "core", FALSE, METACITY "/global_keybindings/show_desktop", OptionKey},

	{"initiate", "move", FALSE, METACITY "/window_keybindings/begin_move", OptionSpecial},
	{"initiate", "resize", FALSE, METACITY "/window_keybindings/begin_resize", OptionSpecial},

	{"next", "switcher", FALSE, METACITY "/global_keybindings/switch_windows", OptionKey},
	{"prev", "switcher", FALSE, METACITY "/global_keybindings/switch_windows_backward", OptionKey},

	{"command1", "core", FALSE, METACITY "/keybinding_commands/command_1", OptionString},
	{"command2", "core", FALSE, METACITY "/keybinding_commands/command_2", OptionString},
	{"command3", "core", FALSE, METACITY "/keybinding_commands/command_3", OptionString},
	{"command4", "core", FALSE, METACITY "/keybinding_commands/command_4", OptionString},
	{"command5", "core", FALSE, METACITY "/keybinding_commands/command_5", OptionString},
	{"command6", "core", FALSE, METACITY "/keybinding_commands/command_6", OptionString},
	{"command7", "core", FALSE, METACITY "/keybinding_commands/command_7", OptionString},
	{"command8", "core", FALSE, METACITY "/keybinding_commands/command_8", OptionString},
	{"command9", "core", FALSE, METACITY "/keybinding_commands/command_9", OptionString},
	{"command10", "core", FALSE, METACITY "/keybinding_commands/command_10", OptionString},
	{"command11", "core", FALSE, METACITY "/keybinding_commands/command_11", OptionString},
	{"command12", "core", FALSE, METACITY "/keybinding_commands/command_12", OptionString},

	{"run_command1", "core", FALSE, METACITY "/global_keybindings/run_command_1", OptionKey},
	{"run_command2", "core", FALSE, METACITY "/global_keybindings/run_command_2", OptionKey},
	{"run_command3", "core", FALSE, METACITY "/global_keybindings/run_command_3", OptionKey},
	{"run_command4", "core", FALSE, METACITY "/global_keybindings/run_command_4", OptionKey},
	{"run_command5", "core", FALSE, METACITY "/global_keybindings/run_command_5", OptionKey},
	{"run_command6", "core", FALSE, METACITY "/global_keybindings/run_command_6", OptionKey},
	{"run_command7", "core", FALSE, METACITY "/global_keybindings/run_command_7", OptionKey},
	{"run_command8", "core", FALSE, METACITY "/global_keybindings/run_command_8", OptionKey},
	{"run_command9", "core", FALSE, METACITY "/global_keybindings/run_command_9", OptionKey},
	{"run_command10", "core", FALSE, METACITY "/global_keybindings/run_command_10", OptionKey},
	{"run_command11", "core", FALSE, METACITY "/global_keybindings/run_command_11", OptionKey},
	{"run_command12", "core", FALSE, METACITY "/global_keybindings/run_command_12", OptionKey},

	{"rotate_to_1", "rotate", FALSE, METACITY "/global_keybindings/switch_to_workspace_1", OptionKey},
	{"rotate_to_2", "rotate", FALSE, METACITY "/global_keybindings/switch_to_workspace_2", OptionKey},
	{"rotate_to_3", "rotate", FALSE, METACITY "/global_keybindings/switch_to_workspace_3", OptionKey},
	{"rotate_to_4", "rotate", FALSE, METACITY "/global_keybindings/switch_to_workspace_4", OptionKey},
	{"rotate_to_5", "rotate", FALSE, METACITY "/global_keybindings/switch_to_workspace_5", OptionKey},
	{"rotate_to_6", "rotate", FALSE, METACITY "/global_keybindings/switch_to_workspace_6", OptionKey},
	{"rotate_to_7", "rotate", FALSE, METACITY "/global_keybindings/switch_to_workspace_7", OptionKey},
	{"rotate_to_8", "rotate", FALSE, METACITY "/global_keybindings/switch_to_workspace_8", OptionKey},
	{"rotate_to_9", "rotate", FALSE, METACITY "/global_keybindings/switch_to_workspace_9", OptionKey},
	{"rotate_to_10", "rotate", FALSE, METACITY "/global_keybindings/switch_to_workspace_10", OptionKey},
	{"rotate_to_11", "rotate", FALSE, METACITY "/global_keybindings/switch_to_workspace_11", OptionKey},
	{"rotate_to_12", "rotate", FALSE, METACITY "/global_keybindings/switch_to_workspace_12", OptionKey},

	{"rotate_left", "rotate", FALSE, METACITY "/global_keybindings/switch_to_workspace_left", OptionKey},
	{"rotate_right", "rotate", FALSE, METACITY "/global_keybindings/switch_to_workspace_right", OptionKey},

	{"plane_to_1", "plane", FALSE, METACITY "/global_keybindings/switch_to_workspace_1", OptionKey},
	{"plane_to_2", "plane", FALSE, METACITY "/global_keybindings/switch_to_workspace_2", OptionKey},
	{"plane_to_3", "plane", FALSE, METACITY "/global_keybindings/switch_to_workspace_3", OptionKey},
	{"plane_to_4", "plane", FALSE, METACITY "/global_keybindings/switch_to_workspace_4", OptionKey},
	{"plane_to_5", "plane", FALSE, METACITY "/global_keybindings/switch_to_workspace_5", OptionKey},
	{"plane_to_6", "plane", FALSE, METACITY "/global_keybindings/switch_to_workspace_6", OptionKey},
	{"plane_to_7", "plane", FALSE, METACITY "/global_keybindings/switch_to_workspace_7", OptionKey},
	{"plane_to_8", "plane", FALSE, METACITY "/global_keybindings/switch_to_workspace_8", OptionKey},
	{"plane_to_9", "plane", FALSE, METACITY "/global_keybindings/switch_to_workspace_9", OptionKey},
	{"plane_to_10", "plane", FALSE, METACITY "/global_keybindings/switch_to_workspace_10", OptionKey},
	{"plane_to_11", "plane", FALSE, METACITY "/global_keybindings/switch_to_workspace_11", OptionKey},
	{"plane_to_12", "plane", FALSE, METACITY "/global_keybindings/switch_to_workspace_12", OptionKey},

	{"up", "wall", FALSE, METACITY "/global_keybindings/switch_to_workspace_up", OptionKey},
	{"down", "wall", FALSE, METACITY "/global_keybindings/switch_to_workspace_down", OptionKey},
	{"left", "wall", FALSE, METACITY "/global_keybindings/switch_to_workspace_left", OptionKey},
	{"right", "wall", FALSE, METACITY "/global_keybindings/switch_to_workspace_right", OptionKey},

	{"rotate_to_1_window", "rotate", FALSE, METACITY "/window_keybindings/move_to_workspace_1", OptionKey},
	{"rotate_to_2_window", "rotate", FALSE, METACITY "/window_keybindings/move_to_workspace_2", OptionKey},
	{"rotate_to_3_window", "rotate", FALSE, METACITY "/window_keybindings/move_to_workspace_3", OptionKey},
	{"rotate_to_4_window", "rotate", FALSE, METACITY "/window_keybindings/move_to_workspace_4", OptionKey},
	{"rotate_to_5_window", "rotate", FALSE, METACITY "/window_keybindings/move_to_workspace_5", OptionKey},
	{"rotate_to_6_window", "rotate", FALSE, METACITY "/window_keybindings/move_to_workspace_6", OptionKey},
	{"rotate_to_7_window", "rotate", FALSE, METACITY "/window_keybindings/move_to_workspace_7", OptionKey},
	{"rotate_to_8_window", "rotate", FALSE, METACITY "/window_keybindings/move_to_workspace_8", OptionKey},
	{"rotate_to_9_window", "rotate", FALSE, METACITY "/window_keybindings/move_to_workspace_9", OptionKey},
	{"rotate_to_10_window", "rotate", FALSE, METACITY "/window_keybindings/move_to_workspace_10", OptionKey},
	{"rotate_to_11_window", "rotate", FALSE, METACITY "/window_keybindings/move_to_workspace_11", OptionKey},
	{"rotate_to_12_window", "rotate", FALSE, METACITY "/window_keybindings/move_to_workspace_12", OptionKey},

	{"rotate_left_window", "rotate", FALSE, METACITY "/window_keybindings/move_to_workspace_left", OptionKey},
	{"rotate_right_window", "rotate", FALSE, METACITY "/window_keybindings/move_to_workspace_right", OptionKey},

	{"command_screenshot", "core", FALSE, METACITY "/keybinding_commands/command_screenshot", OptionString},
	{"command_window_screenshot", "core", FALSE, METACITY "/keybinding_commands/command_window_screenshot", OptionString},

	{"autoraise", "core", FALSE, METACITY "/general/auto_raise", OptionBool},
	{"autoraise_delay", "core", FALSE, METACITY "/general/auto_raise_delay", OptionInt},
	{"raise_on_click", "core", FALSE, METACITY "/general/raise_on_click", OptionBool},
	{"click_to_focus", "core", FALSE, METACITY "/general/focus_mode", OptionSpecial},

	{"initiate", "move", FALSE, METACITY "/general/mouse_button_modifier", OptionSpecial},
	{"initiate", "resize", FALSE, METACITY "/general/mouse_button_modifier", OptionSpecial},
	{"window_menu", "core", FALSE, METACITY "/general/mouse_button_modifier", OptionSpecial},

	{"audible_bell", "core", FALSE, METACITY "/general/audible_bell", OptionBool},
	{"size", "core", TRUE, METACITY "/general/num_workspaces", OptionInt},
};

#define N_SOPTIONS (sizeof (specialOptions) / sizeof (struct _SpecialOption))

static Bool isIntegratedOption(CCSSetting * setting, int * index)
{
	unsigned int i;

	for (i = 0; i < N_SOPTIONS; i++)
	{
		if ((strcmp(setting->name, specialOptions[i].settingName) == 0) &&
			((!setting->parent->name && !specialOptions[i].pluginName) ||
			 (setting->parent->name && specialOptions[i].pluginName &&
			  (strcmp(setting->parent->name, specialOptions[i].pluginName) == 0))) &&
			((setting->isScreen && specialOptions[i].screen) || 
			 (!setting->isScreen && !specialOptions[i].screen)))
		{
			if (index)
				*index = i;
			return TRUE;
		}
	}
	return FALSE;
}

static void valueChanged(GConfClient *client, guint cnxn_id, GConfEntry *entry,
				 		 gpointer user_data)
{
	CCSContext *context = (CCSContext *)user_data;

	char *keyName = (char*) gconf_entry_get_key(entry);
	char *pluginName;
	char *screenName;
	char *settingName;
	Bool isScreen;
	unsigned int screenNum;

	keyName += strlen(COMPIZ_CCS) + 1;

	pluginName = keyName;
	keyName = strchr(keyName, '/');
	*keyName = 0;
	keyName++;

	if (strcmp(pluginName, "general") == 0)
	{
		pluginName = NULL;
	} else {
		pluginName = keyName;
	}

	screenName = keyName;
	keyName = strchr(keyName, '/');
	*keyName = 0;
	keyName++;

	settingName = keyName;

	CCSPlugin *plugin = ccsFindPlugin(context, pluginName);
	if (!plugin)
		return;

	if (strcmp(screenName, "allscreens") == 0)
		isScreen = FALSE;
	else
	{
		isScreen = TRUE;
		sscanf(screenName, "screen%d", &screenNum);
	}

	CCSSetting *setting = ccsFindSetting(plugin, settingName, isScreen, screenNum);
	if (!setting)
		return;

	if (ccsGetIntegrationEnabled(context) && !isIntegratedOption(setting, NULL))
	{
		readInit(context);
		readSetting(context, setting);
		readDone(context);
	}
}

static void gnomeValueChanged(GConfClient *client, guint cnxn_id, GConfEntry *entry,
					  		  gpointer user_data)
{
	CCSContext *context = (CCSContext *)user_data;
	char *keyName = (char*) gconf_entry_get_key(entry);
	int i,num = -1;

	for (i = 0; i < N_SOPTIONS; i++)
	{
		if (strcmp(specialOptions[i].gnomeName, keyName) == 0)
		{
			num = i;
			break;
		}
	}

	if (num < 0)
		return;

	CCSPlugin * plugin = NULL;
	plugin = ccsFindPlugin(context, (char*) specialOptions[num].pluginName);

	if (!plugin)
		return;

	CCSSetting * setting = NULL;
	/* FIXME: where should we get the screen num from? */
	setting = ccsFindSetting(plugin, (char*) specialOptions[num].settingName, 
							specialOptions[num].screen, 0);

	if (!setting)
		return;

	readInit(context);
	readSetting(context, setting);
	readDone(context);
}

static Bool readActionValue(CCSSetting * setting, char * pathName)
{
	char itemPath[BUFSIZE];
	GError *err = NULL;
	Bool ret = FALSE;
	GConfValue *gconfValue;

	CCSSettingActionValue action;
	memset(&action, 0, sizeof(CCSSettingActionValue));

	snprintf(itemPath, 512, "%s/bell", pathName);
	gconfValue = gconf_client_get(client, itemPath, &err);
	if (!err && gconfValue)
	{
		action.onBell = gconf_value_get_bool(gconfValue);
		ret = TRUE;
	}
	if (err)
	{
		g_error_free(err);
		err = NULL;
	}
	if (gconfValue)
		gconf_value_free(gconfValue);

	snprintf(itemPath, 512, "%s/edge", pathName);
	gconfValue = gconf_client_get(client, itemPath, &err);
	if (!err && gconfValue)
	{
		const char* buffer;
		buffer = gconf_value_get_string(gconfValue);
		if (buffer)
		{
			ccsStringToEdge(buffer, &action);
			ret = TRUE;
		}
	}
	if (err)
	{
		g_error_free(err);
		err = NULL;
	}
	if (gconfValue)
		gconf_value_free(gconfValue);

	snprintf(itemPath, 512, "%s/edgebutton", pathName);
	gconfValue = gconf_client_get(client, itemPath, &err);
	if (!err && gconfValue) 
	{
		action.edgeButton = gconf_value_get_int(gconfValue);
		ret = TRUE;
	}
	if (err)
	{
		g_error_free(err);
		err = NULL;
	}
	if (gconfValue)
		gconf_value_free(gconfValue);

	snprintf(itemPath, 512, "%s/key", pathName);
	gconfValue = gconf_client_get(client, itemPath, &err);
	if (!err && gconfValue) 
	{
		const char* buffer;
		buffer = gconf_value_get_string(gconfValue);
		if (buffer)
		{
			ccsStringToKeyBinding(buffer, &action);
			ret = TRUE;
		}
	}
	if (err)
	{
		g_error_free(err);
		err = NULL;
	}
	if (gconfValue)
		gconf_value_free(gconfValue);

	snprintf(itemPath, 512, "%s/button", pathName);
	gconfValue = gconf_client_get(client, itemPath, &err);
	if (!err && gconfValue) 
	{
		const char* buffer;
		buffer = gconf_value_get_string(gconfValue);
		if (buffer)
		{
			ccsStringToButtonBinding(buffer, &action);
			ret = TRUE;
		}
	}
	if (err)
	{
		g_error_free(err);
		err = NULL;
	}
	if (gconfValue)
		gconf_value_free(gconfValue);

	if (ret) 
		ccsSetAction(setting, action);

	return ret;
}

static Bool readListValue(CCSSetting * setting, GConfValue * gconfValue)
{
	GConfValueType valueType;
	unsigned int nItems, i = 0;
	CCSSettingValueList list = NULL;
	GSList *valueList = NULL;

	switch (setting->info.forList.listType)
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

	if (valueType != gconf_value_get_list_type(gconfValue))
		return FALSE;

	valueList = gconf_value_get_list(gconfValue);

	if (!valueList)
		return FALSE;

	nItems = g_slist_length(valueList);

	switch (setting->info.forList.listType)
	{
		case TypeBool:
			{
				Bool *array = malloc(nItems * sizeof(Bool));
				for (; valueList; valueList = valueList->next, i++)
					array[i] = (GPOINTER_TO_INT(valueList->data)) ? TRUE : FALSE;
				list = ccsGetValueListFromBoolArray(array, nItems, setting);
				free(array);
			}
			break;
		case TypeInt:
			{
				int *array = malloc(nItems * sizeof(int));
				for (; valueList; valueList = valueList->next, i++)
					array[i] = GPOINTER_TO_INT(valueList->data);
				list = ccsGetValueListFromIntArray(array, nItems, setting);
				free(array);
			}
			break;
		case TypeFloat:
			{
				float *array = malloc(nItems * sizeof(float));
				for (; valueList; valueList = valueList->next, i++)
					array[i] = *((gdouble*)valueList->data);
				list = ccsGetValueListFromFloatArray(array, nItems, setting);
				free(array);
			}
			break;
		case TypeString:
		case TypeMatch:
			{
				char **array = malloc(nItems * sizeof(char*));
				for (; valueList; valueList = valueList->next, i++)
					array[i] = strdup(valueList->data);
				list = ccsGetValueListFromStringArray(array, nItems, setting);
				for (i = 0; i < nItems; i++)
					free(array[i]);
				free(array);
			}
			break;
		case TypeColor:
			{
				CCSSettingColorValue *array = malloc(nItems * sizeof(CCSSettingColorValue));
				for (; valueList; valueList = valueList->next, i++)
				{
					memset(&array[i], 0, sizeof(CCSSettingColorValue));
					ccsStringToColor(valueList->data, &array[i]);
				}
				list = ccsGetValueListFromColorArray(array, nItems, setting);
				free(array);
			}
			break;
		default:
			break;
	}

	if (list)
	{
		ccsSetList(setting, list);
		ccsSettingValueListFree(list, TRUE);
		return TRUE;
	}

	return FALSE;
}

static Bool readIntegratedOption(CCSContext * context, CCSSetting * setting, int index)
{
	GError *err = NULL;
	Bool ret = FALSE;

	switch (specialOptions[index].type)
	{
		case OptionInt:
			{
				guint value;
				value = gconf_client_get_int(client, specialOptions[index].gnomeName, &err);

				if (!err)
				{
					ccsSetInt(setting, value);
					ret = TRUE;
				}
			}
			break;
		case OptionBool:
			{
				gboolean value;
				value = gconf_client_get_bool(client, specialOptions[index].gnomeName, &err);

				if (!err)
				{
					ccsSetBool(setting, value ? TRUE : FALSE);
					ret = TRUE;
				}
			}
			break;
		case OptionString:
			{
				char *value;
				value = gconf_client_get_string(client, specialOptions[index].gnomeName, &err);

				if (!err && value)
				{
					ccsSetString(setting, value);
					ret = TRUE;
					g_free(value);
				}
			}
			break;
		case OptionKey:
			{
				char *value;
				value = gconf_client_get_string(client, specialOptions[index].gnomeName, &err);

				if (!err && value)
				{
					CCSSettingActionValue action;
					if (ccsStringToKeyBinding(value, &action))
					{
						ccsSetAction(setting, action);
						ret = TRUE;
					}
					g_free(value);
				}
			}
			break;
		case OptionSpecial:
			if (strcmp(specialOptions[index].settingName, "click_to_focus") == 0)
			{
				char *focusMode;
				focusMode = gconf_client_get_string(client, specialOptions[index].gnomeName, &err);

				if (!err && focusMode)
				{
					Bool clickToFocus = (strcmp(focusMode, "click") == 0);
					ccsSetBool(setting, clickToFocus);
					g_free(focusMode);
				}
			}
			else if ((strcmp(specialOptions[index].gnomeName, METACITY "/general/mouse_button_modifier") == 0) ||
				 ((strcmp(specialOptions[index].settingName, "initiate") == 0) &&
				  ((strcmp(specialOptions[index].pluginName, "move") == 0) || 
				   (strcmp(specialOptions[index].pluginName, "resize") == 0))))
			{
				char *modifier = NULL, *moveBinding = NULL, *resizeBinding = NULL;
				int modIndex = -1, moveIndex = -1, resizeIndex = -1, i;

				for (i = 0; i < N_SOPTIONS; i++)
				{
					if (strcmp(METACITY "/general/mouse_button_modifier", specialOptions[i].gnomeName) == 0)
						modIndex = i;
					if (strcmp(METACITY "/window_keybindings/begin_move", specialOptions[i].gnomeName) == 0)
						moveIndex = i;
					if (strcmp(METACITY "/window_keybindings/begin_resize", specialOptions[i].gnomeName) == 0)
						resizeIndex = i;
				}

				if ((modIndex != -1) && (moveIndex != -1) && (resizeIndex != -1))
				{
					modifier = gconf_client_get_string(client, specialOptions[modIndex].gnomeName, &err);
					if (!err)
						moveBinding = gconf_client_get_string(client, specialOptions[moveIndex].gnomeName, &err);
					if (!err)
						resizeBinding = gconf_client_get_string(client, specialOptions[resizeIndex].gnomeName, &err);

					if (!err)
					{
						unsigned int modMask;
						CCSPlugin *p;
						CCSSetting *s;
						CCSSettingActionValue action;

						modMask = (modifier) ? ccsStringToModifiers(modifier) : 0;

						/* set move binding */
						p = ccsFindPlugin (context, "move");
						if (p)
						{
							s = ccsFindSetting (p, "initiate", FALSE, 0);
							if (s)
							{
								action = s->value->value.asAction;
								if (moveBinding)
									ccsStringToKeyBinding (moveBinding, &action);
								action.buttonModMask = modMask;
								ccsSetAction (s, action);
							}
						}
	
						/* set resize binding */
						p = ccsFindPlugin (context, "resize");
						if (p)
						{
							s = ccsFindSetting (p, "initiate", FALSE, 0);
							if (s)
							{
								action = s->value->value.asAction;
								if (resizeBinding)
									ccsStringToKeyBinding (resizeBinding, &action);
								action.buttonModMask = modMask;
								ccsSetAction (s, action);
							}
						}
	
						/* set window menu binding */
						p = ccsFindPlugin (context, "core");	
						if (p)
						{
							s = ccsFindSetting (p, "window_menu", FALSE, 0);
							if (s)
							{
								action = s->value->value.asAction;
								action.buttonModMask = modMask;
								ccsSetAction (s, action);
							}
						}
					}
					if (modifier)
						g_free (modifier);
					if (moveBinding)
						g_free (moveBinding);
					if (resizeBinding)
						g_free (resizeBinding);
				}
			}
			break;
		default:
			break;
	}

	if (err)
		g_error_free(err);

	return ret;
}

static Bool readOption(CCSSetting * setting)
{
	GConfValue *gconfValue = NULL;
	GError *err = NULL;
	Bool ret = FALSE;
	KEYNAME;
	PATHNAME;

	/* first check if the key is set, but only if the setting
	   type is not action - actions are in a subtree and handled
	   separately */
	if (setting->type != TypeAction)
	{
		gconfValue = gconf_client_get_without_default(client, pathName, &err);
		if (err)
		{
			g_error_free(err);
			return FALSE;
		}
		if (!gconfValue)
			/* value is not set */
			return FALSE;
	}

	switch (setting->type)
	{
		case TypeString:
			{
				const char *value;
				value = gconf_value_get_string(gconfValue);
				if (value)
				{
					ccsSetString(setting, value);
					ret = TRUE;
				}
			}
			break;
		case TypeMatch:
			{
				const char * value;
				value = gconf_value_get_string(gconfValue);
				if (value)
				{
					ccsSetMatch(setting, value);
					ret = TRUE;
				}
			}
			break;
		case TypeInt:
			{
				int value;
				value = gconf_value_get_int(gconfValue);

				ccsSetInt(setting, value);
				ret = TRUE;
			}
			break;
		case TypeBool:
			{
				gboolean value;
				value = gconf_value_get_bool(gconfValue);

				ccsSetBool(setting, value ? TRUE : FALSE);
				ret = TRUE;
			}
			break;
		case TypeFloat:
			{
				double value;
				value = gconf_value_get_float(gconfValue);

				ccsSetFloat(setting, (float)value);
				ret = TRUE;
			}
			break;
		case TypeColor:
			{
				const char *value;
				CCSSettingColorValue color;
				value = gconf_value_get_string(gconfValue);

				if (value && ccsStringToColor(value, &color))
				{
					ccsSetColor(setting, color);
					ret = TRUE;
				}
			}
			break;
		case TypeList:
			ret = readListValue(setting, gconfValue);
			break;
		case TypeAction:
			ret = readActionValue(setting, pathName);
			break;
		default:
			printf("GConf backend: attempt to read unsupported setting type %d!\n", setting->type);
			break;
	}

	if (gconfValue)
		gconf_value_free(gconfValue);

	return ret;
}

static void writeActionValue(CCSSettingActionValue * action, char * pathName)
{
	char *buffer;
	char itemPath[BUFSIZE];

	snprintf(itemPath, BUFSIZE, "%s/edge", pathName);
	buffer = ccsEdgeToString(action);
	if (buffer)
	{
		gconf_client_set_string(client, itemPath, buffer, NULL);
		free(buffer);
	}

	snprintf(itemPath, BUFSIZE, "%s/bell", pathName);
	gconf_client_set_bool(client, itemPath, action->onBell, NULL);

	snprintf(itemPath, BUFSIZE, "%s/edgebutton", pathName);
	gconf_client_set_int(client, itemPath, action->edgeButton, NULL);

	snprintf(itemPath, BUFSIZE, "%s/button", pathName);
	buffer = ccsButtonBindingToString(action);
	if (buffer)
	{
		gconf_client_set_string(client, itemPath, buffer, NULL);
		free(buffer);
	}

	snprintf(itemPath, BUFSIZE, "%s/key", pathName);
	buffer = ccsKeyBindingToString(action);
	if (buffer)
	{
		gconf_client_set_string(client, itemPath, buffer, NULL);
		free(buffer);
	}
}

static void writeListValue(CCSSetting * setting, char * pathName)
{
	GSList *valueList = NULL;
	GConfValueType valueType;
	Bool freeItems = FALSE;

	CCSSettingValueList list;
	if (!ccsGetList(setting, &list))
		return;

	switch (setting->info.forList.listType)
	{
		case TypeBool:
			{
				while (list)
				{
					valueList = g_slist_append(valueList, 
									GINT_TO_POINTER(list->data->value.asBool));
					list = list->next;
				}
				valueType = GCONF_VALUE_BOOL;
			}
			break;
		case TypeInt:
			{
				while (list)
				{
					valueList = g_slist_append(valueList, 
									GINT_TO_POINTER(list->data->value.asInt));
					list = list->next;
				}
				valueType = GCONF_VALUE_INT;
			}
			break;
		case TypeFloat:
			{
				float *item;
				while (list)
				{
					item = malloc(sizeof(float));
					*item = list->data->value.asFloat;
					valueList = g_slist_append(valueList, item);
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
					item = ccsColorToString(&list->data->value.asColor);
					valueList = g_slist_append(valueList, item);
					list = list->next;
				}
				freeItems = TRUE;
				valueType = GCONF_VALUE_STRING;
			}
			break;
		default:
			printf("GConf backend: attempt to write unsupported list type %d!\n", setting->info.forList.listType);
			valueType = GCONF_VALUE_INVALID;
			break;
	}

	if (valueType != GCONF_VALUE_INVALID)
	{
		gconf_client_set_list(client, pathName, valueType, valueList, NULL);
		
		if (freeItems) 
		{
			GSList *tmpList = valueList;
			for (; tmpList; tmpList = tmpList->next)
				if (tmpList->data)
					free(tmpList->data);
		}
	}
	if (valueList)
		g_slist_free(valueList);
}

static void writeIntegratedOption(CCSContext * context, CCSSetting * setting, int index)
{
	GError *err = NULL;

	switch (specialOptions[index].type)
	{
		case OptionInt:
			{
				int newValue, currentValue;
				if (!ccsGetInt(setting, &newValue))
					break;
				currentValue = gconf_client_get_int(client, 
													specialOptions[index].gnomeName, &err);

				if (!err && (currentValue != newValue))
					gconf_client_set_int(client, specialOptions[index].gnomeName, 
										 newValue, NULL);
			}
			break;
		case OptionBool:
			{
				Bool newValue;
				gboolean currentValue;
				if (!ccsGetBool(setting, &newValue))
					break;
				currentValue = gconf_client_get_bool(client, 
													 specialOptions[index].gnomeName, &err);

				if (!err && ((currentValue && !newValue) || (!currentValue && newValue)))
					gconf_client_set_bool(client, specialOptions[index].gnomeName,
										  newValue, NULL);
			}
			break;
		case OptionString:
			{
				char *newValue;
				gchar *currentValue;
				if (!ccsGetString(setting, &newValue))
					break;
				currentValue = gconf_client_get_string(client,
													   specialOptions[index].gnomeName, &err);

				if (!err && currentValue)
				{
					if (strcmp(currentValue, newValue) != 0)
						gconf_client_set_string(client, specialOptions[index].gnomeName, 
												newValue, NULL);
					g_free(currentValue);
				}
			}
			break;
		case OptionKey:
			{
				char *newValue;
				gchar *currentValue;

				newValue = ccsKeyBindingToString(&setting->value->value.asAction);
				if (newValue)
				{
					currentValue = gconf_client_get_string(client, 
														   specialOptions[index].gnomeName, &err);

					if (!err && currentValue)
					{
						if (strcmp(currentValue, newValue) != 0)
							gconf_client_set_string(client, specialOptions[index].gnomeName,
													newValue, NULL);
						g_free(currentValue);
					}
					free(newValue);
				}
			}
			break;
		case OptionSpecial:
			if (strcmp(specialOptions[index].settingName, "click_to_focus") == 0)
			{
				Bool clickToFocus;
				gchar *newValue, *currentValue;
				if (!ccsGetBool(setting, &clickToFocus))
					break;

				newValue = clickToFocus ? "click" : "mouse";
				currentValue = gconf_client_get_string(client, 
													   specialOptions[index].gnomeName, &err);

				if (!err && currentValue)
				{
					if (strcmp(currentValue, newValue) != 0)
						gconf_client_set_string(client, specialOptions[index].gnomeName, 
												newValue,NULL);
					g_free(currentValue);
				}
			}
			else if (strcmp(specialOptions[index].gnomeName, METACITY "/general/mouse_button_modifier") == 0)
			{
				char *newBinding;

				newBinding = ccsKeyBindingToString(&setting->value->value.asAction);
				if (newBinding)
				{
					char *modsEnd;
					gchar *currentValue;
					modsEnd = strrchr(newBinding, '>');
					if (modsEnd)
					{
						*(modsEnd+1) = 0;
						/* newBinding now only contains the modifiers */

						currentValue = gconf_client_get_string(client, 
															   specialOptions[index].gnomeName, &err);

						/* write it to GConf */
						if (!err && currentValue)
						{
							unsigned int i;
							CCSPlugin *p;
							CCSSetting *s;

							if (strcmp(currentValue, newBinding) != 0)
								gconf_client_set_string(client, 
														specialOptions[index].gnomeName, 
														newBinding, NULL);
							g_free(currentValue);

							/* and set the other two options */

							for (i = 0; i < N_SOPTIONS; i++)
							{
								if (strcmp(METACITY "/general/mouse_button_modifier", 
										   specialOptions[i].gnomeName) == 0)
								{
									p = ccsFindPlugin (context, (char*) specialOptions[i].pluginName);
									if (p)
									{
										s = ccsFindSetting (p, (char*) specialOptions[i].settingName,
															specialOptions[i].screen, 0);
										if (s && (s != setting))
										{
											CCSSettingActionValue action;
											action = s->value->value.asAction;
											action.buttonModMask = setting->value->value.asAction.buttonModMask;
											ccsSetAction (s, action);
										}
									}
								}
							}
						}
					}
					free(newBinding);
				}
			}
			break;
	}

	if (err)
		g_error_free(err);
}

static void resetOptionToDefault(CCSSetting * setting)
{
	KEYNAME;
	PATHNAME;

	gconf_client_recursive_unset(client, pathName, 0, NULL);
	gconf_client_suggest_sync(client,NULL);
}

static void writeOption(CCSSetting * setting)
{
	KEYNAME;
	PATHNAME;

	switch (setting->type)
	{
		case TypeString:
			{
				char *value;
				if (ccsGetString(setting, &value))
					gconf_client_set_string(client, pathName, value, NULL);
			}
			break;
		case TypeMatch:
			{
				char *value;
				if (ccsGetMatch(setting, &value))
					gconf_client_set_string(client, pathName, value, NULL);
			}
		case TypeFloat:
			{
				float value;
				if (ccsGetFloat(setting, &value))
					gconf_client_set_float(client, pathName, value, NULL);
			}
			break;
		case TypeInt:
			{
				int value;
				if (ccsGetInt(setting, &value))
					gconf_client_set_int(client, pathName, value, NULL);
			}
			break;
		case TypeBool:
			{
				Bool value;
				if (ccsGetBool(setting, &value))
					gconf_client_set_bool(client, pathName, value, NULL);
			}
			break;
		case TypeColor:
			{
				CCSSettingColorValue value;
				char *colString;

				if (!ccsGetColor(setting, &value))
					break;

				colString = ccsColorToString(&value);
				if (!colString)
					break;

				gconf_client_set_string(client, pathName, colString, NULL);
				free(colString);
			}
			break;
		case TypeAction:
			{
			    CCSSettingActionValue value;
			    if (!ccsGetAction(setting, &value))
					break;

				writeActionValue(&value, pathName);
			}
			break;
		case TypeList:
			writeListValue(setting, pathName);
			break;
		default:
			printf("GConf backend: attempt to write unsupported setting type %d\n", setting->type);
			break;
	}
}

static void processEvents(void)
{
	while (g_main_context_pending(NULL))
		g_main_context_iteration(NULL, FALSE);
}

static Bool initBackend(CCSContext * context)
{
	g_type_init();

	client = gconf_client_get_default();

	backendNotifyId = gconf_client_notify_add(client, COMPIZ_CCS, valueChanged,
											  context, NULL, NULL);

	if (ccsGetIntegrationEnabled(context))
		gnomeNotifyId = gconf_client_notify_add(client, METACITY,
												gnomeValueChanged, context, NULL,NULL);

	gconf_client_add_dir(client, COMPIZ_CCS, GCONF_CLIENT_PRELOAD_NONE, NULL);
	gconf_client_add_dir(client, METACITY, GCONF_CLIENT_PRELOAD_NONE, NULL);

	return TRUE;
}

static Bool finiBackend(CCSContext * context)
{
	if (backendNotifyId)
	{
		gconf_client_notify_remove(client, backendNotifyId);
		backendNotifyId = 0;
	}
	if (gnomeNotifyId)
	{
		gconf_client_notify_remove(client, gnomeNotifyId);
		gnomeNotifyId = 0;
	}

	if (currentProfile)
	{
		free (currentProfile);
		currentProfile = NULL;
	}

	gconf_client_remove_dir(client, COMPIZ_CCS, NULL);
	gconf_client_remove_dir(client, METACITY, NULL);

	g_object_unref(client);
	client = NULL;

	return TRUE;
}

static Bool readInit(CCSContext * context)
{
	char *profile;

	if (currentProfile)
		free (currentProfile);

	profile = ccsGetProfile(context);
	if (!profile)
		currentProfile = strdup (DEFAULTPROF);
	else if (!strlen(profile))
		currentProfile = strdup (DEFAULTPROF);
	else
		currentProfile = strdup (profile);

	return TRUE;
}

static void readSetting(CCSContext * context, CCSSetting * setting)
{
	Bool status;
	int index;

	if (ccsGetIntegrationEnabled(context) && isIntegratedOption(setting, &index))
		status = readIntegratedOption(context, setting, index);
	else
		status = readOption(setting);

	if (!status)
		ccsResetToDefault(setting);
}

static void readDone(CCSContext * context)
{
}

static Bool writeInit(CCSContext * context)
{
	char *profile;

	if (currentProfile)
		free (currentProfile);

	profile = ccsGetProfile(context);
	if (!profile)
		currentProfile = strdup (DEFAULTPROF);
	else if (!strlen(profile))
		currentProfile = strdup (DEFAULTPROF);
	else
		currentProfile = strdup (profile);

	return TRUE;
}

static void writeSetting(CCSContext * context, CCSSetting * setting)
{
	int index;

	if (ccsGetIntegrationEnabled(context) && isIntegratedOption(setting, &index))
		writeIntegratedOption(context, setting, index);
	else if (setting->isDefault)
		resetOptionToDefault(setting);
	else
		writeOption(setting);

}

static void writeDone(CCSContext * context)
{
}

static Bool getSettingIsIntegrated(CCSSetting * setting)
{
	if (!ccsGetIntegrationEnabled(setting->parent->context))
		return FALSE;

	if (!isIntegratedOption(setting, NULL))
		return FALSE;

	return TRUE;
}

static Bool getSettingIsReadOnly(CCSSetting * setting)
{
	/* FIXME */
	return FALSE;
}

static CCSStringList getExistingProfiles(void)
{
	gconf_client_suggest_sync(client,NULL);
	GSList * data = gconf_client_all_dirs(client, COMPIZ_CCS, NULL);
	CCSStringList ret = NULL;
	GSList * tmp = data;
	char *name;

	for (;tmp;tmp = g_slist_next(tmp))
	{
		name = strrchr(tmp->data, '/');
		if (name && (strcmp(name+1, DEFAULTPROF) != 0))
			ret = ccsStringListAppend(ret, strdup(name+1));

		g_free(tmp->data);
	}
	g_slist_free(data);

	return ret;
}

static Bool deleteProfile(char * profile)
{
	char path[BUFSIZE];

	if (profile && strlen(profile))
		snprintf(path, BUFSIZE, "%s/%s", COMPIZ_CCS, profile);
	else
		snprintf(path, BUFSIZE, "%s/Default", COMPIZ_CCS);

	gboolean status = FALSE;
	if (gconf_client_dir_exists(client, path, NULL))
	{
		status = gconf_client_recursive_unset(client, path, 0, NULL);
		gconf_client_suggest_sync(client,NULL);
	}

	return status;
}


static CCSBackendVTable gconfVTable = {
    "gconf",
    "GConf Configuration Backend",
    "GConf Configuration Backend for libccs",
    TRUE,
    TRUE,
    processEvents,
    initBackend,
    finiBackend,
	readInit,
	readSetting,
	readDone,
	writeInit,
	writeSetting,
	writeDone,
	getSettingIsIntegrated,
	getSettingIsReadOnly,
	getExistingProfiles,
	deleteProfile
};

CCSBackendVTable *
getBackendInfo (void)
{
    return &gconfVTable;
}

