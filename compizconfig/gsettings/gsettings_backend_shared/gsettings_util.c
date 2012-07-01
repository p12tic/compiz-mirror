#include <glib.h>
#include <string.h>
#include <stdio.h>
#include "gsettings_util.h"
#include "gsettings_shared.h"

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
	printf ("GSettings Backend: Warning: key name %s is not valid in GSettings, it was changed to %s, this may cause problems!\n", gsettingName, translated);

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

    /* FIXME: Truncated and lowercased settings aren't going to work */

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
