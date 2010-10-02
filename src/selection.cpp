/**
 *
 * Compiz group plugin
 *
 * selection.c
 *
 * Copyright : (C) 2006-2007 by Patrick Niklaus, Roi Cohen, Danny Baumann
 * Authors: Patrick Niklaus <patrick.niklaus@googlemail.com>
 *          Roi Cohen       <roico.beryl@gmail.com>
 *          Danny Baumann   <maniac@opencompositing.org>
 *
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
 **/

#include "group.h"

/*
 * windowInRegion
 *
 */
bool
GroupWindow::windowInRegion (CompRegion src,
				  float  precision)
{
    int    i;
    int    area = 0;
    CompRegion buf;

    buf = window->region ().intersected (src);

    /* buf area */
    for (i = 0; i < buf.numRects (); i++)
    {
	CompRect box = buf.rects ().at (i);
	area += (box.x2 () - box.x1 ()) * (box.y2 () - box.y1 ()); /* width * height */
    }

    if (area >= WIN_WIDTH (window) * WIN_HEIGHT (window) * precision)
    {
	src = window->region ().subtracted (src);
	return true;
    }

    return false;
}

/*
 * groupFindGroupInWindows
 *
 */
static inline bool
groupFindGroupInWindows (GroupSelection *group,
			 CompWindowList &windows)
{
    foreach (CompWindow *cw, windows)
    {
	GROUP_WINDOW (cw);

	if (gw->mGroup == group)
	    return true;
    }

    return false;
}

/*
 * groupFindWindowsInRegion
 *
 */
static CompWindowList
groupFindWindowsInRegion (CompRegion     reg)
{
    float      	   precision = GroupScreen::get (screen)->optionGetSelectPrecision () / 100.0f;
    Selection      ret;
    int		   count;
    CompWindowList::reverse_iterator rit = screen->windows ().rbegin ();

    while (rit != screen->windows ().rend ())
    {
	CompWindow *w = *rit;
	GROUP_WINDOW (w);

	if (gw->isGroupWindow () &&
	    gw->windowInRegion (reg, precision))
	{
	    if (gw->mGroup && groupFindGroupInWindows (gw->mGroup, ret))
	    {
		rit++;
		continue;
	    }

	    ret.push_back (w);

	    count++;
	}
	
	rit++;
    }

    return ret;
}

/*
 * groupDeleteSelectionWindow
 *
 */
void
Selection::deselect (CompWindow *w)
{	
    if (size ())
    {
	remove (w);
    }

    GroupWindow::get (w)->mInSelection = false;
}

void
Selection::deselect (GroupSelection *group)
{
    /* unselect group */
    CompWindowList copy = (CompWindowList) *this;
    CompWindowList::iterator it = begin ();

    /* Faster than doing groupDeleteSelectionWindow
       for each window in this group. */
    resize (size () -
	    group->mWindows.size ());

    foreach (CompWindow *cw, copy)
    {
	GROUP_WINDOW (cw);

	if (gw->mGroup == group)
	{
	    gw->mInSelection = false;
	    gw->cWindow->addDamage ();
	    continue;
	}

	(*it) = cw;
	it++;
    }
}
/*
 * Selection::select
 *
 */
void
Selection::select (CompWindow *w)
{
    push_back (w);
    
    GroupWindow::get (w)->mInSelection = true;
    GroupWindow::get (w)->cWindow->addDamage ();
}

void
Selection::select (GroupSelection *g)
{
    foreach (CompWindow *cw, g->mWindows)
    {
	select (cw);
    }
}

/*
 * Selection::toGroup
 *
 */

void
Selection::toGroup ()
{
    GROUP_SCREEN (screen);
	
    CompRegion reg;
    CompRect   rect;
    CompWindowList ws;
    
    int x      = MIN (mX1, mX2) - 2;
    int y      = MIN (mY1, mY2) - 2;
    int width  = MAX (mX1, mX2) -
		  MIN (mX1, mX2) + 4;
    int height = MAX (mY1, mY2) -
		  MIN (mY1, mY2) + 4;

    rect = CompRect (x, y, width, height);
    reg = emptyRegion.united (rect);

    gs->cScreen->damageRegion (reg);

    ws = groupFindWindowsInRegion (reg);
    if (ws.size ())
    {
	/* select windows */
	foreach (CompWindow *w, ws)
	    checkWindow (w);

	if (gs->optionGetAutoGroup ())
	{
	    CompOption::Vector dummy;
	    gs->groupWindows (NULL, 0, dummy);
	}
    }
}

/*
 * Selection::checkWindow
 *
 */
void
Selection::checkWindow (CompWindow *w)
{
    GROUP_WINDOW (w);

    /* filter out windows we don't want to be groupable */
    if (!gw->isGroupWindow ())
	return;

    if (gw->mInSelection)
    {
	if (gw->mGroup)
	{
	    deselect (gw->mGroup);
	}
	else
	{
	    /* unselect single window */
	    deselect (w);
	}
    }
    else
    {
	if (gw->mGroup)
	{
	    /* select group */
	    select (gw->mGroup);
	}
	else
	{
	    /* select single window */
	    select (w);
	}
    }
}

/*
 * selectSingle
 *
 */
bool
GroupScreen::selectSingle (CompAction         *action,
				CompAction::State  state,
				CompOption::Vector options)
{
    Window     xid;
    CompWindow *w;

    xid = CompOption::getIntOptionNamed (options, "window", 0);
    w   = screen->findWindow (xid);
    if (w)
	mTmpSel.checkWindow (w);

    return true;
}

/*
 * select
 *
 */
bool
GroupScreen::select (CompAction         *action,
			  CompAction::State  state,
			  CompOption::Vector options)
{
    if (mGrabState == ScreenGrabNone)
    {
	grabScreen (ScreenGrabSelect);

	if (state & CompAction::StateInitButton)
	{
	    action->setState (state | CompAction::StateTermButton);
	}

	mTmpSel.mX1 = mTmpSel.mX2 = pointerX;
	mTmpSel.mY1 = mTmpSel.mY2 = pointerY;
    }
    
    return true;
}

/*
 * selectTerminate
 *
 */
bool
GroupScreen::selectTerminate (CompAction         *action,
				   CompAction::State  state,
				   CompOption::Vector options)
{
    if (mGrabState == ScreenGrabSelect)
    {
        grabScreen (ScreenGrabNone);

        if (mTmpSel.mX1 != mTmpSel.mX2 && mTmpSel.mY1 != mTmpSel.mY2)
        {
	    mTmpSel.toGroup ();
        }
    }

    action->setState (action->state () & ~(CompAction::StateTermButton | CompAction::StateTermKey));

    return false;
}

/*
 * Selection::damage
 *
 */
void
Selection::damage (int xRoot,
		   int yRoot)
{
    GROUP_SCREEN (screen);
	
    CompRegion reg (mX1 - 5, mY1 - 5, mX2 - mX1 + 5,
				      mY2 - mY1 + 5);

    gs->cScreen->damageRegion (reg);

    mX2 = xRoot;
    mY2 = yRoot;

    reg = CompRegion (MIN (mX1, mX2) - 5,
		      MIN (mY1, mY2) - 5,
		      MAX (mX1, mX2) + 5 - 
		      MIN (mX1, mX2) - 5,
		      MAX (mY1, mY2) + 5 -
		      MIN (mY1, mY2) - 5);

    gs->cScreen->damageRegion (reg);
}
