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
#include <ccs_gsettings_interface_wrapper.h>

static void
valueChanged (GSettings   *settings,
	      gchar	  *keyname,
	      gpointer    user_data);

/* some forward declarations */
static void writeIntegratedOption (CCSBackend *backend,
				   CCSContext *context,
				   CCSSetting *setting,
				   int        index);

static CCSGSettingsWrapper *
ccsGSettingsBackendGetSettingsObjectForPluginWithPathDefault (CCSBackend *backend,
							      const char *plugin,
							      const char *path,
							      CCSContext *context)
{
    CCSGSettingsWrapper *settingsObj = NULL;
    gchar *schemaName = getSchemaNameForPlugin (plugin);
    GVariant        *writtenPlugins;
    gsize            newWrittenPluginsSize;
    gchar           **newWrittenPlugins;

    CCSGSettingsBackendPrivate *priv = (CCSGSettingsBackendPrivate *) ccsObjectGetPrivate (backend);

    settingsObj = findCCSGSettingsWrapperBySchemaName (schemaName, priv->settingsList);

    if (settingsObj)
    {
	g_free (schemaName);
	return settingsObj;
    }

    /* No existing settings object found for this schema, create one */
    
    settingsObj = ccsGSettingsWrapperNewForSchemaWithPath (schemaName, path, &ccsDefaultObjectAllocator);
    ccsGSettingsBackendConnectToChangedSignal (backend, settingsObj);
    priv->settingsList = g_list_append (priv->settingsList, (void *) settingsObj);

    /* Also write the plugin name to the list of modified plugins so
     * that when we delete the profile the keys for that profile are also
     * unset FIXME: This could be a little more efficient, like we could
     * store keys that have changed from their defaults ... though
     * gsettings doesn't seem to give you a way to get all of the schemas */

    writtenPlugins = ccsGSettingsWrapperGetValue (priv->currentProfileSettings, "plugins-with-set-keys");

    appendToPluginsWithSetKeysList (plugin, writtenPlugins, &newWrittenPlugins, &newWrittenPluginsSize);

    GVariant *newWrittenPluginsVariant = g_variant_new_strv ((const gchar * const *) newWrittenPlugins, newWrittenPluginsSize);

    ccsGSettingsWrapperSetValue (priv->currentProfileSettings, "plugins-with-set-keys", newWrittenPluginsVariant);

    g_variant_unref (writtenPlugins);
    g_free (schemaName);
    g_strfreev (newWrittenPlugins);

    return settingsObj;
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
updateSetting (CCSBackend *backend, CCSContext *context, CCSPlugin *plugin, CCSSetting *setting)
{
    int          index;

    readInit (backend, context);
    if (!readOption (backend, setting))
    {
	ccsResetToDefault (setting, TRUE);
    }

    if (ccsGetIntegrationEnabled (context) &&
	isIntegratedOption (setting, &index))
    {
	writeInit (backend, context);
	writeIntegratedOption (backend, context, setting, index);
    }
}

static void
valueChanged (GSettings   *settings,
	      gchar	  *keyName,
	      gpointer    user_data)
{
    CCSBackend   *backend = (CCSBackend *)user_data;
    GValue       schemaNameValue = G_VALUE_INIT;
    CCSGSettingsBackendPrivate *priv = (CCSGSettingsBackendPrivate *) ccsObjectGetPrivate (backend);


    g_value_init (&schemaNameValue, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (settings), "schema-id", &schemaNameValue);

    const char *schemaName = g_value_get_string (&schemaNameValue);
    CCSGSettingsWrapper *wrapper = findCCSGSettingsWrapperBySchemaName (schemaName, priv->settingsList);

    g_value_unset (&schemaNameValue);

    updateSettingWithGSettingsKeyName (backend, wrapper, keyName, updateSetting);
}

static Bool
readIntegratedOption (CCSBackend *backend,
		      CCSContext *context,
		      CCSSetting *setting,
		      int        index)
{
#ifdef USE_GCONF
    return readGConfIntegratedOption (backend, context, setting, index);
#else
    return FALSE;
#endif
}

static GVariant *
getVariantForCCSSetting (CCSBackend *backend, CCSSetting *setting)
{
    CCSGSettingsBackendPrivate *priv = (CCSGSettingsBackendPrivate *) ccsObjectGetPrivate (backend);
    CCSGSettingsWrapper  *settings = getSettingsObjectForCCSSetting (backend, setting);
    char *cleanSettingName = getNameForCCSSetting (setting);
    gchar *pathName = makeSettingPath (priv->currentProfile, setting);
    GVariant *gsettingsValue = getVariantAtKey (settings,
						cleanSettingName,
						pathName,
						ccsSettingGetType (setting));

    free (cleanSettingName);
    free (pathName);
    return gsettingsValue;
}

Bool
readOption (CCSBackend *backend, CCSSetting * setting)
{
    Bool       ret = FALSE;
    GVariant   *gsettingsValue = NULL;

    /* It is impossible for certain settings to have a schema,
     * such as actions and read only settings, so in that case
     * just return FALSE since compizconfig doesn't expect us
     * to read them anyways */

    if (ccsSettingGetType (setting) == TypeAction ||
	ccsSettingIsReadOnly (setting))
    {
	return FALSE;
    }

    gsettingsValue = getVariantForCCSSetting (backend, setting);

    switch (ccsSettingGetType (setting))
    {
    case TypeString:
	{
	    const char *value;
	    value = readStringFromVariant (gsettingsValue);
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
	    value = readStringFromVariant (gsettingsValue);
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
	    value = readIntFromVariant (gsettingsValue);

	    ccsSetInt (setting, value, TRUE);
	    ret = TRUE;
	}
	break;
    case TypeBool:
	{
	    Bool value;
	    value = readBoolFromVariant (gsettingsValue);

	    ccsSetBool (setting, value, TRUE);
	    ret = TRUE;
	}
	break;
    case TypeFloat:
	{
	    float value;
	    value = readFloatFromVariant (gsettingsValue);

	    ccsSetFloat (setting, value, TRUE);
    	    ret = TRUE;
	}
	break;
    case TypeColor:
	{
	    Bool success = FALSE;
	    CCSSettingColorValue color = readColorFromVariant (gsettingsValue, &success);

	    if (success)
	    {
		ccsSetColor (setting, color, TRUE);
		ret = TRUE;
	    }
	}
	break;
    case TypeKey:
	{
	    Bool success = FALSE;
	    CCSSettingKeyValue key = readKeyFromVariant (gsettingsValue, &success);

	    if (success)
	    {
		ccsSetKey (setting, key, TRUE);
		ret = TRUE;
	    }
	}
	break;
    case TypeButton:
	{
	    Bool success = FALSE;
	    CCSSettingButtonValue button = readButtonFromVariant (gsettingsValue, &success);

	    if (success)
	    {
		ccsSetButton (setting, button, TRUE);
		ret = TRUE;
	    }
	}
	break;
    case TypeEdge:
	{
	    unsigned int edges = readEdgeFromVariant (gsettingsValue);

	    ccsSetEdge (setting, edges, TRUE);
	    ret = TRUE;
	}
	break;
    case TypeBell:
	{
	    Bool value;
	    value = readBoolFromVariant (gsettingsValue);

	    ccsSetBell (setting, value, TRUE);
	    ret = TRUE;
	}
	break;
    case TypeList:
	{
	    CCSSettingValueList list = readListValue (gsettingsValue, setting, &ccsDefaultObjectAllocator);

	    if (list)
	    {
		CCSSettingValueList iter = list;

		while (iter)
		{
		    ((CCSSettingValue *) iter->data)->parent = setting;
		    iter = iter->next;
		}

		ccsSetList (setting, list, TRUE);
		ccsSettingValueListFree (list, TRUE);
		ret = TRUE;
	    }
	}
	break;
    default:
	ccsWarning ("Attempt to read unsupported setting type %d!",
	       ccsSettingGetType (setting));
	break;
    }

    g_variant_unref (gsettingsValue);

    return ret;
}

static void
writeIntegratedOption (CCSBackend *backend,
		       CCSContext *context,
		       CCSSetting *setting,
		       int        index)
{
#ifdef USE_GCONF
    writeGConfIntegratedOption (backend, context, setting, index);
#endif

    return;
}

void
writeOption (CCSBackend *backend, CCSSetting * setting)
{
    CCSGSettingsWrapper  *settings = getSettingsObjectForCCSSetting (backend, setting);
    char *cleanSettingName = translateKeyForGSettings (ccsSettingGetName (setting));
    GVariant *gsettingsValue = NULL;
    Bool success = FALSE;

    switch (ccsSettingGetType (setting))
    {
    case TypeString:
	{
	    char *value;
	    if (ccsGetString (setting, &value))
	    {
		success = writeStringToVariant (value, &gsettingsValue);
	    }
	}
	break;
    case TypeMatch:
	{
	    char *value;
	    if (ccsGetMatch (setting, &value))
	    {
		success = writeStringToVariant (value, &gsettingsValue);
	    }
	}
	break;
    case TypeFloat:
	{
	    float value;
	    if (ccsGetFloat (setting, &value))
	    {
		success = writeFloatToVariant (value, &gsettingsValue);
	    }
	}
	break;
    case TypeInt:
	{
	    int value;
	    if (ccsGetInt (setting, &value))
	    {
		success = writeIntToVariant (value, &gsettingsValue);
	    }
	}
	break;
    case TypeBool:
	{
	    Bool value;
	    if (ccsGetBool (setting, &value))
	    {
		success = writeBoolToVariant (value, &gsettingsValue);
	    }
	}
	break;
    case TypeColor:
	{
	    CCSSettingColorValue value;

	    if (!ccsGetColor (setting, &value))
		break;

	    success = writeColorToVariant (value, &gsettingsValue);
	}
	break;
    case TypeKey:
	{
	    CCSSettingKeyValue key;

	    if (!ccsGetKey (setting, &key))
		break;

	    success = writeKeyToVariant (key, &gsettingsValue);
	}
	break;
    case TypeButton:
	{
	    CCSSettingButtonValue button;

	    if (!ccsGetButton (setting, &button))
		break;

	    success = writeButtonToVariant (button, &gsettingsValue);
	}
	break;
    case TypeEdge:
	{
	    unsigned int edges;

	    if (!ccsGetEdge (setting, &edges))
		break;

	    success = writeEdgeToVariant (edges, &gsettingsValue);
	}
	break;
    case TypeBell:
	{
	    Bool value;
	    if (ccsGetBell (setting, &value))
	    {
		success = writeBoolToVariant (value, &gsettingsValue);
	    }
	}
	break;
    case TypeList:
	{
	    CCSSettingValueList  list = NULL;

	    if (!ccsGetList (setting, &list))
		return;

	    success = writeListValue (list,
				      ccsSettingGetInfo (setting)->forList.listType,
				      &gsettingsValue);
	}
	break;
    default:
	ccsWarning ("Attempt to write unsupported setting type %d",
	       ccsSettingGetType (setting));
	break;
    }

    if (success && gsettingsValue)
    {
	/* g_settings_set_value will consume the reference
	 * so there is no need to unref value here */
	writeVariantToKey (settings, cleanSettingName, gsettingsValue);
    }

    free (cleanSettingName);
}

static char*
getCurrentProfileName (CCSBackend *backend)
{
    GVariant *value;
    char     *ret = NULL;

    CCSGSettingsBackendPrivate *priv = (CCSGSettingsBackendPrivate *) ccsObjectGetPrivate (backend);

    value = ccsGSettingsWrapperGetValue (priv->compizconfigSettings, "current-profile");

    if (value)
	ret = strdup (g_variant_get_string (value, NULL));
    else
	ret = strdup (DEFAULTPROF);

    g_variant_unref (value);

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

static CCSContext *
ccsGSettingsBackendGetContextDefault (CCSBackend *backend)
{
    CCSGSettingsBackendPrivate *priv = (CCSGSettingsBackendPrivate *) ccsObjectGetPrivate (backend);

    return priv->context;
}

static void
ccsGSettingsBackendConnectToValueChangedSignalDefault (CCSBackend *backend, CCSGSettingsWrapper *wrapper)
{
    ccsGSettingsWrapperConnectToChangedSignal (wrapper, (GCallback) valueChanged, (gpointer) backend);
}

static void
ccsGSettingsBackendRegisterGConfClientDefault (CCSBackend *backend)
{
#ifdef USE_GCONF
    initGConfClient (backend);
#endif
}

static void
ccsGSettingsBackendUnregisterGConfClientDefault (CCSBackend *backend)
{
#ifdef USE_GCONF
    CCSGSettingsBackendPrivate *priv = (CCSGSettingsBackendPrivate *) ccsObjectGetPrivate (backend);

    gconf_client_clear_cache (priv->client);
    finiGConfClient (backend);
#endif
}

static const char *
ccsGSettingsBackendGetCurrentProfileDefault (CCSBackend *backend)
{
    CCSGSettingsBackendPrivate *priv = (CCSGSettingsBackendPrivate *) ccsObjectGetPrivate (backend);

    return priv->currentProfile;
}

static GVariant *
ccsGSettingsBackendGetExistingProfilesDefault (CCSBackend *backend)
{
    CCSGSettingsBackendPrivate *priv = (CCSGSettingsBackendPrivate *) ccsObjectGetPrivate (backend);

    return ccsGSettingsWrapperGetValue (priv->compizconfigSettings, "existing-profiles");
}

static void
ccsGSettingsBackendSetExistingProfilesDefault (CCSBackend *backend, GVariant *value)
{
    CCSGSettingsBackendPrivate *priv = (CCSGSettingsBackendPrivate *) ccsObjectGetPrivate (backend);

    ccsGSettingsWrapperSetValue (priv->compizconfigSettings, "existing-profiles", value);
}

static void
ccsGSettingsBackendSetCurrentProfileDefault (CCSBackend *backend, const gchar *value)
{
    CCSGSettingsBackendPrivate *priv = (CCSGSettingsBackendPrivate *) ccsObjectGetPrivate (backend);
    gchar           *profilePath = makeCompizProfilePath (value);

    /* Change the current profile and current profile settings */
    if (priv->currentProfile)
	free (priv->currentProfile);

    if (priv->currentProfileSettings)
	ccsGSettingsWrapperUnref (priv->currentProfileSettings);

    priv->currentProfile = strdup (value);
    priv->currentProfileSettings = ccsGSettingsWrapperNewForSchemaWithPath (PROFILE_SCHEMA_ID,
									    profilePath,
									    backend->object.object_allocation);

    GVariant *currentProfileVariant = g_variant_new ("s", value, NULL);

    ccsGSettingsWrapperSetValue (priv->compizconfigSettings, "current-profile", currentProfileVariant);

    g_free (profilePath);
}

GVariant *
ccsGSettingsBackendGetPluginsWithSetKeysDefault (CCSBackend *backend)
{
    CCSGSettingsBackendPrivate *priv = (CCSGSettingsBackendPrivate *) ccsObjectGetPrivate (backend);
    return ccsGSettingsWrapperGetValue (priv->currentProfileSettings, "plugins-with-set-keys");
}

void
ccsGSettingsBackendClearPluginsWithSetKeysDefault (CCSBackend *backend)
{
    CCSGSettingsBackendPrivate *priv = (CCSGSettingsBackendPrivate *) ccsObjectGetPrivate (backend);
    ccsGSettingsWrapperResetKey (priv->currentProfileSettings, "plugins-with-set-keys");
}

static CCSGSettingsBackendInterface gsettingsAdditionalDefaultInterface = {
    ccsGSettingsBackendGetContextDefault,
    ccsGSettingsBackendConnectToValueChangedSignalDefault,
    ccsGSettingsBackendGetSettingsObjectForPluginWithPathDefault,
    ccsGSettingsBackendRegisterGConfClientDefault,
    ccsGSettingsBackendUnregisterGConfClientDefault,
    ccsGSettingsBackendGetCurrentProfileDefault,
    ccsGSettingsBackendGetExistingProfilesDefault,
    ccsGSettingsBackendSetExistingProfilesDefault,
    ccsGSettingsBackendSetCurrentProfileDefault,
    ccsGSettingsBackendGetPluginsWithSetKeysDefault,
    ccsGSettingsBackendClearPluginsWithSetKeysDefault,
    ccsGSettingsBackendUnsetAllChangedPluginKeysInProfileDefault,
    ccsGSettingsBackendUpdateProfileDefault,
    ccsGSettingsBackendUpdateCurrentProfileNameDefault,
    ccsGSettingsBackendAddProfileDefault
};

static Bool
initBackend (CCSBackend *backend, CCSContext * context)
{
    char       *currentProfilePath;

    g_type_init ();

    ccsObjectAddInterface (backend, (CCSInterface *) &gsettingsAdditionalDefaultInterface, GET_INTERFACE_TYPE (CCSGSettingsBackendInterface));

    CCSGSettingsBackendPrivate *priv = calloc (1, sizeof (CCSGSettingsBackendPrivate));

    if (!priv)
	return FALSE;

    ccsObjectSetPrivate (backend, (CCSPrivate *) priv);

    priv->compizconfigSettings = ccsGSettingsWrapperNewForSchema (COMPIZCONFIG_SCHEMA_ID,
								  backend->object.object_allocation);
    priv->currentProfile = getCurrentProfileName (backend);
    currentProfilePath = makeCompizProfilePath (priv->currentProfile);
    priv->currentProfileSettings = ccsGSettingsWrapperNewForSchemaWithPath (PROFILE_SCHEMA_ID,
									    currentProfilePath,
									    backend->object.object_allocation);
    priv->context = context;

    g_free (currentProfilePath);

    return TRUE;
}

static void
ccsGSettingsWrapperDestroyNotify (gpointer o)
{
    ccsGSettingsWrapperUnref ((CCSGSettingsWrapper *) o);
}

static Bool
finiBackend (CCSBackend *backend)
{
    CCSGSettingsBackendPrivate *priv = (CCSGSettingsBackendPrivate *) ccsObjectGetPrivate (backend);

    processEvents (backend, 0);

    ccsGSettingsBackendUnregisterGConfClient (backend);

    if (priv->currentProfile)
    {
	free (priv->currentProfile);
	priv->currentProfile = NULL;
    }

    g_list_free_full (priv->settingsList, ccsGSettingsWrapperDestroyNotify);
    priv->settingsList = NULL;

    if (priv->currentProfileSettings)
    {
	ccsGSettingsWrapperUnref (priv->currentProfileSettings);
	priv->currentProfileSettings = NULL;
    }

    ccsGSettingsWrapperUnref (priv->compizconfigSettings);

    priv->compizconfigSettings = NULL;

    processEvents (backend, 0);

    free (priv);
    ccsObjectSetPrivate (backend, NULL);

    return TRUE;
}

Bool
readInit (CCSBackend *backend, CCSContext * context)
{
    return ccsGSettingsBackendUpdateProfile (backend, context);
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
	status = readIntegratedOption (backend, context, setting, index);
    }
    else
	status = readOption (backend, setting);

    if (!status)
	ccsResetToDefault (setting, TRUE);
}

Bool
writeInit (CCSBackend *backend, CCSContext * context)
{
    return ccsGSettingsBackendUpdateProfile (backend, context);
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
	writeIntegratedOption (backend, context, setting, index);
    }
    else if (ccsSettingGetIsDefault (setting))
    {
	resetOptionToDefault (backend, setting);
    }
    else
	writeOption (backend, setting);

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

    CCSGSettingsBackendPrivate *priv = (CCSGSettingsBackendPrivate *) ccsObjectGetPrivate (backend);

    value = ccsGSettingsWrapperGetValue (priv->compizconfigSettings,  "existing-profiles");
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

static const CCSBackendInfo *
getInfo (CCSBackend *backend)
{
    return &gsettingsBackendInfo;
}

static Bool
ccsGSettingsDeleteProfileWrapper (CCSBackend *backend,
				  CCSContext *context,
				  char       *profile)
{
    return deleteProfile (backend, context, profile);
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
    ccsGSettingsDeleteProfileWrapper
};



CCSBackendInterface *
getBackendInfo (void)
{
    return &gsettingsVTable;
}

