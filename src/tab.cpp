/**
 *
 * Compiz group plugin
 *
 * tab.cpp
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
 * Group::tab
 *
 */
void
Group::tab (CompWindow *main)
{
    int             width, height;
    int             space, thumbSize;

    GROUP_WINDOW (main);
    GROUP_SCREEN (screen);

    if (tabBar)
	return;

    if (!screen->XShape ())
    {
	compLogMessage ("group", CompLogLevelError,
			"No X shape extension! Tabbing disabled.");
	return;
    }

    tabBar = new TabBar (this, main);

    if (!tabBar)
	return;

    tabBar->createIPW ();

    tabbingState = TabBar::NoTabbing;

    /* Slot is initialized after new TabBar (main); */

    tabBar->changeTab (gw->tab, TabBar::RotateUncertain);

    tabBar->recalcPos (WIN_CENTER_X (main), WIN_X (main), WIN_X (main) +
		       WIN_WIDTH (main));

    width = tabBar->region.boundingRect ().x2 () -
	    tabBar->region.boundingRect ().x1 ();
    height = tabBar->region.boundingRect ().y2 () -
	     tabBar->region.boundingRect ().y1 ();

    tabBar->textLayer = new TextLayer ();

    if (tabBar->textLayer)
    {
	tabBar->textLayer->state = CairoLayer::PaintOff;
	tabBar->textLayer->animationTime = 0;
	tabBar->renderWindowTitle ();
	tabBar->textLayer->animationTime =
				     gs->optionGetChangeAnimationTime () * 1000;
	tabBar->textLayer->state = CairoLayer::PaintFadeIn;
    }

    /* we need a buffer for DnD here */
    space = gs->optionGetThumbSpace ();
    thumbSize = gs->optionGetThumbSize ();

    tabBar->bgLayer = CairoLayer::createCairoLayer (width + space + thumbSize,
    						    height);

    if (tabBar->bgLayer)
    {
	tabBar->bgLayer->state = Layer::PaintOn;
	tabBar->bgLayer->animationTime = 0;
	tabBar->renderTabBarBackground ();
    }

    width = tabBar->topTab->region.boundingRect ().x2 () -
	    tabBar->topTab->region.boundingRect ().x1 ();
    height = tabBar->topTab->region.boundingRect ().y2 () -
	     tabBar->topTab->region.boundingRect ().y1 ();

    tabBar->selectionLayer = CairoLayer::createCairoLayer (width, height);
    if (tabBar->selectionLayer)
    {
	tabBar->selectionLayer->state = Layer::PaintOn;
	tabBar->selectionLayer->animationTime = 0;
	tabBar->renderTopTabHighlight ();
    }

    if (!HAS_TOP_WIN (this))
	return;

    foreach (Tab *tab, tabBar->tabs)
    {
	CompWindow *cw = tab->window;

	GROUP_WINDOW (cw);

	if (gw->animateState & (IS_ANIMATED | FINISHED_ANIMATION))
	{
	    cw->move (gw->destination.x () - WIN_X (cw),
		      gw->destination.y () - WIN_Y (cw),
			FALSE);
	}

	/* center the window to the main window */
	gw->destination.setX (WIN_CENTER_X (main) - (WIN_WIDTH (cw) / 2));
	gw->destination.setY (WIN_CENTER_Y (main) - (WIN_HEIGHT (cw) / 2));

	/* Distance from destination. */
	gw->mainTabOffset.setX (WIN_X (cw) - gw->destination.x ());
	gw->mainTabOffset.setY (WIN_Y (cw) - gw->destination.y ());

	if (gw->tx || gw->ty)
	{
	    gw->tx -= (WIN_X (cw) - gw->orgPos.x ());
	    gw->ty -= (WIN_Y (cw) - gw->orgPos.y ());
	}

	gw->orgPos.setX (WIN_X (cw));
	gw->orgPos.setY (WIN_Y (cw));

	gw->animateState = IS_ANIMATED;
	gw->xVelocity = gw->yVelocity = 0.0f;
    }

    //tabbingState = TabBar::NoTabbing;
    startTabbingAnimation (true);
}

/*
 * groupUntabGroup
 *
 */
