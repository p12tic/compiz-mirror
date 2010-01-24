/*
 *
 * Compiz stackswitch switcher plugin
 *
 * stackswitch.c
 *
 * Copyright : (C) 2007 by Danny Baumann
 * E-mail    : maniac@opencompositing.org
 *
 * Based on scale.c and switcher.c:
 * Copyright : (C) 2007 David Reveman
 * E-mail    : davidr@novell.com
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

#include "stackswitch.h"

COMPIZ_PLUGIN_20090315 (stackswitch, StackswitchPluginVTable);

bool textAvailable;

bool
StackswitchWindow::isStackswitchable ()
{
    STACKSWITCH_SCREEN (screen);

    if (window->overrideRedirect ())
	return false;

    if (window->wmType () & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	return false;

    if (!window->mapNum () || window->invisible ())
    {
	if (ss->optionGetMinimized ())
	{
	    if (!window->minimized () && !window->inShowDesktopMode () && !window->shaded ())
		return false;
	}
	else
    	    return false;
    }

    if (ss->mType == StackswitchScreen::TypeNormal)
    {
	if (!window->mapNum () || window->invisible ())
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
    else if (ss->mType == StackswitchScreen::TypeGroup &&
	     ss->mClientLeader != window->clientLeader () &&
	     ss->mClientLeader != window->id ())
    {
	return false;
    }

    if (window->state () & CompWindowStateSkipTaskbarMask)
	return false;

    if (!ss->mCurrentMatch.evaluate (window))
	return false;

    return true;
}

void
StackswitchScreen::renderWindowTitle ()
{
    CompText::Attrib tA;
    int            ox1, ox2, oy1, oy2;
    bool           showViewport;

    if (!textAvailable)
	return;

    if (!optionGetWindowTitle ())
	return;

    CompRect output = screen->getCurrentOutputExtents ();
    
    ox1 = output.x ();
    oy1 = output.y ();
    ox2 = output.x2 ();
    oy2 = output.y2 ();

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

    showViewport = mType == StackswitchScreen::TypeAll;
    mText.renderWindowTitle (mSelectedWindow, showViewport, tA);
}

void
StackswitchScreen::drawWindowTitle (GLMatrix &transform,
				    CompWindow *w)
{
    GLboolean     wasBlend;
    GLint         oldBlendSrc, oldBlendDst;
    GLMatrix      wTransform (transform), mvp;
    float         x, fY, tx, ix, width, height;
    int           ox1, ox2, oy1, oy2;
    GLTexture::Matrix    m;
    GLVector    v;
    GLTexture     *icon;
    
    STACKSWITCH_WINDOW (w);

    CompRect output = screen->getCurrentOutputExtents ();
    
    ox1 = output.x ();
    oy1 = output.y ();
    ox2 = output.x2 ();
    oy2 = output.y2 ();

    width = mText.getWidth ();
    height = mText.getHeight ();

    x = ox1 + ((ox2 - ox1) / 2);
    tx = x - (mText.getWidth () / 2);

    switch (optionGetTitleTextPlacement ())
    {
	case StackswitchOptions::TitleTextPlacementOnThumbnail:
	{
	    GLVector v (w->x () + (w->width () / 2.0),
	    		w->y () + (w->width () / 2.0),
			0.0f,
			1.0f);
	    GLMatrix pm (gScreen->projectionMatrix ());

	    wTransform.scale (1.0, 1.0, 1.0 / screen->height ());
	    wTransform.translate (sw->mTx, sw->mTy, 0.0f);
	    wTransform.rotate (-mRotation, 1.0, 0.0, 0.0);
	    wTransform.scale (sw->mScale, sw->mScale, 1.0);
	    wTransform.translate (w->input ().left, 0.0 -(w->height () + w->input ().bottom), 0.0f);
	    wTransform.translate (-w->x (), -w->y (), 0.0f);

	    mvp = pm * wTransform;;
	    v = mvp * v;
	    v.homogenize ();

	    x = (v[GLVector::x] + 1.0) * (ox2 - ox1) * 0.5;
	    fY = (v[GLVector::y] - 1.0) * (oy2 - oy1) * -0.5;

	    x += ox1;
	    fY += oy1;

	    tx = MAX (ox1, x - (width / 2.0));
	    if (tx + width > ox2)
		tx = ox2 - width;
	    break;
	}
	case TitleTextPlacementCenteredOnScreen:
	    fY = oy1 + ((oy2 - oy1) / 2) + (height / 2);
	    break;
	case TitleTextPlacementAbove:
	case TitleTextPlacementBelow:
	    {
		CompRect workArea;
		workArea = screen->getWorkareaForOutput (screen->currentOutputDev ().id ());

	    	if (optionGetTitleTextPlacement () ==
		    TitleTextPlacementAbove)
    		    fY = oy1 + workArea.y () + height;
		else
		    fY = oy1 + workArea.y () + workArea.height () - 96;
	    }
	    break;
	default:
	    return;
	    break;
    }

    tx = floor (tx);
    fY  = floor (fY);

    glGetIntegerv (GL_BLEND_SRC, &oldBlendSrc);
    glGetIntegerv (GL_BLEND_DST, &oldBlendDst);

    wasBlend = glIsEnabled (GL_BLEND);
    if (!wasBlend)
	glEnable (GL_BLEND);

    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f (1.0, 1.0, 1.0, 1.0);

    icon = sw->gWindow->getIcon (96, 96);
    if (!icon)
	icon = gScreen->defaultIcon ();

    if (icon && (icon->name ()))
    {
	int                 off;

	ix = x - (icon->width () / 2.0);
	ix  = floor (ix);

	icon->enable (GLTexture::Good);

	m = icon->matrix ();

	glColor4f (0.0, 0.0, 0.0, 0.1);

	for (off = 0; off < 6; off++)
	{
	    glBegin (GL_QUADS);

	    glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m ,0));
	    glVertex2f (ix - off, fY - off);
	    glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m, icon->height ()));
	    glVertex2f (ix - off, fY + icon->height () + off);
	    glTexCoord2f (COMP_TEX_COORD_X (m, icon->width ()), COMP_TEX_COORD_Y (m, icon->height ()));
	    glVertex2f (ix + icon->width () + off, fY + icon->height () + off);
	    glTexCoord2f (COMP_TEX_COORD_X (m, icon->width ()), COMP_TEX_COORD_Y (m, 0));
	    glVertex2f (ix + icon->width () + off, fY - off);

	    glEnd ();
	}
	
	glColor4f (1.0, 1.0, 1.0, 1.0);

	glBegin (GL_QUADS);

	glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m ,0));
	glVertex2f (ix, fY);
	glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m, icon->height ()));
	glVertex2f (ix, fY + icon->height ());
	glTexCoord2f (COMP_TEX_COORD_X (m, icon->width ()), COMP_TEX_COORD_Y (m, icon->height ()));
	glVertex2f (ix + icon->width (), fY + icon->height ());
	glTexCoord2f (COMP_TEX_COORD_X (m, icon->width ()), COMP_TEX_COORD_Y (m, 0));
	glVertex2f (ix + icon->width (), fY);

	glEnd ();
	
	icon->disable ();
    }

    mText.draw (tx, fY, 1.0f);
    #if 0
    m = &textData->texture->matrix;

    glBegin (GL_QUADS);

    glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m ,0));
    glVertex2f (tx, fY - height);
    glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m, height));
    glVertex2f (tx, fY);
    glTexCoord2f (COMP_TEX_COORD_X (m, width), COMP_TEX_COORD_Y (m, height));
    glVertex2f (tx + width, fY);
    glTexCoord2f (COMP_TEX_COORD_X (m, width), COMP_TEX_COORD_Y (m, 0));
    glVertex2f (tx + width, fY - height);

    glEnd ();

    disableTexture (s, textData->texture);
    #endif
    glColor4usv (defaultColor);

    if (!wasBlend)
	glDisable (GL_BLEND);
    glBlendFunc (oldBlendSrc, oldBlendDst);
}

bool
StackswitchWindow::glPaint (const GLWindowPaintAttrib &attrib,
			    const GLMatrix	      &transform,
			    const CompRegion	      &region,
			    unsigned int	      mask)
{
    bool       status;

    STACKSWITCH_SCREEN (screen);

    if (ss->mState != StackswitchScreen::StateNone)
    {
	GLWindowPaintAttrib sAttrib (attrib);
	bool		  scaled = false;
	float             rotation;

    	if (window->mapNum ())
	{
	    if (!gWindow->textures ().empty ())
		gWindow->bind ();
	}

	if (mAdjust || mSlot)
	{
	    scaled = (mAdjust && ss->mState != StackswitchScreen::StateSwitching) ||
		     (mSlot && ss->mPaintingSwitcher);
	    mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;
	}
	else if (ss->mState != StackswitchScreen::StateIn)
	{
	    if (ss->optionGetDarkenBack ())
	    {
		/* modify brightness of the other windows */
		sAttrib.brightness = sAttrib.brightness / 2;
	    }
	}
	
	status = gWindow->glPaint (sAttrib, transform, region, mask);

	if (ss->optionGetInactiveRotate ())
	    rotation = MIN (mRotation, ss->mRotation);
	else
	    rotation = ss->mRotation;

	if (scaled && gWindow->textures ().size ())
	{
	    GLFragment::Attrib fragment (gWindow->lastPaintAttrib ());
	    GLMatrix           wTransform (transform);

	    if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
		return false;

	    if (mSlot)
	    {
		if (window->id () != ss->mSelectedWindow)
		    fragment.setOpacity ((float)fragment.getOpacity () *
			                   ss->optionGetInactiveOpacity () / 100);
	    }

	    if (window->alpha () || fragment.getOpacity () != OPAQUE)
		mask |= PAINT_WINDOW_TRANSLUCENT_MASK;


	    wTransform.scale (1.0, 1.0, 1.0 / screen->height ());
	    wTransform.translate (mTx, mTy, 0.0f);

	    wTransform.rotate (-rotation, 1.0, 0.0, 0.0);
	    wTransform.scale (mScale, mScale, 1.0);

	    wTransform.translate (window->input ().left, 0.0 -(window->height () + window->input ().bottom), 0.0f);
	    wTransform.translate (-window->x (), -window->y (), 0.0f);
	    glPushMatrix ();
	    glLoadMatrixf (wTransform.getMatrix ());

	    gWindow->glDraw (wTransform, fragment, region, mask |
	    		     PAINT_WINDOW_TRANSFORMED_MASK);

	    glPopMatrix ();
	}

	if (scaled && !gWindow->textures ().size ())
	{
	    GLTexture *icon;

	    icon = gWindow->getIcon (96, 96);
	    if (!icon)
		icon = ss->gScreen->defaultIcon ();

	    if (icon && (icon->name ()))
	    {
		CompRegion          iconReg (0, 0, icon->width (), icon->height ());
		GLTexture::Matrix   matrix;
		GLTexture::MatrixList matl;
		float               scale;

		scale = MIN (window->width () / icon->width (),
			     window->height () / icon->height ());
		scale *= mScale;

		mask |= PAINT_WINDOW_BLEND_MASK;
		
		/* if we paint the icon for a minimized window, we need
		   to force the usage of a good texture filter */
		if (!gWindow->textures ().size ())
		    mask |= PAINT_WINDOW_TRANSFORMED_MASK;

		matrix = icon->matrix ();
		matl.push_back (matrix);

		gWindow->geometry ().vCount =
			 gWindow->geometry ().indexCount = 0;
		gWindow->glAddGeometry (matl, iconReg, infiniteRegion);

		if (gWindow->geometry ().vCount)
		{
		    GLMatrix           wTransform (transform);

		    if (!gWindow->textures ().size ())
			sAttrib.opacity = gWindow->paintAttrib ().opacity;

		    GLFragment::Attrib fragment (sAttrib);

		    wTransform.scale (1.0, 1.0, 1.0 / screen->height ());
		    wTransform.translate (mTx, mTy, 0.0f);

		    wTransform.rotate (-rotation, 1.0, 0.0, 0.0);
		    wTransform.scale (scale, scale, 1.0);

		    wTransform.translate (0.0, -icon->height (), 0.0f);

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

bool
StackswitchWindow::compareWindows (CompWindow *w1,
				   CompWindow *w2)
{
    if (w1->mapNum () && !w2->mapNum ())
	return false;

    if (w2->mapNum () && !w1->mapNum ())
	return true;

    if (w2->activeNum () - w1->activeNum ())
	return true;
    else
	return false;
}

bool
StackswitchWindow::compareStackswitchWindowDepth (StackswitchDrawSlot *a1,
						  StackswitchDrawSlot *a2)
{
    StackswitchSlot *s1 = *(a1->slot);
    StackswitchSlot *s2 = *(a2->slot);

    if (s1->y < s2->y)
	return true;
    else if (s1->y > s2->y)
	return false;
    else
    {
	CompWindow *w1   = a1->w;
        CompWindow *w2   = a2->w;

	STACKSWITCH_SCREEN (screen);
	if (w1->id () == ss->mSelectedWindow)
	    return false;
	else if (w2->id () == ss->mSelectedWindow)
	    return true;
	else
	    return false;
    }
}

bool
StackswitchScreen::layoutThumbs ()
{
    int        index;
    int        ww, wh;
    float      xScale, yScale;
    int        ox1, ox2, oy1, oy2;
    float      swi = 0.0, oh, rh, ow;
    int        cols, rows, col = 0, row = 0, r, c;
    int        cindex, ci, gap, hasActive = 0;
    bool       exit;

    if ((mState == StateNone) || (mState == StateIn))
	return false;

    CompRect output = screen->getCurrentOutputExtents ();
    
    ox1 = output.x ();
    oy1 = output.y ();
    ox2 = output.x2 ();
    oy2 = output.y2 ();
    ow = (float)(ox2 - ox1) * 0.9;

    foreach (CompWindow *w, mWindows)
    {
	ww = w->width ()  + w->input ().left + w->input ().right;
	wh = w->height () + w->input ().top  + w->input ().bottom;

	swi += ((float)ww / (float)wh) * (ow / (float)(oy2 - oy1));
    }

    cols = ceil (sqrtf (swi));

    swi = 0.0;
    foreach (CompWindow *w, mWindows)
    {
	ww = w->width ()  + w->input ().left + w->input ().right;
	wh = w->height () + w->input ().top  + w->input ().bottom;

	swi += (float)ww / (float)wh;

	if (swi > cols)
	{
	    row++;
	    swi = (float)ww / (float)wh;
	    col = 0;
	}

	col++;
    }
    rows = row + 1;

    oh = ow / cols;
    oh *= (float)(oy2 - oy1) / (float)(oy2 - oy1);

    rh = ((float)(oy2 - oy1) * 0.8) / rows;

    index = 0;
    foreach (CompWindow *w, mWindows)
    {
	StackswitchDrawSlot *dSlot;
	STACKSWITCH_WINDOW (w);

	if (!sw->mSlot)
	    sw->mSlot = new StackswitchSlot;

	if (!sw->mSlot)
	    return false;
	    
	dSlot = new StackswitchDrawSlot;

	mDrawSlots.at (index) = dSlot;

	mDrawSlots.at (index)->w    = w;
	mDrawSlots.at (index)->slot = &sw->mSlot;
	
	index++;
    }

    index = 0;

    for (r = 0; r < rows && index < mWindows.size (); r++)
    {
        CompWindow *w;
	c = 0;
	swi = 0.0;
	cindex = index;
	exit = false;
	while (index < mWindows.size () && !exit)
	{
	    w = mWindows.at (index);

	    STACKSWITCH_WINDOW (w);
	    sw->mSlot->x = ox1 + swi;
	    sw->mSlot->y = oy2 - (rh * r) - ((oy2 - oy1) * 0.1);


	    ww = w->width ()  + w->input ().left + w->input ().right;
	    wh = w->height () + w->input ().top  + w->input ().bottom;

	    if (ww > ow)
		xScale = ow / (float) ww;
	    else
		xScale = 1.0f;

	    if (wh > oh)
		yScale = oh / (float) wh;
	    else
		yScale = 1.0f;

	    sw->mSlot->scale = MIN (xScale, yScale);

	    if (swi + (ww * sw->mSlot->scale) > ow && cindex != index)
	    {
		exit = true;
		continue;
	    }

	    if (w->id () == mSelectedWindow)
		hasActive = 1;
	    swi += ww * sw->mSlot->scale;

	    c++;
	    index++;
	}

	gap = ox2 - ox1 - swi;
	gap /= c + 1;

	index = cindex;
	ci = 1;
	while (ci <= c)
	{
	    w = mWindows.at (index);

	    STACKSWITCH_WINDOW (w);
	    sw->mSlot->x += ci * gap;

	    if (hasActive == 0)
		sw->mSlot->y += sqrt(2 * oh * oh) - rh;

	    ci++;
	    index++;
	}

	if (hasActive == 1)
	    hasActive++;
    }

    /* sort the draw list so that the windows with the 
       lowest Y value (the windows being farest away)
       are drawn first */
    sort (mDrawSlots.begin (), mDrawSlots.end (),
    	  StackswitchWindow::compareStackswitchWindowDepth);

    return true;
}

void
StackswitchWindow::addToList ()
{
    STACKSWITCH_SCREEN (screen);
    
    ss->mWindows.push_back (window);
    ss->mDrawSlots.resize (ss->mWindows.size ());
}

bool
StackswitchScreen::updateWindowList ()
{
    sort (mWindows.begin (), mWindows.end (), StackswitchWindow::compareWindows);
    
    return layoutThumbs ();
}


bool
StackswitchScreen::createWindowList ()
{
    foreach (CompWindow *w, screen->windows ())
    {
        STACKSWITCH_WINDOW (w);
	if (sw->isStackswitchable ())
	{
	    STACKSWITCH_WINDOW (w);

	    sw->addToList ();
	    sw->mAdjust = true;
	}
    }

    return updateWindowList ();
}

void
StackswitchScreen::switchToWindow (bool toNext)
{
    CompWindow *w = NULL, *itwin;
    std::vector <CompWindow *>::iterator it;

    if (!mGrabIndex)
	return;

    foreach (itwin, screen->windows ())
    {
	if (itwin->id () == mSelectedWindow)
	    break;
    }

    it = std::find (mWindows.begin (), mWindows.end (), itwin);

    if (it == mWindows.end ())
	return;

    if (toNext)
    {
        it++;
        if (it == mWindows.end ())
            w = mWindows.front ();
        else
            w = *it;
    }
    else
    {
        if (it == mWindows.begin ())
            w = mWindows.back ();
        else
        {
            it--;
            w = *it;
        }
    }

    if (w)
    {
	Window old = mSelectedWindow;

	mSelectedWindow = w->id ();
	if (old != w->id ())
	{
	    mRotateAdjust = true;
	    mMoreAdjust = true;
	    foreach (CompWindow *win, screen->windows ())
	    {
		STACKSWITCH_WINDOW (win);
		sw->mAdjust = true;
	    }

	    cScreen->damageScreen ();
	    renderWindowTitle ();
	}
    }
}

int
StackswitchScreen::countWindows ()
{
    int	       count = 0;

    foreach (CompWindow *w, screen->windows ())
    {
        STACKSWITCH_WINDOW (w);
        
	if (sw->isStackswitchable ())
	    count++;
    }

    return count;
}

int
StackswitchScreen::adjustStackswitchRotation (float chunk)
{
    float dx, adjust, amount, rot;

    if (mState != StateNone && mState != StateIn)
	rot = optionGetTilt ();
    else
	rot = 0.0;

    dx = rot - mRotation;

    adjust = dx * 0.15f;
    amount = fabs (dx) * 1.5f;
    if (amount < 0.2f)
	amount = 0.2f;
    else if (amount > 2.0f)
	amount = 2.0f;

    mRVelocity = (amount * mRVelocity + adjust) / (amount + 1.0f);

    if (fabs (dx) < 0.1f && fabs (mRVelocity) < 0.2f)
    {
	mRVelocity = 0.0f;
	mRotation = rot;
	return false;
    }

    mRotation += mRVelocity * chunk;
    return true;
}

int
StackswitchWindow::adjustStackswitchVelocity ()
{
    float dx, dy, ds, dr, adjust, amount;
    float x1, y1, scale, rot;
    STACKSWITCH_SCREEN (screen);

    if (mSlot)
    {
	scale = mSlot->scale;
	x1 = mSlot->x;
	y1 = mSlot->y;
    }
    else
    {
	scale = 1.0f;
	x1 = window->x () - window->input ().left;
	y1 = window->y () + window->height () + window->input ().bottom;
    }

    if (window->id () == ss->mSelectedWindow)
	rot = ss->mRotation;
    else
	rot = 0.0;

    dx = x1 - mTx;

    adjust = dx * 0.15f;
    amount = fabs (dx) * 1.5f;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    mXVelocity = (amount * mXVelocity + adjust) / (amount + 1.0f);

    dy = y1 - mTy;

    adjust = dy * 0.15f;
    amount = fabs (dy) * 1.5f;
    if (amount < 0.5f)
	amount = 0.5f;
    else if (amount > 5.0f)
	amount = 5.0f;

    mYVelocity = (amount * mYVelocity + adjust) / (amount + 1.0f);

    ds = scale - mScale;
    adjust = ds * 0.1f;
    amount = fabs (ds) * 7.0f;
    if (amount < 0.01f)
	amount = 0.01f;
    else if (amount > 0.15f)
	amount = 0.15f;

    mScaleVelocity = (amount * mScaleVelocity + adjust) /
	(amount + 1.0f);

    dr = rot - mRotation;
    adjust = dr * 0.15f;
    amount = fabs (dr) * 1.5f;
    if (amount < 0.2f)
	amount = 0.2f;
    else if (amount > 2.0f)
	amount = 2.0f;

    mRotVelocity = (amount * mRotVelocity + adjust) / (amount + 1.0f);
    
    if (fabs (dx) < 0.1f && fabs (mXVelocity) < 0.2f &&
	fabs (dy) < 0.1f && fabs (mYVelocity) < 0.2f &&
	fabs (ds) < 0.001f && fabs (mScaleVelocity) < 0.002f &&
        fabs (dr) < 0.1f && fabs (mRotVelocity) < 0.2f)
    {
	mXVelocity = mYVelocity = mScaleVelocity = 0.0f;
	mTx = x1;
	mTy = y1;
	mRotation = rot;
	mScale = scale;

	return 0;
    }

    return 1;
}

bool
StackswitchScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
				  const GLMatrix            &transform,
				  const CompRegion	    &region,
				  CompOutput		    *output,
				  unsigned int		    mask)
{
    bool status;
    GLMatrix sTransform = transform;

    if (mState != StateNone || mRotation != 0.0)
    {
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
	mask |= PAINT_SCREEN_TRANSFORMED_MASK;
	mask |= PAINT_SCREEN_CLEAR_MASK;
	sTransform.translate (0.0, -0.5, -DEFAULT_Z_CAMERA);
	sTransform.rotate (-mRotation, 1.0, 0.0, 0.0);
	sTransform.translate (0.0, 0.5, DEFAULT_Z_CAMERA);
    }
    
    status = gScreen->glPaintOutput (attrib, sTransform, region, output, mask);

    if (mState != StateNone && (output->id () == ~0 ||
	screen->outputDevs ().at (screen->currentOutputDev ().id ()) == *output))
    {
	int           i;
	CompWindow    *aw = NULL;
	
	sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);
	
	glPushMatrix ();
	glLoadMatrixf (sTransform.getMatrix ());

	mPaintingSwitcher = true;

	for (i = 0; i < mWindows.size (); i++)
	{
	    if (mDrawSlots.at (i)->slot && *(mDrawSlots.at (i)->slot))
	    {
		CompWindow *w = mDrawSlots.at (i)->w;
		if (w->id () == mSelectedWindow)
		    aw = w;

		STACKSWITCH_WINDOW (w);
		
		sw->gWindow->glPaint (sw->gWindow->paintAttrib (), sTransform,
				      infiniteRegion, 0);
	    }
	}
	

	GLMatrix tTransform (transform);
	tTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);
	glLoadMatrixf (tTransform.getMatrix ());
	
	if ((mState != StateIn) && aw)
	    drawWindowTitle (sTransform, aw);
	
	mPaintingSwitcher = false;
	
	glPopMatrix ();
    }

    return status;
}

