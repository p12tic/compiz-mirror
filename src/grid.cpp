/*
 * Compiz Fusion Grid plugin
 *
 * Copyright (c) 2008 Stephen Kennedy <suasol@gmail.com>
 * Copyright (c) 2010 Scott Moreau <oreaus@gmail.com>
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
 * Description:
 *
 * Plugin to act like winsplit revolution (http://www.winsplit-revolution.com/)
 * use <Control><Alt>NUMPAD_KEY to move and tile your windows.
 *
 * Press the tiling keys several times to cycle through some tiling options.
 */

#include "grid.h"
#include "grabhandler.h"

using namespace GridWindowType;

static std::map <unsigned int, GridProps> gridProps;

void
GridScreen::handleCompizEvent(const char*    plugin,
			      const char*    event,
			      CompOption::Vector&  o)
{
    if (strcmp(event, "start_viewport_switch") == 0)
    {
	mSwitchingVp = true;
    }
    else if (strcmp(event, "end_viewport_switch") == 0)
    {
	mSwitchingVp = false;
    }

  screen->handleCompizEvent(plugin, event, o);
}

CompRect
GridScreen::slotToRect (CompWindow      *w,
			const CompRect& slot)
{
    return CompRect (slot.x () + w->border ().left,
		     slot.y () + w->border ().top,
		     slot.width () - (w->border ().left + w->border ().right),
		     slot.height () - (w->border ().top + w->border ().bottom));
}

CompRect
GridScreen::constrainSize (CompWindow      *w,
			   const CompRect& slot)
{
    CompRect result;
    int      cw, ch;

    result = slotToRect (w, slot);

    if (w->constrainNewWindowSize (result.width (), result.height (), &cw, &ch))
    {
	/* constrained size may put window offscreen, adjust for that case */
	int dx = result.x () + cw - workarea.right () + w->border ().right;
	int dy = result.y () + ch - workarea.bottom () + w->border ().bottom;

	if (dx > 0)
	    result.setX (result.x () - dx);
	if (dy > 0)
	    result.setY (result.y () - dy);

	result.setWidth (cw);
	result.setHeight (ch);
    }

    return result;
}

void
GridScreen::getPaintRectangle (CompRect &cRect)
{
    if (typeToMask (edgeToGridType ()) != GridUnknown && optionGetDrawIndicator ())
	cRect = desiredSlot;
    else
	cRect.setGeometry (0, 0, 0, 0);
}

int
applyProgress (int a, int b, float progress)
{
	return a < b ?
	 b - (ABS (a - b) * progress) :
	 b + (ABS (a - b) * progress);
}

void
GridScreen::setCurrentRect (Animation &anim)
{
	anim.currentRect.setLeft (applyProgress (anim.targetRect.x1 (),
													anim.fromRect.x1 (),
													anim.progress));
	anim.currentRect.setRight (applyProgress (anim.targetRect.x2 (),
													anim.fromRect.x2 (),
													anim.progress));
	anim.currentRect.setTop (applyProgress (anim.targetRect.y1 (),
													anim.fromRect.y1 (),
													anim.progress));
	anim.currentRect.setBottom (applyProgress (anim.targetRect.y2 (),
													anim.fromRect.y2 (),
													anim.progress));
}

