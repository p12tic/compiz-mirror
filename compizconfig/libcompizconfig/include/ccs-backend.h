/*
 * Compiz configuration system library
 *
 * Copyright (C) 2007  Dennis Kasprzyk <onestone@opencompositing.org>
 * Copyright (C) 2007  Danny Baumann <maniac@opencompositing.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef CCS_BACKEND_H
#define CCS_BACKEND_H

#include <ccs-object.h>
#include <ccs-string.h>
#include <ccs-list.h>
#include <ccs-setting-types.h>

COMPIZCONFIG_BEGIN_DECLS

typedef struct _CCSSetting	  CCSSetting;
typedef struct _CCSPlugin         CCSPlugin;
typedef struct _CCSContext	  CCSContext;
typedef struct _CCSBackend	  CCSBackend;
typedef struct _CCSBackendInfo    CCSBackendInfo;
typedef struct _CCSBackendPrivate CCSBackendPrivate;
typedef struct _CCSBackendInterface  CCSBackendInterface;

typedef struct _CCSSettingValue CCSSettingValue;
typedef enum _CCSSettingType CCSSettingType;

typedef struct _CCSIntegratedSetting CCSIntegratedSetting;
typedef struct _CCSIntegratedSettingInterface CCSIntegratedSettingInterface;

typedef CCSSettingValue (*CCSIntegratedSettingReadValue) (CCSIntegratedSetting *, CCSSettingType);
typedef void (*CCSIntegratedSettingWriteValue) (CCSIntegratedSetting *, CCSSettingValue, CCSSettingType);
typedef const char * (*CCSIntegratedSettingPluginName) (CCSIntegratedSetting *);
typedef const char * (*CCSIntegratedSettingSettingName) (CCSIntegratedSetting *);
typedef CCSSettingType (*CCSIntegratedSettingGetType) (CCSIntegratedSetting *);
typedef void (*CCSIntegratedSettingFree) (CCSIntegratedSetting *);

struct _CCSIntegratedSettingInterface
{
    CCSIntegratedSettingReadValue readValue;
    CCSIntegratedSettingWriteValue writeValue;
    CCSIntegratedSettingPluginName pluginName;
    CCSIntegratedSettingSettingName settingName;
    CCSIntegratedSettingGetType       getType;
    CCSIntegratedSettingFree       free;
};

struct _CCSIntegratedSetting
{
    CCSObject object;
};

CCSSettingValue ccsIntegratedSettingReadValue(CCSIntegratedSetting *, CCSSettingType);
void ccsIntegratedSettingWriteValue (CCSIntegratedSetting *, CCSSettingValue, CCSSettingType);
const char * ccsIntegratedSettingPluginName (CCSIntegratedSetting *);
const char * ccsIntegratedSettingSettingName (CCSIntegratedSetting *);
CCSSettingType ccsIntegratedSettingGetType (CCSIntegratedSetting *);
void ccsFreeIntegratedSetting (CCSIntegratedSetting *);

CCSREF_HDR (IntegratedSetting, CCSIntegratedSetting);
CCSLIST_HDR (IntegratedSetting, CCSIntegratedSetting);

unsigned int ccsCCSIntegratedSettingInterfaceGetType ();

CCSIntegratedSetting *
ccsSharedIntegratedSettingNew (const char *pluginName,
			       const char *settingName,
			       CCSSettingType type,
			       CCSObjectAllocationInterface *ai);

typedef struct _CCSIntegratedSettingsStorage CCSIntegratedSettingsStorage;
typedef struct _CCSIntegratedSettingsStorageInterface CCSIntegratedSettingsStorageInterface;

typedef Bool (*CCSIntegratedSettingsStorageFindPredicate) (CCSIntegratedSetting *, void *);

typedef CCSIntegratedSettingList (*CCSIntegratedSettingsStorageFindMatchingSettingsByPluginAndSettingName) (CCSIntegratedSettingsStorage *storage,
													    const char *pluginName,
													    const char *settingName);
typedef void (*CCSIntegratedSettingsStorageAddSetting) (CCSIntegratedSettingsStorage *storage,
							CCSIntegratedSetting	     *setting);

typedef CCSIntegratedSettingList (*CCSIntegratedSettingsStorageFindMatchingSettingsByPredicate) (CCSIntegratedSettingsStorage *storage,
												 CCSIntegratedSettingsStorageFindPredicate pred,
												 void			     *data);

struct _CCSIntegratedSettingsStorageInterface
{
    CCSIntegratedSettingsStorageFindMatchingSettingsByPredicate findMatchingSettingsByPredicate;
    CCSIntegratedSettingsStorageFindMatchingSettingsByPluginAndSettingName findMatchingSettingsByPluginAndSettingName;
    CCSIntegratedSettingsStorageAddSetting	     addSetting;
};

struct _CCSIntegratedSettingsStorage
{
    CCSObject object;
};

CCSIntegratedSettingList
ccsIntegratedSettingsStorageFindMatchingSettingsByPredicate (CCSIntegratedSettingsStorage *storage,
							     CCSIntegratedSettingsStorageFindPredicate pred,
							     void			  *data);

CCSIntegratedSettingList
ccsIntegratedSettingsStorageFindMatchingSettingsByPluginAndSettingName (CCSIntegratedSettingsStorage *storage,
									const char *pluginName,
									const char *settingName);

void
ccsIntegratedSettingsStorageAddSetting (CCSIntegratedSettingsStorage *storage,
					CCSIntegratedSetting	    *setting);

unsigned int ccsCCSIntegratedSettingsStorageInterfaceGetType ();

CCSIntegratedSettingsStorage *
ccsIntegratedSettingsStorageDefaultImplNew (CCSObjectAllocationInterface *ai);

void
ccsIntegratedSettingsStorageDefaultImplFree (CCSIntegratedSettingsStorage *storage);

typedef struct _CCSIntegratedSettingFactory CCSIntegratedSettingFactory;
typedef struct _CCSIntegratedSettingFactoryInterface CCSIntegratedSettingFactoryInterface;

typedef CCSIntegratedSetting * (*CCSIntegratedSettingFactoryCreateIntegratedSettingForCCSSettingNameAndType) (CCSIntegratedSettingFactory *factory,
													      const char		  *integratedName,
													      const char		  *pluginName,
													      const char		  *settingName,
													      CCSSettingType		  type);

struct _CCSIntegratedSettingFactoryInterface
{
    CCSIntegratedSettingFactoryCreateIntegratedSettingForCCSSettingNameAndType createIntegratedSettingForCCSSettingNameAndType;
};

struct _CCSIntegratedSettingFactory
{
    CCSObject object;
};

unsigned int ccsCCSIntegratedSettingFactoryInterfaceGetType ();

CCSIntegratedSetting *
ccsIntegratedSettingFactoryCreateIntegratedSettingForCCSSettingNameAndType (CCSIntegratedSettingFactory *factory,
									    const char			*integratedName,
									    const char			*pluginName,
									    const char			*settingName,
									    CCSSettingType		type);

typedef struct _CCSIntegration CCSIntegration;
typedef struct _CCSIntegrationInterface CCSIntegrationInterface;

typedef CCSIntegratedSetting * (*CCSIntegrationGetIntegratedOptionIndex) (CCSIntegration *integration,
									  const char *pluginName,
									  const char *settingName,
									  int	  *index);
typedef Bool (*CCSIntegrationReadOptionIntoSetting) (CCSIntegration *integration,
						     CCSContext	    *context,
						     CCSSetting	    *setting,
						     CCSIntegratedSetting *integratedSetting,
						     int	    index);
typedef void (*CCSIntegrationWriteSettingIntoOption) (CCSIntegration *integration,
						      CCSContext     *context,
						      CCSSetting     *setting,
						      CCSIntegratedSetting *integratedSetting,
						      int	     index);
typedef void (*CCSFreeIntegrationBackend) (CCSIntegration *integration);

struct _CCSIntegrationInterface
{
    CCSIntegrationGetIntegratedOptionIndex getIntegratedOptionIndex;
    CCSIntegrationReadOptionIntoSetting readOptionIntoSetting;
    CCSIntegrationWriteSettingIntoOption writeSettingIntoOption;
    CCSFreeIntegrationBackend			freeIntegrationBackend;
};

struct _CCSIntegration
{
    CCSObject object;
};

CCSREF_HDR (Integration, CCSIntegration)

unsigned int ccsCCSIntegrationInterfaceGetType ();

CCSIntegratedSetting * ccsIntegrationGetIntegratedOptionIndex (CCSIntegration *integration,
							       const char *pluginName,
							       const char *settingName,
							       int	  *index);
Bool ccsIntegrationReadOptionIntoSetting (CCSIntegration *integration,
					  CCSContext		  *context,
					  CCSSetting		  *setting,
					  CCSIntegratedSetting    *integratedSetting,
					  int			  index);
void ccsIntegrationWriteSettingIntoOption (CCSIntegration *integration,
					   CCSContext		   *context,
					   CCSSetting		   *setting,
					   CCSIntegratedSetting *integratedSetting,
					   int			    index);

void ccsFreeIntegration (CCSIntegration *integration);

CCSIntegration *
ccsNullIntegrationBackendNew (CCSObjectAllocationInterface *ai);

struct _CCSBackend
{
    CCSObject        object;
};

struct _CCSBackendInfo
{
    const char *name;              /* name of the backend */
    const char *shortDesc;         /* backend's short description */
    const char *longDesc;          /* backend's long description */
    Bool integrationSupport; /* does the backend support DE integration? */
    Bool profileSupport;     /* does the backend support profiles? */
    unsigned int refCount;   /* reference count */
};

