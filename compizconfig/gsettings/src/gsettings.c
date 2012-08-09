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

#define CCS_LOG_DOMAIN "gsettings"

#include "gsettings.h"
#include "gconf-integration.h"
#include "ccs_gsettings_interface.h"
#include "ccs_gsettings_interface_wrapper.h"

static void
valueChanged (GSettings   *settings,
	      gchar	  *keyname,
	      gpointer    user_data);

static GList	   *settingsList = NULL;
static CCSGSettingsWrapper *compizconfigSettings = NULL;
static CCSGSettingsWrapper *currentProfileSettings = NULL;
static CCSIntegration *integration = NULL;

char *currentProfile = NULL;

/* some forward declarations */
static void writeIntegratedOption (CCSContext *context,
				   CCSSetting *setting,
				   int        index);

static CCSGSettingsWrapper *
getSettingsObjectForPluginWithPath (const char *plugin,
				  const char *path,
				  CCSContext *context)
{
    CCSGSettingsWrapper *settingsObj = NULL;
    gchar *schemaName = getSchemaNameForPlugin (plugin);
    GVariant        *writtenPlugins;
    gsize            newWrittenPluginsSize;
    gchar           **newWrittenPlugins;

    settingsObj = findCCSGSettingsWrapperBySchemaName (schemaName, settingsList);

    if (settingsObj)
    {
	g_free (schemaName);
	return settingsObj;
    }

    /* No existing settings object found for this schema, create one */
    
    settingsObj = ccsGSettingsWrapperNewForSchemaWithPath (schemaName, path, &ccsDefaultObjectAllocator);
    ccsGSettingsWrapperConnectToChangedSignal (settingsObj, (GCallback) valueChanged, (gpointer) context);
    settingsList = g_list_append (settingsList, (void *) settingsObj);

    /* Also write the plugin name to the list of modified plugins so
     * that when we delete the profile the keys for that profile are also
     * unset FIXME: This could be a little more efficient, like we could
     * store keys that have changed from their defaults ... though
     * gsettings doesn't seem to give you a way to get all of the schemas */

    writtenPlugins = ccsGSettingsWrapperGetValue (currentProfileSettings, "plugins-with-set-keys");

    appendToPluginsWithSetKeysList (plugin, writtenPlugins, &newWrittenPlugins, &newWrittenPluginsSize);

    GVariant *newWrittenPluginsVariant = g_variant_new_strv ((const gchar * const *) newWrittenPlugins, newWrittenPluginsSize);

    ccsGSettingsWrapperSetValue (currentProfileSettings, "plugins-with-set-keys", newWrittenPluginsVariant);


    g_free (schemaName);
    g_strfreev (newWrittenPlugins);

    return settingsObj;
}

static gchar *
makeSettingPath (CCSSetting *setting)
{
    return makeCompizPluginPath (currentProfile,
                             ccsPluginGetName (ccsSettingGetParent (setting)));
}

static CCSGSettingsWrapper *
getSettingsObjectForCCSSetting (CCSSetting *setting)
{
    CCSGSettingsWrapper *ret = NULL;
    gchar *pathName = makeSettingPath (setting);

    ret = getSettingsObjectForPluginWithPath (ccsPluginGetName (ccsSettingGetParent (setting)),
					       pathName,
					       ccsPluginGetContext (ccsSettingGetParent (setting)));

    g_free (pathName);
    return ret;
}

static Bool
isIntegratedOption (CCSSetting *setting,
		    int        *index)
{
    const char *settingName = ccsSettingGetName (setting);
    CCSPlugin  *plugin      = ccsSettingGetParent (setting);
    const char *pluginName  = ccsPluginGetName (plugin);

    int indexTmp =  ccsIntegrationGetIntegratedOptionIndex (integration, pluginName, settingName);

    if (index)
	*index = indexTmp;

    return indexTmp != -1;
}

static void
updateSetting (CCSBackend *backend, CCSContext *context, CCSPlugin *plugin, CCSSetting *setting)
{
    int          index = -1;

    readInit (backend, context);
    if (!readOption (setting))
    {
	ccsResetToDefault (setting, TRUE);
    }

    if (ccsGetIntegrationEnabled (context) &&
	isIntegratedOption (setting, &index))
    {
	writeInit (backend, context);
	writeIntegratedOption (context, setting, index);
    }
}

