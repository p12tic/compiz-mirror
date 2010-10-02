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

void
GroupTabBarSlot::setTargetOpacity (int tOpacity)
{
    mOpacity = tOpacity;
}

void
TextureLayer::setPaintWindow (CompWindow *w)
{
    mPaintWindow = w;
}

void
GroupTabBarSlot::List::paint (const GLWindowPaintAttrib &attrib,
			      const GLMatrix	        &transform,
			      const CompRegion	        &region,
			      const CompRegion	        &clipRegion,
			      int			mask)
{
    GROUP_SCREEN (screen);
	
    foreach (GroupTabBarSlot *slot, *this)
    {
	if (slot != gs->mDraggedSlot || !gs->mDragged)
	{
	    slot->setTargetOpacity (attrib.opacity);
	    slot->paint (attrib, transform, clipRegion,
			  clipRegion, mask);
	}
    }
}

/*
 * GroupTabBarSlot::paint - taken from switcher and modified for tab bar
 *
 */
void
GroupTabBarSlot::paint (const GLWindowPaintAttrib &attrib,
			const GLMatrix	          &transform,
			const CompRegion	  &region,
			const CompRegion	  &clipRegion,
			int			  mask)
{
    CompWindow            *w = mWindow;
    unsigned int	  oldGlAddGeometryIndex;
    unsigned int	  oldGlDrawIndex;
    GLWindowPaintAttrib   wAttrib (GLWindow::get (mWindow)->paintAttrib ());
    int                   tw, th;

    GROUP_WINDOW (w);
    GROUP_SCREEN (screen);

    tw = mRegion.boundingRect ().width ();
    th = mRegion.boundingRect ().height ();

    /* Wrap drawWindowGeometry to make sure the general
       drawWindowGeometry function is used */
    oldGlAddGeometryIndex = gw->gWindow->glAddGeometryGetCurrentIndex ();
    gw->gWindow->glAddGeometrySetCurrentIndex (MAXSHORT);

    /* animate fade */
    if (mTabBar->mState == PaintFadeIn)
    {
	wAttrib.opacity -= wAttrib.opacity * mTabBar->mAnimationTime /
	                   (gs->optionGetFadeTime () * 1000);
    }
    else if (mTabBar->mState == PaintFadeOut)
    {
	wAttrib.opacity = wAttrib.opacity * mTabBar->mAnimationTime /
	                  (gs->optionGetFadeTime () * 1000);
    }

    wAttrib.opacity = wAttrib.opacity * mOpacity / OPAQUE;

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

	getDrawOffset (vx, vy);

	wAttrib.xTranslate = (mRegion.boundingRect ().x1 () +
			      mRegion.boundingRect ().x2 ()) / 2 + vx;
	wAttrib.yTranslate = mRegion.boundingRect ().y1 () + vy;

	wTransform.translate (wAttrib.xTranslate, wAttrib.yTranslate, 0.0f);
	wTransform.scale (wAttrib.xScale, wAttrib.yScale, 1.0f);
	wTransform.translate (-(WIN_X (w) + WIN_WIDTH (w) / 2),
			 -(WIN_Y (w) - w->output ().top), 0.0f);

	glPushMatrix ();
	glLoadMatrixf (wTransform.getMatrix ());

	oldGlDrawIndex = gw->gWindow->glDrawGetCurrentIndex ();
	gw->gWindow->glDraw (wTransform, fragment, clipRegion,
			  mask | PAINT_WINDOW_TRANSFORMED_MASK |
			  PAINT_WINDOW_TRANSLUCENT_MASK);
	gw->gWindow->glDrawSetCurrentIndex (oldGlDrawIndex);

	glPopMatrix ();
    }

    gw->gWindow->glAddGeometrySetCurrentIndex (oldGlAddGeometryIndex);
}

/*
 * TextureLayer::paint
 *
 */

