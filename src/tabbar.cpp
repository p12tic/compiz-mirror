/**
 *
 * Compiz group plugin
 *
 * tabbar.cpp
 *
 * Copyright : (C) 2006-2007 by Patrick Niklaus, Roi Cohen, Danny Baumann
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

TabBar::TabBar (Group *g, CompWindow *main) :
    hoveredSlot (NULL),
    textSlot (NULL),
    textLayer (NULL),
    bgLayer (NULL),
    selectionLayer (NULL),
    group (g),
    bgAnimationTime (0),
    bgAnimation (AnimationNone),
    state (Layer::PaintOff),
    animationTime (0),
    oldWidth (0),
    inputPrevention (0),
    ipwMapped (false),
    leftSpringX (0),
    rightSpringX (0),
    leftSpeed (0),
    rightSpeed (0),
    leftMsSinceLastMove (0),
    rightMsSinceLastMove (0)
{
    GROUP_SCREEN (screen);

    if (g->tabBar)
	delete g->tabBar;

    g->tabBar = this;

    foreach (CompWindow *w, group->windows)
    {
	Tab *t = new Tab (group, w);
    }

    recalcPos (WIN_CENTER_X (main), WIN_X (main), WIN_X (main) +
						      WIN_WIDTH (main));

    timeoutHandle.setCallback (boost::bind (&GroupScreen::tabBarTimeout,
					    GroupScreen::get (screen), this));

    timeoutHandle.setTimes (gs->optionGetVisibilityTime () * 1000,
			    gs->optionGetVisibilityTime () * 1200);

    /* XXX: needs a place in optionChange */

    timeoutHandle.stop ();
}


TabBar::~TabBar ()
{
    destroyIPW ();
    timeoutHandle.stop ();
}

/*
 * TabBar::createIPW
 *
 */
void
TabBar::createIPW ()
{
    if (!inputPrevention)
    {
	XSetWindowAttributes attrib;

	attrib.override_redirect = true;

	inputPrevention = XCreateWindow (screen->dpy (), screen->root (), -100, -100, 1, 1, 0,
			  CopyFromParent, InputOnly, CopyFromParent, CWOverrideRedirect, &attrib);

	XMapWindow (screen->dpy (), inputPrevention);

	XWindowChanges xwc;

	xwc.x = 0;
	xwc.y = 0;
	xwc.width = 0;
	xwc.height = 0;

	xwc.stack_mode = Above;

	XConfigureWindow (screen->dpy (), inputPrevention, CWStackMode | CWX | CWY | CWWidth | CWHeight, &xwc);

	XUnmapWindow (screen->dpy (), inputPrevention);

    }
}

/*
 * TabBar::destroyIPW
 *
 */
void
TabBar::destroyIPW ()
{
    if (inputPrevention)
    {
	XDestroyWindow (screen->dpy (), inputPrevention);

	inputPrevention = None;
	ipwMapped = false;
    }
}

/*
 * TabBar::changeTab
 *
 */
bool
TabBar::changeTab (Tab             *topTab,
		   ChangeTabAnimationDirection direction)
{
    CompWindow     *w, *oldTopTab;

    if (!topTab)
    {
	return TRUE;
    }

    w = topTab->window;

    GROUP_WINDOW (w);
    GROUP_SCREEN (screen);

    group = gw->group;

    if (!group || group->tabbingState != NoTabbing)
    {
	return TRUE;
    }

    if (group->changeState == NoTabChange && group->topTab == topTab)
    {
       	return TRUE;
    }

    if (group->changeState != NoTabChange && group->nextTopTab == topTab)
    {
	return TRUE;
    }

    oldTopTab = group->topTab ? group->topTab->window : NULL;

    if (group->changeState != NoTabChange)
	group->nextDirection = direction;
    else if (direction == RotateLeft)
	group->changeAnimationDirection = 1;
    else if (direction == RotateRight)
	group->changeAnimationDirection = -1;
    else
    {
	int             distanceOld = 0, distanceNew = 0;

	if (group->topTab)
	{
	    foreach (Tab *tab, tabs)
	    {
		if (tab == topTab)
		    break;

		distanceOld++;
	    }
        }


	foreach (Tab *tab, tabs)
	{
	    if (tab == topTab)
		break;

	    distanceNew++;
	}

	if (distanceNew < distanceOld)
	    group->changeAnimationDirection = 1;   /*left */
	else
	    group->changeAnimationDirection = -1;  /* right */

	/* check if the opposite direction is shorter */
	if (abs (distanceNew - distanceOld) > (group->tabBar->tabs.size () / 2))
	    group->changeAnimationDirection *= -1;
    }

    if (group->changeState != NoTabChange)
    {
	if (group->prevTopTab == topTab)
	{
	    /* Reverse animation. */
	    Tab *tmp = group->topTab;
	    group->topTab = group->prevTopTab;
	    group->prevTopTab = tmp;

	    group->changeAnimationDirection *= -1;
	    group->changeAnimationTime =
	    gs->optionGetChangeAnimationTime () * 500 -
	    group->changeAnimationTime;
	    group->changeState = (group->changeState == TabChangeOldOut) ?
	    TabChangeNewIn : TabChangeOldOut;

	    group->nextTopTab = NULL;
	}
	else
	    group->nextTopTab = topTab;
    }
    else
    {
	group->topTab = topTab;

	renderWindowTitle ();
	renderTopTabHighlight ();
	if (oldTopTab)
	    CompositeWindow::get (oldTopTab)->addDamage ();
	gw->cWindow->addDamage ();
    }

    if (topTab != group->nextTopTab)
    {
	gw->setVisibility (TRUE);
	if (oldTopTab)
	{
	    int dx, dy;

	    dx = WIN_CENTER_X (oldTopTab) - WIN_CENTER_X (w);
	    dy = WIN_CENTER_Y (oldTopTab) - WIN_CENTER_Y (w);

	    gs->queued = TRUE;
	    w->move (dx, dy, FALSE);
	    w->syncPosition ();
	    gs->queued = FALSE;
	}

	if (HAS_PREV_TOP_WIN (group))
	{
	    /* we use only the half time here -
	       the second half will be PaintFadeOut */
	    group->changeAnimationTime =
		gs->optionGetChangeAnimationTime () * 500;
	    //groupTabChangeActivateEvent (s, TRUE);
	    group->changeState = TabChangeOldOut;
	}
	else
	{
	    Bool activate;

	    /* No window to do animation with. */
	    if (HAS_TOP_WIN (group))
		group->prevTopTab = group->topTab;
	    else
		group->prevTopTab = NULL;

	    activate = !group->checkFocusAfterTabChange;
	    if (!activate)
	    {
	    	/* XXX */
#if 0
		CompFocusResult focus;

		focus    = w->allowFocus (NO_FOCUS_MASK, screen->vp ().x (),
							 screen->vp ().y (), 0);
		activate = focus == CompFocusAllowed;
#endif
	    }

	    if (activate)
		w->activate ();

	    group->checkFocusAfterTabChange = FALSE;
	}
    }

    return TRUE;
}

/*
 * Tab::recalcPos
 *
 */
void
Tab::recalcPos (int slotPos)
{
    Group          *group;
    CompRect       box;
    int            space, thumbSize;

    GROUP_WINDOW (window);
    GROUP_SCREEN (screen);
    group = gw->group;

    if (!HAS_TOP_WIN (group) || !group->tabBar)
	return;

    space = gs->optionGetThumbSpace ();
    thumbSize = gs->optionGetThumbSize ();

    region = CompRegion ();

    box.setX (space + ((thumbSize + space) * slotPos));
    box.setY (space);

    box.setWidth (thumbSize);
    box.setHeight (thumbSize);

    CompRegion boxReg (box);

    region = region.united (boxReg);
}

/*
 * TabBar::recalcPos
 *
 */
void
TabBar::recalcPos (int            middleX,
		   int            minX1,
		   int            maxX2)
{
    Tab             *tab;
    CompWindow      *topTab;
    Bool            isDraggedSlotGroup = FALSE;
    int             space, barWidth;
    int             thumbSize;
    int             tabsWidth = 0, tabsHeight = 0;
    int             currentSlot;
    CompRect        box;

    if (!HAS_TOP_WIN (group))
	return;

    GROUP_SCREEN (screen);

    TabBar *bar = group->tabBar;
    topTab = TOP_TAB (group);
    space = gs->optionGetThumbSpace ();

    /* calculate the space which the tabs need */
    foreach (Tab *tab, tabs)
    {
	if (tab == gs->draggedSlot && gs->dragged)
	{
	    isDraggedSlotGroup = TRUE;
	    continue;
	}

	tabsWidth += (tab->region.boundingRect ().width ());
	if ((tab->region.boundingRect ().height ()) > tabsHeight)
	    tabsHeight = tab->region.boundingRect ().height ();
    }

    /* just a little work-a-round for first call
       FIXME: remove this! */
    thumbSize = gs->optionGetThumbSize ();
    if (!tabs.empty () && tabsWidth <= 0)
    {
	/* first call */
	tabsWidth = thumbSize * tabs.size ();

	if (!tabs.empty () && tabsHeight < thumbSize)
	{
	    /* we need to do the standard height too */
	    tabsHeight = thumbSize;
	}

	if (isDraggedSlotGroup)
	    tabsWidth -= thumbSize;
    }

    barWidth = space * (tabs.size () + 1) + tabsWidth;

    if (isDraggedSlotGroup)
    {
	/* 1 tab is missing, so we have 1 less border */
	barWidth -= space;
    }

    if (maxX2 - minX1 < barWidth)
	box.setX ((maxX2 + minX1) / 2 - barWidth / 2);
    else if (middleX - barWidth / 2 < minX1)
	box.setX (minX1);
    else if (middleX + barWidth / 2 > maxX2)
	box.setX (maxX2 - barWidth);
    else
	box.setX (middleX - barWidth / 2);

    box.setY (WIN_Y (topTab));
    box.setWidth (barWidth);
    box.setHeight (space * 2 + tabsHeight);

    resizeRegion (box, TRUE); // TODO

    /* recalc every slot region */
    currentSlot = 0;
    foreach (Tab *tab, tabs)
    {
	if (tab == gs->draggedSlot && gs->dragged)
	    continue;

	tab->recalcPos (currentSlot);
	tab->region.translate (region.boundingRect ().x1 (),
		       	       region.boundingRect ().y1 ());
		       	       


	tab->springX = (tab->region.boundingRect ().x1 () +
			tab->region.boundingRect ().x2 ()) / 2;
	tab->speed = 0;
	tab->msSinceLastMove = 0;

	currentSlot++;
    }

    leftSpringX = box.x ();
    rightSpringX = box.x () + box.width ();

    rightSpeed = 0;
    leftSpeed = 0;

    rightMsSinceLastMove = 0;
    leftMsSinceLastMove = 0;
}

