#ifndef _CCS_GSETTINGS_INTERFACE_H
#define _CCS_GSETTINGS_INTERFACE_H

#include <ccs-defs.h>

COMPIZCONFIG_BEGIN_DECLS

#include <glib.h>
#include <ccs-object.h>

typedef struct _CCSGSettingsWrapper	      CCSGSettingsWrapper;
typedef struct _CCSGSettingsWrapperInterface  CCSGSettingsWrapperInterface;

typedef void (*CCSGSettingsWrapperSetValue) (CCSGSettingsWrapper *, const char *, GVariant *);
typedef GVariant * (*CCSGSettingsWrapperGetValue) (CCSGSettingsWrapper *, const char *);
typedef void (*CCSGSettingsWrapperResetKey) (CCSGSettingsWrapper *, const char *);
typedef const char ** (*CCSGSettingsWrapperListKeys) (CCSGSettingsWrapper *);

struct _CCSGSettingsWrapperInterface
{
    CCSGSettingsWrapperSetValue gsettingsWrapperSetValue;
    CCSGSettingsWrapperGetValue gsettingsWrapperGetValue;
    CCSGSettingsWrapperResetKey gsettingsWrapperResetKey;
    CCSGSettingsWrapperListKeys gsettingsWrapperListKeys;
};

struct _CCSGSettingsWrapper
{
    CCSObject object;
};

unsigned int ccsCCSGSettingsWrapperInterfaceGetType ();

COMPIZCONFIG_END_DECLS

#endif
