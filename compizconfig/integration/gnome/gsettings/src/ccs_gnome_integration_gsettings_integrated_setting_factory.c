#include <stdlib.h>
#include <string.h>

#include <gio/gio.h>

#include <ccs.h>
#include <ccs-backend.h>
#include <ccs-object.h>

#include <ccs_gsettings_interface.h>
#include <ccs_gsettings_interface_wrapper.h>

#include <gsettings_util.h>

#include "ccs_gnome_integration.h"
#include "ccs_gnome_integrated_setting.h"
#include "ccs_gnome_integration_constants.h"
#include "ccs_gnome_integration_types.h"
#include "ccs_gnome_integration_gsettings_integrated_setting.h"
#include "ccs_gnome_integration_gsettings_integrated_setting_factory.h"

INTERFACE_TYPE (CCSGNOMEIntegrationGSettingsWrapperFactoryInterface);

CCSGSettingsWrapper *
ccsGNOMEIntegrationGSettingsWrapperFactoryNewGSettingsWrapper (CCSGNOMEIntegrationGSettingsWrapperFactory *factory,
							       const gchar				  *schemaName,
							       CCSGNOMEIntegrationGSettingsChangedCallback callback,
							       CCSGNOMEValueChangeData			  *data,
							       CCSObjectAllocationInterface		  *ai)
{
    return (*(GET_INTERFACE (CCSGNOMEIntegrationGSettingsWrapperFactoryInterface, factory))->newGSettingsWrapper) (factory, schemaName, callback, data, ai);
}

/* CCSGNOMEIntegrationGSettingsWrapperFactory implementation */
typedef struct _CCSGNOMEIntegrationGSettingsWrapperFactoryPrivate CCSGNOMEIntegrationGSettingsWrapperFactoryPrivate;
struct _CCSGNOMEIntegrationGSettingsWrapperFactoryPrivate
{
};

CCSGSettingsWrapper *
ccsGNOMEIntegrationGSettingsWrapperFactoryNewGSettingsWrapperDefault (CCSGNOMEIntegrationGSettingsWrapperFactory *factory,
								      const gchar				  *schemaName,
								      CCSGNOMEIntegrationGSettingsChangedCallback callback,
								      CCSGNOMEValueChangeData			  *data,
								      CCSObjectAllocationInterface		  *ai)
{
    CCSGSettingsWrapper *wrapper = ccsGSettingsWrapperNewForSchema (schemaName, ai);
    ccsGSettingsWrapperConnectToChangedSignal (wrapper, (GCallback) callback, data);

    return wrapper;
}

const CCSGNOMEIntegrationGSettingsWrapperFactoryInterface ccsGNOMEIntegrationGSettingsWrapperFactoryInterface =
{
    ccsGNOMEIntegrationGSettingsWrapperFactoryNewGSettingsWrapperDefault
};

CCSGNOMEIntegrationGSettingsWrapperFactory *
ccsGNOMEIntegrationGSettingsWrapperDefaultImplNew (CCSObjectAllocationInterface *ai)
{
    CCSGNOMEIntegrationGSettingsWrapperFactory *wrapperFactory = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGNOMEIntegrationGSettingsWrapperFactory));

    if (!wrapperFactory)
	return NULL;

    CCSGNOMEIntegrationGSettingsWrapperFactoryPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGNOMEIntegrationGSettingsWrapperFactoryPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, wrapperFactory);
	return NULL;
    }

    ccsObjectInit (wrapperFactory, ai);
    ccsObjectAddInterface (wrapperFactory, (const CCSInterface *) &ccsGNOMEIntegrationGSettingsWrapperFactoryInterface, GET_INTERFACE_TYPE (CCSGNOMEIntegrationGSettingsWrapperFactoryInterface));
    ccsObjectSetPrivate (wrapperFactory, (CCSPrivate *) priv);

    return wrapperFactory;
}

void
ccsGNOMEIntegrationGSettingsWrapperDefaultImpl (CCSGNOMEIntegrationGSettingsWrapperFactory *factory)
{
    ccsObjectFinalize (factory);
    (*factory->object.object_allocation->free_) (factory->object.object_allocation->allocator, factory);
}

typedef struct _CCSGSettingsIntegratedSettingFactoryPrivate CCSGSettingsIntegratedSettingFactoryPrivate;

struct _CCSGSettingsIntegratedSettingFactoryPrivate
{
    CCSGNOMEIntegrationGSettingsWrapperFactory *wrapperFactory;
    GHashTable  *pluginsToSettingsGSettingsWrapperQuarksHashTable;
    GHashTable  *quarksToGSettingsWrappersHashTable;
    GHashTable  *pluginsToSettingsSpecialTypesHashTable;
    GHashTable  *pluginsToSettingNameGNOMENameHashTable;
    CCSGNOMEValueChangeData *valueChangeData;
};