bool
GridScreen::initiateCommon (CompAction         *action,
			    CompAction::State  state,
			    CompOption::Vector &option,
			    unsigned int                where,
			    bool               resize,
			    bool	       key)
{
    Window     xid;
    CompWindow *cw = 0;

    xid = CompOption::getIntOptionNamed (option, "window");
    cw  = screen->findWindow (xid);

    if (cw)
    {
	XWindowChanges xwc;
	bool maximizeH = where & (GridBottom | GridTop | GridMaximize);
	bool maximizeV = where & (GridLeft | GridRight | GridMaximize);

	if (!(cw->actions () & CompWindowActionResizeMask))
	    return false;

	if (maximizeH && !(cw->actions () & CompWindowActionMaximizeHorzMask))
	    return false;

	if (maximizeV && !(cw->actions () & CompWindowActionMaximizeVertMask))
	    return false;

	if (where & GridUnknown)
	    return false;

	GRID_WINDOW (cw);

	if (gw->lastTarget & ~(where))
	    gw->resizeCount = 0;
	else if (!key)
	    return false;

	props = gridProps[where];

	/* get current available area */
	if (cw == mGrabWindow)
	    workarea = screen->getWorkareaForOutput
			    (screen->outputDeviceForPoint (pointerX, pointerY));
	else
	{
	    workarea = screen->getWorkareaForOutput (cw->outputDevice ());

	    if (props.numCellsX == 1)
		centerCheck = true;

	    if (!gw->isGridResized)
		/* Store size not including borders when using a keybinding */
		gw->originalSize = slotToRect(cw, cw->serverBorderRect ());
	}

	if ((cw->state () & MAXIMIZE_STATE) &&
	    (resize || optionGetSnapoffMaximized ()))
	{
	    /* maximized state interferes with us, clear it */
	    cw->maximize (0);
	}

	if ((where & GridMaximize) && resize)
	{
	    /* move the window to the correct output */
	    if (cw == mGrabWindow)
	    {
		compiz::window::Geometry ng = cw->serverGeometry ();

		ng.setPos (CompPoint (workarea.pos () + CompPoint (50, 50)));
		cw->positionSetEnabled (gw, false);
		cw->position (ng);
		cw->positionSetEnabled (gw, true);
	    }
	    cw->maximize (MAXIMIZE_STATE);
	    gw->isGridResized = true;
	    gw->isGridMaximized = true;
		for (unsigned int i = 0; i < animations.size (); i++)
			animations.at (i).fadingOut = true;
	    return true;
	}

	/* Convention:
	 * xxxSlot include decorations (it's the screen area occupied)
	 * xxxRect are undecorated (it's the constrained position
	                            of the contents)
	 */

	/* slice and dice to get desired slot - including decorations */
	desiredSlot.setY (workarea.y () + props.gravityDown *
			  (workarea.height () / props.numCellsY));
	desiredSlot.setHeight (workarea.height () / props.numCellsY);
	desiredSlot.setX (workarea.x () + props.gravityRight *
			  (workarea.width () / props.numCellsX));
	desiredSlot.setWidth (workarea.width () / props.numCellsX);

	/* Adjust for constraints and decorations */
	if (where & ~(GridMaximize | GridLeft | GridRight))
	{
	    desiredRect = constrainSize (cw, desiredSlot);
	}
	else
	    desiredRect = slotToRect (cw, desiredSlot);

	/* Get current rect not including decorations */
	currentRect.setGeometry (cw->serverX (), cw->serverY (),
				 cw->serverWidth (),
				 cw->serverHeight ());

	if (desiredRect.y () == currentRect.y () &&
	    desiredRect.height () == currentRect.height () &&
	    where & ~(GridMaximize | GridLeft | GridRight) && gw->lastTarget & where)
	{
	    int slotWidth25  = workarea.width () / 4;
	    int slotWidth33  = (workarea.width () / 3) + cw->border ().left;
	    int slotWidth17  = slotWidth33 - slotWidth25;
	    int slotWidth66  = workarea.width () - slotWidth33;
	    int slotWidth75  = workarea.width () - slotWidth25;

	    if (props.numCellsX == 2) /* keys (1, 4, 7, 3, 6, 9) */
	    {
		if ((currentRect.width () == desiredRect.width () &&
		    currentRect.x () == desiredRect.x ()) ||
		    (gw->resizeCount < 1) || (gw->resizeCount > 5))
		    gw->resizeCount = 3;

		/* tricky, have to allow for window constraints when
		 * computing what the 33% and 66% offsets would be
		 */
		switch (gw->resizeCount)
		{
		    case 1:
			desiredSlot.setWidth (slotWidth66);
			desiredSlot.setX (workarea.x () +
					  props.gravityRight * slotWidth33);
			gw->resizeCount++;
			break;
		    case 2:
			gw->resizeCount++;
			break;
		    case 3:
			desiredSlot.setWidth (slotWidth33);
			desiredSlot.setX (workarea.x () +
					  props.gravityRight * slotWidth66);
			gw->resizeCount++;
			break;
		    case 4:
			desiredSlot.setWidth (slotWidth25);
			desiredSlot.setX (workarea.x () +
					  props.gravityRight * slotWidth75);
			gw->resizeCount++;
			break;
		    case 5:
			desiredSlot.setWidth (slotWidth75);
			desiredSlot.setX (workarea.x () +
					  props.gravityRight * slotWidth25);
			gw->resizeCount++;
			break;
		    default:
			break;
		}
	    }
	    else /* keys (2, 5, 8) */
	    {

		if ((currentRect.width () == desiredRect.width () &&
		    currentRect.x () == desiredRect.x ()) ||
		    (gw->resizeCount < 1) || (gw->resizeCount > 5))
		    gw->resizeCount = 1;
	    
		switch (gw->resizeCount)
		{
		    case 1:
			desiredSlot.setWidth (workarea.width () -
					     (slotWidth17 * 2));
			desiredSlot.setX (workarea.x () + slotWidth17);
			gw->resizeCount++;
			break;
		    case 2:
			desiredSlot.setWidth ((slotWidth25 * 2) +
					      (slotWidth17 * 2));
			desiredSlot.setX (workarea.x () +
					 (slotWidth25 - slotWidth17));
			gw->resizeCount++;
			break;
		    case 3:
			desiredSlot.setWidth ((slotWidth25 * 2));
			desiredSlot.setX (workarea.x () + slotWidth25);
			gw->resizeCount++;
			break;
		    case 4:
			desiredSlot.setWidth (slotWidth33 -
			    (cw->border ().left + cw->border ().right));
			desiredSlot.setX (workarea.x () + slotWidth33);
			gw->resizeCount++;
			break;
		    case 5:
			gw->resizeCount++;
			break;
		    default:
			break;
		}
	    }

	    if (gw->resizeCount == 6)
		gw->resizeCount = 1;

	    desiredRect = constrainSize (cw, desiredSlot);
	}

	compiz::window::Geometry ng (desiredRect.x (),
				     desiredRect.y (),
				     desiredRect.width (),
				     desiredRect.height (),
				     cw->serverGeometry ().border ());

	if (cw->mapNum ())
	    cw->sendSyncRequest ();

	/* TODO: animate move+resize */
	if (resize)
	{
	    gw->lastTarget = where;
	    gw->currentSize = static_cast <CompRect &> (ng);
	    CompWindowExtents lastBorder = gw->window->border ();

	    gw->sizeHintsFlags = 0;

	    /* Special case for left and right, actually vertically maximize
	     * the window */
	    if (where & GridLeft || where & GridRight)
	    {
		/* First restore the window to its original size */
		XWindowChanges rwc;

		compiz::window::Geometry ng (cw->serverGeometry ());
		ng.applyChange (compiz::window::Geometry (gw->originalSize.x (),
							  gw->originalSize.y (),
							  gw->originalSize.width (),
							  gw->originalSize.height (),
							  0), CHANGE_X | CHANGE_Y | CHANGE_WIDTH | CHANGE_HEIGHT);

		cw->positionSetEnabled (gw, false);
		cw->position (ng);
		cw->positionSetEnabled (gw, true);

		gw->isGridMaximized = true;
		gw->isGridResized = false;

		cw->positionSetEnabled (gw, false);

		/* Maximize the window */
		cw->maximize (CompWindowStateMaximizedVertMask);

		cw->positionSetEnabled (gw, true);

		/* Be evil */
		if (cw->sizeHints ().flags & PResizeInc)
		{
		    gw->sizeHintsFlags |= PResizeInc;
		    gw->window->sizeHints ().flags &= ~(PResizeInc);
		}
	    }
	    else
	    {
	        gw->isGridResized = true;
	        gw->isGridMaximized = false;
	    }

	    int dw = (lastBorder.left + lastBorder.right) - 
		      (gw->window->border ().left +
		       gw->window->border ().right);
			
	    int dh = (lastBorder.top + lastBorder.bottom) - 
			(gw->window->border ().top +
			 gw->window->border ().bottom);

	    ng.setWidth (ng.width () + dw);
	    ng.setHeight (ng.height () + dw);

	    printf ("positioned window at %i %i %i %i\n", ng.x (), ng.y (), ng.width (), ng.height ());

	    /* Make window the size that we want */
	    cw->positionSetEnabled (gw, false);
	    cw->position (ng);
	    cw->positionSetEnabled (gw, true);

	    for (unsigned int i = 0; i < animations.size (); i++)
		animations.at (i).fadingOut = true;
	}

	/* This centers a window if it could not be resized to the desired
	 * width. Without this, it can look buggy when desired width is
	 * beyond the minimum or maximum width of the window.
	 */
	if (centerCheck)
	{
	    if ((cw->serverBorderRect ().width () >
		 desiredSlot.width ()) ||
		 cw->serverBorderRect ().width () <
		 desiredSlot.width ())
	    {
		compiz::window::Geometry geom (cw->serverGeometry ());
		geom.setX  (geom.x () + (workarea.width () >> 1) -
			    ((cw->serverBorderRect ().width () >> 1) -
			      cw->border ().left));
		cw->positionSetEnabled (gw, false);
		cw->position (geom);
		cw->positionSetEnabled (gw, true);
	    }

	    centerCheck = false;
	}
    }

    return true;
}

