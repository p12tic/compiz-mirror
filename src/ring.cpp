/*
 *
 * Compiz ring switcher plugin
 *
 * ring.cpp
 *
 * Copyright : (C) 2007 by Danny Baumann
 * E-mail    : maniac@opencompositing.org
 *
 * Based on scale.c and switcher.c:
 * Copyright : (C) 2007 David Reveman
 * E-mail    : davidr@novell.com
 *
 * Ported to Compiz 0.9 by:
 * Copyright : (C) 2009 Sam Spilsbury
 * E-mail    : smspillaz@gmail.com
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
 */

#include "ring.h"

COMPIZ_PLUGIN_20090315 (ring, RingPluginVTable);

bool textAvailable;

static void
toggleFunctions (bool enabled)
{
    RING_SCREEN (screen);

    rs->cScreen->preparePaintSetEnabled (rs, enabled);
    rs->cScreen->donePaintSetEnabled (rs, enabled);
    rs->gScreen->glPaintOutputSetEnabled (rs, enabled);
    screen->handleEventSetEnabled (rs, enabled);

    foreach (CompWindow *w, screen->windows ())
    {
	RING_WINDOW (w);
	rw->gWindow->glPaintSetEnabled (rw, enabled);
	rw->cWindow->damageRectSetEnabled (rw, enabled);
    }
}

void
RingScreen::switchActivateEvent (bool activating)
{
    CompOption::Vector o;

    CompOption o1 ("root", CompOption::TypeInt);
    o1.value ().set ((int) screen->root ());

    o.push_back (o1);

    CompOption o2 ("active", CompOption::TypeBool);
    o2.value ().set (activating);

    o.push_back (o2);

    screen->handleCompizEvent ("ring", "activate", o);
}

