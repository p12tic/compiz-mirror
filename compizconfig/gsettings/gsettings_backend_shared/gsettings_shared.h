#ifndef _COMPIZCONFIG_GSETTINGS_SHARED_H
#define _COMPIZCONFIG_GSETTINGS_SHARED_H

#include <glib.h>

G_BEGIN_DECLS

#include "gsettings_util.h"

extern const char * const COMPIZ_SCHEMA_ID;
extern const char * const COMPIZCONFIG_SCHEMA_ID;
extern const char * const PROFILE_SCHEMA_ID;
#define METACITY "/apps/metacity"
extern const char * const COMPIZ;
extern const char * const COMPIZ_PROFILEPATH;
extern const char * const COMPIZCONFIG;
extern const char * const PROFILEPATH;
extern const char * const DEFAULTPROF;
extern const char * const CORE_NAME;
extern const unsigned int MAX_GSETTINGS_KEY_SIZE;

G_END_DECLS

#endif