void
GridScreen::glPaintRectangle (const GLScreenPaintAttrib &sAttrib,
			      const GLMatrix            &transform,
			      CompOutput                *output)
{
    CompRect rect;
    GLMatrix sTransform (transform);
	std::vector<Animation>::iterator iter;

    getPaintRectangle (rect);

	for (unsigned int i = 0; i < animations.size (); i++)
		setCurrentRect (animations.at (i));

    glPushMatrix ();

    sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

    glLoadMatrixf (sTransform.getMatrix ());

    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
    glEnable (GL_BLEND);

	for (iter = animations.begin (); iter != animations.end () && animating; iter++)
	{
		Animation& anim = *iter;
		float alpha = ((float) optionGetFillColorAlpha () / 65535.0f) * anim.opacity;

		/* fill rectangle */
		glColor4f (((float) optionGetFillColorRed () / 65535.0f) * alpha,
			   ((float) optionGetFillColorGreen () / 65535.0f) * alpha,
			   ((float) optionGetFillColorBlue () / 65535.0f) * alpha,
			   alpha);

		/* fill rectangle */
		glRecti (anim.currentRect.x1 (), anim.currentRect.y2 (),
				 anim.currentRect.x2 (), anim.currentRect.y1 ());

		/* Set outline rect smaller to avoid damage issues */
		anim.currentRect.setGeometry (anim.currentRect.x () + 1,
					      anim.currentRect.y () + 1,
					      anim.currentRect.width () - 2,
					      anim.currentRect.height () - 2);

		alpha = (float) (optionGetOutlineColorAlpha () / 65535.0f) * anim.opacity;

		/* draw outline */
		glColor4f (((float) optionGetOutlineColorRed () / 65535.0f) * alpha,
			   ((float) optionGetOutlineColorGreen () / 65535.0f) * alpha,
			   ((float) optionGetOutlineColorBlue () / 65535.0f) * alpha,
			   alpha);

		glLineWidth (2.0);

		glBegin (GL_LINE_LOOP);
		glVertex2i (anim.currentRect.x1 (),	anim.currentRect.y1 ());
		glVertex2i (anim.currentRect.x2 (),	anim.currentRect.y1 ());
		glVertex2i (anim.currentRect.x2 (),	anim.currentRect.y2 ());
		glVertex2i (anim.currentRect.x1 (),	anim.currentRect.y2 ());
		glEnd ();
	}

	if (!animating)
	{
		/* fill rectangle */
		float alpha = (float) optionGetFillColorAlpha () / 65535.0f;

		/* fill rectangle */
		glColor4f (((float) optionGetFillColorRed () / 65535.0f) * alpha,
			   ((float) optionGetFillColorGreen () / 65535.0f) * alpha,
			   ((float) optionGetFillColorBlue () / 65535.0f) * alpha,
			   alpha);
		glRecti (rect.x1 (), rect.y2 (), rect.x2 (), rect.y1 ());

		/* Set outline rect smaller to avoid damage issues */
		rect.setGeometry (rect.x () + 1, rect.y () + 1,
				  rect.width () - 2, rect.height () - 2);

		/* draw outline */
		alpha = (float) optionGetOutlineColorAlpha () / 65535.0f;

		/* draw outline */
		glColor4f (((float) optionGetOutlineColorRed () / 65535.0f) * alpha,
			   ((float) optionGetOutlineColorGreen () / 65535.0f) * alpha,
			   ((float) optionGetOutlineColorBlue () / 65535.0f) * alpha,
			   alpha);

		glLineWidth (2.0);
		glBegin (GL_LINE_LOOP);
		glVertex2i (rect.x1 (), rect.y1 ());
		glVertex2i (rect.x2 (), rect.y1 ());
		glVertex2i (rect.x2 (), rect.y2 ());
		glVertex2i (rect.x1 (), rect.y2 ());
		glEnd ();
	}

    /* clean up */
    glColor4usv (defaultColor);
    glDisable (GL_BLEND);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    glPopMatrix ();
}

