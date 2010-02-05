/**
 *
 * Compiz group plugin
 *
 * screen.cpp
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

/**
 * GroupScreen::grabScreen:
 * This function is really a wrapper around screen->pushGrab ().
 * We set a state and it automatically handles setting the correct grab.
 * Also makes things a little more simple
 */
void
GroupScreen::grabScreen (GroupScreen::GrabState newState)
{
    if ((grabState != newState) && grabIndex)
    {
	screen->removeGrab (grabIndex, 0);
	grabIndex = 0;
    }

    if (newState == ScreenGrabSelect)
    {
	grabIndex = screen->pushGrab (None, "group");
    }
    else if (newState == ScreenGrabTabDrag)
    {
	grabIndex = screen->pushGrab (None, "group-drag");
    }

    grabState = newState;
}

/*
 * GroupScreen::handleMotionEvent
 *
 */

/* the radius to determine if it was a click or a drag */
#define RADIUS 5

void
GroupScreen::handleMotionEvent (int        xRoot,
				int        yRoot)
{
    Group *group;

    if (grabState == ScreenGrabTabDrag)
    {
	int    dx, dy;
	int    vx, vy;
	CompRegion draggedRegion = draggedSlot->region;

	dx = xRoot - prevX;
	dy = yRoot - prevY;

	if (dragged || abs (dx) > RADIUS || abs (dy) > RADIUS)
	{
	    prevX = xRoot;
	    prevY = yRoot;

	    if (!dragged)
	    {
		CompRect box;

		GROUP_WINDOW (draggedSlot->window);

		dragged = TRUE;

		foreach (group, groups)
		{
		    if (group->tabBar)
			group->tabBar->setVisibility (true, PERMANENT);
		}

		box = gw->group->tabBar->region.boundingRect ();
		if (group->tabBar)
		    group->tabBar->recalcPos ((box.x1 () + box.x2 ()) / 2,
				      	       box.x1 (), box.x2 ());
	    }

	    draggedSlot->getDrawOffset (vx, vy);

	    CompRect rect;
	    
	    rect.setGeometry (draggedRegion.boundingRect ().x1 () + vx,
	    		      draggedRegion.boundingRect ().y1 () + vy,
	    		      draggedRegion.boundingRect ().width () + vx,
	    		      draggedRegion.boundingRect ().height () +vy);

	    CompRegion reg (rect);

	    cScreen->damageRegion (reg);

	    draggedSlot->region.translate (dx, dy);
	    draggedSlot->springX =
		(draggedSlot->region.boundingRect ().x1 () +
		 draggedSlot->region.boundingRect ().x2 ()) / 2;

	    rect.setGeometry (draggedRegion.boundingRect ().x1 () + vx,
	    		      draggedRegion.boundingRect ().y1 () + vy,
	    		      draggedRegion.boundingRect ().width () + vx,
	    		      draggedRegion.boundingRect ().height () +vy);

	    CompRegion reg2 (rect);

	    cScreen->damageRegion (reg2);
	}
    }
    else if (grabState == ScreenGrabSelect)
    {
	masterSelectionRect.damage (xRoot, yRoot);
    }
}

/*
 * groupHandleButtonPressEvent
 *
 */
void
GroupScreen::handleButtonPressEvent (XEvent *event)
{
    int            xRoot, yRoot, button;

    xRoot  = event->xbutton.x_root;
    yRoot  = event->xbutton.y_root;
    button = event->xbutton.button;

    

    foreach (Group *group, groups)
    {
	if (!group->tabBar)
	    continue;
	    
	if (group->tabBar->inputPrevention != event->xbutton.window)
	    continue;

	switch (button) {
	case Button1:
	    {
		foreach (Tab *tab, group->tabBar->tabs)
		{
		    if (tab->region.contains (CompPoint (xRoot, yRoot)))
		    {
			draggedSlot = tab;
			/* The slot isn't dragged yet */
			dragged = FALSE;
			prevX = xRoot;
			prevY = yRoot;

			if (!screen->otherGrabExist ("group", "group-drag",
									  NULL))
			    grabScreen (ScreenGrabTabDrag);
		    }
		}
	    }
	    break;

	case Button4:
	case Button5:
	    {	        
		CompWindow  *ftopTab = NULL;
		TabList &tabs = group->tabBar->tabs;
		GroupWindow *gw;

		if (group->tabBar->nextTopTab)
		    ftopTab = NEXT_TOP_TAB (group);
		else if (group->tabBar->topTab)
		{
		    /* If there are no tabbing animations,
		       topTab is never NULL. */
		    ftopTab = TOP_TAB (group);
		}

		if (!ftopTab)
		    return;

		gw = GroupWindow::get (ftopTab);

		if (button == Button4)
		{
		    if (gw->tab != tabs.front ())
		    {
		        Tab *prev;
		        tabs.getPrevTab (gw->tab, prev);
			group->tabBar->changeTab (prev, TabBar::RotateLeft);
		    }
		    else
		    {
			group->tabBar->changeTab (tabs.back (),
					          TabBar::RotateLeft);
		    }
		}
		else
		{
		    if (gw->tab != tabs.back ())
		    {
		        Tab *next;
		        tabs.getNextTab (gw->tab, next);
			group->tabBar->changeTab (next, TabBar::RotateRight);
		    }
		    else
		    {
			group->tabBar->changeTab (tabs.front (),
							   TabBar::RotateRight);
		    }
		}
		break;
	    }
	}

	break;
    }
}

