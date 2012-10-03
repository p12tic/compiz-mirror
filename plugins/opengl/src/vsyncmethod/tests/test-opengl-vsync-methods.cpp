/*
 * Compiz, opengl plugin, vsync methods
 *
 * Copyright © 2012 Canonical Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Dennis Kasprzyk not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Dennis Kasprzyk makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DENNIS KASPRZYK DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DENNIS KASPRZYK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Sam Spilsbury <sam.spilsbury@canonical.com>
 */
#include <boost/bind.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <blocking-vsync.h>
#include <asynchronous-vsync.h>

namespace cgl = compiz::opengl;
namespace cgli = compiz::opengl::impl;

using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::_;

namespace
{
class MockOpenGLFunctionsTable
{
    public:

	MOCK_METHOD3 (waitVideoSyncSGI, int (int, int, unsigned int *));
	MOCK_METHOD1 (swapIntervalEXT, void (int));
};

cgli::GLXWaitVideoSyncSGIFunc
GetWaitVideoSyncFuncFromMock (MockOpenGLFunctionsTable &mock)
{
    return boost::bind (&MockOpenGLFunctionsTable::waitVideoSyncSGI, &mock, _1, _2, _3);
}

cgli::GLXSwapIntervalEXTFunc
GetSwapIntervalFuncFromMock (MockOpenGLFunctionsTable &mock)
{
    return boost::bind (&MockOpenGLFunctionsTable::swapIntervalEXT, &mock, _1);
}
}

class OpenGLAsynchronousVSync :
    public ::testing::Test
{
    public:

	OpenGLAsynchronousVSync () :
	    swapIntervalMethod (GetSwapIntervalFuncFromMock (functions))
	{
	}

	MockOpenGLFunctionsTable      functions;
	cgli::AsynchronousVSync swapIntervalMethod;
};

class OpenGLBlockingVSyncTest :
    public ::testing::Test
{
    public:

	OpenGLBlockingVSyncTest () :
	    waitVideoSyncMethod (GetWaitVideoSyncFuncFromMock (functions))
	{
	}

	MockOpenGLFunctionsTable      functions;
	cgli::BlockingVSync         waitVideoSyncMethod;
};

TEST_F (OpenGLAsynchronousVSync, TestCallSwapIntervalOnEnableForFlip)
{
    bool throttled;
    EXPECT_CALL (functions, swapIntervalEXT (1));
    EXPECT_TRUE (swapIntervalMethod.enable (cgl::Flip, throttled));
}

TEST_F (OpenGLAsynchronousVSync, TestCallSwapIntervalOnEnableForFlipOnlyOnce)
{
    bool throttled;
    EXPECT_CALL (functions, swapIntervalEXT (1)).Times (1);
    EXPECT_TRUE (swapIntervalMethod.enable (cgl::Flip, throttled));
    /* This is still meant to return true even though it does
     * nothing it just means the operation succeeded */
    EXPECT_TRUE (swapIntervalMethod.enable (cgl::Flip, throttled));
}

TEST_F (OpenGLAsynchronousVSync, TestCallSwapIntervalOnEnableForFlipAndZeroForDisable)
{
    bool throttled;
    EXPECT_CALL (functions, swapIntervalEXT (1));
    EXPECT_TRUE (swapIntervalMethod.enable (cgl::Flip, throttled));
    EXPECT_CALL (functions, swapIntervalEXT (0));
    swapIntervalMethod.disable ();
}

TEST_F (OpenGLAsynchronousVSync, TestCallSwapIntervalZeroForDisableOnce)
{
    bool throttled;
    /* Enable it */
    EXPECT_CALL (functions, swapIntervalEXT (1)).Times (1);
    swapIntervalMethod.enable (cgl::Flip, throttled);

    /* Disable it twice */
    EXPECT_CALL (functions, swapIntervalEXT (0)).Times (1);
    swapIntervalMethod.disable ();
    swapIntervalMethod.disable ();
}