bool
GridScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
			   const GLMatrix            &matrix,
			   const CompRegion          &region,
			   CompOutput                *output,
			   unsigned int              mask)
{
    bool status;

    status = glScreen->glPaintOutput (attrib, matrix, region, output, mask);

    glPaintRectangle (attrib, matrix, output);

    return status;
}

unsigned int
GridScreen::typeToMask (int t)
{
    typedef struct {
	unsigned int mask;
	int type;
    } GridTypeMask;

    std::vector <GridTypeMask> type =
    {
	{ GridWindowType::GridUnknown, 0 },
	{ GridWindowType::GridBottomLeft, 1 },
	{ GridWindowType::GridBottom, 2 },
	{ GridWindowType::GridBottomRight, 3 },
	{ GridWindowType::GridLeft, 4 },
	{ GridWindowType::GridCenter, 5 },
	{ GridWindowType::GridRight, 6 },
	{ GridWindowType::GridTopLeft, 7 },
	{ GridWindowType::GridTop, 8 },
	{ GridWindowType::GridTopRight, 9 },
	{ GridWindowType::GridMaximize, 10 }
    };

    for (unsigned int i = 0; i < type.size (); i++)
    {
	GridTypeMask &tm = type[i];
	if (tm.type == t)
	    return tm.mask;
    }

    return GridWindowType::GridUnknown;
}