void
Group::untab ()
{
    int             oldX, oldY;
    CompWindow      *fprevTopTab;
    
    if (!tabBar)
	return;

    if (!HAS_TOP_WIN (this))
	return;

    GROUP_SCREEN (screen);

    if (tabBar->prevTopTab)
	fprevTopTab = PREV_TOP_TAB (this);
    else
    {
	/* If prevTopTab isn't set, we have no choice but using topTab.
	   It happens when there is still animation, which
	   means the tab wasn't changed anyway. */
	fprevTopTab = TOP_TAB (this);
    }

    tabBar->lastTopTab = TOP_TAB (this);
    tabBar->topTab = NULL;

    foreach (Tab *tab, tabBar->tabs)
    {
	CompWindow *cw = tab->window;

	GROUP_WINDOW (cw);

	if (gw->animateState & (IS_ANIMATED | FINISHED_ANIMATION))
	{
	    gs->queued = TRUE;
	    cw->move (gw->destination.x () - WIN_X (cw),
		      gw->destination.y () - WIN_Y (cw),
		      false);
	    gs->queued = FALSE;
	}
	
	gw->setVisibility (true);

	/* save the old original position - we might need it
	   if constraining fails */
	oldX = gw->orgPos.x ();
	oldY = gw->orgPos.y ();

	gw->orgPos.setX (WIN_CENTER_X (fprevTopTab) - WIN_WIDTH (cw) / 2);
	gw->orgPos.setY (WIN_CENTER_Y (fprevTopTab) - WIN_HEIGHT (cw) / 2);

	gw->destination.setX (gw->orgPos.x () + gw->mainTabOffset.x ());
	gw->destination.setY (gw->orgPos.y () + gw->mainTabOffset.y ());

	if (gw->tx || gw->ty)
	{
	    gw->tx -= (gw->orgPos.x () - oldX);
	    gw->ty -= (gw->orgPos.y () - oldY);
	}

	gw->mainTabOffset.setX (oldX);
	gw->mainTabOffset.setY (oldY);

	gw->animateState = IS_ANIMATED;
	gw->xVelocity = gw->yVelocity = 0.0f;
    }

    tabbingState = TabBar::NoTabbing;
    startTabbingAnimation (false);

    delete tabBar;
    tabBar = NULL;

    changeAnimationTime = 0;
    changeState = TabBar::NoTabChange;

    gs->cScreen->damageScreen ();
}

/*
 * GroupScreen::dragHoverTimeout
 *
 * Description:
 * Activates a window after a certain time a slot has been dragged over it.
 *
 */
bool
GroupScreen::dragHoverTimeout (CompWindow *w)
{
    if (!w)
	return FALSE;

    GROUP_WINDOW (w);

    if (optionGetBarAnimations ())
    {
	TabBar *bar = gw->group->tabBar;

	bar->bgAnimation = TabBar::AnimationPulse;
	bar->bgAnimationTime = optionGetPulseTime () * 1000;
    }

    w->activate ();
    
    return FALSE;
}

/* TabList::getNextTab
 *
 * Description: This function will not set ret to NULL unless the tab passed in
 * curr was not found in the list. ret is set to either the next tab, in
 * which case the function will return true, else it will set ret to the first
 * tab and return false. Use the return check to see if there is a tab, after
 * the current one
 */

bool
TabList::getNextTab (Tab *curr, Tab *&ret)
{
    iterator it = std::find (begin (), end (), curr);
    
    if (it == end ()) // bogus case
    {
        ret = NULL;
	return false;
    }
	
    it++;
    
    /* Return the first tab if there is no next tab */
    
    if (it == end ())
    {
    	ret = front ();
	return false;
    }

    ret = *it;

    return true;
}

/* TabList::getNextTab
 *
 * Description: This function will not set ret to NULL unless the tab passed in
 * curr was not found in the list. ret is set to either the previous tab, in
 * which case the function will return true, else it will set ret to the last
 * tab and return false. Use the return check to see if there is a tab, before
 * the current one
 */

bool
TabList::getPrevTab (Tab *curr, Tab *&ret)
{
    iterator it = std::find (begin (), end (), curr);
    
    if (it == end ())
    {
        ret = NULL;
        return false;
    }
        
    if (it == begin ())
    {
        ret = back ();
    	return false;
    }
    	
    it--;
    
    ret = *it;
    
    return true;
}
