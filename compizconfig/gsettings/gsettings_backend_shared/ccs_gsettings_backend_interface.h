#ifndef _COMPIZCONFIG_GSETTINGS_BACKEND_INTERFACE_H
#define _COMPIZCONFIG_GSETTINGS_BACKEND_INTERFACE_H

#include <ccs-defs.h>
#include <ccs-object.h>
#include <glib.h>

COMPIZCONFIG_BEGIN_DECLS

typedef struct _CCSBackend		     CCSBackend;
typedef struct _CCSGSettingsBackend          CCSGSettingsBackend;
typedef struct _CCSGSettingsWrapper	     CCSGSettingsWrapper;
typedef struct _CCSGSettingsBackendInterface CCSGSettingsBackendInterface;
typedef struct _CCSSetting		     CCSSetting;
typedef struct _CCSContext		     CCSContext;

typedef CCSContext * (*CCSGSettingsBackendGetContext) (CCSBackend *);
typedef void (*CCSGSettingsBackendConnectToChangedSignal) (CCSBackend *, CCSGSettingsWrapper *);
typedef CCSGSettingsWrapper * (*CCSGSettingsBackendGetSettingsObjectForPluginWithPath) (CCSBackend *backend,
											const char *plugin,
											const char *path,
											CCSContext *context);
typedef void (*CCSGSettingsBackendRegisterGConfClient) (CCSBackend *backend);
typedef void (*CCSGSettingsBackendUnregisterGConfClient) (CCSBackend *backend);

typedef const char * (*CCSGSettingsBackendGetCurrentProfile) (CCSBackend *backend);

typedef GVariant * (*CCSGSettingsBackendGetExistingProfiles) (CCSBackend *backend);
typedef void (*CCSGSettingsBackendSetExistingProfiles) (CCSBackend *backend, GVariant *value);
typedef void (*CCSGSettingsBackendSetCurrentProfile) (CCSBackend *backend, const gchar *value);

typedef GVariant * (*CCSGSettingsBackendGetPluginsWithSetKeys) (CCSBackend *backend);
typedef void (*CCSGSettingsBackendClearPluginsWithSetKeys) (CCSBackend *backend);

typedef void (*CCSGSettingsBackendUnsetAllChangedPluginKeysInProfile) (CCSBackend *backend, CCSContext *, GVariant *, const char *);

typedef gboolean (*CCSGSettingsBackendUpdateProfile) (CCSBackend *, CCSContext *);
typedef void (*CCSGSettingsBackendUpdateCurrentProfileName) (CCSBackend *backend, const char *profile);

typedef gboolean (*CCSGSettingsBackendAddProfile) (CCSBackend *backend, const char *profile);

typedef int (*CCSGSettingsBackendGetIntegratedOptionIndex) (CCSBackend *backend, CCSSetting *setting);
typedef Bool (*CCSGSettingsBackendReadIntegratedOption) (CCSBackend *backend, CCSSetting *setting, int index);
typedef void (*CCSGSettingsBackendWriteIntegratedOption) (CCSBackend *backend, CCSSetting *setting, int index);

struct _CCSGSettingsBackendInterface
{
    CCSGSettingsBackendGetContext gsettingsBackendGetContext;
    CCSGSettingsBackendConnectToChangedSignal gsettingsBackendConnectToChangedSignal;
    CCSGSettingsBackendGetSettingsObjectForPluginWithPath gsettingsBackendGetSettingsObjectForPluginWithPath;
    CCSGSettingsBackendRegisterGConfClient gsettingsBackendRegisterGConfClient;
    CCSGSettingsBackendUnregisterGConfClient gsettingsBackendUnregisterGConfClient;
    CCSGSettingsBackendGetCurrentProfile   gsettingsBackendGetCurrentProfile;
    CCSGSettingsBackendGetExistingProfiles gsettingsBackendGetExistingProfiles;
    CCSGSettingsBackendSetExistingProfiles gsettingsBackendSetExistingProfiles;
    CCSGSettingsBackendSetCurrentProfile gsettingsBackendSetCurrentProfile;
    CCSGSettingsBackendGetPluginsWithSetKeys gsettingsBackendGetPluginsWithSetKeys;
    CCSGSettingsBackendClearPluginsWithSetKeys gsettingsBackendClearPluginsWithSetKeys;
    CCSGSettingsBackendUnsetAllChangedPluginKeysInProfile gsettingsBackendUnsetAllChangedPluginKeysInProfile;
    CCSGSettingsBackendUpdateProfile gsettingsBackendUpdateProfile;
    CCSGSettingsBackendUpdateCurrentProfileName gsettingsBackendUpdateCurrentProfileName;
    CCSGSettingsBackendAddProfile gsettingsBackendAddProfile;
    CCSGSettingsBackendGetIntegratedOptionIndex gsettingsBackendGetIntegratedOptionIndex;
    CCSGSettingsBackendReadIntegratedOption gsettingsBackendReadIntegratedOption;
    CCSGSettingsBackendWriteIntegratedOption gsettingsBackendWriteIntegratedOption;
};

unsigned int ccsCCSGSettingsBackendInterfaceGetType ();

gboolean
ccsGSettingsBackendUpdateProfile (CCSBackend *backend, CCSContext *context);

void
ccsGSettingsBackendUpdateCurrentProfileName (CCSBackend *backend, const char *profile);

CCSContext *
ccsGSettingsBackendGetContext (CCSBackend *backend);

void
ccsGSettingsBackendConnectToChangedSignal (CCSBackend *backend, CCSGSettingsWrapper *object);

CCSGSettingsWrapper *
ccsGSettingsGetSettingsObjectForPluginWithPath (CCSBackend *backend,
						const char *plugin,
						const char *path,
						CCSContext *context);

void
ccsGSettingsBackendRegisterGConfClient (CCSBackend *backend);

void
ccsGSettingsBackendUnregisterGConfClient (CCSBackend *backend);

const char *
ccsGSettingsBackendGetCurrentProfile (CCSBackend *backend);

GVariant *
ccsGSettingsBackendGetExistingProfiles (CCSBackend *backend);

void
ccsGSettingsBackendSetExistingProfiles (CCSBackend *backend, GVariant *value);

void
ccsGSettingsBackendSetCurrentProfile (CCSBackend *backend, const gchar *value);

GVariant *
ccsGSettingsBackendGetPluginsWithSetKeys (CCSBackend *backend);

void
ccsGSettingsBackendClearPluginsWithSetKeys (CCSBackend *backend);

void
ccsGSettingsBackendUnsetAllChangedPluginKeysInProfile (CCSBackend *backend,
						       CCSContext *context,
						       GVariant   *pluginKeys,
						       const char *profile);

void
ccsGSettingsBackendAddProfile (CCSBackend *backend,
			       const char *profile);


int
ccsGSettingsBackendGetIntegratedOptionIndex (CCSBackend *backend,
					     CCSSetting *setting);

Bool
ccsGSettingsBackendReadIntegratedOption (CCSBackend *backend,
					 CCSSetting *setting,
					 int index);
void
ccsGSettingsBackendWriteIntegratedOption (CCSBackend *backend,
					  CCSSetting *setting,
					  int index);

COMPIZCONFIG_END_DECLS

#endif