int
GridScreen::edgeToGridType ()
{
    int ret;

    switch (edge) {
    case Left:
	ret = (int) optionGetLeftEdgeAction ();
	break;
    case Right:
	ret = (int) optionGetRightEdgeAction ();
	break;
    case Top:
	ret = (int) optionGetTopEdgeAction ();
	break;
    case Bottom:
	ret = (int) optionGetBottomEdgeAction ();
	break;
    case TopLeft:
	ret = (int) optionGetTopLeftCornerAction ();
	break;
    case TopRight:
	ret = (int) optionGetTopRightCornerAction ();
	break;
    case BottomLeft:
	ret = (int) optionGetBottomLeftCornerAction ();
	break;
    case BottomRight:
	ret = (int) optionGetBottomRightCornerAction ();
	break;
    case NoEdge:
    default:
	ret = -1;
	break;
    }

    return ret;
}

void
GridScreen::handleEvent (XEvent *event)
{
    CompOutput out;
    CompWindow *w;
    bool       check = false;

    screen->handleEvent (event);

    if (event->type != MotionNotify || !mGrabWindow)
	return;

    out = screen->outputDevs ().at (
                   screen->outputDeviceForPoint (CompPoint (pointerX, pointerY)));

    /* Detect corners first */
    /* Bottom Left */
    if (pointerY > (out.y () + out.height () - optionGetBottomEdgeThreshold()) &&
        pointerX < out.x () + optionGetLeftEdgeThreshold())
	edge = BottomLeft;
    /* Bottom Right */
    else if (pointerY > (out.y () + out.height () - optionGetBottomEdgeThreshold()) &&
             pointerX > (out.x () + out.width () - optionGetRightEdgeThreshold()))
	edge = BottomRight;
    /* Top Left */
    else if (pointerY < optionGetTopEdgeThreshold() &&
	    pointerX < optionGetLeftEdgeThreshold())
	edge = TopLeft;
    /* Top Right */
    else if (pointerY < out.y () + optionGetTopEdgeThreshold() &&
             pointerX > (out.x () + out.width () - optionGetRightEdgeThreshold()))
	edge = TopRight;
    /* Left */
    else if (pointerX < out.x () + optionGetLeftEdgeThreshold())
	edge = Left;
    /* Right */
    else if (pointerX > (out.x () + out.width () - optionGetRightEdgeThreshold()))
	edge = Right;
    /* Top */
    else if (pointerY < out.y () + optionGetTopEdgeThreshold())
	edge = Top;
    /* Bottom */
    else if (pointerY > (out.y () + out.height () - optionGetBottomEdgeThreshold()))
	edge = Bottom;
    /* No Edge */
    else
	edge = NoEdge;

    /* Detect when cursor enters another output */

    currentWorkarea = screen->getWorkareaForOutput
			    (screen->outputDeviceForPoint (pointerX, pointerY));
    if (lastWorkarea != currentWorkarea)
    {
	lastWorkarea = currentWorkarea;

	if (cScreen)
	    cScreen->damageRegion (desiredSlot);

	initiateCommon (0, 0, o, typeToMask (edgeToGridType ()), false, false);

	if (cScreen)
	    cScreen->damageRegion (desiredSlot);
    }

    /* Detect edge region change */

    if (lastEdge != edge)
    {
		lastSlot = desiredSlot;

		if (edge == NoEdge)
			desiredSlot.setGeometry (0, 0, 0, 0);

		if (cScreen)
			cScreen->damageRegion (desiredSlot);

		check = initiateCommon (NULL, 0, o, typeToMask (edgeToGridType ()), false, false);

		if (cScreen)
			cScreen->damageRegion (desiredSlot);

		if (lastSlot != desiredSlot)
		{
			if (animations.size ())
				/* Begin fading previous animation instance */
				animations.at (animations.size () - 1).fadingOut = true;

			if (edge != NoEdge && check)
			{
				CompWindow *cw = screen->findWindow (screen->activeWindow ());
				if (cw)
				{
				    animations.push_back (Animation ());
				    int current = animations.size () - 1;
				    animations.at (current).fromRect	= cw->serverBorderRect ();
				    animations.at (current).currentRect	= cw->serverBorderRect ();
				    animations.at (current).timer = animations.at (current).duration;
				    animations.at (current).targetRect = desiredSlot;

				    if (lastEdge == NoEdge || !animating)
				    {
					/* Cursor has entered edge region from non-edge region */
					animating = true;
					glScreen->glPaintOutputSetEnabled (this, true);
					cScreen->preparePaintSetEnabled (this, true);
					cScreen->donePaintSetEnabled (this, true);
				    }
				}
			}
		}

		lastEdge = edge;
    }

    w = screen->findWindow (CompOption::getIntOptionNamed (o, "window"));
}