typedef CCSBackendInterface * (*BackendGetInfoProc) (void);

typedef void (*CCSBackendExecuteEventsFunc) (CCSBackend *backend, unsigned int flags);

typedef Bool (*CCSBackendInitFunc) (CCSBackend *, CCSContext * context);
typedef Bool (*CCSBackendFiniFunc) (CCSBackend *);

typedef Bool (*CCSBackendReadInitFunc) (CCSBackend *, CCSContext * context);
typedef void (*CCSBackendReadSettingFunc)
(CCSBackend *, CCSContext * context, CCSSetting * setting);
typedef void (*CCSBackendReadDoneFunc) (CCSBackend *backend, CCSContext * context);

typedef Bool (*CCSBackendWriteInitFunc) (CCSBackend *backend, CCSContext * context);
typedef void (*CCSBackendWriteSettingFunc)
(CCSBackend *, CCSContext * context, CCSSetting * setting);
typedef void (*CCSBackendWriteDoneFunc) (CCSBackend *, CCSContext * context);

typedef void (*CCSBackendUpdateFunc) (CCSBackend *, CCSContext *, CCSPlugin *, CCSSetting *);

typedef Bool (*CCSBackendGetSettingIsIntegratedFunc) (CCSBackend *, CCSSetting * setting);
typedef Bool (*CCSBackendGetSettingIsReadOnlyFunc) (CCSBackend *, CCSSetting * setting);

