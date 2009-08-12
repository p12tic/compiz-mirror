/**
 * Compiz Opacify 
 *
 * opacify.cpp
 *
 * Copyright (c) 2006 Kristian Lyngstøl <kristian@beryl-project.org>
 * Ported to Compiz and BCOP usage by Danny Baumann <maniac@beryl-project.org>
 * Ported to Compiz 0.9 by Sam Spilsbury <smspillaz@gmail.com>
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
 *
 * Opacify increases opacity on targeted windows and reduces it on
 * blocking windows, making whatever window you are targeting easily
 * visible. 
 *
 */

#include "opacify.h"

COMPIZ_PLUGIN_20090315 (opacify, OpacifyPluginVTable);

/* Core opacify functions. These do the real work. ---------------------*/

/* Sets the real opacity and damages the window if actual opacity and 
 * requested opacity differs. */

void
OpacifyWindow::setOpacity (int fOpacity)
{
    if (opacified || (gWindow->paintAttrib ().opacity != opacity))
	cWindow->addDamage ();

    opacified = true;
    opacity = fOpacity;
}

/* Resets the Window to the original opacity if it still exists.
*/

void
OpacifyScreen::resetOpacity (Window  id)
{
    CompWindow *w;

    w = screen->findWindow (id);
    if (!w)
	return;

    OPACIFY_WINDOW (w);

    ow->opacified = false;
    ow->cWindow->addDamage ();
}

/* Resets the opacity of windows on the passive list.
*/
void
OpacifyScreen::clearPassive ()
{
    for (int i = 0; i < passiveNum; i++)
	resetOpacity (passive[i]);

    passiveNum = 0;
}

/* Dim an (inactive) window. Place it on the passive list and
 * update passiveNum. Then change the opacity.
 */

void
OpacifyWindow::dim ()
{
    OPACIFY_SCREEN (screen);

    if (os->passiveNum >= MAX_WINDOWS - 1)
    {
#warning fixme: use std::vector <>
	compLogMessage ("opacify", CompLogLevelWarn,
			"Trying to store information "
			"about too many windows, or you hit a bug.\nIf "
			"you don't have around %d windows blocking the "
			"currently targeted window, please report this.",
			MAX_WINDOWS);
	return;
    }

    os->passive[os->passiveNum++] = window->id ();
    setOpacity (MIN (OPAQUE * os->optionGetPassiveOpacity () / 100,
		     gWindow->paintAttrib ().opacity));
}

/* Walk through all windows, skip until we've passed the active
 * window, skip if it's invisible, hidden or minimized, skip if
 * it's not a window type we're looking for. 
 * Dim it if it intersects. 
 *
 * Returns number of changed windows.
 */

int
OpacifyScreen::passiveWindows (CompRegion     fRegion)
{
    bool       flag = false;
    int        i = 0;

    foreach (CompWindow *w, screen->windows ())
    {
	if (w->id () == active)
	{
	    flag = true;
	    continue;
	}
	if (!flag)
	    continue;
	if (!optionGetWindowMatch ().evaluate (w))
	    continue;
	if (w->invisible () || w->minimized ())
	    continue;

	intersect = w->region ().intersected (fRegion);
	if (!intersect.isEmpty ())
	{
	    OpacifyWindow::get (w)->dim ();
	    i++;
	}
    }

    return i;
}

/* Check if we switched active window, reset the old passive windows
 * if we did. If we have an active window and switched: reset that too.
 * If we have a window (w is true), update the active id and
 * passive list. justMoved is to make sure we recalculate opacity after
 * moving. We can't reset before moving because if we're using a delay
 * and the window being moved is not the active but overlapping, it will
 * be reset, which would conflict with move's opacity change. 
 */
void
OpacifyWindow::handleEnter ()
{
    OPACIFY_SCREEN (screen);

    if (screen->otherGrabExist (0))
    {
	if (!screen->otherGrabExist ( "move", 0))
	{
	    os->justMoved = TRUE;
	    return;
	}

	os->clearPassive ();
	os->resetOpacity (os->active);
	os->active = 0;
	return;
    }

    if (!window || os->active != window->id () || os->justMoved)
    {
	os->justMoved = false;
	os->clearPassive ();
	os->resetOpacity (os->active);
	os->active = 0;
    }

    if (!window)
	return;

    if (window->id () != os->active && !window->shaded () &&
	os->optionGetWindowMatch ().evaluate (window))
    {
	int num;

	os->active = window->id ();
	num = os->passiveWindows (window->region ());

	if (num || os->optionGetOnlyIfBlock ())
	    setOpacity (MAX (OPAQUE * os->optionGetActiveOpacity () / 100,
				gWindow->paintAttrib ().opacity));
    }
}

/* Timeout-time! Unset the timeout handler, make sure we're on the same
 * screen, handle the event.
 */