bool
GridWindow::position (compiz::window::Geometry &g,
		      unsigned int             source,
		      unsigned int	       constrainment)
{
    /* Don't allow non-pagers to change the size of
     * this window */
    if (source != ClientTypePager && !window->grabbed () &&
	(isGridMaximized || isGridResized))
	g = window->serverGeometry ();

    return window->position (g);
}

void
GridWindow::grabNotify (int          x,
			int          y,
			unsigned int state,
			unsigned int mask)
{
    compiz::grid::window::GrabWindowHandler gwHandler (mask);

    if (gwHandler.track ())
    {
	gScreen->o[0].value ().set ((int) window->id ());

	screen->handleEventSetEnabled (gScreen, true);
	gScreen->mGrabWindow = window;
	pointerBufDx = pointerBufDy = 0;
	grabMask = mask;

	if (!isGridResized && !isGridMaximized && gScreen->optionGetSnapbackWindows ())
	    /* Store size not including borders when grabbing with cursor */
	    originalSize = gScreen->slotToRect(window,
						    window->serverBorderRect ());
    }
    else if (gwHandler.resetResize ())
    {
	isGridResized = false;
	resizeCount = 0;
    }

    window->grabNotify (x, y, state, mask);
}

void
GridWindow::ungrabNotify ()
{
    if (window == gScreen->mGrabWindow)
    {
	gScreen->initiateCommon
			(NULL, 0, gScreen->o, gScreen->typeToMask (gScreen->edgeToGridType ()), true,
			 gScreen->edge != gScreen->lastResizeEdge);

	screen->handleEventSetEnabled (gScreen, false);
	grabMask = 0;
	gScreen->mGrabWindow = NULL;
	gScreen->o[0].value ().set (0);
	gScreen->cScreen->damageRegion (gScreen->desiredSlot);
    }

    gScreen->lastResizeEdge = gScreen->edge;
    gScreen->edge = NoEdge;

    window->ungrabNotify ();
}

