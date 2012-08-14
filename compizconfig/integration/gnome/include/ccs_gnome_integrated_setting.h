#ifndef _CCS_GNOME_INTEGRATED_SETTING_H
#define _CCS_GNOME_INTEGRATED_SETTING_H

#include <ccs-defs.h>
#include <ccs-object.h>

#include "ccs_gnome_integration_types.h"

COMPIZCONFIG_BEGIN_DECLS

typedef struct _CCSIntegratedSetting      CCSIntegratedSetting;

typedef struct _CCSGNOMEIntegratedSetting CCSGNOMEIntegratedSetting;
typedef struct _CCSGNOMEIntegratedSettingInterface CCSGNOMEIntegratedSettingInterface;

typedef SpecialOptionType (*CCSGNOMEIntegratedSettingGetSpecialOptionType) (CCSGNOMEIntegratedSetting *);
typedef const char * (*CCSGNOMEIntegratedSettingGetGNOMEName) (CCSGNOMEIntegratedSetting *);

struct _CCSGNOMEIntegratedSettingInterface
{
    CCSGNOMEIntegratedSettingGetSpecialOptionType getSpecialOptionType;
    CCSGNOMEIntegratedSettingGetGNOMEName getGNOMEName;
};

struct _CCSGNOMEIntegratedSetting
{
    CCSObject object;
};

unsigned int ccsCCSGNOMEIntegratedSettingInterfaceGetType ();

Bool
ccsGNOMEIntegrationFindSettingsMatchingPredicate (CCSIntegratedSetting *setting,
						  void		       *userData);

SpecialOptionType
ccsGNOMEIntegratedSettingGetSpecialOptionType (CCSGNOMEIntegratedSetting *);

const char *
ccsGNOMEIntegratedSettingGetGNOMEName (CCSGNOMEIntegratedSetting *);

CCSGNOMEIntegratedSetting *
ccsGNOMEIntegratedSettingNew (CCSIntegratedSetting *base,
			      SpecialOptionType    type,
			      const char	   *gnomeName,
			      CCSObjectAllocationInterface *ai);

COMPIZCONFIG_END_DECLS

#endif
