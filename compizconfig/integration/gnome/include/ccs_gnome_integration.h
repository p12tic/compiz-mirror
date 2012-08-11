#ifndef _CCS_COMPIZCONFIG_GNOME_GCONF_INTEGRATION
#define _CCS_COMPIZCONFIG_GNOME_GCONF_INTEGRATION

COMPIZCONFIG_BEGIN_DECLS

typedef struct _CCSBackend CCSBackend;
typedef struct _CCSContext CCSContext;
typedef struct _CCSObjectAllocationInterface CCSObjectAllocationInterface;
typedef struct _CCSIntegration CCSIntegration;
typedef struct _CCSIntegratedSetting CCSIntegratedSetting;
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

typedef struct _CCSGNOMEIntegratedSetting CCSGNOMEIntegratedSetting;
typedef struct _CCSGNOMEIntegratedSettingInterface CCSGNOMEIntegratedSettingInterface;

typedef enum {
    OptionInt,
    OptionBool,
    OptionKey,
    OptionString,
    OptionSpecial,
} SpecialOptionType;

typedef SpecialOptionType (*CCSGNOMEIntegratedSettingGetSpecialOptionType) (CCSGNOMEIntegratedSetting *);

struct _CCSGNOMEIntegratedSettingInterface
{
    CCSGNOMEIntegratedSettingGetSpecialOptionType getSpecialOptionType;
};

struct _CCSGNOMEIntegratedSetting
{
    CCSObject object;
};

unsigned int ccsCCSGNOMEIntegratedSettingInterfaceGetType ();

SpecialOptionType
ccsGNOMEIntegratedSettingGetSpecialOptionType (CCSGNOMEIntegratedSetting *);

CCSGNOMEIntegratedSetting *
ccsGNOMEIntegratedSettingNew (CCSIntegratedSetting *base,
			      SpecialOptionType    type,
			      CCSObjectAllocationInterface *ai);

CCSIntegratedSetting *
ccsGConfIntegratedSettingNew (CCSGNOMEIntegratedSetting *base,
			      GConfClient		*client,
			      const char		*section,
			      CCSObjectAllocationInterface *ai);

COMPIZCONFIG_END_DECLS

#endif
