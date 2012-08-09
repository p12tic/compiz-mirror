#ifndef _CCS_COMPIZCONFIG_GNOME_GCONF_INTEGRATION
#define _CCS_COMPIZCONFIG_GNOME_GCONF_INTEGRATION

COMPIZCONFIG_BEGIN_DECLS

#ifdef USE_GCONF

CCSIntegration *
ccsGConfIntegrationBackendNew (CCSBackend *backend,
			       CCSContext *context,
			       CCSObjectAllocationInterface *ai);

void
ccsGConfIntegrationBackendFree (CCSIntegration *integration);

#endif

COMPIZCONFIG_END_DECLS

#endif
