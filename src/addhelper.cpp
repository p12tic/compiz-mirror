/**
 * Compiz ADD Helper. Makes it easier to concentrate.
 *
 * addhelper.cpp
 *
 * Copyright (c) 2007 Kristian Lyngstøl <kristian@beryl-project.org>
 * Ported and highly modified by Patrick Niklaus <marex@beryl-project.org>
 * Ported to compiz 0.9 by Sam Spilsbury <smspillaz@gmail.com>
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
 * This plugin provides a toggle-feature that dims all but the active
 * window. This makes it easier for people with lousy concentration
 * to focus. Like me.
 * 
 * Please note any major changes to the code in this header with who you
 * are and what you did. 
 *
 */

#include "addhelper.h"

COMPIZ_PLUGIN_20090315 (addhelper, AddPluginVTable);

/* Walk through all windows of the screen and adjust them if they
 * are not the active window. If reset is true, this will reset
 * the windows, including the active. Otherwise, it will dim 
 * and reset the active. 
 */

void
AddScreen::walkWindows ()
{
    foreach (CompWindow *w, screen->windows ())
    {
	ADD_WINDOW (w);

	if (!aw->dim)
	    aw->cWindow->addDamage ();

	aw->dim = false;

	if (!isToggle)
	continue;

	if (w->id () == screen->activeWindow ())
	continue;

	if (w->invisible () || w->destroyed () ||
	    !w->isMapped () || w->minimized ())
	continue;

	if (!optionGetWindowTypes ().evaluate (w))
	continue;

	aw->cWindow->addDamage ();

	aw->dim = true;
    }
}

/* Checks if the window is dimmed and, if so, paints it with the modified
 * paint attributes.
 */
bool
AddWindow::glPaint (const GLWindowPaintAttrib &attrib,
     		    const GLMatrix            &transform,
		    const CompRegion          &region,
		    unsigned int              mask)
{
    ADD_SCREEN (screen);

    if (dim)
    {
	/* copy the paint attribute */
	GLWindowPaintAttrib wAttrib = attrib;

	/* applies the lowest value */
	wAttrib.opacity = (MIN (attrib.opacity, as->opacity));
	wAttrib.brightness = (MIN (attrib.brightness, as->brightness));
	wAttrib.saturation = (MIN (attrib.saturation, as->saturation));

	/* continue painting with the modified attribute */
	return gWindow->glPaint (wAttrib, transform, region, mask);
    }
    else
    {
	/* the window is not dimmed, so it's painted normal */
	return gWindow->glPaint (attrib, transform, region, mask);
    }
}

/* Takes the inital event. 
 * This checks for focus change and acts on it.
 */
void
AddScreen::handleEvent (XEvent *event)
{
    Window active = screen->activeWindow ();

    screen->handleEvent (event);

    if (active != screen->activeWindow () &&
	isToggle)
	walkWindows ();
}

/* Configuration, initialization, boring stuff. ----------------------- */

/* Takes the action and toggles us.
*/

bool
AddScreen::toggle (CompAction         *action,
		   CompAction::State  state,
		   CompOption::Vector options)
{
    CompOption::Vector opts = toggleState.getReadTemplate ();
    CompOption::Value  v;

    isToggle = !isToggle;
    if (isToggle)
    {
	walkWindows ();
	foreach (CompWindow *w, screen->windows ())
	{
	    ADD_WINDOW (w);
	    aw->gWindow->glPaintSetEnabled (aw, true);
	}
	screen->handleEventSetEnabled (this, true);
    }
    else
    {
	foreach (CompWindow *w, screen->windows ())
	{
	    ADD_WINDOW (w);
	    aw->gWindow->glPaintSetEnabled (aw, false);
	    aw->cWindow->addDamage ();
	}
	screen->handleEventSetEnabled (this, false);
    }

    v = CompOption::Value (isToggle);
    opts.at (0).set (v);
    
    toggleState.updateProperty (screen->root (), opts, false, XA_CARDINAL);

    return true;

}

