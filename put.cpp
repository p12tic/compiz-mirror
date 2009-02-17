/*
 * Copyright (c) 2006 Darryll Truchan <moppsy@comcast.net>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "put.h"

#include <cmath>
#include <cstring>
#include <iostream>

#define PUT_ONLY_EMPTY(type) (type >= PutEmptyBottomLeft && type <= PutEmptyTopRight)


/*
 * Maximumize functions
 * Functions are from Maximumize plugin (Author:Kristian Lyngstøl  <kristian@bohemians.org>)
 */

/* Generates a region containing free space (here the
 * active window counts as free space). The region argument
 * is the start-region (ie: the output dev).
 * Logic borrowed from opacify (courtesy of myself).
 */
Region
PutScreen::emptyRegion (CompWindow *window,
			Region     region)
{
    Region     newRegion, tmpRegion;
    XRectangle tmpRect;

    newRegion = XCreateRegion ();
    if (!newRegion)
	return NULL;

    tmpRegion = XCreateRegion ();
    if (!tmpRegion)
    {
	XDestroyRegion (newRegion);
	return NULL;
    }

    XUnionRegion (region, newRegion, newRegion);

    foreach(CompWindow *w, screen->windows ())
    {
	EMPTY_REGION (tmpRegion);
        if (w->id () == window->id ())
            continue;

        if (w->invisible () || /*w->hidden () ||*/ w->minimized ())
            continue;

	if (w->wmType () & CompWindowTypeDesktopMask)
	    continue;

	if (w->wmType () & CompWindowTypeDockMask)
	{
	    if (w->struts ()) 
	    {
		XUnionRectWithRegion (&w->struts ()->left, tmpRegion, tmpRegion);
		XUnionRectWithRegion (&w->struts ()->right, tmpRegion, tmpRegion);
		XUnionRectWithRegion (&w->struts ()->top, tmpRegion, tmpRegion);
		XUnionRectWithRegion (&w->struts ()->bottom, tmpRegion, tmpRegion);
		XSubtractRegion (newRegion, tmpRegion, newRegion);
	    }
	    continue;
	}


	tmpRect.x = w->serverX () - w->input().left;
	tmpRect.y = w->serverY () - w->input().top;
	tmpRect.width  = w->serverWidth () + w->input ().right + w->input ().left;
	tmpRect.height = w->serverHeight () + w->input ().top +
	                 w->input ().bottom;

	XUnionRectWithRegion (&tmpRect, tmpRegion, tmpRegion);
	XSubtractRegion (newRegion, tmpRegion, newRegion);
    }

    XDestroyRegion (tmpRegion);
    
    return newRegion;
}

/* Returns true if box a has a larger area than box b.
 */
bool
PutScreen::boxCompare (BOX a,
		       BOX b)
{
    int areaA, areaB;

    areaA = (a.x2 - a.x1) * (a.y2 - a.y1);
    areaB = (b.x2 - b.x1) * (b.y2 - b.y1);

    return (areaA > areaB);
}

/* Extends the given box for Window w to fit as much space in region r.
 * If XFirst is true, it will first expand in the X direction,
 * then Y. This is because it gives different results. 
 * PS: Decorations are icky.
 */
BOX
PutScreen::extendBox (CompWindow *w,		     
	   BOX        tmp,
	   Region     r,
	   bool       xFirst,
	   bool       left,
	   bool       right,
	   bool       up,
	   bool       down)
{
    short int counter = 0;
    Bool      touch = false;

#define CHECKREC \
	XRectInRegion (r, tmp.x1 - w->input ().left, tmp.y1 - w->input ().top,\
		       tmp.x2 - tmp.x1 + w->input ().left + w->input ().right,\
		       tmp.y2 - tmp.y1 + w->input ().top + w->input ().bottom)\
			 == RectangleIn

    while (counter < 1)
    {
	if ((xFirst && counter == 0) || (!xFirst && counter == 1))
	{
	    if(left) {
		while (CHECKREC) {
		    tmp.x1--;
		    touch = true;
		}
		if (touch) tmp.x1++;
	    }
	    touch = false;
	    
	    if(right) {
		while (CHECKREC) {
		    tmp.x2++;
		    touch = true;
		}
		if (touch) tmp.x2--;
	    }
	    touch = false;
	    counter++;
	}

	if ((xFirst && counter == 1) || (!xFirst && counter == 0))
	{
	    if(down) {
		while (CHECKREC) {
		    tmp.y2++;
		    touch = true;
		}
		if (touch) tmp.y2--;
	    }
	    touch = false;
	    
	    if(up) {
		while (CHECKREC) {
		    tmp.y1--;
		    touch = true;
		}
		if (touch) tmp.y1++;
	    }
	    touch = false;
	    counter++;
	}
    }
#undef CHECKREC
    return tmp;
}

