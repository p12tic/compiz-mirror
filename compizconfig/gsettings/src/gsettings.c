/**
 *
 * GSettings libccs backend
 *
 * gsettings.c
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

#include "gsettings.h"

static void
valueChanged (GSettings   *settings,
	      gchar	  *keyname,
	      gpointer    user_data);

static GList	   *settingsList = NULL;
static GSettings   *compizconfigSettings = NULL;
static GSettings   *currentProfileSettings = NULL;

char *currentProfile = NULL;

/* some forward declarations */
static void writeIntegratedOption (CCSContext *context,
				   CCSSetting *setting,
				   int        index);

typedef struct _SpecialOptionGSettings {
    const char*       settingName;
    const char*       pluginName;
    const char*       schemaName;
    const char*	      keyName;
    const char*	      type;
} SpecialOption;

static gchar *
getSchemaNameForPlugin (const char *plugin)
{
    gchar       *schemaName =  NULL;

    schemaName = g_strconcat (COMPIZ_SCHEMA_ID, ".", plugin, NULL);

    return schemaName;
}

static inline gchar *
translateKeyForGSettings (char *gsettingName)
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

static inline gchar *
translateKeyForCCS (char *gsettingName)
{
    gchar *clean        = NULL;
    gchar **delimited   = NULL;

    /* Replace dashes with underscores */
    delimited = g_strsplit (gsettingName, "-", 0);

    clean = g_strjoinv ("_", delimited);

    /* FIXME: Truncated and lowercased settings aren't going to work */

    g_strfreev (delimited);

    return clean;
}

static GSettings *
getSettingsObjectForPluginWithPath (const char *plugin,
				  const char *path,
				  CCSContext *context)
{
    GSettings *settingsObj = NULL;
    GList *l = settingsList;
    gchar *schemaName = getSchemaNameForPlugin (plugin);
    GVariant        *writtenPlugins;
    gsize            writtenPluginsLen;
    gsize            newWrittenPluginsSize;
    gchar           **newWrittenPlugins;
    char	    *plug;
    GVariantIter    iter;
    gboolean	    found = FALSE;

    while (l)
    {
	settingsObj = (GSettings *) l->data;
	gchar	  *name = NULL;

	g_object_get (G_OBJECT (settingsObj),
		      "schema",
		      &name, NULL);
	if (g_strcmp0 (name, schemaName) == 0)
	{
	    g_free (name);
	    g_free (schemaName);

	    return settingsObj;
	}

	l = g_list_next (l);
    }

    /* No existing settings object found for this schema, create one */
    
    settingsObj = g_settings_new_with_path (schemaName, path);

    g_signal_connect (G_OBJECT (settingsObj), "changed", (GCallback) valueChanged, (gpointer) context);

    settingsList = g_list_append (settingsList, (void *) settingsObj);

    /* Also write the plugin name to the list of modified plugins so
     * that when we delete the profile the keys for that profile are also
     * unset FIXME: This could be a little more efficient, like we could
     * store keys that have changed from their defaults ... though
     * gsettings doesn't seem to give you a way to get all of the schemas */

    writtenPlugins = g_settings_get_value (currentProfileSettings, "plugins-with-set-keys");

    g_variant_iter_init (&iter, writtenPlugins);
    newWrittenPluginsSize = g_variant_iter_n_children (&iter);

    while (g_variant_iter_loop (&iter, "s", &plug))
    {
	if (!found)
	    found = (g_strcmp0 (plug, plugin) == 0);
    }

    if (!found)
	newWrittenPluginsSize++;

    newWrittenPlugins = g_variant_dup_strv (writtenPlugins, &writtenPluginsLen);

    if (writtenPluginsLen > newWrittenPluginsSize)
    {
	newWrittenPlugins = g_realloc (newWrittenPlugins, (newWrittenPluginsSize + 1) * sizeof (gchar *));
	newWrittenPlugins[writtenPluginsLen + 1]  = g_strdup (plugin);
	newWrittenPlugins[newWrittenPluginsSize] = NULL;
    }

    g_settings_set_strv (currentProfileSettings, "plugins-with-set-keys", (const gchar * const *)newWrittenPlugins);

    g_free (schemaName);
    g_strfreev (newWrittenPlugins);

    return settingsObj;
}

