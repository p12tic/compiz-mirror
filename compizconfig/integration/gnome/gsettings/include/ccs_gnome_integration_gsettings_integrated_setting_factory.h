#ifndef _CCS_GNOME_GCONF_INTEGRATED_SETTING_FACTORY_H
#define _CCS_GNOME_GCONF_INTEGRATED_SETTING_FACTORY_H

#include <ccs-defs.h>
#include <ccs-object.h>

COMPIZCONFIG_BEGIN_DECLS

typedef struct _CCSIntegratedSettingFactory CCSIntegratedSettingFactory;
typedef struct _CCSGNOMEValueChangeData CCSGNOMEValueChangeData;
typedef struct _CCSGSettingsWrapper CCSGSettingsWrapper;
typedef struct _GSettings	    GSettings;

typedef void (*CCSGNOMEIntegrationGSettingsChangedCallback) (GSettings *, gchar *, gpointer);

typedef struct _CCSGNOMEIntegrationGSettingsWrapperFactory CCSGNOMEIntegrationGSettingsWrapperFactory;
typedef struct _CCSGNOMEIntegrationGSettingsWrapperFactoryInterface CCSGNOMEIntegrationGSettingsWrapperFactoryInterface;

typedef CCSGSettingsWrapper * (*CCSGNOMEIntegrationGSettingsWrapperFactoryNewGSettingsWrapper) (CCSGNOMEIntegrationGSettingsWrapperFactory *,
												const gchar				   *schema,
												CCSGNOMEIntegrationGSettingsChangedCallback callback,
												CCSGNOMEValueChangeData			  *data,
												CCSObjectAllocationInterface		   *ai);

struct _CCSGNOMEIntegrationGSettingsWrapperFactoryInterface
{
    CCSGNOMEIntegrationGSettingsWrapperFactoryNewGSettingsWrapper newGSettingsWrapper;
};

struct _CCSGNOMEIntegrationGSettingsWrapperFactory
{
    CCSObject object;
};

unsigned int ccsCCSGNOMEIntegrationGSettingsWrapperFactoryInterfaceGetType ();

CCSGSettingsWrapper *
ccsGNOMEIntegrationGSettingsWrapperFactoryNewGSettingsWrapper (CCSGNOMEIntegrationGSettingsWrapperFactory *factory,
							       const gchar				  *schemaName,
							       CCSGNOMEIntegrationGSettingsChangedCallback callback,
							       CCSGNOMEValueChangeData			  *data,
							       CCSObjectAllocationInterface		  *ai);

CCSGNOMEIntegrationGSettingsWrapperFactory *
ccsGNOMEIntegrationGSettingsWrapperDefaultImplNew (CCSObjectAllocationInterface *ai);

CCSIntegratedSettingFactory *
ccsGSettingsIntegratedSettingFactoryNew (CCSGNOMEIntegrationGSettingsWrapperFactory	   *wrapperFactory,
					 CCSGNOMEValueChangeData	  *data,
					 CCSObjectAllocationInterface *ai);

COMPIZCONFIG_END_DECLS

#endif
