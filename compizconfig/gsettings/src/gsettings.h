/**
 *
 * GSettings libccs backend
 *
 * gconf-integration.c
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

#ifndef _COMPIZCONFIG_BACKEND_GSETTINGS_GSETTINGS_H
#define _COMPIZCONFIG_BACKEND_GSETTINGS_GSETTINGS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <string.h>
#include <dirent.h>

#include <ccs.h>
#include <ccs-backend.h>

#include <gio/gio.h>

#include "gsettings_shared.h"

#define BUFSIZE 512

#define NUM_WATCHED_DIRS 3

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef USE_GCONF
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>
#endif

#include <ccs_gsettings_interface.h>

typedef enum {
    OptionInt,
    OptionBool,
    OptionKey,
    OptionString,
    OptionSpecial,
} SpecialOptionType;

struct _CCSGSettingsBackendPrivate
{
    GList	   *settingsList;
    CCSGSettingsWrapper *compizconfigSettings;
    CCSGSettingsWrapper *currentProfileSettings;

    char	    *currentProfile;
    CCSContext  *context;

#ifdef USE_GCONF
    GConfClient *client;
    guint       *gnomeGConfNotifyIds;
#endif

};

Bool readInit (CCSBackend *, CCSContext * context);
void readSetting (CCSBackend *, CCSContext * context, CCSSetting * setting);
Bool readOption (CCSBackend *backend, CCSSetting * setting);
Bool writeInit (CCSBackend *, CCSContext * context);
void writeOption (CCSBackend *backend, CCSSetting *setting);

#ifdef USE_GCONF



typedef struct _SpecialOptionGConf {
    const char*       settingName;
    const char*       pluginName;
    Bool	      screen;
    const char*       gnomeName;
    SpecialOptionType type;
} SpecialOptionGConf;

Bool
isGConfIntegratedOption (CCSSetting *setting,
			 int	    *index);

void
gnomeGConfValueChanged (GConfClient *client,
			guint       cnxn_id,
			GConfEntry  *entry,
			gpointer    user_data);

void
initGConfClient (CCSBackend *backend);

void
finiGConfClient (CCSBackend *backend);

Bool
readGConfIntegratedOption (CCSBackend *backend,
			   CCSContext *context,
			   CCSSetting *setting,
			   int	      index);

void
writeGConfIntegratedOption (CCSBackend *backend,
			    CCSContext *context,
			    CCSSetting *setting,
			    int	       index);

#endif

#endif
