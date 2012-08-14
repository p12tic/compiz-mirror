/**
 *
 * compizconfig gnome integration backend
 *
 * gnome-integration.c
 *
 * Copyright (c) 2011 Canonical Ltd
 *
 * Based on the original compizconfig-backend-gconf
 *
 * Copyright (c) 2007 Danny Baumann <maniac@opencompositing.org>
 *
 * Parts of this code are taken from libberylsettings 
 * gconf backend, written by:
 *
 * Copyright (c) 2006 Robert Carr <racarr@opencompositing.org>
 * Copyright (c) 2007 Dennis Kasprzyk <onestone@opencompositing.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authored By:
 *	Sam Spilsbury <sam.spilsbury@canonical.com>
 *
 **/
#ifdef USE_GCONF

#include <stdlib.h>
#include <string.h>
#include <ccs.h>
#include <ccs-backend.h>
#include <ccs-object.h>
#include <gio/gio.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>
#include "ccs_gnome_integration.h"
#include "ccs_gnome_integration_constants.h"

typedef struct _CCSGNOMEIntegrationBackendPrivate CCGNOMEIntegrationBackendPrivate;

struct _CCSGNOMEIntegrationBackendPrivate
{
    CCSBackend *backend;
    CCSContext *context;
    CCSIntegratedSettingFactory  *factory;
    CCSIntegratedSettingsStorage *storage;
};

INTERFACE_TYPE (CCSGNOMEIntegratedSettingInterface);

SpecialOptionType
ccsGNOMEIntegratedSettingGetSpecialOptionType (CCSGNOMEIntegratedSetting *setting)
{
    return (*(GET_INTERFACE (CCSGNOMEIntegratedSettingInterface, setting))->getSpecialOptionType) (setting);
}

const char *
ccsGNOMEIntegratedSettingGetGNOMEName (CCSGNOMEIntegratedSetting *setting)
{
    return (*(GET_INTERFACE (CCSGNOMEIntegratedSettingInterface, setting))->getGNOMEName) (setting);
}

/* CCSGNOMEIntegratedSettingDefaultImpl implementation */

typedef struct _CCSGNOMEIntegratedSettingDefaultImplPrivate CCSGNOMEIntegratedSettingDefaultImplPrivate;

struct _CCSGNOMEIntegratedSettingDefaultImplPrivate
{
    SpecialOptionType type;
    const char	      *gnomeName;
    CCSIntegratedSetting *sharedIntegratedSetting;
};

SpecialOptionType
ccsGNOMEIntegratedSettingGetSpecialOptionDefault (CCSGNOMEIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return priv->type;
}

const char *
ccsGNOMEIntegratedSettingGetGNOMENameDefault (CCSGNOMEIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return priv->gnomeName;
}

CCSSettingValue *
ccsGNOMEIntegratedSettingReadValue (CCSIntegratedSetting *setting, CCSSettingType type)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingReadValue (priv->sharedIntegratedSetting, type);
}

void
ccsGNOMEIntegratedSettingWriteValue (CCSIntegratedSetting *setting, CCSSettingValue *value, CCSSettingType type)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    ccsIntegratedSettingWriteValue (priv->sharedIntegratedSetting, value, type);
}

const char *
ccsGNOMEIntegratedSettingPluginName (CCSIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingPluginName (priv->sharedIntegratedSetting);
}

const char *
ccsGNOMEIntegratedSettingSettingName (CCSIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingSettingName (priv->sharedIntegratedSetting);
}

CCSSettingType
ccsGNOMEIntegratedSettingGetType (CCSIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingGetType (priv->sharedIntegratedSetting);
}

void
ccsGNOMEIntegratedSettingFree (CCSIntegratedSetting *setting)
{
    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (CCSGNOMEIntegratedSettingDefaultImplPrivate *) ccsObjectGetPrivate (setting);

    ccsIntegratedSettingUnref (priv->sharedIntegratedSetting);
    ccsObjectFinalize (setting);

    (*setting->object.object_allocation->free_) (setting->object.object_allocation->allocator, setting);
}

CCSGNOMEIntegratedSettingInterface ccsGNOMEIntegratedSettingDefaultImplInterface =
{
    ccsGNOMEIntegratedSettingGetSpecialOptionDefault,
    ccsGNOMEIntegratedSettingGetGNOMENameDefault
};

const CCSIntegratedSettingInterface ccsGNOMEIntegratedSettingInterface =
{
    ccsGNOMEIntegratedSettingReadValue,
    ccsGNOMEIntegratedSettingWriteValue,
    ccsGNOMEIntegratedSettingPluginName,
    ccsGNOMEIntegratedSettingSettingName,
    ccsGNOMEIntegratedSettingGetType,
    ccsGNOMEIntegratedSettingFree
};

