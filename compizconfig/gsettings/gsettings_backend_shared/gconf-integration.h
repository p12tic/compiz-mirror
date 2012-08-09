#ifndef _CCS_COMPIZCONFIG_GNOME_GCONF_INTEGRATION
#define _CCS_COMPIZCONFIG_GNOME_GCONF_INTEGRATION

COMPIZCONFIG_BEGIN_DECLS

#ifdef USE_GCONF

CCSIntegrationBackend *
ccsGConfIntegrationBackendNew (CCSBackend *backend,
			       CCSContext *context,
			       CCSObjectAllocationInterface *ai);

void
ccsGConfIntegrationBackendFree (CCSIntegrationBackend *integration);

#endif

COMPIZCONFIG_END_DECLS

#endif