void
StackswitchScreen::preparePaint (int msSinceLastPaint)
{
    if (mState != StateNone && (mMoreAdjust || mRotateAdjust))
    {
	int        steps;
	float      amount, chunk;

	amount = msSinceLastPaint * 0.05f * optionGetSpeed ();
	steps  = amount / (0.5f * optionGetTimestep ());

	if (!steps) 
	    steps = 1;
	chunk  = amount / (float) steps;

	layoutThumbs ();
	while (steps--)
	{
	    mRotateAdjust = adjustStackswitchRotation (chunk);
	    mMoreAdjust = false;

	    foreach (CompWindow *w, screen->windows ())
	    {
		STACKSWITCH_WINDOW (w);

		if (sw->mAdjust)
		{
		    sw->mAdjust = sw->adjustStackswitchVelocity ();

		    mMoreAdjust |= sw->mAdjust;

		    sw->mTx       += sw->mXVelocity * chunk;
		    sw->mTy       += sw->mYVelocity * chunk;
		    sw->mScale    += sw->mScaleVelocity * chunk;
		    sw->mRotation += sw->mRotVelocity * chunk;
		}
		else if (sw->mSlot)
		{
		    sw->mScale    = sw->mSlot->scale;
	    	    sw->mTx       = sw->mSlot->x;
	    	    sw->mTy       = sw->mSlot->y;
		    if (w->id () == mSelectedWindow)
			sw->mRotation = mRotation;
		    else
			sw->mRotation = 0.0;
		}
	    }

	    if (!mMoreAdjust && !mRotateAdjust)
		break;
	}
    }
    
    cScreen->preparePaint (msSinceLastPaint);
}