void
TextureLayer::paint (const GLWindowPaintAttrib &attrib,
		     const GLMatrix	       &transform,
		     const CompRegion	       &paintRegion,
		     const CompRegion	       &clipRegion,
		     int		       mask)
{
    GroupWindow *gwTopTab = GroupWindow::get (mPaintWindow);
    const CompRect &box = paintRegion.boundingRect ();

    foreach (GLTexture *tex, mTexture)
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
	x1 = (x1 - mPaintWindow->x ()) / attrib.xScale +
		     mPaintWindow->x ();
	y1 = (y1 - mPaintWindow->y ()) / attrib.yScale +
		     mPaintWindow->y ();

	/* now add the new x1 and y1 so we have a absolute value again,
	also we don't want to stretch the texture... */
	if (x2 * attrib.xScale < width ())
	    x2 += x1;
	else
	    x2 = x1 + width ();

	if (y2 * attrib.yScale < height ())
	    y2 += y1;
	else
	    y2 = y1 + height ();

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
	    GLFragment::Attrib fragment (attrib);
	    GLMatrix	       wTransform (transform);

	    wTransform.translate (WIN_X (mPaintWindow),
				  WIN_Y (mPaintWindow), 0.0f);
	    wTransform.scale (attrib.xScale, attrib.yScale, 1.0f);
	    wTransform.translate (
			 attrib.xTranslate / attrib.xScale - WIN_X (mPaintWindow),
			 attrib.yTranslate / attrib.yScale - WIN_Y (mPaintWindow),
			 0.0f);

	    glPushMatrix ();
	    glLoadMatrixf (wTransform.getMatrix ());

	    fragment.setOpacity (attrib.opacity);

	    gwTopTab->glDrawTexture (tex, fragment, mask |
					PAINT_WINDOW_BLEND_MASK |
					PAINT_WINDOW_TRANSFORMED_MASK |
					PAINT_WINDOW_TRANSLUCENT_MASK);

	    glPopMatrix ();
	}
    }
}

void
BackgroundLayer::paint (const GLWindowPaintAttrib &attrib,
			const GLMatrix	          &transform,
			const CompRegion	  &paintRegion,
			const CompRegion	  &clipRegion,
			int		          mask)
{
    int   newWidth;
    GLWindowPaintAttrib wAttrib (attrib);
    CompRect box = paintRegion.boundingRect ();

    /* handle the repaint of the background */
    newWidth = mGroup->mTabBar->mRegion.boundingRect ().width ();
    if (newWidth > width ())
	newWidth = width ();

    wAttrib.xScale = (double) (mGroup->mTabBar->mRegion.boundingRect ().width () / 
		       (double) newWidth);

    /* FIXME: maybe move this over to groupResizeTabBarRegion -
     * the only problem is that we would have 2 redraws if
     * here is an animation */
    if (newWidth != mGroup->mTabBar->mOldWidth || mGroup->mTabBar->mBgLayer->mBgAnimation)
	render ();

    mGroup->mTabBar->mOldWidth = newWidth;
    box	  = mGroup->mTabBar->mRegion.boundingRect ();
    
    TextureLayer::paint (wAttrib, transform, box, clipRegion, mask);
}

void
SelectionLayer::paint (const GLWindowPaintAttrib &attrib,
		       const GLMatrix	         &transform,
		       const CompRegion	  	 &paintRegion,
		       const CompRegion	  	 &clipRegion,
		       int		         mask)
{
    TextureLayer::paint (attrib, transform,
			 mGroup->mTabBar->mTopTab->mRegion,
			 clipRegion, mask);
}

