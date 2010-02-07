/**
 *
 * Compiz group plugin
 *
 * actions.cpp
 *
 * Copyright : (C) 2006-2009 by Patrick Niklaus, Roi Cohen, Danny Baumann,
 *				Sam Spilsbury
 * Authors: Patrick Niklaus <patrick.niklaus@googlemail.com>
 *          Roi Cohen       <roico.beryl@gmail.com>
 *          Danny Baumann   <maniac@opencompositing.org>
 *	    Sam Spilsbury   <smspillaz@gmail.com>
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
 * groupSelectSingle
 *
 */
bool
GroupScreen::selectSingle (CompAction         *action,
			   CompAction::State  state,
			   CompOption::Vector &options)
{
    Window     xid;
    CompWindow *w;

    xid = CompOption::getIntOptionNamed (options, "window", 0);
    w   = screen->findWindow (xid);
    /* Adds the window to the master selection */

    if (w)
	GroupWindow::get (w)->select ();

    return TRUE;
}

/*
 * groupSelect
 *
 */
bool
GroupScreen::select (CompAction         *action,
		     CompAction::State  state,
		     CompOption::Vector &options)
{
    Window     xid;
    CompWindow *w;

    xid = CompOption::getIntOptionNamed (options, "window", 0);
    w   = screen->findWindow (xid);
    if (w)
    {
	if (grabState == ScreenGrabNone)
	{
	    grabScreen (ScreenGrabSelect);

	    if (state & CompAction::StateInitKey)
		action->setState (action->state () | CompAction::StateTermKey);

	    if (state & CompAction::StateInitButton)
		action->setState (action->state () | CompAction::StateTermButton);

	    masterSelectionRect.setX (pointerX);
	    masterSelectionRect.setY (pointerY);
	    masterSelectionRect.setWidth (0);
	    masterSelectionRect.setHeight (0);
	}

	return TRUE;
    }

    return false;
}

/*
 * groupSelectTerminate
 *
 */
