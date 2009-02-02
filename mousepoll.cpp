/*
 *
 * Compiz mouse position polling plugin
 *
 * mousepoll.c
 *
 * Copyright : (C) 2008 by Dennis Kasprzyk
 * E-mail    : onestone@opencompositing.org
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
 */

#include "private.h"

COMPIZ_PLUGIN_20081216 (mousepoll, MousepollPluginVTable);

bool
MousepollScreen::getMousePosition ()
{
    Window       root_return;
    Window       child_return;
    int          rootX, rootY;
    int          winX, winY;
    int		 w = screen->width (), h = screen->height ();
    unsigned int maskReturn;
    bool         status;

    status = XQueryPointer (screen->dpy (), screen->root (),
			    &root_return, &child_return,
			    &rootX, &rootY, &winX, &winY, &maskReturn);

    if (!status || rootX > w || rootY > h ||
	screen->root () != root_return)
	return false;

    if ((rootX != posX || rootY != posY))
    {
	posX = rootX;
	posY = rootY;
	return true;
    }
    return false;
}

bool
MousepollScreen::updatePosition ()
{
    if (pollers.empty ())
	return false;

    if (getMousePosition ())
    {
	std::list<MousePoller *>::iterator it;
	for (it = pollers.begin (); it != pollers.end (); it++)
	{
	    MousePoller *poller = *it;
	    poller->mPoint.setX(posX);
	    poller->mPoint.setY(posY);
	    poller->mCallback (posX, posY);
	}
    }

    return true;
}

bool
MousepollScreen::addTimer (MousePoller *poller)
{
    bool start   = pollers.empty ();
    std::list<MousePoller *>::iterator it;

    it = std::find (pollers.begin (), pollers.end (), poller);

    if (it != pollers.end ())
	return false;

    pollers.insert (it, poller);

    if (start)
    {
	getMousePosition ();
	timer.start ();
    }

    return true;
}

void
MousepollScreen::removeTimer (MousePoller *poller)
{
    std::list<MousePoller *>::iterator it;

    it = std::find (pollers.begin(), pollers.end (), poller);

    if (it == pollers.end ())
	return;

    pollers.erase (it);

    if (pollers.empty ())
    {
	timer.stop ();
    }
}

void
MousePoller::start ()
{
    MOUSEPOLL_SCREEN  (screen);

    if (!ms)
    {
	compLogMessage ("mousepoll",
			CompLogLevelWarn,
			"Plugin version mismatch, can't start mouse poller");

	return;
    }

    ms->addTimer (this);

    mActive = true;
}

void
MousePoller::stop ()
{
    MOUSEPOLL_SCREEN (screen);

    if (!ms)
    {
	compLogMessage ("mousepoll",
			CompLogLevelWarn,
			"Plugin version mismatch, can't start mouse poller");

	return;
    }

    mActive = false;

    ms->removeTimer (this);
}

CompPoint
MousePoller::getCurrentPosition ()
{
    MOUSEPOLL_SCREEN (screen);

    CompPoint p;

    if (!ms)
    {
	compLogMessage ("mousepoll",
			CompLogLevelWarn,
			"Plugin version mismatch, can't start mouse poller");

	p.setX (0);
	p.setY (0);

	return p;
    }

    ms->getMousePosition ();

    p.setX (ms->posX);
    p.setY (ms->posY);

    return p;
}

CompPoint
MousePoller::getPosition ()
{
    return mPoint;
}

MousePoller::MousePoller () :
    mActive (false),
    mPoint (0, 0)
{
}

MousePoller::~MousePoller ()
{
    if (mActive)
	stop ();
}
static const CompMetadata::OptionInfo mousepollOptionInfo[] = {
    { "mouse_poll_interval", "int",
      "<min>1</min><max>500</max>", 0, 0 }
};

CompOption::Vector &
MousepollScreen::getOptions ()
{
    return opt;
}
 
bool
MousepollScreen::setOption (const char        *name,
		       	    CompOption::Value &value)
{
    CompOption *o;
    bool status;
    unsigned int index;
 
    o = CompOption::findOption (opt, name, &index);
    if (!o)
	return false;
 
    switch (index) {
	case MP_OPTION_MOUSE_POLL_INTERVAL:
	    status = o->set (value);

	    if (timer.active ())
		timer.start
		    (o->value ().i () / 2,
		     o->value ().i ());
	    return status;
    default:
	return CompOption::setOption (*o, value);
    }
 
    return false;
}

MousepollScreen::MousepollScreen (CompScreen *screen) :
    PrivateHandler <MousepollScreen, CompScreen, COMPIZ_MOUSEPOLL_ABI> (screen),
    opt (MP_OPTION_NUM),
    posX (0),
    posY (0)    
{
    if (!mousepollVTable->getMetadata ()->initOptions (mousepollOptionInfo,
						       MP_OPTION_NUM, opt))
    {
	setFailed ();
	return;
    }
}

bool
MousepollPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    getMetadata ()->addFromOptionInfo (mousepollOptionInfo, MP_OPTION_NUM);
    getMetadata ()->addFromFile (name ());

    return true;
}