static GSettings *
getSettingsObjectForCCSSetting (CCSSetting *setting)
{
    KEYNAME(setting->parent->context->screenNum);
    PATHNAME (setting->parent->name, keyName);

    return getSettingsObjectForPluginWithPath (setting->parent->name, pathName, setting->parent->context);
}

static Bool
isIntegratedOption (CCSSetting *setting,
		    int        *index)
{
#ifdef USE_GCONF
    return isGConfIntegratedOption (setting, index);
#else
    return FALSE;
#endif
}

static void
valueChanged (GSettings   *settings,
	      gchar	  *keyName,
	      gpointer    user_data)
{
    CCSContext   *context = (CCSContext *)user_data;
    char	 *uncleanKeyName;
    char	 *path;
    char         *token;
    int          index;
    unsigned int screenNum;
    CCSPlugin    *plugin;
    CCSSetting   *setting;

    g_object_get (G_OBJECT (settings), "path", &path, NULL);

    path += strlen (COMPIZ) + 1;

    token = strsep (&path, "/"); /* Profile name */
    if (!token)
	return;

    token = strsep (&path, "/"); /* plugins */
    if (!token)
	return;

    token = strsep (&path, "/"); /* plugin */
    if (!token)
	return;

    plugin = ccsFindPlugin (context, token);
    if (!plugin)
	return;

    token = strsep (&path, "/"); /* screen%i */
    if (!token)
	return;

    sscanf (token, "screen%d", &screenNum);

    uncleanKeyName = translateKeyForCCS (keyName);

    setting = ccsFindSetting (plugin, uncleanKeyName);
    if (!setting)
    {
	printf ("GSettings Backend: unable to find setting %s, for path %s\n", uncleanKeyName, path);
	free (uncleanKeyName);
	return;
    }

    readInit (context);
    if (!readOption (setting))
    {
	ccsResetToDefault (setting, TRUE);
    }

    if (ccsGetIntegrationEnabled (context) &&
	isIntegratedOption (setting, &index))
    {
	writeInit (context);
	writeIntegratedOption (context, setting, index);
    }

    free (uncleanKeyName);
}

