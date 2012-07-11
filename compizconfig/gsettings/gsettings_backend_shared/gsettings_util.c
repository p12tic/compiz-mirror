#define CCS_LOG_DOMAIN "gsettings"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "gsettings_shared.h"

GList *
variantTypeToPossibleSettingType (const gchar *vt)
{
    struct _variantTypeCCSType
    {
	char variantType;
	CCSSettingType ccsType;
    };

    static const struct _variantTypeCCSType vCCSType[] =
    {
	{ 'b', TypeBool },
	{ 'i', TypeInt },
	{ 'd', TypeFloat },
	{ 's', TypeString },
	{ 's', TypeColor },
	{ 's', TypeKey },
	{ 's', TypeButton },
	{ 's', TypeEdge },
	{ 'b', TypeBell },
	{ 's', TypeMatch },
	{ 'a', TypeList }
    };

    GList *possibleTypesList = NULL;
    unsigned int i = 0;
    for (; i < (sizeof (vCCSType) / sizeof (vCCSType[0])); ++i)
    {
	if (vt[0] == vCCSType[i].variantType)
	    possibleTypesList = g_list_append (possibleTypesList, GINT_TO_POINTER ((gint) vCCSType[i].ccsType));
    }

    return possibleTypesList;
}

GObject *
findObjectInListWithPropertySchemaName (const gchar *schemaName,
					GList	    *iter)
{
    while (iter)
    {
	GObject   *obj = (GObject *) iter->data;
	gchar	  *name = NULL;

	g_object_get (obj,
		      "schema",
		      &name, NULL);
	if (g_strcmp0 (name, schemaName) != 0)
	    obj = NULL;

	g_free (name);

	if (obj)
	    return obj;
	else
	    iter = g_list_next (iter);
    }

    return NULL;
}

gchar *
getSchemaNameForPlugin (const char *plugin)
{
    gchar       *schemaName =  NULL;

    schemaName = g_strconcat (PLUGIN_SCHEMA_ID_PREFIX, plugin, NULL);

    return schemaName;
}

char *
truncateKeyForGSettings (const char *gsettingName)
{
    /* Truncate */
    gchar *truncated = g_strndup (gsettingName, MAX_GSETTINGS_KEY_SIZE);

    return truncated;
}

char *
translateUnderscoresToDashesForGSettings (const char *truncated)
{
    gchar *clean        = NULL;
    gchar **delimited   = NULL;

    /* Replace underscores with dashes */
    delimited = g_strsplit (truncated, "_", 0);

    clean = g_strjoinv ("-", delimited);

    g_strfreev (delimited);
    return clean;
}

void
translateToLowercaseForGSettings (char *name)
{
    unsigned int i;

    /* Everything must be lowercase */
    for (i = 0; i < strlen (name); i++)
	name[i] = g_ascii_tolower (name[i]);
}

char *
translateKeyForGSettings (const char *gsettingName)
{
    char *truncated = truncateKeyForGSettings (gsettingName);
    char *translated = translateUnderscoresToDashesForGSettings (truncated);
    translateToLowercaseForGSettings (translated);

    if (strlen (gsettingName) > MAX_GSETTINGS_KEY_SIZE)
	ccsWarning ("Key name %s is not valid in GSettings, it was changed to %s, this may cause problems!", gsettingName, translated);

    g_free (truncated);

    return translated;
}

gchar *
translateKeyForCCS (const char *gsettingName)
{
    gchar *clean        = NULL;
    gchar **delimited   = NULL;

    /* Replace dashes with underscores */
    delimited = g_strsplit (gsettingName, "-", 0);

    clean = g_strjoinv ("_", delimited);

    g_strfreev (delimited);

    return clean;
}

gboolean
compizconfigTypeHasVariantType (CCSSettingType type)
{
    gint i = 0;

    static const unsigned int nVariantTypes = 6;
    static const CCSSettingType variantTypes[] =
    {
	TypeString,
	TypeMatch,
	TypeColor,
	TypeBool,
	TypeInt,
	TypeFloat
    };

    for (; i < nVariantTypes; i++)
    {
	if (variantTypes[i] == type)
	    return TRUE;
    }

    return FALSE;
}

