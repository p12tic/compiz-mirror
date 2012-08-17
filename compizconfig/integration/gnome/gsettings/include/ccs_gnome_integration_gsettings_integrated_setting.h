#ifndef _CCS_GNOME_GSETTINGS_INTEGRATED_SETTING_H
#define _CCS_GNOME_GSETTINGS_INTEGRATED_SETTING_H

#include <ccs-defs.h>
#include <ccs-object.h>

COMPIZCONFIG_BEGIN_DECLS

typedef struct _CCSIntegratedSetting      CCSIntegratedSetting;
typedef struct _CCSGNOMEIntegratedSettingInfo CCSGNOMEIntegratedSettingInfo;
typedef struct _CCSGSettingsWrapper       CCSGSettingsWrapper;

/**
 * @brief ccsGSettingsIntegratedSettingNew
 * @param base
 * @param wrapper
 * @param ai
 * @return
 *
 * The GSettings implementation of CCSIntegratedSetting *, which will
 * write using a CCSGSettingsWrapper * object when the read and write
 * methods are called.
 */
CCSIntegratedSetting *
ccsGSettingsIntegratedSettingNew (CCSGNOMEIntegratedSettingInfo *base,
				  CCSGSettingsWrapper       *wrapper,
				  CCSObjectAllocationInterface *ai);

COMPIZCONFIG_END_DECLS

#endif