static Bool
readListValue (CCSSetting *setting)
{
    GSettings		*settings = getSettingsObjectForCCSSetting (setting);
    gchar		*variantType;
    unsigned int        nItems, i = 0;
    CCSSettingValueList list = NULL;
    GVariant		*value;
    GVariantIter	iter;

    char *cleanSettingName = translateKeyForGSettings (setting->name);
    
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
	ccsSetList (setting, NULL, TRUE);
	return TRUE;
    }

    g_variant_iter_init (&iter, value);
    nItems = g_variant_iter_n_children (&iter);

    switch (setting->info.forList.listType)
    {
    case TypeBool:
	{
	    Bool *array = malloc (nItems * sizeof (Bool));
	    Bool *arrayCounter = array;
	    gboolean value;

	    if (!array)
		break;

	    /* Reads each item from the variant into arrayCounter */
	    while (g_variant_iter_loop (&iter, variantType, &value))
		*arrayCounter++ = value;

	    list = ccsGetValueListFromBoolArray (array, nItems, setting);
	    free (array);
	}
	break;
    case TypeInt:
	{
	    int *array = malloc (nItems * sizeof (int));
	    int *arrayCounter = array;
	    gint value;

	    if (!array)
		break;

	    /* Reads each item from the variant into arrayCounter */
	    while (g_variant_iter_loop (&iter, variantType, &value))
		*arrayCounter++ = value;

	    list = ccsGetValueListFromIntArray (array, nItems, setting);
	    free (array);
	}
	break;
    case TypeFloat:
	{
	    double *array = malloc (nItems * sizeof (double));
	    double *arrayCounter = array;
	    gdouble value;

	    if (!array)
		break;

	    /* Reads each item from the variant into arrayCounter */
	    while (g_variant_iter_loop (&iter, variantType, &value))
		*arrayCounter++ = value;

	    list = ccsGetValueListFromFloatArray ((float *) array, nItems, setting);
	    free (array);
	}
	break;
    case TypeString:
    case TypeMatch:
	{
	    gchar **array = calloc (1, (nItems + 1) * sizeof (gchar *));
	    gchar **arrayCounter = array;
	    gchar *value;

	    if (!array)
		break;

	    array[nItems] = NULL;

	    /* Reads each item from the variant into arrayCounter */
	    while (g_variant_iter_loop (&iter, variantType, &value))
		*arrayCounter++ = strdup (value);

	    list = ccsGetValueListFromStringArray (array, nItems, setting);
	    g_strfreev (array);
	}
	break;
    case TypeColor:
	{
	    CCSSettingColorValue *array;
	    char		 *colorValue;
	    array = malloc (nItems * sizeof (CCSSettingColorValue));
	    if (!array)
		break;

	    while (g_variant_iter_loop (&iter, variantType, &colorValue))
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

    free (cleanSettingName);
    free (variantType);

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
#ifdef USE_GCONF
    return readGConfIntegratedOption (context, setting, index);
#else
    return FALSE;
#endif
}

Bool
readOption (CCSSetting * setting)
{
    GSettings  *settings = getSettingsObjectForCCSSetting (setting);
    GVariant   *gsettingsValue = NULL;
    Bool       ret = FALSE;
    Bool       valid = TRUE;

    /* It is impossible for certain settings to have a schema,
     * such as actions and read only settings, so in that case
     * just return FALSE since compizconfig doesn't expect us
     * to read them anyways */

    if (setting->type == TypeAction ||
	ccsSettingIsReadOnly (setting))
    {
	return FALSE;
    }

    char *cleanSettingName = translateKeyForGSettings (setting->name);
    KEYNAME(setting->parent->context->screenNum);
    PATHNAME (setting->parent->name, keyName);

    /* first check if the key is set */
    gsettingsValue = g_settings_get_value (settings, cleanSettingName);

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
	free (cleanSettingName);
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
		ccsSetString (setting, value, TRUE);
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
		ccsSetMatch (setting, value, TRUE);
		ret = TRUE;
	    }
	}
	break;
    case TypeInt:
	{
	    int value;
	    value = g_variant_get_int32 (gsettingsValue);

	    ccsSetInt (setting, value, TRUE);
	    ret = TRUE;
	}
	break;
    case TypeBool:
	{
	    gboolean value;
	    value = g_variant_get_boolean (gsettingsValue);

	    ccsSetBool (setting, value ? TRUE : FALSE, TRUE);
	    ret = TRUE;
	}
	break;
    case TypeFloat:
	{
	    double value;
	    value = g_variant_get_double (gsettingsValue);

	    ccsSetFloat (setting, (float)value, TRUE);
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
		ccsSetColor (setting, color, TRUE);
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
		ccsSetKey (setting, key, TRUE);
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
		ccsSetButton (setting, button, TRUE);
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
		ccsSetEdge (setting, edges, TRUE);
		ret = TRUE;
	    }
	}
	break;
    case TypeBell:
	{
	    gboolean value;
	    value = g_variant_get_boolean (gsettingsValue);

	    ccsSetBell (setting, value ? TRUE : FALSE, TRUE);
	    ret = TRUE;
	}
	break;
    case TypeList:
	ret = readListValue (setting);
	break;
    default:
	printf("GSettings backend: attempt to read unsupported setting type %d!\n",
	       setting->type);
	break;
    }

    free (cleanSettingName);
    g_variant_unref (gsettingsValue);

    return ret;
}

