#ifndef _COMPIZCONFIG_GSETTINGS_SHARED_H
#define _COMPIZCONFIG_GSETTINGS_SHARED_H

#include <glib.h>

G_BEGIN_DECLS

#include "gsettings_util.h"

extern const char * const PLUGIN_SCHEMA_ID_PREFIX;
extern const char * const COMPIZCONFIG_SCHEMA_ID;
extern const char * const COMPIZCONFIG_PATH;
extern const char * const PROFILE_SCHEMA_ID;
#define METACITY "/apps/metacity"
extern const char * const PROFILE_PATH_PREFIX;
extern const char * const DEFAULTPROF;
extern const unsigned int MAX_GSETTINGS_KEY_SIZE;

G_END_DECLS

#endif
