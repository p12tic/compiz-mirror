/**
 *
 * Compiz group plugin
 *
 * paint.c
 *
 * Copyright : (C) 2006-2007 by Patrick Niklaus, Roi Cohen, Danny Baumann
 * Authors: Patrick Niklaus <patrick.niklaus@googlemail.com>
 *          Roi Cohen       <roico.beryl@gmail.com>
 *          Danny Baumann   <maniac@opencompositing.org>
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
 * groupPaintThumb - taken from switcher and modified for tab bar
 *
 */
void
GroupSelection::paintThumb (GroupTabBarSlot      *slot,
			    const GLMatrix	   &transform,
			    int		   targetOpacity,
			    bool		   fade)
{
    CompWindow            *w = slot->mWindow;
    unsigned int	  oldGlAddGeometryIndex;
    unsigned int	  oldGlDrawIndex;
    GLWindowPaintAttrib   wAttrib (GLWindow::get (w)->paintAttrib ());
    int                   tw, th;

    GROUP_WINDOW (w);
    GROUP_SCREEN (screen);

    tw = slot->mRegion.boundingRect ().width ();
    th = slot->mRegion.boundingRect ().height ();

    /* Wrap drawWindowGeometry to make sure the general
       drawWindowGeometry function is used */
    oldGlAddGeometryIndex = gw->gWindow->glAddGeometryGetCurrentIndex ();
    gw->gWindow->glAddGeometrySetCurrentIndex (MAXSHORT);

    /* animate fade */
    if (fade && mTabBar->mState == PaintFadeIn)
    {
	wAttrib.opacity -= wAttrib.opacity * mTabBar->mAnimationTime /
	                   (gs->optionGetFadeTime () * 1000);
    }
    else if (fade && mTabBar->mState == PaintFadeOut)
    {
	wAttrib.opacity = wAttrib.opacity * mTabBar->mAnimationTime /
	                  (gs->optionGetFadeTime () * 1000);
    }

    wAttrib.opacity = wAttrib.opacity * targetOpacity / OPAQUE;

    if (w->mapNum ())
    {
	GLFragment::Attrib fragment (wAttrib);
	GLMatrix	   wTransform (transform);
	int           	   width, height;
	int         	   vx, vy;

	width = w->width () + w->output ().left + w->output ().right;
	height = w->height () + w->output ().top + w->output ().bottom;

	if (width > tw)
	    wAttrib.xScale = (float) tw / width;
	else
	    wAttrib.xScale = 1.0f;
	if (height > th)
	    wAttrib.yScale = (float) tw / height;
	else
	    wAttrib.yScale = 1.0f;

	if (wAttrib.xScale < wAttrib.yScale)
	    wAttrib.yScale = wAttrib.xScale;
	else
	    wAttrib.xScale = wAttrib.yScale;

	/* FIXME: do some more work on the highlight on hover feature
	// Highlight on hover
	if (group && group->mTabBar && group->mTabBar->hoveredSlot == slot) {
	wAttrib.saturation = 0;
	wAttrib.brightness /= 1.25f;
	}*/

	gs->groupGetDrawOffsetForSlot (slot, vx, vy);

	wAttrib.xTranslate = (slot->mRegion.boundingRect ().x1 () +
			      slot->mRegion.boundingRect ().x2 ()) / 2 + vx;
	wAttrib.yTranslate = slot->mRegion.boundingRect ().y1 () + vy;

	wTransform.translate (wAttrib.xTranslate, wAttrib.yTranslate, 0.0f);
	wTransform.scale (wAttrib.xScale, wAttrib.yScale, 1.0f);
	wTransform.translate (-(WIN_X (w) + WIN_WIDTH (w) / 2),
			 -(WIN_Y (w) - w->output ().top), 0.0f);

	glPushMatrix ();
	glLoadMatrixf (wTransform.getMatrix ());

	oldGlDrawIndex = gw->gWindow->glDrawGetCurrentIndex ();
	gw->gWindow->glDraw (wTransform, fragment, infiniteRegion,
			  PAINT_WINDOW_TRANSFORMED_MASK |
			  PAINT_WINDOW_TRANSLUCENT_MASK);
	gw->gWindow->glDrawSetCurrentIndex (oldGlDrawIndex);

	glPopMatrix ();
    }

    gw->gWindow->glAddGeometrySetCurrentIndex (oldGlAddGeometryIndex);
}

/*
 * groupPaintTabBar
 *
 */
