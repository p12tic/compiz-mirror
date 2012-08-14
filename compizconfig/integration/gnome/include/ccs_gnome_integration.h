#ifndef _CCS_COMPIZCONFIG_GNOME_GCONF_INTEGRATION
#define _CCS_COMPIZCONFIG_GNOME_GCONF_INTEGRATION

#include <ccs-defs.h>
#include "ccs_gnome_integration_types.h"

COMPIZCONFIG_BEGIN_DECLS

CCSIntegration *
ccsGNOMEIntegrationBackendNew (CCSBackend *backend,
			       CCSContext *context,
			       CCSIntegratedSettingFactory *factory,
			       CCSIntegratedSettingsStorage *storage,
			       CCSObjectAllocationInterface *ai);

CCSIntegratedSettingFactory *
ccsGConfIntegratedSettingFactoryNew (GConfClient		  *client,
				     CCSObjectAllocationInterface *ai);

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

SpecialOptionType
ccsGNOMEIntegratedSettingGetSpecialOptionType (CCSGNOMEIntegratedSetting *);

const char *
ccsGNOMEIntegratedSettingGetGNOMEName (CCSGNOMEIntegratedSetting *);

CCSGNOMEIntegratedSetting *
ccsGNOMEIntegratedSettingNew (CCSIntegratedSetting *base,
			      SpecialOptionType    type,
			      const char	   *gnomeName,
			      CCSObjectAllocationInterface *ai);

CCSIntegratedSetting *
ccsGConfIntegratedSettingNew (CCSGNOMEIntegratedSetting *base,
			      GConfClient		*client,
			      const char		*section,
			      CCSObjectAllocationInterface *ai);

COMPIZCONFIG_END_DECLS

#endif