typedef CCSStringList (*CCSBackendGetExistingProfilesFunc) (CCSBackend *, CCSContext * context);
typedef Bool (*CCSBackendDeleteProfileFunc) (CCSBackend *, CCSContext * context, char * name);

typedef void (*CCSBackendSetIntegration) (CCSBackend *, CCSIntegration *);

typedef const CCSBackendInfo * (*CCSBackendGetInfoFunc) (CCSBackend *);

struct _CCSBackendInterface
{
    CCSBackendGetInfoFunc      backendGetInfo;

    /* something like a event loop call for the backend,
       so it can check for file changes (gconf changes in the gconf backend)
       no need for reload settings signals anymore */
    CCSBackendExecuteEventsFunc executeEvents;

    CCSBackendInitFunc	       backendInit;
    CCSBackendFiniFunc	       backendFini;

    CCSBackendReadInitFunc     readInit;
    CCSBackendReadSettingFunc  readSetting;
    CCSBackendReadDoneFunc     readDone;

    CCSBackendWriteInitFunc    writeInit;
    CCSBackendWriteSettingFunc writeSetting;
    CCSBackendWriteDoneFunc    writeDone;

    CCSBackendUpdateFunc       updateSetting;

    CCSBackendGetSettingIsIntegratedFunc     getSettingIsIntegrated;
    CCSBackendGetSettingIsReadOnlyFunc       getSettingIsReadOnly;