void
GroupSelection::paintTabBar (const GLWindowPaintAttrib   &attrib,
			     const GLMatrix		 &transform,
			     unsigned int		 mask,
			     CompRegion		 	 clipRegion)
{
    CompWindow      *topTab;
    GroupTabBar     *bar = mTabBar;
    int             count;
    CompRect        box;
    
    GROUP_SCREEN (screen);

    if (HAS_TOP_WIN (this))
	topTab = TOP_TAB (this);
    else
	topTab = PREV_TOP_TAB (this);

#define PAINT_BG     0
#define PAINT_SEL    1
#define PAINT_THUMBS 2
#define PAINT_TEXT   3
#define PAINT_MAX    4

    for (count = 0; count < PAINT_MAX; count++)
    {
	int             alpha = OPAQUE;
	float           wScale = 1.0f, hScale = 1.0f;
	GroupCairoLayer *layer = NULL;

	if (bar->mState == PaintFadeIn)
	    alpha -= alpha * bar->mAnimationTime / (gs->optionGetFadeTime () * 1000);
	else if (bar->mState == PaintFadeOut)
	    alpha = alpha * bar->mAnimationTime / (gs->optionGetFadeTime () * 1000);

	switch (count) {
	case PAINT_BG:
	    {
		int newWidth;

		layer = bar->mBgLayer;

		/* handle the repaint of the background */
		newWidth = bar->mRegion.boundingRect ().x2 () - bar->mRegion.boundingRect ().x1 ();
		if (layer && (newWidth > layer->mTexWidth))
		    newWidth = layer->mTexWidth;

		wScale = (double) (bar->mRegion.boundingRect ().x2 () -
				   bar->mRegion.boundingRect ().x1 ()) / (double) newWidth;

		/* FIXME: maybe move this over to groupResizeTabBarRegion -
		   the only problem is that we would have 2 redraws if
		   there is an animation */
		if (newWidth != bar->mOldWidth || bar->mBgAnimation)
		    bar->renderTabBarBackground ();

		bar->mOldWidth = newWidth;
		box	       = bar->mRegion.boundingRect ();
	    }
	    break;

	case PAINT_SEL:
	    if (mTopTab != gs->mDraggedSlot)
	    {
		layer = bar->mSelectionLayer;
		box   = mTopTab->mRegion.boundingRect ();
	    }
	    break;

	case PAINT_THUMBS:
	    {
		GLenum          oldTextureFilter;
		GroupTabBarSlot *slot;

		oldTextureFilter = gs->gScreen->textureFilter ();

		if (gs->optionGetMipmaps ())
		    gs->gScreen->setTextureFilter (GL_LINEAR_MIPMAP_LINEAR);

		foreach (slot, bar->mSlots)
		{
		    if (slot != gs->mDraggedSlot || !gs->mDragged)
			paintThumb (slot, transform,
				    attrib.opacity, true);
		}

		gs->gScreen->setTextureFilter (oldTextureFilter);
	    }
	    break;

	case PAINT_TEXT:
	    if (bar->mTextLayer && (bar->mTextLayer->mState != PaintOff))
	    {
		layer = bar->mTextLayer;

		int x1 = bar->mRegion.boundingRect ().x1 () + 5;
		int x2 = bar->mRegion.boundingRect ().x1 () +
			 bar->mTextLayer->mTexWidth + 5;
		int y1 = bar->mRegion.boundingRect ().y2 () -
			 bar->mTextLayer->mTexHeight - 5;
		int y2 = bar->mRegion.boundingRect ().y2 () - 5;

		if (x2 > bar->mRegion.boundingRect ().x2 ())
		    x2 = bar->mRegion.boundingRect ().x2 ();

		/* recalculate the alpha again for text fade... */
		if (layer->mState == PaintFadeIn)
		    alpha -= alpha * layer->mAnimationTime /
			     (gs->optionGetFadeTextTime () * 1000);
		else if (layer->mState == PaintFadeOut)
		    alpha = alpha * layer->mAnimationTime /
			    (gs->optionGetFadeTextTime () * 1000);
		
		box = CompRect (x1, y1, x2 - x1, y2 - y1);
	    }
	    break;
	}

	if (layer)
	{
	    GroupWindow *gwTopTab = GroupWindow::get (topTab);

	    foreach (GLTexture *tex, layer->mTexture)
	    {
		GLTexture::Matrix matrix = tex->matrix ();
		GLTexture::MatrixList matl;
		CompRegion reg;
		
		int x1 = box.x1 ();
		int y1 = box.y1 ();
		int x2 = box.x2 ();
		int y2 = box.y2 ();

		/* remove the old x1 and y1 so we have a relative value */
		x2 -= x1;
		y2 -= y1;
		x1 = (x1 - topTab->x ()) / wScale +
			     topTab->x ();
		y1 = (y1 - topTab->y ()) / hScale +
			     topTab->y ();

		/* now add the new x1 and y1 so we have a absolute value again,
		also we don't want to stretch the texture... */
		if (x2 * wScale < layer->mTexWidth)
		    x2 += x1;
		else
		    x2 = x1 + layer->mTexWidth;

		if (y2 * hScale < layer->mTexHeight)
		    y2 += y1;
		else
		    y2 = y1 + layer->mTexHeight;

		matrix.x0 -= x1 * matrix.xx;
		matrix.y0 -= y1 * matrix.yy;

		matl.push_back (matrix);

		reg = CompRegion (x1, y1,
				  x2 - x1,
				  y2 - y1);
		
		gwTopTab->gWindow->geometry ().reset ();

		gwTopTab->gWindow->glAddGeometry (matl, reg, clipRegion);

		if (gwTopTab->gWindow->geometry ().vertices)
		{
		    GLWindowPaintAttrib wAttrib (attrib);
		    GLFragment::Attrib fragment (wAttrib);
		    GLMatrix	       wTransform (transform);

		    wTransform.translate (WIN_X (topTab), WIN_Y (topTab), 0.0f);
		    wTransform.scale (wScale, hScale, 1.0f);
		    wTransform.translate (
				 wAttrib.xTranslate / wScale - WIN_X (topTab),
				 wAttrib.yTranslate / hScale - WIN_Y (topTab),
				 0.0f);

		    glPushMatrix ();
		    glLoadMatrixf (wTransform.getMatrix ());

		    alpha = alpha * ((float) wAttrib.opacity / OPAQUE);

		    fragment.setOpacity (alpha);

		    gwTopTab->glDrawTexture (tex, fragment, mask |
						PAINT_WINDOW_BLEND_MASK |
						PAINT_WINDOW_TRANSFORMED_MASK |
						PAINT_WINDOW_TRANSLUCENT_MASK);

		    glPopMatrix ();
		}
	    }
	}
    }
}