void
TextLayer::paint (const GLWindowPaintAttrib &attrib,
	          const GLMatrix	    &transform,
	          const CompRegion	    &paintRegion,
	          const CompRegion	    &clipRegion,
	          int		            mask)
{
    /* add a slight buffer around the clipping region
     * to account for the text
     */
    CompRect	        box;
    int			alpha = OPAQUE;
    GLWindowPaintAttrib wAttrib (attrib);
    
    GROUP_SCREEN (screen);

    int x1 = mGroup->mTabBar->mRegion.boundingRect ().x1 () + 5;
    int x2 = mGroup->mTabBar->mRegion.boundingRect ().x1 () +
	     width () + 5;
    int y1 = mGroup->mTabBar->mRegion.boundingRect ().y2 () -
	     height () - 5;
    int y2 = mGroup->mTabBar->mRegion.boundingRect ().y2 () - 5;

    if (x2 > mGroup->mTabBar->mRegion.boundingRect ().x2 ())
	x2 = mGroup->mTabBar->mRegion.boundingRect ().x2 ();

    box = CompRect (x1, y1, x2 - x1, y2 - y1);

    /* recalculate the alpha again for text fade... */
    if (mState == PaintFadeIn)
	alpha -= alpha * mAnimationTime /
	     (gs->optionGetFadeTextTime () * 1000);
    else if (mState == PaintFadeOut)
	alpha = alpha * mAnimationTime /
	    (gs->optionGetFadeTextTime () * 1000);

    wAttrib.opacity = alpha * ((float) wAttrib.opacity / OPAQUE);
    
    TextureLayer::paint (wAttrib, transform, box, clipRegion, mask);
}

/*
 * GroupTabBar::paint
 *
 */
void
GroupTabBar::paint (const GLWindowPaintAttrib    &attrib,
		    const GLMatrix		 &transform,
		    unsigned int		 mask,
		    CompRegion		 	 clipRegion)
{
    CompWindow      *topTab;
    std::vector <GLLayer *> paintList;
    CompRect        box;
    
    GROUP_SCREEN (screen);

    if (HAS_TOP_WIN (mGroup))
	topTab = TOP_TAB (mGroup);
    else
	topTab = PREV_TOP_TAB (mGroup);

    mBgLayer->setPaintWindow (topTab);
    mSelectionLayer->setPaintWindow (topTab);

    paintList.push_back (mBgLayer);
    paintList.push_back (mSelectionLayer);
    paintList.push_back (&mSlots);
    
    if (mTextLayer && (mTextLayer->mState != PaintOff))
    {
	mTextLayer->setPaintWindow (topTab);
	paintList.push_back (mTextLayer);
    }

    foreach (GLLayer *layer, paintList)
    {
	GLWindowPaintAttrib wAttrib (attrib);
	GLenum              oldTextureFilter;
	int            	    alpha = OPAQUE;
	
	wAttrib.xScale = 1.0f;
	wAttrib.yScale = 1.0f;
	
	oldTextureFilter = gs->gScreen->textureFilter ();

	if (gs->optionGetMipmaps ())
	    gs->gScreen->setTextureFilter (GL_LINEAR_MIPMAP_LINEAR);

	if (mState == PaintFadeIn)
	    alpha -= alpha * mAnimationTime / (gs->optionGetFadeTime () * 1000);
	else if (mState == PaintFadeOut)
	    alpha = alpha * mAnimationTime / (gs->optionGetFadeTime () * 1000);

	wAttrib.opacity = alpha * ((float) wAttrib.opacity / OPAQUE);
	layer->paint (wAttrib, transform, clipRegion, clipRegion, mask);
	
	gs->gScreen->setTextureFilter (oldTextureFilter);
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

    if (gs->mGrabState == GroupScreen::ScreenGrabSelect)
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
 * GroupScreen::preparePaint
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
	    bar->applyForces ((mDragged) ? mDraggedSlot : NULL);
	    bar->applySpeeds (msSinceLastPaint);

	    if (bar->mState == PaintFadeIn || bar->mState == PaintFadeOut)
		bar->handleTabBarFade (msSinceLastPaint);

	    if (bar->mTextLayer)
		bar->handleTextFade (msSinceLastPaint);

	    if (bar->mBgLayer && bar->mBgLayer->mBgAnimation)
		bar->handleTabBarAnimation (msSinceLastPaint);
	}

	if (group->mTabBar &&
	    group->mTabBar->mChangeState != GroupTabBar::NoTabChange)
	{
	    group->mTabBar->mChangeAnimationTime -= msSinceLastPaint;
	    if (group->mTabBar->mChangeAnimationTime <= 0)
		group->handleAnimation ();
	}

	/* groupDrawTabAnimation may delete the group, so better
	   save the pointer to the next chain element */

	it++;

	if (group->mTabbingState != GroupSelection::NoTabbing)
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
	if ((group->mTabBar &&
	     group->mTabBar->mChangeState != GroupTabBar::NoTabChange) ||
	    group->mTabbingState != GroupSelection::NoTabbing)
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

	    wTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

	    glPushMatrix ();
	    glLoadMatrixf (wTransform.getMatrix ());

	    /* prevent tab bar drawing.. */
	    state = mDraggedSlot->mTabBar->mState;
	    mDraggedSlot->mTabBar->mState = PaintOff;
	    mDraggedSlot->setTargetOpacity (OPAQUE);
	    mDraggedSlot->paint (GLWindow::get (mDraggedSlot->mWindow)->paintAttrib (),
				 wTransform, region, region, 0);
	    mDraggedSlot->mTabBar->mState = state;

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
    PaintState state;
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

	    /* prevent tab bar drawing.. */
	    state = mDraggedSlot->mTabBar->mState;
	    mDraggedSlot->mTabBar->mState = PaintOff;
	    mDraggedSlot->setTargetOpacity (OPAQUE);
	    mDraggedSlot->paint (GLWindow::get (mDraggedSlot->mWindow)->paintAttrib (),
				 wTransform, region, region, 0);
	    mDraggedSlot->mTabBar->mState = state;

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
	if (group->mTabbingState != GroupSelection::NoTabbing)
	    cScreen->damageScreen ();
	else if (group->mTabBar &&
		 group->mTabBar->mChangeState != GroupTabBar::NoTabChange)
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

	    if (group->mTabBar->mBgLayer &&
		group->mTabBar->mBgLayer->mBgAnimation)
		needDamage = true;

	    if (mDraggedSlot)
		needDamage = true;

	    if (needDamage)
		group->mTabBar->damageRegion ();
	}
    }
}