CCSGNOMEIntegratedSetting *
ccsGNOMEIntegratedSettingNew (CCSIntegratedSetting *base,
			      SpecialOptionType    type,
			      const char	   *gnomeName,
			      CCSObjectAllocationInterface *ai)
{
    CCSGNOMEIntegratedSetting *setting = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGNOMEIntegratedSetting));

    if (!setting)
	return NULL;

    CCSGNOMEIntegratedSettingDefaultImplPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGNOMEIntegratedSettingDefaultImplPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, setting);
	return NULL;
    }

    priv->sharedIntegratedSetting = base;
    priv->gnomeName = gnomeName;
    priv->type = type;

    ccsObjectInit (setting, ai);
    ccsObjectSetPrivate (setting, (CCSPrivate *) priv);
    ccsObjectAddInterface (setting, (const CCSInterface *) &ccsGNOMEIntegratedSettingInterface, GET_INTERFACE_TYPE (CCSIntegratedSettingInterface));
    ccsObjectAddInterface (setting, (const CCSInterface *) &ccsGNOMEIntegratedSettingDefaultImplInterface, GET_INTERFACE_TYPE (CCSGNOMEIntegratedSettingInterface));
    ccsIntegratedSettingRef ((CCSIntegratedSetting *) setting);

    return setting;
}

/* CCSGConfIntegratedSetting implementation */
typedef struct _CCSGConfIntegratedSettingPrivate CCSGConfIntegratedSettingPrivate;

struct _CCSGConfIntegratedSettingPrivate
{
    CCSGNOMEIntegratedSetting *gnomeIntegratedSetting;
    GConfClient		      *client;
    const char		      *sectionName;
};

SpecialOptionType
ccsGConfIntegratedSettingGetSpecialOptionType (CCSGNOMEIntegratedSetting *setting)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsGNOMEIntegratedSettingGetSpecialOptionType (priv->gnomeIntegratedSetting);
}

const char *
ccsGConfIntegratedSettingGetGNOMEName (CCSGNOMEIntegratedSetting *setting)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsGNOMEIntegratedSettingGetGNOMEName (priv->gnomeIntegratedSetting);
}

CCSSettingValue *
ccsGConfIntegratedSettingReadValue (CCSIntegratedSetting *setting, CCSSettingType type)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);
    CCSSettingValue		     *v = calloc (1, sizeof (CCSSettingValue));
    const char			     *gnomeKeyName = ccsGNOMEIntegratedSettingGetGNOMEName ((CCSGNOMEIntegratedSetting *) setting);
    char			     *gnomeKeyPath = g_strconcat (priv->sectionName, gnomeKeyName, NULL);

    v->isListChild = FALSE;
    v->parent = NULL;
    v->refCount = 1;

    GConfValue *gconfValue;
    GError     *err = NULL;

    gconfValue = gconf_client_get (priv->client,
				   gnomeKeyPath,
				   &err);

    if (!gconfValue)
    {
	asm ("int $3");
	ccsError ("NULL encountered while reading GConf setting");
	free (gnomeKeyPath);
	return v;
    }

    if (err)
    {
	ccsError ("%s", err->message);
	g_error_free (err);
	free (gnomeKeyPath);
	return v;
    }

    switch (type)
    {
	case TypeInt:
	    if (gconfValue->type != GCONF_VALUE_INT)
	    {
		ccsError ("Expected integer value");
		break;
	    }

	    v->value.asInt = gconf_value_get_int (gconfValue);
	    break;
	case TypeBool:
	    if (gconfValue->type != GCONF_VALUE_BOOL)
	    {
		ccsError ("Expected boolean value");
		break;
	    }

	    v->value.asBool = gconf_value_get_bool (gconfValue) ? TRUE : FALSE;
	    break;
	case TypeString:
	    if (gconfValue->type != GCONF_VALUE_STRING)
	    {
		ccsError ("Expected string value");
		break;
	    }

	    const char *str = gconf_value_get_string (gconfValue);

	    v->value.asString = strdup (str ? str : "");
	    break;
	default:
	    g_assert_not_reached ();
    }

    gconf_value_free (gconfValue);
    free (gnomeKeyPath);

    return v;
}

void
ccsGConfIntegratedSettingWriteValue (CCSIntegratedSetting *setting, CCSSettingValue *v, CCSSettingType type)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);
    const char			     *gnomeKeyName = ccsGNOMEIntegratedSettingGetGNOMEName ((CCSGNOMEIntegratedSetting *) setting);
    char			     *gnomeKeyPath = g_strconcat (priv->sectionName, gnomeKeyName, NULL);
    GError			     *err = NULL;

    switch (type)
    {
	case TypeInt:
	    {
		int currentValue = gconf_client_get_int (priv->client, gnomeKeyPath, &err);

		if (!err && (currentValue != v->value.asInt))
		    gconf_client_set_int(priv->client, gnomeKeyPath,
					 v->value.asInt, NULL);
	    }
	    break;
	case TypeBool:
	    {
		Bool     newValue = v->value.asBool;
		gboolean currentValue;

		currentValue = gconf_client_get_bool (priv->client, gnomeKeyPath, &err);

		if (!err && ((currentValue && !newValue) ||
			     (!currentValue && newValue)))
		    gconf_client_set_bool (priv->client, gnomeKeyPath,
					   newValue, NULL);
	    }
	    break;
	case TypeString:
	    {
		char  *newValue = v->value.asString;
		gchar *currentValue;

		currentValue = gconf_client_get_string (priv->client, gnomeKeyPath, &err);

		if (!err && currentValue)
		{
		    if (strcmp (currentValue, newValue) != 0)
			gconf_client_set_string (priv->client, gnomeKeyPath,
						 newValue, NULL);
		    g_free (currentValue);
		}
	    }
	    break;
	default:
	    g_assert_not_reached ();
	    break;
    }

    if (err)
    {
	ccsError ("%s", err->message);
	g_error_free (err);
    }

    free (gnomeKeyPath);
}

