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

#define PATHNAME(p)    char pathName[BUFSIZE]; \
                    if (!p || \
			strcmp (p, "core") == 0) \
                        snprintf (pathName, BUFSIZE, \
				 "%s%s/plugins/%s/", COMPIZ_PROFILEPATH, currentProfile, \
				 p); \
                    else \
			snprintf(pathName, BUFSIZE, \
				 "%s%s/plugins/%s/", COMPIZ_PROFILEPATH, currentProfile, \
				 p);

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

typedef enum {
    OptionInt,
    OptionBool,
    OptionKey,
    OptionString,
    OptionSpecial,
} SpecialOptionType;

char *currentProfile;

Bool readInit (CCSContext * context);
void readSetting (CCSContext * context, CCSSetting * setting);
Bool readOption (CCSSetting * setting);
Bool writeInit (CCSContext * context);
void writeOption (CCSSetting *setting);

#ifdef USE_GCONF

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>

GConfClient *client;
guint gnomeGConfNotifyIds[NUM_WATCHED_DIRS];

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
initGConfClient (CCSContext *context);

void
finiGConfClient (void);

Bool
readGConfIntegratedOption (CCSContext *context,
			   CCSSetting *setting,
			   int	      index);

void
writeGConfIntegratedOption (CCSContext *context,
			    CCSSetting *setting,
			    int	       index);

#endif

#endif