/*
 * groupGetCurrentMousePosition
 *
 * Description:
 * Return the current function of the pointer at the given screen.
 * The position is queried trough XQueryPointer directly from the xserver.
 *
 */
void
GroupScreen::positionUpdate (const CompPoint &p)
{
    mouse = p;
}

/*
 * groupGetClippingRegion
 *
 * Description:
 * This function returns a clipping region which is used to clip
 * several events involving window stack such as hover detection
 * in the tab bar or Drag'n'Drop. It creates the clipping region
 * with getting the region of every window above the given window
 * and then adds this region to the clipping region using
 * XUnionRectWithRegion. w->region won't work since it doesn't include
 * the window decoration.
 *
 */


CompRegion
GroupWindow::getClippingRegion ()
{
    CompWindowList::iterator it = (std::find (screen->windows ().begin (),
    					      screen->windows ().end (),
    					      window));
    CompRegion  clip = CompRegion (0, 0, 0, 0);
    
    it++;

    while (it != screen->windows ().end ())
    {
        CompWindow *cw = *it;
	if (!cw->invisible () && !(cw->state () & CompWindowStateHiddenMask))
	{
	    CompRect   rect;

	    rect.setX (WIN_REAL_X (cw));
	    rect.setY (WIN_REAL_Y (cw));
	    rect.setWidth (WIN_REAL_WIDTH (cw));
	    rect.setHeight (WIN_REAL_HEIGHT (cw));
	    
	    clip = clip.united (CompRegion (rect));
	}
	
	it++;
    }

    return clip;
}


/*
 * groupClearWindowInputShape
 *
 */
void
GroupWindow::clearInputShape (HideInfo *hideInfo)
{
    XRectangle  *rects;
    int         count = 0, ordering;
    Window      xid = hideInfo->shapeWindow;
    
    rects = XShapeGetRectangles (screen->dpy (), xid, ShapeInput,
				 &count, &ordering);

    if (count == 0)
	return;

    /* check if the returned shape exactly matches the window shape -
       if that is true, the window currently has no set input shape */
    if ((count == 1) &&
	(rects[0].x == -window->serverGeometry ().border ()) &&
	(rects[0].y == -window->serverGeometry ().border ()) &&
	(rects[0].width == (window->serverGeometry ().width () + window->serverGeometry ().border ())) &&
	(rects[0].height == (window->serverGeometry ().height () + window->serverGeometry ().border ())))
    {
	count = 0;
    }

    if (hideInfo->inputRects)
	XFree (hideInfo->inputRects);

    hideInfo->inputRects = rects;
    hideInfo->nInputRects = count;
    hideInfo->inputRectOrdering = ordering;

    XShapeSelectInput (screen->dpy (), xid, NoEventMask);

    XShapeCombineRectangles (screen->dpy (), xid, ShapeInput, 0, 0,
			     NULL, 0, ShapeSet, 0);

    XShapeSelectInput (screen->dpy (), xid, ShapeNotify);
}

/*
 * GroupWindow::restoreInputShape
 *
 */
void
GroupWindow::restoreInputShape (HideInfo *info)
{
    Window xid = info->shapeWindow;

    if (info->nInputRects)
    {
        XShapeCombineRectangles (screen->dpy (), xid, ShapeInput, 0, 0,
			         info->inputRects, info->nInputRects,
			         ShapeSet, info->inputRectOrdering);
    }
    else
    {
        XShapeCombineMask (screen->dpy (), xid, ShapeInput,
		           0, 0, None, ShapeSet);
    }

    if (info->inputRects)
        XFree (info->inputRects);

    XShapeSelectInput (screen->dpy (), xid, info->shapeMask);
}
/*
 * groupSetWindowVisibility
 *
 */
void
GroupWindow::setVisibility (bool visible)
{
    if (!visible && !windowHideInfo)
    {
	HideInfo *info;

	windowHideInfo = info = new HideInfo ();
	if (!windowHideInfo)
	    return;

	info->inputRects = NULL;
	info->nInputRects = 0;
	info->shapeMask = XShapeInputSelected (screen->dpy (), window->id ());
	
	/* We are a reparenting window manager now, which means that we either shape
         * the frame window, or if it does not exist, shape the window **/
	
	if (window->frame ())
	{
	    info->shapeWindow = window->frame ();
	} else
	    info->shapeWindow = window->id ();
	
	clearInputShape (info);

	info->skipState = window->state () & (CompWindowStateSkipPagerMask |
				              CompWindowStateSkipTaskbarMask);

	window->changeState (window->state () |
			   CompWindowStateSkipPagerMask |
			   CompWindowStateSkipTaskbarMask);
    }
    else if (visible && windowHideInfo)
    {
	HideInfo *info = windowHideInfo;

	restoreInputShape (info);

	XShapeSelectInput (screen->dpy (), window->id (), info->shapeMask);

	window->changeState ((window->state () & ~(CompWindowStateSkipPagerMask |
					       CompWindowStateSkipTaskbarMask)) |
			                       info->skipState);

	delete info;
	windowHideInfo = NULL;
    }
}

/*
 * groupTabBarTimeout
 *
 * Description:
 * This function is called when the time expired (== timeout).
 * We use this to realize a delay with the bar hiding after tab change.
 * groupHandleAnimation sets up a timer after the animation has finished.
 * This function itself basically just sets the tab bar to a PaintOff status
 * through calling groupSetTabBarVisibility.
 * The PERMANENT mask allows you to force hiding even of
 * PaintPermanentOn tab bars.
 *
 */
bool
GroupScreen::tabBarTimeout (TabBar *bar)
{
    bar->setVisibility (false, PERMANENT);

    return false;	/* This will free the timer. */
}

/*
 * groupShowDelayTimeout
 *
 */
bool
GroupScreen::showDelayTimeout (TabBar *bar)
{
    int            mouseX, mouseY;
    CompWindow     *topTab;;

    if (!HAS_TOP_WIN (bar->group))
    {
	return FALSE;	/* This will free the timer. */
    }

    topTab = TOP_TAB (bar->group);

    mouse = poller.getCurrentPosition ();

    bar->recalcPos (mouse.x (), WIN_REAL_X (topTab),
		    WIN_REAL_X (topTab) + WIN_REAL_WIDTH (topTab));
    bar->setVisibility (TRUE, 0);

    return FALSE;	/* This will free the timer. */
}

/*
 * groupTabSetVisibility
 *
 * Description:
 * This function is used to set the visibility of the tab bar.
 * The "visibility" is indicated through the PaintState, which
 * can be PaintOn, PaintOff, PaintFadeIn, PaintFadeOut
 * and PaintPermantOn.
 * Currently the mask paramater is mostely used for the PERMANENT mask.
 * This mask affects how the visible parameter is handled, for example if
 * visibule is set to TRUE and the mask to PERMANENT state it will set
 * PaintPermanentOn state for the tab bar. When visibile is FALSE, mask 0
 * and the current state of the tab bar is PaintPermanentOn it won't do
 * anything because its not strong enough to disable a
 * Permanent-State, for those you need the mask.
 *
 */
void
TabBar::setVisibility (bool           visible,
		       unsigned int   mask)
{
    Tab         *tab;
    CompWindow  *topTabWin;
    Layer::PaintState  oldState;
    
    GROUP_SCREEN (screen);

    if (group->windows.empty () || !HAS_TOP_WIN (group))
	return;

    topTabWin = TOP_TAB (group);
    oldState = state;

    /* hide tab bars for invisible top windows */
    if ((topTabWin->state () & CompWindowStateHiddenMask) || topTabWin->invisible ())
    {
	state = Layer::PaintOff;
	switchTopTabInput (TRUE);
    }
    else if (visible && state != Layer::PaintPermanentOn && (mask & PERMANENT))
    {
	state = Layer::PaintPermanentOn;
	switchTopTabInput (FALSE);
    }
    else if (visible && state == Layer::PaintPermanentOn && !(mask & PERMANENT))
    {
	state = Layer::PaintOn;
    }
    else if (visible && (state == Layer::PaintOff || state == Layer::PaintFadeOut))
    {
	if (gs->optionGetBarAnimations ())
	{
	    bgAnimation = AnimationReflex;
	    bgAnimationTime = gs->optionGetReflexTime () * 1000.0;
	}
	state = Layer::PaintFadeIn;
	switchTopTabInput (FALSE);
    }
    else if (!visible &&
	     (state != Layer::PaintPermanentOn || (mask & PERMANENT)) &&
	     (state == Layer::PaintOn || state == Layer::PaintPermanentOn ||
	      state == Layer::PaintFadeIn))
    {
	state = Layer::PaintFadeOut;
	switchTopTabInput (TRUE);
    }

    if (state == Layer::PaintFadeIn || state == Layer::PaintFadeOut)
	animationTime = (gs->optionGetFadeTime () * 1000) - animationTime;

    if (state != oldState)
	damageRegion ();
}