bool
RingWindow::is (bool removing)
{
    RING_SCREEN (screen);

    if (!removing && window->destroyed ())
	return false;

    if (window->overrideRedirect ())
	return false;

    if (window->wmType () & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	return false;

    if (!removing && (!window->mapNum () || !window->isViewable ()))
    {
	if (rs->optionGetMinimized ())
	{
	    if (!window->minimized () && !window->inShowDesktopMode () &&
	        !window->shaded ())
		return false;
	}
	else
    	    return false;
    }

    if (!removing && rs->type == RingScreen::RingTypeNormal)
    {
	if (!window->mapNum () || !window->isViewable ())
	{
	    if (window->serverX () + window->width ()  <= 0    ||
		window->serverY () + window->height () <= 0    ||
		window->serverX () >= screen->width () ||
		window->serverY () >= screen->height ())
		return false;
	}
	else
	{
	    if (!window->focus ())
		return false;
	}
    }
    else if (rs->type == RingScreen::RingTypeGroup &&
	     rs->clientLeader != window->clientLeader () &&
	     rs->clientLeader != window->id ())
    {
	return false;
    }

    if (window->state () & CompWindowStateSkipTaskbarMask)
	return false;

    if (!rs->currentMatch.evaluate (window))
	return false;

    return true;
}

void
RingScreen::freeWindowTitle ()
{
}

void
RingScreen::renderWindowTitle ()
{
    if (!textAvailable)
	return;

    CompText::Attrib attrib;
    CompRect       oe;
    int            ox1, ox2, oy1, oy2;

    freeWindowTitle ();

    if (!selectedWindow)
	return;

    if (!optionGetWindowTitle ())
	return;

    oe = screen->getCurrentOutputExtents ();

    ox1 = oe.x1 ();
    ox2 = oe.x2 ();
    oy1 = oe.y1 ();
    oy2 = oe.y2 ();

    /* 75% of the output device as maximum width */
    attrib.maxWidth = (ox2 - ox1) * 3 / 4;
    attrib.maxHeight = 100;

    attrib.size = optionGetTitleFontSize ();
    attrib.color[0] = optionGetTitleFontColorRed ();
    attrib.color[1] = optionGetTitleFontColorGreen ();
    attrib.color[2] = optionGetTitleFontColorBlue ();
    attrib.color[3] = optionGetTitleFontColorAlpha ();
    attrib.flags = CompText::WithBackground | CompText::Ellipsized;
    if (optionGetTitleFontBold ())
	attrib.flags |= CompText::StyleBold;
    attrib.family = "Sans";
    attrib.bgHMargin = 15;
    attrib.bgVMargin = 15;
    attrib.bgColor[0] = optionGetTitleBackColorRed ();
    attrib.bgColor[1] = optionGetTitleBackColorGreen ();
    attrib.bgColor[2] = optionGetTitleBackColorBlue ();
    attrib.bgColor[3] = optionGetTitleBackColorAlpha ();

    text.renderWindowTitle (selectedWindow->id (),
                            type == RingScreen::RingTypeAll,
                            attrib);
}

void
RingScreen::drawWindowTitle ()
{
    if (!textAvailable)
	return;
    float      x, y;
    CompRect   r;
    int        ox1, ox2, oy1, oy2;

    r = screen->getCurrentOutputExtents ();

    ox1 = r.x1 ();
    ox2 = r.x2 ();
    oy1 = r.y1 ();
    oy2 = r.y2 ();

    x = ox1 + ((ox2 - ox1) / 2) - (text.getWidth () / 2);

    /* assign y (for the lower corner!) according to the setting */
    switch (optionGetTitleTextPlacement ())
    {
	case RingOptions::TitleTextPlacementCenteredOnScreen:
	    y = oy1 + ((oy2 - oy1) / 2) + (text.getHeight () / 2);
	    break;
	case RingOptions::TitleTextPlacementAboveRing:
	case RingOptions::TitleTextPlacementBelowRing:
	    {
		CompRect workArea;
		workArea =
		    screen->getWorkareaForOutput (
					     screen->currentOutputDev ().id ());

	    	if (optionGetTitleTextPlacement () ==
		    RingOptions::TitleTextPlacementAboveRing)
    		    y = oy1 + workArea.y () + text.getHeight ();
		else
		    y = oy1 + workArea.y () + workArea.height ();
	    }
	    break;
	default:
	    return;
	    break;
    }

    text.draw (floor (x), floor (y), 1.0f);
}

bool
RingWindow::glPaint (const GLWindowPaintAttrib &attrib,
		     const GLMatrix            &transform,
		     const CompRegion	       &region,
		     unsigned int	       mask)
{
    bool       status;
    bool       pixmap = true;

    RING_SCREEN (screen);

    if (rs->state != RingScreen::RingStateNone)
    {
	GLWindowPaintAttrib sAttrib = attrib;
	bool		  scaled = false;

    	if (window->mapNum ())
	{
	    if (gWindow->textures ().empty ())
		gWindow->bind ();
	}

	if (adjust || slot)
	{
	    scaled = adjust || (slot);
	    mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;
	}
	else if (rs->state != RingScreen::RingStateIn)
	{
	    if (rs->optionGetDarkenBack ())
	    {
		/* modify brightness of the other windows */
		sAttrib.brightness = sAttrib.brightness / 2;
	    }
	}

	gWindow->glPaint (sAttrib, transform, region, mask);

	pixmap = !gWindow->textures ().empty ();

	if (scaled && pixmap)
	{
	    GLFragment::Attrib fragment (gWindow->lastPaintAttrib ());
	    GLMatrix           wTransform = transform;

	    if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
		return false;

	    if (slot)
	    {
    		fragment.setBrightness((float) fragment.getBrightness () *
					 slot->depthBrightness);

		if (window != rs->selectedWindow)
		    fragment.setOpacity ((float)fragment.getOpacity () *
			                 rs->optionGetInactiveOpacity () / 100);
	    }

	    if (window->alpha () || fragment.getOpacity () != OPAQUE)
		mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

	    wTransform.translate (window->x (), window->y (), 0.0f);
	    wTransform.scale (scale, scale, 1.0f);
	    wTransform.translate (tx / scale - window->x (),
			          ty / scale - window->y (),
			          0.0f);

	    glPushMatrix ();
	    glLoadMatrixf (wTransform.getMatrix ());

	    gWindow->glDraw (wTransform, fragment, region,
			     mask | PAINT_WINDOW_TRANSFORMED_MASK);

	    glPopMatrix ();
	}

	if (scaled && (rs->state != RingScreen::RingStateIn) &&
	    ((rs->optionGetOverlayIcon () != RingOptions::OverlayIconNone) || 
	     !pixmap))
	{
	    GLTexture *icon;

	    icon = gWindow->getIcon (256, 256);
	    if (!icon)
		icon = rs->gScreen->defaultIcon ();

	    if (icon)
	    {
		GLTexture::Matrix        matrix;
		GLTexture::MatrixList	 matricies;
		float                    f_scale;
		float                    x, y;
		int                      width, height;
		int                      scaledWinWidth, scaledWinHeight;

		enum RingOptions::OverlayIcon  iconOverlay;

		scaledWinWidth  = window->width () * scale;
		scaledWinHeight = window->height () * scale;

		if (!pixmap)
		    iconOverlay = RingOptions::OverlayIconBig;
		else
		    iconOverlay = (enum RingOptions::OverlayIcon)
						    rs->optionGetOverlayIcon ();

	    	switch (iconOverlay) {
    		case RingOptions::OverlayIconNone:
		case RingOptions::OverlayIconEmblem:
		    f_scale = (slot) ? slot->depthScale : 1.0f;
		    if (icon->width () > ICON_SIZE ||
			 icon->height () > ICON_SIZE)
			f_scale = MIN ((f_scale * ICON_SIZE / icon->width ()),
				     (f_scale * ICON_SIZE / icon->height ()));
		    break;
		case RingOptions::OverlayIconBig:
		default:
		    /* only change opacity if not painting an
		       icon for a minimized window */
		    if (pixmap)
			sAttrib.opacity /= 3;
		    f_scale = MIN (((float) scaledWinWidth / icon->width ()),
				 ((float) scaledWinHeight / icon->height ()));
		    break;
		}

		width  = icon->width ()  * f_scale;
		height = icon->height () * f_scale;

	    	switch (iconOverlay) {
		case RingOptions::OverlayIconNone:
		case RingOptions::OverlayIconEmblem:
		    x = window->x () + scaledWinWidth - width;
		    y = window->y () + scaledWinHeight - height;
		    break;
		case RingOptions::OverlayIconBig:
		default:
		    x = window->x () + scaledWinWidth / 2 - width / 2;
		    y = window->y () + scaledWinHeight / 2 - height / 2;
		    break;
		}

		x += tx;
		y += ty;

		mask |= PAINT_WINDOW_BLEND_MASK;
		
		/* if we paint the icon for a minimized window, we need
		   to force the usage of a good texture filter */
		if (!pixmap)
		    mask |= PAINT_WINDOW_TRANSFORMED_MASK;

		CompRegion iconReg (window->x (), window->y (),
				    icon->width (), icon->height ());

		matrix = icon->matrix (); // ???
		matrix.x0 -= (window->x () * matrix.xx);
		matrix.y0 -= (window->y () * matrix.yy);

		matricies.push_back (matrix);

		gWindow->geometry ().vCount = 0;
		gWindow->geometry ().indexCount =0;

		gWindow->glAddGeometry (matricies, iconReg, infiniteRegion); //???

		if (gWindow->geometry ().vCount)
		{
		    GLFragment::Attrib fragment (sAttrib);
		    GLMatrix	       wTransform = transform;

		    if (!pixmap)
			sAttrib.opacity = gWindow->paintAttrib ().opacity;

		    if (slot)
			fragment.setBrightness (
					(float) fragment.getBrightness () * 
					slot->depthBrightness);

		    wTransform.translate (window->x (), window->y (), 0.0f);
		    wTransform.scale (f_scale, f_scale, 1.0f);
		    wTransform.translate ((x - window->x ()) / f_scale -
							 	window->x (),
				          (y - window->y ()) / f_scale -
								 window->y (),
				          0.0f);

		    glPushMatrix ();
		    glLoadMatrixf (wTransform.getMatrix ());

		    gWindow->glDrawTexture (icon, fragment, mask);

		    glPopMatrix ();
		}
	    }
	}
    }
    else
    {
	status = gWindow->glPaint (attrib, transform, region, mask);
    }

    return status;
}

static inline float 
ringLinearInterpolation (float valX, 
			 float minX, float maxX, 
			 float minY, float maxY)
{
    double factor = (maxY - minY) / (maxX - minX);
    return (minY + (factor * (valX - minX)));
}

bool
RingWindow::compareWindows (CompWindow *w1,
			    CompWindow *w2)
{
    if (w1->mapNum () && !w2->mapNum ())
	return true;

    if (w2->mapNum () && !w1->mapNum ())
	return false;

    return (w2->activeNum () < w1->activeNum ());
}

bool
RingWindow::compareRingWindowDepth (RingScreen::RingDrawSlot e1,
				    RingScreen::RingDrawSlot e2)
{
    RingScreen::RingSlot *a1   = (*(e1.slot));
    RingScreen::RingSlot *a2   = (*(e2.slot));

    if (a1->y < a2->y)
	return true;
    else if (a1->y > a2->y)
	return false;
    else
	return false;
}

bool
RingScreen::layoutThumbs ()
{
    float      baseAngle, angle;
    int        index = 0;
    int        ww, wh;
    float      xScale, yScale;
    int        ox1, ox2, oy1, oy2;
    int        centerX, centerY;
    int        ellipseA, ellipseB;
    CompRect   oe;

    if ((state == RingStateNone) || (state == RingStateIn))
	return false;

    baseAngle = (2 * PI * rotTarget) / 3600;

    oe = screen->getCurrentOutputExtents ();

    ox1 = oe.x1 ();
    ox2 = oe.x2 ();
    oy1 = oe.y1 ();
    oy2 = oe.y2 ();

    /* the center of the ellipse is in the middle 
       of the used output device */
    centerX = ox1 + (ox2 - ox1) / 2;
    centerY = oy1 + (oy2 - oy1) / 2;
    ellipseA = ((ox2 - ox1) * optionGetRingWidth ()) / 200;
    ellipseB = ((oy2 - oy1) * optionGetRingHeight ()) / 200;

    drawSlots.resize (windows.size ());

    foreach (CompWindow *w, windows)
    {
	RING_WINDOW (w);

	if (!rw->slot)
	    rw->slot = new RingSlot ();

	if (!rw->slot)
	    return false;

	/* we subtract the angle from the base angle 
	   to order the windows clockwise */
	angle = baseAngle - (index * (2 * PI / windows.size ()));

	rw->slot->x = centerX + (optionGetRingClockwise () ? -1 : 1) * 
	                        ((float) ellipseA * sin (angle));
	rw->slot->y = centerY + ((float) ellipseB * cos (angle));

	ww = w->width ()  + w->input ().left + w->input ().right;
	wh = w->height () + w->input ().top  + w->input ().bottom;

	if (ww > optionGetThumbWidth ())
	    xScale = (float)(optionGetThumbWidth ()) / (float) ww;
	else
	    xScale = 1.0f;

	if (wh > optionGetThumbHeight ())
	    yScale = (float)(optionGetThumbHeight ()) / (float) wh;
	else
	    yScale = 1.0f;

	rw->slot->scale = MIN (xScale, yScale);

	/* scale and brightness are obtained by doing a linear inter-
	   polation - the y positions are the x values for the interpolation
	   (the larger Y is, the nearer is the window), and scale/brightness
	   are the y values for the interpolation */
	rw->slot->depthScale = 
	    ringLinearInterpolation (rw->slot->y, 
				     centerY - ellipseB, centerY + ellipseB, 
				     optionGetMinScale (), 1.0f);

	rw->slot->depthBrightness = 
	    ringLinearInterpolation (rw->slot->y, 
				     centerY - ellipseB, centerY + ellipseB, 
				     optionGetMinBrightness (), 1.0f);

	drawSlots.at (index).w    = w;
	drawSlots.at (index).slot = &rw->slot;

	index++;
    }

    /* sort the draw list so that the windows with the 
       lowest Y value (the windows being farest away)
       are drawn first */

    sort (drawSlots.begin (), drawSlots.end (),
	  RingWindow::compareRingWindowDepth); // TODO

    return true;
}

void
RingScreen::addWindowToList (CompWindow *w)
{
    windows.push_back (w);
}

bool
RingScreen::updateWindowList ()
{
    sort (windows.begin (), windows.end (), RingWindow::compareWindows);

    rotTarget = 0;
    foreach (CompWindow *w, windows)
    {
	if (w == selectedWindow)
	    break;

	rotTarget += DIST_ROT;
    }

    return layoutThumbs ();
}

bool
RingScreen::createWindowList ()
{
    windows.clear ();

    foreach (CompWindow *w, screen->windows ())
    {
	RING_WINDOW (w);
	if (rw->is ())
	{
	    addWindowToList (w);
	    rw->adjust = true;
	}
    }

    return updateWindowList ();
}

void
RingScreen::switchToWindow (bool	   toNext)
{
    CompWindow *w; // We need w to be in this scope
    int        cur = 0;

    if (!grabIndex)
	return;

    foreach (w, windows)
    {
	if (w == selectedWindow)
	    break;
	cur++;
    }

    if (cur == (int)windows.size ())
	return;

    if (toNext)
	w = windows.at ((cur + 1) % windows.size ());
    else
	w = windows.at ((cur + windows.size () - 1) % windows.size ());

    if (w)
    {
	CompWindow *old = selectedWindow;

	selectedWindow = w;
	if (old != w)
	{
	    if (toNext)
		rotAdjust += DIST_ROT;
	    else
		rotAdjust -= DIST_ROT;

	    rotateAdjust = true;

	    cScreen->damageScreen ();
	    renderWindowTitle ();
	}
    }
}

int
RingScreen::countWindows ()
{
    int	       count = 0;

    foreach (CompWindow *w, screen->windows ())
    {
	RING_WINDOW (w);

	if (rw->is ())
	    count++;
    }

    return count;
}

int
RingScreen::adjustRingRotation (float      chunk)
{
    float dx, adjust, amount;
    int   change;

    dx = rotAdjust;

    adjust = dx * 0.15f;
    amount = fabs (dx) * 1.5f;
    if (amount < 0.2f)
	amount = 0.2f;
    else if (amount > 2.0f)
	amount = 2.0f;

    rVelocity = (amount * rVelocity + adjust) / (amount + 1.0f);

    if (fabs (dx) < 0.1f && fabs (rVelocity) < 0.2f)
    {
	rVelocity = 0.0f;
	rotTarget += rotAdjust;
	rotAdjust = 0;
	return 0;
    }

    change = rVelocity * chunk;
    if (!change)
    {
	if (rVelocity)
	    change = (rotAdjust > 0) ? 1 : -1;
    }

    rotAdjust -= change;
    rotTarget += change;

    if (!layoutThumbs ())
	return false;

    return true;
}

int
RingWindow::adjustVelocity ()
{
    float dx, dy, ds, adjust, amount;
    float x1, y1, fScale;

    if (slot)
    {
	fScale = slot->scale * slot->depthScale;
	x1 = slot->x - (window->width () * fScale) / 2;
	y1 = slot->y - (window->height () * fScale) / 2;
    }
    else
    {
	fScale = 1.0f;
	x1 = window->x ();
	y1 = window->y ();
    }

    dx = x1 - (window->x () + tx);

    adjust = dx * 0.15f;
    amount = fabs (dx) * 1.5f;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    xVelocity = (amount * xVelocity + adjust) / (amount + 1.0f);

    dy = y1 - (window->y () + ty);

    adjust = dy * 0.15f;
    amount = fabs (dy) * 1.5f;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    yVelocity = (amount * yVelocity + adjust) / (amount + 1.0f);

    ds = fScale - scale;
    adjust = ds * 0.1f;
    amount = fabs (ds) * 7.0f;
    if (amount < 0.01f)
	amount = 0.01f;
    else if (amount > 0.15f)
	amount = 0.15f;

    scaleVelocity = (amount * scaleVelocity + adjust) /
	(amount + 1.0f);

    if (fabs (dx) < 0.1f && fabs (xVelocity) < 0.2f &&
	fabs (dy) < 0.1f && fabs (yVelocity) < 0.2f &&
	fabs (ds) < 0.001f && fabs (scaleVelocity) < 0.002f)
    {
	xVelocity = yVelocity = scaleVelocity = 0.0f;
	tx = x1 - window->x ();
	ty = y1 - window->y ();
	scale = fScale;

	return 0;
    }

    return 1;
}

bool
RingScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
			   const GLMatrix	     &transform,
			   const CompRegion	     &region,
			   CompOutput		     *output,
			   unsigned int		     mask)
{
    bool status;

    if (state != RingStateNone)
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    mask |= PAINT_SCREEN_NO_OCCLUSION_DETECTION_MASK;

    gScreen->glPaintOutput (attrib, transform, region, output, mask);

    if (state != RingStateNone)
    {
	GLMatrix      sTransform = transform;

	sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);
	glPushMatrix ();
	glLoadMatrixf (sTransform.getMatrix ());

	/* TODO: This code here should be reworked */

	if (state == RingScreen::RingStateSwitching ||
	    state == RingScreen::RingStateOut)
	{
	    for (std::vector <RingDrawSlot>::iterator it = drawSlots.begin ();
		 it != drawSlots.end (); it++)
	    {
		CompWindow *w = (*it).w;

		RING_WINDOW (w);

		status = rw->gWindow->glPaint (rw->gWindow->paintAttrib (),
					   sTransform, infiniteRegion, 0);
	    }
	}

	if (state != RingStateIn)
	    drawWindowTitle ();
	
	glPopMatrix ();
    }

    return status;
}

