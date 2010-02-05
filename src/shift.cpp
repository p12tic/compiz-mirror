/*
 *
 * Compiz shift switcher plugin
 *
 * shift.cpp
 *
 * Copyright : (C) 2007 by Dennis Kasprzyk
 * E-mail    : onestone@opencompositing.org
 *
 *
 * Based on ring.c:
 * Copyright : (C) 2007 by Danny Baumann
 * E-mail    : maniac@opencompositing.org
 *
 * Based on scale.c and switcher.c:
 * Copyright : (C) 2007 David Reveman
 * E-mail    : davidr@novell.com
 *
 * Rounded corner drawing taken from wall.c:
 * Copyright : (C) 2007 Robert Carr
 * E-mail    : racarr@beryl-project.org
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


#include "shift.h"

COMPIZ_PLUGIN_20090315 (shift, ShiftPluginVTable);

bool textAvailable;

void
toggleFunctions (bool enabled)
{
    SHIFT_SCREEN (screen);

    ss->cScreen->preparePaintSetEnabled (ss, enabled);
    ss->cScreen->paintSetEnabled (ss, enabled);
    ss->cScreen->donePaintSetEnabled (ss, enabled);
    ss->gScreen->glPaintOutputSetEnabled (ss, enabled);
    screen->handleEventSetEnabled (ss, enabled);

    foreach (CompWindow *w, screen->windows ())
    {
	SHIFT_WINDOW (w);
	sw->gWindow->glPaintSetEnabled (sw, enabled);
	sw->cWindow->damageRectSetEnabled (sw, enabled);
    } 
}

void
ShiftScreen::activateEvent (bool       activating)
{
    CompOption::Vector o;

    CompOption o1 ("root", CompOption::TypeInt);
    o1.value ().set ((int) screen->root ());

    o.push_back (o1);

    CompOption o2 ("active", CompOption::TypeBool);
    o2.value ().set (activating);

    o.push_back (o2);

    screen->handleCompizEvent ("shift", "activate", o);
}

bool
ShiftWindow::is ()
{

    SHIFT_SCREEN (screen);

    if (window->overrideRedirect ())
	return false;

    if (window->wmType () & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	return false;

    if (!window->mapNum () || !window->isViewable ())
    {
	if (ss->optionGetMinimized ())
	{
	    if (!window->minimized () && !window->inShowDesktopMode () &&
	        !window->shaded ())
		return false;
	}
	else
    	    return false;
    }

    if (ss->type == ShiftScreen::ShiftTypeNormal)
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
    else if (ss->type == ShiftScreen::ShiftTypeGroup &&
	     ss->clientLeader != window->clientLeader () &&
	     ss->clientLeader != window->id ())
    {
	return false;
    }

    if (window->state () & CompWindowStateSkipTaskbarMask)
	return false;

    if (!ss->currentMatch.evaluate (window))
	return false;

    return true;
}

void
ShiftScreen::freeWindowTitle ()
{
}

void
ShiftScreen::renderWindowTitle ()
{
    CompText::Attrib tA;
    CompRegion	     ox;
    int              ox1, ox2, oy1, oy2;

    freeWindowTitle ();

    if (!textAvailable)
	return;

    if (!optionGetWindowTitle ())
	return;

    if (optionGetMultioutputMode () ==
				    ShiftOptions::MultioutputModeOneBigSwitcher)
    {
	ox1 = oy1 = 0;
	ox2 = screen->width ();
	oy2 = screen->height ();
    }
    else
	ox = screen->getCurrentOutputExtents ();

    ox1 = ox.handle ()->extents.x1;
    oy1 = ox.handle ()->extents.y1;
    ox2 = ox.handle ()->extents.x2;
    oy2 = ox.handle ()->extents.y2;

    /* 75% of the output device as maximum width */
    tA.maxWidth = (ox2 - ox1) * 3 / 4;
    tA.maxHeight = 100;

    tA.family = "Sans";
    tA.size = optionGetTitleFontSize ();
    tA.color[0] = optionGetTitleFontColorRed ();
    tA.color[1] = optionGetTitleFontColorGreen ();
    tA.color[2] = optionGetTitleFontColorBlue ();
    tA.color[3] = optionGetTitleFontColorAlpha ();

    tA.flags = CompText::WithBackground | CompText::Ellipsized;
    if (optionGetTitleFontBold ())
	tA.flags |= CompText::StyleBold;

    tA.bgHMargin = 15;
    tA.bgVMargin = 15;
    tA.bgColor[0] = optionGetTitleBackColorRed ();
    tA.bgColor[1] = optionGetTitleBackColorGreen ();
    tA.bgColor[2] = optionGetTitleBackColorBlue ();
    tA.bgColor[3] = optionGetTitleBackColorAlpha ();

    text.renderWindowTitle (selectedWindow, type == ShiftTypeAll, tA);
}

void
ShiftScreen::drawWindowTitle ()
{
    float width, height, border = 10.0f;
    int ox1, ox2, oy1, oy2;

    width = text.getWidth ();
    height = text.getHeight ();

    if (optionGetMultioutputMode () == MultioutputModeOneBigSwitcher)
    {
	ox1 = oy1 = 0;
	ox2 = screen->width ();
	oy2 = screen->height ();
    }
    else
    {
        ox1 = screen->outputDevs ()[usedOutput].region ()->extents.x1;
        ox2 = screen->outputDevs ()[usedOutput].region ()->extents.x2;
        oy1 = screen->outputDevs ()[usedOutput].region ()->extents.y1;
        oy2 = screen->outputDevs ()[usedOutput].region ()->extents.y2;
    }

    float x = ox1 + ((ox2 - ox1) / 2) - (text.getWidth () / 2);
    float y;

    /* assign y (for the lower corner!) according to the setting */
    switch (optionGetTitleTextPlacement ())
    {
    case TitleTextPlacementCenteredOnScreen:
	y = oy1 + ((oy2 - oy1) / 2) + (height / 2);
	break;
    case TitleTextPlacementAbove:
    case TitleTextPlacementBelow:
	{
	    CompRect   workArea;
	    workArea = screen->currentOutputDev ().workArea ();

	    if (optionGetTitleTextPlacement () ==
		TitleTextPlacementAbove)
		y = oy1 + workArea.y () + (2 * border) + height;
	    else
		y = oy1 + workArea.y () + workArea.height () - (2 * border);
	}
	break;
    default:
	return;
    }

    text.draw (floor (x), floor (y), 1.0f);
}