const char *
ccsGConfIntegratedSettingPluginName (CCSIntegratedSetting *setting)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingPluginName ((CCSIntegratedSetting *) priv->gnomeIntegratedSetting);
}

const char *
ccsGConfIntegratedSettingSettingName (CCSIntegratedSetting *setting)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingSettingName ((CCSIntegratedSetting *) priv->gnomeIntegratedSetting);
}

CCSSettingType
ccsGConfIntegratedSettingGetType (CCSIntegratedSetting *setting)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    return ccsIntegratedSettingGetType ((CCSIntegratedSetting *) priv->gnomeIntegratedSetting);
}

void
ccsGConfIntegratedSettingFree (CCSIntegratedSetting *setting)
{
    CCSGConfIntegratedSettingPrivate *priv = (CCSGConfIntegratedSettingPrivate *) ccsObjectGetPrivate (setting);

    if (priv->client)
	g_object_unref (priv->client);

    ccsIntegratedSettingUnref ((CCSIntegratedSetting *) priv->gnomeIntegratedSetting);
    ccsObjectFinalize (setting);

    (*setting->object.object_allocation->free_) (setting->object.object_allocation->allocator, setting);
}

const CCSGNOMEIntegratedSettingInterface ccsGConfGNOMEIntegratedSettingInterface =
{
    ccsGConfIntegratedSettingGetSpecialOptionType,
    ccsGConfIntegratedSettingGetGNOMEName
};

const CCSIntegratedSettingInterface ccsGConfIntegratedSettingInterface =
{
    ccsGConfIntegratedSettingReadValue,
    ccsGConfIntegratedSettingWriteValue,
    ccsGConfIntegratedSettingPluginName,
    ccsGConfIntegratedSettingSettingName,
    ccsGConfIntegratedSettingGetType,
    ccsGConfIntegratedSettingFree
};

CCSIntegratedSetting *
ccsGConfIntegratedSettingNew (CCSGNOMEIntegratedSetting *base,
			      GConfClient		*client,
			      const char		*section,
			      CCSObjectAllocationInterface *ai)
{
    CCSIntegratedSetting *setting = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSIntegratedSetting));

    if (!setting)
	return NULL;

    CCSGConfIntegratedSettingPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGConfIntegratedSettingPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, priv);
	return NULL;
    }

    priv->gnomeIntegratedSetting = base;
    priv->client = (GConfClient *) g_object_ref (client);
    priv->sectionName = section;

    ccsObjectInit (setting, ai);
    ccsObjectSetPrivate (setting, (CCSPrivate *) priv);
    ccsObjectAddInterface (setting, (const CCSInterface *) &ccsGConfIntegratedSettingInterface, GET_INTERFACE_TYPE (CCSIntegratedSettingInterface));
    ccsObjectAddInterface (setting, (const CCSInterface *) &ccsGConfGNOMEIntegratedSettingInterface, GET_INTERFACE_TYPE (CCSGNOMEIntegratedSettingInterface));
    ccsIntegratedSettingRef (setting);

    return setting;
}

typedef struct _CCSGConfIntegratedSettingFactoryPrivate CCSGConfIntegratedSettingFactoryPrivate;

struct _CCSGConfIntegratedSettingFactoryPrivate
{
    GConfClient *client;
    guint       gnomeGConfNotifyIds[NUM_WATCHED_DIRS];
    GHashTable  *pluginsToSettingsSectionsHashTable;
    GHashTable  *pluginsToSettingsSpecialTypesHashTable;
    GHashTable  *pluginsToSettingNameGNOMENameHashTable;
};

static CCSIntegratedSetting *
createNewGConfIntegratedSetting (GConfClient *client,
				 const char  *sectionName,
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

    CCSIntegratedSetting *gconfIntegratedSetting = ccsGConfIntegratedSettingNew (gnomeIntegratedSetting, client, sectionName, ai);

    if (!gconfIntegratedSetting)
    {
	ccsIntegratedSettingUnref ((CCSIntegratedSetting *) gnomeIntegratedSetting);
	return NULL;
    }

    return gconfIntegratedSetting;
}

static Bool
ccsGNOMEIntegrationFindSettingsMatchingPredicate (CCSIntegratedSetting *setting,
						  void		       *userData)
{
    const char *findGnomeName = (const char *) userData;
    const char *gnomeNameOfSetting = ccsGNOMEIntegratedSettingGetGNOMEName ((CCSGNOMEIntegratedSetting *) setting);

    if (strcmp (findGnomeName, gnomeNameOfSetting) == 0)
	return TRUE;

    return FALSE;
}


static void
gnomeGConfValueChanged (GConfClient *client,
			guint       cnxn_id,
			GConfEntry  *entry,
			gpointer    user_data)
{
    CCSIntegration *integration = (CCSIntegration *)user_data;
    CCGNOMEIntegrationBackendPrivate *priv = (CCGNOMEIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);
    const gchar *keyName = (char*) gconf_entry_get_key (entry);
    gchar       *baseName = g_path_get_basename (keyName);

    /* We don't care if integration is not enabled */
    if (!ccsGetIntegrationEnabled (priv->context))
	return;

    CCSIntegratedSettingList settingList = ccsIntegratedSettingsStorageFindMatchingSettingsByPredicate (priv->storage,
													ccsGNOMEIntegrationFindSettingsMatchingPredicate,
													baseName);

    ccsIntegrationUpdateIntegratedSettings (integration,
					    priv->context,
					    settingList);

    g_free (baseName);
}