TEST_F (OpenGLAsynchronousVSync, TestCallSwapIntervalFailsToEnableForCopy)
{
    bool throttled;
    EXPECT_FALSE (swapIntervalMethod.enable (cgl::PartialCopy, throttled));
}

TEST_F (OpenGLAsynchronousVSync, TestUnthrottledWhereFailure)
{
    bool throttled;
    EXPECT_FALSE (swapIntervalMethod.enable (cgl::PartialCopy, throttled));
    EXPECT_FALSE (throttled);
}

TEST_F (OpenGLAsynchronousVSync, TestCallSwapIntervalUnthrottledWhereSuccess)
{
    bool throttled;
    EXPECT_CALL (functions, swapIntervalEXT (1));
    EXPECT_TRUE (swapIntervalMethod.enable (cgl::Flip, throttled));
    EXPECT_FALSE (throttled);
}

TEST_F (OpenGLBlockingVSyncTest, TestCallsGetVideoSyncAndWaitVideoSyncForCopy)
{
    bool throttled;
    EXPECT_CALL (functions, waitVideoSyncSGI (_, _, _));
    EXPECT_TRUE (waitVideoSyncMethod.enable (cgl::PartialCopy, throttled));
}

TEST_F (OpenGLBlockingVSyncTest, TestCallsGetVideoSyncAndWaitVideoSyncForFlip)
{
    bool throttled;
    EXPECT_CALL (functions, waitVideoSyncSGI (_, _, _));
    EXPECT_TRUE (waitVideoSyncMethod.enable (cgl::Flip, throttled));
}

TEST_F (OpenGLBlockingVSyncTest, TestCallsWaitVideoSyncAndThrottled)
{
    bool throttled = false;

    /* Frame 0 to frame 1 */
    EXPECT_CALL (functions, waitVideoSyncSGI (1, _, _)).WillOnce (DoAll (SetArgPointee<2> (1),
									 Return (0)));
    /* Returned next frame, this frame was throttled */
    EXPECT_TRUE (waitVideoSyncMethod.enable (cgl::PartialCopy, throttled));
    EXPECT_TRUE (throttled);
}

TEST_F (OpenGLBlockingVSyncTest, TestCallsWaitVideoSyncAndThrottledEveryFrame)
{
    bool throttled = false;

    /* Frame 0 to frame 1 */
    EXPECT_CALL (functions, waitVideoSyncSGI (1, 0, _)).WillOnce (DoAll (SetArgPointee<2> (1),
									 Return (0)));
    /* Returned next frame, this frame was throttled */
    EXPECT_TRUE (waitVideoSyncMethod.enable (cgl::PartialCopy, throttled));
    EXPECT_TRUE (throttled);

    /* Frame 1 to frame 2 */
    EXPECT_CALL (functions, waitVideoSyncSGI (1, 0, _)).WillOnce (DoAll (SetArgPointee<2> (2),
									 Return (0)));
    /* Returned next frame, this frame was throttled */
    EXPECT_TRUE (waitVideoSyncMethod.enable (cgl::PartialCopy, throttled));
    EXPECT_TRUE (throttled);
}

TEST_F (OpenGLBlockingVSyncTest, TestCallsWaitVideoSyncAndUnthrottledDueToBrokenWaitVSync)
{
    bool throttled = false;

    /* Frame 0 to frame 1 */
    EXPECT_CALL (functions, waitVideoSyncSGI (1, 0, _)).WillOnce (DoAll (SetArgPointee<2> (1),
									 Return (0)));
    /* Returned next frame, this frame was throttled */
    EXPECT_TRUE (waitVideoSyncMethod.enable (cgl::PartialCopy, throttled));
    EXPECT_TRUE (throttled);

    /* Frame 1 to frame 1 (eg, broken waitVideoSyncSGI */
    EXPECT_CALL (functions, waitVideoSyncSGI (1, 0, _)).WillOnce (DoAll (SetArgPointee<2> (1),
									 Return (0)));
    /* Returned next frame, this frame was throttled */
    EXPECT_TRUE (waitVideoSyncMethod.enable (cgl::PartialCopy, throttled));
    EXPECT_FALSE (throttled);
}