bool
OpacifyScreen::handleTimeout ()
{

    OpacifyWindow::get (newActive)->handleEnter ();

    return false;
}

/* Checks whether we should delay or not.
 * Returns true if immediate execution.
 */
bool
OpacifyScreen::checkDelay ()
{
    if (optionGetFocusInstant () && newActive && 
	(newActive->id () == screen->activeWindow ()))
	return true;
    if (!optionGetTimeout ())
	return TRUE;
    if (!newActive || (newActive->id () == screen->root ()))
	return FALSE;
    if (newActive->type () & (CompWindowTypeDesktopMask |
			      CompWindowTypeDockMask))
    {
	return FALSE;
    }
    if (optionGetNoDelayChange () && passiveNum)
	return TRUE;

    return FALSE;
}

bool
OpacifyWindow::glPaint (const GLWindowPaintAttrib &attrib,
			const GLMatrix		  &transform,
			const CompRegion	  &region,
			unsigned int		  mask)
{
    bool       status;

    if (opacified)
    {
	GLWindowPaintAttrib wAttrib = attrib;

	wAttrib.opacity = opacity;

	return gWindow->glPaint (wAttrib, transform, region, mask);
    }
    else
    {
	return gWindow->glPaint (attrib, transform, region, mask);
    }

    return status;
}

/* Takes the inital event. 
 * If we were configured, recalculate the opacify-windows if 
 * it was our window. 
 * If a window was entered: call upon handle_timeout after od->timeout 
 * micro seconds, or directly if od->timeout is 0 (no delay).
 *
 */

void
OpacifyScreen::handleEvent (XEvent *event)
{

    screen->handleEvent (event);

    if (!isToggle)
	return;

    switch (event->type) {
    case EnterNotify:
	Window id;

	id = event->xcrossing.window;
	    newActive = screen->findTopLevelWindow (id);

	if (timeoutHandle.active ())
	    timeoutHandle.stop ();

	if (checkDelay ())
	    handleTimeout ();
	else
	    timeoutHandle.start ();
     	break;
    case ConfigureNotify:

	if (active != event->xconfigure.window)
	    break;

	clearPassive ();
	if (active)
	{
	    CompWindow *w;

	    w = screen->findWindow (active);
	    if (w)
		passiveWindows (w->region ());
	}
     	break;
    default:
	break;
    }
}

/* Toggle opacify on/off. We are in Display-context, make sure we handle all
 * screens. 
 */

bool
OpacifyScreen::toggle (CompAction         *action,
		       CompAction::State  state,
		       CompOption::Vector options)
{
    isToggle = !isToggle;
    if (!isToggle && optionGetToggleReset ())
    {
	if (active)
	{
	    clearPassive ();
	    resetOpacity (active);
	    active = 0;
	}
    }

    return TRUE;
}

/* Configuration, initialization, boring stuff. ----------------------- */

/** This is called when an option changes. The only option we are looking for
 *  here is 'init_toggle' so when that changes, we adjust our internal values
 *  appropriately
 */

void
OpacifyScreen::optionChanged (CompOption              *option,
			      OpacifyOptions::Options num)
{
    switch (num) {
    case OpacifyOptions::InitToggle:
	isToggle = option->value ().b ();
	break;
    default:
	break;
    }
}

/** Constructor for OpacifyWindow. This is called whenever a new window 
 *  is created and we set our custom variables to it and also register to 
 *  paint this window
 */

OpacifyWindow::OpacifyWindow (CompWindow *window) :
    PluginClassHandler <OpacifyWindow, CompWindow> (window),
    window (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window)),
    opacified (false),
    opacity (100)
{
    GLWindowInterface::setHandler (gWindow);
}

/** Constructor for OpacifyWindow. This is called whenever a new window 
 *  is created and we set our custom variables to it and also register to
 *  handle X.org events when they come through
 */

OpacifyScreen::OpacifyScreen (CompScreen *screen) :
    PluginClassHandler <OpacifyScreen, CompScreen> (screen),
    isToggle (false),
    newActive (NULL),
    active (screen->activeWindow ()),
    intersect (emptyRegion),
    passiveNum (0),
    justMoved (false)
{
    ScreenInterface::setHandler (screen);

    timeoutHandle.setTimes (optionGetTimeout (), optionGetTimeout () * 1.2);
    timeoutHandle.setCallback (boost::bind (&OpacifyScreen::handleTimeout,
									 this));

    optionSetToggleKeyInitiate (boost::bind (&OpacifyScreen::toggle, this, _1,
								       _2, _3));
    optionSetInitToggleNotify (boost::bind (&OpacifyScreen::optionChanged,
								 this, _1, _2));
}

bool
OpacifyPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;
    if (!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI))
	return false;
    if (!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	return false;

    return true;
}