/*
 * Selection::paint
 *
 */
void
Selection::paint (const GLScreenPaintAttrib sa,
		  const GLMatrix	    transform,
		  CompOutput                *output,
		  bool                      transformed)
{
    GROUP_SCREEN (screen);
	
    int x1, x2, y1, y2;

    x1 = MIN (mX1, mX2);
    y1 = MIN (mY1, mY2);
    x2 = MAX (mX1, mX2);
    y2 = MAX (mY1, mY2);

    if (gs->mGrabState == ScreenGrabSelect)
    {
	GLMatrix sTransform (transform);

	if (transformed)
	{
	    gs->gScreen->glApplyTransform (sa, output, &sTransform);
	    sTransform.toScreenSpace (output, -sa.zTranslate);
	}
	else
	    sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

	glPushMatrix ();
	glLoadMatrixf (sTransform.getMatrix ());

	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	glEnable (GL_BLEND);

	glColor4usv (gs->optionGetFillColor ());
	glRecti (x1, y2, x2, y1);

	glColor4usv (gs->optionGetLineColor ());
	glBegin (GL_LINE_LOOP);
	glVertex2i (x1, y1);
	glVertex2i (x2, y1);
	glVertex2i (x2, y2);
	glVertex2i (x1, y2);
	glEnd ();

	glColor4usv (defaultColor);
	glDisable (GL_BLEND);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);
	glPopMatrix ();
    }
}

/*
 * groupPreparePaintScreen
 *
 */
void
GroupScreen::preparePaint (int msSinceLastPaint)
{
    GroupSelection *group;
    GroupSelection::List::iterator it = mGroups.begin ();

    cScreen->preparePaint (msSinceLastPaint);

    while (it != mGroups.end ())
    {
	group = *it;
	GroupTabBar *bar = group->mTabBar;

	if (bar)
	{
	    group->applyForces ((mDragged) ? mDraggedSlot : NULL);
	    group->applySpeeds (msSinceLastPaint);

	    if ((bar->mState != PaintOff) && HAS_TOP_WIN (group))
		group->handleHoverDetection ();

	    if (bar->mState == PaintFadeIn || bar->mState == PaintFadeOut)
		group->handleTabBarFade (msSinceLastPaint);

	    if (bar->mTextLayer)
		group->handleTextFade (msSinceLastPaint);

	    if (bar->mBgAnimation)
		group->handleTabBarAnimation (msSinceLastPaint);
	}

	if (group->mChangeState != NoTabChange)
	{
	    group->mChangeAnimationTime -= msSinceLastPaint;
	    if (group->mChangeAnimationTime <= 0)
		group->handleAnimation ();
	}

	/* groupDrawTabAnimation may delete the group, so better
	   save the pointer to the next chain element */

	it++;

	if (group->mTabbingState != NoTabbing)
	    group->drawTabAnimation (msSinceLastPaint);
    }
}

/*
 * groupPaintOutput
 *
 */