static void
gnomeGSettingsValueChanged (GSettings *settings,
			    gchar     *key,
			    gpointer  user_data)
{
    CCSGNOMEValueChangeData *data = (CCSGNOMEValueChangeData *) user_data;
    char *baseName = translateKeyForCCS (key);

    /* We don't care if integration is not enabled */
    if (!ccsGetIntegrationEnabled (data->context))
	return;

    CCSIntegratedSettingList settingList = ccsIntegratedSettingsStorageFindMatchingSettingsByPredicate (data->storage,
													ccsGNOMEIntegrationFindSettingsMatchingPredicate,
													baseName);

    ccsIntegrationUpdateIntegratedSettings (data->integration,
					    data->context,
					    settingList);

    g_free (baseName);

}

static CCSIntegratedSetting *
createNewGSettingsIntegratedSetting (CCSGSettingsWrapper *wrapper,
				     const char  *gnomeName,
				     const char  *pluginName,
				     const char  *settingName,
				     CCSSettingType type,
				     SpecialOptionType specialOptionType,
				     CCSObjectAllocationInterface *ai)
{
    CCSIntegratedSetting *sharedIntegratedSetting = ccsSharedIntegratedSettingNew (pluginName, settingName, type, ai);

    if (!sharedIntegratedSetting)
	return NULL;

    CCSGNOMEIntegratedSetting *gnomeIntegratedSetting = ccsGNOMEIntegratedSettingNew (sharedIntegratedSetting, specialOptionType, gnomeName, ai);

    if (!gnomeIntegratedSetting)
    {
	ccsIntegratedSettingUnref (sharedIntegratedSetting);
	return NULL;
    }

    CCSIntegratedSetting *gsettingsIntegratedSetting = ccsGSettingsIntegratedSettingNew (gnomeIntegratedSetting, wrapper, ai);

    if (!gsettingsIntegratedSetting)
    {
	ccsIntegratedSettingUnref ((CCSIntegratedSetting *) gsettingsIntegratedSetting);
	return NULL;
    }

    return gsettingsIntegratedSetting;
}

static void
ccsGSettingsWrapperUnrefWrapper (gpointer wrapper)
{
    ccsGSettingsWrapperUnref ((CCSGSettingsWrapper *) wrapper);
}

static GHashTable *
initializeGSettingsWrappers (CCSGNOMEIntegrationGSettingsWrapperFactory *factory,
			     CCSGNOMEValueChangeData			*data)
{
    const CCSGSettingsWrapperIntegratedSchemasQuarks *quarks = ccsGNOMEGSettingsWrapperQuarks ();
    GHashTable *quarksToGSettingsWrappers = g_hash_table_new_full (g_int_hash, g_int_equal, NULL, ccsGSettingsWrapperUnrefWrapper);

    g_hash_table_insert (quarksToGSettingsWrappers, GINT_TO_POINTER (quarks->ORG_COMPIZ_INTEGRATED),
			 ccsGNOMEIntegrationGSettingsWrapperFactoryNewGSettingsWrapper (factory,
											g_quark_to_string (quarks->ORG_COMPIZ_INTEGRATED),
											gnomeGSettingsValueChanged,
											data,
											factory->object.object_allocation));
    g_hash_table_insert (quarksToGSettingsWrappers, GINT_TO_POINTER (quarks->ORG_GNOME_DESKTOP_WM_KEYBINDINGS),
			 ccsGNOMEIntegrationGSettingsWrapperFactoryNewGSettingsWrapper (factory,
											g_quark_to_string (quarks->ORG_GNOME_DESKTOP_WM_KEYBINDINGS),
											gnomeGSettingsValueChanged,
											data,
											factory->object.object_allocation));
    g_hash_table_insert (quarksToGSettingsWrappers, GINT_TO_POINTER (quarks->ORG_GNOME_DESKTOP_WM_PREFERENCES),
			 ccsGNOMEIntegrationGSettingsWrapperFactoryNewGSettingsWrapper (factory,
											g_quark_to_string (quarks->ORG_GNOME_DESKTOP_WM_PREFERENCES),
											gnomeGSettingsValueChanged,
											data,
											factory->object.object_allocation));

    return quarksToGSettingsWrappers;
}