static void
finiGConfClient (GConfClient    *client,
		 guint		*gnomeGConfNotifyIds)
{
    int i;

    gconf_client_clear_cache (client);

    for (i = 0; i < NUM_WATCHED_DIRS; i++)
    {
	if (gnomeGConfNotifyIds[i])
	{
	    gconf_client_notify_remove (client, gnomeGConfNotifyIds[0]);
	    gnomeGConfNotifyIds[i] = 0;
	}
	gconf_client_remove_dir (client, watchedGConfGnomeDirectories[i], NULL);
    }
    gconf_client_suggest_sync (client, NULL);

    g_object_unref (client);
}

static void
registerGConfClient (GConfClient    *client,
		     guint	    *gnomeGConfNotifyIds,
		     CCSIntegration *integration)
{
    int i;

    for (i = 0; i < NUM_WATCHED_DIRS; i++)
    {
	gnomeGConfNotifyIds[i] = gconf_client_notify_add (client,
							  watchedGConfGnomeDirectories[i],
							  gnomeGConfValueChanged, (gpointer) integration,
							  NULL, NULL);
	gconf_client_add_dir (client, watchedGConfGnomeDirectories[i],
			      GCONF_CLIENT_PRELOAD_NONE, NULL);
    }
}

static void
initGConfClient (CCSIntegratedSettingFactory *factory)
{
    CCSGConfIntegratedSettingFactoryPrivate *priv = (CCSGConfIntegratedSettingFactoryPrivate *) ccsObjectGetPrivate (factory);
    priv->client = gconf_client_get_default ();
}

CCSIntegratedSetting *
ccsGConfIntegratedSettingFactoryCreateIntegratedSettingForCCSSettingNameAndType (CCSIntegratedSettingFactory *factory,
										 CCSIntegration		     *integration,
										 const char		     *pluginName,
										 const char		     *settingName,
										 CCSSettingType		     type)
{
    CCSGConfIntegratedSettingFactoryPrivate *priv = (CCSGConfIntegratedSettingFactoryPrivate *) ccsObjectGetPrivate (factory);
    GHashTable                              *settingsSectionsHashTable = g_hash_table_lookup (priv->pluginsToSettingsSectionsHashTable, pluginName);
    GHashTable                              *settingsSpecialTypesHashTable = g_hash_table_lookup (priv->pluginsToSettingsSpecialTypesHashTable, pluginName);
    GHashTable				    *settingsSettingNameGNOMENameHashTable = g_hash_table_lookup (priv->pluginsToSettingNameGNOMENameHashTable, pluginName);

    if (!priv->client)
	initGConfClient (factory);

    if (!priv->gnomeGConfNotifyIds[0])
	registerGConfClient (priv->client, priv->gnomeGConfNotifyIds, integration);

    if (settingsSectionsHashTable &&
	settingsSpecialTypesHashTable &&
	settingsSettingNameGNOMENameHashTable)
    {
	const gchar *sectionName = g_hash_table_lookup (settingsSectionsHashTable, settingName);
	SpecialOptionType specialType = (SpecialOptionType) GPOINTER_TO_INT (g_hash_table_lookup (settingsSpecialTypesHashTable, settingName));
	const gchar *integratedName = g_hash_table_lookup (settingsSettingNameGNOMENameHashTable, settingName);

	return createNewGConfIntegratedSetting (priv->client,
						sectionName,
						integratedName,
						pluginName,
						settingName,
						type,
						specialType,
						factory->object.object_allocation);
    }


    return NULL;
}

const CCSIntegratedSettingFactoryInterface ccsGConfIntegratedSettingFactoryInterface =
{
    ccsGConfIntegratedSettingFactoryCreateIntegratedSettingForCCSSettingNameAndType
};

CCSIntegratedSettingFactory *
ccsGConfIntegratedSettingFactoryNew (GConfClient		  *client,
				     CCSObjectAllocationInterface *ai)
{
    CCSIntegratedSettingFactory *factory = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSIntegratedSettingFactory));

    if (!factory)
	return NULL;

    CCSGConfIntegratedSettingFactoryPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSGConfIntegratedSettingFactoryPrivate));

    if (!priv)
    {
	(*ai->free_) (ai->allocator, factory);
	return NULL;
    }

    priv->client = client ? (GConfClient *) g_object_ref (client) : NULL;
    priv->pluginsToSettingsSectionsHashTable = ccsGNOMEIntegrationPopulateCategoriesHashTables ();
    priv->pluginsToSettingsSpecialTypesHashTable = ccsGNOMEIntegrationPopulateSpecialTypesHashTables ();
    priv->pluginsToSettingNameGNOMENameHashTable = ccsGNOMEIntegrationPopulateSettingNameToGNOMENameHashTables ();

    ccsObjectInit (factory, ai);
    ccsObjectSetPrivate (factory, (CCSPrivate *) priv);
    ccsObjectAddInterface (factory, (const CCSInterface *) &ccsGConfIntegratedSettingFactoryInterface, GET_INTERFACE_TYPE (CCSIntegratedSettingFactoryInterface));

    return factory;
}