static void
writeListValue (CCSSetting *setting,
		char       *pathName)
{
    GSettings  		*settings = getSettingsObjectForCCSSetting (setting);
    GVariant 		*value;
    gchar		*variantType = NULL;
    CCSSettingValueList list;

    char *cleanSettingName = translateKeyForGSettings (setting->name);
    
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
	}
	break;
    case TypeFloat:
	{
	    variantType = "ad";
	    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("ad"));
	    while (list)
	    {
		g_variant_builder_add (builder, "d", (gdouble) list->data->value.asFloat);
		list = list->next;
	    }
	    value = g_variant_new ("ad", builder);
	    g_variant_builder_unref (builder);
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
		g_variant_builder_add (builder, "s", item);
		list = list->next;
	    }
	    value = g_variant_new ("as", builder);
	    g_variant_builder_unref (builder);
	}
	break;
    default:
	printf("GSettings backend: attempt to write unsupported list type %d!\n",
	       setting->info.forList.listType);
	variantType = NULL;
	break;
    }

    if (variantType != NULL)
    {
	g_settings_set_value (settings, cleanSettingName, value);
	g_variant_unref (value);
    }
    
    free (cleanSettingName);
}

static void
writeIntegratedOption (CCSContext *context,
		       CCSSetting *setting,
		       int        index)
{
#ifdef USE_GCONF
    writeGConfIntegratedOption (context, setting, index);
#endif

    return;
}

static void
resetOptionToDefault (CCSSetting * setting)
{
    GSettings  *settings = getSettingsObjectForCCSSetting (setting);
  
    char *cleanSettingName = translateKeyForGSettings (setting->name);
    KEYNAME (setting->parent->context->screenNum);
    PATHNAME (setting->parent->name, keyName);

    g_settings_reset (settings, cleanSettingName);

    free (cleanSettingName);
}

void
writeOption (CCSSetting * setting)
{
    GSettings  *settings = getSettingsObjectForCCSSetting (setting);
    char *cleanSettingName = translateKeyForGSettings (setting->name);
    KEYNAME (setting->parent->context->screenNum);
    PATHNAME (setting->parent->name, keyName);

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

	    g_settings_set (settings, cleanSettingName, "s", colString, NULL);
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

    free (cleanSettingName);
}

static void
updateCurrentProfileName (char *profile)
{
    GVariant        *profiles;
    char	    *prof;
    char	    *profilePath = COMPIZ_PROFILEPATH;
    char	    *currentProfilePath;
    GVariant        *newProfiles;
    GVariantBuilder *newProfilesBuilder;
    GVariantIter    iter;
    gboolean        found = FALSE;

    profiles = g_settings_get_value (compizconfigSettings, "existing-profiles");

    newProfilesBuilder = g_variant_builder_new (G_VARIANT_TYPE ("as"));

    g_variant_iter_init (&iter, profiles);
    while (g_variant_iter_loop (&iter, "s", &prof))
    {
	g_variant_builder_add (newProfilesBuilder, "s", prof);

	if (!found)
	    found = (g_strcmp0 (prof, profile) == 0);
    }

    if (!found)
	g_variant_builder_add (newProfilesBuilder, "s", profile);

    newProfiles = g_variant_new ("as", newProfilesBuilder);
    g_settings_set_value (compizconfigSettings, "existing-profiles", newProfiles);

    g_variant_unref (newProfiles);
    g_variant_builder_unref (newProfilesBuilder);

    /* Change the current profile and current profile settings */
    free (currentProfile);

    currentProfile = strdup (profile);
    currentProfilePath = g_strconcat (profilePath, profile, "/", NULL);
    currentProfileSettings = g_settings_new_with_path (PROFILE_SCHEMA_ID, profilePath);

    g_free (currentProfilePath);

    g_settings_set (compizconfigSettings, "current-profile", "s", profile, NULL);
}

static gboolean
updateProfile (CCSContext *context)
{
    char *profile = strdup (ccsGetProfile (context));

    if (!profile || !strlen (profile))
	profile = strdup (DEFAULTPROF);

    if (g_strcmp0 (profile, currentProfile))
	updateCurrentProfileName (profile);

    free (profile);

    return TRUE;
}

static char*
getCurrentProfileName (void)
{
    GVariant *value;
    char     *ret = NULL;

    value = g_settings_get_value (compizconfigSettings, "current-profile");

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
    const char *profilePath = PROFILEPATH;
    char       *currentProfilePath;

    g_type_init ();

    compizconfigSettings = g_settings_new (COMPIZCONFIG_SCHEMA_ID);

#ifdef USE_GCONF
    initGConfClient (context);
#endif

    currentProfile = getCurrentProfileName ();
    currentProfilePath = g_strconcat (profilePath, currentProfile, "/", NULL);
    currentProfileSettings = g_settings_new_with_path (PROFILE_SCHEMA_ID, currentProfilePath);

    g_free (currentProfilePath);

    return TRUE;
}

