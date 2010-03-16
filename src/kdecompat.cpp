/*
 *
 * Compiz KDE compatibility plugin
 *
 * kdecompat.cpp
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

#include "kdecompat.h"

void
KDECompatWindow::stopCloseAnimation ()
{
    while (mUnmapCnt)
    {
	window->unmap ();
	mUnmapCnt--;
    }

    while (mDestroyCnt)
    {
	window->destroy ();
	mDestroyCnt--;
    }
}

void
KDECompatWindow::sendSlideEvent (bool start)
{
    CompOption::Vector  o;
    
    o.resize (2);

    o.at (0) = CompOption ("window", CompOption::TypeInt);
    o.at (0).value ().set ((int) window->id ());

    o.at (1) = CompOption ("active", CompOption::TypeBool);
    o.at (1).value ().set (start);
    
    screen->handleCompizEvent ("kdecompat", "slide", o);
}

void
KDECompatWindow::startSlideAnimation (bool appearing)
{
    if (mSlideData)
    {
	SlideData *data = mSlideData;

	KDECOMPAT_SCREEN (screen);

	if (appearing)
	    data->duration = ks->optionGetSlideInDuration ();
	else
	    data->duration = ks->optionGetSlideOutDuration ();

	if (data->remaining > data->duration)
	    data->remaining = data->duration;
	else
	    data->remaining = data->duration - data->remaining;

	data->appearing      = appearing;
	ks->mHasSlidingPopups = TRUE;
	cWindow->addDamage ();
	sendSlideEvent (TRUE);
    }
}

void
KDECompatWindow::endSlideAnimation ()
{
    if (mSlideData)
    {
	mSlideData->remaining = 0;
	stopCloseAnimation ();
	sendSlideEvent (FALSE);
    }
}

void
KDECompatScreen::preparePaint (int msSinceLastPaint)
{
    if (mHasSlidingPopups)
    {
	foreach (CompWindow *w, screen->windows ())
	{
	    KDECOMPAT_WINDOW (w);

	    if (!kw->mSlideData)
		continue;

	    kw->mSlideData->remaining -= msSinceLastPaint;
	    if (kw->mSlideData->remaining <= 0)
		kw->endSlideAnimation ();
	}
    }

    cScreen->preparePaint (msSinceLastPaint);
}

bool
KDECompatScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
				const GLMatrix		  &transform,
				const CompRegion	  &region,
				CompOutput		  *output,
				unsigned int		  mask)
{
    bool status;

    if (mHasSlidingPopups)
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    status = gScreen->glPaintOutput (attrib, transform, region, output, mask);

    return status;
}

void
KDECompatScreen::donePaint ()
{
    if (mHasSlidingPopups)
    {
	mHasSlidingPopups = FALSE;

	foreach (CompWindow *w, screen->windows ())
	{
	    KDECOMPAT_WINDOW (w);

	    if (kw->mSlideData && kw->mSlideData->remaining)
	    {
		kw->cWindow->addDamage ();
		mHasSlidingPopups = TRUE;
	    }
	}
    }
    
    cScreen->donePaint ();
}

bool
KDECompatWindow::glPaint (const GLWindowPaintAttrib &attrib,
			  const GLMatrix	    &transform,
			  const CompRegion	    &region,
			  unsigned int		    mask)
{
    Bool         status;

    KDECOMPAT_SCREEN (screen);

    if (mSlideData && mSlideData->remaining)
    {
	GLFragment::Attrib fragment (gWindow->paintAttrib ());
	GLMatrix           wTransform = transform;
	SlideData          *data = mSlideData;
	float              xTranslate = 0, yTranslate = 0, remainder;
	CompRect           clipBox (window->x (), window->y (),
				    window->width (), window->height ());

	if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
	    return FALSE;

	remainder = (float) data->remaining / data->duration;
	if (!data->appearing)
	    remainder = 1.0 - remainder;

	switch (data->position) {
	case East:
	    xTranslate = (data->start - window->x ()) * remainder;
	    clipBox.setWidth (data->start - clipBox.x ());
	    break;
	case West:
	    xTranslate = (data->start - window->width ()) * remainder;
	    clipBox.setX (data->start);
	    break;
	case North:
	    yTranslate = (data->start - window->height ()) * remainder;
	    clipBox.setY (data->start);
	    break;
	case South:
	default:
	    yTranslate = (data->start - window->y ()) * remainder;
	    clipBox.setWidth (data->start - clipBox.y1 ());
	    break;
	}
	
	status = gWindow->glPaint (attrib, transform, region, mask |
				   PAINT_WINDOW_NO_CORE_INSTANCE_MASK);

	if (window->alpha () || fragment.getOpacity () != OPAQUE)
	    mask |= PAINT_WINDOW_TRANSLUCENT_MASK;

	wTransform.translate (xTranslate, yTranslate, 0.0f);

	glPushMatrix ();
	glLoadMatrixf (wTransform.getMatrix ());

	glPushAttrib (GL_SCISSOR_BIT);
	glEnable (GL_SCISSOR_TEST);

	glScissor (clipBox.x1 (), screen->height () - clipBox.y2 (),
		   clipBox.x2 () - clipBox.x1 (), clipBox.y2 () - clipBox.y1 ());

	gWindow->glDraw (wTransform, fragment, region,
			 mask | PAINT_WINDOW_TRANSFORMED_MASK);

	glDisable (GL_SCISSOR_TEST);
	glPopAttrib ();
	glPopMatrix ();
    }
    else
    {
	status = gWindow->glPaint (attrib, transform, region, mask);
    }

    status = gWindow->glPaint (attrib, transform, region, mask);

    if (!ks->optionGetPlasmaThumbnails () ||
	mPreviews.empty ()                ||
	!window->mapNum ()                ||
	(mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK))
    {
	return status;
    }

    foreach (const Thumb& thumb, mPreviews)
    {
	CompWindow     *tw = screen->findWindow (thumb.id);
	GLWindow       *gtw;
	const CompRect &rect = thumb.thumb;
	unsigned int   paintMask = mask | PAINT_WINDOW_TRANSFORMED_MASK;
	float          xScale = 1.0f, yScale = 1.0f, xTranslate, yTranslate;
	GLTexture      *icon = NULL;

	if (!tw)
	    continue;

	gtw = GLWindow::get (tw);

	xTranslate = rect.x () + window->x () - tw->x ();
	yTranslate = rect.y () + window->x () - tw->y ();

	if (!gtw->textures ().empty ())
	{
	    CompRect     inputRect = tw->inputRect ();

	    xScale = (float) rect.width () / inputRect.width ();
	    yScale = (float) rect.height () / inputRect.height ();

	    xTranslate += tw->input ().left * xScale;
	    yTranslate += tw->input ().top * yScale;
	}
	else
	{
	    icon = gWindow->getIcon (256, 256);
	    if (!icon)
		icon = ks->gScreen->defaultIcon ();

	    if (icon && !icon->name ())
		icon = NULL;

	    if (icon)
	    {
		GLTexture::MatrixList matrices (1);

		paintMask |= PAINT_WINDOW_BLEND_MASK;

		if (icon->width () >= rect.width () ||
		    icon->height () >= rect.height ())
		{
		    xScale = (float) rect.width () / icon->width ();
		    yScale = (float) rect.height () / icon->height ();

		    if (xScale < yScale)
			yScale = xScale;
		    else
			xScale = yScale;
		}

		xTranslate += rect.width () / 2 -
			      (icon->width () * xScale / 2);
		yTranslate += rect.height () / 2 -
			      (icon->height () * yScale / 2);

		matrices[0] = icon->matrix ();
		matrices[0].x0 -= (tw->x () * icon->matrix ().xx);
		matrices[0].y0 -= (tw->y () * icon->matrix ().yy);

		gtw->geometry ().reset ();
		gtw->glAddGeometry (matrices, tw->geometry (), infiniteRegion);

		if (!gtw->geometry ().vertices)
		    icon = NULL;
	    }
	}

	if (!gtw->textures ().empty () || icon)
	{
	    GLFragment::Attrib fragment (attrib);
	    GLMatrix           wTransform (transform);

	    if (tw->alpha () || fragment.getOpacity () != OPAQUE)
		paintMask |= PAINT_WINDOW_TRANSLUCENT_MASK;

	    wTransform.translate (tw->x (), tw->y (), 0.0f);
	    wTransform.scale (xScale, yScale, 1.0f);
	    wTransform.translate (xTranslate / xScale - tw->x (),
	    			  yTranslate / yScale - tw->y (),
	    			  0.0f);

	    glPushMatrix ();
	    glLoadMatrixf (wTransform.getMatrix ());

	    if (!gtw->textures ().empty ())
		gWindow->glDraw (wTransform, fragment,
				 infiniteRegion, paintMask);
	    else if (icon)
		gWindow->glDrawTexture (icon, fragment, paintMask);

	    glPopMatrix ();
	}
    }

    return status;
}

void
KDECompatWindow::updatePreviews ()
{
    Atom	    actual;
    int		    result, format;
    unsigned long   n, left;
    unsigned char   *propData;

    KDECOMPAT_SCREEN (screen);

    mPreviews.clear ();

    result = XGetWindowProperty (screen->dpy (), window->id (),
				 ks->mKdePreviewAtom, 0,
				 32768, FALSE, AnyPropertyType, &actual,
				 &format, &n, &left, &propData);

    if (result == Success && propData)
    {
	if (format == 32 && actual == ks->mKdePreviewAtom)
	{
	    long         *data    = (long *) propData;
	    unsigned int nPreview = *data++;

	    if (n == (6 * nPreview + 1))
	    {
		while (mPreviews.size () < nPreview)
		{
		    Thumb t;

		    if (*data++ != 5)
			break;

		    t.id = *data++;
		    t.thumb.setX (*data++);
		    t.thumb.setY (*data++);
		    t.thumb.setWidth (*data++);
		    t.thumb.setHeight (*data++);

		    mPreviews.push_back (t);
		}
	    }
	}

	XFree (propData);
    }

    foreach (CompWindow *cw, screen->windows ())
    {
	CompWindow      *rw;
	KDECompatWindow *kcw = KDECompatWindow::get (cw);

	kcw->mIsPreview = false;

	foreach (rw, screen->windows ())
	{
	    KDECompatWindow *krw = KDECompatWindow::get (rw);

	    foreach (const Thumb& t, krw->mPreviews)
	    {
		if (t.id == cw->id ())
		{
		    kcw->mIsPreview = true;
		    break;
		}
	    }

	    if (kcw->mIsPreview)
		break;
	}
    }
}

void
KDECompatWindow::updateSlidePosition ()
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *propData;

    
    KDECOMPAT_SCREEN (screen);

    if (mSlideData)
    {
	delete mSlideData;
	mSlideData = NULL;
    }

    result = XGetWindowProperty (screen->dpy (), window->id (), ks->mKdeSlideAtom, 0,
				 32768, FALSE, AnyPropertyType, &actual,
				 &format, &n, &left, &propData);

    if (result == Success && propData)
    {
	if (format == 32 && actual == ks->mKdeSlideAtom && n == 2)
	{
	    long *data = (long *) propData;

	    mSlideData = new SlideData;
	    if (mSlideData)
	    {
		mSlideData->remaining = 0;
		mSlideData->start     = data[0];
		mSlideData->position  = (KDECompatWindow::SlidePosition) data[1];
	    }
	}

	XFree (propData);
    }
}

void
KDECompatWindow::handleClose (bool destroy)
{
    KDECOMPAT_SCREEN (screen);

    if (mSlideData && ks->optionGetSlidingPopups ())
    {
	if (destroy)
	{
	    mDestroyCnt++;
	    window->incrementDestroyReference ();
	}
	else
	{
	    mUnmapCnt++;
	    window->incrementDestroyReference ();
	}

	if (mSlideData->appearing || !mSlideData->remaining)
	    startSlideAnimation (FALSE);
    }
}

void
KDECompatScreen::handleEvent (XEvent *event)
{
    CompWindow *w;

    switch (event->type) {
    case DestroyNotify:
	w = screen->findWindow (event->xdestroywindow.window);
	if (w)
	    KDECompatWindow::get (w)->handleClose (TRUE);
	break;
    case UnmapNotify:
	w = screen->findWindow (event->xunmap.window);
	if (w && !w->pendingUnmaps ())
	    KDECompatWindow::get (w)->handleClose (FALSE);
	break;
    case MapNotify:
	w = screen->findWindow (event->xmap.window);
	if (w)
	    KDECompatWindow::get (w)->stopCloseAnimation ();
	break;
    }

    screen->handleEvent (event);

    switch (event->type) {
    case PropertyNotify:
	if (event->xproperty.atom == mKdePreviewAtom)
	{
	    w = screen->findWindow (event->xproperty.window);
	    if (w)
		KDECompatWindow::get (w)->updatePreviews ();
	}
	else if (event->xproperty.atom == mKdeSlideAtom)
	{
	    w = screen->findWindow (event->xproperty.window);
	    if (w)
		KDECompatWindow::get (w)->updateSlidePosition ();
	}
	break;
    }
}

bool
KDECompatWindow::damageRect (bool           initial,
			     const CompRect &rect)
{
    Bool       status;

    KDECOMPAT_SCREEN (screen);

    if (mIsPreview && ks->optionGetPlasmaThumbnails ())
    {
	foreach (CompWindow *cw, screen->windows ())
	{
	    KDECompatWindow *kdw = KDECompatWindow::get (cw);
	
	    foreach (const Thumb& thumb, kdw->mPreviews)
	    {
	        if (thumb.id != window->id ())
		    continue;
		
		CompRect rect (thumb.thumb.x () + cw->x (),
			       thumb.thumb.y () + cw->y (),
			       thumb.thumb.width (),
			       thumb.thumb.height ());

		ks->cScreen->damageRegion (rect);
	    }
        }
    }

    if (initial && ks->optionGetSlidingPopups ())
	startSlideAnimation (TRUE);
    
    status = cWindow->damageRect (initial, rect);

    return status;
}

void
KDECompatScreen::advertiseSupport (Atom atom,
				   bool enable)
{
    if (enable)
    {
	unsigned char value = 0;

	XChangeProperty (screen->dpy (), screen->root (), atom,
			 mKdePreviewAtom, 8, PropModeReplace, &value, 1);
    }
    else
    {
	XDeleteProperty (screen->dpy (), screen->root (), atom);
    }
}

void
KDECompatScreen::optionChanged (CompOption                *option,
				KdecompatOptions::Options num)
{
    if (num == KdecompatOptions::PlasmaThumbnails)
	advertiseSupport (mKdePreviewAtom, option->value ().b ());
    else if (num == KdecompatOptions::SlidingPopups)
	advertiseSupport (mKdeSlideAtom, option->value (). b ());
}


KDECompatScreen::KDECompatScreen (CompScreen *screen) :
    PluginClassHandler <KDECompatScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    mKdePreviewAtom (XInternAtom (screen->dpy (), "_KDE_WINDOW_PREVIEW", 0))
{
    ScreenInterface::setHandler (screen);
    
    advertiseSupport (mKdePreviewAtom, optionGetPlasmaThumbnails ());
    advertiseSupport (mKdeSlideAtom, optionGetSlidingPopups ());
    optionSetPlasmaThumbnailsNotify (
	boost::bind (&KDECompatScreen::optionChanged, this, _1, _2));
}

KDECompatScreen::~KDECompatScreen ()
{
    advertiseSupport (mKdePreviewAtom, FALSE);
    advertiseSupport (mKdeSlideAtom, FALSE);
}

KDECompatWindow::KDECompatWindow (CompWindow *window) :
    PluginClassHandler <KDECompatWindow, CompWindow> (window),
    window (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window)),
    mIsPreview (false),
    mSlideData (NULL),
    mDestroyCnt (0),
    mUnmapCnt (0)
{
    CompositeWindowInterface::setHandler (cWindow);
    GLWindowInterface::setHandler (gWindow);
}

KDECompatWindow::~KDECompatWindow ()
{
    stopCloseAnimation ();
    
    if (mSlideData)
	delete mSlideData;
}
    
bool
KDECompatPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
        return false;

    return true;
}