/* Create a box for resizing in the given region
 * Also shrinks the window box in case of minor overlaps.
 * FIXME: should be somewhat cleaner.
 */
BOX
PutScreen::findRect (CompWindow *w,
		     Region     r,
		     bool       left,
		     bool       right,
		     bool       up,
		     bool       down)
{
    BOX windowBox, ansA, ansB, orig;

    windowBox.x1 = w->serverX ();
    windowBox.x2 = w->serverX ()  + w->serverWidth ();
    windowBox.y1 = w->serverY ();
    windowBox.y2 = w->serverY () + w->serverHeight ();

    orig = windowBox;

    ansA = extendBox (w, windowBox, r, true, left, right, up, down);
    ansB = extendBox (w, windowBox, r, false, left, right, up, down);

    if (boxCompare (orig, ansA) &&
	boxCompare (orig, ansB))
	return orig;
    if (boxCompare (ansA, ansB))
	return ansA;
    else
	return ansB;

}

/* Calls out to compute the resize */
unsigned int
PutScreen::computeResize(CompWindow     *w,
			 XWindowChanges *xwc,
			 bool           left,
			 bool           right,
			 bool           up,
			 bool           down)
{
    CompOutput   *output;
    Region       region;
    unsigned int mask = 0;
    BOX          box;
    int outputDevice = w->outputDevice ();
    

    output = &screen->outputDevs ()[outputDevice];
    region = emptyRegion (w, output->region ());
    if (!region)
	return mask;

    box = findRect (w, region, left, right, up, down);
    XDestroyRegion (region);

    if (box.x1 != w->serverX ())
	mask |= CWX;

    if (box.y1 != w->serverY ())
	mask |= CWY;

    if ((box.x2 - box.x1) != w->serverWidth ())
	mask |= CWWidth;

    if ((box.y2 - box.y1) != w->height ())
	mask |= CWHeight;

    xwc->x = box.x1;
    xwc->y = box.y1;
    xwc->width = box.x2 - box.x1; 
    xwc->height = box.y2 - box.y1;

    return mask;
}


/*
 * End of Maximumize functions
 */

/*
 * calculate the velocity for the moving window
 */
int
PutScreen::adjustVelocity (CompWindow *w)
{
    float dx, dy, adjust, amount;
    float x1, y1;

    PUT_WINDOW (w);

    x1 = pw->targetX;
    y1 = pw->targetY;

    dx = x1 - (w->x () + pw->tx);
    dy = y1 - (w->y () + pw->ty);

    adjust = dx * 0.15f;
    amount = fabs (dx) * 1.5;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    pw->xVelocity = (amount * pw->xVelocity + adjust) / (amount + 1.0f);

    adjust = dy * 0.15f;
    amount = fabs (dy) * 1.5f;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    pw->yVelocity = (amount * pw->yVelocity + adjust) / (amount + 1.0f);

    if (fabs (dx) < 0.1f && fabs (pw->xVelocity) < 0.2f &&
	fabs (dy) < 0.1f && fabs (pw->yVelocity) < 0.2f)
    {
	/* animation done */
	pw->xVelocity = pw->yVelocity = 0.0f;

	pw->tx = x1 - w->x ();
	pw->ty = y1 - w->y ();
	return 0;
    }
    return 1;
}

void
PutScreen::finishWindowMovement (CompWindow *w)
{
    PUT_WINDOW (w);

    w->move (pw->targetX - w->x (),
	     pw->targetY - w->y (),
	     true);
    w->syncPosition ();

    if (w->state () & (MAXIMIZE_STATE | CompWindowStateFullscreenMask))
	w->updateAttributes (CompStackingUpdateModeNone);
}

unsigned int
PutScreen::getOutputForWindow (CompWindow *w)
{
    PUT_WINDOW (w);

    if (!pw->adjust)
	return w->outputDevice ();

    /* outputDeviceForWindow uses the server geometry,
       so specialcase a running animation, which didn't
       apply the server geometry yet */
    return screen->outputDeviceForGeometry (CompWindow::Geometry(
								 w->x () + pw->tx,
								 w->y () + pw->ty,
								 w->width (),
								 w->height (),
								 w->geometry ().border() ));
}


