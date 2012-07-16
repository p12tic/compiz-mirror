#ifndef _COMPIZ_GSETTINGS_UTIL_H
#define _COMPIZ_GSETTINGS_UTIL_H

#include <ccs.h>

COMPIZCONFIG_BEGIN_DECLS

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

typedef struct _CCSGSettingsBackendPrivate CCSGettingsBackendPrivate;
typedef struct _CCSGSettingsBackendInterface CCSGSettingsBackendInterface;

typedef CCSContext * (*CCSGSettingsBackendGetContext) (CCSBackend *);
typedef void (*CCSGSettingsBackendConnectToChangedSignal) (CCSBackend *, GObject *);

struct _CCSGSettingsBackendInterface
{
    CCSGSettingsBackendGetContext gsettingsBackendGetContext;
    CCSGSettingsBackendConnectToChangedSignal gsettingsBackendConnectToChangedSignal;
};

struct _CCSGSettingsBackendPrivate
{
    CCSContext *context;
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
readListValue (GVariant *gsettingsValue, CCSSettingType listType);

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

CCSContext *
ccsGSettingsBackendGetContext (CCSBackend *backend);

void
ccsGSettingsBackendConnectToChangedSignal (CCSBackend *backend, GObject *object);

COMPIZCONFIG_END_DECLS

#endif
