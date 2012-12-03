/*
 * Compiz XOrg GTest
 *
 * Copyright (C) 2012 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authored By:
 * Sam Spilsbury <sam.spilsbury@canonical.com>
 */
#include <list>
#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <xorg/gtest/xorg-gtest.h>
#include <compiz-xorg-gtest.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "compiz-xorg-gtest-config.h"

using ::testing::MatchResultListener;
using ::testing::MatcherInterface;

namespace ct = compiz::testing;
namespace
{
    void RemoveEventFromQueue (Display *dpy)
    {
	XEvent event;

	if (XNextEvent (dpy, &event) != Success)
	    throw std::runtime_error("Failed to remove X event");
    }
}

bool
ct::AdvanceToNextEventOnSuccess (Display *dpy,
				 bool waitResult)
{
    if (waitResult)
	RemoveEventFromQueue (dpy);

    return waitResult;
}

bool
ct::WaitForEventOfTypeOnWindow (Display *dpy,
				Window  w,
				int     type,
				int     ext,
				int     extType,
				int     timeout)
{
    while (xorg::testing::XServer::WaitForEventOfType (dpy, type, ext, extType, timeout))
    {
	XEvent event;
	if (!XPeekEvent (dpy, &event))
	    throw std::runtime_error ("Failed to peek event");

	if (event.xany.window != w)
	{
	    RemoveEventFromQueue (dpy);
	    continue;
	}

	return true;
    }

    return false;
}

bool
ct::WaitForEventOfTypeOnWindowMatching (Display             *dpy,
					Window              w,
					int                 type,
					int                 ext,
					int                 extType,
					const XEventMatcher &matcher,
					int                 timeout)
{
    while (ct::WaitForEventOfTypeOnWindow (dpy, w, type, ext, extType, timeout))
    {
	XEvent event;
	if (!XPeekEvent (dpy, &event))
	    throw std::runtime_error ("Failed to peek event");

	if (!matcher.MatchAndExplain (event, NULL))
	{
	    RemoveEventFromQueue (dpy);
	    continue;
	}

	return true;
    }

    return false;
}

std::list <Window>
ct::NET_CLIENT_LIST_STACKING (Display *dpy)
{
    Atom property = XInternAtom (dpy, "_NET_CLIENT_LIST_STACKING", false);
    Atom actual_type;
    int actual_fmt;
    unsigned long nitems, nleft;
    unsigned char *prop;
    std::list <Window> stackingOrder;

    /* _NET_CLIENT_LIST_STACKING */
    if (XGetWindowProperty (dpy, DefaultRootWindow (dpy), property, 0L, 512L, false, XA_WINDOW,
			    &actual_type, &actual_fmt, &nitems, &nleft, &prop) == Success)
    {
	if (nitems && !nleft && actual_fmt == 32 && actual_type == XA_WINDOW)
	{
	    Window *window = reinterpret_cast <Window *> (prop);

	    while (nitems--)
		stackingOrder.push_back (*window++);

	}

	if (prop)
	    XFree (prop);
    }

    return stackingOrder;
}

void
ct::XorgSystemTest::SetUp ()
{
    xorg::testing::Test::SetUp ();
}

void
ct::XorgSystemTest::TearDown ()
{
    if (mCompizProcess.GetState () == xorg::testing::Process::RUNNING)
	mCompizProcess.Kill ();

    xorg::testing::Test::TearDown ();
}

xorg::testing::Process::State
ct::XorgSystemTest::CompizProcessState ()
{
    return mCompizProcess.GetState ();
}

namespace
{
class StartupClientMessageMatcher :
    public ct::XEventMatcher
{
    public:

	StartupClientMessageMatcher (Atom                             startup,
				     Window                           root,
				     ct::XorgSystemTest::StartupState state) :
	    mStartup (startup),
	    mRoot (root),
	    mExpectedState (state)
	{
	}

	virtual bool MatchAndExplain (const XEvent &event, MatchResultListener *listener) const
	{
	    int state = mExpectedState == ct::XorgSystemTest::ExpectStartupSuccess ? 1 : 0;

	    if (event.xclient.window == mRoot &&
		event.xclient.message_type == mStartup &&
		event.xclient.data.l[0] == state)
		return true;

	    return false;
	}

	virtual void DescribeTo (std::ostream *os) const
	{
	    *os << "is startup message";
	}

	virtual void DescribeNegationTo (std::ostream *os) const
	{
	    *os << "is not startup message";
	}

    private:

	Atom mStartup;
	Window mRoot;
	ct::XorgSystemTest::StartupState mExpectedState;
};
}


void
ct::XorgSystemTest::StartCompiz (StartupState startupState)
{
    XWindowAttributes attrib;

    xorg::testing::Process::SetEnv ("LD_LIBRARY_PATH", compizLDLibraryPath, true);
    mCompizProcess.Start (compizBinaryPath, "--replace", "--send-startup-message", NULL);

    /* Wait for the startup message */
    ::Display *dpy = Display ();
    Window    root = DefaultRootWindow (dpy);

    Atom    startup = XInternAtom (dpy,
				   "_COMPIZ_TESTING_STARTUP",
				   false);

    StartupClientMessageMatcher matcher (startup, root, startupState);

    ASSERT_EQ (mCompizProcess.GetState (), xorg::testing::Process::RUNNING);

    /* Save the current event mask and subscribe to StructureNotifyMask only */
    ASSERT_TRUE (XGetWindowAttributes (dpy, root, &attrib));
    XSelectInput (dpy, root, StructureNotifyMask |
			     attrib.your_event_mask);

    ASSERT_TRUE (ct::AdvanceToNextEventOnSuccess (
		     dpy,
		     ct::WaitForEventOfTypeOnWindowMatching (dpy,
							     root,
							     ClientMessage,
							     -1,
							     -1,
							     matcher,
							     3000)));

    XSelectInput (dpy, root, attrib.your_event_mask);
}