void
StackswitchScreen::donePaint ()
{
    if (mState != StateNone)
    {
	if (mMoreAdjust)
	{
	    cScreen->damageScreen ();
	}
	else
	{
	    if (mRotateAdjust)
		cScreen->damageScreen ();

	    if (mState == StateIn)
		mState = StateNone;
	    else if (mState == StateOut)
		mState = StateSwitching;
	}
    }
    
    cScreen->donePaint ();
}

bool
StackswitchScreen::terminate (CompAction *action,
			      CompAction::State state,
			      CompOption::Vector options)
{
    if (mGrabIndex)
    {
        screen->removeGrab (mGrabIndex, 0);
        mGrabIndex = 0;
    }

    if (mState != StateNone)
    {
        CompWindow *w;

        foreach (CompWindow *w, screen->windows ())
        {
	    STACKSWITCH_WINDOW (w);

	    if (sw->mSlot)
	    {
	        delete sw->mSlot;
	        sw->mSlot = NULL;

	        sw->mAdjust = true;
	    }
        }
        mMoreAdjust = true;
        mState = StateIn;
        cScreen->damageScreen ();

        if (!(state & CompAction::StateCancel) && mSelectedWindow)
        {
	    w = screen->findWindow (mSelectedWindow);
	    if (w)
	        screen->sendWindowActivationRequest (w->id ());
        }
    }
    

    if (action)
	action->setState (action->state () & ~(CompAction::StateTermKey |
			   		       CompAction::StateTermButton |
			   		       CompAction::StateTermEdge));

    return false;
}

