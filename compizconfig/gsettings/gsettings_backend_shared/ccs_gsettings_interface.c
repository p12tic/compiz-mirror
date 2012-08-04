#include <gio/gio.h>
#include "ccs_gsettings_interface.h"

INTERFACE_TYPE (CCSGSettingsWrapperInterface);

void
ccsGSettingsWrapperSetValue (CCSGSettingsWrapper *wrapper,
			     const char *key,
			     GVariant *value)
{
    (*(GET_INTERFACE (CCSGSettingsWrapperInterface, wrapper))->gsettingsWrapperSetValue) (wrapper, key, value);
}

GVariant *
ccsGSettingsWrapperGetValue (CCSGSettingsWrapper *wrapper,
			     const char *key)
{
    return (*(GET_INTERFACE (CCSGSettingsWrapperInterface, wrapper))->gsettingsWrapperGetValue) (wrapper, key);
}

void
ccsGSettingsWrapperResetKey (CCSGSettingsWrapper *wrapper,
			     const char *key)
{
    return (*(GET_INTERFACE (CCSGSettingsWrapperInterface, wrapper))->gsettingsWrapperResetKey) (wrapper, key);
}

char **
ccsGSettingsWrapperListKeys (CCSGSettingsWrapper *wrapper)
{
    return (*(GET_INTERFACE (CCSGSettingsWrapperInterface, wrapper))->gsettingsWrapperListKeys) (wrapper);
}

GSettings *
ccsGSettingsWrapperGetGSettings (CCSGSettingsWrapper *wrapper)
{
    return (*(GET_INTERFACE (CCSGSettingsWrapperInterface, wrapper))->gsettingsWrapperGetGSettings) (wrapper);
}
