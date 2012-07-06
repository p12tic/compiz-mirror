#include <string.h>
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

    schemaName = g_strconcat (COMPIZ_SCHEMA_ID, ".", plugin, NULL);

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
    char         *path = (char *) pathInput;
    char         *token;

    path += strlen (COMPIZ) + 1;

    *pluginName = NULL;
    *screenNum = 0;

    token = strsep (&path, "/"); /* Profile name */
    if (!token)
	return FALSE;

    token = strsep (&path, "/"); /* plugins */
    if (!token)
	return FALSE;

    token = strsep (&path, "/"); /* plugin */
    if (!token)
	return FALSE;

    *pluginName = g_strdup (token);

    if (!*pluginName)
	return FALSE;

    token = strsep (&path, "/"); /* screen%i */
    if (!token)
    {
	g_free (pluginName);
	*pluginName = NULL;
	return FALSE;
    }

    sscanf (token, "screen%d", screenNum);

    return TRUE;
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