bool
GroupScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
			    const GLMatrix	      &transform,
			    const CompRegion	      &region,
			    CompOutput		      *output,
			    unsigned int	      mask)
{
    GroupSelection *group;
    bool           status;

    mTmpSel.mPainted = false;
    mTmpSel.mVpX = screen->vp ().x ();
    mTmpSel.mVpY = screen->vp ().y ();

    foreach (group, mGroups)
    {
	if (group->mResizeInfo)
	    mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
    }
    foreach (group, mGroups)
    {
	if (group->mChangeState != NoTabChange ||
	    group->mTabbingState != NoTabbing)
	{
	    mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
	}
	else if (group->mTabBar && (group->mTabBar->mState != PaintOff))
	{
	    mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
	}
    }

    status = gScreen->glPaintOutput (attrib, transform, region, output, mask);

    if (status && !mTmpSel.mPainted)
    {
	if ((mGrabState == ScreenGrabTabDrag) && mDraggedSlot)
	{
	    GLMatrix wTransform (transform);
	    PaintState    state;

	    GROUP_WINDOW (mDraggedSlot->mWindow);

	    wTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

	    glPushMatrix ();
	    glLoadMatrixf (wTransform.getMatrix ());

	    /* prevent tab bar drawing.. */
	    state = gw->mGroup->mTabBar->mState;
	    gw->mGroup->mTabBar->mState = PaintOff;
	    gw->mGroup->paintThumb (mDraggedSlot, wTransform, OPAQUE, true);
	    gw->mGroup->mTabBar->mState = state;

	    glPopMatrix ();
	}
	else  if (mGrabState == ScreenGrabSelect)
	{
	    mTmpSel.paint (attrib, transform, output, false);
	}
    }

    return status;
}

/*
 * groupPaintTransformedOutput
 *
 */
void
GroupScreen::glPaintTransformedOutput (const GLScreenPaintAttrib &attrib,
				       const GLMatrix		&transform,
				       const CompRegion	        &region,
				       CompOutput		*output,
				       unsigned int		mask)
{
    gScreen->glPaintTransformedOutput (attrib, transform, region, output, mask);

    if ((mTmpSel.mVpX == screen->vp ().x ()) && (mTmpSel.mVpY == screen->vp ().y ()))
    {
	mTmpSel.mPainted = true;

	if ((mGrabState == ScreenGrabTabDrag) &&
	    mDraggedSlot && mDragged)
	{
	    GLMatrix wTransform (transform);

	    gScreen->glApplyTransform (attrib, output, &wTransform);
	    wTransform.toScreenSpace (output, -attrib.zTranslate);
	    glPushMatrix ();
	    glLoadMatrixf (wTransform.getMatrix ());

	    mGroups.front ()->paintThumb (mDraggedSlot,
					  wTransform, OPAQUE, false);

	    glPopMatrix ();
	}
	else if (mGrabState == ScreenGrabSelect)
	{
	    mTmpSel.paint (attrib, transform, output, true);
	}
    }
}

/*
 * groupDonePaintScreen
 *
 */
void
GroupScreen::donePaint ()
{
    GroupSelection *group;

    cScreen->donePaint ();

    foreach (group, mGroups)
    {
	if (group->mTabbingState != NoTabbing)
	    cScreen->damageScreen ();
	else if (group->mChangeState != NoTabChange)
	    cScreen->damageScreen ();
	else if (group->mTabBar)
	{
	    bool needDamage = false;

	    if ((group->mTabBar->mState == PaintFadeIn) ||
		(group->mTabBar->mState == PaintFadeOut))
	    {
		needDamage = true;
	    }

	    if (group->mTabBar->mTextLayer)
	    {
		if ((group->mTabBar->mTextLayer->mState == PaintFadeIn) ||
		    (group->mTabBar->mTextLayer->mState == PaintFadeOut))
		{
		    needDamage = true;
		}
	    }

	    if (group->mTabBar->mBgAnimation)
		needDamage = true;

	    if (mDraggedSlot)
		needDamage = true;

	    if (needDamage)
		group->damageTabBarRegion ();
	}
    }
}

