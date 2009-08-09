/*
 *
 * Compiz widget handling plugin
 *
 * widget.c
 *
 * Copyright : (C) 2007 by Danny Baumann
 * E-mail    : maniac@opencompositing.org
 *
 * Ported to Compiz 0.9 by:
 * Copyright : (C) 2008 Sam Spilsbury
 * E-mail    : smspillaz@gmail.com
 *
 * Idea based on widget.c:
 * Copyright : (C) 2006 Quinn Storm
 * E-mail    : livinglatexkali@gmail.com
 *
 * Copyright : (C) 2007 Mike Dransfield
 * E-mail    : mike@blueroot.co.uk
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

#include "widget.h"

class WidgetExp :
    public CompMatch::Expression
{
    public:
	WidgetExp (const CompString &str);

	bool evaluate (CompWindow *w);

	bool value;
};

COMPIZ_PLUGIN_20090315 (widget, WidgetPluginVTable);

static void enableFunctions (WidgetScreen *ws, bool enabled)
{
    ws->cScreen->preparePaintSetEnabled (ws, enabled);
    ws->cScreen->donePaintSetEnabled (ws, enabled);

    foreach (CompWindow *w, screen->windows ())
    {
	WIDGET_WINDOW (w);

	ww->window->focusSetEnabled (ww, enabled);
	ww->gWindow->glPaintSetEnabled (ww, enabled);
    }
}

void
WidgetWindow::updateTreeStatus ()
{
    /* first clear out every reference to our window */
    foreach (CompWindow *win, screen->windows ())
    {
	WIDGET_WINDOW (win);
	if (ww->parentWidget == win)
	    ww->parentWidget = NULL;
    }

    if (window->destroyed ())
	return;

    if (!isWidget)
	return;

    foreach (CompWindow *win, screen->windows ())
    {
	Window clientLeader;

	/*if (window->overrideRedirect ())
	    clientLeader = win->getClientLeader ();
	else*/
	    clientLeader = window->clientLeader ();

	if ((clientLeader == window->clientLeader ()) && (window->id () != win->id ()))
	{
	    WIDGET_WINDOW (win);
	    ww->parentWidget = window;
	}
    }
}

bool
WidgetWindow::updateWidgetStatus ()
{
    Bool f_isWidget, retval, managed;

    WIDGET_SCREEN (screen);

    switch (propertyState) {
    case PropertyWidget:
	f_isWidget = TRUE;
	break;
    case PropertyNoWidget:
	f_isWidget = FALSE;
	break;
    default:
	managed = window->managed () || oldManaged;
	if (!managed || (window->wmType () & CompWindowTypeDesktopMask))
	    f_isWidget = FALSE;
	else
	    f_isWidget = ws->optionGetMatch ().evaluate (window);
	break;
    }

    retval = (!f_isWidget && isWidget) || (f_isWidget && !isWidget);
    isWidget = f_isWidget;

    return retval;
}

bool
WidgetWindow::updateWidgetPropertyState ()
{
    Atom          retType;
    int           format, result;
    unsigned long nitems, remain;
    unsigned char *data = NULL;

    WIDGET_SCREEN (screen);

    result = XGetWindowProperty (screen->dpy (), window->id (), ws->compizWidgetAtom,
				 0, 1L, FALSE, AnyPropertyType, &retType,
				 &format, &nitems, &remain, &data);

    if (result == Success && data)
    {
	if (nitems && format == 32)
	{
	    unsigned long int *retData = (unsigned long int *) data;
	    if (*retData)
		propertyState = PropertyWidget;
	    else
		propertyState = PropertyNoWidget;
	}

	XFree (data);
    }
    else
	propertyState = PropertyNotSet;

    return updateWidgetStatus ();
}

void
WidgetWindow::updateWidgetMapState (bool       map)
{
    if (map && wasUnmapped)
    {
	XMapWindow (screen->dpy (), window->id ());
	window->raise ();
	wasUnmapped = FALSE;
#warning * [FIXME] Need to make managed var setter or wrappable in core
	//window->setManaged (oldManaged); // ???
    }
    else if (!map && !wasUnmapped)
    {
	/* never set ww->wasUnmapped on previously unmapped windows -
	   it might happen that we map windows when entering the
	   widget mode which aren't supposed to be unmapped */
	if (window->isViewable ())
	{
	    XUnmapWindow (screen->dpy (), window->id ());
	    wasUnmapped = TRUE;
	    oldManaged = window->managed ();
	}
    }
}