/*
 * groupHandleButtonReleaseEvent
 *
 */
void
GroupScreen::handleButtonReleaseEvent (XEvent *event)
{
    int            vx, vy;
    CompRegion     newRegion;
    Bool           inserted = FALSE;
    Bool           wasInTabBar = FALSE;

    if (event->xbutton.button != 1)
	return;

    if (!draggedSlot)
    {
	return;
    }

    if (!dragged)
    {
	draggedSlot->bar->changeTab (draggedSlot, TabBar::RotateUncertain);
	draggedSlot = NULL;

	if (grabState == ScreenGrabTabDrag)
	    grabScreen (ScreenGrabNone);

	return;
    }

    GROUP_WINDOW (draggedSlot->window);

    newRegion = newRegion.united (draggedSlot->region);
    
    draggedSlot->getDrawOffset (vx, vy);
    
    newRegion.translate (vx, vy);

    foreach (Group *group, groups)
    {
	Bool            inTabBar;
	CompRegion      clip, buf;
	TabList::iterator it;
	//Tab             *tab;

	if (!group->tabBar || !HAS_TOP_WIN (group))
	    continue;

	/* create clipping region */
	clip = GroupWindow::get (TOP_TAB (group))->getClippingRegion ();
	if (clip.isEmpty ())
	{
	    continue;
	}

	buf = group->tabBar->region.intersected (newRegion);
	buf = buf.subtracted (clip);

	inTabBar = !buf.isEmpty ();

	if (!inTabBar)
	{
	    continue;
	}

	wasInTabBar = TRUE;

	for (it = group->tabBar->tabs.begin ();
	     it != group->tabBar->tabs.end ();
	     it++)
	{
	    Tab             *tab = *it;
	    Tab             *tmpDraggedTab;
	    Tab		    *prevTab = NULL, *nextTab = NULL;
	    Tab		    *draggedSlotSideTab;
	    Group           *tmpGroup;
	    CompRegion      slotRegion, buf;
	    CompRect        rect;
	    Bool            inSlot;

	    if (tab == draggedSlot)
		continue;


	    /* XXX: there must be stack smashing here */
	    if (!group->tabBar->tabs.getPrevTab (tab, prevTab))
	        prevTab = NULL;
	    
	    if (!group->tabBar->tabs.getNextTab (tab, nextTab))
	        nextTab = NULL;

	    if (prevTab && prevTab != draggedSlot)
	    {
		rect.setX (prevTab->region.boundingRect ().x2 ());
	    }
	    else if (prevTab && prevTab == draggedSlot
	             && draggedSlot->bar->tabs.getPrevTab (draggedSlot,
	             					   draggedSlotSideTab))
	    {	        
		rect.setX (draggedSlotSideTab->region.boundingRect ().x2 ());
	    }
	    else
		rect.setY (group->tabBar->region.boundingRect ().x1 ());

	    rect.setY (tab->region.boundingRect ().y1 ());

	    if (nextTab && nextTab != draggedSlot)
	    {
		rect.setWidth (nextTab->region.boundingRect ().x1 () - 
								     rect.x ());
	    }
	    else if (nextTab && nextTab == draggedSlot &&
	    	     draggedSlot->bar->tabs.getNextTab (draggedSlot,
	    	     					draggedSlotSideTab))
	    {
	        rect.setWidth (draggedSlotSideTab->region.boundingRect ().x1 () - 
	        						     rect.x ());
	    }
	    else
		rect.setWidth (group->tabBar->region.boundingRect ().x2 ());

	    rect.setHeight (tab->region.boundingRect ().height ());

	    slotRegion = CompRegion (rect);

	    buf = slotRegion.intersected (newRegion);
	    inSlot = !buf.isEmpty ();
	    
	    buf = CompRegion ();
	    slotRegion = CompRegion ();

	    if (!inSlot)
	    {
		continue;
	    }

	    tmpDraggedTab = draggedSlot;

	    if (group != gw->group)
	    {
		CompWindow     *w = draggedSlot->window;
		Group          *tmpGroup = gw->group;
		int            oldPosX = WIN_CENTER_X (w);
		int            oldPosY = WIN_CENTER_Y (w);

		/* if the dragged window is not the top tab,
		   move it onscreen */
		if (tmpGroup->tabBar->topTab && !IS_TOP_TAB (w, tmpGroup))
		{
		    CompWindow *tw = TOP_TAB (tmpGroup);

		    oldPosX = WIN_CENTER_X (tw) + gw->mainTabOffset.x ();
		    oldPosY = WIN_CENTER_Y (tw) + gw->mainTabOffset.y ();

		    gw->setVisibility (true);
		}

		/* Change the group. */
		GroupWindow::get (draggedSlot->window)->deleteGroupWindow ();
		group->addWindow (draggedSlot->window);

		/* we saved the original center position in oldPosX/Y before -
		   now we should apply that to the new main tab offset */
		if (HAS_TOP_WIN (group))
		{
		    CompWindow *tw = TOP_TAB (group);
		    gw->mainTabOffset.setX (oldPosX - WIN_CENTER_X (tw));
		    gw->mainTabOffset.setY (oldPosY - WIN_CENTER_Y (tw));
		}
	    }
	    else
		group->tabBar->unhookTab (draggedSlot, TRUE);

	    draggedSlot = NULL;
	    dragged = FALSE;
	    inserted = TRUE;

	    if ((tmpDraggedTab->region.boundingRect ().x1 () +
		 tmpDraggedTab->region.boundingRect ().x2 () + (2 * vx)) / 2 >
		(tab->region.boundingRect ().x1 () +
					 tab->region.boundingRect ().x2 ()) / 2)
	    {
		group->tabBar->insertTabAfter (tmpDraggedTab, tab);
	    }
	    else
	    {
		group->tabBar->insertTabBefore (tmpDraggedTab, tab);
	    }

	    group->tabBar->damageRegion ();

	    /* Hide tab-bars. */
	    foreach (tmpGroup, groups)
	    {
		if (group == tmpGroup && tmpGroup->tabBar)
		    tmpGroup->tabBar->setVisibility (TRUE, 0);
		else if (tmpGroup->tabBar)
		    tmpGroup->tabBar->setVisibility (FALSE, PERMANENT);
	    }

	    break;
	}

	if (inserted)
	    break;
    }

    newRegion = CompRegion ();

    if (!inserted)
    {
	CompWindow     *draggedSlotWindow = draggedSlot->window;
	Group          *tmpGroup;

	foreach (tmpGroup, groups)
	{
	    if (tmpGroup->tabBar)
		tmpGroup->tabBar->setVisibility (FALSE, PERMANENT);
	}

	draggedSlot = NULL;
	dragged = FALSE;

	if (optionGetDndUngroupWindow () && !wasInTabBar)
	{
	    GroupWindow::get (draggedSlotWindow)->removeFromGroup ();
	}
	else if (gw->group && gw->group->tabBar->topTab)
	{
	    gw->group->tabBar->recalcPos (
	    		  (gw->group->tabBar->region.boundingRect ().x1 () +
			   gw->group->tabBar->region.boundingRect ().x2 ()) / 2,
			   gw->group->tabBar->region.boundingRect ().x1 (),
			   gw->group->tabBar->region.boundingRect ().x2 ());
	}

	/* to remove the painted slot */
	cScreen->damageScreen ();
    }

    if (grabState == ScreenGrabTabDrag)
	grabScreen (ScreenGrabNone);

    if (dragHoverTimeoutHandle.active ())
    {
        dragHoverTimeoutHandle.stop ();
    }
}


