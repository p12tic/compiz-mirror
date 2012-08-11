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
#include "gsettings_shared.h"
#include "gconf-integration.h"
#include "ccs_gsettings_interface.h"
#include "ccs_gsettings_interface_wrapper.h"
#include "ccs_gsettings_backend.h"
#include "ccs_gsettings_backend_interface.h"

static gchar *
makeSettingPath (CCSBackend *backend, CCSSetting *setting)
{
    return makeCompizPluginPath (ccsGSettingsBackendGetCurrentProfile (backend),
                             ccsPluginGetName (ccsSettingGetParent (setting)));
}

static CCSGSettingsWrapper *
getSettingsObjectForCCSSetting (CCSBackend *backend,
				CCSSetting *setting)
{
    CCSGSettingsWrapper *ret = NULL;
    gchar *pathName = makeSettingPath (backend, setting);
    CCSPlugin *plugin = ccsSettingGetParent (setting);
    CCSContext *context = ccsPluginGetContext (plugin);
    const char *pluginName  = ccsPluginGetName (plugin);

    ret = ccsGSettingsGetSettingsObjectForPluginWithPath (backend,
							  pluginName,
							  pathName,
							  context);

    g_free (pathName);
    return ret;
}

static Bool
readListValue (CCSBackend *backend,
	       CCSSetting *setting)
{
    CCSGSettingsWrapper	*settings = getSettingsObjectForCCSSetting (backend, setting);
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
	    gchar **array = g_malloc0 ((nItems + 1) * sizeof (gchar *));
	    gchar **arrayCounter = array;
	    gchar *value;

	    if (!array)
		break;

	    array[nItems] = NULL;

	    /* Reads each item from the variant into arrayCounter */
	    while (g_variant_iter_next (&iter, "s", &value))
		*arrayCounter++ = value;

	    list = ccsGetValueListFromStringArray ((const gchar **) array, nItems, setting);
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
readIntegratedOption (CCSBackend *backend,
		      CCSContext *context,
		      CCSSetting *setting,
		      int        index)
{
    return ccsGSettingsBackendReadIntegratedOption (backend, setting, index);
}

Bool
readOption (CCSBackend *backend, CCSSetting * setting)
{
    CCSGSettingsWrapper  *settings = getSettingsObjectForCCSSetting (backend, setting);
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
	gchar *pathName = makeSettingPath (backend, setting);
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
	ret = readListValue (backend, setting);
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
writeListValue (CCSBackend *backend,
		CCSSetting *setting,
		char       *pathName)
{
    CCSGSettingsWrapper *settings = getSettingsObjectForCCSSetting (backend, setting);
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
writeIntegratedOption (CCSBackend *backend,
		       CCSContext *context,
		       CCSSetting *setting,
		       int        index)
{
    ccsGSettingsBackendWriteIntegratedOption (backend, setting, index);
}

static void
resetOptionToDefault (CCSBackend *backend, CCSSetting * setting)
{
    CCSGSettingsWrapper  *settings = getSettingsObjectForCCSSetting (backend, setting);
    char *cleanSettingName = translateKeyForGSettings (ccsSettingGetName (setting));

    ccsGSettingsWrapperResetKey (settings, cleanSettingName);
    free (cleanSettingName);
}

void
writeOption (CCSBackend *backend,
	     CCSSetting *setting)
{
    CCSGSettingsWrapper  *settings = getSettingsObjectForCCSSetting (backend, setting);
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
	    gchar *pathName = makeSettingPath (backend, setting);
	    writeListValue (backend, setting, pathName);
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
updateSetting (CCSBackend *backend, CCSContext *context, CCSPlugin *plugin, CCSSetting *setting)
{
    int          index = ccsGSettingsBackendGetIntegratedOptionIndex (backend, setting);

    readInit (backend, context);
    if (!readOption (backend, setting))
    {
	ccsResetToDefault (setting, TRUE);
    }

    if (ccsGetIntegrationEnabled (context) &&
	index != -1)
    {
	writeInit (backend, context);
	writeIntegratedOption (backend, context, setting, index);
    }
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
    return ccsGSettingsBackendAttachNewToBackend (backend, context);
}

static Bool
finiBackend (CCSBackend *backend)
{
    processEvents (backend, 0);
    ccsGSettingsBackendDetachFromBackend (backend);
    processEvents (backend, 0);

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
    int  index = ccsGSettingsBackendGetIntegratedOptionIndex (backend, setting);

    if (ccsGetIntegrationEnabled (context) &&
	index == -1)
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
    int  index = ccsGSettingsBackendGetIntegratedOptionIndex (backend, setting);

    if (ccsGetIntegrationEnabled (context) &&
	index != -1)
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

    if (ccsGSettingsBackendGetIntegratedOptionIndex (backend, setting) == -1)
	return FALSE;

    return TRUE;
}

static Bool
getSettingIsReadOnly (CCSBackend *backend, CCSSetting * setting)
{
    /* FIXME */
    return FALSE;
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

static Bool
ccsGSettingsWrapDeleteProfile (CCSBackend *backend,
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
    ccsGSettingsGetExistingProfiles,
    ccsGSettingsWrapDeleteProfile,
    ccsGSettingsSetIntegration
};

CCSBackendInterface *
getBackendInfo (void)
{
    return &gsettingsVTable;
}
