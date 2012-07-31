#ifndef _COMPIZ_GSETTINGS_UTIL_H
#define _COMPIZ_GSETTINGS_UTIL_H

#include <ccs.h>
#include <ccs-backend.h>

COMPIZCONFIG_BEGIN_DECLS

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

typedef struct _CCSGSettingsBackendPrivate CCSGSettingsBackendPrivate;
typedef struct _CCSGSettingsBackendInterface CCSGSettingsBackendInterface;

typedef CCSContext * (*CCSGSettingsBackendGetContext) (CCSBackend *);
typedef void (*CCSGSettingsBackendConnectToChangedSignal) (CCSBackend *, GObject *);
typedef GSettings * (*CCSGSettingsBackendGetSettingsObjectForPluginWithPath) (CCSBackend *backend,
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
typedef void (*CCSGSettingsBackendClearPluginsWithSetKeys) (CCSBackend *backend, const char *profile);

typedef void (*CCSGSettingsBackendUnsetAllChangedPluginKeysInProfile) (CCSBackend *backend, CCSContext *, GVariant *, const char *);

typedef gboolean (*CCSGSettingsBackendUpdateProfile) (CCSBackend *, CCSContext *);
typedef void (*CCSGSettingsBackendUpdateCurrentProfileName) (CCSBackend *backend, const char *profile);

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
};

unsigned int ccsCCSGSettingsBackendInterfaceGetType ();

gchar *
getSchemaNameForPlugin (const char *plugin);

char *
truncateKeyForGSettings (const char *gsettingName);

char *
translateUnderscoresToDashesForGSettings (const char *truncated);

void
translateToLowercaseForGSettings (char *name);

gchar *
translateKeyForGSettings (const char *gsettingName);

gchar *
translateKeyForCCS (const char *gsettingName);

gboolean
compizconfigTypeHasVariantType (CCSSettingType t);

gboolean
decomposeGSettingsPath (const char *path,
			char **pluginName,
			unsigned int *screenNum);

gboolean
variantIsValidForCCSType (GVariant *gsettingsValue,
			  CCSSettingType settingType);

Bool
appendToPluginsWithSetKeysList (const gchar    *plugin,
				GVariant       *writtenPlugins,
				char	       ***newWrittenPlugins,
				gsize	       *newWrittenPluginsSize);

GObject *
findObjectInListWithPropertySchemaName (const gchar *schemaName,
					GList	    *iter);

CCSSettingList
filterAllSettingsMatchingType (CCSSettingType type,
			       CCSSettingList settingList);

CCSSettingList
filterAllSettingsMatchingPartOfStringIgnoringDashesUnderscoresAndCase (const gchar *keyName,
								       CCSSettingList sList);

CCSSetting *
attemptToFindCCSSettingFromLossyName (CCSSettingList settingList, const gchar *lossyName, CCSSettingType type);

Bool
findSettingAndPluginToUpdateFromPath (GSettings  *settings,
				      const char *path,
				      const gchar *keyName,
				      CCSContext *context,
				      CCSPlugin **plugin,
				      CCSSetting **setting,
				      char **uncleanKeyName);

Bool updateSettingWithGSettingsKeyName (CCSBackend *backend,
					GSettings *settings,
					gchar     *keyName,
					CCSBackendUpdateFunc updateSetting);

GList *
variantTypeToPossibleSettingType (const gchar *vt);

gchar *
makeCompizProfilePath (const gchar *profilename);

gchar *
makeCompizPluginPath (const gchar *profileName, const gchar *pluginName);

gchar *
getNameForCCSSetting (CCSSetting *setting);

Bool
checkReadVariantIsValid (GVariant *gsettingsValue, CCSSettingType type, const gchar *pathName);

GVariant *
getVariantAtKey (GSettings *settings, const char *key, const char *pathName, CCSSettingType type);

const char * readStringFromVariant (GVariant *gsettingsValue);

int readIntFromVariant (GVariant *gsettingsValue);

Bool readBoolFromVariant (GVariant *gsettingsValue);

float readFloatFromVariant (GVariant *gsettingsValue);

CCSSettingColorValue readColorFromVariant (GVariant *gsettingsValue, Bool *success);

CCSSettingKeyValue readKeyFromVariant (GVariant *gsettingsValue, Bool *success);

CCSSettingButtonValue readButtonFromVariant (GVariant *gsettingsValue, Bool *success);

unsigned int readEdgeFromVariant (GVariant *gsettingsValue);

CCSSettingValueList
readListValue (GVariant *gsettingsValue, CCSSetting *setting);

Bool
writeListValue (CCSSettingValueList list,
		CCSSettingType	    listType,
		GVariant	    **gsettingsValue);

Bool writeStringToVariant (const char *value, GVariant **variant);

Bool writeFloatToVariant (float value, GVariant **variant);

Bool writeIntToVariant (int value, GVariant **variant);

Bool writeBoolToVariant (Bool value, GVariant **variant);

Bool writeColorToVariant (CCSSettingColorValue value, GVariant **variant);

Bool writeKeyToVariant (CCSSettingKeyValue key, GVariant **variant);

Bool writeButtonToVariant (CCSSettingButtonValue button, GVariant **variant);

Bool writeEdgeToVariant (unsigned int edges, GVariant **variant);

void writeVariantToKey (GSettings  *settings,
			const char *key,
			GVariant   *value);

typedef int (*ComparisonPredicate) (const void *s1, const void *s2);

int voidcmp0 (const void *v1, const void *v2);

gboolean
deleteProfile (CCSBackend *backend,
	       CCSContext *context,
	       const char *profile);

gboolean
appendStringToVariantIfUnique (GVariant	  **variant,
			       const char *string);

void
removeItemFromVariant (GVariant	  **variant,
		       const char *string);

gboolean
ccsGSettingsBackendUpdateProfile (CCSBackend *backend, CCSContext *context);

void
ccsGSettingsBackendUpdateCurrentProfileName (CCSBackend *backend, const char *profile);

CCSContext *
ccsGSettingsBackendGetContext (CCSBackend *backend);

void
ccsGSettingsBackendConnectToChangedSignal (CCSBackend *backend, GObject *object);

GSettings *
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
ccsGSettingsBackendClearPluginsWithSetKeys (CCSBackend *backend, const char *profile);

void
ccsGSettingsBackendUnsetAllChangedPluginKeysInProfile (CCSBackend *backend,
						       CCSContext *context,
						       GVariant   *pluginKeys,
						       const char *profile);

/* Default implementations, should be moved */

void
ccsGSettingsBackendUpdateCurrentProfileNameDefault (CCSBackend *backend, const char *profile);

gboolean
ccsGSettingsBackendUpdateProfileDefault (CCSBackend *backend, CCSContext *context);

COMPIZCONFIG_END_DECLS

#endif
