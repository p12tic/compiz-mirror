/**
 *
 * GSettings libccs backend
 *
 * gsettings.c
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

#define CCS_LOG_DOMAIN "gsettings"

#include "gsettings.h"
#include "ccs_gsettings_backend.h"

static void
updateSetting (CCSBackend *backend, CCSContext *context, CCSPlugin *plugin, CCSSetting *setting)
{
    int          index;

    readInit (backend, context);
    if (!readOption (backend, setting))
    {
	ccsResetToDefault (setting, TRUE);
    }

    if (ccsGetIntegrationEnabled (context) &&
	isIntegratedOption (setting, &index))
    {
	writeInit (backend, context);
	ccsGSettingsWriteIntegratedOption (backend, context, setting, index);
    }
}

static void
processEvents (CCSBackend *backend, unsigned int flags)
{
    if (!(flags & ProcessEventsNoGlibMainLoopMask))
    {
	while (g_main_context_pending(NULL))
	    g_main_context_iteration(NULL, FALSE);
    }
}

static Bool
initBackend (CCSBackend *backend, CCSContext * context)
{
    g_type_init ();

    return ccsGSettingsBackendAttachNewToBackend (backend, context);
}

static Bool
finiBackend (CCSBackend *backend)
{
    processEvents (backend, 0);
    ccsGSettingsBackendDetachFromBackend (backend);
    processEvents (backend, 0);

    return TRUE;
}

static Bool
getSettingIsIntegrated (CCSBackend *backend, CCSSetting * setting)
{
    if (!ccsGetIntegrationEnabled (ccsPluginGetContext (ccsSettingGetParent (setting))))
	return FALSE;

    if (!isIntegratedOption (setting, NULL))
	return FALSE;

    return TRUE;
}

static Bool
getSettingIsReadOnly (CCSBackend *backend, CCSSetting * setting)
{
    /* FIXME */
    return FALSE;
}

static const CCSBackendInfo *
getInfo (CCSBackend *backend)
{
    return &gsettingsBackendInfo;
}

static Bool
ccsGSettingsDeleteProfileWrapper (CCSBackend *backend,
				  CCSContext *context,
				  char       *profile)
{
    return deleteProfile (backend, context, profile);
}


static CCSBackendInterface gsettingsVTable = {
    getInfo,
    processEvents,
    initBackend,
    finiBackend,
    readInit,
    readSetting,
    0,
    writeInit,
    writeSetting,
    0,
    updateSetting,
    getSettingIsIntegrated,
    getSettingIsReadOnly,
    ccsGSettingsGetExistingProfiles,
    ccsGSettingsDeleteProfileWrapper
};

CCSBackendInterface *
getBackendInfo (void)
{
    return &gsettingsVTable;
}

