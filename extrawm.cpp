/*
 * Compiz extra WM actions plugins
 * extrawm.cpp
 *
 * Copyright: (C) 2007 Danny Baumann <maniac@beryl-project.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "extrawm.h"

COMPIZ_PLUGIN_20081216 (extrawm, ExtraWMPluginVTable);

void
ExtraWMScreen::addAttentionWindow (CompWindow *w)
{
    std::list <CompWindow *>::iterator it;

    /* check if the window is already there */
    for (it = attentionWindows.begin (); it != attentionWindows.end (); it++)
    {
	if (*it == w)
	    return;
    }

    attentionWindows.push_back (w);
}

void
ExtraWMScreen::removeAttentionWindow (CompWindow *w)
{
    attentionWindows.remove (w);
}

void
ExtraWMScreen::updateAttentionWindow (CompWindow *w)
{
    XWMHints *hints;
    bool     urgent = false;

    hints = XGetWMHints (screen->dpy (), w->id ());
    if (hints)
    {
	if (hints->flags & XUrgencyHint)
	    urgent = true;

	XFree (hints);
    }

    if (urgent || (w->state () & CompWindowStateDemandsAttentionMask))
	addAttentionWindow (w);
    else
	removeAttentionWindow (w);
}

bool
ExtraWMScreen::activateDemandsAttention (CompAction         *action,
					 CompAction::State  state,
					 CompOption::Vector &options)
{
    EXTRAWM_SCREEN (screen);

    if (!es->attentionWindows.empty ())
    {
	CompWindow *w = es->attentionWindows.front ();

	es->attentionWindows.pop_front ();
	w->activate ();
    }

    return false;
}

bool
ExtraWMScreen::activateWin (CompAction         *action,
			    CompAction::State  state,
			    CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");
    w   = screen->findWindow (xid);

    if (w)
	screen->sendWindowActivationRequest (w->id ());

    return true;
}

void
ExtraWMScreen::fullscreenWindow (CompWindow   *w,
		  		 unsigned int state)
{
    unsigned int newState = w->state ();
	
    if (w->overrideRedirect ())
	return;

    /* It would be a bug, to put a shaded window to fullscreen. */
    if (w->shaded ())
	return;

    state = w->constrainWindowState (state, w->actions ());
    state &= CompWindowStateFullscreenMask;

    if (state == (w->state () & CompWindowStateFullscreenMask))
	return;

    newState &= ~CompWindowStateFullscreenMask;
    newState |= state;

    w->changeState (newState);
    w->updateAttributes (CompStackingUpdateModeNormal);
}

bool
ExtraWMScreen::toggleFullscreen (CompAction         *action,
			         CompAction::State  state,
			         CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");
    w   = screen->findWindow (xid);

    if (w && (w->actions () & CompWindowActionFullscreenMask))
    {
	EXTRAWM_SCREEN (screen);

	es->fullscreenWindow (w, w->state () ^ CompWindowStateFullscreenMask);
    }

    return true;
}

bool
ExtraWMScreen::toggleRedirect (CompAction         *action,
			       CompAction::State  state,
			       CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");
    w   = screen->findTopLevelWindow (xid);

    if (w)
    {
	CompositeWindow *cWindow = CompositeWindow::get (w);

	if (cWindow)
	{
	    if (cWindow->redirected ())
		cWindow->unredirect ();
	    else
		cWindow->redirect ();
	}
    }

    return true;
}

bool
ExtraWMScreen::toggleAlwaysOnTop (CompAction         *action,
			          CompAction::State  state,
			          CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");
    w   = screen->findTopLevelWindow (xid);

    if (w)
    {
	unsigned int newState;

	newState = w->state () ^ CompWindowStateAboveMask;
	w->changeState (newState);
	w->updateAttributes (CompStackingUpdateModeNormal);
    }

    return TRUE;
}

bool
ExtraWMScreen::toggleSticky (CompAction         *action,
			     CompAction::State  state,
			     CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");
    w   = screen->findTopLevelWindow (xid);

    if (w && (w->actions () & CompWindowActionStickMask))
    {
	unsigned int newState;
	newState = w->state () ^ CompWindowStateStickyMask;
	w->changeState (newState);
    }

    return TRUE;
}

void
ExtraWMScreen::handleEvent (XEvent *event)
{
    screen->handleEvent (event);

    switch (event->type) {
    case PropertyNotify:
	if (event->xproperty.atom == XA_WM_HINTS)
	{
	    CompWindow *w;

	    w = screen->findWindow (event->xproperty.window);
	    if (w)
		updateAttentionWindow (w);
	}
	break;
    default:
	break;
    }
}

void
ExtraWMWindow::stateChangeNotify (unsigned int lastState)
{
    EXTRAWM_SCREEN (screen);

    window->stateChangeNotify (lastState);

    if ((window->state () ^ lastState) & CompWindowStateDemandsAttentionMask)
	es->updateAttentionWindow (window);
}

ExtraWMScreen::ExtraWMScreen (CompScreen *screen) :
    PrivateHandler <ExtraWMScreen, CompScreen> (screen),
    ExtrawmOptions (extrawmVTable->getMetadata ())
{
    ScreenInterface::setHandler (screen);

    optionSetToggleRedirectKeyInitiate (toggleRedirect);
    optionSetToggleAlwaysOnTopKeyInitiate (toggleAlwaysOnTop);
    optionSetToggleStickyKeyInitiate (toggleSticky);
    optionSetToggleFullscreenKeyInitiate (toggleFullscreen);
    optionSetActivateInitiate (activateWin);
    optionSetActivateDemandsAttentionKeyInitiate (activateDemandsAttention);
}

ExtraWMWindow::ExtraWMWindow (CompWindow *window) :
    PrivateHandler <ExtraWMWindow, CompWindow> (window),
    window (window)
{
    WindowInterface::setHandler (window);
}

bool
ExtraWMPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    return true;
}