void
RingScreen::preparePaint (int msSinceLastPaint)
{
    if (state != RingStateNone && (moreAdjust || rotateAdjust))
    {
	int        steps;
	float      amount, chunk;

	amount = msSinceLastPaint * 0.05f * optionGetSpeed ();
	steps  = amount / (0.5f * optionGetTimestep ());

	if (!steps) 
	    steps = 1;
	chunk  = amount / (float) steps;

	while (steps--)
	{
	    rotateAdjust = adjustRingRotation (chunk);
	    moreAdjust = false;

	    foreach (CompWindow *w, screen->windows ())
	    {
		RING_WINDOW (w);

		if (rw->adjust)
		{
		    rw->adjust = rw->adjustVelocity ();

		    moreAdjust |= rw->adjust;

		    rw->tx += rw->xVelocity * chunk;
		    rw->ty += rw->yVelocity * chunk;
		    rw->scale += rw->scaleVelocity * chunk;
		}
		else if (rw->slot)
		{
		    rw->scale = rw->slot->scale * rw->slot->depthScale;
	    	    rw->tx = rw->slot->x - w->x () -
			     (w->width () * rw->scale) / 2;
	    	    rw->ty = rw->slot->y - w->y () - 
			     (w->height () * rw->scale) / 2;
		}
	    }

	    if (!moreAdjust && !rotateAdjust)
	    {
		switchActivateEvent (false);
		break;
	    }
	}
    }

    cScreen->preparePaint (msSinceLastPaint);
}