void
GridWindow::applyOffset (const CompPoint &d)
{
    if ((isGridResized || isGridMaximized) &&
	!GridScreen::get (screen)->mSwitchingVp)
    {
	if (window->grabbed () && (grabMask & CompWindowGrabMoveMask))
	{
	    pointerBufDx += d.x ();
	    pointerBufDy += d.y ();

	    printf ("%i %i\n", pointerBufDx, pointerBufDy);

	    if ((abs (pointerBufDx) > SNAPOFF_THRESHOLD ||
		 abs (pointerBufDy) > SNAPOFF_THRESHOLD) &&
		 (isGridResized || isGridMaximized) &&
		 gScreen->optionGetSnapbackWindows ())
	    {
		printf ("restore window\n");
		    gScreen->restoreWindow (0, 0, gScreen->o);
	    }

	    /* Do not allow the window to be moved while it
	     * is resized */
	    return;
	}
    }

    CompositeWindow::get (window)->applyOffset (d);
}

void
GridWindow::stateChangeNotify (unsigned int lastState)
{
    if (lastState & MAXIMIZE_STATE &&
	!(window->state () & MAXIMIZE_STATE))
	lastTarget = GridUnknown;
    else if (!(lastState & MAXIMIZE_STATE) &&
	     window->state () & MAXIMIZE_STATE)
    {
	lastTarget = GridMaximize;
	if (window->grabbed ())
	{
	    originalSize = gScreen->slotToRect (window,
					        window->serverBorderRect ());
	}
    }

    window->stateChangeNotify (lastState);
}

bool
GridScreen::restoreWindow (CompAction         *action,
			   CompAction::State  state,
			   CompOption::Vector &option)
{
    CompWindow *cw = screen->findWindow (screen->activeWindow ());

    if (!cw)
	return false;

    GRID_WINDOW (cw);

    if (gw->isGridMaximized & !(cw->state () & MAXIMIZE_STATE))
    {
	gw->window->sizeHints ().flags |= gw->sizeHintsFlags;
	gw->isGridMaximized = false;
    }
    else
    {
	compiz::window::Geometry ng (cw->serverGeometry ());

        if (cw == mGrabWindow)
	{
	    ng.setX (pointerX - (gw->originalSize.width () >> 1));
	    ng.setY (pointerY - (cw->border ().top));
	}
	else
	{
	    ng.setX (gw->originalSize.x ());
	    ng.setY (gw->originalSize.y ());
	}

	ng.setWidth (gw->originalSize.width ());
	ng.setHeight (gw->originalSize.height ());
	cw->maximize (0);
	gw->currentSize = CompRect ();
	cw->positionSetEnabled (gw, false);
	cw->position (ng);
	cw->positionSetEnabled (gw, true);
	gw->pointerBufDx = 0;
	gw->pointerBufDy = 0;
    }
    gw->isGridResized = false;
    gw->resizeCount = 0;
    gw->lastTarget = GridUnknown;

    return true;
}

void
GridScreen::snapbackOptionChanged (CompOption *option,
				    Options    num)
{
    GRID_WINDOW (screen->findWindow
		    (CompOption::getIntOptionNamed (o, "window")));
    gw->isGridResized = false;
    gw->isGridMaximized = false;
    gw->resizeCount = 0;
}

void
GridScreen::preparePaint (int msSinceLastPaint)
{
	std::vector<Animation>::iterator iter;

	for (iter = animations.begin (); iter != animations.end (); iter++)
	{
		Animation& anim = *iter;
		anim.timer -= msSinceLastPaint;

		if (anim.timer < 0)
			anim.timer = 0;

		if (anim.fadingOut)
			anim.opacity -= msSinceLastPaint * 0.002;
		else
			if (anim.opacity < 1.0f)
				anim.opacity = anim.progress * anim.progress;
			else
				anim.opacity = 1.0f;

		if (anim.opacity < 0)
		{
			anim.opacity = 0.0f;
			anim.fadingOut = false;
			anim.complete = true;
		}

		anim.progress =	(anim.duration - anim.timer) / anim.duration;
	}

    cScreen->preparePaint (msSinceLastPaint);
}

void
GridScreen::donePaint ()
{
	std::vector<Animation>::iterator iter;

	for (iter = animations.begin (); iter != animations.end (); )
	{
		Animation& anim = *iter;
		if (anim.complete)
			iter = animations.erase(iter);
		else
			iter++;
	}

	if (animations.empty ())
	{
		cScreen->preparePaintSetEnabled (this, false);
		cScreen->donePaintSetEnabled (this, false);
		if (edge == NoEdge)
		{
			glScreen->glPaintOutputSetEnabled (this, false);
		}
		animations.clear ();
		animating = false;
	}

	cScreen->damageScreen ();

    cScreen->donePaint ();
}

