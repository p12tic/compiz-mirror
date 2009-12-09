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
    if (grabState == ScreenGrabTabDrag)
    {
	#if 0
	int    dx, dy;
	int    vx, vy;
	REGION reg;
	Region draggedRegion = gs->draggedSlot->region;

	reg.rects = &reg.extents;
	reg.numRects = 1;

	dx = xRoot - gs->prevX;
	dy = yRoot - gs->prevY;

	if (gs->dragged || abs (dx) > RADIUS || abs (dy) > RADIUS)
	{
	    gs->prevX = xRoot;
	    gs->prevY = yRoot;

	    if (!gs->dragged)
	    {
		GroupSelection *group;
		BoxRec         *box;

		GROUP_WINDOW (gs->draggedSlot->window);

		gs->dragged = TRUE;

		for (group = gs->groups; group; group = group->next)
		    groupTabSetVisibility (group, TRUE, PERMANENT);

		box = &gw->group->tabBar->region->extents;
		groupRecalcTabBarPos (gw->group, (box->x1 + box->x2) / 2,
				      box->x1, box->x2);
	    }

	    groupGetDrawOffsetForSlot (gs->draggedSlot, &vx, &vy);

	    reg.extents.x1 = draggedRegion->extents.x1 + vx;
	    reg.extents.y1 = draggedRegion->extents.y1 + vy;
	    reg.extents.x2 = draggedRegion->extents.x2 + vx;
	    reg.extents.y2 = draggedRegion->extents.y2 + vy;
	    damageScreenRegion (s, &reg);

	    XOffsetRegion (gs->draggedSlot->region, dx, dy);
	    gs->draggedSlot->springX =
		(gs->draggedSlot->region->extents.x1 +
		 gs->draggedSlot->region->extents.x2) / 2;

	    reg.extents.x1 = draggedRegion->extents.x1 + vx;
	    reg.extents.y1 = draggedRegion->extents.y1 + vy;
	    reg.extents.x2 = draggedRegion->extents.x2 + vx;
	    reg.extents.y2 = draggedRegion->extents.y2 + vy;
	    damageScreenRegion (s, &reg);
	}
	#endif
    }
    else if (grabState == ScreenGrabSelect)
    {
	masterSelectionRect.damage (xRoot, yRoot);
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
	//handleButtonPressEvent (event);
	break;

    case ButtonRelease:
	//handleButtonReleaseEvent (event);
	break;

    case MapNotify:
	/* Unmap frame windows that are supposed to be hidden */
	w = screen->findWindow (event->xmap.window);
	if (w)
	{
	    CompWindow *cw;
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
	 * the group in the same way */
	w = screen->findWindow (event->xunmap.window);
	if (w)
	{
	    GROUP_WINDOW (w);

	    if (w->pendingUnmaps ())
	    {
		if (w->shaded ())
		{
		    gw->windowState = GroupWindow::WindowShaded;

		    //if (gw->group && optionGetShadeAll ())
			//groupShadeWindows (w, gw->group, TRUE);
		}
		else if (w->minimized ())
		{
		    gw->windowState = GroupWindow::WindowMinimized;

		    //if (gw->group && groupGetMinimizeAll (w->screen))
			//groupMinimizeWindows (w, gw->group, TRUE);
		}
	    }

	    if (gw->group)
	    {
		if (gw->group->tabBar && IS_TOP_TAB (w, gw->group))
		{
		    /* on unmap of the top tab, hide the tab bar and the
		       input prevention window */
		    //groupTabSetVisibility (gw->group, FALSE, PERMANENT);
		}
		if (!w->pendingUnmaps ())
		{
		    /* close event */
		    if (!(gw->animateState & IS_UNGROUPING))
		    {
			//groupDeleteGroupWindow (w);
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
		    gw->group->checkFocusAfterTabChange = TRUE;
		    //groupChangeTab (gw->slot, RotateUncertain);
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
		    int        i;
		    CompRect   rect;

		    rect.setX (event->xclient.data.l[0]);
		    rect.setY (event->xclient.data.l[1]);
		    rect.setWidth (event->xclient.data.l[2]);
		    rect.setHeight (event->xclient.data.l[3]);

		    foreach (CompWindow *cw, gw->group->windows)
		    {
			GroupWindow *gcw;

			gcw = GroupWindow::get (cw);
			if (gcw->resizeGeometry)
			{
			    //if (gcw->updateResizeRectangle (rect, true)
				//cWindow->addDamage ();
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

		    //if (gw->windowHideInfo)
			//gw->clearInputShape (gw->windowHideInfo);
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

		    //gw->group->tabBar->textLayer = TextLayer::renderWindowTitle (group);

		    //gw->group->tabBar->damageRegion ();
		}
	    }
	}
	break;

    case EnterNotify:
	{
	    CompWindow *w;
	    w = screen->findWindow (event->xcrossing.window);
	    if (w)
	    {
		GROUP_WINDOW (w);

		if (showDelayTimeoutHandle.active ())
		    showDelayTimeoutHandle.stop ();

		/*if (w->id () != screen->grabWindow ())
		    updateTabBars (w->id ());*/

		if (gw->group)
		{
		    if (draggedSlot && dragged &&
			IS_TOP_TAB (w, gw->group))
		    {
			/* We entered a group with a dragged window slot */

			#if 0
			int hoverTime;
			hoverTime = groupGetDragHoverTime (w->screen) * 1000;
			if (gs->dragHoverTimeoutHandle)
			    compRemoveTimeout (gs->dragHoverTimeoutHandle);

			if (hoverTime > 0)
			    gs->dragHoverTimeoutHandle =
				compAddTimeout (hoverTime,
						(float) hoverTime * 1.2,
						groupDragHoverTimeout, w);
			#endif
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
		    gw->group->inputPrevention && gw->group->ipwMapped)
		{
		    XWindowChanges xwc;

		    xwc.stack_mode = Above;
		    xwc.sibling = w->id ();

		    XConfigureWindow (screen->dpy (),
				      gw->group->inputPrevention,
				      CWSibling | CWStackMode, &xwc);
		}

		if (event->xconfigure.above != None)
		{
		    if (gw->group && !gw->group->tabBar &&
			(gw->group != lastRestackedGroup))
		    {
			//if (optionGetRaiseAll ())
			    //gw->group->raiseWindows ();
		    }
		    if (w->managed () && !w->overrideRedirect ())
			lastRestackedGroup = gw->group;
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
    Group *group;
    std::list <Group *>::iterator it = groups.begin ();

    cScreen->preparePaint (msSinceLastPaint);

    foreach (Group *group, groups)
    {
#if 0

	GroupTabBar *bar = group->tabBar;

	if (bar)
	{
	    groupApplyForces (s, bar, (dragged) ? draggedSlot : NULL);
	    groupApplySpeeds (s, group, msSinceLastPaint);

	    if ((bar->state != PaintOff) && HAS_TOP_WIN (group))
		groupHandleHoverDetection (group);

	    if (bar->state == PaintFadeIn || bar->state == PaintFadeOut)
		groupHandleTabBarFade (group, msSinceLastPaint);

	    if (bar->textLayer)
		groupHandleTextFade (group, msSinceLastPaint);

	    if (bar->bgAnimation)
		gro/0!	#DIV/0!	#DIV/0!	#DIV/0!	

.
							0				0	0	#DIV/0!	#DIV/0!	#DIV/0!	#DIV/0!	#DIV/0!	#DIV/0!	#DIV/0!	upHandleTabBarAnimation (group, msSinceLastPaint);
	}

	if (group->changeState != NoTabChange)
	{
	    group->changeAnimationTime -= msSinceLastPaint;
	    if (group->changeAnimationTime <= 0)
		groupHandleAnimation (group);
	}

	/* groupDrawTabAnimation may delete the group, so better
	   save the pointer to the next chain element */
	next = group->next;

	if (group->tabbingState != NoTabbing)
	    groupDrawTabAnimation (group, msSinceLastPaint);

	group = next;
#endif
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
    Group *group;
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
	    if (group->changeState != Group::NoTabChange ||
		group->tabbingState != Group::NoTabbing)
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
#if 0
	    state = gw->group->tabBar->state;
	    gw->group->tabBar->state = PaintOff;
	    groupPaintThumb (NULL, draggedSlot, &wTransform, OPAQUE);
	    gw->group->tabBar->state = state;
#endif

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

	    //groupPaintThumb (NULL, draggedSlot, &wTransform, OPAQUE);

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
	if (group->tabbingState != Group::NoTabbing)
	    cScreen->damageScreen ();
	else if (group->changeState != Group::NoTabChange)
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
#if 0
	    if (needDamage)
		groupDamageTabBarRegion (group);
#endif
	}
    }
}