/*
 * groupGetDrawOffsetForSlot
 *
 * Description:
 * Its used when the draggedSlot is dragged to another viewport.
 * It calculates a correct offset to the real slot position.
 *
 */
void
Tab::getDrawOffset (int &hoffset,
		    int &voffset)
{
    CompWindow *w, *topTab;
    int        vx, vy, x, y;
    CompPoint  vp;
    CompWindow::Geometry winGeometry (window->serverGeometry ());/*x, y, window->serverGeometry ().width (),
				      window->serverGeometry ().height (),
				      window->serverGeometry ().border ());*/

    if (!window)
	return;

    w = window;

    GROUP_WINDOW (w);
    GROUP_SCREEN (screen);

    if (this != gs->draggedSlot)
    {
	hoffset = 0;
	voffset = 0;
	return;
    }

    if (HAS_TOP_WIN (gw->group))
	topTab = TOP_TAB (gw->group);
    else if (HAS_PREV_TOP_WIN (gw->group))
	topTab = PREV_TOP_TAB (gw->group);
    else
    {
	hoffset = 0;
	voffset = 0;
	return;
    }

    x = WIN_CENTER_X (topTab) - WIN_WIDTH (w) / 2;
    y = WIN_CENTER_Y (topTab) - WIN_HEIGHT (w) / 2;

    screen->viewportForGeometry (winGeometry,
			 	 vp);
			 	 
    vx = vp.x ();
    vy = vp.y ();

    hoffset = screen->vp ().x () - vx % screen->vpSize ().width ();
    voffset = screen->vp ().y () - vy % screen->vpSize ().height ();
}

/*
 * groupHandleHoverDetection
 *
 * Description:
 * This function is called from groupPreparePaintScreen to handle
 * the hover detection. This is needed for the text showing up,
 * when you hover a thumb on the thumb bar.
 *
 * FIXME: we should better have a timer for that ...
 */
void
Group::handleHoverDetection ()
{
    TabBar *bar = tabBar;
    CompWindow  *topTabWin = TOP_TAB (this);
    int         mouseX, mouseY;
    Bool        mouseOnScreen, inLastSlot;

    GROUP_SCREEN (screen);

    /* first get the current mouse position */
    gs->mouse = gs->poller.getCurrentPosition ();

    /* then check if the mouse is in the last hovered slot --
       this saves a lot of CPU usage */
    inLastSlot = bar->hoveredSlot &&
	         bar->hoveredSlot->region.contains (gs->mouse);

    if (!inLastSlot)
    {
	CompRegion          clip;

	bar->hoveredSlot = NULL;
	clip = GroupWindow::get (topTabWin)->getClippingRegion ();

	foreach (Tab *tab, bar->tabs)
	{
	    /* We need to clip the slot region with the clip region first.
	       This is needed to respect the window stack, so if a window
	       covers a port of that slot, this part won't be used
	       for in-slot-detection. */
	    CompRegion reg;

	    reg = tab->region.subtracted (clip);
	    
	    CompRect tmpRegBox = reg.boundingRect ();
	    if (reg.contains (gs->mouse))
	    {
		bar->hoveredSlot = tab;
		break;
	    }
	}

	if (bar->textLayer)
	{	
	    /* trigger a FadeOut of the text */
	    if ((bar->hoveredSlot != bar->textSlot) &&
		(bar->textLayer->state == Layer::PaintFadeIn ||
		 bar->textLayer->state == Layer::PaintOn))
	    {
		bar->textLayer->animationTime =
		    (gs->optionGetFadeTextTime () * 1000) -
		    bar->textLayer->animationTime;
		bar->textLayer->state = Layer::PaintFadeOut;
	    }
	    /* or trigger a FadeIn of the text */
	    else if ((bar->textLayer->state == Layer::PaintFadeOut ||
	    	      bar->textLayer->state == Layer::PaintOff) &&
		     bar->hoveredSlot == bar->textSlot && bar->hoveredSlot)
	    {
		bar->textLayer->animationTime =
		    (gs->optionGetFadeTextTime () * 1000) -
		    bar->textLayer->animationTime;
		bar->textLayer->state = Layer::PaintFadeIn;
	    }
	}
    }
}

/*
 * groupHandleTabBarFade
 *
 * Description:
 * This function is called from groupPreparePaintScreen
 * to handle the tab bar fade. It checks the animationTime and updates it,
 * so we can calculate the alpha of the tab bar in the painting code with it.
 *
 */
void
TabBar::handleFade (int            msSinceLastPaint)
{
    animationTime -= msSinceLastPaint;

    if (animationTime < 0)
	animationTime = 0;

    /* Fade finished */
    if (animationTime == 0)
    {
	if (state == Layer::PaintFadeIn)
	{
	    state = Layer::PaintOn;
	}
	else if (state == Layer::PaintFadeOut)
	{
	    state = Layer::PaintOff;

	    if (textLayer)
	    {
		/* Tab-bar is no longer painted, clean up
		   text animation variables. */
		textLayer->animationTime = 0;
		textLayer->state = Layer::PaintOff;
		textSlot = hoveredSlot = NULL;

		renderWindowTitle ();
	    }
	}
    }
}

/*
 * groupHandleTextFade
 *
 * Description:
 * This function is called from groupPreparePaintScreen
 * to handle the text fade. It checks the animationTime and updates it,
 * so we can calculate the alpha of the text in the painting code with it.
 *
 */
void
TabBar::handleTextFade (int msSinceLastPaint)
{
    GROUP_SCREEN (screen);

    /* Fade in progress... */
    if ((textLayer->state == Layer::PaintFadeIn || textLayer->state == Layer::PaintFadeOut) &&
	textLayer->animationTime > 0)
    {
	textLayer->animationTime -= msSinceLastPaint;

	if (textLayer->animationTime < 0)
	    textLayer->animationTime = 0;

	/* Fade has finished. */
	if (textLayer->animationTime == 0)
	{
	    if (textLayer->state == Layer::PaintFadeIn)
		textLayer->state = Layer::PaintOn;

	    else if (textLayer->state == Layer::PaintFadeOut)
	    {
		textLayer->state = Layer::PaintOff;
	    }
	}
    }
    if (hoveredSlot && textLayer->state == Layer::PaintOff)
    {
	/* Start text animation for the new hovered slot. */
	textSlot = hoveredSlot;
	textLayer->state = Layer::PaintFadeIn;
	textLayer->animationTime =
	    (gs->optionGetFadeTextTime () * 1000);

	renderWindowTitle ();
    }
    else if (textSlot && textLayer->state == Layer::PaintOff)
    {
	/* Clean Up. */
	textSlot = NULL;
	renderWindowTitle ();
    }
}

/*
 * groupHandleTabBarAnimation
 *
 * Description: Handles the different animations for the tab bar defined in
 * GroupAnimationType. Basically that means this function updates
 * tabBar->animation->time as well as checking if the animation is already
 * finished.
 *
 */
void
TabBar::handleAnimation (int            msSinceLastPaint)
{
    bgAnimationTime -= msSinceLastPaint;

    if (bgAnimationTime <= 0)
    {
	bgAnimationTime = 0;
	bgAnimation = AnimationNone;

	renderTabBarBackground ();
    }
}

/*
 * groupTabChangeActivateEvent
 *
 * Description: Creates a compiz event to let other plugins know about
 * the starting and ending point of the tab changing animation
 */
void
GroupScreen::tabChangeActivateEvent (bool activating)
{
    CompOption::Vector o;

    CompOption opt ("root", CompOption::TypeInt);
    opt.value ().set ((int) screen->root ());

    o.push_back (opt);

    CompOption opt2 ("active", CompOption::TypeBool);
    opt2.value ().set (activating);

    o.push_back (opt2);

    screen->handleCompizEvent ("group", "tabChangeActivate", o);
}

/*
 * groupHandleAnimation
 *
 * Description:
 * This function handles the change animation. It's called
 * from groupHandleChanges. Don't let the changeState
 * confuse you, PaintFadeIn equals with the start of the
 * rotate animation and PaintFadeOut is the end of these
 * animation.
 *
 */