Animation::Animation ()
{
	progress = 0.0f;
	fromRect = CompRect (0, 0, 0, 0);
	targetRect = CompRect (0, 0, 0, 0);
	currentRect = CompRect (0, 0, 0, 0);
	opacity = 0.0f;
	timer = 0.0f;
	duration = 250;
	complete = false;
	fadingOut = false;
}


GridScreen::GridScreen (CompScreen *screen) :
    PluginClassHandler<GridScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    glScreen (GLScreen::get (screen)),
    centerCheck (false),
    mGrabWindow (NULL),
    animating (false),
    mSwitchingVp (false)
{
    o.push_back (CompOption ("window", CompOption::TypeInt));

    ScreenInterface::setHandler (screen, false);
    screen->handleCompizEventSetEnabled (this, true);
    CompositeScreenInterface::setHandler (cScreen, false);
    GLScreenInterface::setHandler (glScreen, false);

    edge = lastEdge = lastResizeEdge = NoEdge;
    currentWorkarea = lastWorkarea = screen->getWorkareaForOutput
			    (screen->outputDeviceForPoint (pointerX, pointerY));
    gridProps[GridUnknown] = GridProps {0,1, 1,1};
    gridProps[GridBottomLeft]  = GridProps {0,1, 2,2};
    gridProps[GridBottom]  = GridProps {0,1, 1,2};
    gridProps[GridBottomRight] = GridProps {1,1, 2,2};
    gridProps[GridLeft]  = GridProps {0,0, 2,1};
    gridProps[GridCenter]  = GridProps{0,0, 1,1};
    gridProps[GridRight]  = GridProps {1,0, 2,1};
    gridProps[GridTopLeft]  = GridProps{0,0, 2,2};
    gridProps[GridTop]  = GridProps {0,0, 1,2};
    gridProps[GridTopRight]  = GridProps {1,0, 2,2};
    gridProps[GridMaximize]  = GridProps {0,0, 1,1};

	animations.clear ();

#define GRIDSET(opt,where,resize,key)					       \
    optionSet##opt##Initiate (boost::bind (&GridScreen::initiateCommon, this,  \
					   _1, _2, _3, where, resize, key))

    GRIDSET (PutCenterKey, GridWindowType::GridCenter, true, true);
    GRIDSET (PutLeftKey, GridWindowType::GridLeft, true, true);
    GRIDSET (PutRightKey, GridWindowType::GridRight, true, true);
    GRIDSET (PutTopKey, GridWindowType::GridTop, true, true);
    GRIDSET (PutBottomKey, GridWindowType::GridBottom, true, true);
    GRIDSET (PutTopleftKey, GridWindowType::GridTopLeft, true, true);
    GRIDSET (PutToprightKey, GridWindowType::GridTopRight, true, true);
    GRIDSET (PutBottomleftKey, GridWindowType::GridBottomLeft, true, true);
    GRIDSET (PutBottomrightKey, GridWindowType::GridBottomRight, true, true);
    GRIDSET (PutMaximizeKey, GridWindowType::GridMaximize, true, true);

#undef GRIDSET

    optionSetSnapbackWindowsNotify (boost::bind (&GridScreen::
				    snapbackOptionChanged, this, _1, _2));

    optionSetPutRestoreKeyInitiate (boost::bind (&GridScreen::
					    restoreWindow, this, _1, _2, _3));

}

GridWindow::GridWindow (CompWindow *window) :
    PluginClassHandler <GridWindow, CompWindow> (window),
    window (window),
    gScreen (GridScreen::get (screen)),
    isGridResized (false),
    isGridMaximized (false),
    grabMask (0),
    pointerBufDx (0),
    pointerBufDy (0),
    resizeCount (0),	
    lastTarget (GridUnknown)
{
    WindowInterface::setHandler (window);
    CompositeWindowInterface::setHandler (CompositeWindow::get (window));
}

GridWindow::~GridWindow ()
{
    if (gScreen->mGrabWindow == window)
	gScreen->mGrabWindow = NULL;

    gScreen->o[0].value ().set (0);
}

/* Initial plugin init function called. Checks to see if we are ABI
 * compatible with core, otherwise unload */

bool
GridPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
        return false;

    return true;
}