bool
PutScreen::getDistance (CompWindow         *w,
			PutType            type,
			CompOption::Vector &option,
			int                *distX,
			int                *distY)
{

    CompScreen *s = screen;
    int        x, y, dx, dy, posX, posY;
    int        viewport, output;
    XRectangle workArea;

    PUT_SCREEN (s);
    PUT_WINDOW (w);

    posX = CompOption::getIntOptionNamed (option,"x", 0);
    posY = CompOption::getIntOptionNamed (option,"y", 0);

    /* get the output device number from the options list */
    output = CompOption::getIntOptionNamed (option,"output", -1);
    /* no output in options list -> use the current output */
    if (output == -1)
    {
	/* no output given, so use the current output if
	   this wasn't a double tap */

	if (ps->lastType != type || ps->lastWindow != w->id ())
	    output = getOutputForWindow (w);
    }
    else
    {
	/* make sure the output number is not out of bounds */
	output = MIN (output,  s->outputDevs ().size () - 1);
    }

    if (output == -1)
    {
	/* user double-tapped the key, so use the screen work area */
	workArea = s->workArea ();
	/* set the type to unknown to have a toggle-type behaviour
	   between 'use output work area' and 'use screen work area' */
	ps->lastType = PutUnknown;
    }
    else
    {
	/* single tap or output provided via options list,
	   use the output work area */
	s->getWorkareaForOutput (output, &workArea);
	ps->lastType = type;
    }

    if(PUT_ONLY_EMPTY(type))
    {
	unsigned int   mask;
	XWindowChanges xwc;
	bool left, right, up, down;
	left = right = up = down = false;
	switch(type) {
	    case PutEmptyBottomLeft:
		left = down = true;
		break;
	    case PutEmptyBottom:
		down = true;
		break;
	    case PutEmptyBottomRight:
		right = down = true;
		break;
	    case PutEmptyLeft:
		left = true;
		break;
	    case PutEmptyCenter:
		left = right = up = down = true;
		break;
	    case PutEmptyRight:
		right = true;
		break;
	    case PutEmptyTopLeft:
		left = up = true;
		break;
	    case PutEmptyTop:
		up = true;
		break;
	    case PutEmptyTopRight:
		right = up = true;
		break;
	    default:
		left = right = up = down = false;
	}
	mask = computeResize (w, &xwc, left,right,up,down);
	if (mask)
	{
	    int width_, height_;
	    if (w->constrainNewWindowSize ( xwc.width, xwc.height,
					    &width_, &height_))
	    {
		mask |= CWWidth | CWHeight;
		xwc.width  = width_;
		xwc.height = height_;
	    }
	}
	workArea.width  = xwc.width;
	workArea.height = xwc.height;
	workArea.x      = xwc.x;
	workArea.y      = xwc.y;
    }
    /* the windows location */
    x = w->x () + pw->tx;
    y = w->y () + pw->ty;

    switch (type) {
    case PutEmptyCenter:
    case PutCenter:
	/* center of the screen */
	dx = (workArea.width / 2) - (w->serverWidth () / 2) - (x - workArea.x);
	dy = (workArea.height / 2) - (w->serverHeight () / 2) - (y - workArea.y);
	break;
    case PutLeft:
	/* center of the left edge */
	dx = -(x - workArea.x) + w->input().left + ps->optionGetPadLeft ();
	dy = (workArea.height / 2) - (w->serverHeight () / 2) - (y - workArea.y);
	break;
    case PutEmptyLeft:
	/* center of the left edge */
	workArea.x -= w->input().left;
	dx = -(x - workArea.x) + w->input().left + ps->optionGetPadLeft ();
	dy = (workArea.height / 2) - (w->serverHeight () / 2) - (y - workArea.y);
	break;
    case PutTopLeft:
	/* top left corner */
	dx = -(x - workArea.x) + w->input().left + ps->optionGetPadLeft ();
	dy = -(y - workArea.y) + w->input().top + ps->optionGetPadTop ();
	break;
    case PutEmptyTopLeft:
	/* top left corner */
	workArea.y -= w->input().top;
	workArea.x -= w->input().left;
	dx = -(x - workArea.x) + w->input().left + ps->optionGetPadLeft ();
	dy = -(y - workArea.y) + w->input().top + ps->optionGetPadTop ();
	break;
    case PutTop:
	/* center of top edge */
	dx = (workArea.width / 2) - (w->serverWidth () / 2) - (x - workArea.x);
	dy = -(y - workArea.y) + w->input().top + ps->optionGetPadTop ();
	break;
    case PutEmptyTop:
	/* center of top edge */
	workArea.y -= w->input().top;
	dx = (workArea.width / 2) - (w->serverWidth () / 2) - (x - workArea.x);
	dy = -(y - workArea.y) + w->input().top + ps->optionGetPadTop ();
	break;
    case PutTopRight:
	/* top right corner */
	dx = workArea.width - w->serverWidth () - (x - workArea.x) -
	    w->input().right - ps->optionGetPadRight ();
	dy = -(y - workArea.y) + w->input().top + ps->optionGetPadTop ();
	break;
    case PutEmptyTopRight:
	/* top right corner */
	workArea.y -= w->input().top;
	workArea.x += w->input().right;
	dx = workArea.width - w->serverWidth () - (x - workArea.x) -
	     w->input().right - ps->optionGetPadRight ();
	dy = -(y - workArea.y) + w->input().top + ps->optionGetPadTop ();
	break;
    case PutRight:
	/* center of right edge */
	dx = workArea.width - w->serverWidth () - (x - workArea.x) -
	    w->input().right - ps->optionGetPadRight ();
	dy = (workArea.height / 2) - (w->serverHeight () / 2) - (y - workArea.y);
	break;
    case PutEmptyRight:
	/* center of right edge */
	workArea.x += w->input().right;
	dx = workArea.width - w->serverWidth () - (x - workArea.x) -
	     w->input().right - ps->optionGetPadRight ();
	dy = (workArea.height / 2) - (w->serverHeight () / 2) - (y - workArea.y);
	break;
    case PutBottomRight:
	/* bottom right corner */
	dx = workArea.width - w->serverWidth () - (x - workArea.x) -
	    w->input().right - ps->optionGetPadRight ();
	dy = workArea.height - w->serverHeight () - (y - workArea.y) -
	    w->input().bottom - ps->optionGetPadBottom ();
	break;
    case PutEmptyBottomRight:
	/* bottom right corner */
	workArea.y += w->input().bottom;
	workArea.x += w->input().right;
	dx = workArea.width - w->serverWidth () - (x - workArea.x) -
	     w->input().right - ps->optionGetPadRight ();
	dy = workArea.height - w->serverHeight () - (y - workArea.y) -
	     w->input().bottom - ps->optionGetPadBottom ();
	break;
    case PutBottom:
	/* center of bottom edge */
	dx = (workArea.width / 2) - (w->serverWidth () / 2) - (x - workArea.x);
	dy = workArea.height - w->serverHeight () - (y - workArea.y) -
	     w->input().bottom - ps->optionGetPadBottom ();
	break;
    case PutEmptyBottom:
	/* center of bottom edge */
	workArea.y += w->input().bottom;
	dx = (workArea.width / 2) - (w->serverWidth () / 2) - (x - workArea.x);
	dy = workArea.height - w->serverHeight () - (y - workArea.y) -
	     w->input().bottom - ps->optionGetPadBottom ();
	break;
    case PutBottomLeft:
	/* bottom left corner */
	dx = -(x - workArea.x) + w->input().left + ps->optionGetPadLeft ();
	dy = workArea.height - w->serverHeight () - (y - workArea.y) -
	     w->input().bottom - ps->optionGetPadBottom ();
	break;
    case PutEmptyBottomLeft:
	/* bottom left corner */
	workArea.y += w->input().bottom;
	workArea.x -= w->input().left;
	dx = -(x - workArea.x) + w->input().left + ps->optionGetPadLeft ();
	dy = workArea.height - w->serverHeight () - (y - workArea.y) -
	     w->input().bottom - ps->optionGetPadBottom ();
	break;
    case PutRestore:
	/* back to last position */
	dx = pw->lastX - x;
	dy = pw->lastY - y;
	break;
    case PutViewport:
	{
	    int vpX, vpY, hDirection, vDirection;

	    /* get the viewport to move to from the options list */
	    viewport = CompOption::getIntOptionNamed (option, "viewport", -1);

	    /* if viewport wasn't supplied, bail out */
	    if (viewport < 0)
		return false;

	    /* split 1D viewport value into 2D x and y viewport */
	    vpX = viewport % s->vpSize ().width ();
	    vpY = viewport / s->vpSize ().height () ;
	    if (vpY > s->vpSize ().width ())
		vpY = s->vpSize ().width () - 1;

	    /* take the shortest horizontal path to the destination viewport */
	    hDirection = (vpX - s->vp ().x ());
	    if (hDirection > s->vpSize ().height () / 2)
		hDirection = (hDirection - s->vpSize ().height ());
	    else if (hDirection < -s->vpSize ().height () / 2)
		hDirection = (hDirection + s->vpSize ().height ());

	    /* we need to do this for the vertical destination viewport too */
	    vDirection = (vpY - s->vp ().y ());
	    if (vDirection > s->vpSize ().width () / 2)
		vDirection = (vDirection - s->vpSize ().width ());
	    else if (vDirection < -s->vpSize ().width () / 2)
		vDirection = (vDirection + s->vpSize ().width ());

	    dx = s->width () * hDirection;
	    dy = s->height () * vDirection;
	    break;
	}
    case PutViewportLeft:
	/* move to the viewport on the left */
	dx = -s->width ();
	dy = 0;
	break;
    case PutViewportRight:
	/* move to the viewport on the right */
	dx = s->width ();
	dy = 0;
	break;
    case PutViewportUp:
	/* move to the viewport above */
	dx = 0;
	dy = -s->height ();
	break;
    case PutViewportDown:
	/* move to the viewport below */
	dx = 0;
	dy = s->height ();
	break;
    case PutNextOutput:
	{
	    int        outputNum, currentNum;
	    int nOutputDev = s->outputDevs ().size ();
	    CompOutput *currentOutput, *newOutput;

	    if (nOutputDev < 2)
		return false;

	    currentNum = getOutputForWindow (w);
	    outputNum  = (currentNum + 1) % nOutputDev;
	    outputNum  = CompOption::getIntOptionNamed (option,"output", outputNum);

	    if (outputNum >= nOutputDev)
		return false;

	    currentOutput = &s->outputDevs ().at(currentNum);
	    newOutput     = &s->outputDevs ().at(outputNum);

	    /* move by the distance of the output center points */
	    dx = (newOutput->x1 () + newOutput->width () / 2) -
		 (currentOutput->x1 () + currentOutput->width () / 2);
	    dy = (newOutput->y1 () + newOutput->height () / 2) -
		 (currentOutput->y1 () + currentOutput->height () / 2);

	    /* update work area for new output */
	    workArea = newOutput->workArea ();
	}
	break;
    case PutAbsolute:
	/* move the window to an exact position */
	if (posX < 0)
	    /* account for a specified negative position,
	       like geometry without (-0) */
	    dx = posX + s->width () - w->serverWidth () - x - w->input().right;
	else
	    dx = posX - x + w->input().left;

	if (posY < 0)
	    /* account for a specified negative position,
	       like geometry without (-0) */
	    dy = posY + s->height () - w->height () - y - w->input().bottom;
	else
	    dy = posY - y + w->input().top;
	break;
    case PutRelative:
	/* move window by offset */
	dx = posX;
	dy = posY;
	break;
    case PutPointer:
	{
	    /* move the window to the pointers position
	     * using the current quad of the screen to determine
	     * which corner to move to the pointer
	     */
	    int          rx, ry;
	    Window       root, child;
	    int          winX, winY;
	    unsigned int pMask;

	    /* get the pointers position from the X server */
	    if (XQueryPointer (s->dpy (), w->id (), &root, &child,
			       &rx, &ry, &winX, &winY, &pMask)) 
	    {
		if (ps->optionGetWindowCenter ())
		{
		    /* window center */
		    dx = rx - (w->serverWidth () / 2) - x;
		    dy = ry - (w->serverHeight () / 2) - y;
		}
		else if (rx < s->workArea ().width / 2 &&
			 ry < s->workArea ().height / 2)
		{
		    /* top left quad */
		    dx = rx - x + w->input().left;
		    dy = ry - y + w->input().top;
		}
		else if (rx < s->workArea ().width / 2 &&
			 ry >= s->workArea ().height / 2)
		{
		    /* bottom left quad */
		    dx = rx - x + w->input().left;
		    dy = ry - w->height () - y - w->input().bottom;
		}
		else if (rx >= s->workArea ().width / 2 &&
			 ry < s->workArea ().height / 2)
		{
		    /* top right quad */
		    dx = rx - w->width () - x - w->input().right;
		    dy = ry - y + w->input().top;
		}
		else
		{
		    /* bottom right quad */
		    dx = rx - w->width () - x - w->input().right;
		    dy = ry - w->height () - y - w->input().bottom;
		}
	    }
	    else
	    {
		dx = dy = 0;
	    }
	}
	break;
    default:
	/* if an unknown type is specified, do nothing */
	dx = dy = 0;
	break;
    }

    if ((dx || dy) && ps->optionGetAvoidOffscreen () &&
	!(w->type () & CompWindowTypeFullscreenMask))
    {
	/* avoids window borders offscreen, but allow full
	   viewport movements */
	int               inDx, dxBefore;
	int               inDy, dyBefore;
	CompWindowExtents extents, area;

	inDx = dxBefore = dx % s->width ();
	inDy = dyBefore = dy % s->height ();

	extents.left   = x + inDx - w->input().left;
	extents.top    = y + inDy - w->input().top;
	extents.right  = x + inDx + w->serverWidth () + w->input().right;
	extents.bottom = y + inDy + w->serverHeight () + w->input().bottom;

	area.left   = workArea.x + ps->optionGetPadLeft ();
	area.top    = workArea.y + ps->optionGetPadTop ();
	area.right  = workArea.x + workArea.width - ps->optionGetPadRight ();
	area.bottom = workArea.y + workArea.height - ps->optionGetPadBottom ();

	if (extents.left < area.left)
	    inDx += area.left - extents.left;
	else if (w->serverWidth () <= workArea.width && extents.right > area.right)
	    inDx += area.right - extents.right;

	if (extents.top < area.top)
	    inDy += area.top - extents.top;
	else if (w->serverHeight () <= workArea.height &&
		 extents.bottom > area.bottom)
	    inDy += area.bottom - extents.bottom;

	/* apply the change */
	dx += inDx - dxBefore;
	dy += inDy - dyBefore;
    }

    *distX = dx;
    *distY = dy;

    return true;
}

