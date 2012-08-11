#ifndef _CCS_COMPIZCONFIG_GNOME_GCONF_INTEGRATION
#define _CCS_COMPIZCONFIG_GNOME_GCONF_INTEGRATION

COMPIZCONFIG_BEGIN_DECLS

#ifdef USE_GCONF

typedef struct _CCSBackend CCSBackend;
typedef struct _CCSContext CCSContext;
typedef struct _CCSObjectAllocationInterface CCSObjectAllocationInterface;
typedef struct _CCSIntegration CCSIntegration;


CCSIntegration *
ccsGConfIntegrationBackendNew (CCSBackend *backend,
			       CCSContext *context,
			       CCSObjectAllocationInterface *ai);

void
ccsGConfIntegrationBackendFree (CCSIntegration *integration);

#endif

COMPIZCONFIG_END_DECLS

#endif