static Bool
finiBackend (CCSContext * context)
{
    GList *l = settingsList;

    processEvents (0);

#ifdef USE_GCONF
    gconf_client_clear_cache (client);
    finiGConfClient ();
#endif

    if (currentProfile)
    {
	free (currentProfile);
	currentProfile = NULL;
    }

    while (l)
    {
	g_object_unref (G_OBJECT (l->data));
	l = g_list_next (l);
    }

    g_object_unref (G_OBJECT (compizconfigSettings));

    processEvents (0);
    return TRUE;
}

Bool
readInit (CCSContext * context)
{
    return updateProfile (context);
}

void
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
	ccsResetToDefault (setting, TRUE);
}

Bool
writeInit (CCSContext * context)
{
    return updateProfile (context);
}

void
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
    char	  *profile;
    GVariantIter  iter;
    CCSStringList ret = NULL;

    value = g_settings_get_value (compizconfigSettings,  "existing-profiles");
    g_variant_iter_init (&iter, value);
    while (g_variant_iter_loop (&iter, "s", &profile))
    {
	CCSString *str = malloc (sizeof (CCSString));
	
	str->value = strdup (profile);

	ret = ccsStringListAppend (ret, str);
    }

    g_variant_unref (value);

    return ret;
}

static Bool
deleteProfile (CCSContext *context,
	       char       *profile)
{
    GVariant        *plugins;
    GVariant        *profiles;
    GVariant        *newProfiles;
    GVariantBuilder *newProfilesBuilder;
    char            *plugin, *prof;
    GVariantIter    iter;
    char            *profileSettingsPath = g_strconcat (PROFILEPATH, profile, "/", NULL);
    GSettings       *profileSettings = g_settings_new_with_path (PROFILE_SCHEMA_ID, profileSettingsPath);

    plugins = g_settings_get_value (currentProfileSettings, "plugins-with-set-keys");
    profiles = g_settings_get_value (compizconfigSettings, "existing-profiles");

    g_variant_iter_init (&iter, plugins);
    while (g_variant_iter_loop (&iter, "s", &plugin))
    {
	GSettings *settings;

	KEYNAME (context->screenNum);
	PATHNAME (plugin, keyName);

	settings = getSettingsObjectForPluginWithPath (plugin, pathName, context);

	/* The GSettings documentation says not to use this API
	 * because we should know our own schema ... though really
	 * we don't because we autogenerate schemas ... */
	if (settings)
	{
	    char **keys = g_settings_list_keys (settings);
	    char **key_ptr;

	    /* Unset all the keys */
	    for (key_ptr = keys; *key_ptr; key_ptr++)
		g_settings_reset (settings, *key_ptr);

	    g_strfreev (keys);
	}
    }

    /* Remove the profile from existing-profiles */
    g_settings_reset (profileSettings, "plugins-with-set-values");

    g_variant_iter_init (&iter, profiles);
    newProfilesBuilder = g_variant_builder_new (G_VARIANT_TYPE ("as"));

    while (g_variant_iter_loop (&iter, "s", &prof))
    {
	if (g_strcmp0 (prof, profile))
	    g_variant_builder_add (newProfilesBuilder, "s", prof);
    }

    newProfiles = g_variant_new ("as", newProfilesBuilder);
    g_settings_set_value (compizconfigSettings, "existing-profiles", newProfiles);

    g_variant_unref (newProfiles);
    g_variant_builder_unref (newProfilesBuilder);

    free (profileSettingsPath);

    updateProfile (context);

    return TRUE;
}

static CCSBackendVTable gsettingsVTable = {
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
    getExistingProfiles,
    deleteProfile
};

CCSBackendVTable *
getBackendInfo (void)
{
    return &gsettingsVTable;
}