bool
GroupScreen::selectTerminate (CompAction         *action,
			      CompAction::State  state,
			      CompOption::Vector &options)
{
    if (grabState == ScreenGrabSelect)
    {
        grabScreen (ScreenGrabNone);

        if (masterSelectionRect.x1 () != masterSelectionRect.x2 () &&
	    masterSelectionRect.y1 () != masterSelectionRect.y2 ())
        {
	    CompWindowList tempWindowList;
	    Selection::Rect  &msr = masterSelectionRect;
	    Selection	   sel = msr.toSelection ();

	    CompRegion reg (MIN (msr.x1 (), msr.x2 ()) - 2,
			    MIN (msr.y1 (), msr.y2 ()) - 2,
			    (MAX (msr.x1 (), msr.x2 ()) -
	                     MIN (msr.x1 (), msr.x2 ()) + 4),
			    (MAX (msr.y1 (), msr.y2 ()) -
	                     MIN (msr.y1 (), msr.y2 ()) + 4));

	    cScreen->damageRegion (reg);

	    masterSelection.push_back (sel);
#if 0
		
	        ws = findWindowsInRegion (reg)
	        if (!ws.empty ())
	        {
		    /* select windows */
		    for (int i = 0; i < count; i++)
		        GroupWindow::get (groupSelectWindow (ws[i]);

		    if (optionGetAutoGroup ())
		        selection->toGroup ();

		    free (ws);
	        }
#endif
        }
    }


    action->setState (action->state () & ~(CompAction::StateTermKey |
					   CompAction::StateTermButton));

    return true;
}


bool
GroupScreen::groupWindows (CompAction         *action,
			   CompAction::State  state,
			   CompOption::Vector &options)
{
    if (!masterSelection.empty ())
    {
	masterSelection.toGroup ();
    }

    return true;
}

/*
 * GroupScreen::ungroupWindows
 *
 */
bool
GroupScreen::ungroupWindows (CompAction         *action,
			     CompAction::State  state,
			     CompOption::Vector &options)
{
    CompWindow *w = screen->findTopLevelWindow (
			CompOption::getIntOptionNamed (options, "window", 0));
    if (w)
    {
	GROUP_WINDOW (w);

	if (gw->group)
	{
	    gw->group->destroy (false);
	}
    }

    return true;
}

/*
 * groupRemoveWindow
 *
 */
bool
GroupScreen::removeWindow (CompAction         *action,
			   CompAction::State  state,
			   CompOption::Vector &options)
{
    CompWindow *w = screen->findWindow (CompOption::getIntOptionNamed (options,
								       "window",
									0));

    if (w)
    {
	GROUP_WINDOW (w);

	if (gw->group)
	{
	    gw->removeFromGroup ();
	}
    }

    return true;
}

/*
 * GroupScreen::closeWindows
 *
 */
bool
GroupScreen::closeWindows (CompAction         *action,
			   CompAction::State  state,
			   CompOption::Vector &options)
{
    CompWindow *w = screen->findWindow (CompOption::getIntOptionNamed (options,
								       "window",
									0));
    if (w)
    {
	GROUP_WINDOW (w);

	if (gw->group)
	{
	    foreach (CompWindow *w, gw->group->windows)
		w->close (screen->getCurrentTime ());
	}
    }

    return true;
}

/*
 * GroupScreen::changeColor
 *
 */
bool
GroupScreen::changeColor (CompAction         *action,
			  CompAction::State  state,
			  CompOption::Vector &options)
{
    Window     xid;
    CompWindow *w;

    xid = CompOption::getIntOptionNamed (options, "window", 0);
    w   = screen->findWindow (xid);
    if (w)
    {
	GROUP_WINDOW (w);

	if (gw->group)
	{
	    GLushort *color = gw->group->color;
	    float    factor = ((float)RAND_MAX + 1) / 0xffff;

	    color[0] = (int)(rand () / factor);
	    color[1] = (int)(rand () / factor);
	    color[2] = (int)(rand () / factor);

	    if (gw->group->tabBar && gw->group->tabBar->selectionLayer)
		gw->group->tabBar->selectionLayer->renderTopTabHighlight (
							     gw->group->tabBar);
	    cScreen->damageScreen ();
	}
    }

    return false;
}

/*
 * GroupScreen::setIgnore
 *
 */
bool
GroupScreen::setIgnore (CompAction         *action,
			CompAction::State  state,
			CompOption::Vector &options)
{
    ignoreMode = TRUE;

    if (state & CompAction::StateInitKey)
	action->setState (action->state () | CompAction::StateTermKey);

    return false;
}

bool
GroupScreen::unsetIgnore (CompAction         *action,
			  CompAction::State  state,
			  CompOption::Vector &options)
{
    ignoreMode = false;

    action->setState (action->state () & ~CompAction::StateTermKey);

    return false;
}


bool
GroupScreen::initTab (CompAction         *action,
		      CompAction::State  state,
		      CompOption::Vector &options)
{
    bool       allowUntab = TRUE;

    CompWindow *w = screen->findWindow (CompOption::getIntOptionNamed (options,
								       "window",
									0));
    if (!w)
	return true;

    GROUP_WINDOW (w);

    if (gw->inSelection)
    {
	groupWindows (action, state, options);
	/* If the window was selected, we don't want to
	   untab the group, because the user probably
	   wanted to tab the selected windows. */
	allowUntab = false;
    }

    if (!gw->group)
	return TRUE;

    if (!gw->group->tabBar)
    {
	gw->group->tab (w);
    }
    else if (allowUntab)
	gw->group->untab ();

    cScreen->damageScreen ();

    return TRUE;
}

/*
 * GroupScreen::changeTabLeft
 *
 */
bool
GroupScreen::changeTabLeft (CompAction         *action,
			    CompAction::State  state,
			    CompOption::Vector &options)
{
    CompWindow *topTab;
    Tab	       *prev;

    CompWindow *w = screen->findWindow (CompOption::getIntOptionNamed (options,
								       "window",
									0));

    topTab = w;
    if (!topTab)
	return TRUE;

    GROUP_WINDOW (topTab);

    if (!gw->tab || !gw->group || !gw->group->tabBar)
	return TRUE;

    if (gw->group->tabBar->nextTopTab)
	topTab = NEXT_TOP_TAB (gw->group);
    else if (gw->group->tabBar->topTab)
    {
	/* If there are no tabbing animations,
	   topTab is never NULL. */
	topTab = TOP_TAB (gw->group);
    }

    gw = GroupWindow::get (topTab);
    
    gw->group->tabBar->getPrevTab (gw->group->tabBar->topTab, prev);
    
    return gw->group->tabBar->changeTab (prev, TabBar::RotateLeft);
}

/*
 * GroupScreen::changeTabRight
 *
 */
bool
GroupScreen::changeTabRight (CompAction         *action,
		     	     CompAction::State  state,
		     	     CompOption::Vector &options)
{
    CompWindow *topTab;
    Tab	       *next;

    CompWindow *w = screen->findWindow (CompOption::getIntOptionNamed (options,
								       "window",
									0));

    topTab = w;
    if (!w)
	return TRUE;

    GROUP_WINDOW (w);

    if (!gw->tab || !gw->group || !gw->group->tabBar)
	return TRUE;

    if (gw->group->tabBar->nextTopTab)
	topTab = NEXT_TOP_TAB (gw->group);
    else if (gw->group->tabBar->topTab)
    {
	/* If there are no tabbing animations,
	   topTab is never NULL. */
	topTab = TOP_TAB (gw->group);
    }

    gw = GroupWindow::get (topTab);

    gw->group->tabBar->getNextTab (gw->group->tabBar->topTab, next);
    
    return gw->group->tabBar->changeTab (next, TabBar::RotateRight);
}