    CCSBackendGetExistingProfilesFunc getExistingProfiles;
    CCSBackendDeleteProfileFunc       deleteProfile;
    CCSBackendSetIntegration	      setIntegration;
};

unsigned int ccsCCSBackendInterfaceGetType ();

const CCSBackendInfo * ccsBackendGetInfo (CCSBackend *backend);
void ccsBackendExecuteEvents (CCSBackend *backend, unsigned int flags);
Bool ccsBackendInit (CCSBackend *backend, CCSContext *context);
Bool ccsBackendFini (CCSBackend *backend);
Bool ccsBackendReadInit (CCSBackend *backend, CCSContext *context);
void ccsBackendReadSetting (CCSBackend *backend, CCSContext *context, CCSSetting *setting);
void ccsBackendReadDone (CCSBackend *backend, CCSContext *context);
Bool ccsBackendWriteInit (CCSBackend *backend, CCSContext *context);
void ccsBackendWriteSetting (CCSBackend *backend, CCSContext *context, CCSSetting *setting);
void ccsBackendWriteDone (CCSBackend *backend, CCSContext *context);
void ccsBackendUpdateSetting (CCSBackend *backend, CCSContext *context, CCSPlugin *plugin, CCSSetting *setting);
Bool ccsBackendGetSettingIsIntegrated (CCSBackend *backend, CCSSetting *setting);
Bool ccsBackendGetSettingIsReadOnly (CCSBackend *backend, CCSSetting *setting);
CCSStringList ccsBackendGetExistingProfiles (CCSBackend *backend, CCSContext *context);
Bool ccsBackendDeleteProfile (CCSBackend *backend, CCSContext *context, char *name);
void ccsBackendSetIntegration (CCSBackend *backend, CCSIntegration *integration);
void ccsFreeBackend (CCSBackend *backend);

typedef struct _CCSDynamicBackend	  CCSDynamicBackend;
typedef struct _CCSDynamicBackendPrivate CCSDynamicBackendPrivate;
typedef struct _CCSDynamicBackendInterface  CCSDynamicBackendInterface;
typedef struct _CCSInterfaceTable         CCSInterfaceTable;

struct _CCSDynamicBackend
{
    CCSObject object;
};

typedef const char * (*CCSDynamicBackendGetBackendName) (CCSDynamicBackend *);
typedef Bool (*CCSDynamicBackendSupportsRead) (CCSDynamicBackend *);
typedef Bool (*CCSDynamicBackendSupportsWrite) (CCSDynamicBackend *);
typedef Bool (*CCSDynamicBackendSupportsProfiles) (CCSDynamicBackend *);
typedef Bool (*CCSDynamicBackendSupportsIntegration) (CCSDynamicBackend *);
typedef CCSBackend * (*CCSDynamicBackendGetRawBackend) (CCSDynamicBackend *);

struct _CCSDynamicBackendInterface
{
    CCSDynamicBackendGetBackendName getBackendName;
    CCSDynamicBackendSupportsRead supportsRead;
    CCSDynamicBackendSupportsWrite supportsWrite;
    CCSDynamicBackendSupportsProfiles supportsProfiles;
    CCSDynamicBackendSupportsIntegration supportsIntegration;
    CCSDynamicBackendGetRawBackend getRawBackend;
};

const char * ccsDynamicBackendGetBackendName (CCSDynamicBackend *);
Bool ccsDynamicBackendSupportsRead (CCSDynamicBackend *);
Bool ccsDynamicBackendSupportsWrite (CCSDynamicBackend *);
Bool ccsDynamicBackendSupportsProfiles (CCSDynamicBackend *);
Bool ccsDynamicBackendSupportsIntegration (CCSDynamicBackend *);
CCSBackend * ccsDynamicBackendGetRawBackend (CCSDynamicBackend *);

unsigned int ccsCCSDynamicBackendInterfaceGetType ();

void ccsFreeDynamicBackend (CCSDynamicBackend *);

/* Backend opener method */
CCSBackend * ccsOpenBackend (const CCSInterfaceTable *, CCSContext *context, const char *name);

/* Constructor method */
CCSBackend *
ccsBackendNewWithDynamicInterface (CCSContext *context, const CCSBackendInterface *interface);

CCSBackendInterface* getBackendInfo (void);

COMPIZCONFIG_END_DECLS

#endif
