#ifndef _COMPIZCONFIG_CCS_GSETTINGS_BACKEND_H
#define _COMPIZCONFIG_CCS_GSETTINGS_BACKEND_H

#include <ccs-defs.h>
#include <ccs-backend.h>
#include <glib.h>

COMPIZCONFIG_BEGIN_DECLS

#ifdef USE_GCONF
#include <gconf/gconf-client.h>
#endif

typedef struct _CCSBackend CCSBackend;
typedef struct _CCSGSettingsWrapper CCSGSettingsWrapper;

/* FIXME: This should not be here, but getting it to be
 * part of its own file is a lot more work than we currently
 * have time to do right now */
struct _CCSGSettingsBackendPrivate
{
    GList	   *settingsList;
    CCSGSettingsWrapper *compizconfigSettings;
    CCSGSettingsWrapper *currentProfileSettings;

    char	    *currentProfile;
    CCSContext  *context;

    CCSIntegration *integration;

};

Bool
ccsGSettingsBackendAttachNewToBackend (CCSBackend *backend, CCSContext *context);

void
ccsGSettingsBackendDetachFromBackend (CCSBackend *backend);

/* Default implementations, should be moved */

void
ccsGSettingsBackendUpdateCurrentProfileNameDefault (CCSBackend *backend, const char *profile);

gboolean
ccsGSettingsBackendUpdateProfileDefault (CCSBackend *backend, CCSContext *context);

void
ccsGSettingsBackendUnsetAllChangedPluginKeysInProfileDefault (CCSBackend *backend,
							      CCSContext *context,
							      GVariant *pluginsWithChangedKeys,
							      const char * profile);

gboolean ccsGSettingsBackendAddProfileDefault (CCSBackend *backend,
					       const char *profile);

COMPIZCONFIG_END_DECLS

#endif
