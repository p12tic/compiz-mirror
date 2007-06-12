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
 * Workarounds' header file.
 */

#ifndef COMPIZ_WORKAROUNDS_H
#define COMPIZ_WORKAROUNDS_H

typedef struct _WorkaroundsDisplay {
    int screenPrivateIndex;

    /* Options */
    Bool legacyApps;
} WorkaroundsDisplay;

#define GET_WORKAROUNDS_DISPLAY(d) \
    ((WorkaroundsDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define WORKAROUNDS_DISPLAY(d) \
    WorkaroundsDisplay *wd = GET_WORKAROUNDS_DISPLAY (d)

#endif /* COMPIZ_WORKAROUNDS_H */
