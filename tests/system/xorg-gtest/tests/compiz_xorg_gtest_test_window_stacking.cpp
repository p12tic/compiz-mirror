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
#include <gtest/gtest.h>
#include <xorg/gtest/xorg-gtest.h>
#include <compiz-xorg-gtest.h>

#include <X11/Xlib.h>

namespace ct = compiz::testing;

class CompizXorgSystemStackingTest :
    public compiz::testing::XorgSystemTest
{
};

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


    const long                 WINDOW_ATTRIB_VALUE_MASK= 0;
}

TEST_F (CompizXorgSystemStackingTest, TestSetup)
{
}

TEST_F (CompizXorgSystemStackingTest, TestCreateWindowsAndRestackRelativeToEachOther)
{
    XSetWindowAttributes WINDOW_ATTRIB;
    ::Display *dpy = Display ();

    Window w1 = XCreateWindow (dpy,
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

    Window w2 = XCreateWindow (dpy,
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

    /* Both reparented */
    xorg::testing::XServer::WaitForEventOfType (dpy, ReparentNotify, -1, -1);
    xorg::testing::XServer::WaitForEventOfType (dpy, ReparentNotify, -1, -1);

    /* Check the client list to see that w2 > w1 */
    std::list <Window> clientList = ct::NET_CLIENT_LIST_STACKING (dpy);

    ASSERT_EQ (clientList.size (), 2);
    EXPECT_EQ (clientList.front (), w1);
    EXPECT_EQ (clientList.back (), w2);
}
