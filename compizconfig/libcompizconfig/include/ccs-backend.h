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
#include <ccs-list.h>

typedef struct _CCSBackend	  CCSBackend;
typedef struct _CCSBackendPrivate CCSBackendPrivate;
typedef struct _CCSBackendVTable  CCSBackendInterface;

typedef struct _CCSContext CCSContext;
typedef struct _CCSSetting CCSSetting;

struct _CCSBackend
{
    CCSObject        object;
};

typedef CCSBackendInterface * (*BackendGetInfoProc) (void);

typedef void (*CCSBackendExecuteEventsFunc) (unsigned int flags);

typedef Bool (*CCSBackendInitFunc) (CCSContext * context);
typedef Bool (*CCSBackendFiniFunc) (CCSContext * context);

typedef Bool (*CCSBackendReadInitFunc) (CCSContext * context);
typedef void (*CCSBackendReadSettingFunc)
(CCSContext * context, CCSSetting * setting);
typedef void (*CCSBackendReadDoneFunc) (CCSContext * context);

typedef Bool (*CCSBackendWriteInitFunc) (CCSContext * context);
typedef void (*CCSBackendWriteSettingFunc)
(CCSContext * context, CCSSetting * setting);
typedef void (*CCSBackendWriteDoneFunc) (CCSContext * context);

typedef Bool (*CCSBackendGetSettingIsIntegratedFunc) (CCSSetting * setting);
typedef Bool (*CCSBackendGetSettingIsReadOnlyFunc) (CCSSetting * setting);

typedef CCSStringList (*CCSBackendGetExistingProfilesFunc) (CCSContext * context);
typedef Bool (*CCSBackendDeleteProfileFunc) (CCSContext * context, char * name);

typedef char * (*CCSBackendGetNameFunc) (CCSBackend *);
typedef char * (*CCSBackendGetShortDescFunc) (CCSBackend *);
typedef char * (*CCSBackendGetLongDescFunc) (CCSBackend *);
typedef Bool (*CCSBackendHasIntegrationSupportFunc) (CCSBackend *);
typedef Bool (*CCSBackendHasProfileSupportFunc) (CCSBackend *);

struct _CCSBackendVTable
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


    CCSBackendGetSettingIsIntegratedFunc     getSettingIsIntegrated;
    CCSBackendGetSettingIsReadOnlyFunc       getSettingIsReadOnly;

    CCSBackendGetExistingProfilesFunc getExistingProfiles;
    CCSBackendDeleteProfileFunc       deleteProfile;
};

CCSBackendInterface * ccsBackendGetVTable (CCSBackend *);
void * ccsBackendGetDlHand (CCSBackend *);

void ccsFreeBackend (CCSBackend *);

CCSBackendInterface* getBackendInfo (void);

#endif