void
GroupWindow::groupComputeGlowQuads (GLTexture::Matrix *matrix)
{
    BoxRec            *box;
    GLTexture::Matrix *quadMatrix;
    int               glowSize, glowOffset;
    int		      glowType;
    CompWindow	      *w = window;

    GROUP_SCREEN (screen);

    if (gs->optionGetGlow () && matrix)
    {
	if (!mGlowQuads)
	    mGlowQuads = (GlowQuad *) malloc (NUM_GLOWQUADS * sizeof (GlowQuad));
	if (!mGlowQuads)
	    return;
    }
    else
    {
	if (mGlowQuads)
	{
	    free (mGlowQuads);
	    mGlowQuads = NULL;
	}
	return;
    }

    glowSize = gs->optionGetGlowSize ();
    glowType = gs->optionGetGlowType ();
    glowOffset = (glowSize * gs->mGlowTextureProperties[glowType].glowOffset /
		  gs->mGlowTextureProperties[glowType].textureSize) + 1;

    /* Top left corner */
    box = &mGlowQuads[GLOWQUAD_TOPLEFT].mBox;
    mGlowQuads[GLOWQUAD_TOPLEFT].mMatrix = *matrix;
    quadMatrix = &mGlowQuads[GLOWQUAD_TOPLEFT].mMatrix;

    box->x1 = WIN_REAL_X (w) - glowSize + glowOffset;
    box->y1 = WIN_REAL_Y (w) - glowSize + glowOffset;
    box->x2 = WIN_REAL_X (w) + glowOffset;
    box->y2 = WIN_REAL_Y (w) + glowOffset;

    quadMatrix->xx = 1.0f / glowSize;
    quadMatrix->yy = 1.0f / (glowSize);
    quadMatrix->x0 = -(box->x1 * quadMatrix->xx);
    quadMatrix->y0 = -(box->y1 * quadMatrix->yy);

    box->x2 = MIN (WIN_REAL_X (w) + glowOffset,
		   WIN_REAL_X (w) + (WIN_REAL_WIDTH (w) / 2));
    box->y2 = MIN (WIN_REAL_Y (w) + glowOffset,
		   WIN_REAL_Y (w) + (WIN_REAL_HEIGHT (w) / 2));

    /* Top right corner */
    box = &mGlowQuads[GLOWQUAD_TOPRIGHT].mBox;
    mGlowQuads[GLOWQUAD_TOPRIGHT].mMatrix = *matrix;
    quadMatrix = &mGlowQuads[GLOWQUAD_TOPRIGHT].mMatrix;

    box->x1 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset;
    box->y1 = WIN_REAL_Y (w) - glowSize + glowOffset;
    box->x2 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) + glowSize - glowOffset;
    box->y2 = WIN_REAL_Y (w) + glowOffset;

    quadMatrix->xx = -1.0f / glowSize;
    quadMatrix->yy = 1.0f / glowSize;
    quadMatrix->x0 = 1.0 - (box->x1 * quadMatrix->xx);
    quadMatrix->y0 = -(box->y1 * quadMatrix->yy);

    box->x1 = MAX (WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset,
		   WIN_REAL_X (w) + (WIN_REAL_WIDTH (w) / 2));
    box->y2 = MIN (WIN_REAL_Y (w) + glowOffset,
		   WIN_REAL_Y (w) + (WIN_REAL_HEIGHT (w) / 2));

    /* Bottom left corner */
    box = &mGlowQuads[GLOWQUAD_BOTTOMLEFT].mBox;
    mGlowQuads[GLOWQUAD_BOTTOMLEFT].mMatrix = *matrix;
    quadMatrix = &mGlowQuads[GLOWQUAD_BOTTOMLEFT].mMatrix;

    box->x1 = WIN_REAL_X (w) - glowSize + glowOffset;
    box->y1 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset;
    box->x2 = WIN_REAL_X (w) + glowOffset;
    box->y2 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) + glowSize - glowOffset;

    quadMatrix->xx = 1.0f / glowSize;
    quadMatrix->yy = -1.0f / glowSize;
    quadMatrix->x0 = -(box->x1 * quadMatrix->xx);
    quadMatrix->y0 = 1.0f - (box->y1 * quadMatrix->yy);

    box->y1 = MAX (WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset,
		   WIN_REAL_Y (w) + (WIN_REAL_HEIGHT (w) / 2));
    box->x2 = MIN (WIN_REAL_X (w) + glowOffset,
		   WIN_REAL_X (w) + (WIN_REAL_WIDTH (w) / 2));

    /* Bottom right corner */
    box = &mGlowQuads[GLOWQUAD_BOTTOMRIGHT].mBox;
    mGlowQuads[GLOWQUAD_BOTTOMRIGHT].mMatrix = *matrix;
    quadMatrix = &mGlowQuads[GLOWQUAD_BOTTOMRIGHT].mMatrix;

    box->x1 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset;
    box->y1 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset;
    box->x2 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) + glowSize - glowOffset;
    box->y2 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) + glowSize - glowOffset;

    quadMatrix->xx = -1.0f / glowSize;
    quadMatrix->yy = -1.0f / glowSize;
    quadMatrix->x0 = 1.0 - (box->x1 * quadMatrix->xx);
    quadMatrix->y0 = 1.0 - (box->y1 * quadMatrix->yy);

    box->x1 = MAX (WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset,
		   WIN_REAL_X (w) + (WIN_REAL_WIDTH (w) / 2));
    box->y1 = MAX (WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset,
		   WIN_REAL_Y (w) + (WIN_REAL_HEIGHT (w) / 2));

    /* Top edge */
    box = &mGlowQuads[GLOWQUAD_TOP].mBox;
    mGlowQuads[GLOWQUAD_TOP].mMatrix = *matrix;
    quadMatrix = &mGlowQuads[GLOWQUAD_TOP].mMatrix;

    box->x1 = WIN_REAL_X (w) + glowOffset;
    box->y1 = WIN_REAL_Y (w) - glowSize + glowOffset;
    box->x2 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset;
    box->y2 = WIN_REAL_Y (w) + glowOffset;

    quadMatrix->xx = 0.0f;
    quadMatrix->yy = 1.0f / glowSize;
    quadMatrix->x0 = 1.0;
    quadMatrix->y0 = -(box->y1 * quadMatrix->yy);

    /* Bottom edge */
    box = &mGlowQuads[GLOWQUAD_BOTTOM].mBox;
    mGlowQuads[GLOWQUAD_BOTTOM].mMatrix = *matrix;
    quadMatrix = &mGlowQuads[GLOWQUAD_BOTTOM].mMatrix;

    box->x1 = WIN_REAL_X (w) + glowOffset;
    box->y1 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset;
    box->x2 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset;
    box->y2 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) + glowSize - glowOffset;

    quadMatrix->xx = 0.0f;
    quadMatrix->yy = -1.0f / glowSize;
    quadMatrix->x0 = 1.0;
    quadMatrix->y0 = 1.0 - (box->y1 * quadMatrix->yy);

    /* Left edge */
    box = &mGlowQuads[GLOWQUAD_LEFT].mBox;
    mGlowQuads[GLOWQUAD_LEFT].mMatrix = *matrix;
    quadMatrix = &mGlowQuads[GLOWQUAD_LEFT].mMatrix;

    box->x1 = WIN_REAL_X (w) - glowSize + glowOffset;
    box->y1 = WIN_REAL_Y (w) + glowOffset;
    box->x2 = WIN_REAL_X (w) + glowOffset;
    box->y2 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset;

    quadMatrix->xx = 1.0f / glowSize;
    quadMatrix->yy = 0.0f;
    quadMatrix->x0 = -(box->x1 * quadMatrix->xx);
    quadMatrix->y0 = 1.0;

    /* Right edge */
    box = &mGlowQuads[GLOWQUAD_RIGHT].mBox;
    mGlowQuads[GLOWQUAD_RIGHT].mMatrix = *matrix;
    quadMatrix = &mGlowQuads[GLOWQUAD_RIGHT].mMatrix;

    box->x1 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset;
    box->y1 = WIN_REAL_Y (w) + glowOffset;
    box->x2 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) + glowSize - glowOffset;
    box->y2 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset;

    quadMatrix->xx = -1.0f / glowSize;
    quadMatrix->yy = 0.0f;
    quadMatrix->x0 = 1.0 - (box->x1 * quadMatrix->xx);
    quadMatrix->y0 = 1.0;
}

