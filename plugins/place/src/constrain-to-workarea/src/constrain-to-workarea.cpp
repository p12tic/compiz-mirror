/*
 * Copyright (C) 2001 Havoc Pennington
 * Copyright (C) 2002, 2003 Red Hat, Inc.
 * Copyright (C) 2003 Rob Adams
 * Copyright (C) 2005 Novell, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "constrain-to-workarea.h"

namespace compiz
{
namespace place
{
unsigned int clampGeometrySizeOnly = (1 << 0);
unsigned int clampGeometryToViewport = (1 << 1);
}
}

namespace cp = compiz::place;
namespace cw = compiz::window;

void
cp::clampGeometryToWorkArea (cw::Geometry            &g,
			     const CompRect          &workArea,
			     const CompWindowExtents &border,
			     unsigned int            flags,
			     const CompSize          &screenSize)
{
    int	     x, y, left, right, bottom, top;

    if (flags & clampGeometryToViewport)
    {
	/* left, right, top, bottom target coordinates, clamed to viewport
	 * sizes as we don't need to validate movements to other viewports;
	 * we are only interested in inner-viewport movements */

	x = g.x () % screenSize.width ();
	if ((x + g.width ()) < 0)
	    x += screenSize.width ();

	y = g.y () % screenSize.height ();
	if ((y + g.height ()) < 0)
	    y += screenSize.height ();
    }
    else
    {
	x = g.x ();
	y = g.y ();
    }

    left   = x - border.left;
    right  = left + g.widthIncBorders () +  (border.left +
					     border.right);
    top    = y - border.top;
    bottom = top + g.heightIncBorders () + (border.top +
					    border.bottom);

    if ((right - left) > workArea.width ())
    {
	left  = workArea.left ();
	right = workArea.right ();
    }
    else
    {
	if (left < workArea.left ())
	{
	    right += workArea.left () - left;
	    left  = workArea.left ();
	}

	if (right > workArea.right ())
	{
	    left -= right - workArea.right ();
	    right = workArea.right ();
	}
    }

    if ((bottom - top) > workArea.height ())
    {
	top    = workArea.top ();
	bottom = workArea.bottom ();
    }
    else
    {
	if (top < workArea.top ())
	{
	    bottom += workArea.top () - top;
	    top    = workArea.top ();
	}

	if (bottom > workArea.bottom ())
	{
	    top   -= bottom - workArea.bottom ();
	    bottom = workArea.bottom ();
	}
    }

    /* bring left/right/top/bottom to actual window coordinates */
    left   += border.left;
    right  -= border.right + 2 * g.border ();
    top    += border.top;
    bottom -= border.bottom + 2 * g.border ();

    if ((right - left) != g.width ())
    {
	g.setWidth (right - left);
	flags &= ~clampGeometrySizeOnly;
    }

    if ((bottom - top) != g.height ())
    {
	g.setHeight (bottom - top);
	flags &= ~clampGeometrySizeOnly;
    }

    if (!(flags & clampGeometrySizeOnly))
    {
	if (left != x)
	    g.setX (g.x () + left - x);

	if (top != y)
	    g.setY (g.y () + top - y);
    }
}

CompPoint &
cp::constrainPositionToWorkArea (CompPoint               &pos,
			         const cw::Geometry      &serverGeometry,
			         const CompWindowExtents &border,
			         const CompRect          &workArea,
			         bool                    staticGravity)
{
    CompWindowExtents extents;
    int               delta;

    CompWindowExtents effectiveBorders = border;

    /* Ignore borders in the StaticGravity case for placement
     * because the window intended to be placed as if it didn't
     * have them */
    if (staticGravity)
    {
	effectiveBorders.left = 0;
	effectiveBorders.right = 0;
	effectiveBorders.top = 0;
	effectiveBorders.bottom = 0;
    }

    extents.left   = pos.x () - effectiveBorders.left;
    extents.top    = pos.y () - effectiveBorders.top;
    extents.right  = extents.left + serverGeometry.widthIncBorders () +
		     (effectiveBorders.left +
		      effectiveBorders.right);
    extents.bottom = extents.top + serverGeometry.heightIncBorders () +
		     (effectiveBorders.top +
		      effectiveBorders.bottom);

    delta = workArea.right () - extents.right;
    if (delta < 0)
	extents.left += delta;

    delta = workArea.left () - extents.left;
    if (delta > 0)
	extents.left += delta;

    delta = workArea.bottom () - extents.bottom;
    if (delta < 0)
	extents.top += delta;

    delta = workArea.top () - extents.top;
    if (delta > 0)
	extents.top += delta;

    pos.setX (extents.left + effectiveBorders.left);
    pos.setY (extents.top  + effectiveBorders.top);

    return pos;
}
