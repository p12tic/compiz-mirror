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

bool
KDECompatWindow::glPaint (const GLWindowPaintAttrib &attrib,
			  const GLMatrix	    &transform,
			  const CompRegion	    &region,
			  unsigned int		    mask)
{
    Bool         status;

    KDECOMPAT_SCREEN (screen);
    
    status = gWindow->glPaint (attrib, transform, region, mask);

    if (!ks->optionGetPlasmaThumbnails () ||
	mPreviews.empty ()               ||
	!window->mapNum ()                ||
	(mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK))
    {
	return status;
    }

    foreach (Thumb *thumb, mPreviews)
    {
	CompWindow   *tw = screen->findWindow (thumb->id);
	CompRect     &rect = thumb->thumb;
	unsigned int paintMask = mask | PAINT_WINDOW_TRANSFORMED_MASK;
	float        xScale = 1.0f, yScale = 1.0f, xTranslate, yTranslate;
	GLTexture   *icon = NULL;

	if (!tw)
	    continue;

	xTranslate = rect.x () + window->x () - tw->x ();
	yTranslate = rect.y () + window->x () - tw->y ();

	if (GLWindow::get (tw)->textures ().size ())
	{
	    unsigned int width, height;

	    width  = tw->width () + tw->input ().left + tw->input ().right;
	    height = tw->height () + tw->input ().top + tw->input ().bottom;

	    xScale = (float) rect.width () / width;
	    yScale = (float) rect.height () / height;

	    xTranslate += tw->input ().left * xScale;
	    yTranslate += tw->input ().top * yScale;
	}
	else
	{
	    icon = gWindow->getIcon (256, 256);
	    if (!icon)
		icon = ks->gScreen->defaultIcon ();

	    if (icon)
		if (!icon->name ())
		    icon = NULL;

	    if (icon)
	    {
		CompRegion iconReg (tw->x (), tw->y (), tw->width (), tw->height ());
		GLTexture::MatrixList matl;

		paintMask |= PAINT_WINDOW_BLEND_MASK;

		if (icon->width () >= rect.width () || icon->height () >= rect.height ())
		{
		    xScale = (float) rect.width () / icon->width ();
		    yScale = (float) rect.height () / icon->height ();

		    if (xScale < yScale)
			yScale = xScale;
		    else
			xScale = yScale;
		}

		xTranslate += rect.width () / 2 - (icon->width () * xScale / 2);
		yTranslate += rect.height () / 2 - (icon->height () * yScale / 2);

		matl.resize (1);

		matl[0] = icon->matrix ();
		matl[0].x0 -= (tw->x () * icon->matrix ().xx);
		matl[0].y0 -= (tw->y () * icon->matrix ().yy);

		GLWindow::get (tw)->geometry ().reset ();
		
		GLWindow::get (tw)->glAddGeometry (matl, iconReg, infiniteRegion);

		if (!GLWindow::get (tw)->geometry ().vertices)
		    icon = NULL;
	    }
	}

	if (GLWindow::get (tw)->textures ().size () || icon)
	{
	    GLFragment::Attrib fragment (attrib);
	    GLMatrix  wTransform (transform);

	    if (tw->alpha () || fragment.getOpacity () != OPAQUE)
		paintMask |= PAINT_WINDOW_TRANSLUCENT_MASK;

	    wTransform.translate (tw->x (), tw->y (), 0.0f);
	    wTransform.scale (xScale, yScale, 1.0f);
	    wTransform.translate (xTranslate / xScale - tw->x (),
	    			  yTranslate / yScale - tw->y (),
	    			  0.0f);

	    glPushMatrix ();
	    glLoadMatrixf (wTransform.getMatrix ());

	    if (GLWindow::get (tw)->textures ().size ())
		gWindow->glDraw (wTransform, fragment, infiniteRegion, paintMask);
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
    unsigned int    nPreview;

    KDECOMPAT_SCREEN (screen);

    nPreview = 0;

    result = XGetWindowProperty (screen->dpy (), window->id (), ks->mKdePreviewAtom, 0,
				 32768, FALSE, AnyPropertyType, &actual,
				 &format, &n, &left, &propData);

    if (result == Success && propData)
    {
	if (format == 32 && actual == ks->mKdePreviewAtom)
	{
	    long *data    = (long *) propData;
	    unsigned int  nPreview = *data++;

	    if (n == (6 * nPreview + 1))
	    {
		while (!mPreviews.empty ())
		{
		    Thumb *t = mPreviews.front ();
		    delete t;
		    mPreviews.pop_front ();
		}
		
		while (mPreviews.size () < nPreview)
		{
		    if (*data++ != 5)
			break;

		    Thumb *t = new Thumb ();

		    t->id = *data++;
		    t->thumb.setX (*data++);
		    t->thumb.setY (*data++);
		    t->thumb.setWidth (*data++);
		    t->thumb.setHeight (*data++);

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

	kcw->mIsPreview = FALSE;
	foreach (rw, screen->windows ())
	{
	    KDECompatWindow *krw = KDECompatWindow::get (rw);

	    foreach (Thumb *t, krw->mPreviews)
	    {
		if (t->id == cw->id ())
		{
		    kcw->mIsPreview = TRUE;
		    break;
		}
	    }

	    if (kcw->mIsPreview)
		break;
	}
    }
}

void
KDECompatScreen::handleEvent (XEvent *event)
{    
    screen->handleEvent (event);

    switch (event->type) {
    case PropertyNotify:
	if (event->xproperty.atom == mKdePreviewAtom)
	{
	    CompWindow *w;

	    w = screen->findWindow (event->xproperty.window);
	    if (w)
		KDECompatWindow::get (w)->updatePreviews ();
	}
	break;
    }
}

bool
KDECompatWindow::damageRect (bool initial,
			     const CompRect &rect)
{
    Bool       status;

    KDECOMPAT_SCREEN (screen);

    if (mIsPreview && ks->optionGetPlasmaThumbnails ())
    {
	CompRegion reg;
	
	foreach (CompWindow *cw, screen->windows ())
	{
	    KDECompatWindow *kdw = KDECompatWindow::get (cw);
	
	    foreach (Thumb *thumb, kdw->mPreviews)
	    {
	        CompRect region;

	        if (thumb->id != window->id ())
		    continue;
		
	        region.setGeometry (thumb->thumb.x () + cw->x (),
				    thumb->thumb.y () + cw->y (),
				    thumb->thumb.width (),
				    thumb->thumb.height ());
	        		 
	        ks->cScreen->damageRegion (region);
	    }
        }
    }
    
    status = cWindow->damageRect (initial, rect);

    return status;
}

void
KDECompatScreen::advertiseThumbSupport (bool supportThumbs)
{
    if (supportThumbs)
    {
	unsigned char value = 0;

	XChangeProperty (screen->dpy (), screen->root (), mKdePreviewAtom,
			 mKdePreviewAtom, 8, PropModeReplace, &value, 1);
    }
    else
    {
	XDeleteProperty (screen->dpy (), screen->root (), mKdePreviewAtom);
    }
}

void
KDECompatScreen::optionChanged (CompOption                *option,
				KdecompatOptions::Options num)
{
    if (num == KdecompatOptions::PlasmaThumbnails)
	advertiseThumbSupport (option->value ().b ());
}


KDECompatScreen::KDECompatScreen (CompScreen *screen) :
    PluginClassHandler <KDECompatScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    mKdePreviewAtom (XInternAtom (screen->dpy (), "_KDE_WINDOW_PREVIEW", 0))
{
    ScreenInterface::setHandler (screen);
    
    advertiseThumbSupport (optionGetPlasmaThumbnails ());
    optionSetPlasmaThumbnailsNotify (boost::bind (&KDECompatScreen::optionChanged, this, _1, _2));
}

KDECompatScreen::~KDECompatScreen ()
{
}

KDECompatWindow::KDECompatWindow (CompWindow *window) :
    PluginClassHandler <KDECompatWindow, CompWindow> (window),
    window (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window)),
    mIsPreview (false)
{
    CompositeWindowInterface::setHandler (cWindow);
    GLWindowInterface::setHandler (gWindow);
}

KDECompatWindow::~KDECompatWindow ()
{
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