CCSIntegratedSetting *
ccsGSettingsIntegratedSettingFactoryCreateIntegratedSettingForCCSSettingNameAndType (CCSIntegratedSettingFactory *factory,
										 CCSIntegration		     *integration,
										 const char		     *pluginName,
										 const char		     *settingName,
										 CCSSettingType		     type)
{
    CCSGSettingsIntegratedSettingFactoryPrivate *priv = (CCSGSettingsIntegratedSettingFactoryPrivate *) ccsObjectGetPrivate (factory);
    GHashTable                              *settingsGSettingsWrapperQuarksHashTable = g_hash_table_lookup (priv->pluginsToSettingsGSettingsWrapperQuarksHashTable, pluginName);
    GHashTable                              *settingsSpecialTypesHashTable = g_hash_table_lookup (priv->pluginsToSettingsSpecialTypesHashTable, pluginName);
    GHashTable				    *settingsSettingNameGNOMENameHashTable = g_hash_table_lookup (priv->pluginsToSettingNameGNOMENameHashTable, pluginName);

    if (!priv->quarksToGSettingsWrappersHashTable)
	priv->quarksToGSettingsWrappersHashTable = initializeGSettingsWrappers (priv->wrapperFactory, priv->valueChangeData);

    if (settingsGSettingsWrapperQuarksHashTable &&
	settingsSpecialTypesHashTable &&
	settingsSettingNameGNOMENameHashTable)
    {
	GQuark  wrapperQuark = GPOINTER_TO_INT (g_hash_table_lookup (settingsGSettingsWrapperQuarksHashTable, settingName));
	CCSGSettingsWrapper *wrapper = g_hash_table_lookup (priv->quarksToGSettingsWrappersHashTable, GINT_TO_POINTER (wrapperQuark));
	SpecialOptionType specialType = (SpecialOptionType) GPOINTER_TO_INT (g_hash_table_lookup (settingsSpecialTypesHashTable, settingName));
	const gchar *integratedName = g_hash_table_lookup (settingsSettingNameGNOMENameHashTable, settingName);

	return createNewGSettingsIntegratedSetting (wrapper,
						integratedName,
						pluginName,
						settingName,
						type,
						specialType,
						factory->object.object_allocation);
    }


    return NULL;
}

void
ccsGSettingsIntegratedSettingFactoryFree (CCSIntegratedSettingFactory *factory)
{
    CCSGSettingsIntegratedSettingFactoryPrivate *priv = (CCSGSettingsIntegratedSettingFactoryPrivate *) ccsObjectGetPrivate (factory);

    if (priv->pluginsToSettingsGSettingsWrapperQuarksHashTable)
	g_hash_table_unref (priv->pluginsToSettingsGSettingsWrapperQuarksHashTable);

    if (priv->quarksToGSettingsWrappersHashTable)
	g_hash_table_unref (priv->quarksToGSettingsWrappersHashTable);

    if (priv->pluginsToSettingsSpecialTypesHashTable)
	g_hash_table_unref (priv->pluginsToSettingsSpecialTypesHashTable);

    if (priv->pluginsToSettingNameGNOMENameHashTable)
	g_hash_table_unref (priv->pluginsToSettingNameGNOMENameHashTable);

    ccsObjectFinalize (factory);
    (*factory->object.object_allocation->free_) (factory->object.object_allocation->allocator, factory);
}


const CCSIntegratedSettingFactoryInterface ccsGSettingsIntegratedSettingFactoryInterface =
{
    ccsGSettingsIntegratedSettingFactoryCreateIntegratedSettingForCCSSettingNameAndType,
    ccsGSettingsIntegratedSettingFactoryFree
};

CCSIntegratedSettingFactory *
ccsGSettingsIntegratedSettingFactoryNew (CCSGNOMEIntegrationGSettingsWrapperFactory	  *wrapperFactory,
					 CCSGNOMEValueChangeData			  *valueChangeData,
					 CCSObjectAllocationInterface			  *ai)
{
    CCSIntegratedSettingFactory *factory = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSIntegratedSettingFactory));

    if (!factory)
	return NULL;

    CCSGSettingsIntegratedSettingFactoryPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGSettingsIntegratedSettingFactoryPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, factory);
	return NULL;
    }

    priv->wrapperFactory = wrapperFactory;
    priv->pluginsToSettingsGSettingsWrapperQuarksHashTable = ccsGNOMEGSettingsIntegrationPopulateSettingNameToIntegratedSchemasQuarksHashTable ();
    priv->pluginsToSettingsSpecialTypesHashTable = ccsGNOMEIntegrationPopulateSpecialTypesHashTables ();
    priv->pluginsToSettingNameGNOMENameHashTable = ccsGNOMEIntegrationPopulateSettingNameToGNOMENameHashTables ();
    priv->valueChangeData = valueChangeData;

    ccsObjectInit (factory, ai);
    ccsObjectSetPrivate (factory, (CCSPrivate *) priv);
    ccsObjectAddInterface (factory, (const CCSInterface *) &ccsGSettingsIntegratedSettingFactoryInterface, GET_INTERFACE_TYPE (CCSIntegratedSettingFactoryInterface));

    return factory;
}