gboolean
decomposeGSettingsPath (const char *pathInput,
			char **pluginName,
			unsigned int *screenNum)
{
    const char *path = pathInput;
    const int prefixLen = strlen (PROFILE_PATH_PREFIX);
    char pluginBuf[1024];

    if (strncmp (path, PROFILE_PATH_PREFIX, prefixLen))
        return FALSE;
    path += prefixLen;

    *pluginName = NULL;
    *screenNum = 0;

    /* Can't overflow, limit is 1023 chars */
    if (sscanf (path, "%*[^/]/plugins/%1023[^/]", pluginBuf) == 1)
    {
        pluginBuf[1023] = '\0';
        *pluginName = g_strdup (pluginBuf);
        return TRUE;
    }

    return FALSE;
}

gboolean
variantIsValidForCCSType (GVariant *gsettingsValue,
			  CCSSettingType settingType)
{
    gboolean valid = FALSE;

    switch (settingType)
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

    return valid;
}

Bool
appendToPluginsWithSetKeysList (const gchar *plugin,
				GVariant    *writtenPlugins,
				char	       ***newWrittenPlugins,
				gsize	       *newWrittenPluginsSize)
{
    gsize writtenPluginsLen = 0;
    Bool            found = FALSE;
    char	    *plug;
    GVariantIter    iter;

    g_variant_iter_init (&iter, writtenPlugins);
    *newWrittenPluginsSize = g_variant_iter_n_children (&iter);

    while (g_variant_iter_loop (&iter, "s", &plug))
    {
	if (!found)
	    found = (g_strcmp0 (plug, plugin) == 0);
    }

    if (!found)
	(*newWrittenPluginsSize)++;

    *newWrittenPlugins = g_variant_dup_strv (writtenPlugins, &writtenPluginsLen);

    if (*newWrittenPluginsSize > writtenPluginsLen)
    {
	*newWrittenPlugins = g_realloc (*newWrittenPlugins, (*newWrittenPluginsSize + 1) * sizeof (gchar *));

	/* Next item becomes plugin */
	(*newWrittenPlugins)[writtenPluginsLen]  = g_strdup (plugin);

	/* Last item becomes NULL */
	(*newWrittenPlugins)[*newWrittenPluginsSize] = NULL;
    }

    return !found;
}

CCSSettingList
filterAllSettingsMatchingType (CCSSettingType type,
			       CCSSettingList settingList)
{
    CCSSettingList filteredList = NULL;
    CCSSettingList iter = settingList;

    while (iter)
    {
	CCSSetting *s = (CCSSetting *) iter->data;

	if (ccsSettingGetType (s) == type)
	    filteredList = ccsSettingListAppend (filteredList, s);

	iter = iter->next;
    }

    return filteredList;
}

CCSSettingList
filterAllSettingsMatchingPartOfStringIgnoringDashesUnderscoresAndCase (const gchar *keyName,
								       CCSSettingList settingList)
{
    CCSSettingList filteredList = NULL;
    CCSSettingList iter = settingList;

    while (iter)
    {
	CCSSetting *s = (CCSSetting *) iter->data;

	char *name = ccsSettingGetName (s);
	char *underscores_as_dashes = translateUnderscoresToDashesForGSettings (name);

	if (g_ascii_strncasecmp (underscores_as_dashes, keyName, strlen (keyName)) == 0)
	    filteredList = ccsSettingListAppend (filteredList, s);

	g_free (underscores_as_dashes);

	iter = iter->next;
    }

    return filteredList;
}

CCSSetting *
attemptToFindCCSSettingFromLossyName (CCSSettingList settingList, const gchar *lossyName, CCSSettingType type)
{
    CCSSettingList ofThatType = filterAllSettingsMatchingType (type, settingList);
    CCSSettingList lossyMatchingName = filterAllSettingsMatchingPartOfStringIgnoringDashesUnderscoresAndCase (lossyName, ofThatType);
    CCSSetting	   *found = NULL;

    if (ccsSettingListLength (lossyMatchingName) == 1)
	found = lossyMatchingName->data;

    ccsSettingListFree (ofThatType, FALSE);
    ccsSettingListFree (lossyMatchingName, FALSE);

    return found;
}

gchar *
makeCompizProfilePath (const gchar *profilename)
{
    return g_build_path ("/", PROFILE_PATH_PREFIX, profilename, "/", NULL);
}

