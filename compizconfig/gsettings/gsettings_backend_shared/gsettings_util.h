#ifndef _COMPIZ_GSETTINGS_UTIL_H
#define _COMPIZ_GSETTINGS_UTIL_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <ccs.h>

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
getVariantAtKey (GSettings *settings, char *key, const char *pathName, CCSSettingType type);

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

#endif