void
WidgetScreen::setWidgetLayerMapState (bool map)
{
    CompWindow   *highest = NULL;
    unsigned int highestActiveNum = 0;

    foreach (CompWindow *window, screen->windows ())
    {
	WIDGET_WINDOW (window);

	if (!ww->isWidget)
	    continue;

	if (window->activeNum () > highestActiveNum)
	{
	    highest = window;
	    highestActiveNum = window->activeNum ();
	}

	ww->updateWidgetMapState (map);
    }

    if (map && highest)
    {
	if (!lastActiveWindow)
	    lastActiveWindow = screen->activeWindow ();
	highest->moveInputFocusTo ();
    }
    else if (!map)
    {
	CompWindow *w = screen->findWindow (lastActiveWindow);
	lastActiveWindow = None;
	if (w)
	    w->moveInputFocusTo ();
    }
}

bool
WidgetScreen::registerExpHandler ()
{
    screen->matchExpHandlerChanged ();

    return FALSE;
}

/* Check to see if 'false' or 'true' is used in str,
    giving a long value of '0' or '1' */

WidgetExp::WidgetExp (const CompString &str) :
    value (strtol (str.c_str () + 7, NULL, 0))
{
}

/* Check whether the internal value for the match is
   0 or 1 and compare it to what the window is */

bool
WidgetExp::evaluate (CompWindow *w)
{
    WIDGET_WINDOW (w);

    return ((value && ww->isWidget) || (!value && !ww->isWidget));
}

CompMatch::Expression *
WidgetScreen::matchInitExp (CompString &str)
{
    /* Create a new match object */

    if (str.find ("widget=") == 0)
    {
	return new WidgetExp (str.substr (7));
    }
    
    return screen->matchInitExp (str);
}

void
WidgetScreen::matchExpHandlerChanged ()
{
    screen->matchExpHandlerChanged ();

    /* match options are up to date after the call to matchExpHandlerChanged */

    foreach (CompWindow *w, screen->windows ())
    {
	WIDGET_WINDOW (w);
	if (ww->updateWidgetStatus ())
	{
	    bool map;

	    map = !ww->isWidget || (state != StateOff);
	    ww->updateWidgetMapState (map);

	    ww->updateTreeStatus ();

	    screen->matchPropertyChanged (w);
	}
    }
}

bool
WidgetScreen::toggle (CompAction         *action,
		      CompAction::State  aState,
		      CompOption::Vector &options)
{
    switch (state) {
	case StateOn:
	case StateFadeIn:
	    setWidgetLayerMapState (false);
	    fadeTime = 1000.0f * optionGetFadeTime ();
	    state = StateFadeOut;
	    break;
	case StateOff:
	case StateFadeOut:
	    setWidgetLayerMapState (true);
	    fadeTime = 1000.0f * optionGetFadeTime ();
	    state = StateFadeIn;
	    break;
	default:
	    break;
    }

    if (!grabIndex)
	grabIndex = screen->pushGrab (cursor, "widget");

    enableFunctions (this, true);

    cScreen->damageScreen ();

    return TRUE;
}

void
WidgetScreen::endWidgetMode (CompWindow *closedWidget)
{
    CompOption::Vector options;
    CompOption o ("root", CompOption::TypeInt);
    CompOption::Value root = CompOption::Value ((int) screen->root ());

    if (state != StateOn && state != StateFadeIn)
	return;

    if (closedWidget)
    {
	/* end widget mode if the closed widget was the last one */

	WIDGET_WINDOW (closedWidget);
	if (ww->isWidget)
	{
	    foreach (CompWindow *w, screen->windows ())
	    {
		WIDGET_WINDOW (w);
		if (w == closedWidget)
		    continue;
		if (ww->isWidget)
		    return;
	    }
	}
    }


    o.set (root);
    options.push_back (o);

    toggle (NULL, 0, options);
}

