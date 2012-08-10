#define CCS_LOG_DOMAIN "gsettings"
#include <string.h>
#include <stdio.h>
#include "gsettings_shared.h"
#include "ccs_gsettings_interface.h"
#include "ccs_gsettings_interface_wrapper.h"
#include "ccs_gsettings_backend.h"
#include "ccs_gsettings_backend_interface.h"

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

typedef void (*VariantItemCheckAndInsertFunc) (GVariantBuilder *, const char *item, void *userData);

typedef struct _FindItemInVariantData
{
    gboolean   found;
    const char *item;
} FindItemInVariantData;

typedef struct _InsertIfNotEqualData
{
    gboolean   skipped;
    const char *item;
} InsertIfNotEqualData;

static void
insertIfNotEqual (GVariantBuilder *builder, const char *item, void *userData)
{
    InsertIfNotEqualData *data = (InsertIfNotEqualData *) userData;

    if (g_strcmp0 (item, data->item))
	g_variant_builder_add (builder, "s", item);
    else
	data->skipped = TRUE;
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

gboolean removeItemFromVariant (GVariant   **variant,
				const char *string)
{
    GVariantBuilder newVariantBuilder;

    InsertIfNotEqualData data =
    {
	FALSE,
	string
    };

    g_variant_builder_init (&newVariantBuilder, G_VARIANT_TYPE ("as"));

    rebuildVariant (&newVariantBuilder, *variant, insertIfNotEqual, (void *) &data);

    g_variant_unref (*variant);
    *variant = g_variant_builder_end (&newVariantBuilder);

    return data.skipped;
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
	g_free (uncleanKeyName);

    return ret;
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

gboolean
deleteProfile (CCSBackend *backend,
	       CCSContext *context,
	       const char *profile)
{
    GVariant        *plugins;
    GVariant        *profiles;
    const char      *currentProfile = ccsGSettingsBackendGetCurrentProfile (backend);
    gboolean        ret = FALSE;

    plugins = ccsGSettingsBackendGetPluginsWithSetKeys (backend);
    profiles = ccsGSettingsBackendGetExistingProfiles (backend);

    ccsGSettingsBackendUnsetAllChangedPluginKeysInProfile (backend, context, plugins, currentProfile);
    ccsGSettingsBackendClearPluginsWithSetKeys (backend);

    ret = removeItemFromVariant (&profiles, profile);

    /* Remove the profile from existing-profiles */
    ccsGSettingsBackendSetExistingProfiles (backend, profiles);
    ccsGSettingsBackendUpdateProfile (backend, context);

    /* Since we do not call g_settings_set_value on
     * plugins, we must also unref the variant */
    g_variant_unref (plugins);

    return ret;
}
