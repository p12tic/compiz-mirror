/*
 * Compiz XOrg GTest, window stacking
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
#include <string>
#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <xorg/gtest/xorg-gtest.h>
#include <compiz-xorg-gtest.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

using ::testing::MatchResultListener;
using ::testing::MakeMatcher;
using ::testing::Matcher;

namespace ct = compiz::testing;

class CompizXorgSystemStackingTest :
    public ct::AutostartCompizXorgSystemTest
{
    public:

	virtual void SetUp ()
	{
	    ct::AutostartCompizXorgSystemTest::SetUp ();

	    ::Display *dpy = Display ();
	    XSelectInput (dpy, DefaultRootWindow (dpy), SubstructureNotifyMask | PropertyChangeMask);
	}
};

namespace
{
    bool Advance (Display *dpy,
		  bool waitResult)
    {
	return ct::AdvanceToNextEventOnSuccess (dpy, waitResult);
    }

    void MakeDock (Display *dpy, Window w)
    {
	Atom _NET_WM_WINDOW_TYPE = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE", false);
	Atom _NET_WM_WINDOW_TYPE_DOCK = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DOCK", false);

	XChangeProperty (dpy,
			 w,
			 _NET_WM_WINDOW_TYPE,
			 XA_ATOM,
			 32,
			 PropModeReplace,
			 reinterpret_cast <const unsigned char *> (&_NET_WM_WINDOW_TYPE_DOCK),
			 1);
    }

    void SetUserTime (Display *dpy, Window w, Time time)
    {
	Atom _NET_WM_USER_TIME = XInternAtom (dpy, "_NET_WM_USER_TIME", false);
	unsigned int value = (unsigned int) time;

	XChangeProperty (dpy, w,
			 _NET_WM_USER_TIME,
			 XA_CARDINAL, 32, PropModeReplace,
			 (unsigned char *) &value, 1);
    }

    void SetClientLeader (Display *dpy, Window w, Window leader)
    {
	Atom WM_CLIENT_LEADER = XInternAtom (dpy, "WM_CLIENT_LEADER", false);

	XChangeProperty (dpy, w,
			 WM_CLIENT_LEADER,
			 XA_WINDOW, 32, PropModeReplace,
			 (unsigned char *) &leader, 1);
    }
}

TEST_F (CompizXorgSystemStackingTest, TestSetup)
{
}

TEST_F (CompizXorgSystemStackingTest, TestCreateWindowsAndRestackRelativeToEachOther)
{
    ::Display *dpy = Display ();

    Window w1 = ct::CreateNormalWindow (dpy);
    Window w2 = ct::CreateNormalWindow (dpy);

    XMapRaised (dpy, w1);
    XMapRaised (dpy, w2);

    /* Both reparented and both mapped */
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindow (dpy,w1, ReparentNotify, -1, -1)));
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindow (dpy, w1, MapNotify, -1, -1)));
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindow (dpy, w2, ReparentNotify, -1, -1)));
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindow (dpy, w2, MapNotify, -1, -1)));

    ct::PropertyNotifyXEventMatcher matcher (dpy, "_NET_CLIENT_LIST_STACKING");

    /* Wait for property change notify on the root window to happen twice */
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindowMatching (dpy,
								       DefaultRootWindow (dpy),
								       PropertyNotify,
								       -1,
								       -1,
								       matcher)));
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindowMatching (dpy,
								       DefaultRootWindow (dpy),
								       PropertyNotify,
								       -1,
								       -1,
								       matcher)));

    /* Check the client list to see that w2 > w1 */
    std::list <Window> clientList = ct::NET_CLIENT_LIST_STACKING (dpy);

    ASSERT_EQ (clientList.size (), 2);
    EXPECT_EQ (clientList.front (), w1);
    EXPECT_EQ (clientList.back (), w2);
}