void
GroupWindow::computeGlowQuads (GLTexture::Matrix *matrix)
{
    CompRect	      *box;
    int		      x1, x2, y1, y2;
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

    x1 = WIN_REAL_X (w) - glowSize + glowOffset;
    y1 = WIN_REAL_Y (w) - glowSize + glowOffset;
    x2 = WIN_REAL_X (w) + glowOffset;
    y2 = WIN_REAL_Y (w) + glowOffset;

    quadMatrix->xx = 1.0f / glowSize;
    quadMatrix->yy = 1.0f / (glowSize);
    quadMatrix->x0 = -(x1 * quadMatrix->xx);
    quadMatrix->y0 = -(y1 * quadMatrix->yy);

    x2 = MIN (WIN_REAL_X (w) + glowOffset,
	      WIN_REAL_X (w) + (WIN_REAL_WIDTH (w) / 2));
    y2 = MIN (WIN_REAL_Y (w) + glowOffset,
	      WIN_REAL_Y (w) + (WIN_REAL_HEIGHT (w) / 2));

    *box = CompRect (x1, y1, x2 - x1, y2 - y1);

    /* Top right corner */
    box = &mGlowQuads[GLOWQUAD_TOPRIGHT].mBox;
    mGlowQuads[GLOWQUAD_TOPRIGHT].mMatrix = *matrix;
    quadMatrix = &mGlowQuads[GLOWQUAD_TOPRIGHT].mMatrix;

    x1 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset;
    y1 = WIN_REAL_Y (w) - glowSize + glowOffset;
    x2 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) + glowSize - glowOffset;
    y2 = WIN_REAL_Y (w) + glowOffset;

    quadMatrix->xx = -1.0f / glowSize;
    quadMatrix->yy = 1.0f / glowSize;
    quadMatrix->x0 = 1.0 - (x1 * quadMatrix->xx);
    quadMatrix->y0 = -(y1 * quadMatrix->yy);

    x1 = MAX (WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset,
	      WIN_REAL_X (w) + (WIN_REAL_WIDTH (w) / 2));
    y2 = MIN (WIN_REAL_Y (w) + glowOffset,
	      WIN_REAL_Y (w) + (WIN_REAL_HEIGHT (w) / 2));

    *box = CompRect (x1, y1, x2 - x1, y2 - y1);

    /* Bottom left corner */
    box = &mGlowQuads[GLOWQUAD_BOTTOMLEFT].mBox;
    mGlowQuads[GLOWQUAD_BOTTOMLEFT].mMatrix = *matrix;
    quadMatrix = &mGlowQuads[GLOWQUAD_BOTTOMLEFT].mMatrix;

    x1 = WIN_REAL_X (w) - glowSize + glowOffset;
    y1 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset;
    x2 = WIN_REAL_X (w) + glowOffset;
    y2 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) + glowSize - glowOffset;

    quadMatrix->xx = 1.0f / glowSize;
    quadMatrix->yy = -1.0f / glowSize;
    quadMatrix->x0 = -(x1 * quadMatrix->xx);
    quadMatrix->y0 = 1.0f - (y1 * quadMatrix->yy);

    y1 = MAX (WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset,
	      WIN_REAL_Y (w) + (WIN_REAL_HEIGHT (w) / 2));
    x2 = MIN (WIN_REAL_X (w) + glowOffset,
	      WIN_REAL_X (w) + (WIN_REAL_WIDTH (w) / 2));

    *box = CompRect (x1, y1, x2 - x1, y2 - y1);

    /* Bottom right corner */
    box = &mGlowQuads[GLOWQUAD_BOTTOMRIGHT].mBox;
    mGlowQuads[GLOWQUAD_BOTTOMRIGHT].mMatrix = *matrix;
    quadMatrix = &mGlowQuads[GLOWQUAD_BOTTOMRIGHT].mMatrix;

    x1 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset;
    y1 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset;
    x2 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) + glowSize - glowOffset;
    y2 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) + glowSize - glowOffset;

    quadMatrix->xx = -1.0f / glowSize;
    quadMatrix->yy = -1.0f / glowSize;
    quadMatrix->x0 = 1.0 - (x1 * quadMatrix->xx);
    quadMatrix->y0 = 1.0 - (y1 * quadMatrix->yy);

    x1 = MAX (WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset,
	      WIN_REAL_X (w) + (WIN_REAL_WIDTH (w) / 2));
    y1 = MAX (WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset,
	      WIN_REAL_Y (w) + (WIN_REAL_HEIGHT (w) / 2));

    *box = CompRect (x1, y1, x2 - x1, y2 - y1);

    /* Top edge */
    box = &mGlowQuads[GLOWQUAD_TOP].mBox;
    mGlowQuads[GLOWQUAD_TOP].mMatrix = *matrix;
    quadMatrix = &mGlowQuads[GLOWQUAD_TOP].mMatrix;

    x1 = WIN_REAL_X (w) + glowOffset;
    y1 = WIN_REAL_Y (w) - glowSize + glowOffset;
    x2 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset;
    y2 = WIN_REAL_Y (w) + glowOffset;

    quadMatrix->xx = 0.0f;
    quadMatrix->yy = 1.0f / glowSize;
    quadMatrix->x0 = 1.0;
    quadMatrix->y0 = -(y1 * quadMatrix->yy);

    *box = CompRect (x1, y1, x2 - x1, y2 - y1);

    /* Bottom edge */
    box = &mGlowQuads[GLOWQUAD_BOTTOM].mBox;
    mGlowQuads[GLOWQUAD_BOTTOM].mMatrix = *matrix;
    quadMatrix = &mGlowQuads[GLOWQUAD_BOTTOM].mMatrix;

    x1 = WIN_REAL_X (w) + glowOffset;
    y1 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset;
    x2 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset;
    y2 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) + glowSize - glowOffset;

    quadMatrix->xx = 0.0f;
    quadMatrix->yy = -1.0f / glowSize;
    quadMatrix->x0 = 1.0;
    quadMatrix->y0 = 1.0 - (y1 * quadMatrix->yy);

    *box = CompRect (x1, y1, x2 - x1, y2 - y1);

    /* Left edge */
    box = &mGlowQuads[GLOWQUAD_LEFT].mBox;
    mGlowQuads[GLOWQUAD_LEFT].mMatrix = *matrix;
    quadMatrix = &mGlowQuads[GLOWQUAD_LEFT].mMatrix;

    x1 = WIN_REAL_X (w) - glowSize + glowOffset;
    y1 = WIN_REAL_Y (w) + glowOffset;
    x2 = WIN_REAL_X (w) + glowOffset;
    y2 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset;

    quadMatrix->xx = 1.0f / glowSize;
    quadMatrix->yy = 0.0f;
    quadMatrix->x0 = -(x1 * quadMatrix->xx);
    quadMatrix->y0 = 1.0;

    *box = CompRect (x1, y1, x2 - x1, y2 - y1);

    /* Right edge */
    box = &mGlowQuads[GLOWQUAD_RIGHT].mBox;
    mGlowQuads[GLOWQUAD_RIGHT].mMatrix = *matrix;
    quadMatrix = &mGlowQuads[GLOWQUAD_RIGHT].mMatrix;

    x1 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) - glowOffset;
    y1 = WIN_REAL_Y (w) + glowOffset;
    x2 = WIN_REAL_X (w) + WIN_REAL_WIDTH (w) + glowSize - glowOffset;
    y2 = WIN_REAL_Y (w) + WIN_REAL_HEIGHT (w) - glowOffset;

    quadMatrix->xx = -1.0f / glowSize;
    quadMatrix->yy = 0.0f;
    quadMatrix->x0 = 1.0 - (x1 * quadMatrix->xx);
    quadMatrix->y0 = 1.0;

    *box = CompRect (x1, y1, x2 - x1, y2 - y1);
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
	    CompRegion reg;
	    int    i;

	    gWindow->geometry ().reset ();

	    for (i = 0; i < NUM_GLOWQUADS; i++)
	    {
		reg = CompRegion (mGlowQuads[i].mBox);

		if (reg.boundingRect ().x1 () < reg.boundingRect ().x2 () &&
		    reg.boundingRect ().y1 () < reg.boundingRect ().y2 ())
		{
		    GLTexture::MatrixList matl;
		    reg = CompRegion (reg.boundingRect ().x1 (),
				      reg.boundingRect ().y1 (),
			  	      reg.boundingRect ().width (),
				      reg.boundingRect ().height ());

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
GroupWindow::getStretchRectangle (CompRect &box,
				       float    &xScaleRet,
				       float    &yScaleRet)
{
    int    x1, x2, y1, y2;
    int    width, height;
    float  xScale, yScale;

    x1 = mResizeGeometry.x () - window->input ().left;
    y1 = mResizeGeometry.y () - window->input ().top;
    x2 = mResizeGeometry.x () + mResizeGeometry.width () +
	     window->serverGeometry ().border () * 2 + window->input ().right;

    if (window->shaded ())
    {
	y2 = mResizeGeometry.y () + window->height () + window->input ().bottom;
    }
    else
    {
	y2 = mResizeGeometry.y () + mResizeGeometry.height () +
	         window->serverGeometry ().border () * 2 + window->input ().bottom;
    }

    width  = window->width ()  + window->input ().left + window->input ().right;
    height = window->height () + window->input ().top  + window->input ().bottom;

    xScale = (width)  ? (x2 - x1) / (float) width  : 1.0f;
    yScale = (height) ? (y2 - y1) / (float) height : 1.0f;

    x1 = x1 - (window->output ().left - window->input ().left) * xScale;
    y1 = y1 - (window->output ().top - window->input ().top) * yScale;
    x2 = x2 + window->output ().right * xScale;
    y2 = y2 + window->output ().bottom * yScale;
    
    box = CompRect (x1, y1, x2 - x1, y2 - y1);

    xScaleRet = xScale;
    yScaleRet = yScale;
}

void
GroupScreen::damagePaintRectangle (const CompRect &box)
{
    CompRegion reg (box);

    reg.translate (-1, -1);
    reg.shrink (1, 1);

    cScreen->damageRegion (reg);
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

	doRotate = (group->mTabBar &&
		    group->mTabBar->mChangeState != GroupTabBar::NoTabChange) &&
	           HAS_TOP_WIN (group) && HAS_PREV_TOP_WIN (group) &&
	           (IS_TOP_TAB (w, group) || IS_PREV_TOP_TAB (w, group));

	doTabbing = (mAnimateState & (IS_ANIMATED | FINISHED_ANIMATION)) &&
	            !(group->mTabBar && IS_TOP_TAB (w, group) &&
		      (group->mTabbingState == GroupSelection::Tabbing));

	showTabbar = group->mTabBar && (group->mTabBar->mState != PaintOff) &&
	             (((IS_TOP_TAB (w, group)) &&
		       ((group->mTabBar->mChangeState == GroupTabBar::NoTabChange) ||
			(group->mTabBar->mChangeState == GroupTabBar::TabChangeNewIn))) ||
		      (IS_PREV_TOP_TAB (w, group) &&
		       (group->mTabBar->mChangeState == GroupTabBar::TabChangeOldOut)));
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
		drawnPosX = mDestination.x ();
		drawnPosY = mDestination.y ();
	    }
	    else
	    {
		drawnPosX = mOrgPos.x () + mTx;
		drawnPosY = mOrgPos.y () + mTy;
	    }

	    distanceX = drawnPosX - mDestination.x ();
	    distanceY = drawnPosY - mDestination.y ();
	    distance = sqrt (pow (distanceX, 2) + pow (distanceY, 2));

	    distanceX = (mOrgPos.x () - mDestination.x ());
	    distanceY = (mOrgPos.y () - mDestination.y ());
	    origDistance = sqrt (pow (distanceX, 2) + pow (distanceY, 2));

	    if (!distanceX && !distanceY)
		progress = 1.0f;
	    else
		progress = 1.0f - (distance / origDistance);

	    animProgress = progress;

	    progress = MAX (progress, 0.0f);
	    if (mGroup->mTabbingState == GroupSelection::Tabbing)
		progress = 1.0f - progress;

	    wAttrib.opacity = (float)wAttrib.opacity * progress;
	}

	if (doRotate)
	{
	    float timeLeft = mGroup->mTabBar->mChangeAnimationTime;
	    int   animTime = gs->optionGetChangeAnimationTime () * 500;

	    if (mGroup->mTabBar->mChangeState == GroupTabBar::TabChangeOldOut)
		timeLeft += animTime;

	    /* 0 at the beginning, 1 at the end */
	    animProgress = 1 - (timeLeft / (2 * animTime));
	}

	if (!mResizeGeometry.isEmpty ())
	{
	    int    xOrigin, yOrigin;
	    float  xScale, yScale;
	    CompRect box;

	    getStretchRectangle (box, xScale, yScale);

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
		if (mGroup->mTabbingState == GroupSelection::Tabbing)
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
			morphBase = mGroup->mTabBar->mLastTopTab;
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

		if (mGroup->mTabBar->mChangeAnimationDirection < 0)
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
	    mGroup->mTabBar->paint (wAttrib, wTransform, mask, region);
	    gWindow->glPaintSetEnabled (this, true);
	}
    }
    else
    {
	status = gWindow->glPaint (attrib, transform, region, mask);
    }

    return status;
}
