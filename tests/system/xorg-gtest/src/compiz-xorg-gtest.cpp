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
#include <gtest/gtest.h>
#include <xorg/gtest/xorg-gtest.h>
#include <compiz-xorg-gtest.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "compiz-xorg-gtest-config.h"

namespace ct = compiz::testing;

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

    const std::string dispString (":998");

    xorg::testing::Process::SetEnv ("LD_LIBRARY_PATH", compizLDLibraryPath, true);
    xorg::testing::Process::SetEnv ("DISPLAY", dispString, true);

    xorg::testing::XServer::WaitForEventOfType (Display (), ClientMessage, -1, -1);

    mCompizProcess.Start (compizBinaryPath, "--replace", NULL);

    ASSERT_EQ (mCompizProcess.GetState (), xorg::testing::Process::RUNNING);
}

void
ct::XorgSystemTest::TearDown ()
{
    mCompizProcess.Kill ();
    xorg::testing::Test::TearDown ();
}