/*
 * groupDrawWindow
 *
 */
bool
GroupWindow::glDraw (const GLMatrix           &transform,
		     GLFragment::Attrib       &attrib,
		     const CompRegion	      &region,
		     unsigned int	      mask)
{
    bool       status;
    CompRegion paintRegion (region);

    GROUP_SCREEN (screen);

    if (mGroup && (mGroup->mWindows.size () > 1) && mGlowQuads)
    {
	if (mask & PAINT_WINDOW_TRANSFORMED_MASK)
	    paintRegion = CompRegion (infiniteRegion);

	if (paintRegion.numRects ())
	{
	    REGION box;
	    CompRegion reg;
	    int    i;

	    box.rects = &box.extents;
	    box.numRects = 1;

	    gWindow->geometry ().reset ();

	    for (i = 0; i < NUM_GLOWQUADS; i++)
	    {
		box.extents = mGlowQuads[i].mBox;

		if (box.extents.x1 < box.extents.x2 &&
		    box.extents.y1 < box.extents.y2)
		{
		    GLTexture::MatrixList matl;
		    reg = CompRegion (box.extents.x1, box.extents.y1,
			  	      box.extents.x2 - box.extents.x1,
			  	      box.extents.y2 - box.extents.y1);

		    matl.push_back (mGlowQuads[i].mMatrix);
		    gWindow->glAddGeometry (matl, reg, paintRegion);
		}
	    }

	    if (gWindow->geometry ().vertices)
	    {
		GLFragment::Attrib fAttrib (attrib);
		GLushort       average;
		GLushort       color[3] = {mGroup->mColor[0],
		                           mGroup->mColor[1],
		                           mGroup->mColor[2]};

		/* Apply brightness to color. */
		color[0] *= (float)attrib.getBrightness () / BRIGHT;
		color[1] *= (float)attrib.getBrightness () / BRIGHT;
		color[2] *= (float)attrib.getBrightness () / BRIGHT;

		/* Apply saturation to color. */
		average = (color[0] + color[1] + color[2]) / 3;
		color[0] = average + (color[0] - average) *
		           attrib.getSaturation () / COLOR;
		color[1] = average + (color[1] - average) *
		           attrib.getSaturation () / COLOR;
		color[2] = average + (color[2] - average) *
		           attrib.getSaturation () / COLOR;

		fAttrib.setOpacity (OPAQUE);
		fAttrib.setSaturation (COLOR);
		fAttrib.setBrightness  (BRIGHT);

		gs->gScreen->setTexEnvMode (GL_MODULATE);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4us (color[0], color[1], color[2], attrib.getOpacity ());

		/* we use PAINT_WINDOW_TRANSFORMED_MASK here to force
		   the usage of a good texture filter */
		foreach (GLTexture *tex, gs->mGlowTexture)
		{
		    gWindow->glDrawTexture (tex, fAttrib, mask | 
						PAINT_WINDOW_BLEND_MASK       |
						PAINT_WINDOW_TRANSLUCENT_MASK |
						PAINT_WINDOW_TRANSFORMED_MASK);
		}

		glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		gs->gScreen->setTexEnvMode (GL_REPLACE);
		glColor4usv (defaultColor);
	    }
	}
    }

    status = gWindow->glDraw (transform, attrib, region, mask);

    return status;
}