TEST_F (CompizXorgSystemStackingTest, TestCreateWindowsAndRestackRelativeToEachOtherDockAlwaysOnTop)
{
    ::Display *dpy = Display ();
    ct::PropertyNotifyXEventMatcher matcher (dpy, "_NET_CLIENT_LIST_STACKING");

    Window dock = ct::CreateNormalWindow (dpy);

    /* Make it a dock */
    MakeDock (dpy, dock);

    /* Immediately map the dock window and clear the event queue for it */
    XMapRaised (dpy, dock);

    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindow (dpy, dock, ReparentNotify, -1, -1)));
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindow (dpy, dock, MapNotify, -1, -1)));

    /* Dock window needs to be in the client list */
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindowMatching (dpy,
								       DefaultRootWindow (dpy),
								       PropertyNotify,
								       -1,
								       -1,
								       matcher)));
    std::list <Window> clientList = ct::NET_CLIENT_LIST_STACKING (dpy);
    ASSERT_EQ (clientList.size (), 1);

    Window w1 = ct::CreateNormalWindow (dpy);
    Window w2 = ct::CreateNormalWindow (dpy);

    XSelectInput (dpy, w2, StructureNotifyMask);

    XMapRaised (dpy, w1);
    XMapRaised (dpy, w2);

    /* Both reparented and both mapped */
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindow (dpy, w1, ReparentNotify, -1, -1)));
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindow (dpy, w1, MapNotify, -1, -1)));
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindow (dpy, w2, ReparentNotify, -1, -1)));
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindow (dpy, w2, MapNotify, -1, -1)));

    /* Wait for property change notify on the root window to happen twice */
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindowMatching (dpy,
								       DefaultRootWindow (dpy),
								       PropertyNotify,
								       -1,
								       -1,
								       matcher)));
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindowMatching (dpy,
								       DefaultRootWindow (dpy),
								       PropertyNotify,
								       -1,
								       -1,
								       matcher)));
    clientList = ct::NET_CLIENT_LIST_STACKING (dpy);

    /* Check the client list to see that dock > w2 > w1 */
    ASSERT_EQ (clientList.size (), 3);

    std::list <Window>::iterator it = clientList.begin ();

    EXPECT_EQ (*it++, w1); /* first should be the bottom normal window */
    EXPECT_EQ (*it++, w2); /* second should be the top normal window */
    EXPECT_EQ (*it++, dock); /* dock must always be on top */
}

TEST_F (CompizXorgSystemStackingTest, TestMapWindowWithOldUserTime)
{
    ::Display *dpy = Display ();
    ct::PropertyNotifyXEventMatcher matcher (dpy, "_NET_CLIENT_LIST_STACKING");

    Window w1 = ct::CreateNormalWindow (dpy);
    Window w2 = ct::CreateNormalWindow (dpy);
    Window w3 = ct::CreateNormalWindow (dpy);

    XMapRaised (dpy, w1);
    XMapRaised (dpy, w2);

    /* Wait for property change notify on the root window to happen twice */
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindowMatching (dpy,
								       DefaultRootWindow (dpy),
								       PropertyNotify,
								       -1,
								       -1,
								       matcher)));
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindowMatching (dpy,
								       DefaultRootWindow (dpy),
								       PropertyNotify,
								       -1,
								       -1,
								       matcher)));

    SetUserTime (dpy, w2, 200);
    SetClientLeader (dpy, w2, w2);
    SetUserTime (dpy, w3, 100);
    SetClientLeader (dpy, w3, w3);

    XMapRaised (dpy, w3);
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindowMatching (dpy, DefaultRootWindow (dpy), PropertyNotify, -1, -1, matcher)));

    /* Check the client list to see that w2 > w3 > w1 */
    std::list <Window> clientList = ct::NET_CLIENT_LIST_STACKING (dpy);
    ASSERT_EQ (clientList.size (), 3);

    std::list <Window>::iterator it = clientList.begin ();
    EXPECT_EQ (*it++, w1);
    EXPECT_EQ (*it++, w3);
    EXPECT_EQ (*it++, w2);
}

TEST_F (CompizXorgSystemStackingTest, TestMapWindowAndDenyFocus)
{
    ::Display *dpy = Display ();
    ct::PropertyNotifyXEventMatcher matcher (dpy, "_NET_CLIENT_LIST_STACKING");

    Window w1 = ct::CreateNormalWindow (dpy);
    Window w2 = ct::CreateNormalWindow (dpy);
    Window w3 = ct::CreateNormalWindow (dpy);

    XMapRaised (dpy, w1);
    XMapRaised (dpy, w2);

    /* Wait for property change notify on the root window to happen twice */
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindowMatching (dpy,
								       DefaultRootWindow (dpy),
								       PropertyNotify,
								       -1,
								       -1,
								       matcher)));
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindowMatching (dpy,
								       DefaultRootWindow (dpy),
								       PropertyNotify,
								       -1,
								       -1,
								       matcher)));
    SetUserTime (dpy, w3, 0);
    SetClientLeader (dpy, w3, w3);

    XMapRaised (dpy, w3);
    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindowMatching (dpy,
								       DefaultRootWindow (dpy),
								       PropertyNotify,
								       -1,
								       -1,
								       matcher)));
    /* Check the client list to see that w2 > w3 > w1 */
    std::list <Window> clientList = ct::NET_CLIENT_LIST_STACKING (dpy);
    ASSERT_EQ (clientList.size (), 3);

    std::list <Window>::iterator it = clientList.begin ();
    EXPECT_EQ (*it++, w1);
    EXPECT_EQ (*it++, w3);
    EXPECT_EQ (*it++, w2);
}