void
PutScreen::preparePaint (int ms)
{

    PUT_SCREEN (screen);

    if (ps->moreAdjust && ps->grabIndex)
    {
	int        steps;
	float      amount, chunk;

	amount = ms * 0.025f * ps->optionGetSpeed ();
	steps = amount / (0.5f * ps->optionGetTimestep ());
	if (!steps)
	    steps = 1;
	chunk = amount / (float)steps;

	while (steps--)
	{
	    Window endAnimationWindow = None;

	    ps->moreAdjust = 0;
	    foreach(CompWindow *w, screen->windows ())
	    {
		PUT_WINDOW (w);

		if (pw->adjust)
		{
		    pw->adjust = adjustVelocity (w);
		    ps->moreAdjust |= pw->adjust;

		    pw->tx += pw->xVelocity * chunk;
		    pw->ty += pw->yVelocity * chunk;

		    if (!pw->adjust)
		    {
			/* animation done */
			finishWindowMovement (w);

			if (w->id  () == screen->activeWindow ())
			    endAnimationWindow = w->id ();

			pw->tx = pw->ty = 0;
		    }
		}
    	    }
	    if (!ps->moreAdjust)
	    {
		/* unfocus moved window if enabled */
		if (ps->optionGetUnfocusWindow ())
		    screen->focusDefaultWindow ();
		else if (endAnimationWindow)
		    screen->sendWindowActivationRequest (endAnimationWindow);
		break;
    	    }
	}
    }
    
    cScreen->preparePaint (ms);
}

