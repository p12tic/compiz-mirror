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

Window GetTopParent (Display *display,
		     Window w)
{
    Window rootReturn;
    Window parentReturn = w;
    Window *childrenReturn;
    unsigned int nChildrenReturn;

    Window lastParent = 0;

    do
    {
	lastParent = parentReturn;

	XQueryTree (display,
		    lastParent,
		    &rootReturn,
		    &parentReturn,
		    &childrenReturn,
		    &nChildrenReturn);
	XFree (childrenReturn);
    } while (parentReturn != rootReturn);

    return lastParent;
}

bool QueryGeometry (Display *dpy,
		    Window w,
		    int &x,
		    int &y,
		    unsigned int &width,
		    unsigned int &height)
{
    Window rootRet;
    unsigned int  depth, border;

    if (!XGetGeometry (dpy,
		       w,
		       &rootRet,
		       &x,
		       &y,
		       &width,
		       &height,
		       &depth,
		       &border))
	return false;

    return true;
}

bool WaitForReparentAndMap (Display *dpy,
			    Window w)
{
    bool ret = Advance (dpy, ct::WaitForEventOfTypeOnWindow (dpy,
							     w,
							     ReparentNotify,
							     -1,
							     -1));
    EXPECT_TRUE (ret);
    if (!ret)
	return false;


    ret = Advance (dpy, ct::WaitForEventOfTypeOnWindow (dpy,
							w,
							MapNotify,
							-1,
							-1));
    EXPECT_TRUE (ret);
    if (!ret)
	return false;

    return true;
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
	void SendSetFrameExtentsRequest (Window w, int left, int right, int top, int bottom);
	bool VerifyConfigureResponse (Window w, int x, int y, int width, int height);
	bool VerifySetFrameExtentsResponse (Window w, int left, int right, int top, int bottom);
	bool VerifyWindowSize (Window w, int x, int y, int width, int height);

    protected:

	int GetEventMask ();

    private:

	TmpEnv env;
};

int
CompizXorgSystemConfigureWindowTest::GetEventMask ()
{
    return ct::AutostartCompizXorgSystemTestWithTestHelper::GetEventMask () |
	    SubstructureNotifyMask;
}

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

void
CompizXorgSystemConfigureWindowTest::SendSetFrameExtentsRequest (Window w,
								 int    left,
								 int    right,
								 int    top,
								 int    bottom)
{
    ::Display *dpy = Display ();

    std::vector <long> data;
    data.push_back (left);
    data.push_back (right);
    data.push_back (top);
    data.push_back (bottom);

    ct::SendClientMessage (dpy,
			   FetchAtom (ct::messages::TEST_HELPER_CHANGE_FRAME_EXTENTS),
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
CompizXorgSystemConfigureWindowTest::VerifySetFrameExtentsResponse (Window w,
								    int left,
								    int right,
								    int top,
								    int bottom)
{
    ::Display *dpy = Display ();
    XEvent event;

    while (ct::ReceiveMessage (dpy,
			       FetchAtom (ct::messages::TEST_HELPER_FRAME_EXTENTS_CHANGED),
			       event))
    {
	bool requestAcknowledged =
		left == event.xclient.data.l[0] &&
		right == event.xclient.data.l[1] &&
		top == event.xclient.data.l[2] &&
		bottom == event.xclient.data.l[3];

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

    int          xRet, yRet;
    unsigned int widthRet, heightRet;
    if (!QueryGeometry (dpy, w, xRet, yRet, widthRet, heightRet))
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

    XMapRaised (dpy, w);
    WaitForReparentAndMap (dpy, w);

    ASSERT_TRUE (Advance (dpy, ct::WaitForEventOfTypeOnWindow (dpy,
							       w,
							       ReparentNotify,
							       -1,
							       -1)));

    SendConfigureRequest (w, x, y, width, height, mask);

    /* Wait for a response */
    ASSERT_TRUE (VerifyConfigureResponse (w, x, y, width, height));

    /* Query the window size again */
    ASSERT_TRUE (VerifyWindowSize (w, x, y, width, height));

}

TEST_F (CompizXorgSystemConfigureWindowTest, FrameExtentsAndReponseUnlocked)
{
    ::Display *dpy = Display ();

    int left = 1;
    int right = 2;
    int top = 3;
    int bottom = 4;

    Window w = ct::CreateNormalWindow (dpy);
    WaitForWindowCreation (w);

    XMapRaised (dpy, w);
    WaitForReparentAndMap (dpy, w);

    int x, y;
    unsigned int width, height;
    ASSERT_TRUE (QueryGeometry (dpy, w, x, y, width, height));

    SendSetFrameExtentsRequest (w, left, right, top, bottom);

    /* Wait for a response */
    ASSERT_TRUE (VerifySetFrameExtentsResponse (w, left, right, top, bottom));

    /* Client geometry is always unchanged */
    ASSERT_TRUE (VerifyWindowSize (w, x, y, width, height));

    /* Frame geometry is frame geometry offset by extents */
    x -= left;
    y -= top;
    width += left + right;
    height += top + bottom;

    Window frame = GetTopParent (dpy, w);
    ASSERT_TRUE (VerifyWindowSize (frame, x, y, width, height));
}
