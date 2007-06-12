/*
 * Copyright (C) 2007 Andrew Riedi <andrewriedi@gmail.com>
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * This plug-in for Metacity-like workarounds.
 */

#include <compiz.h>
#include <workarounds_options.h>
#include "workarounds.h"

static CompMetadata workaroundsMetadata;
static int displayPrivateIndex;

static void workaroundsDisplayOptionChanged( CompDisplay *d, CompOption *opt, 
                                             WorkaroundsDisplayOptions num )
{
    WORKAROUNDS_DISPLAY( d );

    switch( num ) {
        case WorkaroundsDisplayOptionLegacyApps:
            wd->legacyApps = opt->value.b;
            break;
        default:
            break;
    }
}

static Bool workaroundsInitDisplay( CompPlugin *plugin, CompDisplay *d )
{
    WorkaroundsDisplay *wd = 
        (WorkaroundsDisplay *) malloc( sizeof( WorkaroundsDisplay ) );

    wd->screenPrivateIndex = allocateScreenPrivateIndex( d );
    if ( wd->screenPrivateIndex < 0 )
    {
        free( wd );
        return FALSE;
    }
    d->privates[displayPrivateIndex].ptr = wd;

    workaroundsSetLegacyAppsNotify( d, workaroundsDisplayOptionChanged );

    return TRUE;
}

static void workaroundsFiniDisplay( CompPlugin *plugin, CompDisplay *d )
{
    WORKAROUNDS_DISPLAY( d );

    freeScreenPrivateIndex( d, wd->screenPrivateIndex );
    free(wd);
}

static Bool workaroundsInitWindow( CompPlugin *plugin, CompWindow *w )
{
    WORKAROUNDS_DISPLAY( w->screen->display );

    if ( wd->legacyApps )
    {
        unsigned int type;

        type = w->wmType;

        /* Some code to make Wine and legacy applications work. */
        if (w->width == w->screen->width && w->height == w->screen->height &&
           !(w->type & CompWindowTypeFullscreenMask) &&
           !(type & CompWindowTypeDesktopMask))
               type = CompWindowTypeFullscreenMask;

        w->type = type;
    }

    return TRUE;
}

static void workaroundsFiniWindow( CompPlugin *plugin, CompWindow *w )
{
}

static Bool workaroundsInit( CompPlugin *plugin )
{
   if ( !compInitPluginMetadataFromInfo( &workaroundsMetadata,
                                         plugin->vTable->name, 0, 0, 0, 0 ) )
        return FALSE;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if ( displayPrivateIndex < 0 )
    {
        compFiniMetadata( &workaroundsMetadata );
        return FALSE;
    }

    compAddMetadataFromFile( &workaroundsMetadata, plugin->vTable->name );

    return TRUE;
}

static void workaroundsFini( CompPlugin *plugin )
{
    freeDisplayPrivateIndex( displayPrivateIndex );
    compFiniMetadata( &workaroundsMetadata );
}

static int workaroundsGetVersion( CompPlugin *plugin, int version )
{   
    return ABIVERSION;
}

static CompMetadata *workaroundsGetMetadata( CompPlugin *plugin )
{
    return &workaroundsMetadata;
}

CompPluginVTable workaroundsVTable = 
{
    "workarounds",
    workaroundsGetVersion,
    workaroundsGetMetadata,
    workaroundsInit,
    workaroundsFini,
    workaroundsInitDisplay,
    workaroundsFiniDisplay,
    0, /* InitScreen */
    0, /* FiniScreen */
    workaroundsInitWindow,
    workaroundsFiniWindow,
    0, /* GetDisplayOptions */
    0, /* SetDisplayOption */
    0, /* GetScreenOptions */
    0, /* SetScreenOption */
    0, /* Deps */
    0, /* nDeps */
    0, /* Features */
    0  /* nFeatures */
};

CompPluginVTable *getCompPluginInfo( void )
{
    return &workaroundsVTable;
}

