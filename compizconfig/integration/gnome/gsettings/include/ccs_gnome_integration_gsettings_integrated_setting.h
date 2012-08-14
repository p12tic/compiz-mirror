#ifndef _CCS_GNOME_GSETTINGS_INTEGRATED_SETTING_H
#define _CCS_GNOME_GSETTINGS_INTEGRATED_SETTING_H

#include <ccs-defs.h>
#include <ccs-object.h>

COMPIZCONFIG_BEGIN_DECLS

typedef struct _CCSIntegratedSetting      CCSIntegratedSetting;
typedef struct _CCSGNOMEIntegratedSetting CCSGNOMEIntegratedSetting;
typedef struct _CCSGSettingsWrapper       CCSGSettingsWrapper;


CCSIntegratedSetting *
ccsGSettingsIntegratedSettingNew (CCSGNOMEIntegratedSetting *base,
				  CCSGSettingsWrapper       *wrapper,
				  CCSObjectAllocationInterface *ai);

COMPIZCONFIG_END_DECLS

#endif
