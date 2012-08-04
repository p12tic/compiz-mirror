#ifndef _CCS_GSETTINGS_INTERFACE_H
#define _CCS_GSETTINGS_INTERFACE_H

#include <ccs-defs.h>

COMPIZCONFIG_BEGIN_DECLS

#include <glib.h>
#include <gio/gio.h>
#include <ccs-object.h>

typedef struct _CCSGSettingsWrapper	      CCSGSettingsWrapper;
typedef struct _CCSGSettingsWrapperInterface  CCSGSettingsWrapperInterface;

typedef void (*CCSGSettingsWrapperSetValue) (CCSGSettingsWrapper *, const char *, GVariant *);
typedef GVariant * (*CCSGSettingsWrapperGetValue) (CCSGSettingsWrapper *, const char *);
typedef void (*CCSGSettingsWrapperResetKey) (CCSGSettingsWrapper *, const char *);
typedef char ** (*CCSGSettingsWrapperListKeys) (CCSGSettingsWrapper *);
typedef GSettings * (*CCSGSettingsWrapperGetGSettings) (CCSGSettingsWrapper *);

struct _CCSGSettingsWrapperInterface
{
    CCSGSettingsWrapperSetValue gsettingsWrapperSetValue;
    CCSGSettingsWrapperGetValue gsettingsWrapperGetValue;
    CCSGSettingsWrapperResetKey gsettingsWrapperResetKey;
    CCSGSettingsWrapperListKeys gsettingsWrapperListKeys;
    CCSGSettingsWrapperGetGSettings gsettingsWrapperGetGSettings;
};

struct _CCSGSettingsWrapper
{
    CCSObject object;
};

void ccsGSettingsWrapperSetValue (CCSGSettingsWrapper *, const char *, GVariant *);
GVariant * ccsGSettingsWrapperGetValue (CCSGSettingsWrapper *, const char *);
void ccsGSettingsWrapperResetKey (CCSGSettingsWrapper *, const char *);
char **ccsGSettingsWrapperListKeys (CCSGSettingsWrapper *);
GSettings * ccsGSettingsWrapperGetGSettings (CCSGSettingsWrapper *);

unsigned int ccsCCSGSettingsWrapperInterfaceGetType ();

COMPIZCONFIG_END_DECLS

#endif
