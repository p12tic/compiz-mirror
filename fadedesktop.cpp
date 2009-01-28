/**
 *
 * Compiz fade to desktop plugin
 *
 * fadedesktop.cpp
 *
 * Copyright (c) 2006 Robert Carr <racarr@beryl-project.org>
 * 		 2007 Danny Baumann <maniac@beryl-project.org>
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

#include "fadedesktop.h"

COMPIZ_PLUGIN_20081216 (fadedesktop, FadedesktopPluginVTable);

static void
fadeDesktopActivateEvent (bool activating)
{
    CompOption::Vector o;

    o.push_back (CompOption ("root", CompOption::TypeInt));
    o.push_back (CompOption ("active", CompOption::TypeBool));

    o[0].value (). set ((int) screen->root ());
    o[1].value (). set ((bool) activating);
}

static bool
isFDWin (CompWindow *w)
{
    CompMatch match (FadedesktopScreen::get (screen)->optionGetWindowMatch ());
    if (w->overrideRedirect ())
	return false;

    if (w->grabbed ())
	return false;

    if (!w->managed ())
	return false;

    if (w->wmType () & (CompWindowTypeDesktopMask |
		        CompWindowTypeDockMask))
	return false;

    if (w->state () & CompWindowStateSkipPagerMask)
	return false;

    if (!match.evaluate (w))
	return false;

    return true;
}

void 
FadedesktopScreen::preparePaint (int msSinceLastPaint)
{
    bool doFade;

    fadeTime -= msSinceLastPaint;
    if (fadeTime < 0)
	fadeTime = 0;

    if ((state == FD_STATE_OUT) || (state == FD_STATE_IN))
    {
	foreach (CompWindow *w, screen->windows ())
	{
	    FD_WINDOW (w);

	    if (state == FD_STATE_OUT)
		doFade = fw->fading && w->inShowDesktopMode ();
	    else
		doFade = fw->fading && !w->inShowDesktopMode ();

	    if (doFade)
	    {
		fw->opacity = fw->cWindow->opacity () *
		    (float)((state == FD_STATE_OUT) ? fadeTime : 
		    optionGetFadetime () - fadeTime) / 
		    (float)optionGetFadetime();
	    }
	}
    }

    cScreen->preparePaint (msSinceLastPaint);
}

void
FadedesktopScreen::donePaint ()
{
    if ((state == FD_STATE_OUT) || (state == FD_STATE_IN))
    {
	if (fadeTime <= 0)
	{
	    bool isStillSD = false;

	    foreach (CompWindow *w, screen->windows ())
	    {
		FD_WINDOW (w);

		if (fw->fading)
		{
		    if (state == FD_STATE_OUT)
		    {
			w->hide ();
			fw->isHidden = true;
		    }
		    fw->fading = false;
		}
		if (w->inShowDesktopMode ())
		    isStillSD = true;
	    }

	    if ((state == FD_STATE_OUT) || isStillSD)
		state = FD_STATE_ON;
	    else
		state = FD_STATE_OFF;
	    fadeDesktopActivateEvent (false);
	}
	else
	    cScreen->damageScreen ();
    }

    cScreen->donePaint ();
}

bool
FadedesktopWindow::glPaint (const GLWindowPaintAttrib &attrib,
		      	    const GLMatrix &transform,
		      	    const CompRegion &region,
		      	    unsigned int mask)
{
    bool status;

    if (fading || isHidden)
    {
	GLWindowPaintAttrib wAttrib = attrib;
	wAttrib.opacity = opacity;

	status = gWindow->glPaint (wAttrib, transform, region, mask);
    }
    else
	status = gWindow->glPaint (attrib, transform, region, mask);

    return status;
}

void
FadedesktopScreen::enterShowDesktopMode ()
{
    if ((state == FD_STATE_OFF) || (state == FD_STATE_IN))
    {
	if (state == FD_STATE_OFF)
	    fadeDesktopActivateEvent (true);

	state = FD_STATE_OUT;
	fadeTime = optionGetFadetime() - fadeTime;

	foreach (CompWindow *w, screen->windows ())
	{
	    if (isFDWin(w))
	    {
		FD_WINDOW(w);

		fw->fading = true;
		w->setShowDesktopMode (true);
		fw->opacity = fw->cWindow->opacity ();
	    }
	}

	cScreen->damageScreen ();
    }

    screen->enterShowDesktopMode ();
}

void
FadedesktopScreen::leaveShowDesktopMode (CompWindow *w)
{
    if (state != FD_STATE_OFF)
    {
	if (state != FD_STATE_IN)
	{
	    if (state == FD_STATE_ON)
		fadeDesktopActivateEvent (true);

	    state = FD_STATE_IN;
	    fadeTime = optionGetFadetime() - fadeTime;
	}

	foreach (CompWindow *cw, screen->windows ())
	{
	    if (w && (w->id () != cw->id ()))
		continue;

	    FD_WINDOW(cw);

	    if (fw->isHidden)
	    {
		cw->setShowDesktopMode (false);
		cw->show ();
		fw->isHidden = false;
		fw->fading = false;
	    }
	    else if (fw->fading)
	    {
		cw->setShowDesktopMode (false);
	    }
	}

	cScreen->damageScreen ();
    }

    screen->leaveShowDesktopMode (w);
}

FadedesktopScreen::FadedesktopScreen (CompScreen *screen) :
    PrivateHandler <FadedesktopScreen, CompScreen> (screen),
    FadedesktopOptions (fadedesktopVTable->getMetadata ()),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    state (FD_STATE_OFF),
    fadeTime (0)
{
    ScreenInterface::setHandler (screen);
    CompositeScreenInterface::setHandler (cScreen);
    GLScreenInterface::setHandler (gScreen);
}

FadedesktopWindow::FadedesktopWindow (CompWindow *window) :
    PrivateHandler <FadedesktopWindow, CompWindow> (window),
    window (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window)),
    fading (false),
    isHidden (false),
    opacity (OPAQUE)
{
    WindowInterface::setHandler (window);
    GLWindowInterface::setHandler (gWindow);
}

bool
FadedesktopPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	return false;

    return true;
}
