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
 * groupWindowInRegion
 *
 */
bool
GroupWindow::groupWindowInRegion (Region src,
				  float  precision)
{
    Region buf;
    int    i;
    int    area = 0;
    BOX    *box;

    buf = XCreateRegion ();
    if (!buf)
	return false;

    XIntersectRegion (window->region ().handle (), src, buf);

    /* buf area */
    for (i = 0; i < buf->numRects; i++)
    {
	box = &buf->rects[i];
	area += (box->x2 - box->x1) * (box->y2 - box->y1); /* width * height */
    }

    XDestroyRegion (buf);

    if (area >= WIN_WIDTH (window) * WIN_HEIGHT (window) * precision)
    {
	XSubtractRegion (src, window->region ().handle (), src);
	return TRUE;
    }

    return FALSE;
}

/*
 * groupFindGroupInWindows
 *
 */
bool
GroupScreen::groupFindGroupInWindows (GroupSelection *group,
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
CompWindowList
GroupScreen::groupFindWindowsInRegion (Region     reg)
{
    float      	   precision = optionGetSelectPrecision () / 100.0f;
    CompWindowList ret;
    int		   count;
    CompWindowList::reverse_iterator rit = screen->windows ().rbegin ();

    while (rit != screen->windows ().rend ())
    {
	CompWindow *w = *rit;
	GROUP_WINDOW (w);

	if (gw->groupIsGroupWindow () &&
	    gw->groupWindowInRegion (reg, precision))
	{
	    if (gw->mGroup && groupFindGroupInWindows (gw->mGroup, ret))
		continue;

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
GroupWindow::groupDeleteSelectionWindow ()
{
    GROUP_SCREEN (screen);
	
    if (gs->mTmpSel.mNWins > 0 && gs->mTmpSel.mWindows)
    {
	CompWindow **buf = gs->mTmpSel.mWindows;
	int        counter = 0;
	int        i;

	gs->mTmpSel.mWindows = (CompWindow **) calloc (gs->mTmpSel.mNWins - 1,
				     sizeof (CompWindow *));

	for (i = 0; i < gs->mTmpSel.mNWins; i++)
	{
	    if (buf[i]->id () == window->id ())
		continue;

	    gs->mTmpSel.mWindows[counter++] = buf[i];
	}

	gs->mTmpSel.mNWins = counter;
	free (buf);
    }

    mInSelection = FALSE;
}

/*
 * groupAddWindowToSelection
 *
 */
void
GroupWindow::addWindowToSelection ()
{
    GROUP_SCREEN (screen);

    gs->mTmpSel.mWindows = (CompWindow **) realloc (gs->mTmpSel.mWindows,
						   sizeof (CompWindow *) *
						   (gs->mTmpSel.mNWins + 1));

    gs->mTmpSel.mWindows[gs->mTmpSel.mNWins] = window;
    gs->mTmpSel.mNWins++;

    mInSelection = true;
}

/*
 * groupSelectWindow
 *
 */
void
GroupWindow::groupSelectWindow ()
{
    GROUP_SCREEN (screen);

    /* filter out windows we don't want to be groupable */
    if (!groupIsGroupWindow ())
	return;

    if (mInSelection)
    {
	if (mGroup)
	{
	    /* unselect group */
	    GroupSelection *group = mGroup;
	    CompWindow     **buf = gs->mTmpSel.mWindows;
	    int            i, counter = 0;

	    /* Faster than doing groupDeleteSelectionWindow
	       for each window in this group. */
	    gs->mTmpSel.mWindows = (CompWindow **)
				     calloc (gs->mTmpSel.mNWins - mGroup->mNWins,
					     sizeof (CompWindow *));

	    for (i = 0; i < gs->mTmpSel.mNWins; i++)
	    {
		CompWindow *cw = buf[i];
		GROUP_WINDOW (cw);

		if (gw->mGroup == group)
		{
		    gw->mInSelection = FALSE;
		    cWindow->addDamage ();
		    continue;
		}

		gs->mTmpSel.mWindows[counter++] = buf[i];
	    }
	    gs->mTmpSel.mNWins = counter;
	    free (buf);
	}
	else
	{
	    /* unselect single window */
	    groupDeleteSelectionWindow ();
	    cWindow->addDamage ();
	}
    }
    else
    {
	if (mGroup)
	{
	    /* select group */
	    int i;
	    for (i = 0; i < mGroup->mNWins; i++)
	    {
		CompWindow *cw = mGroup->mWindows[i];

		GROUP_WINDOW (cw);

		gw->addWindowToSelection ();
		gw->cWindow->addDamage ();
	    }
	}
	else
	{
	    /* select single window */
	    addWindowToSelection ();
	    cWindow->addDamage ();
	}
    }
}

/*
 * groupSelectSingle
 *
 */
bool
GroupScreen::groupSelectSingle (CompAction         *action,
				CompAction::State  state,
				CompOption::Vector options)
{
    Window     xid;
    CompWindow *w;

    xid = CompOption::getIntOptionNamed (options, "window", 0);
    w   = screen->findWindow (xid);
    if (w)
	GroupWindow::get (w)->groupSelectWindow ();

    return true;
}

/*
 * groupSelect
 *
 */
bool
GroupScreen::groupSelect (CompAction         *action,
			  CompAction::State  state,
			  CompOption::Vector options)
{
    if (mGrabState == ScreenGrabNone)
    {
	groupGrabScreen (ScreenGrabSelect);

	if (state & CompAction::StateInitButton)
	{
	    action->setState (state | CompAction::StateTermButton);
	}

	mX1 = mX2 = pointerX;
	mY1 = mY2 = pointerY;
    }
    
    return true;
}

/*
 * groupSelectTerminate
 *
 */
bool
GroupScreen::groupSelectTerminate (CompAction         *action,
				   CompAction::State  state,
				   CompOption::Vector options)
{
    if (mGrabState == ScreenGrabSelect)
    {
        groupGrabScreen (ScreenGrabNone);

        if (mX1 != mX2 && mY1 != mY2)
        {		
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

	    cScreen->damageRegion (reg);

	    ws = groupFindWindowsInRegion (reg.handle ());
	    if (ws.size ())
	    {
		/* select windows */
		foreach (CompWindow *w, ws)
		    GroupWindow::get (w)->groupSelectWindow ();

		if (optionGetAutoGroup ())
		{
		    CompOption::Vector dummy;
		    groupGroupWindows (NULL, 0, dummy);
		}
	    }
        }
    }

    action->setState (action->state () & ~(CompAction::StateTermButton | CompAction::StateTermKey));

    return false;
}

/*
 * groupDamageSelectionRect
 *
 */
void
GroupScreen::groupDamageSelectionRect (int xRoot,
				       int yRoot)
{
    CompRegion reg (mX1 - 5, mY1 - 5, mX2 - mX1 + 5,
				      mY2 - mY1 + 5);

    cScreen->damageRegion (reg);

    mX2 = xRoot;
    mY2 = yRoot;

    reg = CompRegion (MIN (mX1, mX2) - 5,
		      MIN (mY1, mY2) - 5,
		      MAX (mX1, mX2) + 5 - 
		      MIN (mX1, mX2) - 5,
		      MAX (mY1, mY2) + 5 -
		      MIN (mY1, mY2) - 5);

    cScreen->damageRegion (reg);
}
