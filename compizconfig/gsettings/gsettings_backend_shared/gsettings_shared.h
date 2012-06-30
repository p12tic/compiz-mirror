#ifndef _COMPIZCONFIG_GSETTINGS_SHARED_H
#define _COMPIZCONFIG_GSETTINGS_SHARED_H

#include <glib.h>

G_BEGIN_DECLS

#include "gsettings_util.h"

extern const char * COMPIZ_SCHEMA_ID;
extern const char * COMPIZCONFIG_SCHEMA_ID;
extern const char * PROFILE_SCHEMA_ID;
#define METACITY "/apps/metacity"
extern const char * COMPIZ;
extern const char * COMPIZ_PROFILEPATH;
extern const char * COMPIZCONFIG;
extern const char * PROFILEPATH;
extern const char * DEFAULTPROF;
extern const char * CORE_NAME;
extern const unsigned int MAX_GSETTINGS_KEY_SIZE;

G_END_DECLS

#endif