static void
valueChanged (GSettings   *settings,
	      gchar	  *keyName,
	      gpointer    user_data)
{
    GValue       schemaNameValue = G_VALUE_INIT;
    CCSContext   *context = (CCSContext *)user_data;
    char	 *uncleanKeyName;
    char	 *path;
    const char   *pathOrig;
    char         *pluginName;
    unsigned int screenNum;
    CCSPlugin    *plugin;
    CCSSetting   *setting;

    g_value_init (&schemaNameValue, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (settings), "schema-id", &schemaNameValue);

    const char *schemaName = g_value_get_string (&schemaNameValue);
    CCSGSettingsWrapper *wrapper = findCCSGSettingsWrapperBySchemaName (schemaName, settingsList);

    pathOrig = ccsGSettingsWrapperGetPath (wrapper);
    path = strdup (pathOrig);

    if (!decomposeGSettingsPath (path, &pluginName, &screenNum))
    {
	g_value_unset (&schemaNameValue);
	g_free (path);
	return;
    }

    plugin = ccsFindPlugin (context, pluginName);

    uncleanKeyName = translateKeyForCCS (keyName);

    setting = ccsFindSetting (plugin, uncleanKeyName);
    if (!setting)
    {
	/* Couldn't find setting straight off the bat,
	 * try and find the best match */
	GVariant *value = ccsGSettingsWrapperGetValue (wrapper, keyName);

	if (value)
	{
	    GList *possibleSettingTypes = variantTypeToPossibleSettingType (g_variant_get_type_string (value));
	    GList *iter = possibleSettingTypes;

	    while (iter)
	    {
		setting = attemptToFindCCSSettingFromLossyName (ccsGetPluginSettings (plugin),
								keyName,
								(CCSSettingType) GPOINTER_TO_INT (iter->data));

		if (setting)
		    break;

		iter = iter->next;
	    }

	    g_list_free (possibleSettingTypes);
	    g_variant_unref (value);
	}

	/* We hit a situation where either the key stored in GSettings couldn't be
	 * matched at all to a key in the xml file, or where there were multiple matches.
	 * Unfortunately, there isn't much we can do about this, other than try
	 * and warn the user and bail out. It just means that if the key was updated
	 * externally we won't know about the change until the next reload of settings */
	if (!setting)
	{
	    ccsWarning ("Unable to find setting %s, for path %s", uncleanKeyName, path);
	    g_value_unset (&schemaNameValue);
	    g_free (pluginName);
	    free (uncleanKeyName);
	    g_free (path);
	    return;
	}
    }

    updateSetting (NULL, context, plugin, setting);

    g_value_unset (&schemaNameValue);
    g_free (pluginName);
    free (uncleanKeyName);
    g_free (path);
}