void
ccsGConfIntegratedSettingFactoryFree (CCSIntegratedSettingFactory *factory)
{
    CCSGConfIntegratedSettingFactoryPrivate *priv = (CCSGConfIntegratedSettingFactoryPrivate *) ccsObjectGetPrivate (factory);

    finiGConfClient (priv->client, priv->gnomeGConfNotifyIds);

    if (priv->pluginsToSettingsSectionsHashTable)
	g_hash_table_unref (priv->pluginsToSettingsSectionsHashTable);

    if (priv->pluginsToSettingsSpecialTypesHashTable)
	g_hash_table_unref (priv->pluginsToSettingsSpecialTypesHashTable);

    if (priv->pluginsToSettingNameGNOMENameHashTable)
	g_hash_table_unref (priv->pluginsToSettingNameGNOMENameHashTable);

    ccsObjectFinalize (factory);
    (*factory->object.object_allocation->free_) (factory->object.object_allocation->allocator, factory);
}

static CCSSetting *
findDisplaySettingForPlugin (CCSContext *context,
			     const char *plugin,
			     const char *setting)
{
    CCSPlugin  *p;
    CCSSetting *s;

    p = ccsFindPlugin (context, plugin);
    if (!p)
	return NULL;

    s = ccsFindSetting (p, setting);
    if (!s)
	return NULL;

    return s;
}

static void
unregisterAllIntegratedOptions (CCSIntegration *integration)
{
    CCGNOMEIntegrationBackendPrivate *priv = (CCGNOMEIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);

    if (priv->storage)
	ccsIntegratedSettingsStorageDefaultImplFree (priv->storage);

    if (priv->factory)
	ccsGConfIntegratedSettingFactoryFree (priv->factory);

    priv->storage = NULL;
    priv->factory = NULL;
}

static void
registerAllIntegratedOptions (CCSIntegration *integration)
{
    CCGNOMEIntegrationBackendPrivate *priv = (CCGNOMEIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);

    unsigned int i = 0;
    const CCSGNOMEIntegratedSettingsList *array = ccsGNOMEIntegratedSettingsList ();

    for (; i < CCS_GNOME_INTEGRATED_SETTINGS_LIST_SIZE; i++)
    {
	CCSIntegratedSetting *setting = ccsIntegratedSettingFactoryCreateIntegratedSettingForCCSSettingNameAndType (priv->factory,
														    integration,
														    array[i].pluginName,
														    array[i].settingName,
														    TypeInt);

	ccsIntegratedSettingsStorageAddSetting (priv->storage, setting);
    }
}

static CCSIntegratedSetting *
ccsGNOMEIntegrationBackendGetIntegratedSetting (CCSIntegration *integration,
						const char		  *pluginName,
						const char		  *settingName)
{
    CCGNOMEIntegrationBackendPrivate *priv = (CCGNOMEIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);

    if (ccsIntegratedSettingsStorageEmpty (priv->storage))
	registerAllIntegratedOptions (integration);

    CCSIntegratedSettingList integratedSettings = ccsIntegratedSettingsStorageFindMatchingSettingsByPluginAndSettingName (priv->storage,
															  pluginName,
															  settingName);

    if (integratedSettings)
	return integratedSettings->data;

    return NULL;
}

static unsigned int
getGnomeMouseButtonModifier (CCSIntegratedSetting *mouseButtonModifierSetting)
{
    unsigned int modMask = 0;
    CCSSettingType type = TypeString;
    CCSSettingValue *v = ccsIntegratedSettingReadValue (mouseButtonModifierSetting, type);

    modMask = ccsStringToModifiers (v->value.asString);

    ccsFreeSettingValueWithType (v, type);

    return modMask;
}

static unsigned int
getButtonBindingForSetting (CCSContext   *context,
			    const char   *plugin,
			    const char   *setting)
{
    CCSSetting *s;

    s = findDisplaySettingForPlugin (context, plugin, setting);
    if (!s)
	return 0;

    if (ccsSettingGetType (s) != TypeButton)
	return 0;

    return ccsSettingGetValue (s)->value.asButton.button;
}