void
WidgetScreen::handleEvent (XEvent      *event)
{
    CompWindow *w;

    screen->handleEvent (event);

    switch (event->type)
    {
    case PropertyNotify:
	if (event->xproperty.atom == compizWidgetAtom)
	{
	    w = screen->findWindow (event->xproperty.window);
	    if (w)
	    {
		WIDGET_WINDOW (w);
		if (ww->updateWidgetPropertyState ())
		{
		    bool map;

		    map = !ww->isWidget || (state != StateOff);
		    ww->updateWidgetMapState (map);
		    ww->updateTreeStatus ();
		    screen->matchPropertyChanged (w);
		}
	    }
	}
	else if (event->xproperty.atom == Atoms::wmClientLeader)
	{
	    w = screen->findWindow (event->xproperty.window);
	    if (w)
	    {
		WIDGET_WINDOW (w);

		if (ww->isWidget)
		    ww->updateTreeStatus ();
		else if (ww->parentWidget)
		{
		    WidgetWindow *pww = WidgetWindow::get (ww->parentWidget);
		    pww->updateTreeStatus ();
		}
	    }
	}
	break;
    case ButtonPress:
	/* terminate widget mode if a non-widget window was clicked */
	if (optionGetEndOnClick ())
	{
	    if (state == StateOn)
	    {
		w = screen->findWindow (event->xbutton.window);
		if (w && w->managed ())
		{
		    WIDGET_WINDOW (w);

		    if (!ww->isWidget && !ww->parentWidget)
			endWidgetMode (NULL);
		}
	    }
	}
	break;
    case MapNotify:
	w = screen->findWindow (event->xmap.window);
	if (w)
	{
	    WIDGET_WINDOW (w);

	    ww->updateWidgetStatus ();
	    if (ww->isWidget)
		ww->updateWidgetMapState (state != StateOff);
	}
	break;
    case UnmapNotify:
	w = screen->findWindow (event->xunmap.window);
	if (w)
	{
	    WIDGET_WINDOW (w);
	    ww->updateTreeStatus ();
	    endWidgetMode (w);
	}
	break;
    case DestroyNotify:
	w = screen->findWindow (event->xdestroywindow.window);
	if (w)
	{
	    WIDGET_WINDOW (w);
	    ww->updateTreeStatus ();
	    endWidgetMode (w);
	}
	break;
    }
}

bool
WidgetWindow::updateMatch ()
{
    if (updateWidgetStatus ())
    {
	WIDGET_SCREEN (screen);

	updateTreeStatus ();
	updateWidgetMapState (ws->state != WidgetScreen::StateOff);
	screen->matchPropertyChanged (window);
    }

    return FALSE;
}

bool
WidgetScreen::updateStatus (CompWindow *w)
{
    Window     clientLeader = 0;

    WIDGET_WINDOW (w);

    if (ww->updateWidgetPropertyState ())
	ww->updateWidgetMapState (state != StateOff);

/*    if (w->overrideRedirect ())
	clientLeader = getClientLeader (w); // ???
    else*/
	clientLeader = w->clientLeader ();

    if (ww->isWidget)
    {
	ww->updateTreeStatus ();
    }
    else if (clientLeader)
    {
	CompWindow *lw;

	lw = screen->findWindow (clientLeader);
	if (lw)
	{
	    WidgetWindow *lww;
	    lww = WidgetWindow::get (lw);

	    if (lww->isWidget)
		ww->parentWidget = lw;
	    else if (lww->parentWidget)
		ww->parentWidget = lww->parentWidget;
	}
    }

    return FALSE;
}

void
WidgetScreen::matchPropertyChanged (CompWindow  *w)
{
    WIDGET_WINDOW (w);

    /* one shot timeout which will update the widget status (timer
       is needed because we don't want to call wrapped functions
       recursively) */
    if (!ww->matchUpdate.active ())
	ww->matchUpdate.start (boost::bind (&WidgetWindow::updateMatch, ww),
			       0, 0);

    screen->matchPropertyChanged (w);
}

bool
WidgetWindow::glPaint (const GLWindowPaintAttrib &attrib,
		       const GLMatrix &transform,
		       const CompRegion &region,
		       unsigned int mask)
{
    bool       status;

    WIDGET_SCREEN (screen);

    if (ws->state != WidgetScreen::StateOff)
    {
	GLWindowPaintAttrib wAttrib = attrib;
	float               fadeProgress;

	if (ws->state == WidgetScreen::StateOn)
	    fadeProgress = 1.0f;
	else
	{
	    fadeProgress = ws->optionGetFadeTime ();
	    if (fadeProgress)
		fadeProgress = (float) ws->fadeTime / (1000.0f * fadeProgress);
	    fadeProgress = 1.0f - fadeProgress;
	}

	if (!isWidget && !parentWidget)
	{
	    float progress;

	    if ((ws->state == WidgetScreen::StateFadeIn) || (ws->state == WidgetScreen::StateOn))
		fadeProgress = 1.0f - fadeProgress;

	    progress = ws->optionGetBgSaturation () / 100.0f;
	    progress += (1.0f - progress) * fadeProgress;
	    wAttrib.saturation = (float) wAttrib.saturation * progress;

	    progress = ws->optionGetBgBrightness () / 100.0f;
	    progress += (1.0f - progress) * fadeProgress;

	    wAttrib.brightness = (float) wAttrib.brightness * progress;
	}

	status = gWindow->glPaint (wAttrib, transform, region, mask);
    }
    else
    {
	status = gWindow->glPaint (attrib, transform, region, mask);
    }

    return status;
}

