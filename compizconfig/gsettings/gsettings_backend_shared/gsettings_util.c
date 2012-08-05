#define CCS_LOG_DOMAIN "gsettings"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "gsettings_shared.h"

INTERFACE_TYPE (CCSGSettingsBackendInterface);

const CCSBackendInfo gsettingsBackendInfo =
{
    "gsettings",
    "GSettings Configuration Backend",
    "GSettings Configuration Backend for libccs",
    TRUE,
    TRUE,
    1
};

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

CCSGSettingsWrapper *
findCCSGSettingsWrapperBySchemaName (const gchar *schemaName,
				     GList	 *iter)
{
    while (iter)
    {
	CCSGSettingsWrapper   *obj = iter->data;
	const gchar	      *name = ccsGSettingsWrapperGetSchemaName (obj);

	if (g_strcmp0 (name, schemaName) != 0)
	    obj = NULL;

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

Bool
findSettingAndPluginToUpdateFromPath (CCSGSettingsWrapper  *settings,
				      const char *path,
				      const gchar *keyName,
				      CCSContext *context,
				      CCSPlugin **plugin,
				      CCSSetting **setting,
				      char **uncleanKeyName)
{
    char         *pluginName;
    unsigned int screenNum;

    if (!decomposeGSettingsPath (path, &pluginName, &screenNum))
	return FALSE;

    *plugin = ccsFindPlugin (context, pluginName);

    if (*plugin)
    {
	*uncleanKeyName = translateKeyForCCS (keyName);

	*setting = ccsFindSetting (*plugin, *uncleanKeyName);
	if (!*setting)
	{
	    /* Couldn't find setting straight off the bat,
	     * try and find the best match */
	    GVariant *value = ccsGSettingsWrapperGetValue (settings, keyName);

	    if (value)
	    {
		GList *possibleSettingTypes = variantTypeToPossibleSettingType (g_variant_get_type_string (value));
		GList *iter = possibleSettingTypes;

		while (iter)
		{
		    *setting = attemptToFindCCSSettingFromLossyName (ccsGetPluginSettings (*plugin),
								     keyName,
								     (CCSSettingType) GPOINTER_TO_INT (iter->data));

		    if (*setting)
			break;

		    iter = iter->next;
		}

		g_list_free (possibleSettingTypes);
		g_variant_unref (value);
	    }
	}
    }

    g_free (pluginName);

    if (!*plugin || !*setting)
	return FALSE;

    return TRUE;
}

Bool
updateSettingWithGSettingsKeyName (CCSBackend *backend,
				   CCSGSettingsWrapper *settings,
				   const gchar     *keyName,
				   CCSBackendUpdateFunc updateSetting)
{
    CCSContext   *context = ccsGSettingsBackendGetContext (backend);
    char	 *uncleanKeyName = NULL;
    char	 *pathOrig;
    CCSPlugin    *plugin;
    CCSSetting   *setting;
    Bool         ret = TRUE;

    pathOrig = strdup (ccsGSettingsWrapperGetPath (settings));

    if (findSettingAndPluginToUpdateFromPath (settings, pathOrig, keyName, context, &plugin, &setting, &uncleanKeyName))
	(*updateSetting) (backend, context, plugin, setting);
    else
    {
	/* We hit a situation where either the key stored in GSettings couldn't be
	 * matched at all to a key in the xml file, or where there were multiple matches.
	 * Unfortunately, there isn't much we can do about this, other than try
	 * and warn the user and bail out. It just means that if the key was updated
	 * externally we won't know about the change until the next reload of settings */
	 ccsWarning ("Unable to find setting %s, for path %s", uncleanKeyName, pathOrig);
	 ret = FALSE;
    }

    g_free (pathOrig);

    if (uncleanKeyName)
	free (uncleanKeyName);

    return ret;
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
getVariantAtKey (CCSGSettingsWrapper *settings, const char *key, const char *pathName, CCSSettingType type)
{
    GVariant *gsettingsValue = ccsGSettingsWrapperGetValue (settings, key);

    if (!checkReadVariantIsValid (gsettingsValue, type, pathName))
    {
	g_variant_unref (gsettingsValue);
	return NULL;
    }

    return gsettingsValue;
}

CCSSettingValueList
readBoolListValue (GVariantIter *iter, guint nItems, CCSSetting *setting, CCSObjectAllocationInterface *ai)
{
    CCSSettingValueList list = NULL;
    Bool *array = (*ai->calloc_) (ai->allocator, 1, nItems * sizeof (Bool));
    Bool *arrayCounter = array;
    gboolean value;

    if (!array)
	return NULL;

    /* Reads each item from the variant into arrayCounter */
    while (g_variant_iter_loop (iter, "b", &value))
	*arrayCounter++ = value ? TRUE : FALSE;

    list = ccsGetValueListFromBoolArray (array, nItems, setting);
    free (array);

    return list;
}

CCSSettingValueList
readIntListValue (GVariantIter *iter, guint nItems, CCSSetting *setting, CCSObjectAllocationInterface *ai)
{
    CCSSettingValueList list = NULL;
    int *array = (*ai->calloc_) (ai->allocator, 1, nItems * sizeof (int));
    int *arrayCounter = array;
    gint value;

    if (!array)
	return NULL;

    /* Reads each item from the variant into arrayCounter */
    while (g_variant_iter_loop (iter, "i", &value))
	*arrayCounter++ = value;

    list = ccsGetValueListFromIntArray (array, nItems, setting);
    free (array);

    return list;
}

CCSSettingValueList
readFloatListValue (GVariantIter *iter, guint nItems, CCSSetting *setting, CCSObjectAllocationInterface *ai)
{
    CCSSettingValueList list = NULL;
    float *array = (*ai->calloc_) (ai->allocator, 1, nItems * sizeof (float));
    float *arrayCounter = array;
    gdouble value;

    if (!array)
	return NULL;

    /* Reads each item from the variant into arrayCounter */
    while (g_variant_iter_loop (iter, "d", &value))
	*arrayCounter++ = value;

    list = ccsGetValueListFromFloatArray (array, nItems, setting);
    free (array);

    return list;
}

CCSSettingValueList
readStringListValue (GVariantIter *iter, guint nItems, CCSSetting *setting, CCSObjectAllocationInterface *ai)
{
    CCSSettingValueList list = NULL;
    const gchar **array = (*ai->calloc_) (ai->allocator, 1, (nItems + 1) * sizeof (gchar *));
    const gchar **arrayCounter = array;
    gchar *value;

    if (!array)
	return NULL;

    array[nItems] = NULL;

    /* Reads each item from the variant into arrayCounter */
    while (g_variant_iter_next (iter, "s", &value))
	*arrayCounter++ = value;

    list = ccsGetValueListFromStringArray (array, nItems, setting);
    g_strfreev ((char **) array);

    return list;
}

CCSSettingValueList
readColorListValue (GVariantIter *iter, guint nItems, CCSSetting *setting, CCSObjectAllocationInterface *ai)
{
    CCSSettingValueList list = NULL;
    char		 *colorValue;
    CCSSettingColorValue *array = (*ai->calloc_) (ai->allocator, 1, nItems * sizeof (CCSSettingColorValue));
    unsigned int i = 0;

    if (!array)
	return NULL;

    while (g_variant_iter_loop (iter, "s", &colorValue))
    {
	ccsStringToColor (colorValue,
			  &array[i]);
	i++;
    }

    list = ccsGetValueListFromColorArray (array, nItems, setting);
    free (array);

    return list;
}

CCSSettingValueList
readListValue (GVariant *gsettingsValue, CCSSetting *setting, CCSObjectAllocationInterface *ai)
{
    CCSSettingType      listType = ccsSettingGetInfo (setting)->forList.listType;
    gboolean		hasVariantType;
    unsigned int        nItems;
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
	list = readBoolListValue (&iter, nItems, setting, ai);
	break;
    case TypeInt:
	list = readIntListValue (&iter, nItems, setting, ai);
	break;
    case TypeFloat:
	list = readFloatListValue (&iter, nItems, setting, ai);
	break;
    case TypeString:
    case TypeMatch:
	list = readStringListValue (&iter, nItems, setting, ai);
	break;
    case TypeColor:
	list = readColorListValue (&iter, nItems, setting, ai);
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

GVariant *
writeBoolListValue (CCSSettingValueList list)
{
    GVariant *value = NULL;
    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("ab"));
    while (list)
    {
	g_variant_builder_add (builder, "b", list->data->value.asBool);
	list = list->next;
    }
    value = g_variant_new ("ab", builder);
    g_variant_builder_unref (builder);

    return value;
}

GVariant *
writeIntListValue (CCSSettingValueList list)
{
    GVariant *value = NULL;
    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("ai"));
    while (list)
    {
	g_variant_builder_add (builder, "i", list->data->value.asInt);
	list = list->next;
    }
    value = g_variant_new ("ai", builder);
    g_variant_builder_unref (builder);

    return value;
}

GVariant *
writeFloatListValue (CCSSettingValueList list)
{
    GVariant *value = NULL;
    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("ad"));
    while (list)
    {
	g_variant_builder_add (builder, "d", (gdouble) list->data->value.asFloat);
	list = list->next;
    }
    value = g_variant_new ("ad", builder);
    g_variant_builder_unref (builder);

    return value;
}

GVariant *
writeStringListValue (CCSSettingValueList list)
{
    GVariant *value = NULL;
    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
    while (list)
    {
	g_variant_builder_add (builder, "s", list->data->value.asString);
	list = list->next;
    }
    value = g_variant_new ("as", builder);
    g_variant_builder_unref (builder);

    return value;
}

GVariant *
writeMatchListValue (CCSSettingValueList list)
{
    GVariant *value = NULL;
    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
    while (list)
    {
	g_variant_builder_add (builder, "s", list->data->value.asMatch);
	list = list->next;
    }
    value = g_variant_new ("as", builder);
    g_variant_builder_unref (builder);

    return value;
}

GVariant *
writeColorListValue (CCSSettingValueList list)
{
    GVariant *value = NULL;
    GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
    char *item;
    while (list)
    {
	item = ccsColorToString (&list->data->value.asColor);
	g_variant_builder_add (builder, "s", item);
	g_free (item);
	list = list->next;
    }
    value = g_variant_new ("as", builder);
    g_variant_builder_unref (builder);

    return value;
}

Bool
writeListValue (CCSSettingValueList list,
		CCSSettingType	    listType,
		GVariant	    **gsettingsValue)
{
    GVariant *value = NULL;

    switch (listType)
    {
    case TypeBool:
	value = writeBoolListValue (list);
	break;
    case TypeInt:
	value = writeIntListValue (list);
	break;
    case TypeFloat:
	 value = writeFloatListValue (list);
	break;
    case TypeString:
	value = writeStringListValue (list);
	break;
    case TypeMatch:
	value = writeMatchListValue (list);
	break;
    case TypeColor:
	value = writeColorListValue (list);
	break;
    default:
	ccsWarning ("Attempt to write unsupported list type %d!",
	       listType);
	return FALSE;
	break;
    }

    *gsettingsValue = value;
    return TRUE;
}

Bool writeStringToVariant (const char *value, GVariant **variant)
{
    *variant = g_variant_new_string (value);
    return TRUE;
}

Bool writeFloatToVariant (float value, GVariant **variant)
{
    *variant = g_variant_new_double ((double) value);
    return TRUE;
}

Bool writeIntToVariant (int value, GVariant **variant)
{
    *variant = g_variant_new_int32 (value);
    return TRUE;
}

Bool writeBoolToVariant (Bool value, GVariant **variant)
{
    *variant = g_variant_new_boolean (value);
    return TRUE;
}

Bool writeColorToVariant (CCSSettingColorValue value, GVariant **variant)
{
    char                 *colString;

    colString = ccsColorToString (&value);
    if (!colString)
	return FALSE;

    *variant = g_variant_new_string (colString);
    free (colString);

    return TRUE;
}

Bool writeKeyToVariant (CCSSettingKeyValue key, GVariant **variant)
{
    char               *keyString;

    keyString = ccsKeyBindingToString (&key);
    if (!keyString)
	return FALSE;

    *variant = g_variant_new_string (keyString);
    free (keyString);

    return TRUE;
}

Bool writeButtonToVariant (CCSSettingButtonValue button, GVariant **variant)
{
    char                  *buttonString;

    buttonString = ccsButtonBindingToString (&button);
    if (!buttonString)
	return FALSE;

    *variant = g_variant_new_string (buttonString);
    free (buttonString);
    return TRUE;
}

Bool writeEdgeToVariant (unsigned int edges, GVariant **variant)
{
    char         *edgeString;

    edgeString = ccsEdgesToString (edges);
    if (!edgeString)
	return FALSE;

    *variant = g_variant_new_string (edgeString);
    free (edgeString);
    return TRUE;
}

void
writeVariantToKey (CCSGSettingsWrapper  *settings,
		   const char *key,
		   GVariant   *value)
{
    ccsGSettingsWrapperSetValue (settings, key, value);
}

typedef void (*VariantItemCheckAndInsertFunc) (GVariantBuilder *, const char *item, void *userData);

typedef struct _FindItemInVariantData
{
    gboolean   found;
    const char *item;
} FindItemInVariantData;

static void
insertIfNotEqual (GVariantBuilder *builder, const char *item, void *userData)
{
    const char *cmp = (const char *) userData;

    if (g_strcmp0 (item, cmp))
	g_variant_builder_add (builder, "s", item);
}

static void
findItemForVariantData (GVariantBuilder *builder, const char *item, void *userData)
{
    FindItemInVariantData *data = (FindItemInVariantData *) userData;

    if (!data->found)
	data->found = g_str_equal (data->item, item);

    g_variant_builder_add (builder, "s", item);
}

static void
rebuildVariant (GVariantBuilder		      *builder,
		GVariant		      *originalVariant,
		VariantItemCheckAndInsertFunc checkAndInsert,
		void			      *userData)
{
    GVariantIter    iter;
    char	    *str;

    g_variant_iter_init (&iter, originalVariant);
    while (g_variant_iter_loop (&iter, "s", &str))
    {
	(*checkAndInsert) (builder, str, userData);
    }
}

gboolean
appendStringToVariantIfUnique (GVariant	  **variant,
			       const char *string)
{
    GVariantBuilder newVariantBuilder;
    FindItemInVariantData findItemData;

    memset (&findItemData, 0, sizeof (FindItemInVariantData));
    g_variant_builder_init (&newVariantBuilder, G_VARIANT_TYPE ("as"));

    findItemData.item = string;

    rebuildVariant  (&newVariantBuilder, *variant, findItemForVariantData, &findItemData);

    if (!findItemData.found)
	g_variant_builder_add (&newVariantBuilder, "s", string);

    g_variant_unref (*variant);
    *variant = g_variant_builder_end (&newVariantBuilder);

    return !findItemData.found;
}

void
removeItemFromVariant (GVariant	  **variant,
		       const char *string)
{
    GVariantBuilder newVariantBuilder;

    g_variant_builder_init (&newVariantBuilder, G_VARIANT_TYPE ("as"));

    rebuildVariant (&newVariantBuilder, *variant, insertIfNotEqual, (void *) string);

    g_variant_unref (*variant);
    *variant = g_variant_builder_end (&newVariantBuilder);
}

void
resetOptionToDefault (CCSBackend *backend, CCSSetting * setting)
{
    CCSGSettingsWrapper  *settings = getSettingsObjectForCCSSetting (backend, setting);
    char *cleanSettingName = translateKeyForGSettings (ccsSettingGetName (setting));

    ccsGSettingsWrapperResetKey (settings, cleanSettingName);

    free (cleanSettingName);
}

gchar *
makeSettingPath (const char *currentProfile, CCSSetting *setting)
{
    return makeCompizPluginPath (currentProfile,
				 ccsPluginGetName (ccsSettingGetParent (setting)));
}

CCSGSettingsWrapper *
getSettingsObjectForCCSSetting (CCSBackend *backend, CCSSetting *setting)
{
    CCSGSettingsWrapper *ret = NULL;
    gchar *pathName = makeSettingPath (ccsGSettingsBackendGetCurrentProfile (backend), setting);

    ret = ccsGSettingsGetSettingsObjectForPluginWithPath (backend,
							  ccsPluginGetName (ccsSettingGetParent (setting)),
							  pathName,
							  ccsPluginGetContext (ccsSettingGetParent (setting)));

    g_free (pathName);
    return ret;
}

gboolean
ccsGSettingsBackendAddProfileDefault (CCSBackend *backend, const char *profile)
{
    GVariant        *profiles;
    gboolean	    ret = FALSE;

    profiles = ccsGSettingsBackendGetExistingProfiles (backend);
    if (appendStringToVariantIfUnique (&profiles, profile))
    {
	ret = TRUE;
	ccsGSettingsBackendSetExistingProfiles (backend, profiles);
    }
    else
	g_variant_unref (profiles);

    return ret;
}

void
ccsGSettingsBackendUpdateCurrentProfileNameDefault (CCSBackend *backend, const char *profile)
{
    ccsGSettingsBackendAddProfile (backend, profile);
    ccsGSettingsBackendSetCurrentProfile (backend, profile);
}

gboolean
ccsGSettingsBackendUpdateProfileDefault (CCSBackend *backend, CCSContext *context)
{
    const char *currentProfile = ccsGSettingsBackendGetCurrentProfile (backend);
    const char *ccsProfile = ccsGetProfile (context);
    char *profile = NULL;

    if (!ccsProfile)
	profile = strdup (DEFAULTPROF);
    else
	profile = strdup (ccsProfile);

    if (!strlen (profile))
    {
	free (profile);
	profile = strdup (DEFAULTPROF);
    }

    if (g_strcmp0 (profile, currentProfile))
	ccsGSettingsBackendUpdateCurrentProfileName (backend, profile);

    free (profile);

    return TRUE;
}

gboolean
deleteProfile (CCSBackend *backend,
	       CCSContext *context,
	       const char *profile)
{
    GVariant        *plugins;
    GVariant        *profiles;
    const char      *currentProfile = ccsGSettingsBackendGetCurrentProfile (backend);

    plugins = ccsGSettingsBackendGetPluginsWithSetKeys (backend);
    profiles = ccsGSettingsBackendGetExistingProfiles (backend);

    ccsGSettingsBackendUnsetAllChangedPluginKeysInProfile (backend, context, plugins, currentProfile);
    ccsGSettingsBackendClearPluginsWithSetKeys (backend);

    removeItemFromVariant (&profiles, profile);

    /* Remove the profile from existing-profiles */
    ccsGSettingsBackendSetExistingProfiles (backend, profiles);

    ccsGSettingsBackendUpdateProfile (backend, context);

    return TRUE;
}

void
ccsGSettingsBackendUnsetAllChangedPluginKeysInProfileDefault (CCSBackend *backend,
							      CCSContext *context,
							      GVariant *pluginsWithChangedKeys,
							      const char * profile)
{
    GVariantIter    iter;
    char            *plugin;

    g_variant_iter_init (&iter, pluginsWithChangedKeys);
    while (g_variant_iter_loop (&iter, "s", &plugin))
    {
	CCSGSettingsWrapper *settings;
	gchar *pathName = makeCompizPluginPath (profile, plugin);

	settings = ccsGSettingsGetSettingsObjectForPluginWithPath (backend, plugin, pathName, context);
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
}

CCSContext *
ccsGSettingsBackendGetContext (CCSBackend *backend)
{
    return (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendGetContext) (backend);
}


void
ccsGSettingsBackendConnectToChangedSignal (CCSBackend *backend,
					   CCSGSettingsWrapper *object)
{
     (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendConnectToChangedSignal) (backend, object);
}

CCSGSettingsWrapper *
ccsGSettingsGetSettingsObjectForPluginWithPath (CCSBackend *backend,
						const char *plugin,
						const char *path,
						CCSContext *context)
{
    return (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendGetSettingsObjectForPluginWithPath) (backend, plugin, path, context);
}

void
ccsGSettingsBackendRegisterGConfClient (CCSBackend *backend)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendRegisterGConfClient) (backend);
}

