#ifndef _CCS_COMPIZCONFIG_GNOME_GCONF_INTEGRATION
#define _CCS_COMPIZCONFIG_GNOME_GCONF_INTEGRATION

#include <ccs-defs.h>
#include "ccs_gnome_integration_types.h"

COMPIZCONFIG_BEGIN_DECLS

typedef struct _CCSGNOMEValueChangeData
{
    CCSIntegration *integration;
    CCSIntegratedSettingsStorage *storage;
    CCSIntegratedSettingFactory *factory;
    CCSContext     *context;
} CCSGNOMEValueChangeData;

CCSIntegration *
ccsGNOMEIntegrationBackendNew (CCSBackend *backend,
			       CCSContext *context,
			       CCSIntegratedSettingFactory *factory,
			       CCSIntegratedSettingsStorage *storage,
			       CCSObjectAllocationInterface *ai);

COMPIZCONFIG_END_DECLS

#endif