/* This is the guts of the paint function. You can transform the way the entire output is painted
 * or you can just draw things on screen with openGL. The unsigned int here is a mask for painting
 * the screen, see opengl/opengl.h on how you can change it */

bool
PutScreen::glPaintOutput (const GLScreenPaintAttrib     &attrib,
			  const GLMatrix		&transform, 
			  const CompRegion		&region, 
			  CompOutput 		        *output,
			  unsigned int		        mask) 
{
    bool status;

    PUT_SCREEN (screen);

    if (ps->moreAdjust)
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    status = gScreen->glPaintOutput (attrib, transform, region, output, mask);

    return status;
}

void
PutScreen::donePaint ()
{
    PUT_SCREEN (screen);

    if (ps->moreAdjust && ps->grabIndex)
	cScreen->damageScreen (); // FIXME
    else
    {
	if (ps->grabIndex)
	{
	    /* release the screen grab */
	    screen->removeGrab (ps->grabIndex, NULL);
	    ps->grabIndex = 0;
	}
    }

    cScreen->donePaint ();
}

/* This is our event handler. It directly hooks into the screen's X Event handler and allows us to handle
 * our raw X Events
 */

void
PutScreen::handleEvent (XEvent *event)
{
    switch (event->type)
    {
	/* handle client events */
    case ClientMessage:
	/* accept the custom atom for putting windows */
	if (event->xclient.message_type == compizPutWindowAtom)
	{
	    CompWindow *w;

	    w = screen->findWindow(event->xclient.window);
	    if (w)
	    {
		/*
		 * get the values from the xclientmessage event and populate
		 * the options for put initiate
		 *
		 * the format is 32
		 * and the data is
		 * l[0] = x position - unused (for future PutExact)
		 * l[1] = y position - unused (for future PutExact)
		 * l[2] = viewport number
		 * l[3] = put type, int value from enum
		 * l[4] = output number
		 */
		CompOption::Vector opt;
		opt.reserve (5);

		CompOption::Value value0 = (int) event->xclient.window;
		opt.push_back (CompOption ( "window",CompOption::TypeInt));
		opt[0].set (value0);

		CompOption::Value value1 = (int) event->xclient.data.l[0];

		opt.push_back (CompOption ( "x",CompOption::TypeInt));
		opt[1].set (value1);

		CompOption::Value value2 = (int) event->xclient.data.l[1];
		opt.push_back (CompOption ( "y",CompOption::TypeInt));
		opt[2].set (value2);

		CompOption::Value value3 = (int) event->xclient.data.l[2];
		opt.push_back (CompOption ( "viewport",CompOption::TypeInt));
		opt[3].set (value3);

		CompOption::Value value4 = (int) event->xclient.data.l[4];
		opt.push_back (CompOption ( "output",CompOption::TypeInt));
		opt[4].set (value4);

		initiateCommon (NULL, 0, opt, 
				(PutType) event->xclient.data.l[3]);
	    }
	}
	break;
    default:
	break;
    }
    screen->handleEvent (event);
}