static Bool
readListValue (CCSSetting *setting)
{
    CCSGSettingsWrapper	*settings = getSettingsObjectForCCSSetting (setting);
    gboolean		hasVariantType;
    unsigned int        nItems, i = 0;
    CCSSettingValueList list = NULL;
    GVariant		*value;
    GVariantIter	iter;

    char *cleanSettingName = translateKeyForGSettings (ccsSettingGetName (setting));

    hasVariantType = compizconfigTypeHasVariantType (ccsSettingGetInfo (setting)->forList.listType);

    if (!hasVariantType)
	return FALSE;

    value = ccsGSettingsWrapperGetValue (settings, cleanSettingName);
    if (!value)
    {
	ccsSetList (setting, NULL, TRUE);
	return TRUE;
    }

    g_variant_iter_init (&iter, value);
    nItems = g_variant_iter_n_children (&iter);

    switch (ccsSettingGetInfo (setting)->forList.listType)
    {
    case TypeBool:
	{
	    Bool *array = malloc (nItems * sizeof (Bool));
	    Bool *arrayCounter = array;
	    gboolean value;

	    if (!array)
		break;

	    /* Reads each item from the variant into arrayCounter */
	    while (g_variant_iter_loop (&iter, "b", &value))
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
	    while (g_variant_iter_loop (&iter, "i", &value))
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
	    while (g_variant_iter_loop (&iter, "d", &value))
		*arrayCounter++ = value;

	    list = ccsGetValueListFromFloatArray ((float *) array, nItems, setting);
	    free (array);
	}
	break;
    case TypeString:
    case TypeMatch:
	{
	    const gchar **array = g_malloc0 ((nItems + 1) * sizeof (gchar *));
	    const gchar **arrayCounter = array;
	    gchar *value;

	    if (!array)
		break;

	    array[nItems] = NULL;

	    /* Reads each item from the variant into arrayCounter */
	    while (g_variant_iter_next (&iter, "s", &value))
		*arrayCounter++ = value;

	    list = ccsGetValueListFromStringArray (array, nItems, setting);
	    g_strfreev ((char **) array);
	}
	break;
    case TypeColor:
	{
	    CCSSettingColorValue *array;
	    char		 *colorValue;
	    array = malloc (nItems * sizeof (CCSSettingColorValue));
	    if (!array)
		break;

	    while (g_variant_iter_loop (&iter, "s", &colorValue))
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

Bool
readOption (CCSSetting * setting)
{
    CCSGSettingsWrapper  *settings = getSettingsObjectForCCSSetting (setting);
    GVariant   *gsettingsValue = NULL;
    Bool       ret = FALSE;

    /* It is impossible for certain settings to have a schema,
     * such as actions and read only settings, so in that case
     * just return FALSE since compizconfig doesn't expect us
     * to read them anyways */

    if (ccsSettingGetType (setting) == TypeAction ||
	ccsSettingIsReadOnly (setting))
    {
	return FALSE;
    }

    char *cleanSettingName = translateKeyForGSettings (ccsSettingGetName (setting));

    /* first check if the key is set */
    gsettingsValue = ccsGSettingsWrapperGetValue (settings, cleanSettingName);

    if (!gsettingsValue)
    {
	free (cleanSettingName);
	return FALSE;
    }

    if (!variantIsValidForCCSType (gsettingsValue, ccsSettingGetType (setting)))
    {
	gchar *pathName = makeSettingPath (setting);
	ccsWarning ("There is an unsupported value at path %s. "
		"Settings from this path won't be read. Try to remove "
		"that value so that operation can continue properly.",
		pathName);
	g_free (pathName);
	free (cleanSettingName);
	g_variant_unref (gsettingsValue);
	return FALSE;
    }

    switch (ccsSettingGetType (setting))
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
	ccsWarning ("Attempt to read unsupported setting type %d!",
	       ccsSettingGetType (setting));
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
    CCSGSettingsWrapper *settings = getSettingsObjectForCCSSetting (setting);
    GVariant 		*value = NULL;
    CCSSettingValueList list;

    char *cleanSettingName = translateKeyForGSettings (ccsSettingGetName (setting));
    
    if (!ccsGetList (setting, &list))
	return;

    switch (ccsSettingGetInfo (setting)->forList.listType)
    {
    case TypeBool:
	{
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
	ccsWarning ("Attempt to write unsupported list type %d!",
	       ccsSettingGetInfo (setting)->forList.listType);
	break;
    }

    if (value)
    {
	ccsGSettingsWrapperSetValue (settings, cleanSettingName, value);
    }
    
    free (cleanSettingName);
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
    CCSGSettingsWrapper  *settings = getSettingsObjectForCCSSetting (setting);
    char *cleanSettingName = translateKeyForGSettings (ccsSettingGetName (setting));

    ccsGSettingsWrapperResetKey (settings, cleanSettingName);
    free (cleanSettingName);
}

void
writeOption (CCSSetting * setting)
{
    CCSGSettingsWrapper  *settings = getSettingsObjectForCCSSetting (setting);
    char *cleanSettingName = translateKeyForGSettings (ccsSettingGetName (setting));

    switch (ccsSettingGetType (setting))
    {
    case TypeString:
	{
	    char *value;
	    if (ccsGetString (setting, &value))
	    {
		GVariant *v = g_variant_new ("s", value);
		ccsGSettingsWrapperSetValue (settings, cleanSettingName, v);
	    }
	}
	break;
    case TypeMatch:
	{
	    char *value;
	    if (ccsGetMatch (setting, &value))
	    {
		GVariant *v = g_variant_new ("s", value);
		ccsGSettingsWrapperSetValue (settings, cleanSettingName, v);
	    }
	}
    case TypeFloat:
	{
	    float value;
	    if (ccsGetFloat (setting, &value))
	    {
		GVariant *v = g_variant_new ("d", (double) value);
		ccsGSettingsWrapperSetValue (settings, cleanSettingName, v);
	    }
	}
	break;
    case TypeInt:
	{
	    int value;
	    if (ccsGetInt (setting, &value))
	    {
		GVariant *v = g_variant_new ("i", value);
		ccsGSettingsWrapperSetValue (settings, cleanSettingName, v);
	    }
	}
	break;
    case TypeBool:
	{
	    Bool value;
	    if (ccsGetBool (setting, &value))
	    {
		GVariant *v = g_variant_new ("b", value);
		ccsGSettingsWrapperSetValue (settings, cleanSettingName, v);
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

	    GVariant *v = g_variant_new ("s", colString);
	    ccsGSettingsWrapperSetValue (settings, cleanSettingName, v);
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

	    GVariant *v = g_variant_new ("s", keyString);
	    ccsGSettingsWrapperSetValue (settings, cleanSettingName, v);
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

	    GVariant *v = g_variant_new ("s", buttonString);
	    ccsGSettingsWrapperSetValue (settings, cleanSettingName, v);
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

	    GVariant *v = g_variant_new ("s", edgeString);
	    ccsGSettingsWrapperSetValue (settings, cleanSettingName, v);
	    free (edgeString);
	}
	break;
    case TypeBell:
	{
	    Bool value;
	    if (ccsGetBell (setting, &value))
	    {
		GVariant *v = g_variant_new ("b", value);
		ccsGSettingsWrapperSetValue (settings, cleanSettingName, v);
	    }
	}
	break;
    case TypeList:
	{
	    gchar *pathName = makeSettingPath (setting);
	    writeListValue (setting, pathName);
	    g_free (pathName);
	}
	break;
    default:
	ccsWarning ("Attempt to write unsupported setting type %d",
	       ccsSettingGetType (setting));
	break;
    }

    free (cleanSettingName);
}

static void
updateCurrentProfileName (const char *profile)
{
    GVariant        *profiles;
    char	    *prof;
    gchar           *profilePath = makeCompizProfilePath (profile);
    GVariant        *newProfiles;
    GVariantBuilder *newProfilesBuilder;
    GVariantIter    iter;
    gboolean        found = FALSE;

    profiles = ccsGSettingsWrapperGetValue (compizconfigSettings,  "existing-profiles");

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
    ccsGSettingsWrapperSetValue (compizconfigSettings, "existing-profiles", newProfiles);

    g_variant_unref (newProfiles);
    g_variant_builder_unref (newProfilesBuilder);

    /* Change the current profile and current profile settings */
    free (currentProfile);

    currentProfile = strdup (profile);
    currentProfileSettings = ccsGSettingsWrapperNewForSchemaWithPath (PROFILE_SCHEMA_ID,
								      profilePath,
								      &ccsDefaultObjectAllocator);

    GVariant *currentProfileVariant = g_variant_new ("s", currentProfile);
    ccsGSettingsWrapperSetValue (compizconfigSettings, "current-profile", currentProfileVariant);

    g_free (profilePath);
}

static gboolean
updateProfile (CCSContext *context)
{
    char *profile = strdup (ccsGetProfile (context));

    if (!profile)
	profile = strdup (DEFAULTPROF);

    if (!strlen (profile))
    {
	free (profile);
	profile = strdup (DEFAULTPROF);
    }

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

    value = ccsGSettingsWrapperGetValue (compizconfigSettings, "current-profile");

    if (value)
	ret = strdup (g_variant_get_string (value, NULL));
    else
	ret = strdup (DEFAULTPROF);

    return ret;
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
    char       *currentProfilePath;

    g_type_init ();

    compizconfigSettings = ccsGSettingsWrapperNewForSchema (COMPIZCONFIG_SCHEMA_ID,
							    backend->object.object_allocation);
    currentProfile = getCurrentProfileName ();
    currentProfilePath = makeCompizProfilePath (currentProfile);
    currentProfileSettings = ccsGSettingsWrapperNewForSchemaWithPath (PROFILE_SCHEMA_ID,
								      currentProfilePath,
								      backend->object.object_allocation);

    g_free (currentProfilePath);

#ifdef USE_GCONF
    integration = ccsGConfIntegrationBackendNew (backend, context, backend->object.object_allocation);
#else
    integration = ccsNullIntegrationBackendNew (backend->object.object_allocation);
#endif

    return TRUE;
}

static Bool
finiBackend (CCSBackend *backend)
{
    GList *l = settingsList;

    processEvents (backend, 0);

    if (currentProfile)
    {
	free (currentProfile);
	currentProfile = NULL;
    }

    while (l)
    {
	ccsGSettingsWrapperUnref ((CCSGSettingsWrapper *) (l->data));
	l = g_list_next (l);
    }

    if (currentProfileSettings)
    {
	ccsGSettingsWrapperUnref (currentProfileSettings);
	currentProfileSettings = NULL;
    }

    ccsGSettingsWrapperUnref (compizconfigSettings);

    ccsIntegrationUnref (integration);
    integration = NULL;

    processEvents (backend, 0);
    return TRUE;
}

Bool
readInit (CCSBackend *backend, CCSContext * context)
{
    return updateProfile (context);
}

void
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

Bool
writeInit (CCSBackend *backend, CCSContext * context)
{
    return updateProfile (context);
}

void
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
    GVariant      *value;
    char	  *profile;
    GVariantIter  iter;
    CCSStringList ret = NULL;

    value = ccsGSettingsWrapperGetValue (compizconfigSettings,  "existing-profiles");
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
deleteProfile (CCSBackend *backend,
	       CCSContext *context,
	       char       *profile)
{
    GVariant        *plugins;
    GVariant        *profiles;
    GVariant        *newProfiles;
    GVariantBuilder *newProfilesBuilder;
    char            *plugin, *prof;
    GVariantIter    iter;
    char            *profileSettingsPath = makeCompizProfilePath (profile);
    CCSGSettingsWrapper *profileSettings = ccsGSettingsWrapperNewForSchemaWithPath (PROFILE_SCHEMA_ID, profileSettingsPath, &ccsDefaultObjectAllocator);

    plugins = ccsGSettingsWrapperGetValue (currentProfileSettings, "plugins-with-set-keys");
    profiles = ccsGSettingsWrapperGetValue (compizconfigSettings, "existing-profiles");

    g_variant_iter_init (&iter, plugins);
    while (g_variant_iter_loop (&iter, "s", &plugin))
    {
	CCSGSettingsWrapper *settings;
	gchar *pathName = makeCompizPluginPath (currentProfile, plugin);

	settings = getSettingsObjectForPluginWithPath (plugin, pathName, context);
	g_free (pathName);

	/* The GSettings documentation says not to use this API
	 * because we should know our own schema ... though really
	 * we don't because we autogenerate schemas ... */
	if (settings)
	{
	    char **keys = ccsGSettingsWrapperListKeys (settings);
	    char **key_ptr;

	    /* Unset all the keys */
	    for (key_ptr = keys; *key_ptr; key_ptr++)
		ccsGSettingsWrapperResetKey (settings, *key_ptr);

	    g_strfreev (keys);
	}
    }

    /* Remove the profile from existing-profiles */
    ccsGSettingsWrapperResetKey (profileSettings, "plugins-with-set-values");

    g_variant_iter_init (&iter, profiles);
    newProfilesBuilder = g_variant_builder_new (G_VARIANT_TYPE ("as"));

    while (g_variant_iter_loop (&iter, "s", &prof))
    {
	if (g_strcmp0 (prof, profile))
	    g_variant_builder_add (newProfilesBuilder, "s", prof);
    }

    newProfiles = g_variant_new ("as", newProfilesBuilder);
    ccsGSettingsWrapperSetValue (compizconfigSettings, "existing-profiles", newProfiles);

    g_variant_unref (newProfiles);
    g_variant_builder_unref (newProfilesBuilder);

    g_free (profileSettingsPath);

    updateProfile (context);

    return TRUE;
}

const CCSBackendInfo gconfBackendInfo =
{
    "gsettings",
    "GSettings Configuration Backend",
    "GSettings Configuration Backend for libccs",
    TRUE,
    TRUE,
    1
};

static const CCSBackendInfo *
getInfo (CCSBackend *backend)
{
    return &gconfBackendInfo;
}

static CCSBackendInterface gsettingsVTable = {
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
    return &gsettingsVTable;
}

