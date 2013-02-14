/*
 * Compiz XOrg GTest, ConfigureWindow handling
 *
 * Copyright (C) 2012 Sam Spilsbury (smspillaz@gmail.com)
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
 * Authored By:
 * Sam Spilsbury <smspillaz@gmail.com>
 */
#include <list>
#include <string>
#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <xorg/gtest/xorg-gtest.h>
#include <compiz-xorg-gtest.h>
#include <compiz_xorg_gtest_communicator.h>

#include <gtest_shared_tmpenv.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

using ::testing::MatchResultListener;
using ::testing::MakeMatcher;
using ::testing::Matcher;

namespace ct = compiz::testing;

namespace
{

bool Advance (Display *d, bool r)
{
    return ct::AdvanceToNextEventOnSuccess (d, r);
}

}

class CompizXorgSystemConfigureWindowTest :
    public ct::AutostartCompizXorgSystemTestWithTestHelper
{
    public:

	CompizXorgSystemConfigureWindowTest () :
	    /* See note in the acceptance tests about this */
	    env ("XORG_GTEST_CHILD_STDOUT", "1")
	{
	}

	void SendConfigureRequest (Window w, int x, int y, int width, int height, int mask);
	bool VerifyConfigureResponse (Window w, int x, int y, int width, int height);
	bool VerifyWindowSize (Window w, int x, int y, int width, int height);

	TmpEnv env;
};

void
CompizXorgSystemConfigureWindowTest::SendConfigureRequest (Window w,
							   int x,
							   int y,
							   int width,
							   int height,
							   int mask)
{
    ::Display *dpy = Display ();

    std::vector <long> data;
    data.push_back (x);
    data.push_back (y);
    data.push_back (width);
    data.push_back (height);
    data.push_back (CWX | CWY | CWWidth | CWHeight);

    ct::SendClientMessage (dpy,
			   FetchAtom (ct::messages::TEST_HELPER_CONFIGURE_WINDOW),
			   DefaultRootWindow (dpy),
			   w,
			   data);
}

bool
CompizXorgSystemConfigureWindowTest::VerifyConfigureResponse (Window w,
							      int x,
							      int y,
							      int width,
							      int height)
{
    ::Display *dpy = Display ();
    XEvent event;

    while (ct::ReceiveMessage (dpy,
			       FetchAtom (ct::messages::TEST_HELPER_WINDOW_CONFIGURE_PROCESSED),
			       event))
    {
	bool requestAcknowledged =
		x == event.xclient.data.l[0] &&
		y == event.xclient.data.l[1] &&
		width == event.xclient.data.l[2] &&
		height == event.xclient.data.l[3];

	if (requestAcknowledged)
	    return true;

    }

    return false;
}

bool
CompizXorgSystemConfigureWindowTest::VerifyWindowSize (Window w,
						       int x,
						       int y,
						       int width,
						       int height)
{
    ::Display *dpy = Display ();

    Window       rootRet;
    int          xRet, yRet;
    unsigned int widthRet, heightRet, depth, border;
    if (!XGetGeometry (dpy,
		       w,
		       &rootRet,
		       &xRet,
		       &yRet,
		       &widthRet,
		       &heightRet,
		       &depth,
		       &border))
	return false;

    EXPECT_EQ (x, xRet);
    EXPECT_EQ (y, yRet);
    EXPECT_EQ (width, widthRet);
    EXPECT_EQ (height, heightRet);

    return true;
}

TEST_F (CompizXorgSystemConfigureWindowTest, ConfigureAndReponseUnlocked)
{
    ::Display *dpy = Display ();

    int x = 1;
    int y = 1;
    int width = 100;
    int height = 200;
    int mask = CWX | CWY | CWWidth | CWHeight;

    Window w = ct::CreateNormalWindow (dpy);
    WaitForWindowCreation (w);

    SendConfigureRequest (w, x, y, width, height, mask);

    /* Wait for a response */
    ASSERT_TRUE (VerifyConfigureResponse (w, x, y, width, height));

    /* Query the window size again */
    ASSERT_TRUE (VerifyWindowSize (w, x, y, width, height));

}