/* This gets called whenever the window needs to be repainted. WindowPaintAttrib gives you some
 * attributes like brightness/saturation etc to play around with. GLMatrix is the window's
 * transformation matrix. the unsigned int is the mask, have a look at opengl.h on what you can do
 * with it */
bool
PutWindow::glPaint (const GLWindowPaintAttrib &attrib, 
		    const GLMatrix &transform, 
		    const CompRegion &region, 
		    unsigned int mask) 
{
    bool status;
    GLMatrix wTransform = transform;

    PUT_WINDOW (window);

    if (pw->adjust)
    {

	wTransform.translate ( pw->tx, pw->ty, 0.0f);

	mask |= PAINT_WINDOW_TRANSFORMED_MASK;

    }

    status = gWindow->glPaint (attrib, wTransform, region, mask);

    return status;
}


/*
 * initiate action callback
 */
bool
PutScreen::initiateCommon (CompAction         *action,
			   CompAction::State  state,
			   CompOption::Vector &option,
			   PutType            type)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (option,  "window", 0);
    if (!xid)
	xid = screen->activeWindow ();

    w = screen->findWindow (xid);
    if (w)
    {
	CompScreen *s = screen;
	int        dx, dy;

	PUT_SCREEN (s);

	/* we don't want to do anything with override redirect windows */
	if (w->overrideRedirect ())
	    return false;

	/* we don't want to be moving the desktop and docks */
	if (w->type () & (CompWindowTypeDesktopMask |
			  CompWindowTypeDockMask))
	    return false;

	/* don't move windows without move action */
	if (!(w->actions () & CompWindowActionMoveMask))
	    return false;

	/* only allow movement of fullscreen windows to next output */
	if (type != PutNextOutput &&
	    (w->type () & CompWindowTypeFullscreenMask))
	{
	    return false;
	}

	/*
	 * handle the put types
	 */
	if (!getDistance (w, type, option, &dx, &dy))
	    return false;

	/* don't do anything if there is nothing to do */
	if (!dx && !dy)
	    return true;

	if (!ps->grabIndex)
	{
	    /* this will keep put from working while something
	       else has a screen grab */
	    if (s->otherGrabExist ("put", 0))
		return false;

	    /* we are ok, so grab the screen */
	    ps->grabIndex = s->pushGrab (s->invisibleCursor (), "put");
	}

	if (ps->grabIndex)
	{
	    PUT_WINDOW (w);

	    ps->lastWindow = w->id ();

	    /* save the windows position in the saveMask
	     * this is used when unmaximizing the window
	     */
	    if (w->saveMask () & CWX)
		w->saveWc ().x += dx;

	    if (w->saveMask () & CWY)
		w->saveWc ().y += dy;

	    /* Make sure everyting starts out at the windows
	       current position */
	    pw->lastX = w->x () + pw->tx;
	    pw->lastY = w->y () + pw->ty;

	    pw->targetX = pw->lastX + dx;
	    pw->targetY = pw->lastY + dy;

	    /* mark for animation */
	    pw->adjust = true;
	    ps->moreAdjust = true;

	    /* cause repainting */
	    pw->cWindow->addDamage ();
	}
    }

    /* tell event.c handleEvent to not call XAllowEvents */
    return false;
}