bool
ShiftWindow::glPaint (const GLWindowPaintAttrib &attrib,
		      const GLMatrix		&transform,
		      const CompRegion		&region,
		      unsigned int		mask)
{
    SHIFT_SCREEN (screen);

    bool status = true;

    if (ss->state != ShiftScreen::ShiftStateNone && !ss->paintingAbove)
    {
	GLWindowPaintAttrib sAttrib = attrib;
	bool		  scaled = false;

    	if (window->mapNum ())
	{
	    if (gWindow->textures ().empty ())
		gWindow->bind ();
	}

	
	if (active)
	    scaled = (ss->activeSlot != NULL);

	if (opacity > 0.01 && (ss->activeSlot == NULL))
	{
	    sAttrib.brightness = sAttrib.brightness * brightness;
	    sAttrib.opacity = sAttrib.opacity * opacity;
	}
	else
	    mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

	if (active &&
	    ((unsigned int) ss->output->id () == (unsigned int) ss->usedOutput ||
	     (unsigned int) ss->output->id () == (unsigned int) ~0))
	    mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

	status = gWindow->glPaint (sAttrib, transform, region, mask);

	if (scaled && gWindow->textures ().size ())
	{
	    GLFragment::Attrib fragment (attrib);
	    GLMatrix	       wTransform = transform;
	    ShiftSlot	       *slot = ss->activeSlot->slot;

	    float sx     = ss->anim * slot->tx;
	    float sy     = ss->anim * slot->ty;
	    float sz     = ss->anim * slot->z;
	    float srot   = (ss->anim * slot->rotation);
	    float anim   = MIN (1.0, MAX (0.0, ss->anim));

	    float sscale;
	    float sopacity;
	    

	    if (slot->primary)
		sscale = (ss->anim * slot->scale) + (1 - ss->anim);
	    else
		sscale = ss->anim * slot->scale;
	
	    if (slot->primary && !ss->reflectActive)
	    {
		sopacity = (ss->anim * slot->opacity) + (1 - ss->anim);
	    }
	    else
	    {
		sopacity = anim * anim * slot->opacity;
	    }

	    if (sopacity <= 0.05)
		return status;

	    /* FIXME: Core's occlusion detection is broken and if this check
	     * is not disabled then nothing paints */

	    if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
		return false;

	    fragment.setOpacity ((float)fragment.getOpacity () * sopacity);
	    fragment.setBrightness ((float)fragment.getBrightness () *
				  ss->reflectBrightness);

	    if (window->alpha () || fragment.getOpacity () != OPAQUE)
		mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

	    wTransform.translate (sx, sy, sz);

	    wTransform.translate (window->x () + (window->width ()  * sscale / 2),
			          window->y () + (window->height ()  * sscale / 2.0),
				  0.0f);

	    wTransform.scale (ss->output->width (), -ss->output->height (),
			      1.0f);
	
	    wTransform.rotate (srot, 0.0, 1.0, 0.0);

	    wTransform.scale (1.0f  / ss->output->width (),
			      -1.0f / ss->output->height (), 1.0f);

	    wTransform.scale (sscale, sscale, 1.0f);
	    wTransform.translate (-window->x () - (window->width () / 2),
				  -window->y () - (window->height () / 2), 0.0f);

	    glPushMatrix ();
	    glLoadMatrixf (wTransform.getMatrix ());

	    gWindow->glDraw (wTransform, fragment, region,
			     mask | PAINT_WINDOW_TRANSFORMED_MASK);

	    glPopMatrix ();
	}



	if (scaled && ((ss->optionGetOverlayIcon () != ShiftOptions::OverlayIconNone) ||
	     gWindow->textures ().empty ()))
	{
	    GLTexture *icon;

	    icon = gWindow->getIcon (96, 96);
	    if (!icon)
		icon = ss->gScreen->defaultIcon ();

	    if (icon && (icon->name ()))
	    {
		float  f_scale;
		float  x, y;
		int    width, height;
		int    scaledWinWidth, scaledWinHeight;

		int iconOverlay = ss->optionGetOverlayIcon ();
		ShiftSlot      *slot = ss->activeSlot->slot;

		float sx       = ss->anim * slot->tx;
		float sy       = ss->anim * slot->ty;
		float sz       = ss->anim * slot->z;
		float srot     = (ss->anim * slot->rotation);
		float sopacity = ss->anim * slot->opacity;

		float sscale;

		if (slot->primary)
		    sscale = (ss->anim * slot->scale) + (1 - ss->anim);
		else
		    sscale = ss->anim * ss->anim * slot->scale;

		scaledWinWidth  = window->width ()  * sscale;
		scaledWinHeight = window->height () * sscale;

		if (gWindow->textures ().empty ())
		    iconOverlay = ShiftOptions::OverlayIconBig;

	    	switch (iconOverlay) 
		{
		case ShiftOptions::OverlayIconNone:
		case ShiftOptions::OverlayIconEmblem:
		    f_scale = 1.0f;
		    break;
		case ShiftOptions::OverlayIconBig:
		default:
		    /* only change opacity if not painting an
		       icon for a minimized window */
		    if (gWindow->textures ().size ())
			sAttrib.opacity /= 3;
		    f_scale = MIN (((float) scaledWinWidth / icon->width ()),
				 ((float) scaledWinHeight / icon->height ()));
		    break;
		}

		width  = icon->width ()  * f_scale;
		height = icon->height () * f_scale;

		switch (iconOverlay)
		{
		case ShiftOptions::OverlayIconNone:
		case ShiftOptions::OverlayIconEmblem:
		    x = scaledWinWidth - width;
		    y = scaledWinHeight - height;
		    break;
		case ShiftOptions::OverlayIconBig:
		default:
		    x = scaledWinWidth / 2 - width / 2;
		    y = scaledWinHeight / 2 - height / 2;
		    break;
		}

		mask |= PAINT_WINDOW_BLEND_MASK;
		
		/* if we paint the icon for a minimized window, we need
		   to force the usage of a good texture filter */
		if (gWindow->textures ().empty ())
		    mask |= PAINT_WINDOW_TRANSFORMED_MASK;

		CompRegion iconReg (0, 0, icon->width (), icon->height ());
		GLTexture::MatrixList matl;

		matl.push_back (icon->matrix ());

		gWindow->geometry ().vCount = 0;
		gWindow->geometry ().indexCount =0;
		gWindow->glAddGeometry (matl, iconReg,
					    infiniteRegion);

		if (gWindow->geometry ().vCount)
		{
		    GLFragment::Attrib fragment (attrib);
		    GLMatrix           wTransform = transform;

		    if (gWindow->textures ().empty ())
			sAttrib.opacity = gWindow->paintAttrib ().opacity;

		    fragment.setOpacity ((float)fragment.getOpacity () * sopacity);
		    fragment.setBrightness ((float)fragment.getBrightness () *
					    ss->reflectBrightness);

		    wTransform.translate (sx, sy, sz);

		    wTransform.translate (window->x () +
				     	  (window->width ()  * sscale / 2),
				     	  window->y () +
				     	  (window->height ()  * sscale / 2.0),
					  0.0f);

		    wTransform.scale (ss->output->width (),
                		      -ss->output->height (), 1.0f);

		    wTransform.rotate (srot, 0.0, 1.0, 0.0);

		    wTransform.scale (1.0f  / ss->output->width (),
                		      -1.0f / ss->output->height (), 1.0f);

		    wTransform.translate (x -
				          (window->width () * sscale / 2), y -
				          (window->height () * sscale / 2.0),
					  0.0f);
		    wTransform.scale (f_scale, f_scale, 1.0f);

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
	GLWindowPaintAttrib sAttrib = attrib;
	
	if (ss->paintingAbove)
	{
	    sAttrib.opacity = sAttrib.opacity * (1.0 - ss->anim);
	    
	    if (ss->anim > 0.99)
		mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;
	}

	status = gWindow->glPaint (sAttrib, transform, region, mask);
    }

//#endif

    return status;
}

bool
ShiftWindow::compareWindows (CompWindow *w1,
			     CompWindow *w2)
{
    CompWindow *w  = w1;

    if (w1 == w2)
	return 0;

    if (!w1->shaded () && !w1->isViewable () &&
        (w2->shaded () || w2->isViewable ()))
    {
	return false;
    }

    if (!w2->shaded () && w2->isViewable () &&
        (w1->shaded () || w1->isViewable ()))
    {
	return true;
    }

    while (w)
    {
	if (w == w2)
	    return false;
	w = w->next;
    }
    return true;
}

bool
ShiftWindow::compareShiftWindowDistance (ShiftDrawSlot elem1,
					 ShiftDrawSlot elem2)
{
    float a1   = (elem1).distance;
    float a2   = (elem2).distance;
    float ab   = fabs (a1 - a2); 

    if (ab > 0.3 && a1 > a2)
	return true;
    else if (ab > 0.3 && a1 < a2)
	return false;
    else
	return compareWindows ((elem2.w),
			       (elem1.w));
}

bool
ShiftScreen::layoutThumbsCover ()
{
    CompWindow *w;
    unsigned int index;
    int ww, wh;
    float xScale, yScale;
    float distance;
    int i;

    int ox1, ox2, oy1, oy2;

    if (optionGetMultioutputMode () ==
				    ShiftOptions::MultioutputModeOneBigSwitcher)
    {
	ox1 = oy1 = 0;
	ox2 = screen->width ();
	oy2 = screen->height ();
    }
    else
    {
        ox1 = screen->outputDevs ()[usedOutput].region ()->extents.x1;
        ox2 = screen->outputDevs ()[usedOutput].region ()->extents.x2;
        oy1 = screen->outputDevs ()[usedOutput].region ()->extents.y1;
        oy2 = screen->outputDevs ()[usedOutput].region ()->extents.y2;
    }
    
    /* the center of the ellipse is in the middle 
       of the used output device */
    int centerX = ox1 + (ox2 - ox1) / 2;
    int centerY = oy1 + (oy2 - oy1) / 2;

    int maxThumbWidth  = (ox2 - ox1) * optionGetSize() / 100;
    int maxThumbHeight = (oy2 - oy1) * optionGetSize() / 100;

    drawSlots.resize (windows.size () * 2);
    
    for (index = 0; index < windows.size (); index++)
    {
	w = windows.at (index);
	SHIFT_WINDOW (w);

	ww = w->width ()  + w->input ().left + w->input ().right;
	wh = w->height () + w->input ().top  + w->input ().bottom;

	if (ww > maxThumbWidth)
	    xScale = (float)(maxThumbWidth) / (float)ww;
	else
	    xScale = 1.0f;

	if (wh > maxThumbHeight)
	    yScale = (float)(maxThumbHeight) / (float)wh;
	else
	    yScale = 1.0f;


	float val1 = floor((float)windows.size () / 2.0);

	float pos;
	float space = maxThumbWidth / 2;
	space *= cos (sin (PI / 4) * PI / 3);
	space *= 2;
	//space += (space / sin (PI / 4)) - space;

	for (i = 0; i < 2; i++)
	{
	    if (invert ^ (i == 0))
	    {
	        distance = mvTarget - index;
	        distance += optionGetCoverOffset ();
	    }
	    else
	    {
	        distance = mvTarget - index + windows.size ();
	        distance += optionGetCoverOffset ();
	        if (distance > windows.size ())
		    distance -= windows.size () * 2;
	    }

	    pos = MIN (1.0, MAX (-1.0, distance));	

	    sw->slots[i].opacity = 1.0 - MIN (1.0,
			           MAX (0.0, fabs(distance) - val1));
	    sw->slots[i].scale   = MIN (xScale, yScale);
	
	    sw->slots[i].y = centerY + (maxThumbHeight / 2.0) -
			     (((w->height () / 2.0) + w->input ().bottom) *
			     sw->slots[i].scale);

	    if (fabs(distance) < 1.0)
	    {
	        sw->slots[i].x  = centerX + (sin(pos * PI * 0.5) * space);
	        sw->slots[i].z  = fabs (distance);
	        sw->slots[i].z *= -(maxThumbWidth / (2.0 * (ox2 - ox1)));

	        sw->slots[i].rotation = sin(pos * PI * 0.5) * -60;
	    }
	    else 
	    {
	        float rad = (space / (ox2 - ox1)) / sin(PI / 6.0);

	        float ang = (PI / MAX(72.0, windows.size () * 2)) *
			    (distance - pos) + (pos * (PI / 6.0));

	        sw->slots[i].x  = centerX;
	        sw->slots[i].x += sin(ang) * rad * (ox2 - ox1);
		
	        sw->slots[i].rotation  = 90;
	        sw->slots[i].rotation -= fabs(ang) * 180.0 / PI;
	        sw->slots[i].rotation *= -pos;

	        sw->slots[i].z  = -(maxThumbWidth / (2.0 * (ox2 - ox1)));
	        sw->slots[i].z += -(cos(PI / 6.0) * rad);
	        sw->slots[i].z += (cos(ang) * rad);
	    }

	    drawSlots.at (index * 2 + i).w     = w;
	    drawSlots.at (index * 2 + i).slot  = &sw->slots[i];
	    drawSlots.at (index * 2 + i).distance = fabs(distance);
	}
	

	if (drawSlots.at (index * 2).distance >
	    drawSlots.at (index * 2 + 1).distance)
	{
	    drawSlots.at (index * 2).slot->primary     = false;
	    drawSlots.at (index * 2 + 1).slot->primary = true;
	}
	else
	{
	    drawSlots.at (index * 2).slot->primary     = true;
	    drawSlots.at (index * 2 + 1).slot->primary = false;
	}

    }

    std::sort (drawSlots.begin (), drawSlots.end (), 
		ShiftWindow::compareShiftWindowDistance);

    return true;
}

bool
ShiftScreen::layoutThumbsFlip ()
{
#warning: fixme: correct opacity fadein/out is broken here
    CompWindow *w;
    unsigned int index;
    int ww, wh;
    float xScale, yScale;
    float distance;
    int i;
    float angle;
    int slotNum;

    int ox1, ox2, oy1, oy2;

    if (optionGetMultioutputMode () ==
				     ShiftScreen::MultioutputModeOneBigSwitcher)
    {
	ox1 = oy1 = 0;
	ox2 = screen->width ();
	oy2 = screen->height ();
    }
    else
    {
        ox1 = screen->outputDevs ()[usedOutput].region ()->extents.x1;
        ox2 = screen->outputDevs ()[usedOutput].region ()->extents.x2;
        oy1 = screen->outputDevs ()[usedOutput].region ()->extents.y1;
        oy2 = screen->outputDevs ()[usedOutput].region ()->extents.y2;
    }
    
    /* the center of the ellipse is in the middle 
       of the used output device */
    int centerX = ox1 + (ox2 - ox1) / 2;
    int centerY = oy1 + (oy2 - oy1) / 2;

    int maxThumbWidth  = (ox2 - ox1) * optionGetSize() / 100;
    int maxThumbHeight = (oy2 - oy1) * optionGetSize() / 100;

    slotNum = 0;

    drawSlots.resize (windows.size () * 2);
    
    for (index = 0; index < windows.size (); index++)
    {
	w = windows.at (index);
	SHIFT_WINDOW (w);

	ww = w->width ()  + w->input ().left + w->input ().right;
	wh = w->height () + w->input ().top  + w->input ().bottom;

	if (ww > maxThumbWidth)
	    xScale = (float)(maxThumbWidth) / (float)ww;
	else
	    xScale = 1.0f;

	if (wh > maxThumbHeight)
	    yScale = (float)(maxThumbHeight) / (float)wh;
	else
	    yScale = 1.0f;

	angle = optionGetFlipRotation () * PI / 180.0;

	for (i = 0; i < 2; i++)
	{
	    if (invert ^ (i == 0))
	        distance = mvTarget - index;
	    else
	    {
	        distance = mvTarget - index + windows.size ();
	        if (distance > 1.0)
		    distance -= windows.size () * 2;
	    }

	    if (distance > 0.0)
	        sw->slots[i].opacity = MAX (0.0, 1.0 - (distance * 1.0));
	    else
	    {
	        if (((int) distance) < -((int) windows.size () - 1))
	        	sw->slots[i].opacity = MAX (0.0, windows.size () +
					        distance);
	        else
		    sw->slots[i].opacity = 1.0;
	    }

	    if (distance > 0.0 && w->id () != selectedWindow)
	    {
	        sw->slots[i].primary = false;
	    }
	    else
	    {
	        sw->slots[i].primary = true;
	    }


	    sw->slots[i].scale   = MIN (xScale, yScale);
	
	    sw->slots[i].y = centerY + (maxThumbHeight / 2.0) -
			     (((w->height () / 2.0) + w->input ().bottom) *
			     sw->slots[i].scale);

	    sw->slots[i].x  = sin(angle) * distance * (maxThumbWidth / 2);
	    if (distance > 0 && false)
	        sw->slots[i].x *= 1.5;
	    sw->slots[i].x += centerX;
	
	    sw->slots[i].z  = cos(angle) * distance;
	    if (distance > 0)
	        sw->slots[i].z *= 1.5;
	    sw->slots[i].z *= (maxThumbWidth / (2.0 * (ox2 - ox1)));

	    sw->slots[i].rotation = optionGetFlipRotation ();

	    if (sw->slots[i].opacity > 0.0)
	    {
	        drawSlots.at (slotNum).w     = w;
	        drawSlots.at (slotNum).slot  = &sw->slots[i];
	        drawSlots.at (slotNum).distance = -distance;
	        slotNum++;
	    }
	}
    }


    drawSlots.resize (slotNum);

    std::sort (drawSlots.begin (), drawSlots.end (),
		ShiftWindow::compareShiftWindowDistance);

    return true;
}


bool
ShiftScreen::layoutThumbs ()
{
    bool result = false;

    if (state == ShiftScreen::ShiftStateNone)
	return false;

    switch (optionGetMode ())
    {
    case ShiftOptions::ModeCover:
	result = layoutThumbsCover ();
	break;
    case ShiftOptions::ModeFlip:
    	result = layoutThumbsFlip ();
    	break;
    }

    if (state == ShiftScreen::ShiftStateIn)
    	return false;

    return result;
}


void
ShiftScreen::addWindowToList (CompWindow *w)
{

    windows.push_back (w);

    if (drawSlots.size () <= windows.size ())
    {
	drawSlots.resize (windows.size () * 2);
    }
}

bool
ShiftScreen::updateWindowList ()
{
    int        idx;
    unsigned int i;

    std::sort (windows.begin (), windows.end (), ShiftWindow::compareWindows);

    mvTarget = 0;
    mvAdjust = 0;
    mvVelocity = 0;
    for (i = 0; i < windows.size (); i++)
    {
	if (windows.at (i)->id () == selectedWindow)
	    break;

	mvTarget++;
    }
    if (mvTarget == windows.size ())
	mvTarget = 0;

    /* create spetial window order to create a good animation
       A,B,C,D,E --> A,B,D,E,C to get B,D,E,C,(A),B,D,E,C as initial state */
    if (optionGetMode () == ModeCover)
    {
	std::vector <CompWindow *> wins (windows);
	
	for (i = 0; i < windows.size (); i++)
	{
	    idx = ceil (i * 0.5);
	    idx *= (i & 1) ? 1 : -1;
	    if (idx < 0)
		idx += windows.size ();
	    windows. at(idx) = wins. at(i);
	}
	wins.clear ();
    }

    return layoutThumbs ();
}

bool
ShiftScreen::createWindowList ()
{
    windows.clear ();

    foreach (CompWindow *w, screen->windows ())
    {
	SHIFT_WINDOW (w);

	if (sw->is ())
	{
	    addWindowToList (w);
	    sw->active = true;
	}
    }

    selectedWindow = windows.back ()->id (); // ??? This really shouldn't be here....

    return updateWindowList ();
}

void
ShiftScreen::switchToWindow (bool toNext)
{
    CompWindow *w;
    unsigned int cur = 0;

    if (!grabIndex)
	return;

    foreach (w, windows)
    {
	if (w->id () == selectedWindow)
	    break;
	cur++;
    }

    if (cur == windows.size ())
	return;

#warning: fixme: In theory this should select the right window

    if (toNext)
	w = windows.at ((cur + 1) % windows.size ());
    else
	w = windows.at ((cur + windows.size () - 1) % windows.size ());

    if (w)
    {

	Window old = selectedWindow;
	selectedWindow = w->id ();

	if (old != w->id ())
	{
	    if (toNext)
		mvAdjust += 1;
	    else
		mvAdjust -= 1;

	    moveAdjust = true;
	    cScreen->damageScreen ();
	    renderWindowTitle ();
	}
    }
}

int
ShiftScreen::countWindows ()
{
    int	       count = 0;

    foreach (CompWindow *w, screen->windows ())
    {
	if (ShiftWindow::get (w)->is ())
	    count++;
    }

    return count;
}

int
ShiftScreen::adjustShiftMovement (float chunk)
{
    float dx, adjust, amount;
    float change;

    dx = mvAdjust;

    adjust = dx * 0.15f;
    amount = fabs(dx) * 1.5f;
    if (amount < 0.2f)
	amount = 0.2f;
    else if (amount > 2.0f)
	amount = 2.0f;

    mvVelocity = (amount * mvVelocity + adjust) / (amount + 1.0f);

    if (fabs (dx) < 0.002f && fabs (mvVelocity) < 0.004f)
    {
	mvVelocity = 0.0f;
	mvTarget = mvTarget + mvAdjust;
	mvAdjust = 0;
	layoutThumbs ();
	return false;
    }

    change = mvVelocity * chunk;
    if (!change)
    {
	if (mvVelocity)
	    change = (mvAdjust > 0) ? 0.01 : -0.01;
    }

    mvAdjust -= change;
    mvTarget += change;

    while (mvTarget >= windows.size ())
    {
	mvTarget -= windows.size ();
	invert = !invert;
    }

    while (mvTarget < 0)
    {
	mvTarget += windows.size ();
	invert = !invert;
    }

    if (!layoutThumbs ())
	return false;

    return true;
}

bool
ShiftWindow::adjustShiftWindowAttribs (float chunk)
{
    float dp, db, adjust, amount;
    float f_opacity, f_brightness;

    SHIFT_SCREEN (screen);

    if ((active && ss->state != ShiftScreen::ShiftStateIn &&
	ss->state != ShiftScreen::ShiftStateNone) ||
	(ss->optionGetHideAll () && !(window->type () & CompWindowTypeDesktopMask) &&
	(ss->state == ShiftScreen::ShiftStateOut || ss->state == ShiftScreen::ShiftStateSwitching ||
	 ss->state == ShiftScreen::ShiftStateFinish)))
	f_opacity = 0.0;
    else
	f_opacity = 1.0;

    if (ss->state == ShiftScreen::ShiftStateIn || ss->state == ShiftScreen::ShiftStateNone)
	f_brightness = 1.0;
    else
	f_brightness = ss->optionGetBackgroundIntensity ();

    dp = f_opacity - opacity;
    adjust = dp * 0.1f;
    amount = fabs (dp) * 7.0f;
    if (amount < 0.01f)
	amount = 0.01f;
    else if (amount > 0.15f)
	amount = 0.15f;

    opacityVelocity = (amount * opacityVelocity + adjust) /
	(amount + 1.0f);

    db = f_brightness - brightness;
    adjust = db * 0.1f;
    amount = fabs (db) * 7.0f;
    if (amount < 0.01f)
	amount = 0.01f;
    else if (amount > 0.15f)
	amount = 0.15f;

    brightnessVelocity = (amount * brightnessVelocity + adjust) /
	(amount + 1.0f);


    if (fabs (dp) < 0.01f && fabs (opacityVelocity) < 0.02f &&
	fabs (db) < 0.01f && fabs (brightnessVelocity) < 0.02f)
    {
	brightness = f_brightness;
	opacity = f_opacity;
	return false;
    }

    brightness += brightnessVelocity * chunk;
    opacity += opacityVelocity * chunk;

    return true;
}

bool
ShiftScreen::adjustShiftAnimationAttribs (float chunk)
{
    float dr, adjust, amount;
    float f_anim;

    if (state != ShiftScreen::ShiftStateIn && state != ShiftScreen::ShiftStateNone)
	f_anim = 1.0;
    else
	f_anim = 0.0;

    dr = f_anim - anim;
    adjust = dr * 0.1f;
    amount = fabs (dr) * 7.0f;
    if (amount < 0.002f)
	amount = 0.002f;
    else if (amount > 0.15f)
	amount = 0.15f;

    animVelocity = (amount * animVelocity + adjust) /
	(amount + 1.0f);

    if (fabs (dr) < 0.002f && fabs (animVelocity) < 0.004f)
    {

	anim = f_anim;
	return false;
    }

    anim += animVelocity * chunk;
    return true;
}

bool
ShiftScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
			    const GLMatrix	      &transform,
			    const CompRegion	      &region,
			    CompOutput		      *f_output,
			    unsigned int	      mask)
{
    bool status;

    if (state != ShiftScreen::ShiftStateNone)
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    paintingAbove = false;

    output = f_output;

    status = gScreen->glPaintOutput (attrib, transform, region, f_output, mask);

    if (state != ShiftScreen::ShiftStateNone &&
	((unsigned int) f_output->id () == (unsigned int) usedOutput ||
	 (unsigned int) f_output->id () == (unsigned int) ~0))
    {
	CompWindow    *w;
	GLMatrix      sTransform = transform;
	unsigned int  i;
	int           oy1 =
			 screen->outputDevs ()[usedOutput].region ()->extents.y1;
	int           oy2 =
			 screen->outputDevs ()[usedOutput].region ()->extents.y2;
	int           maxThumbHeight = (oy2 - oy1) * optionGetSize() / 100;
	int           oldFilter = gScreen->textureFilter ();

	if (optionGetMultioutputMode () == MultioutputModeOneBigSwitcher)
	{
	    oy1 = 0;
	    oy2 = screen->height ();
	}

	sTransform.toScreenSpace (f_output, -DEFAULT_Z_CAMERA);

	GLdouble clip[4] = { 0.0, -1.0, 0.0, 0.0};

	clip[3] = ((oy1 + (oy2 - oy1) / 2)) + (maxThumbHeight / 2.0);

	if (optionGetReflection ())
	{
	    GLMatrix       rTransform = sTransform;
	    unsigned short color[4];
	    int            cull, cullInv;
	    glGetIntegerv (GL_CULL_FACE_MODE, &cull);
	    cullInv = (cull == GL_BACK)? GL_FRONT : GL_BACK;

	    rTransform.translate (0.0, oy1 + oy2 + maxThumbHeight, 0.0);
	    rTransform.scale (1.0, -1.0, 1.0);

	    glPushMatrix ();
	    glLoadMatrixf (rTransform.getMatrix ());

	    glCullFace (cullInv);

	    if (optionGetMipmaps ())
		gScreen->setTextureFilter (GL_LINEAR_MIPMAP_LINEAR); // ???


	    if (anim == 1.0)
	    {
		glClipPlane (GL_CLIP_PLANE0, clip);
		glEnable (GL_CLIP_PLANE0);
	    }

	    reflectActive = true;
	    reflectBrightness = optionGetIntensity();

	    /* Draw reflected window first, so that reflection background
	       appears on top of it */
	    for (i = 0; i < drawSlots.size (); i++)
	    {
		w = drawSlots.at (i).w;
		SHIFT_WINDOW (w);

		activeSlot = &(drawSlots.at (i));
		{
		    sw->gWindow->glPaint (sw->gWindow->paintAttrib (), rTransform,
				      infiniteRegion, 0);
		}
	    }

	    /* Draw the reflective plane */

	    glDisable (GL_CLIP_PLANE0);
	    glCullFace (cull);

	    glLoadIdentity();
	    glTranslatef (0.0, 0.0, -DEFAULT_Z_CAMERA);

	    glEnable(GL_BLEND);
	    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	    glBegin (GL_QUADS);
	    glColor4f (0.0, 0.0, 0.0, 0.0);
	    glVertex2f (0.5, 0.0);
	    glVertex2f (-0.5, 0.0);
	    glColor4f (0.0, 0.0, 0.0,
		       MIN (1.0, 1.0 - optionGetIntensity ()) * 2.0 *
		       anim);
	    glVertex2f (-0.5, -0.5);
	    glVertex2f (0.5, -0.5);
	    glEnd();

	    if (optionGetGroundSize () > 0.0)
	    {
		glBegin (GL_QUADS);
		color[0] = optionGetGroundColor1 ()[0];
		color[1] = optionGetGroundColor1 ()[1];
		color[2] = optionGetGroundColor1 ()[2];
		color[3] = (float)optionGetGroundColor1 ()[3] * anim;
		glColor4usv (color);
		glVertex2f (-0.5, -0.5);
		glVertex2f (0.5, -0.5);
		color[0] = optionGetGroundColor2 ()[0];
		color[1] = optionGetGroundColor2 ()[1];
		color[2] = optionGetGroundColor2 ()[2];
		color[3] = (float)optionGetGroundColor2 ()[3] * anim;
		glColor4usv (color);
		glVertex2f (0.5, -0.5 + optionGetGroundSize ());
		glVertex2f (-0.5, -0.5 + optionGetGroundSize ());
		glEnd();
	    }

	    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	    glDisable(GL_BLEND);
	    glColor4f (1.0, 1.0, 1.0, 1.0);
	    glPopMatrix ();
	}

	glPushMatrix ();
	glLoadMatrixf (sTransform.getMatrix ());

	if (optionGetReflection () && anim == 1.0)
	{
	    glClipPlane (GL_CLIP_PLANE0, clip);
	    glEnable (GL_CLIP_PLANE0);
	}

	reflectBrightness = 1.0;
	reflectActive     = false;

	/* Draw non-reflected window */ 

	for (i = 0; i < drawSlots.size (); i++)
	{
	    w = drawSlots.at (i).w;

	    SHIFT_WINDOW (w);

	    activeSlot = &(drawSlots.at (i));
	    {
		sw->gWindow->glPaint (sw->gWindow->paintAttrib (), sTransform,
				     infiniteRegion, mask);
	    }
	}

	glDisable (GL_CLIP_PLANE0);
	
	activeSlot = NULL;

	gScreen->setTextureFilter (oldFilter);

	if ((state != ShiftScreen::ShiftStateIn))
	    drawWindowTitle ();

	if (state == ShiftScreen::ShiftStateIn || state == ShiftScreen::ShiftStateOut)
	{
	    bool found;
	    paintingAbove = true;

	    w = screen->findWindow (selectedWindow);
	    
	    for (; w; w = w->next)
	    {

		SHIFT_WINDOW (w);

		if (w->destroyed ())
		    continue;

		if (!w->shaded ())
		{
		    if (!w->isViewable ()| !sw->cWindow->damaged ()) // ???
			continue;
		}
		found = false;
		for (i = 0; i < windows.size (); i++)
		    if (windows. at(i) == w)
			found = true;
		if (found)
		    continue;

		sw->gWindow->glPaint (sw->gWindow->paintAttrib (), sTransform,
				  infiniteRegion, 0);
	    }

	    paintingAbove = false;
	}	
	
	glPopMatrix ();
    }

    return status;
}

void
ShiftScreen::paint (CompOutput::ptrList &outputs,
		    unsigned int mask)
{
    CompOutput::ptrList newOutputs = outputs;

    if (state != ShiftScreen::ShiftStateNone && outputs.size () > 0 &&
        optionGetMultioutputMode () != MultioutputModeDisabled)
    {
	newOutputs.push_back (&screen->fullscreenOutput ());
    }

    cScreen->paint (newOutputs, mask);
}

void
ShiftScreen::preparePaint (int msSinceLastPaint)
{
    if (state != ShiftScreen::ShiftStateNone &&
	(moreAdjust || moveAdjust))
    {
	int        steps;
	float      amount, chunk;
	int        i;

	amount = msSinceLastPaint * 0.05f * optionGetShiftSpeed ();
	steps  = amount / (0.5f * optionGetTimestep ());

	if (!steps) 
	    steps = 1;
	chunk  = amount / (float) steps;


	while (steps--)
	{
	    moveAdjust = adjustShiftMovement (chunk);
	    if (!moveAdjust)
		break;
	}
	
	amount = msSinceLastPaint * 0.05f * optionGetSpeed ();
	steps  = amount / (0.5f * optionGetTimestep ());

	if (!steps) 
	    steps = 1;
	chunk  = amount / (float) steps;

	while (steps--)
	{
	    moreAdjust = adjustShiftAnimationAttribs (chunk);

	    foreach (CompWindow *w, screen->windows ())
	    {
		SHIFT_WINDOW (w);

		moreAdjust |= sw->adjustShiftWindowAttribs (chunk);
		for (i = 0; i < 2; i++)
		{
		    ShiftSlot *slot = &sw->slots[i];
		    slot->tx = slot->x - w->x () -
			(w->width () * slot->scale) / 2;
		    slot->ty = slot->y - w->y () -
			(w->height () * slot->scale) / 2;
		}
	    }

	    if (!moreAdjust)
		break;
	}
    }

    cScreen->preparePaint (msSinceLastPaint);
}

bool
ShiftWindow::canStackRelativeTo ()
{
    if (window->overrideRedirect ())
        return false;

    if (!window->shaded () && !window->pendingMaps ())
    {
        if (!window->isViewable () || window->mapNum () == 0)
            return false;
    }

    return true;
}

void
ShiftScreen::donePaint ()
{
    if (state != ShiftScreen::ShiftStateNone)
    {
	if (moreAdjust)
	{
	    cScreen->damageScreen ();
	}
	else
	{
	    if (state == ShiftScreen::ShiftStateIn)
	    {
		state = ShiftScreen::ShiftStateNone;
		activateEvent (false);
		foreach (CompWindow *w, screen->windows ())
		{
		    ShiftWindow::get (w)->active = false;
		}
		toggleFunctions (false);
		drawSlots.clear ();
		windows.clear ();
		cScreen->damageScreen ();
	    }
	    else if (state == ShiftScreen::ShiftStateOut)
		state = ShiftScreen::ShiftStateSwitching;

	    if (moveAdjust)
	    {
		cScreen->damageScreen ();
	    }
	    else if (state == ShiftScreen::ShiftStateFinish)
	    {
		CompWindow *w;

		CompWindow *pw = NULL;
		unsigned int i;
		
		state = ShiftScreen::ShiftStateIn;
		moreAdjust = true;
		cScreen->damageScreen ();

		if (!canceled && mvTarget != 0)
		for (i = 0; i < drawSlots.size (); i++)
		{
		    w = drawSlots.at (i).w;
		    SHIFT_WINDOW (w);

		    if (drawSlots.at (i).slot->primary &&
			sw->canStackRelativeTo ())
		    {
			if (pw)
#warning: fixme: w->restackAbove (CompWindow * where NULL) is broken and causes compiz to not allow mouse-based stacking anymore
			    w->restackAbove (pw);
			pw = w;
		    }
		}

		if (!canceled && selectedWindow)
		{
		    w = screen->findWindow (selectedWindow);

		    if (w)
			screen->sendWindowActivationRequest (w->id ());
		}
	    }
	}
    }

    cScreen->donePaint ();
}

void
ShiftScreen::term (bool cancel)
{
    if (grabIndex)
    {
        screen->removeGrab (grabIndex, 0);
        grabIndex = 0;
    }

    if (state != ShiftScreen::ShiftStateNone)
    {

	if (cancel && mvTarget != 0)
	{
	    if (windows.size () - mvTarget > mvTarget)
		mvAdjust = -mvTarget;
	    else
		mvAdjust = windows.size () - mvTarget;
	    moveAdjust = true;
	}

	moreAdjust = true;
	state = ShiftScreen::ShiftStateFinish;
	canceled = cancel;
	cScreen->damageScreen ();
    }
}

bool
ShiftScreen::terminate (CompAction         *action,
			CompAction::State  aState,
			CompOption::Vector options)
{
    term ((aState & CompAction::StateCancel));

    if (aState & CompAction::StateTermButton)
    {
        action->setState (action->state () & (unsigned)~CompAction::StateTermButton);
    }

    if (aState & CompAction::StateTermKey)
    {
        action->setState (action->state () & (unsigned)~CompAction::StateTermKey);
    }

    return false;
}

bool
ShiftScreen::initiateScreen (CompAction         *action,
			     CompAction::State  aState,
			     CompOption::Vector options)
{
    CompMatch f_match;
    CompMatch defaultVal;
    int       count;

    if (screen->otherGrabExist ("shift", NULL))
	return false;
	   
    currentMatch = optionGetWindowMatch ();

    f_match = CompOption::getMatchOptionNamed (options, "match", defaultVal);
    if (!f_match.isEmpty ())
    {
	match = f_match;

	match.update ();
	currentMatch = match;
    }

    count = countWindows ();

    if (count < 1)
	return false;

    if (!grabIndex)
	grabIndex = screen->pushGrab (screen->invisibleCursor (), "shift");


    if (grabIndex)
    {
	state = ShiftScreen::ShiftStateOut;
	activateEvent (true);

	renderWindowTitle ();
	mvTarget = 0;
	mvAdjust = 0;
	mvVelocity = 0;

	toggleFunctions (true);

	if (!createWindowList ())
	    return false;

    	moreAdjust = true;
	cScreen->damageScreen ();
    }

    usedOutput = screen->currentOutputDev ().id ();
    
    return true;
}

bool
ShiftScreen::doSwitch (CompAction         *action,
		       CompAction::State  aState,
		       CompOption::Vector options,
		       bool		  nextWindow,
		       ShiftType	  aType)
{
    bool       ret = true;
    bool       initial = false;

    if ((state == ShiftScreen::ShiftStateNone) || (state == ShiftScreen::ShiftStateIn))
    {
        if (aType == ShiftTypeGroup)
        {
	    CompWindow *w;
	    w = screen->findWindow (CompOption::getIntOptionNamed (options, "window", 0));
	    if (w)
	    {
	        type = ShiftTypeGroup;
	        clientLeader =
		    (w->clientLeader ()) ? w->clientLeader () : w->id ();
	        ret = initiateScreen (action, aState, options);
	    }
        }
        else
        {
	    type = aType;
	    ret = initiateScreen (action, aState, options);
        }

        if (aState & CompAction::StateInitKey)
	    action->setState (action->state () | CompAction::StateTermKey);

        if (aState & CompAction::StateInitButton)
	    action->setState (action->state () | CompAction::StateTermButton);

        if (aState & CompAction::StateInitEdge)
	    action->setState (action->state () | CompAction::StateTermEdge);

        initial = true;
    }

    if (ret)
    {
        switchToWindow (nextWindow);
        if (initial && false)
        {
	    mvTarget += mvAdjust;
	    mvAdjust  = 0.0;
        }
    }

    return ret;
}

bool
ShiftScreen::initiate (CompAction         *action,
		       CompAction::State  aState,
		       CompOption::Vector options)
{
    bool       ret = true;

    type = ShiftTypeNormal;

    if ((state == ShiftScreen::ShiftStateNone) || (state == ShiftScreen::ShiftStateIn) ||
        (state == ShiftScreen::ShiftStateFinish))
        ret = initiateScreen (action, aState, options);
    else
        ret = terminate (action, aState, options);

    if (aState & CompAction::StateTermButton)
        action->setState (action->state () & ~CompAction::StateTermButton);

    if (aState & CompAction::StateTermKey)
        action->setState (action->state () & ~CompAction::StateTermKey);

    return ret;
}

bool
ShiftScreen::initiateAll (CompAction         *action,
			  CompAction::State  aState,
			  CompOption::Vector options)
{
    bool       ret = true;

    type = ShiftTypeAll;

    if ((state == ShiftScreen::ShiftStateNone) || (state == ShiftScreen::ShiftStateIn) ||
        (state == ShiftScreen::ShiftStateFinish))
        ret = initiateScreen (action, aState, options);
    else
        ret = terminate (action, aState, options);

    if (aState & CompAction::StateTermButton)
        action->setState (action->state () & ~CompAction::StateTermButton);

    if (aState & CompAction::StateTermKey)
        action->setState (action->state () & ~CompAction::StateTermKey);

    return ret;
}


void
ShiftScreen::windowRemove (Window id)
{
    CompWindow *w = screen->findWindow (id);
    if (w)
    {

	bool inList = false;
	Window selected;

	std::vector <CompWindow *>::iterator it = windows.end ();

	SHIFT_WINDOW (w);

	if (state == ShiftScreen::ShiftStateNone)
	    return;

	if (!sw->is ())
    	    return;

	selected = selectedWindow;

	while (it != windows.begin ())
	{
    	    if (w->id () == (*it)->id ())
	    {
		inList = true;

		if (w->id () == selected)
		{
		    if (it < windows.end ()--)
			selected = (*(it + 1))->id ();
    		    else
			selected = windows.front ()->id ();

		    selectedWindow = selected;
		}

		windows.erase (it); // ???
		break;
	    }
	    it--;
	}

	if (!inList)
	    return;

	if (windows.size () == 0)
	{
	    CompOption::Vector opts;
	    CompOption o ("root", CompOption::TypeInt);
	    o.value ().set ((int) screen->root ());

	    opts.push_back (o);

	    terminate (NULL, 0, opts);
	    return;
	}

	// Let the window list be updated to avoid crash
	// when a window is closed while ending shift (ShiftScreen::ShiftStateIn).
	if (!grabIndex && state != ShiftScreen::ShiftStateIn)
	    return;

	if (updateWindowList ())
	{
	    moreAdjust = true;
	    state = ShiftScreen::ShiftStateOut;
	    cScreen->damageScreen ();
	}
    }
}

void
ShiftScreen::handleEvent (XEvent *event)
{

    screen->handleEvent (event);

    switch (event->type) {
    case PropertyNotify:
	if (event->xproperty.atom == XA_WM_NAME)
	{
	    CompWindow *w;
	    w = screen->findWindow (event->xproperty.window);
	    if (w)
	    {
    		if (grabIndex && (w->id () == selectedWindow))
    		{
    		    renderWindowTitle ();
    		    cScreen->damageScreen ();
		}
	    }
	}
	break;
    case UnmapNotify:
	windowRemove (event->xunmap.window);
	break;
    case DestroyNotify:
	windowRemove (event->xdestroywindow.window);
	break;
    case KeyPress:

        if (state == ShiftScreen::ShiftStateSwitching)
        {
	    if (event->xkey.keycode == leftKey)
	        switchToWindow (false);
	    else if (event->xkey.keycode == rightKey)
	        switchToWindow (true);
	    else if (event->xkey.keycode == upKey)
	        switchToWindow (false);
	    else if (event->xkey.keycode == downKey)
	        switchToWindow (true);
        }

	break;
    case ButtonPress:

        if (state == ShiftScreen::ShiftStateSwitching || state == ShiftScreen::ShiftStateOut)
        {
	    if (event->xbutton.button == Button5)
	        switchToWindow (false);
	    else if (event->xbutton.button == Button4)
	        switchToWindow (true);
	    if (event->xbutton.button == Button1)
	    {
	        buttonPressTime = event->xbutton.time;
	        buttonPressed   = true;
	        startX          = event->xbutton.x_root;
	        startY          = event->xbutton.y_root;
	        startTarget     = mvTarget + mvAdjust;
	    }
        }
	break;
    case ButtonRelease:

        if (state == ShiftScreen::ShiftStateSwitching || state == ShiftScreen::ShiftStateOut)
        {
	    if (event->xbutton.button == Button1 && buttonPressed)
	    {
	        int new_i; // 'new' is reserved
	        if ((int)(event->xbutton.time - buttonPressTime) <
	            optionGetClickDuration ())
	        	term (false);

	        buttonPressTime = 0;
	        buttonPressed   = false;

	        if (mvTarget - floor (mvTarget) >= 0.5)
	        {
		    mvAdjust = ceil(mvTarget) - mvTarget;
		    new_i = ceil(mvTarget);
	        }
	        else
	        {
		    mvAdjust = floor(mvTarget) - mvTarget;
		    new_i = floor(mvTarget);
	        }

	        while (new_i < 0)
		    new_i += windows.size ();
	        new_i = new_i % windows.size ();

	        selectedWindow = windows.at (new_i)->id ();

	        renderWindowTitle ();
	        moveAdjust = true;
	        cScreen->damageScreen ();
	    }

        }
	break;
    case MotionNotify:
	{
	    if (state == ShiftScreen::ShiftStateSwitching || state == ShiftScreen::ShiftStateOut)
	    {
		if (buttonPressed)
		{
		    int ox1 = screen->outputDevs ()[usedOutput].region ()->extents.x1;
		    int ox2 = screen->outputDevs ()[usedOutput].region ()->extents.x2;
		    int oy1 = screen->outputDevs ()[usedOutput].region ()->extents.y1;
		    int oy2 = screen->outputDevs ()[usedOutput].region ()->extents.y2;

		    float div = 0;
		    int   wx  = 0;
		    int   wy  = 0;
		    int   new_i; // 'new' is reserved
		    
		    switch (optionGetMode ())
		    {
		    case ModeCover:
			div = event->xmotion.x_root - startX;
			div /= (ox2 - ox1) / optionGetMouseSpeed ();
			break;
		    case ModeFlip:
			div = event->xmotion.y_root - startY;
			div /= (oy2 - oy1) / optionGetMouseSpeed ();
			break;
		    }

		    mvTarget = startTarget + div - mvAdjust;
		    moveAdjust = true;
		    int count = 0;

		    while (mvTarget >= windows.size ())
		    {
			mvTarget -= windows.size ();
			invert = !invert;
			count++;
		    }

		    count = 0;

		    while (mvTarget < 0)
		    {
			mvTarget += windows.size ();
			invert = !invert;
		    }

		    if (mvTarget - floor (mvTarget) >= 0.5)
			new_i = ceil(mvTarget);
		    else
			new_i = floor(mvTarget);

		    count = 0;

		    while (new_i < 0)
		    {
			new_i += windows.size ();
		    }
		    new_i = new_i % windows.size ();

		    if (selectedWindow != windows.at (new_i)->id ())
		    {
		    	selectedWindow = windows.at (new_i)->id ();
			renderWindowTitle ();
		    }	        

		    if (event->xmotion.x_root < 50)
		        wx = 50;
		    if (screen->width () - event->xmotion.x_root < 50)
		        wx = -50;
		    if (event->xmotion.y_root < 50)
		        wy = 50;
		    if (screen->height () - event->xmotion.y_root < 50)
		        wy = -50;
		    if (wx != 0 || wy != 0)
		    {
			screen->warpPointer (wx, wy);
		        startX += wx;
		        startY += wy;
		    }

		    cScreen->damageScreen ();
		}
	    }
        }
	break;
    }
}

bool
ShiftWindow::damageRect (bool     initial,
			 const CompRect &rect)
{
    bool status = false;

    SHIFT_SCREEN (screen);

    if (initial)
    {
	if (ss->grabIndex && is ())
	{
	    ss->addWindowToList (window);
	    if (ss->updateWindowList ())
	    {
    		active = true;
		ss->moreAdjust = true;
		ss->state = ShiftScreen::ShiftStateOut;
		ss->cScreen->damageScreen ();
	    }
	}
    }
    else if (ss->state == ShiftScreen::ShiftStateSwitching)
    {
	if (active)
	{
	    ss->cScreen->damageScreen ();
	    status = true;
	}
    }

    status |= cWindow->damageRect (initial, rect);

    return status;
}

ShiftScreen::ShiftScreen (CompScreen *screen) :
    PluginClassHandler <ShiftScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    leftKey (XKeysymToKeycode (screen->dpy (), XStringToKeysym ("Left"))),
    rightKey (XKeysymToKeycode (screen->dpy (), XStringToKeysym ("Right"))),
    upKey (XKeysymToKeycode (screen->dpy (), XStringToKeysym ("Up"))),
    downKey (XKeysymToKeycode (screen->dpy (), XStringToKeysym ("Down"))),
    grabIndex (0),
    state (ShiftScreen::ShiftStateNone),
    moreAdjust (false),
    mvTarget (0),
    mvVelocity (0),
    invert (false),
    cursor (XCreateFontCursor (screen->dpy (), XC_left_ptr)),
    activeSlot (NULL),
    usedOutput (0),
    anim (0.0),
    animVelocity (0.0),
    buttonPressTime (0),
    buttonPressed (false),
    startX (0),
    startY (0),
    startTarget (0.0f)
{
    ScreenInterface::setHandler (screen, false);
    CompositeScreenInterface::setHandler (cScreen, false);
    GLScreenInterface::setHandler (gScreen, false);

    /* Key actions */

    optionSetInitiateKeyInitiate (boost::bind (&ShiftScreen::initiate, this,
					       _1, _2, _3));
    optionSetInitiateKeyTerminate (boost::bind (&ShiftScreen::terminate, this,
					       _1, _2, _3));
    optionSetInitiateAllKeyInitiate (boost::bind (&ShiftScreen::initiateAll,
						  this,  _1, _2, _3));
    optionSetInitiateAllKeyTerminate (boost::bind (&ShiftScreen::terminate, this,
					       _1, _2, _3));
    optionSetNextKeyInitiate (boost::bind (&ShiftScreen::doSwitch, this,
					   _1, _2, _3, true, ShiftTypeNormal));
    optionSetNextKeyTerminate (boost::bind (&ShiftScreen::terminate, this,
					      _1, _2, _3));
    optionSetPrevKeyInitiate (boost::bind (&ShiftScreen::doSwitch, this,
					   _1, _2, _3, false, ShiftTypeNormal));
    optionSetPrevKeyTerminate (boost::bind (&ShiftScreen::terminate, this,
					      _1, _2, _3));
    optionSetNextAllKeyInitiate (boost::bind (&ShiftScreen::doSwitch, this,
					   _1, _2, _3, true, ShiftTypeAll));
    optionSetNextAllKeyTerminate (boost::bind (&ShiftScreen::terminate, this,
					      _1, _2, _3));
    optionSetPrevAllKeyInitiate (boost::bind (&ShiftScreen::doSwitch, this,
					   _1, _2, _3, false, ShiftTypeAll));
    optionSetPrevAllKeyTerminate (boost::bind (&ShiftScreen::terminate, this,
					      _1, _2, _3));
    optionSetNextGroupKeyInitiate (boost::bind (&ShiftScreen::doSwitch, this,
					   _1, _2, _3, true, ShiftTypeGroup));
    optionSetNextGroupKeyTerminate (boost::bind (&ShiftScreen::terminate, this,
					      _1, _2, _3));
    optionSetPrevGroupKeyInitiate (boost::bind (&ShiftScreen::doSwitch, this,
					   _1, _2, _3, false, ShiftTypeGroup));
    optionSetPrevGroupKeyTerminate (boost::bind (&ShiftScreen::terminate, this,
					      _1, _2, _3));

    /* Button Actions */

    optionSetInitiateButtonInitiate (boost::bind (&ShiftScreen::initiate, this,
					       _1, _2, _3));
    optionSetInitiateButtonTerminate (boost::bind (&ShiftScreen::terminate, this,
					       _1, _2, _3));
    optionSetInitiateAllButtonInitiate (boost::bind (&ShiftScreen::initiateAll,
						  this,  _1, _2, _3));
    optionSetInitiateAllButtonTerminate (boost::bind (&ShiftScreen::terminate, this,
					       _1, _2, _3));
    optionSetNextButtonInitiate (boost::bind (&ShiftScreen::doSwitch, this,
					   _1, _2, _3, true, ShiftTypeNormal));
    optionSetNextButtonTerminate (boost::bind (&ShiftScreen::terminate, this,
					      _1, _2, _3));
    optionSetPrevButtonInitiate (boost::bind (&ShiftScreen::doSwitch, this,
					   _1, _2, _3, false, ShiftTypeNormal));
    optionSetPrevButtonTerminate (boost::bind (&ShiftScreen::terminate, this,
					      _1, _2, _3));
    optionSetNextAllButtonInitiate (boost::bind (&ShiftScreen::doSwitch, this,
					   _1, _2, _3, true, ShiftTypeAll));
    optionSetNextAllButtonTerminate (boost::bind (&ShiftScreen::terminate, this,
					      _1, _2, _3));
    optionSetPrevAllButtonInitiate (boost::bind (&ShiftScreen::doSwitch, this,
					   _1, _2, _3, false, ShiftTypeAll));
    optionSetPrevAllButtonTerminate (boost::bind (&ShiftScreen::terminate, this,
					      _1, _2, _3));
    optionSetNextGroupButtonInitiate (boost::bind (&ShiftScreen::doSwitch, this,
					   _1, _2, _3, true, ShiftTypeGroup));
    optionSetNextGroupButtonTerminate (boost::bind (&ShiftScreen::terminate, this,
					      _1, _2, _3));
    optionSetPrevGroupButtonInitiate (boost::bind (&ShiftScreen::doSwitch, this,
					   _1, _2, _3, false, ShiftTypeGroup));
    optionSetPrevGroupButtonTerminate (boost::bind (&ShiftScreen::terminate, this,
					      _1, _2, _3));

    /* Edge Actions */

    optionSetInitiateEdgeInitiate (boost::bind (&ShiftScreen::initiate, this,
						_1, _2, _3));
    optionSetInitiateEdgeTerminate (boost::bind (&ShiftScreen::terminate, this,
						_1, _2, _3));
    optionSetInitiateAllEdgeInitiate (boost::bind (&ShiftScreen::initiateAll, this,
						_1, _2, _3));
    optionSetInitiateAllEdgeTerminate (boost::bind (&ShiftScreen::terminate, this,
						_1, _2, _3));

    optionSetTerminateButtonInitiate (boost::bind (&ShiftScreen::terminate,
						   this, _1, _2, _3));
}

ShiftScreen::~ShiftScreen ()
{
    freeWindowTitle ();

    XFreeCursor (screen->dpy (), cursor);

    windows.clear ();
    drawSlots.clear ();
}

ShiftWindow::ShiftWindow (CompWindow *window) :
    PluginClassHandler <ShiftWindow, CompWindow> (window),
    window (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window)),
    opacity (1.0),
    brightness (1.0)
{
    CompositeWindowInterface::setHandler (cWindow, false);
    GLWindowInterface::setHandler (gWindow, false);

    slots[0].scale = 1.0;
    slots[1].scale = 1.0;
}

ShiftWindow::~ShiftWindow ()
{
}

bool
ShiftPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
    	return false;

    if (!CompPlugin::checkPluginABI ("text", COMPIZ_TEXT_ABI))
    {
	compLogMessage ("shift", CompLogLevelWarn, "No compatible text plugin"\
						   " loaded");
	textAvailable = false;
    }
    else
	textAvailable = true;

    return true;
}