void
RingScreen::donePaint ()
{
    if (state != RingStateNone)
    {
	if (moreAdjust)
	{
	    cScreen->damageScreen ();
	}
	else
	{
	    if (rotateAdjust)
		cScreen->damageScreen ();

	    if (state == RingStateIn)
	    {
		toggleFunctions (false);
		state = RingStateNone;
	    }
	    else if (state == RingStateOut)
		state = RingStateSwitching;
	}
    }

    cScreen->donePaint ();
}

bool
RingScreen::terminate (CompAction         *action,
		       CompAction::State  aState,
		       CompOption::Vector options)
{
    if (grabIndex)
    {
        screen->removeGrab (grabIndex, 0);
        grabIndex = 0;
    }

    if (state != RingStateNone)
    {
        foreach (CompWindow *w, screen->windows ())
        {
	    RING_WINDOW (w);

	    if (rw->slot)
	    {
	        delete rw->slot;
	        rw->slot = NULL;

	        rw->adjust = true;
	    }
        }
        moreAdjust = true;
        state = RingStateIn;
        cScreen->damageScreen ();

        if (!(aState & CompAction::StateCancel) &&
            selectedWindow && !selectedWindow->destroyed ())
        {
            screen->sendWindowActivationRequest (selectedWindow->id ());
        }
    }

    if (action)
	action->setState ( ~(CompAction::StateTermKey |
			     CompAction::StateTermButton |
			     CompAction::StateTermEdge));

    return false;
}

