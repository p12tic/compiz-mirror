#ifndef _CCS_COMPIZCONFIG_GNOME_GCONF_INTEGRATION
#define _CCS_COMPIZCONFIG_GNOME_GCONF_INTEGRATION

COMPIZCONFIG_BEGIN_DECLS

typedef struct _CCSBackend CCSBackend;
typedef struct _CCSContext CCSContext;
typedef struct _CCSObjectAllocationInterface CCSObjectAllocationInterface;
typedef struct _CCSIntegration CCSIntegration;
typedef struct _GConfClient GConfClient;


CCSIntegration *
ccsGConfIntegrationBackendNew (CCSBackend *backend,
			       CCSContext *context,
			       CCSObjectAllocationInterface *ai);

CCSIntegration *
ccsGConfIntegrationBackendNewWithClient (CCSBackend *backend,
					 CCSContext *context,
					 CCSObjectAllocationInterface *ai,
					 GConfClient *client);

COMPIZCONFIG_END_DECLS

#endif