PutType
PutScreen::typeFromString (const CompString &type)
{
    if (type.compare("absolute") == 0)
	return PutAbsolute;
    else if (type.compare( "relative") == 0)
	return PutRelative;
    else if (type.compare( "pointer") == 0)
	return PutPointer;
    else if (type.compare( "viewport") == 0)
	return (PutType) PutViewport;
    else if (type.compare( "viewportleft") == 0)
	return PutViewportLeft;
    else if (type.compare( "viewportright") == 0)
	return PutViewportRight;
    else if (type.compare( "viewportup") == 0)
	return PutViewportUp;
    else if (type.compare( "viewportdown") == 0)
	return PutViewportDown;
    else if (type.compare( "nextoutput") == 0)
	return PutNextOutput;
    else if (type.compare( "restore") == 0)
	return PutRestore;
    else if (type.compare( "bottomleft") == 0)
	return PutBottomLeft;
    else if (type.compare( "emptybottomleft") == 0)
	return PutEmptyBottomLeft;
    else if (type.compare( "left") == 0)
	return PutLeft;
    else if (type.compare( "emptyleft") == 0)
	return PutEmptyLeft;
    else if (type.compare( "topleft") == 0)
	return PutTopLeft;
    else if (type.compare( "emptytopleft") == 0)
	return PutEmptyTopLeft;
    else if (type.compare( "top") == 0)
	return PutTop;
    else if (type.compare( "emptytop") == 0)
	return PutEmptyTop;
    else if (type.compare( "topright") == 0)
	return PutTopRight;
    else if (type.compare( "emptytopright") == 0)
	return PutEmptyTopRight;
    else if (type.compare( "right") == 0)
	return PutRight;
    else if (type.compare( "emptyright") == 0)
	return PutEmptyRight;
    else if (type.compare( "bottomright") == 0)
	return PutBottomRight;
    else if (type.compare( "emptybottomright") == 0)
	return PutEmptyBottomRight;
    else if (type.compare( "bottom") == 0)
	return PutBottom;
    else if (type.compare( "emptybottom") == 0)
	return PutEmptyBottom;
    else if (type.compare( "center") == 0)
	return PutCenter;
    else if (type.compare( "emptycenter") == 0)
	return PutEmptyCenter;
    else
	return PutUnknown;
}


