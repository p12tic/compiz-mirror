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
#include <iomanip>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <boost/shared_ptr.hpp>
#include <xorg/gtest/xorg-gtest.h>
#include <compiz-xorg-gtest.h>
#include <gtest_shared_tmpenv.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "compiz-xorg-gtest-config.h"

using ::testing::MatchResultListener;
using ::testing::MatcherInterface;

namespace ct = compiz::testing;
namespace
{
const int          WINDOW_X = 0;
const int          WINDOW_Y = 0;
const unsigned int WINDOW_WIDTH = 640;
const unsigned int WINDOW_HEIGHT = 480;
const unsigned int WINDOW_BORDER = 0;
const unsigned int WINDOW_DEPTH = CopyFromParent;
const unsigned int WINDOW_CLASS = InputOutput;
Visual             *WINDOW_VISUAL = CopyFromParent;


const long                 WINDOW_ATTRIB_VALUE_MASK = 0;

void RemoveEventFromQueue (Display *dpy)
{
    XEvent event;

    if (XNextEvent (dpy, &event) != Success)
	throw std::runtime_error("Failed to remove X event");
}
}

Window
ct::CreateNormalWindow (Display *dpy)
{
    XSetWindowAttributes WINDOW_ATTRIB;
    Window w = XCreateWindow (dpy,
			      DefaultRootWindow (dpy),
			      WINDOW_X,
			      WINDOW_Y,
			      WINDOW_WIDTH,
			      WINDOW_HEIGHT,
			      WINDOW_BORDER,
			      WINDOW_DEPTH,
			      WINDOW_CLASS,
			      WINDOW_VISUAL,
			      WINDOW_ATTRIB_VALUE_MASK,
			      &WINDOW_ATTRIB);

    XSelectInput (dpy, w, StructureNotifyMask);
    return w;
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

namespace
{
class StartupClientMessageMatcher :
    public ct::XEventMatcher
{
    public:

	StartupClientMessageMatcher (Atom                             startup,
				     Window                           root,
				     ct::CompizProcess::StartupFlags state) :
	    mStartup (startup),
	    mRoot (root),
	    mFlags (state)
	{
	}

	virtual bool MatchAndExplain (const XEvent &event, MatchResultListener *listener) const
	{
	    int state = mFlags & ct::CompizProcess::ExpectStartupFailure ? 0 : 1;

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
	ct::CompizProcess::StartupFlags mFlags;
};
}

class ct::PrivatePropertyNotifyXEventMatcher
{
    public:

	PrivatePropertyNotifyXEventMatcher (Display           *dpy,
					    const std::string &propertyName) :
	    mPropertyName (propertyName),
	    mProperty (XInternAtom (dpy, propertyName.c_str (), false))
	{
	}

	std::string mPropertyName;
	Atom	mProperty;
};

ct::PropertyNotifyXEventMatcher::PropertyNotifyXEventMatcher (Display           *dpy,
							      const std::string &propertyName) :
    priv (new ct::PrivatePropertyNotifyXEventMatcher (dpy, propertyName))
{
}

bool
ct::PropertyNotifyXEventMatcher::MatchAndExplain (const XEvent &event, MatchResultListener *listener) const
{
    const XPropertyEvent *propertyEvent = reinterpret_cast <const XPropertyEvent *> (&event);

    if (priv->mProperty == propertyEvent->atom)
	return true;
    else
	return false;
}

void
ct::PropertyNotifyXEventMatcher::DescribeTo (std::ostream *os) const
{
    *os << "Is property identified by " << priv->mPropertyName;
}

class ct::PrivateConfigureNotifyXEventMatcher
{
    public:

	PrivateConfigureNotifyXEventMatcher (Window       above,
					     unsigned int border,
					     int          x,
					     int          y,
					     unsigned int width,
					     unsigned int height) :
	    mAbove (above),
	    mBorder (border),
	    mX (x),
	    mY (y),
	    mWidth (width),
	    mHeight (height)
	{
	}

	Window       mAbove;
	int          mBorder;
	int          mX;
	int          mY;
	int          mWidth;
	int          mHeight;
};

ct::ConfigureNotifyXEventMatcher::ConfigureNotifyXEventMatcher (Window       above,
								unsigned int border,
								int          x,
								int          y,
								unsigned int width,
								unsigned int height) :
    priv (new ct::PrivateConfigureNotifyXEventMatcher (above,
						       border,
						       x,
						       y,
						       width,
						       height))
{
}

bool
ct::ConfigureNotifyXEventMatcher::MatchAndExplain (const XEvent &event, MatchResultListener *listener) const
{
    const XConfigureEvent *ce = reinterpret_cast <const XConfigureEvent *> (&event);

    return ce->above == priv->mAbove &&
	   ce->border_width == priv->mBorder &&
	   ce->x == priv->mX &&
	   ce->y == priv->mY &&
	   ce->width == priv->mWidth &&
	   ce->height == priv->mHeight;
}

void
ct::ConfigureNotifyXEventMatcher::DescribeTo (std::ostream *os) const
{
    *os << "Matches ConfigureNotify with parameters : " << std::endl <<
	   " x: " << priv->mX <<
	   " y: " << priv->mY <<
	   " width: " << priv->mWidth <<
	   " height: " << priv->mHeight <<
	   " border: " << priv->mBorder <<
	   " above: " << std::hex << priv->mAbove << std::dec;
}

class ct::PrivateCompizProcess
{
    public:
	PrivateCompizProcess (ct::CompizProcess::StartupFlags flags) :
	    mFlags (flags),
	    mIsRunning (true)
	{
	}

	void WaitForStartupMessage (Display                         *dpy,
				    ct::CompizProcess::StartupFlags flags,
				    unsigned int                    waitTimeout);

	typedef boost::shared_ptr <xorg::testing::Process> ProcessPtr;

	ct::CompizProcess::StartupFlags mFlags;
	bool                            mIsRunning;
	xorg::testing::Process          mProcess;
};

void
ct::PrivateCompizProcess::WaitForStartupMessage (Display                         *dpy,
						 ct::CompizProcess::StartupFlags flags,
						 unsigned int                    waitTimeout)
{
    XWindowAttributes attrib;
    Window    root = DefaultRootWindow (dpy);

    Atom    startup = XInternAtom (dpy,
				   "_COMPIZ_TESTING_STARTUP",
				   false);

    StartupClientMessageMatcher matcher (startup, root, flags);

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
							     waitTimeout)));

    XSelectInput (dpy, root, attrib.your_event_mask);
}