bool
StackswitchScreen::initiate (CompAction         *action,
			     CompAction::State  state,
			     CompOption::Vector options)
{
    CompMatch  match;
    CompMatch  nilMatch = CompMatch ();
    int        count; 

    if (screen->otherGrabExist ("stackswitch", 0))
	return false;
	   
    mCurrentMatch = optionGetWindowMatch ();

    match = CompOption::getMatchOptionNamed (options, "match", nilMatch);
    if (!match.isEmpty ())
    {
        mCurrentMatch = match;
    }

    count = countWindows ();

    if (count < 1)
	return false;

    if (!mGrabIndex)
    {
	mGrabIndex = screen->pushGrab (screen->invisibleCursor (), "stackswitch");
    }

    if (mGrabIndex)
    {
	mState = StateOut;

	if (!createWindowList ())
	    return false;

    	mSelectedWindow = mWindows.front ()->id ();
	renderWindowTitle ();

	foreach (CompWindow *w, screen->windows ())
	{
	    STACKSWITCH_WINDOW (w);

	    sw->mTx = w->x () - w->input ().left;
	    sw->mTy = w->y () + w->height () + w->input ().bottom;
	}
    	mMoreAdjust = true;
	cScreen->damageScreen ();
    }

    return true;
}

bool
StackswitchScreen::doSwitch (CompAction         *action,
			     CompAction::State  state,
			     CompOption::Vector options,
			     bool		nextWindow,
			     StackswitchScreen::StackswitchType type)
{
    bool       ret = true;

    if ((mState == StateNone) || (mState == StateIn))
    {
        if (type == TypeGroup)
        {
	    CompWindow *w;
	    w = screen->findWindow (CompOption::getIntOptionNamed (options,
							           "window",
							           0));
	    if (w)
	    {
	        mType = TypeGroup;
	        mClientLeader =
		    (w->clientLeader ()) ? w->clientLeader () : w->id ();
	        ret = initiate (action, state, options);
	    }
        }
        else
        {
	    mType = type;
	    ret = initiate (action, state, options);
        }

        if (state & CompAction::StateInitKey)
	    action->setState (action->state () | CompAction::StateTermKey);

        if (state & CompAction::StateInitEdge)
	    action->setState (action->state () | CompAction::StateTermEdge);
        else if (state & CompAction::StateInitButton)
	    action->setState (action->state () | CompAction::StateTermButton);
    }

    if (ret)
        switchToWindow (nextWindow);
    

    return ret;
}