bool
PutScreen::initiate (CompAction         *action,
		     CompAction::State  state,
		     CompOption::Vector &option)
{
    PutType type = PutUnknown;
    CompString    typeString;

    typeString = CompOption::getStringOptionNamed (option, "type", 0);
    if (!typeString.empty())
    	type = typeFromString (typeString);

/*    if (type == (PutType) PutViewport)
	return toViewport (action, state, option);
    else*/
    return initiateCommon (action, state, option,type);
}


PutScreen::PutScreen (CompScreen *screen) :
    PrivateHandler <PutScreen, CompScreen> (screen), 
    PutOptions (putVTable->getMetadata ()),
    screen (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    lastWindow (None),
    lastType (PutUnknown),
    moreAdjust (false),
    grabIndex (0)
{
    ScreenInterface::setHandler (screen); 
    CompositeScreenInterface::setHandler (cScreen); 
    GLScreenInterface::setHandler (gScreen); 

    compizPutWindowAtom = XInternAtom(screen->dpy (),
				      "_COMPIZ_PUT_WINDOW", 0);

#define setAction(action, type) \
    optionSet##action##Initiate (boost::bind (&PutScreen::initiateCommon, \
					      this, _1,_2,_3,type))

    /* Keys */
    setAction(PutCenterKey,PutCenter);
    setAction(PutEmptyCenterKey,PutEmptyCenter);
    setAction(PutLeftKey,PutLeft);
    setAction(PutEmptyLeftKey,PutEmptyLeft);
    setAction(PutRightKey,PutRight);
    setAction(PutEmptyRightKey,PutEmptyRight);
    setAction(PutTopKey,PutTop);
    setAction(PutEmptyTopKey,PutEmptyTop);
    setAction(PutBottomKey,PutBottom);
    setAction(PutEmptyBottomKey,PutEmptyBottom);
    setAction(PutTopleftKey,PutTopLeft);
    setAction(PutEmptyTopleftKey,PutEmptyTopLeft);
    setAction(PutToprightKey,PutTopRight);
    setAction(PutEmptyToprightKey,PutEmptyTopRight);
    setAction(PutBottomleftKey,PutBottomLeft);
    setAction(PutEmptyBottomleftKey,PutEmptyBottomLeft);
    setAction(PutBottomrightKey,PutBottomRight);
    setAction(PutEmptyBottomrightKey,PutEmptyBottomRight);

    /* Buttons */

    setAction(PutCenterButton,PutCenter);
    setAction(PutEmptyCenterButton,PutEmptyCenter);
    setAction(PutLeftButton,PutLeft);
    setAction(PutEmptyLeftButton,PutEmptyLeft);
    setAction(PutRightButton,PutRight);
    setAction(PutEmptyRightButton,PutEmptyRight);
    setAction(PutTopButton,PutTop);
    setAction(PutEmptyTopButton,PutEmptyTop);
    setAction(PutBottomButton,PutBottom);
    setAction(PutEmptyBottomButton,PutEmptyBottom);
    setAction(PutTopleftButton,PutTopLeft);
    setAction(PutEmptyTopleftButton,PutEmptyTopLeft);
    setAction(PutToprightButton,PutTopRight);
    setAction(PutEmptyToprightButton,PutEmptyTopRight);
    setAction(PutBottomleftButton,PutBottomLeft);
    setAction(PutEmptyBottomleftButton,PutEmptyBottomLeft);
    setAction(PutBottomrightButton,PutBottomRight);
    setAction(PutEmptyBottomrightButton,PutEmptyBottomRight);
}

PutScreen::~PutScreen () { }

PutWindow::PutWindow (CompWindow *window) :
    PrivateHandler <PutWindow, CompWindow> (window), 
    window (window), 
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window)),
    xVelocity (0),
    yVelocity (0),
    tx (0),
    ty (0),
    lastX (window->serverX ()),
    lastY (window->serverY ()),
    adjust (false)
{
    WindowInterface::setHandler (window); // Sets the window function hook handler
    CompositeWindowInterface::setHandler (cWindow); // Ditto for cWindow
    GLWindowInterface::setHandler (gWindow); // Ditto for gWindow
}

PutWindow::~PutWindow () { }

bool
PutPluginVTable::init ()
{
    std::cout << "Plugin loaded" << std::endl;
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	 return false;
    if (!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI))
	 return false;
    if (!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	 return false;
    return true;
}