/** GroupScreen::handleEvent
  * Out main event handler queue. This takes X Events as input
  * and uses them likewise. The more complex events have beeen
  * handled in separate functions to reduce the size of this
  * function
  */

void
GroupScreen::handleEvent (XEvent *event)
{
    CompWindow *w;

    switch (event->type) {
    case MotionNotify:
	handleMotionEvent (pointerX, pointerY);
	break;

    case ButtonPress:
	handleButtonPressEvent (event);
	break;

    case ButtonRelease:
	handleButtonReleaseEvent (event);
	break;

    case MapNotify:
	/* Unmap frame windows that are supposed to be hidden */
	w = screen->findWindow (event->xmap.window);
	if (w)
	{
	    foreach (CompWindow *cw, screen->windows ())
	    {
		if (w->id () == cw->frame ())
		{
		    GROUP_WINDOW (cw);
		    if (gw->windowHideInfo)
			XUnmapWindow (screen->dpy (), cw->frame ());
		}
	    }
	}
	break;

    case UnmapNotify:
	/* Set internal window state depending on how the window was
	 * unmapped. If it was a shade or a minimize, unmap all windows in
	 * the group in the same way
	 * XXX: Shoud really reimplement this using the new wrappable functions
	 */
	w = screen->findWindow (event->xunmap.window);
	if (w)
	{
	    GROUP_WINDOW (w);

	    if (gw->group)
	    {
		if (gw->group->tabBar && IS_TOP_TAB (w, gw->group))
		{
		    /* on unmap of the top tab, hide the tab bar and the
		       input prevention window */
		    gw->group->tabBar->setVisibility (FALSE, PERMANENT);
		}
		if (!w->pendingUnmaps ())
		{
		    /* close event */
		    if (!(gw->animateState & IS_UNGROUPING))
		    {
			gw->deleteGroupWindow ();
			cScreen->damageScreen ();
		    }
		}
	    }
	}
	break;

    case ClientMessage:
	if (event->xclient.message_type == Atoms::winActive)
	{
	    w = screen->findWindow (event->xclient.window);
	    if (w)
	    {
		GROUP_WINDOW (w);

		if (gw->group && gw->group->tabBar &&
		    !IS_TOP_TAB (w, gw->group))
		{
		    gw->group->tabBar->checkFocusAfterTabChange = TRUE;
		    gw->group->tabBar->changeTab (gw->tab,
		    				       TabBar::RotateUncertain);
		}
	    }
	}
	else if (event->xclient.message_type == resizeNotifyAtom)
	/* We don't use resizeNotify wrappable function because that is only
	 * called when the window size changes on the X Server side of things.
	 * The resize plugin sets this atom whenever a window is visually
	 * "resized" to the user [i.e stretch, rectangle etc]
	 */
	{
	    CompWindow *w;
	    w = screen->findWindow (event->xclient.window);

	    if (w && resizeInfo && (w == resizeInfo->resizedWindow))
	    {
		GROUP_WINDOW (w);

		if (gw->group)
		{
		    CompRect   rect;

		    rect.setX (event->xclient.data.l[0]);
		    rect.setY (event->xclient.data.l[1]);
		    rect.setWidth (event->xclient.data.l[2]);
		    rect.setHeight (event->xclient.data.l[3]);

		    foreach (CompWindow *cw, gw->group->windows)
		    {
			GroupWindow *gcw;

			gcw = GroupWindow::get (cw);
			if (!gcw->resizeGeometry.isEmpty ())
			{
			    if (gcw->updateResizeRectangle (rect, true))
			    {
				gcw->cWindow->addDamage ();
			    }
			}
		    }
		}
	    }
	}
	break;

    default:
	if (event->type == screen->shapeEvent () + ShapeNotify)
	{
	    XShapeEvent *se = (XShapeEvent *) event;
	    if (se->kind == ShapeInput)
	    {
		CompWindow *w;
		w = screen->findWindow (se->window);
		if (w)
		{
		    GROUP_WINDOW (w);

		    if (gw->windowHideInfo)
			gw->clearInputShape (gw->windowHideInfo);
		}
	    }
	}
	break;
    }

    screen->handleEvent (event);

    /* Everything here happens after core handles events */

    switch (event->type) {
    case PropertyNotify:
	if (event->xproperty.atom == Atoms::wmName)
	{
	    CompWindow *w;
	    w = screen->findWindow (event->xproperty.window);
	    if (w)
	    {
		GROUP_WINDOW (w);

		if (gw->group && gw->group->tabBar &&
		    gw->group->tabBar->textSlot    &&
		    gw->group->tabBar->textSlot->window == w)
		{
		    /* make sure we are using the updated name */

		    if (gw->group->tabBar->textLayer)
			delete gw->group->tabBar->textLayer;

		    gw->group->tabBar->textLayer = new TextLayer;

		    gw->group->tabBar->renderWindowTitle ();

		    gw->group->tabBar->damageRegion ();
		}
	    }
	}
	break;

    case EnterNotify:
	{
	    CompWindow *w;
	    w = screen->findWindow (event->xcrossing.window);
	    
	    if (!w)
	    {
	        foreach (CompWindow *cw, screen->windows ())
	        {
	            if (cw->frame () == event->xcrossing.window)
	            {
	        	w = cw;
	        	break;
	            }
	        }
	    }
	        
	    
	    if (w)
	    {
		GROUP_WINDOW (w);

		if (showDelayTimeoutHandle.active ())
		    showDelayTimeoutHandle.stop ();
		
		if (gw->group)
		{

		    if (w->id () != gw->group->grabWindow)
		        updateTabBars (event->xcrossing.window);

		    if (draggedSlot && dragged &&
			IS_TOP_TAB (w, gw->group))
		    {
			/* We entered a group with a dragged window slot */

			int hoverTime;
			hoverTime = optionGetDragHoverTime () * 1000;
			if (dragHoverTimeoutHandle.active ())
			    dragHoverTimeoutHandle.stop ();

			if (hoverTime > 0)
			{
			    dragHoverTimeoutHandle.setTimes (hoverTime,
							     hoverTime * 1.2);

			    dragHoverTimeoutHandle.setCallback (
				boost::bind (&GroupScreen::dragHoverTimeout,
					     this, w));

			    dragHoverTimeoutHandle.start (); /* XXX */
			}
		    }
		}
	    }
	}
	break;

    case ConfigureNotify:
	{
	    CompWindow *w;

	    w = screen->findWindow (event->xconfigure.window);
	    if (w)
	    {
		GROUP_WINDOW (w);

		if (gw->group && gw->group->tabBar &&
		    IS_TOP_TAB (w, gw->group)      &&
		    gw->group->tabBar->inputPrevention && 
		    gw->group->tabBar->ipwMapped)
		{
		    XWindowChanges xwc;

		    xwc.stack_mode = Above;
		    xwc.sibling = w->id ();

		    XConfigureWindow (screen->dpy (),
				      gw->group->tabBar->inputPrevention,
				      CWSibling | CWStackMode, &xwc);
		}
	    }
	}
	break;

    default:
	break;
	
    }
}