bool
RingScreen::initiate (CompAction         *action,
		      CompAction::State  aState,
		      CompOption::Vector options)
{
    int       count; 

    if (screen->otherGrabExist ("ring", NULL))
	return false;
	   
    currentMatch = optionGetWindowMatch ();

    match = CompOption::getMatchOptionNamed (options, "match", CompMatch ());
    if (!match.isEmpty ())
    {
	currentMatch = match;
    }

    count = countWindows ();

    if (count < 1)
    {
	return false;
    }

    if (!grabIndex)
    {
	if (optionGetSelectWithMouse ())
	    grabIndex = screen->pushGrab (screen->normalCursor (), "ring");
	else
	    grabIndex = screen->pushGrab (screen->invisibleCursor (), "ring");
    }

    if (grabIndex)
    {
	state = RingScreen::RingStateOut;

	if (!createWindowList ())
	    return false;

	selectedWindow = windows.front ();
	renderWindowTitle ();
	rotTarget = 0;

    	moreAdjust = true;
	toggleFunctions (true);
	cScreen->damageScreen ();

	switchActivateEvent (true);
    }

    return true;
}

bool
RingScreen::doSwitch (CompAction         *action,
		      CompAction::State  aState,
		      CompOption::Vector options,
		      bool		 nextWindow,
		      RingType		 f_type)
{
    bool       ret = true;

    if ((state == RingStateNone) || (state == RingStateIn))
    {
        if (f_type == RingTypeGroup)
        {
	    CompWindow *w;
	    w = screen->findWindow (CompOption::getIntOptionNamed (options,
							           "window",
							           0));
	    if (w)
	    {
	        type = RingTypeGroup;
	        clientLeader = 
		    (w->clientLeader ()) ? w->clientLeader () : w->id ();
	        ret = initiate (action, aState, options);
	    }
        }
        else
        {
	    type = f_type;
	    ret = initiate (action, state, options);
        }

        if (aState & CompAction::StateInitKey)
	    action->setState (action->state () | CompAction::StateTermKey);

        if (aState & CompAction::StateInitEdge)
	    action->setState (action->state () | CompAction::StateTermEdge);
        else if (state & CompAction::StateInitButton)
	    action->setState (action->state () |
			      CompAction::StateTermButton);
    }

    if (ret)
        switchToWindow (nextWindow);


    return ret;
}