void
StackswitchScreen::windowRemove (Window id)
{
    CompWindow *w;

    w = screen->findWindow (id);
    if (w)
    {
	bool   inList = false;
	std::vector <CompWindow *>::iterator it;
	Window selected;
	
	STACKSWITCH_WINDOW (w);

	if (mState == StateNone)
	    return;

	if (sw->isStackswitchable ())
    	    return;

	selected = mSelectedWindow;

	while (it != mWindows.begin ())
	{
	    //CompWindow *w = *it;

    	    if (w && id == (*it)->id ())
	    {
		inList = true;

		if (w->id () == selected)
		{
		    if (it < mWindows.end ()--)
		    {
		        it++;
			selected = (*(it))->id ();
			it--;
		    }
    		    else
			selected = mWindows.front ()->id ();

		    mSelectedWindow = selected;
		}

		mWindows.erase (it); // ???
		break;

	    }
	    it--;
	}

	if (!inList)
	    return;

	if (mWindows.size ())
	{
	    CompOption o ("root", CompOption::TypeInt);
	    CompOption::Vector opts;

	    o.value ().set ((int) screen->root ());

	    terminate (NULL, 0, opts);
	    return;
	}

	if (!mGrabIndex)
	    return;

	if (updateWindowList ())
	{
	    mMoreAdjust = true;
	    mState = StateOut;
	    cScreen->damageScreen ();
	}
    }
}