/**
 * GroupScreen::preparePaint
 * Handles animation etc
 *
 */
void
GroupScreen::preparePaint (int        msSinceLastPaint)
{
    std::list <Group *>::iterator it = groups.begin ();
    cScreen->preparePaint (msSinceLastPaint);
    
    /* TabBar::drawTabAnimation could call Group::finishTabbing which implicitly
     * free's the *group here since the memory is free'd immediately.
     */

    while (it != groups.end ())
    {
        Group *group = *it;
        
        it++;
        
        if (!group)
            continue; /* XXX */
        
	TabBar *bar = group->tabBar;

	if (bar)
	{
	    bar->applyForces ((dragged) ? draggedSlot : NULL);
	    bar->applySpeeds (msSinceLastPaint);

	    if ((bar->state != Layer::PaintOff) && HAS_TOP_WIN (group))
		group->handleHoverDetection ();

	    if (bar->state == Layer::PaintFadeIn ||
	        bar->state == Layer::PaintFadeOut)
		bar->handleFade (msSinceLastPaint);

	    if (bar->textLayer)
		bar->handleTextFade (msSinceLastPaint);

	    if (bar->bgAnimation)
		bar->handleAnimation (msSinceLastPaint);
	
	    if (bar->changeState != TabBar::NoTabChange)
	    {
	        bar->changeAnimationTime -= msSinceLastPaint;
	        if (bar->changeAnimationTime <= 0)
		    group->handleAnimation ();
	    }
	    
	    if (group->tabBar->tabbingState != TabBar::NoTabbing)
		group->drawTabAnimation (msSinceLastPaint);
	}
    }
}

