/**
 *
 * Compiz group plugin
 *
 * group.cpp
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
 * Group::minimizeWindows
 *
 * Description: Minimizes all windows in a group. 'top' refers to the window
 * which was just minimized - i.e don't minimize it again
 *
 */


void
Group::minimizeWindows (CompWindow *top,
			bool	   minimize)
{
    foreach (CompWindow *w, windows)
    {
	if (w->id () == top->id ())
	    continue;

	if (minimize)
	    w->minimize ();
	else
	    w->unminimize ();
    }
}

/*
 * Group::shadeWindows
 *
 * Description: Shades all windows in a group. Works similar to above
 *
 */


void
Group::shadeWindows(CompWindow     *top,
		    bool           shade)
{
    unsigned int state;

    foreach (CompWindow *w, windows)
    {
	if (w->id () == top->id ())
	    continue;

	if (shade)
	    state = w->state () | CompWindowStateShadedMask;
	else
	    state = w->state () & ~CompWindowStateShadedMask;

	w->changeState (state);
	w->updateAttributes (CompStackingUpdateModeNone);
    }
}

/*
 * Group::raiseWindows
 *
 * Description: Raises all windows in a group. Works similar to the above
 *
 */

void
Group::raiseWindows (CompWindow *top)
{
    CompWindowList stack = windows;

    if (stack.size () == 1)
	return;

    stack.remove (top);

    while (!stack.empty ())
    {
	CompWindow *w = stack.front ();
	w->restackBelow (top);

	stack.pop_front ();
    }
}

Group *
Group::create (unsigned int initialIdent)
{
    GROUP_SCREEN (screen);

    Group *ret = new Group (initialIdent);

    gs->groups.push_back (ret);

    return ret;
}

Group::Group (unsigned int initialIdent) :
    identifier (initialIdent),
    tabBar (NULL),
    ungroupState (UngroupNone),
    grabWindow (None),
    grabMask (0)
{
    GROUP_SCREEN (screen);

    /* glow color */
    color[0] = (int)(rand () / (((double)RAND_MAX + 1) / 0xffff));
    color[1] = (int)(rand () / (((double)RAND_MAX + 1) / 0xffff));
    color[2] = (int)(rand () / (((double)RAND_MAX + 1) / 0xffff));
    color[3] = 0xffff;

    if (!identifier)
    {
        /* we got no valid group Id passed, so find out a new valid
           unique one */
        bool           invalidID = false;

        identifier = !gs->groups.empty () ? gs->groups.front ()->identifier : 0;
        do
        {
	    invalidID = false;
	    foreach (Group *tg, gs->groups)
	    {
	        if (tg->identifier == identifier)
	        {
		    invalidID = true;

		    identifier++;
		    break;
	        }
	    }
        }
        while (invalidID);
    }

    windows.clear ();
}

/*
 * Group::destroy
 *
 * Description: This is a wrapper around the destructor for group. If the group
 * is tabbed, set up the untabbing animation and then delete the group. If the
 * group must be deleted straight away, immediate should be true
 */

void
Group::destroy (bool immediate)
{
    GROUP_SCREEN (screen);

    if (windows.size ())
    {
	if (tabBar && !immediate)
	{
	    /* set up untabbing animation and delete the group
	       at the end of the animation */

	    untab ();
	    ungroupState = UngroupAll;
	    
	    return;
	}
	else
	{
	    foreach (CompWindow *cw, windows)
	    {
	        if (!cw)
	            continue;
	    
	        GROUP_WINDOW (cw);

	        gw->cWindow->damageOutputExtents ();
	        gw->group = NULL;
	        gw->window->updateWindowOutputExtents ();
	        gw->updateProperty ();

	        if (gs->optionGetAutotabCreate () && gw->isGroupable ())
	        {
		    Selection sel;
		    Group     *group;
		
		    sel.push_back (cw);
		
		    group = sel.toGroup ();
		    if (group)
		        group->tab (cw);
	        }
	    }

	    windows.clear ();
	}
    }
    else if (tabBar)
	delete tabBar;
    
    if (this == gs->lastHoveredGroup)
	gs->lastHoveredGroup = NULL;
    if (this == gs->lastRestackedGroup)
	gs->lastRestackedGroup = NULL;

    gs->groups.remove (this);
    
    delete this;
}

Group::~Group ()
{

}

void
Group::addWindow (CompWindow *w)
{
    CompWindow *topTabWin = NULL;
    
    foreach (CompWindow *cw, windows)
    	if (w == cw)
	    return;

    windows.push_back (w);

    GROUP_WINDOW (w);

    gw->group = this;

    w->updateWindowOutputExtents ();
    gw->updateProperty ();

    if (windows.size () == 2)
    {
        /* first window in the group got its glow, too */
        windows.front ()->updateWindowOutputExtents ();
    }

    if (tabBar)
    {

        if (HAS_TOP_WIN (this))
	    topTabWin = TOP_TAB (this);
        else if (HAS_PREV_TOP_WIN (this))
        {
	    topTabWin = PREV_TOP_TAB (this);
	    tabBar->topTab = tabBar->prevTopTab;
	    tabBar->prevTopTab = NULL;
        }

        if (topTabWin)
        {
	    if (!gw->tab)
	        tabBar->createTab (w);

	    gw->destination.setX (WIN_CENTER_X (topTabWin) -
	    			 (WIN_WIDTH (w) / 2));
	    gw->destination.setY (WIN_CENTER_Y (topTabWin) -
	                        (WIN_HEIGHT (w) / 2));
	                        
	    gw->mainTabOffset.setX (WIN_X (w) - gw->destination.x ());
	    gw->mainTabOffset.setY (WIN_Y (w) - gw->destination.y ());
	    gw->orgPos.setX (WIN_X (w));
	    gw->orgPos.setY (WIN_Y (w));

	    gw->xVelocity = gw->yVelocity = 0.0f;

	    gw->animateState = IS_ANIMATED;

	    startTabbingAnimation (true);

	    gw->cWindow->addDamage ();
        }
    }
}