void
StackswitchScreen::handleEvent (XEvent *event)
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
    		if (mGrabIndex && (w->id () == mSelectedWindow))
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
    }
}

bool
StackswitchWindow::damageRect (bool	initial,
			       const CompRect &rect)
{
    bool       status = false;

    STACKSWITCH_SCREEN (screen);

    if (initial)
    {
	if (ss->mGrabIndex && isStackswitchable ())
	{
	    addToList ();
	    if (ss->updateWindowList ())
	    {
		mAdjust = true;
		ss->mMoreAdjust = true;
		ss->mState = StackswitchScreen::StateOut;
		ss->cScreen->damageScreen ();
	    }
	}
    }
    else if (ss->mState == StackswitchScreen::StateSwitching)
    {
	if (mSlot)
	{
	    ss->cScreen->damageScreen ();
	    status = true;
	}
    }
    
    status |= cWindow->damageRect (initial, rect);

    return status;
}

StackswitchScreen::StackswitchScreen (CompScreen *screen) :
    PluginClassHandler <StackswitchScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    mGrabIndex (0),
    mState (StateNone),
    mMoreAdjust (false),
    mRotateAdjust (false),
    mPaintingSwitcher (false),
    mRVelocity (0.0f),
    mRotation (0.0f),
    mClientLeader (None),
    mSelectedWindow (None)
{
    ScreenInterface::setHandler (screen);
    CompositeScreenInterface::setHandler (cScreen);
    GLScreenInterface::setHandler (gScreen);
    
    optionSetNextKeyInitiate (boost::bind (&StackswitchScreen::doSwitch, this,
    					   _1, _2, _3, true, TypeNormal));
    optionSetNextAllKeyInitiate (boost::bind (&StackswitchScreen::doSwitch, this,
    					   _1, _2, _3, true, TypeAll));
    optionSetNextGroupKeyInitiate (boost::bind (&StackswitchScreen::doSwitch, this,
    					   _1, _2, _3, true, TypeGroup));
    optionSetNextKeyTerminate (boost::bind (&StackswitchScreen::terminate, this,
    					    _1, _2, _3));
    optionSetNextAllKeyTerminate (boost::bind (&StackswitchScreen::terminate, this,
    					    _1, _2, _3));
    optionSetNextGroupKeyTerminate (boost::bind (&StackswitchScreen::terminate, this,
    					    _1, _2, _3));
    optionSetPrevKeyInitiate (boost::bind (&StackswitchScreen::doSwitch, this,
    					   _1, _2, _3, false, TypeNormal));
    optionSetPrevAllKeyInitiate (boost::bind (&StackswitchScreen::doSwitch, this,
    					   _1, _2, _3, false, TypeAll));
    optionSetPrevGroupKeyInitiate (boost::bind (&StackswitchScreen::doSwitch, this,
    					   _1, _2, _3, false, TypeGroup));
    optionSetPrevKeyTerminate (boost::bind (&StackswitchScreen::terminate, this,
    					    _1, _2, _3));
    optionSetPrevAllKeyTerminate (boost::bind (&StackswitchScreen::terminate, this,
    					    _1, _2, _3));
    optionSetPrevGroupKeyTerminate (boost::bind (&StackswitchScreen::terminate, this,
    					    _1, _2, _3));
    					    
    optionSetNextButtonInitiate (boost::bind (&StackswitchScreen::doSwitch, this,
    					   _1, _2, _3, true, TypeNormal));
    optionSetNextAllButtonInitiate (boost::bind (&StackswitchScreen::doSwitch, this,
    					   _1, _2, _3, true, TypeAll));
    optionSetNextGroupButtonInitiate (boost::bind (&StackswitchScreen::doSwitch, this,
    					   _1, _2, _3, true, TypeGroup));
    optionSetNextButtonTerminate (boost::bind (&StackswitchScreen::terminate, this,
    					    _1, _2, _3));
    optionSetNextAllButtonTerminate (boost::bind (&StackswitchScreen::terminate, this,
    					    _1, _2, _3));
    optionSetNextGroupButtonTerminate (boost::bind (&StackswitchScreen::terminate, this,
    					    _1, _2, _3));
    optionSetPrevButtonInitiate (boost::bind (&StackswitchScreen::doSwitch, this,
    					   _1, _2, _3, false, TypeNormal));
    optionSetPrevAllButtonInitiate (boost::bind (&StackswitchScreen::doSwitch, this,
    					   _1, _2, _3, false, TypeAll));
    optionSetPrevGroupButtonInitiate (boost::bind (&StackswitchScreen::doSwitch, this,
    					   _1, _2, _3, false, TypeGroup));
    optionSetPrevButtonTerminate (boost::bind (&StackswitchScreen::terminate, this,
    					    _1, _2, _3));
    optionSetPrevAllButtonTerminate (boost::bind (&StackswitchScreen::terminate, this,
    					    _1, _2, _3));
    optionSetPrevGroupButtonTerminate (boost::bind (&StackswitchScreen::terminate, this,
    					    _1, _2, _3));
}
    
StackswitchScreen::~StackswitchScreen ()
{
    mWindows.clear ();
    mDrawSlots.clear ();
}

StackswitchWindow::StackswitchWindow (CompWindow *window) :
    PluginClassHandler <StackswitchWindow, CompWindow> (window),
    window (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window)),
    mSlot (NULL),
    mXVelocity (0.0f),
    mYVelocity (0.0f),
    mScaleVelocity (0.0f),
    mRotVelocity (0.0f),
    mTx (0.0f),
    mTy (0.0f),
    mScale (1.0f),
    mRotation (0.0f),
    mAdjust (false)
{
    CompositeWindowInterface::setHandler (cWindow);
    GLWindowInterface::setHandler (gWindow);
}

StackswitchWindow::~StackswitchWindow ()
{
    if (mSlot)
	delete mSlot;
}

bool
StackswitchPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
    	return false;

    if (!CompPlugin::checkPluginABI ("text", COMPIZ_TEXT_ABI))
    {
	compLogMessage ("stackswitch", CompLogLevelWarn, "No compatible text plugin"\
						          " loaded");
	textAvailable = false;
    }
    else
	textAvailable = true;

    return true;
}