/**
 * GroupScreen::glPaintOutput
 * Draws our own stuff using openGL on the output. Note that this is
 * all done _after_ the chain is passed so it goes on top of windows
 *
 */
bool
GroupScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
			    const GLMatrix	      &transform,
			    const CompRegion	      &region,
			    CompOutput 		      *output,
			    unsigned int	      mask)
{
    bool           status;

    painted = FALSE;
    vpX = screen->vp ().x ();
    vpY = screen->vp ().y ();

    if (resizeInfo)
    {
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
    }
    else
    {
	foreach (Group *group, groups)
	{
	    if (group->tabBar && (
	    	group->tabBar->changeState != TabBar::NoTabChange ||
		group->tabBar->tabbingState != TabBar::NoTabbing))
	    {
		mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
	    }
	    else if (group->tabBar && (group->tabBar->state != Layer::PaintOff))
	    {
		mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
	    }
	}
    }

    status = gScreen->glPaintOutput (attrib, transform, region, output, mask);

    if (status && !painted)
    {
	if ((grabState == ScreenGrabTabDrag) && draggedSlot)
	{
	    GLMatrix wTransform = transform;
	    Layer::PaintState    state;

	    GROUP_WINDOW (draggedSlot->window);

	    wTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

	    glPushMatrix ();
	    glLoadMatrixf (wTransform.getMatrix ());

	    /* prevent tab bar drawing.. */

	    state = gw->group->tabBar->state;
	    gw->group->tabBar->state = Layer::PaintOff;
	    draggedSlot->paint (gw->group, wTransform, OPAQUE);
	    gw->group->tabBar->state = state;

	    glPopMatrix ();
	}
	else  if (grabState == ScreenGrabSelect)
	{
	    masterSelectionRect.paint (attrib, transform, output, FALSE);
	}
    }

    return status;
}