void
RingScreen::windowSelectAt (int  x,
			    int  y,
			    bool f_terminate)
{
    CompWindow *selected = NULL;

    if (!optionGetSelectWithMouse ())
	return;
 
    /* first find the top-most window the mouse 
       pointer is over */
    foreach (CompWindow *w, windows)
    {
	RING_WINDOW (w);
    	if (rw->slot)
	{
    	    if ((x >= (rw->tx + w->x ())) &&
		(x <= (rw->tx + w->x () + (w->width () * rw->scale))) &&
		(y >= (rw->ty + w->y ())) &&
		(y <= (rw->ty + w->y () + (w->height () * rw->scale))))
	    {
		/* we have found one, select it */
		selected = w;
		break;
	    }
	}
    }

    if (selected && f_terminate)
    {
	CompOption o ("root", CompOption::TypeInt);
	CompOption::Vector opts;

	o.value ().set ((int) screen->root ());

	opts.push_back (o);

	selectedWindow = selected;

	terminate (NULL, 0, opts);
    }
    else if (!f_terminate && (selected != selectedWindow ))
    {
	if (!selected)
	{
	    freeWindowTitle ();
	}
	else
	{
	    selectedWindow = selected;
	    renderWindowTitle ();
	}
	cScreen->damageScreen ();
    }
}

