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

static Bool workaroundsInit( CompPlugin *plugin )
{
    return TRUE;
}

static void workaroundsFini( CompPlugin *plugin )
{
}

static int workaroundsGetVersion( CompPlugin *plugin, int version )
{   
    return ABIVERSION;
}

CompPluginVTable workaroundsVTable = 
{
    "workarounds",
    workaroundsGetVersion,
    0, /* GetMetadata */
    workaroundsInit,
    workaroundsFini,
    0, /* InitDisplay */
    0, /* FiniDisplay */
    0, /* InitScreen */
    0, /* FiniScreen */
    0, /* InitWindow */
    0, /* FiniWindow */
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
