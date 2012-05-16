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

#include <X11/X.h>
#include <X11/Xlib.h>

#define CompAltMask        (1 << 16)
#define CompMetaMask       (1 << 17)
#define CompSuperMask      (1 << 18)
#define CompHyperMask      (1 << 19)
#define CompModeSwitchMask (1 << 20)
#define CompNumLockMask    (1 << 21)
#define CompScrollLockMask (1 << 22)

#define COMPIZ_SCHEMA_ID   "org.freedesktop.compiz"
#define COMPIZCONFIG_SCHEMA_ID "org.freedesktop.compizconfig"
#define PROFILE_SCHEMA_ID "org.freedesktop.compizconfig.profile"
#define METACITY     "/apps/metacity"
#define COMPIZ       "/apps/compiz-1"
#define COMPIZ_PROFILEPATH COMPIZ "/profiles"
#define COMPIZCONFIG "/org/freedesktop/compizconfig"
#define PROFILEPATH  COMPIZCONFIG "/profiles"
#define DEFAULTPROF "Default"
#define CORE_NAME   "core"

#define BUFSIZE 512

#define NUM_WATCHED_DIRS 3

#define KEYNAME(sn)     char keyName[BUFSIZE]; \
                    snprintf (keyName, BUFSIZE, "screen%i", sn);

#define PATHNAME(p,k)    char pathName[BUFSIZE]; \
                    if (!p || \
			strcmp (p, "core") == 0) \
                        snprintf (pathName, BUFSIZE, \
				 "%s/%s/plugins/%s/%s/options/", COMPIZ, currentProfile, \
				 p, k); \
                    else \
			snprintf(pathName, BUFSIZE, \
				 "%s/%s/plugins/%s/%s/options/", COMPIZ, currentProfile, \
				 p, k);

#define _GNU_SOURCE

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