void
RingScreen::windowRemove (CompWindow *w)
{
    if (w)
    {
	bool   inList = false;
	CompWindow *selected;
	CompWindowVector::iterator it = windows.begin ();

	RING_WINDOW (w);

	if (state == RingStateNone)
	    return;

	if (!rw->is (true))
    	    return;

	selected = selectedWindow;

	while (it != windows.end ())
	{
	    if (*it == w)
	    {
		inList = true;

		if (w == selected)
		{
		    it++;
		    if (it != windows.end ())
			selected = *it;
    		    else
			selected = windows.front ();
		    it--;

		    selectedWindow = selected;
		    renderWindowTitle ();
		}

		windows.erase (it);
		break;
	    }
	    it++;
	}

	if (!inList)
	    return;

	/* Terminate if the window closed was the last window in the list */

	if (!windows.size ())
	{
	    CompOption o ("root", CompOption::TypeInt);
	    CompOption::Vector opts;

	    o.value ().set ((int) screen->root ());

	    opts.push_back (o);

	    terminate (NULL, 0, opts);
	    return;
	}

	// Let the window list be updated to avoid crash
	// when a window is closed while ending (RingStateIn).
	if (!grabIndex && state != RingStateIn)
	    return;

	if (updateWindowList ())
	{
	    moreAdjust = true;
	    state = RingStateOut;
	    cScreen->damageScreen ();
	}
    }
}

void
RingScreen::handleEvent (XEvent *event)
{
    CompWindow *w = NULL;

    switch (event->type) {
    case DestroyNotify:
	/* We need to get the CompWindow * for event->xdestroywindow.window
	   here because in the ::screen->handleEvent call below, that
	   CompWindow's id will become 1, so findWindow won't be
	   able to find the CompWindow after that. */
	   w = ::screen->findWindow (event->xdestroywindow.window);
	break;
    default:
	break;
    }

    screen->handleEvent (event);

    switch (event->type) {
    case PropertyNotify:
	if (event->xproperty.atom == XA_WM_NAME)
	{
	    w = screen->findWindow (event->xproperty.window);
	    if (w)
	    {
		if (grabIndex && (w == selectedWindow))
    		{
    		    renderWindowTitle ();
    		    cScreen->damageScreen ();
		}
	    }
	}
	break;
    case ButtonPress:
	if (event->xbutton.button == Button1)
	{
	    if (grabIndex)
	        windowSelectAt (event->xbutton.x_root, 
				event->xbutton.y_root,
				true);
	}
	break;
    case MotionNotify:
        if (grabIndex)
	    windowSelectAt (event->xmotion.x_root,
			    event->xmotion.y_root,
			    false);
    case UnmapNotify:
	w = ::screen->findWindow (event->xunmap.window);
	windowRemove (w);
	break;
    case DestroyNotify:
	windowRemove (w);
	break;
    }
}

bool
RingWindow::damageRect (bool     initial,
			const CompRect &rect)
{
    bool       status = false;

    RING_SCREEN (screen);

    if (initial)
    {
	if (rs->grabIndex && is ())
	{
	    rs->addWindowToList (window);
	    if (rs->updateWindowList ())
	    {
		adjust = true;
		rs->moreAdjust = true;
		rs->state = RingScreen::RingStateOut;
		rs->cScreen->damageScreen ();
	    }
	}
    }
    else if (rs->state == RingScreen::RingStateSwitching)
    {
	
	if (slot)
	{
	    cWindow->damageTransformedRect (scale, scale,
					    tx, ty,
					    rect);
	    status = true;
	}
	
    }

    status |= cWindow->damageRect (initial, rect);

    return status;
}