void
GroupWindow::groupGetStretchRectangle (BoxPtr pBox,
				       float  &xScaleRet,
				       float  &yScaleRet)
{
    BoxRec box;
    int    width, height;
    float  xScale, yScale;

    box.x1 = mResizeGeometry.x () - window->input ().left;
    box.y1 = mResizeGeometry.y () - window->input ().top;
    box.x2 = mResizeGeometry.x () + mResizeGeometry.width () +
	     window->serverGeometry ().border () * 2 + window->input ().right;

    if (window->shaded ())
    {
	box.y2 = mResizeGeometry.y () + window->height () + window->input ().bottom;
    }
    else
    {
	box.y2 = mResizeGeometry.y () + mResizeGeometry.height () +
	         window->serverGeometry ().border () * 2 + window->input ().bottom;
    }

    width  = window->width ()  + window->input ().left + window->input ().right;
    height = window->height () + window->input ().top  + window->input ().bottom;

    xScale = (width)  ? (box.x2 - box.x1) / (float) width  : 1.0f;
    yScale = (height) ? (box.y2 - box.y1) / (float) height : 1.0f;

    pBox->x1 = box.x1 - (window->output ().left - window->input ().left) * xScale;
    pBox->y1 = box.y1 - (window->output ().top - window->input ().top) * yScale;
    pBox->x2 = box.x2 + window->output ().right * xScale;
    pBox->y2 = box.y2 + window->output ().bottom * yScale;

    xScaleRet = xScale;
    yScaleRet = yScale;
}

void
GroupScreen::groupDamagePaintRectangle (BoxPtr pBox)
{
    REGION reg;
    CompRegion cReg;

    reg.rects    = &reg.extents;
    reg.numRects = 1;

    reg.extents = *pBox;

    reg.extents.x1 -= 1;
    reg.extents.y1 -= 1;
    reg.extents.x2 += 1;
    reg.extents.y2 += 1;

    cReg = CompRegion (reg.extents.x1, reg.extents.y1,
		       reg.extents.x2 - reg.extents.x1,
		       reg.extents.y2 - reg.extents.y1);

    cScreen->damageRegion (cReg);
}

/*
 * groupPaintWindow
 *
 */