ct::CompizProcess::CompizProcess (::Display                       *dpy,
				  ct::CompizProcess::StartupFlags flags,
				  unsigned int                    waitTimeout) :
    priv (new PrivateCompizProcess (flags))
{
    xorg::testing::Process::SetEnv ("LD_LIBRARY_PATH", compizLDLibraryPath, true);

    std::vector <std::string> args;

    if (flags & ct::CompizProcess::ReplaceCurrentWM)
	args.push_back ("--replace");

    args.push_back ("--send-startup-message");

    priv->mProcess.Start (compizBinaryPath, args);
    EXPECT_EQ (priv->mProcess.GetState (), xorg::testing::Process::RUNNING);

    if (flags & ct::CompizProcess::WaitForStartupMessage)
	priv->WaitForStartupMessage (dpy, flags, waitTimeout);
}

ct::CompizProcess::~CompizProcess ()
{
    if (priv->mProcess.GetState () == xorg::testing::Process::RUNNING)
	priv->mProcess.Kill ();
}

xorg::testing::Process::State
ct::CompizProcess::State ()
{
    return priv->mProcess.GetState ();
}

pid_t
ct::CompizProcess::Pid ()
{
    return priv->mProcess.Pid ();
}

class ct::PrivateCompizXorgSystemTest
{
    public:

	PrivateCompizXorgSystemTest () :
	    mStdoutEnv ("XORG_GTEST_CHILD_STDOUT", "1")
	{
	}

	boost::shared_ptr <ct::CompizProcess> mProcess;
	TmpEnv                                mStdoutEnv;
};

ct::CompizXorgSystemTest::CompizXorgSystemTest () :
    priv (new PrivateCompizXorgSystemTest)
{
}

void
ct::CompizXorgSystemTest::SetUp ()
{
    xorg::testing::Test::SetUp ();
}

void
ct::CompizXorgSystemTest::TearDown ()
{
    priv->mProcess.reset ();

    xorg::testing::Test::TearDown ();
}

xorg::testing::Process::State
ct::CompizXorgSystemTest::CompizProcessState ()
{
    if (priv->mProcess)
	return priv->mProcess->State ();
    return xorg::testing::Process::NONE;
}

void
ct::CompizXorgSystemTest::StartCompiz (ct::CompizProcess::StartupFlags flags)
{
    priv->mProcess.reset (new ct::CompizProcess (Display (), flags, 3000));
}

void
ct::AutostartCompizXorgSystemTest::SetUp ()
{
    ct::CompizXorgSystemTest::SetUp ();
    StartCompiz (static_cast <ct::CompizProcess::StartupFlags> (
		     ct::CompizProcess::ReplaceCurrentWM |
		     ct::CompizProcess::WaitForStartupMessage));
}