bool
WidgetWindow::focus ()
{
    Bool       status;

    WIDGET_SCREEN (screen);

    /* Don't focus non-widget windows while widget mode is enabled */

    if (ws->state != WidgetScreen::StateOff && !isWidget && !parentWidget)
    {
	status = FALSE;
    }
    else
    {
	status = window->focus ();
    }

    return status;
}

void
WidgetScreen::preparePaint (int msSinceLastPaint)
{
    if ((state == StateFadeIn) || (state == StateFadeOut))
    {
	fadeTime -= msSinceLastPaint;
	fadeTime = MAX (fadeTime, 0);
    }

    cScreen->preparePaint (msSinceLastPaint);
}

void
WidgetScreen::donePaint ()
{
    if ((state == StateFadeIn) || (state == StateFadeOut))
    {
	if (fadeTime)
	    cScreen->damageScreen ();
	else
	{
	    if (grabIndex)
	    {
		screen->removeGrab (grabIndex, NULL);
		grabIndex = 0;
	    }

	    if (state == StateFadeIn)
		state = StateOn;
	    else
		state = StateOff;
	}
    }

    if (state == StateOff)
    {
	cScreen->damageScreen ();
	enableFunctions (this, false);
    }

    cScreen->donePaint ();
}

void
WidgetScreen::optionChanged (CompOption              *option,
			     WidgetOptions::Options  num)
{
    switch (num)
    {
        case WidgetOptions::Match:
	{
	    foreach (CompWindow *w, screen->windows ())
	    {
		WIDGET_WINDOW (w);

		if (ww->updateWidgetStatus ())
		{
		    bool map;

		    map = !ww->isWidget || (state != StateOff);
		    ww->updateWidgetMapState (map);

		    ww->updateTreeStatus ();
		    screen->matchPropertyChanged (w);
		}
	    }
	}
	break;
    default:
	break;
    }
}

/* ------ */

WidgetScreen::WidgetScreen (CompScreen *screen) :
    PluginClassHandler <WidgetScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    lastActiveWindow (None),
    compizWidgetAtom (XInternAtom (screen->dpy (), "_COMPIZ_WIDGET", false)),
    fadeTime (0),
    grabIndex (0),
    cursor (XCreateFontCursor (screen->dpy (), XC_watch))
{
    CompTimer registerTimer;
    ScreenInterface::setHandler (screen, false);
    CompositeScreenInterface::setHandler (cScreen, false);

#define initiateSet(name)						       \
    optionSet##name##Initiate (						       \
		       boost::bind (&WidgetScreen::toggle, this, _1, _2, _3));

    initiateSet (ToggleKey);
    initiateSet (ToggleButton);
    initiateSet (ToggleEdge);

#undef initiateSet

#define optionNotify(name) \
optionSet##name##Notify(boost::bind(&WidgetScreen::optionChanged, this, _1, _2))

    optionNotify(Match);

#undef optionNotify

    /* one shot timeout to which will register the expression handler
       after all screens and windows have been initialized */
    registerTimer.start (boost::bind(&WidgetScreen::registerExpHandler, this),
			 0, 0);

    state = StateOff;


}

WidgetScreen::~WidgetScreen ()
{
    screen->matchExpHandlerChanged ();

    if (cursor)
    {
	XFreeCursor (screen->dpy (), cursor);
    }
}

WidgetWindow::WidgetWindow (CompWindow *window) :
    PluginClassHandler <WidgetWindow, CompWindow> (window),
    window (window),
    gWindow (GLWindow::get (window)),
    isWidget (FALSE),
    wasUnmapped (FALSE),
    oldManaged (FALSE),
    parentWidget (NULL),
    propertyState (PropertyNotSet)
{

    WindowInterface::setHandler (window);
    GLWindowInterface::setHandler (gWindow, false);

    widgetStatusUpdate.start (boost::bind (&WidgetScreen::updateStatus,
				 WidgetScreen::get (screen), window),
			      0, 0);
}

WidgetWindow::~WidgetWindow ()
{
    if (wasUnmapped)
	updateWidgetMapState (true);

    if (matchUpdate.active ())
	matchUpdate.stop ();

    if (widgetStatusUpdate.active ())
	widgetStatusUpdate.stop ();
}

bool
WidgetPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;
    if (!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI))
	return false;
    if (!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	return false;

    return true;
}