void
ccsGSettingsBackendUnregisterGConfClient (CCSBackend *backend)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendUnregisterGConfClient) (backend);
}

const char *
ccsGSettingsBackendGetCurrentProfile (CCSBackend *backend)
{
    return (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendGetCurrentProfile) (backend);
}

GVariant *
ccsGSettingsBackendGetExistingProfiles (CCSBackend *backend)
{
    return (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendGetExistingProfiles) (backend);
}

void
ccsGSettingsBackendSetExistingProfiles (CCSBackend *backend, GVariant *value)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendSetExistingProfiles) (backend, value);
}

void
ccsGSettingsBackendSetCurrentProfile (CCSBackend *backend, const gchar *value)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendSetCurrentProfile) (backend, value);
}

GVariant *
ccsGSettingsBackendGetPluginsWithSetKeys (CCSBackend *backend)
{
    return (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendGetPluginsWithSetKeys) (backend);
}

void
ccsGSettingsBackendClearPluginsWithSetKeys (CCSBackend *backend)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendClearPluginsWithSetKeys) (backend);
}

void
ccsGSettingsBackendUnsetAllChangedPluginKeysInProfile (CCSBackend *backend,
						       CCSContext *context,
						       GVariant   *pluginKeys,
						       const char *profile)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendUnsetAllChangedPluginKeysInProfile) (backend, context, pluginKeys, profile);
}

gboolean
ccsGSettingsBackendUpdateProfile (CCSBackend *backend, CCSContext *context)
{
    return (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendUpdateProfile) (backend, context);
}

void
ccsGSettingsBackendUpdateCurrentProfileName (CCSBackend *backend, const char *profile)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendUpdateCurrentProfileName) (backend, profile);
}

void
ccsGSettingsBackendAddProfile (CCSBackend *backend, const char *profile)
{
    (*(GET_INTERFACE (CCSGSettingsBackendInterface, backend))->gsettingsBackendAddProfile) (backend, profile);
}

CCSContext *
ccsGSettingsBackendGetContext (CCSBackend *backend);
