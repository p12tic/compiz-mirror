/*
* Copyright © 2013 Sam Spilsbury
*
* Permission to use, copy, modify, distribute, and sell this software
* and its documentation for any purpose is hereby granted without
* fee, provided that the above copyright notice appear in all copies
* and that both that copyright notice and this permission notice
* appear in supporting documentation, and that the name of
* Novell, Inc. not be used in advertising or publicity pertaining to
* distribution of the software without specific, written prior permission.
* Novell, Inc. makes no representations about the suitability of this
* software for any purpose. It is provided "as is" without express or
* implied warranty.
*
* NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
* INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
* NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
* CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
* OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
* NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
* WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*
* Author: Sam Spilsbury <smspillaz@gmail.com>
*/

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include "testhelper.h"

COMPIZ_PLUGIN_20090315 (testhelper, TestHelperPluginVTable)

namespace
{
template <typename T>
void XFreeT (T *t)
{
    XFree (t);
}
}

namespace ct = compiz::testing;
namespace ctm = compiz::testing::messages;

void
TestHelperScreen::handleEvent (XEvent *event)
{
    if (event->type == ClientMessage)
    {
	if (event->xclient.window != screen->root ())
	{
	    std::map <Atom, ClientMessageHandler>::iterator it =
		mMessageHandlers.find (event->xclient.message_type);

	    if (it != mMessageHandlers.end ())
	    {
		ClientMessageHandler handler (it->second);
		CompWindow *w = screen->findWindow (event->xclient.window);

		XClientMessageEvent *xce = &event->xclient;
		long                *data = xce->data.l;

		if (w)
		    ((*TestHelperWindow::get (w)).*(handler)) (data);
	    }
	}
    }

    screen->handleEvent (event);
}

void
TestHelperScreen::watchForMessage (Atom message, ClientMessageHandler handler)
{
    if (mMessageHandlers.find (message) != mMessageHandlers.end ())
    {
	boost::shared_ptr <char> name (XGetAtomName (screen->dpy (), message),
				       boost::bind (XFreeT <char>, _1));
	compLogMessage ("testhelper", CompLogLevelWarn,
			"a message handler was already defined for %s",
			name.get ());
	return;
    }

    mMessageHandlers[message] = handler;
}

void
TestHelperScreen::removeMessageWatch (Atom message)
{
    std::map <Atom, ClientMessageHandler>::iterator it =
	mMessageHandlers.find (message);

    if (it != mMessageHandlers.end ())
	mMessageHandlers.erase (it);
}

TestHelperWindow::TestHelperWindow (CompWindow *w) :
    PluginClassHandler <TestHelperWindow, CompWindow> (w),
    window (w),
    configureLock ()
{
    WindowInterface::setHandler (w);
}

TestHelperScreen::TestHelperScreen (CompScreen *s) :
    PluginClassHandler <TestHelperScreen, CompScreen> (s),
    screen (s),
    mAtomStore (s->dpy ())
{
    ScreenInterface::setHandler (s);

    ct::SendClientMessage (s->dpy (),
			   mAtomStore.FetchForString (ctm::TEST_HELPER_READY_MSG),
			   s->root (),
			   s->root (),
			   std::vector <long> ());
}

bool
TestHelperPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    return true;
}
