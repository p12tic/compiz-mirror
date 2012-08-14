#ifndef _CCS_GNOME_GCONF_INTEGRATED_SETTING_H
#define _CCS_GNOME_GCONF_INTEGRATED_SETTING_H

#include <ccs-defs.h>
#include <ccs-object.h>

COMPIZCONFIG_BEGIN_DECLS

typedef struct _CCSIntegratedSetting      CCSIntegratedSetting;
typedef struct _CCSGNOMEIntegratedSetting CCSGNOMEIntegratedSetting;
typedef struct _GConfClient		   GConfClient;

CCSIntegratedSetting *
ccsGConfIntegratedSettingNew (CCSGNOMEIntegratedSetting *base,
			      GConfClient		*client,
			      const char		*section,
			      CCSObjectAllocationInterface *ai);

COMPIZCONFIG_END_DECLS

#endif