bool
GroupWindow::glPaint (const GLWindowPaintAttrib &attrib,
		      const GLMatrix		&transform,
		      const CompRegion		&region,
		      unsigned int		mask)
{
    bool       status;
    bool       doRotate, doTabbing, showTabbar;
    CompWindow *w = window;

    GROUP_SCREEN (screen);

    if (mGroup)
    {
	GroupSelection *group = mGroup;

	doRotate = (group->mChangeState != NoTabChange) &&
	           HAS_TOP_WIN (group) && HAS_PREV_TOP_WIN (group) &&
	           (IS_TOP_TAB (w, group) || IS_PREV_TOP_TAB (w, group));

	doTabbing = (mAnimateState & (IS_ANIMATED | FINISHED_ANIMATION)) &&
	            !(IS_TOP_TAB (w, group) &&
		      (group->mTabbingState == Tabbing));

	showTabbar = group->mTabBar && (group->mTabBar->mState != PaintOff) &&
	             (((IS_TOP_TAB (w, group)) &&
		       ((group->mChangeState == NoTabChange) ||
			(group->mChangeState == TabChangeNewIn))) ||
		      (IS_PREV_TOP_TAB (w, group) &&
		       (group->mChangeState == TabChangeOldOut)));
    }
    else
    {
	doRotate   = false;
	doTabbing  = false;
	showTabbar = false;
    }

    if (mWindowHideInfo)
	mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

    if (mInSelection || !mResizeGeometry.isEmpty () || doRotate ||
	doTabbing || showTabbar)
    {
	GLWindowPaintAttrib wAttrib (attrib);
	GLMatrix            wTransform (transform);
	float               animProgress = 0.0f;
	int                 drawnPosX = 0, drawnPosY = 0;

	if (mInSelection)
	{
	    wAttrib.opacity    = OPAQUE * gs->optionGetSelectOpacity () / 100;
	    wAttrib.saturation = COLOR * gs->optionGetSelectSaturation () / 100;
	    wAttrib.brightness = BRIGHT * gs->optionGetSelectBrightness () / 100;
	}

	if (doTabbing)
	{
	    /* fade the window out */
	    float progress;
	    int   distanceX, distanceY;
	    float origDistance, distance;

	    if (mAnimateState & FINISHED_ANIMATION)
	    {
		drawnPosX = mDestination.x;
		drawnPosY = mDestination.y;
	    }
	    else
	    {
		drawnPosX = mOrgPos.x + mTx;
		drawnPosY = mOrgPos.y + mTy;
	    }

	    distanceX = drawnPosX - mDestination.x;
	    distanceY = drawnPosY - mDestination.y;
	    distance = sqrt (pow (distanceX, 2) + pow (distanceY, 2));

	    distanceX = (mOrgPos.x - mDestination.x);
	    distanceY = (mOrgPos.y - mDestination.y);
	    origDistance = sqrt (pow (distanceX, 2) + pow (distanceY, 2));

	    if (!distanceX && !distanceY)
		progress = 1.0f;
	    else
		progress = 1.0f - (distance / origDistance);

	    animProgress = progress;

	    progress = MAX (progress, 0.0f);
	    if (mGroup->mTabbingState == Tabbing)
		progress = 1.0f - progress;

	    wAttrib.opacity = (float)wAttrib.opacity * progress;
	}

	if (doRotate)
	{
	    float timeLeft = mGroup->mChangeAnimationTime;
	    int   animTime = gs->optionGetChangeAnimationTime () * 500;

	    if (mGroup->mChangeState == TabChangeOldOut)
		timeLeft += animTime;

	    /* 0 at the beginning, 1 at the end */
	    animProgress = 1 - (timeLeft / (2 * animTime));
	}

	if (!mResizeGeometry.isEmpty ())
	{
	    int    xOrigin, yOrigin;
	    float  xScale, yScale;
	    BoxRec box;

	    groupGetStretchRectangle (&box, xScale, yScale);

	    xOrigin = window->x () - w->input ().left;
	    yOrigin = window->y () - w->input ().top;

	    wTransform.translate (xOrigin, yOrigin, 0.0f);
	    wTransform.scale (xScale, yScale, 1.0f);
	    wTransform.translate ((mResizeGeometry.x () - window->x ()) /
			     	   xScale - xOrigin,
			     	  (mResizeGeometry.y () - window->y ()) /
			     	   yScale - yOrigin,
			     	  0.0f);

	    mask |= PAINT_WINDOW_TRANSFORMED_MASK;
	}
	else if (doRotate || doTabbing)
	{
	    float      animWidth, animHeight;
	    float      animScaleX, animScaleY;
	    CompWindow *morphBase, *morphTarget;

	    if (doTabbing)
	    {
		if (mGroup->mTabbingState == Tabbing)
		{
		    morphBase   = w;
		    morphTarget = TOP_TAB (mGroup);
		}
		else
		{
		    morphTarget = w;
		    if (HAS_TOP_WIN (mGroup))
			morphBase = TOP_TAB (mGroup);
		    else
			morphBase = mGroup->mLastTopTab;
		}
	    }
	    else
	    {
		morphBase   = PREV_TOP_TAB (mGroup);
		morphTarget = TOP_TAB (mGroup);
	    }

	    animWidth = (1 - animProgress) * WIN_REAL_WIDTH (morphBase) +
		        animProgress * WIN_REAL_WIDTH (morphTarget);
	    animHeight = (1 - animProgress) * WIN_REAL_HEIGHT (morphBase) +
		         animProgress * WIN_REAL_HEIGHT (morphTarget);

	    animWidth = MAX (1.0f, animWidth);
	    animHeight = MAX (1.0f, animHeight);
	    animScaleX = animWidth / WIN_REAL_WIDTH (w);
	    animScaleY = animHeight / WIN_REAL_HEIGHT (w);

	    if (doRotate)
		wTransform.scale (1.0f, 1.0f, 1.0f / screen->width ());

	    wTransform.translate (WIN_REAL_X (w) + WIN_REAL_WIDTH (w) / 2.0f,
			          WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) / 2.0f,
			          0.0f);

	    if (doRotate)
	    {
		float rotateAngle = animProgress * 180.0f;
		if (IS_TOP_TAB (w, mGroup))
		    rotateAngle += 180.0f;

		if (mGroup->mChangeAnimationDirection < 0)
		    rotateAngle *= -1.0f;

		wTransform.rotate (rotateAngle, 0.0f, 1.0f, 0.0f);
	    }

	    if (doTabbing)
		wTransform.translate (drawnPosX - WIN_X (w),
				      drawnPosY - WIN_Y (w), 0.0f);

	    wTransform.scale (animScaleX, animScaleY, 1.0f);

	    wTransform.translate (-(WIN_REAL_X (w) + WIN_REAL_WIDTH (w) / 2.0f),
			          -(WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) / 2.0f),
			         0.0f);

	    mask |= PAINT_WINDOW_TRANSFORMED_MASK;
	}

	status = gWindow->glPaint (wAttrib, wTransform, region, mask);

	if (showTabbar)
	{
	    gWindow->glPaintSetEnabled (this, false);
	    mGroup->paintTabBar (wAttrib, wTransform, mask, region);
	    gWindow->glPaintSetEnabled (this, true);
	}
    }
    else
    {
	status = gWindow->glPaint (attrib, transform, region, mask);
    }

    return status;
}
