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
GroupScreen::groupPaintThumb (GroupSelection       *group,
			      GroupTabBarSlot      *slot,
			      const GLMatrix	   &transform,
			      int		   targetOpacity)
{
    CompWindow            *w = slot->mWindow;
    unsigned int	  oldGlAddGeometryIndex;
    GLWindowPaintAttrib   wAttrib (GLWindow::get (w)->paintAttrib ());
    int                   tw, th;

    GROUP_WINDOW (w);

    tw = slot->mRegion->extents.x2 - slot->mRegion->extents.x1;
    th = slot->mRegion->extents.y2 - slot->mRegion->extents.y1;

    /* Wrap drawWindowGeometry to make sure the general
       drawWindowGeometry function is used */
    oldGlAddGeometryIndex = gw->gWindow->glAddGeometryGetCurrentIndex ();
    gw->gWindow->glAddGeometrySetCurrentIndex (MAXSHORT);

    /* animate fade */
    if (group && group->mTabBar->mState == PaintFadeIn)
    {
	wAttrib.opacity -= wAttrib.opacity * group->mTabBar->mAnimationTime /
	                   (optionGetFadeTime () * 1000);
    }
    else if (group && group->mTabBar->mState == PaintFadeOut)
    {
	wAttrib.opacity = wAttrib.opacity * group->mTabBar->mAnimationTime /
	                  (optionGetFadeTime () * 1000);
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

	groupGetDrawOffsetForSlot (slot, vx, vy);

	wAttrib.xTranslate = (slot->mRegion->extents.x1 +
			      slot->mRegion->extents.x2) / 2 + vx;
	wAttrib.yTranslate = slot->mRegion->extents.y1 + vy;

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

    gw->gWindow->glAddGeometrySetCurrentIndex (oldGlAddGeometryIndex);
}

/*
 * groupPaintTabBar
 *
 */
void
GroupScreen::groupPaintTabBar (GroupSelection            *group,
			       const GLWindowPaintAttrib &attrib,
			       const GLMatrix		 &transform,
			       unsigned int		 mask,
			       Region			 clipRegion)
{
    CompWindow      *topTab;
    GroupTabBar     *bar = group->mTabBar;
    int             count;
    REGION          box;

    if (HAS_TOP_WIN (group))
	topTab = TOP_TAB (group);
    else
	topTab = PREV_TOP_TAB (group);

#define PAINT_BG     0
#define PAINT_SEL    1
#define PAINT_THUMBS 2
#define PAINT_TEXT   3
#define PAINT_MAX    4

    box.rects = &box.extents;
    box.numRects = 1;

    for (count = 0; count < PAINT_MAX; count++)
    {
	int             alpha = OPAQUE;
	float           wScale = 1.0f, hScale = 1.0f;
	GroupCairoLayer *layer = NULL;

	if (bar->mState == PaintFadeIn)
	    alpha -= alpha * bar->mAnimationTime / (optionGetFadeTime () * 1000);
	else if (bar->mState == PaintFadeOut)
	    alpha = alpha * bar->mAnimationTime / (optionGetFadeTime () * 1000);

	switch (count) {
	case PAINT_BG:
	    {
		int newWidth;

		layer = bar->mBgLayer;

		/* handle the repaint of the background */
		newWidth = bar->mRegion->extents.x2 - bar->mRegion->extents.x1;
		if (layer && (newWidth > layer->mTexWidth))
		    newWidth = layer->mTexWidth;

		wScale = (double) (bar->mRegion->extents.x2 -
				   bar->mRegion->extents.x1) / (double) newWidth;

		/* FIXME: maybe move this over to groupResizeTabBarRegion -
		   the only problem is that we would have 2 redraws if
		   there is an animation */
		if (newWidth != bar->mOldWidth || bar->mBgAnimation)
		    groupRenderTabBarBackground (group);

		bar->mOldWidth = newWidth;
		box.extents = bar->mRegion->extents;
	    }
	    break;

	case PAINT_SEL:
	    if (group->mTopTab != mDraggedSlot)
	    {
		layer = bar->mSelectionLayer;
		box.extents = group->mTopTab->mRegion->extents;
	    }
	    break;

	case PAINT_THUMBS:
	    {
		GLenum          oldTextureFilter;
		GroupTabBarSlot *slot;

		oldTextureFilter = gScreen->textureFilter ();

		if (optionGetMipmaps ())
		    gScreen->setTextureFilter (GL_LINEAR_MIPMAP_LINEAR);

		for (slot = bar->mSlots; slot; slot = slot->mNext)
		{
		    if (slot != mDraggedSlot || !mDragged)
			groupPaintThumb (group, slot, transform,
					 attrib.opacity);
		}

		gScreen->setTextureFilter (oldTextureFilter);
	    }
	    break;

	case PAINT_TEXT:
	    if (bar->mTextLayer && (bar->mTextLayer->mState != PaintOff))
	    {
		layer = bar->mTextLayer;

		box.extents.x1 = bar->mRegion->extents.x1 + 5;
		box.extents.x2 = bar->mRegion->extents.x1 +
		                 bar->mTextLayer->mTexWidth + 5;
		box.extents.y1 = bar->mRegion->extents.y2 -
		                 bar->mTextLayer->mTexHeight - 5;
		box.extents.y2 = bar->mRegion->extents.y2 - 5;

		if (box.extents.x2 > bar->mRegion->extents.x2)
		    box.extents.x2 = bar->mRegion->extents.x2;

		/* recalculate the alpha again for text fade... */
		if (layer->mState == PaintFadeIn)
		    alpha -= alpha * layer->mAnimationTime /
			     (optionGetFadeTextTime() * 1000);
		else if (layer->mState == PaintFadeOut)
		    alpha = alpha * layer->mAnimationTime /
			    (optionGetFadeTextTime() * 1000);
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
		CompRegion reg, compClipReg;

		/* remove the old x1 and y1 so we have a relative value */
		box.extents.x2 -= box.extents.x1;
		box.extents.y2 -= box.extents.y1;
		box.extents.x1 = (box.extents.x1 - topTab->x ()) / wScale +
			     topTab->x ();
		box.extents.y1 = (box.extents.y1 - topTab->y ()) / hScale +
			     topTab->y ();

		/* now add the new x1 and y1 so we have a absolute value again,
		also we don't want to stretch the texture... */
		if (box.extents.x2 * wScale < layer->mTexWidth)
		    box.extents.x2 += box.extents.x1;
		else
		    box.extents.x2 = box.extents.x1 + layer->mTexWidth;

		if (box.extents.y2 * hScale < layer->mTexHeight)
		    box.extents.y2 += box.extents.y1;
		else
		    box.extents.y2 = box.extents.y1 + layer->mTexHeight;

		matrix.x0 -= box.extents.x1 * matrix.xx;
		matrix.y0 -= box.extents.y1 * matrix.yy;

		matl.push_back (matrix);

		reg = CompRegion (box.extents.x1, box.extents.y1,
				  box.extents.x2 - box.extents.x1,
				  box.extents.y2 - box.extents.y1);

		compClipReg = CompRegion (clipRegion->extents.x1,
					  clipRegion->extents.y1,
				  clipRegion->extents.x2 - clipRegion->extents.x1,
				  clipRegion->extents.y2 - clipRegion->extents.y1);
		
		gwTopTab->gWindow->geometry ().reset ();

		gwTopTab->gWindow->glAddGeometry (matl, reg, compClipReg);

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
 * groupPaintSelectionOutline
 *
 */
void
GroupScreen::groupPaintSelectionOutline (const GLScreenPaintAttrib sa,
					 const GLMatrix	           transform,
					 CompOutput                *output,
					 bool                      transformed)
{
    int x1, x2, y1, y2;

    x1 = MIN (mX1, mX2);
    y1 = MIN (mY1, mY2);
    x2 = MAX (mX1, mX2);
    y2 = MAX (mY1, mY2);

    if (mGrabState == ScreenGrabSelect)
    {
	GLMatrix sTransform (transform);

	if (transformed)
	{
	    gScreen->glApplyTransform (sa, output, &sTransform);
	    sTransform.toScreenSpace (output, -sa.zTranslate);
	} else
	    sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

	glPushMatrix ();
	glLoadMatrixf (sTransform.getMatrix ());

	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	glEnable (GL_BLEND);

	glColor4usv (optionGetFillColor ());
	glRecti (x1, y2, x2, y1);

	glColor4usv (optionGetLineColor ());
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
    GroupSelection *group, *next;

    cScreen->preparePaint (msSinceLastPaint);

    group = mGroups;
    while (group)
    {
	GroupTabBar *bar = group->mTabBar;

	if (bar)
	{
	    groupApplyForces (bar, (mDragged) ? mDraggedSlot : NULL);
	    groupApplySpeeds (group, msSinceLastPaint);

	    if ((bar->mState != PaintOff) && HAS_TOP_WIN (group))
		groupHandleHoverDetection (group);

	    if (bar->mState == PaintFadeIn || bar->mState == PaintFadeOut)
		groupHandleTabBarFade (group, msSinceLastPaint);

	    if (bar->mTextLayer)
		groupHandleTextFade (group, msSinceLastPaint);

	    if (bar->mBgAnimation)
		groupHandleTabBarAnimation (group, msSinceLastPaint);
	}

	if (group->mChangeState != NoTabChange)
	{
	    group->mChangeAnimationTime -= msSinceLastPaint;
	    if (group->mChangeAnimationTime <= 0)
		groupHandleAnimation (group);
	}

	/* groupDrawTabAnimation may delete the group, so better
	   save the pointer to the next chain element */
	next = group->mNext;

	if (group->mTabbingState != NoTabbing)
	    drawTabAnimation (group, msSinceLastPaint);

	group = next;
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

    mPainted = FALSE;
    mVpX = screen->vp ().x ();
    mVpY = screen->vp ().y ();

    if (mResizeInfo)
    {
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
    }
    else
    {
	for (group = mGroups; group; group = group->mNext)
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
    }

    status = gScreen->glPaintOutput (attrib, transform, region, output, mask);

    if (status && !mPainted)
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
	    groupPaintThumb (NULL, mDraggedSlot, wTransform, OPAQUE);
	    gw->mGroup->mTabBar->mState = state;

	    glPopMatrix ();
	}
	else  if (mGrabState == ScreenGrabSelect)
	{
	    groupPaintSelectionOutline (attrib, transform, output, FALSE);
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

    if ((mVpX == screen->vp ().x ()) && (mVpY == screen->vp ().y ()))
    {
	mPainted = TRUE;

	if ((mGrabState == ScreenGrabTabDrag) &&
	    mDraggedSlot && mDragged)
	{
	    GLMatrix wTransform (transform);

	    gScreen->glApplyTransform (attrib, output, &wTransform);
	    wTransform.toScreenSpace (output, -attrib.zTranslate);
	    glPushMatrix ();
	    glLoadMatrixf (wTransform.getMatrix ());

	    groupPaintThumb (NULL, mDraggedSlot, wTransform, OPAQUE);

	    glPopMatrix ();
	}
	else if (mGrabState == ScreenGrabSelect)
	{
	    groupPaintSelectionOutline (attrib, transform, output, TRUE);
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

    for (group = mGroups; group; group = group->mNext)
    {
	if (group->mTabbingState != NoTabbing)
	    cScreen->damageScreen ();
	else if (group->mChangeState != NoTabChange)
	    cScreen->damageScreen ();
	else if (group->mTabBar)
	{
	    bool needDamage = FALSE;

	    if ((group->mTabBar->mState == PaintFadeIn) ||
		(group->mTabBar->mState == PaintFadeOut))
	    {
		needDamage = TRUE;
	    }

	    if (group->mTabBar->mTextLayer)
	    {
		if ((group->mTabBar->mTextLayer->mState == PaintFadeIn) ||
		    (group->mTabBar->mTextLayer->mState == PaintFadeOut))
		{
		    needDamage = TRUE;
		}
	    }

	    if (group->mTabBar->mBgAnimation)
		needDamage = TRUE;

	    if (mDraggedSlot)
		needDamage = TRUE;

	    if (needDamage)
		groupDamageTabBarRegion (group);
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

    if (mGroup && (mGroup->mNWins > 1) && mGlowQuads)
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

    box.x1 = mResizeGeometry->x - window->input ().left;
    box.y1 = mResizeGeometry->y - window->input ().top;
    box.x2 = mResizeGeometry->x + mResizeGeometry->width +
	     window->serverGeometry ().border () * 2 + window->input ().right;

    if (window->shaded ())
    {
	box.y2 = mResizeGeometry->y + window->height () + window->input ().bottom;
    }
    else
    {
	box.y2 = mResizeGeometry->y + mResizeGeometry->height +
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
	doRotate   = FALSE;
	doTabbing  = FALSE;
	showTabbar = FALSE;
    }

    if (mWindowHideInfo)
	mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

    if (mInSelection || mResizeGeometry || doRotate ||
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

	if (mResizeGeometry)
	{
	    int    xOrigin, yOrigin;
	    float  xScale, yScale;
	    BoxRec box;

	    groupGetStretchRectangle (&box, xScale, yScale);

	    xOrigin = window->x () - w->input ().left;
	    yOrigin = window->y () - w->input ().top;

	    wTransform.translate (xOrigin, yOrigin, 0.0f);
	    wTransform.scale (xScale, yScale, 1.0f);
	    wTransform.translate ((mResizeGeometry->x - window->x ()) /
			     	   xScale - xOrigin,
			     	  (mResizeGeometry->y - window->y ()) /
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
	    gs->groupPaintTabBar (mGroup, wAttrib, wTransform, mask, region.handle ());
	    gWindow->glPaintSetEnabled (this, true);
	}
    }
    else
    {
	status = gWindow->glPaint (attrib, transform, region, mask);
    }

    return status;
}
