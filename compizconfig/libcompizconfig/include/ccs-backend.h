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

COMPIZCONFIG_BEGIN_DECLS

typedef struct _CCSBackend	  CCSBackend;
typedef struct _CCSBackendPrivate CCSBackendPrivate;
typedef struct _CCSBackendInterface  CCSBackendInterface;

typedef struct _CCSContext CCSContext;
typedef struct _CCSPlugin  CCSPlugin;
typedef struct _CCSSetting CCSSetting;

struct _CCSBackend
{
    CCSObject        object;
};

typedef CCSBackendInterface * (*BackendGetInfoProc) (void);

typedef void (*CCSBackendExecuteEventsFunc) (CCSBackend *backend, unsigned int flags);

typedef Bool (*CCSBackendInitFunc) (CCSBackend *, CCSContext * context);
typedef Bool (*CCSBackendFiniFunc) (CCSBackend *, CCSContext * context);

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

typedef char * (*CCSBackendGetNameFunc) (CCSBackend *);
typedef char * (*CCSBackendGetShortDescFunc) (CCSBackend *);
typedef char * (*CCSBackendGetLongDescFunc) (CCSBackend *);
typedef Bool (*CCSBackendHasIntegrationSupportFunc) (CCSBackend *);
typedef Bool (*CCSBackendHasProfileSupportFunc) (CCSBackend *);

struct _CCSBackendInterface
{
    CCSBackendGetNameFunc getName;
    CCSBackendGetShortDescFunc getShortDesc;
    CCSBackendGetLongDescFunc getLongDesc;
    CCSBackendHasIntegrationSupportFunc hasIntegrationSupport;
    CCSBackendHasProfileSupportFunc hasProfileSupport;

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
};

unsigned int ccsCCSBackendInterfaceGetType ();

char * ccsBackendGetName (CCSBackend *backend);
char * ccsBackendGetShortDesc (CCSBackend *backend);
char * ccsBackendGetLongDesc (CCSBackend *backend);
Bool ccsBackendHasIntegrationSupport (CCSBackend *backend);
Bool ccsBackendHasProfileSupport (CCSBackend *backend);

void ccsBackendExecuteEvents (CCSBackend *backend, unsigned int flags);
Bool ccsBackendInit (CCSBackend *backend, CCSContext *context);
Bool ccsBackendFini (CCSBackend *backend, CCSContext *context);
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
void ccsFreeBackend (CCSBackend *backend);

typedef struct _CCSBackendWithCapabilities	  CCSBackendWithCapabilities;
typedef struct _CCSBackendWithCapabilitiesPrivate CCSBackendWithCapabilitiesPrivate;
typedef struct _CCSBackendCapabilitiesInterface  CCSBackendCapabilitiesInterface;

struct _CCSBackendWithCapabilities
{
    CCSObject object;
};

typedef Bool (*CCSBackendCapabilitiesSupportsRead) (CCSBackendWithCapabilities *);
typedef Bool (*CCSBackendCapabilitiesSupportsWrite) (CCSBackendWithCapabilities *);
typedef Bool (*CCSBackendCapabilitiesSupportsProfiles) (CCSBackendWithCapabilities *);
typedef Bool (*CCSBackendCapabilitiesSupportsIntegration) (CCSBackendWithCapabilities *);

struct _CCSBackendCapabilitiesInterface
{
    CCSBackendCapabilitiesSupportsRead supportsRead;
    CCSBackendCapabilitiesSupportsWrite supportsWrite;
    CCSBackendCapabilitiesSupportsProfiles supportsProfiles;
    CCSBackendCapabilitiesSupportsIntegration supportsIntegration;
};

Bool ccsBackendCapabilitiesSupportsRead (CCSBackendWithCapabilities *);
Bool ccsBackendCapabilitiesSupportsWrite (CCSBackendWithCapabilities *);
Bool ccsBackendCapabilitiesSupportsProfiles (CCSBackendWithCapabilities *);
Bool ccsBackendCapabilitiesSupportsIntegration (CCSBackendWithCapabilities *);

unsigned int ccsCCSBackendCapabilitiesInterfaceGetType ();

void ccsFreeBackendWithCapabilities (CCSBackendWithCapabilities *);

/* Backend opener method */
void *
ccsOpenBackend (const char *name, CCSBackendInterface **interface);

/* Constructor method */
CCSBackend *
ccsBackendNewWithDynamicInterface (CCSContext *context, const CCSBackendInterface *interface, void *dlhand);

typedef struct _CCSInterfaceTable CCSInterfaceTable;

/* Constructor method */
CCSBackendWithCapabilities *
ccsBackendWithCapabilitiesWrapBackend (const CCSInterfaceTable *interfaces, CCSBackend *backend);

CCSBackendInterface* getBackendInfo (void);

COMPIZCONFIG_END_DECLS

#endif
