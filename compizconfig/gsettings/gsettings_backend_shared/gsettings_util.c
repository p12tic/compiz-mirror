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
translateKeyForGSettings (char *gsettingName)
{
    gchar *clean        = NULL;
    gchar **delimited   = NULL;
    guint i		= 0;

    /* Replace underscores with dashes */
    delimited = g_strsplit (gsettingName, "_", 0);

    clean = g_strjoinv ("-", delimited);

    gchar *ret = g_strndup (clean, 1024);

    if (strlen (clean) > 1024)
	printf ("GSettings Backend: Warning: key name %s is not valid in GSettings, it was changed to %s, this may cause problems!\n", clean, ret);

    /* Everything must be lowercase */
    for (; i < strlen (ret); i++)
	ret[i] = g_ascii_tolower (ret[i]);

    g_free (clean);
    g_strfreev (delimited);

    return ret;
}

gchar *
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