gchar *
makeCompizPluginPath (const gchar *profileName, const gchar *pluginName)
{
    return g_build_path ("/", PROFILE_PATH_PREFIX, profileName,
                         "plugins", pluginName, "/", NULL);
}

gchar *
getNameForCCSSetting (CCSSetting *setting)
{
    return translateKeyForGSettings (ccsSettingGetName (setting));
}

Bool
checkReadVariantIsValid (GVariant *gsettingsValue, CCSSettingType type, const gchar *pathName)
{
    /* first check if the key is set */
    if (!gsettingsValue)
    {
	ccsWarning ("There is no key at the path %s. "
		"Settings from this path won't be read. Try to remove "
		"that value so that operation can continue properly.",
		pathName);
	return FALSE;
    }

    if (!variantIsValidForCCSType (gsettingsValue, type))
    {
	ccsWarning ("There is an unsupported value at path %s. "
		"Settings from this path won't be read. Try to remove "
		"that value so that operation can continue properly.",
		pathName);
	return FALSE;
    }

    return TRUE;
}

GVariant *
getVariantAtKey (GSettings *settings, char *key, const char *pathName, CCSSettingType type)
{
    GVariant *gsettingsValue = g_settings_get_value (settings, key);

    if (!checkReadVariantIsValid (gsettingsValue, type, pathName))
    {
	g_variant_unref (gsettingsValue);
	return NULL;
    }

    return gsettingsValue;
}

CCSSettingValueList
readListValue (GVariant *gsettingsValue, CCSSettingType listType)
{
    gboolean		hasVariantType;
    unsigned int        nItems, i = 0;
    CCSSettingValueList list = NULL;
    GVariantIter	iter;

    hasVariantType = compizconfigTypeHasVariantType (listType);

    if (!hasVariantType)
	return NULL;

    g_variant_iter_init (&iter, gsettingsValue);
    nItems = g_variant_iter_n_children (&iter);

    switch (listType)
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

	    list = ccsGetValueListFromBoolArray (array, nItems, NULL);
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

	    list = ccsGetValueListFromIntArray (array, nItems, NULL);
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

	    list = ccsGetValueListFromFloatArray ((float *) array, nItems, NULL);
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

	    list = ccsGetValueListFromStringArray (array, nItems, NULL);
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
	    list = ccsGetValueListFromColorArray (array, nItems, NULL);
	    free (array);
	}
	break;
    default:
	break;
    }

    return list;
}

const char * readStringFromVariant (GVariant *gsettingsValue)
{
    return g_variant_get_string (gsettingsValue, NULL);
}

int readIntFromVariant (GVariant *gsettingsValue)
{
    return g_variant_get_int32 (gsettingsValue);
}

Bool readBoolFromVariant (GVariant *gsettingsValue)
{
    return g_variant_get_boolean (gsettingsValue) ? TRUE : FALSE;
}

float readFloatFromVariant (GVariant *gsettingsValue)
{
    return (float) g_variant_get_double (gsettingsValue);
}

CCSSettingColorValue readColorFromVariant (GVariant *gsettingsValue, Bool *success)
{
    const char           *value;
    CCSSettingColorValue color;
    value = g_variant_get_string (gsettingsValue, NULL);

    if (value)
	*success = ccsStringToColor (value, &color);
    else
	*success = FALSE;

    return color;
}

CCSSettingKeyValue readKeyFromVariant (GVariant *gsettingsValue, Bool *success)
{
    const char         *value;
    CCSSettingKeyValue key;
    value = g_variant_get_string (gsettingsValue, NULL);

     if (value)
	 *success = ccsStringToKeyBinding (value, &key);
     else
	 *success = FALSE;

     return key;
}

CCSSettingButtonValue readButtonFromVariant (GVariant *gsettingsValue, Bool *success)
{
    const char            *value;
    CCSSettingButtonValue button;
    value = g_variant_get_string (gsettingsValue, NULL);

    if (value)
	*success = ccsStringToButtonBinding (value, &button);
    else
	*success = FALSE;

    return button;
}

unsigned int readEdgeFromVariant (GVariant *gsettingsValue)
{
    const char   *value;
    value = g_variant_get_string (gsettingsValue, NULL);

    if (value)
	return ccsStringToEdges (value);

    return 0;
}