void
Group::handleAnimation ()
{
    GROUP_SCREEN (screen);

    if (changeState == TabBar::TabChangeOldOut && tabBar)
    {
	CompWindow      *top = TOP_TAB (this);
	Bool            activate;

	/* recalc here is needed (for y value)! */
	tabBar->recalcPos ((tabBar->region.boundingRect ().x1 () +
			    tabBar->region.boundingRect ().x2 ()) / 2,
			    WIN_REAL_X (top),
			    WIN_REAL_X (top) + WIN_REAL_WIDTH (top));

	changeAnimationTime += gs->optionGetChangeAnimationTime () * 500;

	if (changeAnimationTime <= 0)
	    changeAnimationTime = 0;

	changeState = TabBar::TabChangeNewIn;

	activate = !checkFocusAfterTabChange;
	
	/* XXX */
	
	if (!activate)
	{
#if 0
	    CompFocusResult focus;
	    focus    = allowWindowFocus (top, NO_FOCUS_MASK, screen->vp ().x (),
	    						 screen->vp ().y (), 0);
	    activate = focus == CompFocusAllowed;
#endif
	}

	if (activate)
	    top->activate ();

	checkFocusAfterTabChange = FALSE;
    }

    if (changeState == TabBar::TabChangeNewIn &&
	changeAnimationTime <= 0)
    {
	int oldChangeAnimationTime = changeAnimationTime;

	gs->tabChangeActivateEvent (FALSE);

	if (prevTopTab)
	    GroupWindow::get (PREV_TOP_TAB (this))->setVisibility (false);

	prevTopTab = topTab;
	changeState = TabBar::NoTabChange;

	if (nextTopTab)
	{
	    Tab *next = nextTopTab;
	    nextTopTab = NULL;

	    tabBar->changeTab (next, nextDirection);

	    if (changeState == TabBar::TabChangeOldOut)
	    {
		/* If a new animation was started. */
		changeAnimationTime += oldChangeAnimationTime;
	    }
	}

	if (changeAnimationTime <= 0)
	{
	    changeAnimationTime = 0;
	}
	else if (gs->optionGetVisibilityTime () != 0.0f &&
		 changeState == TabBar::NoTabChange)
	{
	    tabBar->setVisibility (TRUE, PERMANENT | SHOW_BAR_INSTANTLY_MASK);

	    if (tabBar->timeoutHandle.active ())
		tabBar->timeoutHandle.stop ();

	    tabBar->timeoutHandle.start ();

	}
    }
}

/* adjust velocity for each animation step (adapted from the scale plugin) */
/* Used for the tab/untab animation for _windows_ not tabs */
int
GroupWindow::adjustTabVelocity ()
{
    float dx, dy, adjust, amount;
    float x1, y1;

    x1 = destination.x ();
    y1 = destination.y ();

    dx = x1 - (orgPos.x () + tx);
    adjust = dx * 0.15f;
    amount = fabs (dx) * 1.5f;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    xVelocity = (amount * xVelocity + adjust) / (amount + 1.0f);

    dy = y1 - (orgPos.y () + ty);
    adjust = dy * 0.15f;
    amount = fabs (dy) * 1.5f;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    yVelocity = (amount * yVelocity + adjust) / (amount + 1.0f);

    if (fabs (dx) < 0.1f && fabs (xVelocity) < 0.2f &&
	fabs (dy) < 0.1f && fabs (yVelocity) < 0.2f)
    {
	xVelocity = yVelocity = 0.0f;
	tx = x1 - window->serverGeometry ().x ();
	ty = y1 - window->serverGeometry ().y ();

	return 0;
    }
    return 1;
}

/*
 * Tab::paint
 *
 */
void
Tab::paint (Group *group,
	        const GLMatrix &transform,
	        int targetOpacity)
{
    CompWindow            *w = window;
    int                   oldAddWindowGeometryIndex;
    GLWindowPaintAttrib     wAttrib;
    int                   tw, th;

    GROUP_WINDOW (w);
    GROUP_SCREEN (screen);
    
    wAttrib = gw->gWindow->paintAttrib ();
    
    tw = region.boundingRect ().x2 () - region.boundingRect ().x1 ();
    th = region.boundingRect ().y2 () - region.boundingRect ().y1 ();

    /* Wrap drawWindowGeometry to make sure the general
       drawWindowGeometry function is used */

    oldAddWindowGeometryIndex = gw->gWindow->glAddGeometryGetCurrentIndex ();
    gw->gWindow->glAddGeometrySetCurrentIndex (MAXSHORT); 

    /* animate fade */
    if (group && group->tabBar->state == Layer::PaintFadeIn)
    {
	wAttrib.opacity -= wAttrib.opacity * group->tabBar->animationTime /
	                   (gs->optionGetFadeTime () * 1000);
    }
    else if (group && group->tabBar->state == Layer::PaintFadeOut)
    {
	wAttrib.opacity = wAttrib.opacity * group->tabBar->animationTime /
	                  (gs->optionGetFadeTime () * 1000);
    }

    wAttrib.opacity = wAttrib.opacity * targetOpacity / OPAQUE;

    if (w->mapNum ())
    {
	GLFragment::Attrib fragment (wAttrib);
	GLMatrix  wTransform (transform);
	int            width, height;
	int            vx, vy;

	width = w->width () + w->output ().left + w->output ().right;
	height = w->height () + w->output ().top + w->output ().bottom;

	if (width > tw)
	    wAttrib.xScale = (float) tw / width;
	else
	    wAttrib.xScale = 1.0f;
	if (height > th)
	    wAttrib.yScale = (float) th / height;
	else
	    wAttrib.yScale = 1.0f;

	if (wAttrib.xScale < wAttrib.yScale)
	    wAttrib.yScale = wAttrib.xScale;
	else
	    wAttrib.xScale = wAttrib.yScale;

	/* FIXME: do some more work on the highlight on hover feature
	// Highlight on hover
	if (group && group->tabBar && group->tabBar->hoveredSlot == slot) {
	wAttrib.saturation = 0;
	wAttrib.brightness /= 1.25f;
	}*/

	getDrawOffset (vx, vy);

	wAttrib.xTranslate =  (region.boundingRect ().x1 () +
			       region.boundingRect ().x2 ()) / 2 + vx;
	wAttrib.yTranslate =  region.boundingRect ().y1 () + vy;

	wTransform.translate (wAttrib.xTranslate, wAttrib.yTranslate, 0.0f);
	wTransform.scale (wAttrib.xScale, wAttrib.yScale, 1.0f);
	wTransform.translate (-(WIN_X (w) + WIN_WIDTH (w) / 2),
			 -(WIN_Y (w) - w->output ().top), 0.0f);

	glPushMatrix ();
	glLoadMatrixf (wTransform.getMatrix ());
	gw->gWindow->glDraw (wTransform, fragment, infiniteRegion,
			  PAINT_WINDOW_TRANSFORMED_MASK |
			  PAINT_WINDOW_TRANSLUCENT_MASK);

	glPopMatrix ();
    }

    gw->gWindow->glAddGeometrySetCurrentIndex (oldAddWindowGeometryIndex);
}

/*
 * groupPaintTabBar
 *
 */