static Bool
ccsGNOMEIntegrationBackendReadOptionIntoSetting (CCSIntegration *integration,
						 CCSContext	       *context,
						 CCSSetting	       *setting,
						 CCSIntegratedSetting   *integratedSetting)
{
    Bool       ret = FALSE;
    CCSSettingValue *v = NULL;
    CCSSettingType  type = TypeNum;

    CCGNOMEIntegrationBackendPrivate *priv = (CCGNOMEIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);
    
    if (ccsIntegratedSettingsStorageEmpty (priv->storage))
	registerAllIntegratedOptions (integration);

    ret = ccsSettingIsReadableByBackend (setting);

    if (!ret)
	return FALSE;

    switch (ccsGNOMEIntegratedSettingGetSpecialOptionType ((CCSGNOMEIntegratedSetting *) integratedSetting)) {
    case OptionInt:
	{
	    type = TypeInt;
	    v = ccsIntegratedSettingReadValue (integratedSetting, type);
	    ccsSetInt (setting, v->value.asInt, TRUE);
	    ret = TRUE;
	}
	break;
    case OptionBool:
	{
	    type = TypeBool;
	    v = ccsIntegratedSettingReadValue (integratedSetting, type);
	    ccsSetBool (setting, v->value.asBool, TRUE);
	    ret = TRUE;
	}
	break;
    case OptionString:
	{
	    type = TypeString;
	    v = ccsIntegratedSettingReadValue (integratedSetting, type);
	    char *str = v->value.asString;

	    ccsSetString (setting, str, TRUE);
	    ret = TRUE;
	}
	break;
    case OptionKey:
	{
	    type = TypeString;
	    v = ccsIntegratedSettingReadValue (integratedSetting, type);

	    if (v->value.asString)
	    {
		CCSSettingKeyValue key;

		memset (&key, 0, sizeof (CCSSettingKeyValue));
		ccsGetKey (setting, &key);
		if (ccsStringToKeyBinding (v->value.asString, &key))
		{
		    ccsSetKey (setting, key, TRUE);
		    ret = TRUE;
		}
	    }
	}
	break;
    case OptionSpecial:
	{
	    const char *settingName = ccsSettingGetName (setting);
	    const char *pluginName  = ccsPluginGetName (ccsSettingGetParent (setting));

	    if (strcmp (settingName, "current_viewport") == 0)
	    {
		type = TypeBool;
		v = ccsIntegratedSettingReadValue (integratedSetting, type);

		Bool showAll = v->value.asBool;
		ccsSetBool (setting, !showAll, TRUE);
		ret = TRUE;
	    }
	    else if (strcmp (settingName, "fullscreen_visual_bell") == 0)
	    {
		type = TypeString;
		v = ccsIntegratedSettingReadValue (integratedSetting, type);

		const char *value = v->value.asString;
		if (value)
		{
		    Bool fullscreen;

		    fullscreen = strcmp (value, "fullscreen") == 0;
		    ccsSetBool (setting, fullscreen, TRUE);
		    ret = TRUE;
		}
	    }
	    else if (strcmp (settingName, "click_to_focus") == 0)
	    {
		type = TypeString;
		v = ccsIntegratedSettingReadValue (integratedSetting, type);

		const char *focusMode = v->value.asString;

		if (focusMode)
		{
		    Bool clickToFocus = (strcmp (focusMode, "click") == 0);
		    ccsSetBool (setting, clickToFocus, TRUE);
		    ret = TRUE;
		}
	    }
	    else if (((strcmp (settingName, "initiate_button") == 0) &&
		      ((strcmp (pluginName, "move") == 0) ||
		       (strcmp (pluginName, "resize") == 0))) ||
		      ((strcmp (settingName, "window_menu_button") == 0) &&
		       (strcmp (pluginName, "core") == 0)))
	    {
		gboolean              resizeWithRightButton;
		CCSSettingButtonValue button;

		memset (&button, 0, sizeof (CCSSettingButtonValue));
		ccsGetButton (setting, &button);

		CCSIntegratedSettingList integratedSettingsMBM = ccsIntegratedSettingsStorageFindMatchingSettingsByPluginAndSettingName (priv->storage,
														ccsGNOMEIntegratedPluginNames.SPECIAL,
														ccsGNOMEIntegratedSettingNames.NULL_MOUSE_BUTTON_MODIFIER.compizName);

		button.buttonModMask = getGnomeMouseButtonModifier (integratedSettingsMBM->data);

		CCSIntegratedSettingList integratedSettings = ccsIntegratedSettingsStorageFindMatchingSettingsByPluginAndSettingName (priv->storage,
														ccsGNOMEIntegratedPluginNames.SPECIAL,
														ccsGNOMEIntegratedSettingNames.NULL_RESIZE_WITH_RIGHT_BUTTON.compizName);

		type = TypeBool;
		v = ccsIntegratedSettingReadValue (integratedSettings->data, type);

		resizeWithRightButton =
		   v->value.asBool;

		if (strcmp (settingName, "window_menu_button") == 0)
		    button.button = resizeWithRightButton ? 2 : 3;
		else if (strcmp (pluginName, "resize") == 0)
		    button.button = resizeWithRightButton ? 3 : 2;
		else
		    button.button = 1;

		ccsSetButton (setting, button, TRUE);
		ret = TRUE;
	    }

	}
     	break;
    default:
	break;
    }

    if (v)
	ccsFreeSettingValueWithType (v, type);

    return ret;
}

static Bool
setGnomeMouseButtonModifier (CCSIntegratedSetting *setting,
			     unsigned int modMask)
{
    char   *modifiers;
    CCSSettingValue *v = calloc (1, sizeof (CCSSettingValue));

    v->isListChild = FALSE;
    v->parent = NULL;
    v->refCount = 1;

    modifiers = ccsModifiersToString (modMask);
    if (!modifiers)
    {
	ccsFreeSettingValueWithType (v, TypeString);
	return FALSE;
    }

    v->value.asString = modifiers;
    ccsIntegratedSettingWriteValue (setting, v, TypeString);

    ccsFreeSettingValueWithType (v, TypeString);

    return TRUE;
}

