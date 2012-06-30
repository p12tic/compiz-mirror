#ifndef _COMPIZ_GSETTINGS_UTIL_H
#define _COMPIZ_GSETTINGS_UTIL_H

#include <glib.h>

gchar *
getSchemaNameForPlugin (const char *plugin);

gchar *
translateKeyForGSettings (char *gsettingName);

gchar *
translateKeyForCCS (char *gsettingName);

#endif