void
TabBar::draw (const GLWindowPaintAttrib  &wAttrib,
	          const GLMatrix		 &transform,
	          unsigned int		 mask,
	          CompRegion		 clipRegion)
{
    CompWindow      *topTab;
    int             count;
    CompRegion      box;
    
    GROUP_SCREEN (screen);

    if (HAS_TOP_WIN (group))
	topTab = TOP_TAB (group);
    else
	topTab = PREV_TOP_TAB (group);

#define PAINT_BG     0
#define PAINT_SEL    1
#define PAINT_THUMBS 2
#define PAINT_TEXT   3
#define PAINT_MAX    4

    for (count = 0; count < PAINT_MAX; count++)
    {
	int             alpha = OPAQUE;
	float           wScale = 1.0f, hScale = 1.0f;
	Layer           *layer = NULL;

	if (state == Layer::PaintFadeIn)
	    alpha -= alpha * animationTime / (gs->optionGetFadeTime () * 1000);
	else if (state == Layer::PaintFadeOut)
	    alpha = alpha * animationTime / (gs->optionGetFadeTime () * 1000);

	switch (count) {
	case PAINT_BG:
	    {
		int newWidth;

		/* handle the repaint of the background */
		newWidth = region.boundingRect ().x2 () - region.boundingRect ().x1 ();
		if (bgLayer && (newWidth > bgLayer->texWidth))
		    newWidth = bgLayer->texWidth;

		wScale = (double) (region.boundingRect ().x2 () -
				   region.boundingRect ().x1 ()) / (double) newWidth;

		/* FIXME: maybe move this over to groupResizeTabBarRegion -
		   the only problem is that we would have 2 redraws if
		   there is an animation */
		if (newWidth != oldWidth || bgAnimation)
		    renderTabBarBackground ();
		
		layer = (Layer *) bgLayer;

		oldWidth = newWidth;
		box = region;
	    }
	    break;

	case PAINT_SEL:
	    if (group->topTab != gs->draggedSlot)
	    {
		layer = (Layer *) selectionLayer;
		box = group->topTab->region;
	    }
	    break;

	case PAINT_THUMBS:
	    {
		GLenum          oldTextureFilter;
		Tab 		*tab;

		oldTextureFilter = gs->gScreen->textureFilter ();

		if (gs->optionGetMipmaps ())
		    gs->gScreen->setTextureFilter (GL_LINEAR_MIPMAP_LINEAR);

		foreach (Tab *tab, tabs)
		{
		    if (tab != gs->draggedSlot || !gs->dragged)
		    {
			tab->paint (group, transform,
					    wAttrib.opacity);
		    }
		}

		gs->gScreen->setTextureFilter (oldTextureFilter);
	    }
	    break;

	case PAINT_TEXT:
	    if (textLayer && (textLayer->state != Layer::PaintOff))
	    {
		layer = (Layer *) textLayer;

		CompRect regBox;
		
		regBox.setGeometry (region.boundingRect ().x1 () + 5,
		                    region.boundingRect ().y2 () - textLayer->texHeight - 5,
		                    textLayer->texWidth + 5,
		                    textLayer->texHeight - 5);

		if (regBox.x2 () > region.boundingRect ().x2 ())
		    regBox.setWidth (region.boundingRect ().x2 () - region.boundingRect ().x1 ());

		/* recalculate the alpha again for text fade... */
		if (layer->state == Layer::PaintFadeIn)
		    alpha -= alpha * layer->animationTime /
			     (gs->optionGetFadeTextTime () * 1000);
		else if (layer->state == Layer::PaintFadeOut)
		    alpha = alpha * layer->animationTime /
			    (gs->optionGetFadeTextTime () * 1000);
			    
		box = CompRegion (regBox);
	    }
	    break;
	}

	if (layer)
	{	
	    foreach (GLTexture *tex, layer->texture)
	    {
		GLTexture::Matrix matrix = tex->matrix ();
		GLTexture::MatrixList matricies;
		
		CompRect boxRect (box.boundingRect ());

		/* remove the old x1 and y1 so we have a relative value */
		
		boxRect.setGeometry ((boxRect.x1 () - topTab->x ()) / wScale + topTab->x (),
				     (boxRect.y1 () - topTab->y ()) / hScale + topTab->y (),
				     boxRect.x2 () - boxRect.x1 (), boxRect.y2 () - boxRect.y1 ());

		/* now add the new x1 and y1 so we have a absolute value again,
		also we don't want to stretch the texture... */
		if (boxRect.x2 () * wScale < layer->texWidth)
		{
		    boxRect.setX (boxRect.x1 ());
		    boxRect.setWidth (boxRect.x1 () + boxRect.x2 () - boxRect.x1 ());
		}
		else
		    boxRect.setWidth (layer->texWidth);

		if (boxRect.y2 () * hScale < layer->texHeight)
		    boxRect.setHeight (boxRect.y2 () + box.boundingRect ().y1 () - boxRect.x1 ());
		else
		    boxRect.setHeight (layer->texHeight);

		matrix.x0 -= boxRect.x1 () * matrix.xx;
		matrix.y0 -= boxRect.y1 () * matrix.yy;
		
		/* FIXME */
		GLWindow::get (topTab)->geometry ().reset ();
		
		matricies.push_back (matrix);
		
		box = CompRegion (boxRect);

		GLWindow::get (topTab)->glAddGeometry (matricies, box, clipRegion);

		if (GLWindow::get (topTab)->geometry ().vertices)
		{
		    GLFragment::Attrib fragment (wAttrib);
		    GLMatrix wTransform (transform);

		    wTransform.translate (WIN_X (topTab), WIN_Y (topTab), 0.0f);
		    wTransform.scale (wScale, hScale, 1.0f);
		    wTransform.translate (wAttrib.xTranslate / wScale - WIN_X (topTab),
				          wAttrib.yTranslate / hScale - WIN_Y (topTab),
				          0.0f);

		    alpha = alpha * ((float) wAttrib.opacity / OPAQUE);

		    fragment.setOpacity (alpha);	
		

		    glPushMatrix ();
		    glLoadMatrixf (wTransform.getMatrix ());

		    GLWindow::get (topTab)->glDrawTexture (tex, fragment,
					       mask |
					      PAINT_WINDOW_BLEND_MASK |
					      PAINT_WINDOW_TRANSFORMED_MASK |
					      PAINT_WINDOW_TRANSLUCENT_MASK);
					      
		    glPopMatrix ();
		}
	    }
	}
    }
}

void
Group::finishTabbing ()
{
    int        i;
    CompWindowList::iterator it;

    GROUP_SCREEN (screen);

    tabbingState = TabBar::NoTabbing;
    gs->tabChangeActivateEvent (FALSE);

    if (tabBar)
    {
	/* tabbing case - hide all non-toptab windows */
	foreach (Tab *tab, tabBar->tabs)
	{
	    CompWindow *w = tab->window;
	    if (!w)
		continue;

	    GROUP_WINDOW (w);

	    if (tab == topTab || (gw->animateState & IS_UNGROUPING))
		continue;

	    gw->setVisibility (FALSE);
	}

	prevTopTab = topTab;
    }
    
    it = windows.begin ();

    while (it != windows.end ())
    {
        CompWindow *w = *it;
        
	GROUP_WINDOW (w);
	GROUP_SCREEN (screen);
	
	it++;

	/* move window to target position */
	gs->queued = TRUE;
	w->move (gw->destination.x () - WIN_X (w),
		 gw->destination.y ()- WIN_Y (w), TRUE);
	gs->queued = FALSE;
	w->syncPosition ();

	if (ungroupState == UngroupSingle &&
	    (gw->animateState & IS_UNGROUPING))
	{
	    gw->removeFromGroup ();
	}

	gw->animateState = 0;
	gw->tx = gw->ty = gw->xVelocity = gw->yVelocity = 0.0f;
    }

    if (ungroupState == UngroupAll)
	destroy (true);
    else
	ungroupState = UngroupNone;
}

/*
 * groupDrawTabAnimation
 *
 * Description:
 * This function is called from groupPreparePaintScreen, to move
 * all the animated windows, with the required animation step.
 * The function goes through all grouped animated windows, calculates
 * the required step using adjustTabVelocity, moves the window,
 * and then checks if the animation is finished for that window.
 *
 */
void
Group::drawTabAnimation (int            msSinceLastPaint)
{
    int        steps, i;
    float      amount, chunk;
    Bool       doTabbing;
    
    GROUP_SCREEN (screen);

    amount = msSinceLastPaint * 0.05f * gs->optionGetTabbingSpeed ();
    steps = amount / (0.5f * gs->optionGetTabbingTimestep ());
    if (!steps)
	steps = 1;
    chunk = amount / (float)steps;

    while (steps--)
    {
	doTabbing = FALSE;

	foreach (CompWindow *cw, windows)
	{
	    if (!cw)
		continue;

	    GROUP_WINDOW (cw);

	    if (!(gw->animateState & IS_ANIMATED))
		continue;

	    if (!gw->adjustTabVelocity ())
	    {
		gw->animateState |= FINISHED_ANIMATION;
		gw->animateState &= ~IS_ANIMATED;
	    }

	    gw->tx += gw->xVelocity * chunk;
	    gw->ty += gw->yVelocity * chunk;

	    doTabbing |= (gw->animateState & IS_ANIMATED);
	}

	if (!doTabbing)
	{
	    /* tabbing animation finished */
	    finishTabbing ();
	    break;
	}
    }
}

/*
 * groupUpdateTabBars
 *
 * Description:
 * This function is responsible for showing / unshowing the tab-bars,
 * when the title-bars / tab-bars are hovered.
 * The function is called whenever a new window is entered,
 * checks if the entered window is a window frame (and if the title
 * bar part of that frame was hovered) or if it was the input
 * prevention window of a tab bar, and sets tab-bar visibility
 * according to that.
 *
 */