RingScreen::RingScreen (CompScreen *screen) :
    PluginClassHandler <RingScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    grabIndex (0),
    state (RingScreen::RingStateNone),
    moreAdjust (false),
    rotateAdjust (false),
    rotAdjust (0),
    rVelocity (0.0f),
    selectedWindow (NULL)
{

    ScreenInterface::setHandler (screen, false);
    CompositeScreenInterface::setHandler (cScreen, false);
    GLScreenInterface::setHandler (gScreen, false);

    optionSetNextKeyInitiate (boost::bind (&RingScreen::doSwitch, this, _1, _2,
					   _3, true, RingTypeNormal));
    optionSetPrevKeyInitiate (boost::bind (&RingScreen::doSwitch, this, _1, _2,
					   _3, false, RingTypeNormal));
    optionSetNextAllKeyInitiate (boost::bind (&RingScreen::doSwitch, this, _1,
					      _2, _3, true, RingTypeAll));
    optionSetPrevAllKeyInitiate (boost::bind (&RingScreen::doSwitch, this, _1,
					      _2, _3, false, RingTypeAll));
    optionSetNextGroupKeyInitiate (boost::bind (&RingScreen::doSwitch, this, _1,
					      _2, _3, true, RingTypeGroup));
    optionSetPrevGroupKeyInitiate (boost::bind (&RingScreen::doSwitch, this, _1,
					      _2, _3, false, RingTypeGroup));

    optionSetNextKeyTerminate (boost::bind (&RingScreen::terminate, this, _1,
					    _2, _3));
    optionSetNextAllKeyTerminate (boost::bind (&RingScreen::terminate, this, _1,
					       _2, _3));
    optionSetNextGroupKeyTerminate (boost::bind (&RingScreen::terminate, this,
					         _1, _2, _3));
    optionSetPrevKeyTerminate (boost::bind (&RingScreen::terminate, this, _1,
					    _2, _3));
    optionSetPrevAllKeyTerminate (boost::bind (&RingScreen::terminate, this, _1,
					       _2, _3));
    optionSetPrevGroupKeyTerminate (boost::bind (&RingScreen::terminate, this,
					         _1, _2, _3));

    optionSetNextButtonInitiate (boost::bind (&RingScreen::doSwitch, this, _1, _2,
					   _3, true, RingTypeNormal));
    optionSetPrevButtonInitiate (boost::bind (&RingScreen::doSwitch, this, _1, _2,
					   _3, false, RingTypeNormal));
    optionSetNextAllButtonInitiate (boost::bind (&RingScreen::doSwitch, this, _1,
					      _2, _3, true, RingTypeAll));
    optionSetPrevAllButtonInitiate (boost::bind (&RingScreen::doSwitch, this, _1,
					      _2, _3, false, RingTypeAll));
    optionSetNextGroupButtonInitiate (boost::bind (&RingScreen::doSwitch, this, _1,
					      _2, _3, true, RingTypeGroup));
    optionSetPrevGroupButtonInitiate (boost::bind (&RingScreen::doSwitch, this, _1,
					      _2, _3, false, RingTypeGroup));

    optionSetNextButtonTerminate (boost::bind (&RingScreen::terminate, this, _1,
					    _2, _3));
    optionSetNextAllButtonTerminate (boost::bind (&RingScreen::terminate, this, _1,
					       _2, _3));
    optionSetNextGroupButtonTerminate (boost::bind (&RingScreen::terminate, this,
					         _1, _2, _3));
    optionSetPrevButtonTerminate (boost::bind (&RingScreen::terminate, this, _1,
					    _2, _3));
    optionSetPrevAllButtonTerminate (boost::bind (&RingScreen::terminate, this, _1,
					       _2, _3));
    optionSetPrevGroupButtonTerminate (boost::bind (&RingScreen::terminate, this,
					         _1, _2, _3));
}


RingScreen::~RingScreen ()
{
    windows.clear ();
    drawSlots.clear ();
}

RingWindow::RingWindow (CompWindow *window) :
    PluginClassHandler <RingWindow, CompWindow> (window),
    window (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window)),
    slot (NULL),
    xVelocity (0.0f),
    yVelocity (0.0f),
    scaleVelocity (0.0f),
    tx (0.0f),
    ty (0.0f),
    scale (1.0f),
    adjust (false)
{
    CompositeWindowInterface::setHandler (cWindow, false);
    GLWindowInterface::setHandler (gWindow, false);
}

RingWindow::~RingWindow ()
{
    if (slot)
	delete slot;
}

bool
RingPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
    	return false;

    if (!CompPlugin::checkPluginABI ("text", COMPIZ_TEXT_ABI))
    {
	compLogMessage ("ring", CompLogLevelWarn, "No compatible text plugin"\
						  " loaded");
	textAvailable = false;
    }
    else
	textAvailable = true;

    return true;
}