void
AddScreen::optionChanged (CompOption                *options,
			  AddhelperOptions::Options num)
{
    switch (num)
    {
	case AddhelperOptions::Brightness:
	    brightness = (optionGetBrightness () * 0xffff) / 100;
	    break;
	case AddhelperOptions::Saturation:
	    saturation = (optionGetSaturation () * 0xffff) / 100;
	    break;
	case AddhelperOptions::Opacity:
	    opacity = (optionGetOpacity () * 0xffff) / 100;
	    break;
	case AddhelperOptions::Ononinit: // <- Turn AddHelper on on initiation
	    isToggle = optionGetOnoninit ();
	    if (isToggle)
	    {
		walkWindows ();
		foreach (CompWindow *w, screen->windows ())
		{
		    ADD_WINDOW (w);
		    aw->gWindow->glPaintSetEnabled (aw, true);
		}
		screen->handleEventSetEnabled (this, true);
	    }
	    else
	    {
		foreach (CompWindow *w, screen->windows ())
		{
		    ADD_WINDOW (w);
		    aw->gWindow->glPaintSetEnabled (aw, false);
		}
		screen->handleEventSetEnabled (this, false);
		}
	    break;
	default:
	    break;
    }
}

bool
AddScreen::checkStateTimeout ()
{
    CompOption::Vector atomTemplate;
    CompOption::Vector currentToggleState;
    CompOption::Value  v;
    
    atomTemplate.resize (1);
    atomTemplate.at (0).setName ("toggled", CompOption::TypeBool);
    
    toggleState = PropertyWriter ("_COMPIZ_ADDHELPER_TOGGLE_STATE", atomTemplate);
    
    /* Attempt to read the property on the root window
     * from where we may have previously set data
     */
     
    currentToggleState = toggleState.readProperty (screen->root ());
    
    if (!currentToggleState.empty ())
    {
	isToggle = currentToggleState.at (0).value ().b ();
	if (isToggle)
	{
	    walkWindows ();
	    foreach (CompWindow *w, screen->windows ())
	    {
		ADD_WINDOW (w);
		aw->gWindow->glPaintSetEnabled (aw, true);
	    }
	    screen->handleEventSetEnabled (this, true);
	}
	else
	{
	    foreach (CompWindow *w, screen->windows ())
	    {
		ADD_WINDOW (w);
		aw->gWindow->glPaintSetEnabled (aw, false);
	    }
	    screen->handleEventSetEnabled (this, false);
	}

    }
    
    return false;
}

AddWindow::AddWindow (CompWindow *window) :
    PluginClassHandler <AddWindow, CompWindow> (window),
    window (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window)),
    dim (false)
{
    ADD_SCREEN (screen);

    GLWindowInterface::setHandler (gWindow, false);

    if (as->isToggle &&
	window->id () != screen->activeWindow () &&
	!window->overrideRedirect ())
	dim = true;
}

AddWindow::~AddWindow ()
{
    if (dim)
	cWindow->addDamage ();
}

AddScreen::AddScreen (CompScreen *screen) :
    PluginClassHandler <AddScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    opacity ((optionGetOpacity () * 0xffff) / 100),
    brightness ((optionGetBrightness () * 0xffff) / 100),
    saturation ((optionGetSaturation () * 0xffff) / 100),
    isToggle (optionGetOnoninit ())
{
    ScreenInterface::setHandler (screen, false);

    checkStateTimer.setTimes (0, 0);
    checkStateTimer.setCallback (boost::bind (&AddScreen::checkStateTimeout,
    					      this));
    checkStateTimer.start ();

    optionSetToggleKeyInitiate (boost::bind (&AddScreen::toggle, this,
						_1, _2, _3));

    optionSetBrightnessNotify (boost::bind (&AddScreen::optionChanged, this, _1,
					    _2));
    optionSetSaturationNotify (boost::bind (&AddScreen::optionChanged, this, _1,
					    _2));
    optionSetOpacityNotify (boost::bind (&AddScreen::optionChanged, this, _1,
					    _2));
    optionSetOnoninitNotify (boost::bind (&AddScreen::optionChanged, this, _1,
					    _2));
}
    
bool
AddPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	return false;

    return true;
}