void
GroupScreen::updateTabBars (Window     enteredWin)
{
    CompWindow     *w = NULL;
    Group *hoveredGroup = NULL;

    /* do nothing if the screen is grabbed, as the frame might be drawn
       transformed */
    if (!screen->otherGrabExist ("group", "group-drag", NULL))
    {
	/* first check if the entered window is a frame */
	foreach (w, screen->windows ())
	{
	    if (w->frame () == enteredWin)
		break;
	}
    }

    if (w)
    {
	/* is the window the entered frame belongs to inside
	   a tabbed group? if no, it's not interesting for us */
	GROUP_WINDOW (w);

	if (gw->group && gw->group->tabBar)
	{
	    mouse = poller.getCurrentPosition ();
	    /* it is grouped and tabbed, so now we have to
	       check if we hovered the title bar or the frame */
#if 0
	    if (groupGetCurrentMousePosition (s, &mouseX, &mouseY))
	    {
#endif
	    CompRect   rect;
	    CompRegion reg;

	    rect.setX (WIN_X (w) - w->input ().left);
	    rect.setY (WIN_Y (w) - w->input ().top);
	    rect.setWidth (WIN_WIDTH (w) + w->input ().right);
	    rect.setHeight (WIN_Y (w) - rect.y ());

	    CompRegion rectReg (rect);

	    reg = reg.united (rectReg);

	    if (reg.contains (mouse))
	    {
		hoveredGroup = gw->group;
	    }
	    //}
	}
    }

    /* if we didn't hover a title bar, check if we hovered
       a tab bar (means: input prevention window) */
    if (!hoveredGroup)
    {
	foreach (Group *group, groups)
	{
	    if (group->tabBar && group->tabBar->inputPrevention == enteredWin)
	    {
		/* only accept it if the IPW is mapped */
		if (group->tabBar->ipwMapped)
		{
		    hoveredGroup = group;
		    break;
		}
	    }
	}
    }

    /* if we found a hovered tab bar different than the last one
       (or left a tab bar), hide the old one */
    if (lastHoveredGroup && (hoveredGroup != lastHoveredGroup))
	lastHoveredGroup->tabBar->setVisibility (FALSE, 0);

    /* if we entered a tab bar (or title bar), show the tab bar */
    if (hoveredGroup && HAS_TOP_WIN (hoveredGroup) &&
	!TOP_TAB (hoveredGroup)->grabbed ())
    {
	TabBar *bar = hoveredGroup->tabBar;
	

	if (bar && ((bar->state == Layer::PaintOff) || (bar->state == Layer::PaintFadeOut)))
	{
	    int showDelayTime = optionGetTabbarShowDelay () * 1000;

	    /* Show the tab-bar after a delay,
	       only if the tab-bar wasn't fading out. */
	    if (showDelayTime > 0 && (bar->state == Layer::PaintOff))
	    {
	    
		if (showDelayTimeoutHandle.active ())
		    showDelayTimeoutHandle.stop ();
		showDelayTimeoutHandle.setCallback (boost::bind (&GroupScreen::showDelayTimeout,
							this, hoveredGroup->tabBar));
		showDelayTimeoutHandle.setTimes (showDelayTime,
						     (float) showDelayTime * 1.2);
		showDelayTimeoutHandle.start ();
	    }
	    else
		showDelayTimeout (hoveredGroup->tabBar);
	}
    }

    lastHoveredGroup = hoveredGroup;
}

/*
 * groupGetConstrainRegion
 *
 */
CompRegion
GroupScreen::getConstrainRegion ()
{
    CompRegion region;
    CompRegion r;
    CompRect   strutRect;
    int        i;

    for (i = 0; i < screen->outputDevs ().size (); i++)
	region = CompRegion (screen->outputDevs ().at (i)).united (region);

    foreach (CompWindow *w, screen->windows ())
    {
	if (!w->mapNum ())
	    continue;

	if (w->struts ())
	{
	    
	    strutRect.setGeometry (w->struts ()->top.x,
	    			   w->struts ()->top.y,
	    		      	   w->struts ()->top.width,
	    		      	   w->struts ()->top.height);
	    		      
	    r = CompRegion (strutRect);
	    
	    region = region.subtracted (r);
	    
    	    strutRect.setGeometry (w->struts ()->bottom.x,
    	    			   w->struts ()->bottom.y,
    		      		   w->struts ()->bottom.width,
    		      		   w->struts ()->bottom.height);
	    		      
	    r = CompRegion (strutRect);
	    
	    region = region.subtracted (r);
	    
	    strutRect.setGeometry (w->struts ()->left.x,
    	    			   w->struts ()->left.y,
    		      		   w->struts ()->left.width,
    		      		   w->struts ()->left.height);
	    		      
	    r = CompRegion (strutRect);
	    
	    region = region.subtracted (r);
	    
	    strutRect.setGeometry (w->struts ()->right.x,
    	    			   w->struts ()->right.y,
    		      		   w->struts ()->right.width,
    		      		   w->struts ()->right.height);
	    		      
	    r = CompRegion (strutRect);
	    
	    region = region.subtracted (r);
	}
    }

    return region;
}

/*
 * groupConstrainMovement
 *
 */
bool
GroupWindow::constrainMovement (CompRegion     constrainRegion,
				int        dx,
				int        dy,
				int        &new_dx,
				int        &new_dy)
{
    int status, xStatus;
    int origDx = dx, origDy = dy;
    int x, y, width, height;

    GROUP_WINDOW (window);

    if (!gw->group)
	return FALSE;

    if (!dx && !dy)
	return FALSE;

    x = gw->orgPos.x () - window->input ().left + dx;
    y = gw->orgPos.y () - window->input ().top + dy;
    width = WIN_REAL_WIDTH (window);
    height = WIN_REAL_HEIGHT (window);

    CompRect winRect (x, y, width, height);

    status = constrainRegion.contains (winRect);

    xStatus = status;
    while (dx && (xStatus != RectangleIn))
    {
	CompRect newWinRect (x, y - dy, width, height);

	xStatus = constrainRegion.contains (x, y - dy, width, height);

	if (xStatus != RectangleIn)
	    dx += (dx < 0) ? 1 : -1;

	x = gw->orgPos.x () - window->input ().left + dx;
    }

    while (dy && (status != RectangleIn))
    {
	CompRect newWinRect (x, y, width, height);

	status = constrainRegion.contains (newWinRect);

	if (status != RectangleIn)
	    dy += (dy < 0) ? 1 : -1;

	y = gw->orgPos.y () - window->input ().top + dy;
    }

    new_dx = dx;

    new_dy = dy;

    return ((dx != origDx) || (dy != origDy));
}

/*
 * groupApplyConstraining
 *
 */
void
Group::applyConstraining (CompRegion         constrainRegion,
			  Window         constrainedWindow,
			  int            dx,
			  int            dy)
{
    int        i;
    CompWindow *w;

    if (!dx && !dy)
	return;

    foreach (CompWindow *w, windows)
    {
	GROUP_WINDOW (w);

	/* ignore certain windows: we don't want to apply the constraining
	   results on the constrained window itself, nor do we want to
	   change the target position of unamimated windows and of
	   windows which already are constrained */
	if (w->id () == constrainedWindow)
	    continue;

	if (!(gw->animateState & IS_ANIMATED))
	    continue;

	if (gw->animateState & DONT_CONSTRAIN)
	    continue;

	if (!(gw->animateState & CONSTRAINED_X))
	{
	    int dummyDy;
	    
	    gw->animateState |= IS_ANIMATED;

	    /* applying the constraining result of another window
	       might move the window offscreen, too, so check
	       if this is not the case */
	    if (gw->constrainMovement (constrainRegion, dx, 0, dx, dummyDy))
		gw->animateState |= CONSTRAINED_X;

	    gw->destination.setX (gw->destination.x () + dx);
	}

	if (!(gw->animateState & CONSTRAINED_Y))
	{
	    int dummyDx;
	
	    gw->animateState |= IS_ANIMATED;

	    /* analog to X case */
	    if (gw->constrainMovement (constrainRegion, 0, dy, dummyDx, dy))
		gw->animateState |= CONSTRAINED_Y;

	    gw->destination.setY (gw->destination.y () + dy);
	}
    }
}

/*
 * groupStartTabbingAnimation
 *
 */
void
Group::startTabbingAnimation (Bool           tab)
{
    int        i;
    int        dx (0), dy (0);
    int        constrainStatus;
    
    GROUP_SCREEN (screen);

    if ((tabbingState != TabBar::NoTabbing))
	return;

    tabbingState = (tab) ? TabBar::Tabbing : TabBar::Untabbing;
    gs->tabChangeActivateEvent (TRUE);

    if (!tab)
    {
	/* we need to set up the X/Y constraining on untabbing */
	CompRegion constrainRegion = gs->getConstrainRegion ();
	Bool   constrainedWindows = TRUE;

	if (constrainRegion.isEmpty ())
	    return;

	/* reset all flags */
	foreach (CompWindow *w, windows)
	{
	    GROUP_WINDOW (w);
	    gw->animateState &= ~(CONSTRAINED_X | CONSTRAINED_Y |
				  DONT_CONSTRAIN);
	}

	/* as we apply the constraining in a flat loop,
	   we may need to run multiple times through this
	   loop until all constraining dependencies are met */
	while (constrainedWindows)
	{
	    constrainedWindows = FALSE;
	    /* loop through all windows and try to constrain their
	       animation path (going from gw->orgPos to
	       gw->destination) to the active screen area */
	    foreach (CompWindow *w, windows)
	    {
		GROUP_WINDOW (w);

		CompRect winRect (gw->orgPos.x () - w->input ().left,
				  gw->orgPos.y () - w->input ().top,
				  WIN_REAL_WIDTH (w),
				  WIN_REAL_HEIGHT (w));

		/* ignore windows which aren't animated and/or
		   already are at the edge of the screen area */
		if (!(gw->animateState & IS_ANIMATED))
		    continue;

		if (gw->animateState & DONT_CONSTRAIN)
		    continue;

		/* is the original position inside the screen area? */
		constrainStatus = constrainRegion.contains (winRect);

		/* constrain the movement */
		if (gw->constrainMovement (constrainRegion,
					    gw->destination.x () - gw->orgPos.x (),
					    gw->destination.y () - gw->orgPos.y (),
					    dx, dy))
		{
		    /* handle the case where the window is outside the screen
		       area on its whole animation path */
		    if (constrainStatus != RectangleIn && !dx && !dy)
		    {
			gw->animateState |= DONT_CONSTRAIN;
			gw->animateState |= CONSTRAINED_X | CONSTRAINED_Y;

			/* use the original position as last resort */
			gw->destination.setX (gw->mainTabOffset.x ());
			gw->destination.setY (gw->mainTabOffset.y ());
		    }
		    else
		    {
			/* if we found a valid target position, apply
			   the change also to other windows to retain
			   the distance between the windows */
			applyConstraining (constrainRegion, w->id (),
					   dx - gw->destination.x () +
					   gw->orgPos.x (),
					   dy - gw->destination.y () +
					   gw->orgPos.y ());

			/* if we hit constraints, adjust the mask and the
			   target position accordingly */
			if (dx != (gw->destination.x () - gw->orgPos.x ()))
			{
			    gw->animateState |= CONSTRAINED_X;
			    gw->destination.setX (gw->orgPos.x () + dx);
			}

			if (dy != (gw->destination.y () - gw->orgPos.y ()))
			{
			    gw->animateState |= CONSTRAINED_Y;
			    gw->destination.setY (gw->orgPos.y () + dy);
			}

			constrainedWindows = TRUE;
		    }
		}
	    }
	}
    }
}

/*
 * groupRecalcSlotPos
 *
 */
 #if 0
void
Tab::recalcPos (int slotPos)
{
    Group *group;
    CompRect       box;
    int            space, thumbSize;

    GROUP_WINDOW (tab->window);
    GROUP_SCREEN (screen);


    group = gw->group;

    if (!HAS_TOP_WIN (group) || !group->tabBar)
	return;

    space = gs->optionGetThumbSpace ();
    thumbSize = gs->optionGetThumbSize ();

    EMPTY_REGION (slot->region);

    box.setX (space + ((thumbSize + space) * slotPos));
    box.setY (space);

    box.setWidth (thumbSize);
    box.setHeight (thumbSize);

    CompRegion boxReg (box);

    tab->region = tab->region.united (box);
}
#endif

void
TabBar::damageRegion ()
{
    CompRegion reg (region);
    CompRect   bbox;

    /* we use 15 pixels as damage buffer here, as there is a 10 pixel wide
       border around the selected slot which also needs to be damaged
       properly - however the best way would be if slot->region was
       sized including the border */

#define DAMAGE_BUFFER 20

    if (group->tabBar->tabs.size ())
    {
        CompRect tabRect = tabs.front ()->region.boundingRect ();
        bbox.setGeometry (MIN (reg.boundingRect ().x1 (), tabRect.x1 ()),
        		  MIN (reg.boundingRect ().y1 (), tabRect.y1 ()),
        		  MAX (reg.boundingRect ().width (), tabRect.width ()),
        		  MAX (reg.boundingRect ().height (), tabRect.height ()));
    }

    reg = reg.united (CompRect (bbox.x1 () - DAMAGE_BUFFER,
    				bbox.y1 () - DAMAGE_BUFFER,
    				bbox.x2 () - DAMAGE_BUFFER,
    				bbox.y2 () - DAMAGE_BUFFER));

    GroupScreen::get (screen)->cScreen->damageRegion (reg);
}

void
TabBar::moveRegion (int dx, int dy, bool syncIPW)
{
    damageRegion ();

    region.translate (dx, dy);

    if (syncIPW)
    {
	XMoveWindow (screen->dpy (),
		     inputPrevention,
		     leftSpringX,
		     region.boundingRect ().y1 ());
    }

    damageRegion ();
}

void
TabBar::resizeRegion (CompRect box, bool syncIPW)
{
    int fOldWidth;

    GROUP_SCREEN (screen);

    damageRegion ();

    fOldWidth = region.boundingRect ().x2 () -
	region.boundingRect ().x1 ();

    if (bgLayer && fOldWidth != box.width () && syncIPW)
    {
	bgLayer =
	    CairoLayer::rebuildCairoLayer (bgLayer,
					   box.width () +
					   gs->optionGetThumbSpace () +
					   gs->optionGetThumbSize (),
					   box.height ());
	renderTabBarBackground ();

	/* invalidate old width */
	oldWidth = 0;
    }

    region = CompRegion ();

    CompRegion boxReg (box);

    region = region.united (boxReg);

    if (syncIPW)
    {
	XWindowChanges xwc;

	xwc.x = box.x ();
	xwc.y = box.y ();
	xwc.width = box.width ();
	xwc.height = box.height ();

	xwc.stack_mode = Above;

	if (!ipwMapped)
	    XMapWindow (screen->dpy (), inputPrevention);

	XConfigureWindow (screen->dpy (), inputPrevention, CWStackMode | CWX | CWY | CWWidth | CWHeight, &xwc);

	if (!ipwMapped)
	    XUnmapWindow (screen->dpy (), inputPrevention);
    }

    damageRegion ();
}

/*
 * groupInsertTabBarSlotBefore
 *
 */
void
TabBar::insertTabBefore (Tab *tab, Tab *nextTab)
{
    std::list <Tab *>::iterator it = std::find (tabs.begin (), tabs.end (),
						nextTab);

    it--;

    tabs.insert (it, tab);

    /* Moving bar->region->extents.x1 / x2 as minX1 / maxX2 will work,
       because the tab-bar got wider now, so it will put it in
       the average between them, which is
       (bar->region->extents.x1 + bar->region->extents.x2) / 2 anyway. */
    recalcPos ((region.boundingRect  ().x1 () +
		region.boundingRect ().x2 ()) / 2,
		region.boundingRect ().x1 (), region.boundingRect ().x2 ());
}

/*
 * groupInsertTabBarSlotAfter
 *
 */
void
TabBar::insertTabAfter (Tab *tab, Tab *prevTab)
{
    std::list <Tab *>::iterator it = std::find (tabs.begin (), tabs.end (),
						prevTab);

    it++;
    
    tabs.insert (it, tab);

    /* Moving bar->region->extents.x1 / x2 as minX1 / maxX2 will work,
       because the tab-bar got wider now, so it will put it in the
       average between them, which is
       (bar->region->extents.x1 + bar->region->extents.x2) / 2 anyway. */
    recalcPos ((region.boundingRect ().x1 () +
		region.boundingRect ().x2 ()) / 2,
		region.boundingRect ().x1 (), region.boundingRect ().x2 ());
}

/*
 * groupInsertTabBarSlot
 *
 */
void
TabBar::insertTab (Tab *tab)
{
    CompWindow *w = tab->window;

    GROUP_WINDOW (w);

    tabs.push_back (tab);

    /* Moving bar->region->extents.x1 / x2 as minX1 / maxX2 will work,
       because the tab-bar got wider now, so it will put it in
       the average between them, which is
       (bar->region->extents.x1 + bar->region->extents.x2) / 2 anyway. */
    recalcPos ((region.boundingRect ().x1 () +
		region.boundingRect ().x2 ()) / 2,
		region.boundingRect ().x1 (), region.boundingRect ().x2 ());
}

/*
 * groupUnhookTabBarSlot
 *
 */
void
TabBar::unhookTab (Tab *tab,
		   bool            temporary)
{
    TabList::iterator tempit = tabs.begin ();
    CompWindow      *w = tab->window;

    GROUP_WINDOW (w);

    GROUP_SCREEN (screen);

    /* check if slot is not already unhooked */
    tempit = std::find (tabs.begin (), tabs.end (), tab);

    if (tempit == tabs.end ())
	return;

    tabs.remove (tab);

    if (!temporary)
    {
	if (IS_PREV_TOP_TAB (w, group))
	    group->prevTopTab = NULL;
	if (IS_TOP_TAB (w, group))
	{
	    Tab *next, *prev;
	    
	    group->topTab = NULL;

	    if (tabs.getNextTab (tab, next))
		changeTab (next, RotateRight);
	    else if (tabs.getPrevTab (tab, prev))
		changeTab (prev, RotateLeft);

	    if (gs->optionGetUntabOnClose ())
		group->untab ();
	}
    }

    if (tab == hoveredSlot)
	hoveredSlot = NULL;

    if (tab == textSlot)
    {
	textSlot = NULL;

	if (textLayer)
	{
	    if (textLayer->state == Layer::PaintFadeIn ||
		textLayer->state == Layer::PaintOn)
	    {
		textLayer->animationTime =
		    (gs->optionGetFadeTextTime () * 1000) -
		    textLayer->animationTime;
		textLayer->state = Layer::PaintFadeOut;
	    }
	}
    }

    /* Moving region->extents.x1 / x2 as minX1 / maxX2 will work,
       because the tab-bar got thiner now, so
       (region->extents.x1 + region->extents.x2) / 2
       Won't cause the new x1 / x2 to be outside the original region. */
    recalcPos ((region.boundingRect ().x1 () +
		region.boundingRect ().x2 ()) / 2,
		region.boundingRect ().x1 (),
		region.boundingRect ().x2 ());
}

/*
 * groupDeleteTabBarSlot
 *
 */
Tab::~Tab ()
{
    CompWindow *w = window;

    GROUP_WINDOW (w);
    GROUP_SCREEN (screen);

    bar->tabs.remove (this);

    if (this == gs->draggedSlot)
    {
	gs->draggedSlot = NULL;
	gs->dragged = FALSE;

	if (gs->grabState == GroupScreen::ScreenGrabTabDrag)
	    gs->grabScreen (GroupScreen::ScreenGrabNone);
    }
    
    if (bar->group && this == bar->group->topTab)
        bar->group->topTab = NULL; /* XXX: shouldn't toptab be something else here? */

    gw->tab = NULL;
    gw->updateProperty ();
}

/*
 * groupCreateSlot
 *
 */
Tab::Tab (Group *group, CompWindow *w) :
    window (w),
    bar (group->tabBar),
    springX (0),
    speed (0),
    msSinceLastMove (0)
{
    GROUP_WINDOW (w);

    if (group->tabBar)
    {
	group->tabBar->insertTab (this);
	gw->tab = this;
	gw->updateProperty ();
    }
}

#define SPRING_K     gs->optionGetDragSpringK()
#define FRICTION     gs->optionGetDragFriction()
#define SIZE	     gs->optionGetThumbSize()
#define BORDER	     gs->optionGetBorderRadius()
#define Y_START_MOVE gs->optionGetDragYDistance()
#define SPEED_LIMIT  gs->optionGetDragSpeedLimit()

/*
 * groupSpringForce
 *
 */
static inline int
groupSpringForce (int        centerX,
		  int        springX)
{
    GROUP_SCREEN (screen);

    /* Each slot has a spring attached to it, starting at springX,
       and ending at the center of the slot (centerX).
       The spring will cause the slot to move, using the
       well-known physical formula F = k * dl... */
    return -SPRING_K * (centerX - springX);
}

/*
 * groupDraggedSlotForce
 *
 */
static int
groupDraggedSlotForce (int        distanceX,
		       int        distanceY)
{
    GROUP_SCREEN (screen);
    /* The dragged slot will make the slot move, to get
       DnD animations (slots will make room for the newly inserted slot).
       As the dragged slot is closer to the slot, it will put
       more force on the slot, causing it to make room for the dragged slot...
       But if the dragged slot gets too close to the slot, they are
       going to be reordered soon, so the force will get lower.

       If the dragged slot is in the other side of the slot,
       it will have to make force in the opposite direction.

       So we the needed funtion is an odd function that goes
       up at first, and down after that.
       Sinus is a function like that... :)

       The maximum is got when x = (x1 + x2) / 2,
       in this case: x = SIZE + BORDER.
       Because of that, for x = SIZE + BORDER,
       we get a force of SPRING_K * (SIZE + BORDER) / 2.
       That equals to the force we get from the the spring.
       This way, the slot won't move when its distance from
       the dragged slot is SIZE + BORDER (which is the default
       distance between slots).
       */

    /* The maximum value */
    float a = SPRING_K * (SIZE + BORDER) / 2;
    /* This will make distanceX == 2 * (SIZE + BORDER) to get 0,
       and distanceX == (SIZE + BORDER) to get the maximum. */
    float b = PI /  (2 * SIZE + 2 * BORDER);

    /* If there is some distance between the slots in the y axis,
       the slot should get less force... For this, we change max
       to a lower value, using a simple linear function. */

    if (distanceY < Y_START_MOVE)
	a *= 1.0f - (float)distanceY / Y_START_MOVE;
    else
	a = 0;

    if (abs (distanceX) < 2 * (SIZE + BORDER))
	return a * sin (b * distanceX);
    else
	return 0;
}

/*
 * groupApplyFriction
 *
 */
static inline void
groupApplyFriction (int        *speed)
{
    GROUP_SCREEN (screen);
    
    if (abs (*speed) < FRICTION)
	*speed = 0;
    else if (*speed > 0)
	*speed -= FRICTION;
    else if (*speed < 0)
	*speed += FRICTION;
}

/*
 * groupApplySpeedLimit
 *
 */
static inline void
groupApplySpeedLimit (int        *speed)
{
    GROUP_SCREEN (screen);

    if (*speed > SPEED_LIMIT)
	*speed = SPEED_LIMIT;
    else if (*speed < -SPEED_LIMIT)
	*speed = -SPEED_LIMIT;
}

/*
 * groupApplyForces
 *
 */
void
TabBar::applyForces (Tab *draggedSlot)
{
    Tab *tab, *tab2;
    int             centerX, centerY;
    int             draggedCenterX, draggedCenterY;
    
    GROUP_SCREEN (screen);

    if (draggedSlot)
    {
	int vx, vy;

	draggedSlot->getDrawOffset (vx, vy);

	draggedCenterX = ((draggedSlot->region.boundingRect ().x1 () +
			   draggedSlot->region.boundingRect ().x2 ()) / 2) + vx;
	draggedCenterY = ((draggedSlot->region.boundingRect ().y1 () +
			   draggedSlot->region.boundingRect ().y2 ()) / 2) + vy;
    }
    else
    {
	draggedCenterX = 0;
	draggedCenterY = 0;
    }

    leftSpeed += groupSpringForce(region.boundingRect ().x1 (),
				       leftSpringX);
    rightSpeed += groupSpringForce (region.boundingRect ().x2 (),
					rightSpringX);

    if (draggedSlot)
    {
	int leftForce, rightForce;

	leftForce = groupDraggedSlotForce(region.boundingRect ().x1 () -
					  SIZE / 2 - draggedCenterX,
					  abs ((region.boundingRect ().y1 () +
						region.boundingRect ().y2 ()) / 2 -
					       draggedCenterY));

	rightForce = groupDraggedSlotForce (region.boundingRect ().x2 () +
					    SIZE / 2 - draggedCenterX,
					    abs ((region.boundingRect ().y1 () +
						  region.boundingRect ().y2 ()) / 2 -
						 draggedCenterY));

	if (leftForce < 0)
	    leftSpeed += leftForce;
	if (rightForce > 0)
	    rightSpeed += rightForce;
    }

    foreach (tab, tabs)
    {
	centerX = (tab->region.boundingRect ().x1 () + tab->region.boundingRect ().x2 ()) / 2;
	centerY = (tab->region.boundingRect ().y1 () + tab->region.boundingRect ().y2 ()) / 2;

	tab->speed += groupSpringForce (centerX, tab->springX);

	if (draggedSlot && draggedSlot != tab)
	{
	    int draggedSlotForce;
	    draggedSlotForce =
		groupDraggedSlotForce(centerX - draggedCenterX,
				      abs (centerY - draggedCenterY));

	    tab->speed += draggedSlotForce;
	    tab2 = NULL;

	    if (draggedSlotForce < 0)
	    {
		tabs.getPrevTab (tab, tab2);
		leftSpeed += draggedSlotForce;
	    }
	    else if (draggedSlotForce > 0)
	    {
		tabs.getNextTab (tab, tab2);
		rightSpeed += draggedSlotForce;
	    }

	    while (tab2)
	    {
	        Tab *prevTab2, *nextTab2;
	        
		if (tab2 != draggedSlot)
		    tab2->speed += draggedSlotForce;
		    
		if (!tabs.getPrevTab (tab2, prevTab2))
		    prevTab2 = NULL;
		
		if (!tabs.getNextTab (tab2, nextTab2))
		    nextTab2 = NULL;
		    
		tab2 = (draggedSlotForce < 0) ? prevTab2 : nextTab2;
	    }
	}
    }

    foreach (Tab *tab, tabs)
    {
	groupApplyFriction (&tab->speed);
	groupApplySpeedLimit (&tab->speed);
    }

    groupApplyFriction (&leftSpeed);
    groupApplySpeedLimit (&leftSpeed);

    groupApplyFriction (&rightSpeed);
    groupApplySpeedLimit (&rightSpeed);
}

/*
 * groupApplySpeeds
 *
 */
void
TabBar::applySpeeds (int            msSinceLastRepaint)
{
    Tab *slot;
    int             move;
    CompRect        box;
    Bool            updateTabBar = FALSE;
    
    GROUP_SCREEN (screen);

    box.setGeometry (region.boundingRect ().x1 (), region.boundingRect ().y1 (),
		     region.boundingRect ().x2 () - region.boundingRect ().x1 (),
		     region.boundingRect ().y2 () - region.boundingRect ().y1 ());

    leftMsSinceLastMove += msSinceLastRepaint;
    rightMsSinceLastMove += msSinceLastRepaint;

    /* Left */
    move = leftSpeed * leftMsSinceLastMove / 1000;
    if (move)
    {
	box.setX (box.x () + move);
	box.setWidth (box.width () - move);

	leftMsSinceLastMove = 0;
	updateTabBar = TRUE;
    }
    else if (leftSpeed == 0 &&
	     region.boundingRect ().x1 () != leftSpringX &&
	     (SPRING_K * abs (region.boundingRect ().x1 () - leftSpringX) <
	      FRICTION))
    {
	/* Friction is preventing from the left border to get
	   to its original position. */
	box.setX (box.x () + leftSpringX - region.boundingRect ().x1 ());
	box.setWidth (box.width () - leftSpringX - region.boundingRect ().x1 ());

	leftMsSinceLastMove = 0;
	updateTabBar = TRUE;
    }
    else if (leftSpeed == 0)
	leftMsSinceLastMove = 0;

    /* Right */
    move = rightSpeed * rightMsSinceLastMove / 1000;
    if (move)
    {
	box.setWidth (box.width () + move);

	rightMsSinceLastMove = 0;
	updateTabBar = TRUE;
    }
    else if (rightSpeed == 0 &&
	     region.boundingRect ().x2 () != rightSpringX &&
	     (SPRING_K * abs (region.boundingRect ().x2 () - rightSpringX) <
	      FRICTION))
    {
	/* Friction is preventing from the right border to get
	   to its original position. */
	box.setWidth (box.width () + leftSpringX - region.boundingRect ().x1 ());

	leftMsSinceLastMove = 0;
	updateTabBar = TRUE;
    }
    else if (rightSpeed == 0)
	rightMsSinceLastMove = 0;

    if (updateTabBar)
	resizeRegion (box, FALSE);

    foreach (Tab *tab, tabs)
    {
	int slotCenter;

	tab->msSinceLastMove += msSinceLastRepaint;
	move = tab->speed * tab->msSinceLastMove / 1000;
	slotCenter = (tab->region.boundingRect ().x1 () +
		      tab->region.boundingRect ().x2 ()) / 2;

	if (move)
	{
	    tab->region.translate (move, 0);
	    tab->msSinceLastMove = 0;
	}
	else if (tab->speed == 0 &&
		 slotCenter != tab->springX &&
		 SPRING_K * abs (slotCenter - tab->springX) < FRICTION)
	{
	    /* Friction is preventing from the slot to get
	       to its original position. */

	    tab->region.translate (tab->springX - slotCenter, 0);
	    tab->msSinceLastMove = 0;
	}
	else if (tab->speed == 0)
	    tab->msSinceLastMove = 0;
    }
}

/*
 * groupSwitchTopTabInput
 *
 */
void
TabBar::switchTopTabInput (bool enable)
{

    if (!HAS_TOP_WIN (group))
	return;

    if (!inputPrevention)
	createIPW ();

    if (!enable)
    {
	XMapWindow (screen->dpy (),
		    inputPrevention);
    }
    else
    {
	XUnmapWindow (screen->dpy (),
		      inputPrevention);
     }

    ipwMapped = !enable;
}