/*
 * GroupScreen::glPaintTransformedOutput
 * Adjusts thumbnail painting etc if the screen is transformed
 *
 */

void
GroupScreen::glPaintTransformedOutput (const GLScreenPaintAttrib &attrib,
				       const GLMatrix	         &transform,
				       const CompRegion	         &region,
				       CompOutput		 *output,
				       unsigned int	         mask)
{

    gScreen->glPaintTransformedOutput (attrib, transform, region, output, mask);

    if ((vpX == screen->vp ().x ()) && (vpY == screen->vp ().y ()))
    {
	painted = TRUE;

	if ((grabState == ScreenGrabTabDrag) &&
	    draggedSlot && dragged)
	{
	    GLMatrix wTransform = transform;

	    gScreen->glApplyTransform (attrib, output, &wTransform);
	    wTransform.toScreenSpace (output, -attrib.zTranslate);
	    glPushMatrix ();
	    glLoadMatrixf (wTransform.getMatrix ());

	    draggedSlot->paint (GroupWindow::get (draggedSlot->window)->group,
	    		        wTransform, OPAQUE);

	    glPopMatrix ();
	}
	else if (grabState == ScreenGrabSelect)
	{
	    masterSelectionRect.paint (attrib, transform, output, FALSE);
	}
    }
}

/*
 * GroupScreen::donePaint
 * Handles damage
 *
 */
void
GroupScreen::donePaint ()
{

    cScreen->donePaint ();

    foreach (Group *group, groups)
    {
	if (group->tabBar && group->tabBar->tabbingState != TabBar::NoTabbing)
	    cScreen->damageScreen ();
	else if (group->tabBar && group->tabBar->changeState != TabBar::NoTabChange)
	    cScreen->damageScreen ();
	else if (group->tabBar)
	{
	    Bool needDamage = FALSE;

	    if ((group->tabBar->state == Layer::PaintFadeIn) ||
		(group->tabBar->state == Layer::PaintFadeOut))
	    {
		needDamage = TRUE;
	    }

	    if (group->tabBar->textLayer)
	    {
		if ((group->tabBar->textLayer->state == Layer::PaintFadeIn) ||
		    (group->tabBar->textLayer->state == Layer::PaintFadeOut))
		{
		    needDamage = TRUE;
		}
	    }

	    if (group->tabBar->bgAnimation)
		needDamage = TRUE;

	    if (draggedSlot)
		needDamage = TRUE;

	    if (needDamage)
		group->tabBar->damageRegion (); 
	}
    }
}