static void
setButtonBindingForSetting (CCSContext   *context,
			    const char   *plugin,
			    const char   *setting,
			    unsigned int button,
			    unsigned int buttonModMask)
{
    CCSSetting            *s;
    CCSSettingButtonValue value;

    s = findDisplaySettingForPlugin (context, plugin, setting);
    if (!s)
	return;

    if (ccsSettingGetType (s) != TypeButton)
	return;

    value = ccsSettingGetValue (s)->value.asButton;

    if ((value.button != button) || (value.buttonModMask != buttonModMask))
    {
	value.button = button;
	value.buttonModMask = buttonModMask;

	ccsSetButton (s, value, TRUE);
    }
}

static void
ccsGNOMEIntegrationBackendWriteOptionFromSetting (CCSIntegration *integration,
						  CCSContext		 *context,
						  CCSSetting		 *setting,
						  CCSIntegratedSetting   *integratedSetting)
{
    GError     *err = NULL;
    CCSSettingType type = TypeNum;

    CCGNOMEIntegrationBackendPrivate *priv = (CCGNOMEIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);

    if (ccsIntegratedSettingsStorageEmpty (priv->storage))
	registerAllIntegratedOptions (integration);

    CCSSettingValue *v = ccsSettingGetValue (setting);

    switch (ccsGNOMEIntegratedSettingGetSpecialOptionType ((CCSGNOMEIntegratedSetting *) integratedSetting))
    {
    case OptionInt:
	ccsIntegratedSettingWriteValue (integratedSetting, v, TypeInt);
	break;
    case OptionBool:
	ccsIntegratedSettingWriteValue (integratedSetting, v, TypeBool);
	break;
    case OptionString:
	ccsIntegratedSettingWriteValue (integratedSetting, v, TypeString);
	break;
    case OptionKey:
	{   
	    char  *newValue;

	    newValue = ccsKeyBindingToString (&(ccsSettingGetValue (setting)->value.asKey));
	    if (newValue)
    	    {
		if (strcmp (newValue, "Disabled") == 0)
		{
		    /* Metacity doesn't like "Disabled", it wants "disabled" */
		    newValue[0] = 'd';
		}

		type = TypeString;

		ccsIntegratedSettingWriteValue (integratedSetting, v, type);
		free (newValue);
	    }
	}
	break;
    case OptionSpecial:
	{
	    const char *settingName = ccsSettingGetName (setting);
	    const char *pluginName  = ccsPluginGetName (ccsSettingGetParent (setting));
	    CCSSettingValue *newValue = calloc (1, sizeof (CCSSettingValue));

	    newValue->isListChild = FALSE;
	    newValue->parent = NULL;
	    newValue->refCount = 1;

	    if (strcmp (settingName, "current_viewport") == 0)
	    {
		newValue->value.asBool = !v->value.asBool;
		type = TypeBool;

		ccsIntegratedSettingWriteValue (integratedSetting, newValue, type);
	    }
	    else if (strcmp (settingName, "fullscreen_visual_bell") == 0)
	    {
		const char *newValueString = v->value.asBool ? "fullscreen" : "frame_flash";
		newValue->value.asString = strdup (newValueString);
		type = TypeString;

		ccsIntegratedSettingWriteValue (integratedSetting, newValue, type);
	    }
	    else if (strcmp (settingName, "click_to_focus") == 0)
	    {
		const char *newValueString = v->value.asBool ? "click" : "sloppy";
		newValue->value.asString = strdup (newValueString);
		type = TypeString;

		ccsIntegratedSettingWriteValue (integratedSetting, v, type);
	    }
	    else if (((strcmp (settingName, "initiate_button") == 0) &&
		      ((strcmp (pluginName, "move") == 0) ||
		       (strcmp (pluginName, "resize") == 0))) ||
		      ((strcmp (settingName, "window_menu_button") == 0) &&
		       (strcmp (pluginName, "core") == 0)))
	    {
		unsigned int modMask;
		Bool         resizeWithRightButton = FALSE;

		if ((getButtonBindingForSetting (context, "resize",
						 "initiate_button") == 3) ||
		    (getButtonBindingForSetting (context, "core",
						 "window_menu_button") == 2))
		{
		     resizeWithRightButton = TRUE;
		}

		CCSIntegratedSettingList integratedSettings = ccsIntegratedSettingsStorageFindMatchingSettingsByPluginAndSettingName (priv->storage,
														ccsGNOMEIntegratedPluginNames.SPECIAL,
														ccsGNOMEIntegratedSettingNames.NULL_RESIZE_WITH_RIGHT_BUTTON.compizName);

		newValue->value.asBool = resizeWithRightButton;
		type = TypeBool;

		ccsIntegratedSettingWriteValue (integratedSettings->data, newValue, type);

		CCSIntegratedSettingList integratedSettingsMBM = ccsIntegratedSettingsStorageFindMatchingSettingsByPluginAndSettingName (priv->storage,
														ccsGNOMEIntegratedPluginNames.SPECIAL,
														ccsGNOMEIntegratedSettingNames.NULL_MOUSE_BUTTON_MODIFIER.compizName);

		modMask = ccsSettingGetValue (setting)->value.asButton.buttonModMask;
		if (setGnomeMouseButtonModifier (integratedSettingsMBM->data, modMask))
		{
		    setButtonBindingForSetting (context, "move",
						"initiate_button", 1, modMask);
		    setButtonBindingForSetting (context, "resize",
						"initiate_button", 
						resizeWithRightButton ? 3 : 2,
						modMask);
		    setButtonBindingForSetting (context, "core",
						"window_menu_button",
						resizeWithRightButton ? 2 : 3,
						modMask);
		}
	    }

	    if (newValue)
		ccsFreeSettingValueWithType (newValue, type);
	}
     	break;
    }

    if (err)
    {
	ccsError ("%s", err->message);
	g_error_free (err);
    }
}

static void
ccsGNOMEIntegrationBackendUpdateIntegratedSettings (CCSIntegration *integration,
						    CCSContext	 *context,
						    CCSIntegratedSettingList integratedSettings)
{
    CCGNOMEIntegrationBackendPrivate *priv = (CCGNOMEIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);
    Bool needInit = TRUE;

    CCSIntegratedSettingList iter = integratedSettings;

    while (iter)
    {
	CCSIntegratedSetting *integrated = iter->data;
	const char           *settingName = ccsIntegratedSettingSettingName (integrated);
	const char	     *pluginName = ccsIntegratedSettingPluginName (integrated);

	/* Special case for mouse button modifier etc */
	if ((strcmp (settingName,
		     "mouse_button_modifier") == 0) ||
	    (strcmp (settingName,
		    "resize_with_right_button") == 0))
	{
	    CCSSetting *s;

	    if (needInit)
	    {
		ccsBackendReadInit (priv->backend, priv->context);
		needInit = FALSE;
	    }

	    s = findDisplaySettingForPlugin (priv->context, "core",
					     "window_menu_button");
	    if (s)
		ccsBackendReadSetting (priv->backend, priv->context, s);

	    s = findDisplaySettingForPlugin (priv->context, "move",
					     "initiate_button");
	    if (s)
		ccsBackendReadSetting (priv->backend, priv->context, s);

	    s = findDisplaySettingForPlugin (priv->context, "resize",
					     "initiate_button");
	    if (s)
		ccsBackendReadSetting (priv->backend, priv->context, s);
	}
	else
	{
	    CCSPlugin     *plugin = NULL;
	    CCSSetting    *setting;

	    plugin = ccsFindPlugin (priv->context, pluginName);
	    if (plugin)
	    {
		setting = ccsFindSetting (plugin, settingName);

		if (setting)
		{
		    if (needInit)
		    {
			ccsBackendReadInit (priv->backend, priv->context);
			needInit = FALSE;
		    }

		    ccsBackendReadSetting (priv->backend, priv->context, setting);
		}
	    }
	}

	iter = iter->next;
    }
}

static void
ccsGNOMEIntegrationBackendFree (CCSIntegration *integration)
{
    CCGNOMEIntegrationBackendPrivate *priv = (CCGNOMEIntegrationBackendPrivate *) ccsObjectGetPrivate (integration);

    unregisterAllIntegratedOptions (integration);

    priv->backend = NULL;

    ccsObjectFinalize (integration);
    free (integration);
}

const CCSIntegrationInterface ccsGNOMEIntegrationBackendInterface =
{
    ccsGNOMEIntegrationBackendGetIntegratedSetting,
    ccsGNOMEIntegrationBackendReadOptionIntoSetting,
    ccsGNOMEIntegrationBackendWriteOptionFromSetting,
    ccsGNOMEIntegrationBackendUpdateIntegratedSettings,
    ccsGNOMEIntegrationBackendFree
};

static CCGNOMEIntegrationBackendPrivate *
addPrivate (CCSIntegration *backend,
	    CCSObjectAllocationInterface *ai)
{
    CCGNOMEIntegrationBackendPrivate *priv = (*ai->calloc_) (ai->allocator, 1, sizeof (CCGNOMEIntegrationBackendPrivate));

    if (!priv)
    {
	ccsObjectFinalize (backend);
	free (backend);
    }

    ccsObjectSetPrivate (backend, (CCSPrivate *) priv);

    return priv;
}

static CCSIntegration *
ccsGNOMEIntegrationBackendNewCommon (CCSBackend *backend,
				     CCSContext *context,
				     CCSIntegratedSettingFactory *factory,
				     CCSIntegratedSettingsStorage *storage,
				     CCSObjectAllocationInterface *ai)
{
    CCSIntegration *integration = (*ai->calloc_) (ai->allocator, 1, sizeof (CCSIntegration));

    if (!integration)
	return NULL;

    ccsObjectInit (integration, ai);

    CCGNOMEIntegrationBackendPrivate *priv = addPrivate (integration, ai);
    priv->backend = backend;
    priv->context = context;
    priv->factory = factory;
    priv->storage = storage;

    ccsObjectAddInterface (integration,
			   (const CCSInterface *) &ccsGNOMEIntegrationBackendInterface,
			   GET_INTERFACE_TYPE (CCSIntegrationInterface));

    ccsIntegrationRef (integration);

    return integration;
}

CCSIntegration *
ccsGNOMEIntegrationBackendNew (CCSBackend *backend,
			       CCSContext *context,
			       CCSIntegratedSettingFactory *factory,
			       CCSIntegratedSettingsStorage *storage,
			       CCSObjectAllocationInterface *ai)
{
    return ccsGNOMEIntegrationBackendNewCommon (backend, context, factory, storage, ai);
}


#endif
