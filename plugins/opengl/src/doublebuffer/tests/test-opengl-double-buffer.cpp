#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <opengl/doublebuffer.h>
#include <vsyncmethod.h>

using namespace compiz::opengl;
using testing::_;
using testing::StrictMock;
using testing::Return;
using testing::SetArgReferee;

class MockDoubleBuffer :
    public DoubleBuffer
{
    public:

	MockDoubleBuffer (const std::list <VSyncMethod::Ptr> &vsyncMethods) :
	    DoubleBuffer (vsyncMethods)
	{
	}

	MOCK_CONST_METHOD0 (swap, void ());
	MOCK_CONST_METHOD0 (blitAvailable, bool ());
	MOCK_CONST_METHOD1 (blit, void (const CompRegion &));
	MOCK_CONST_METHOD0 (fallbackBlitAvailable, bool ());
	MOCK_CONST_METHOD1 (fallbackBlit, void (const CompRegion &));
	MOCK_CONST_METHOD0 (copyFrontToBack, void ());
};

class MockVSyncMethod :
    public VSyncMethod
{
    public:

	MOCK_METHOD2 (enable, bool (BufferSwapType, bool &));
	MOCK_METHOD0 (disable, void ());
};

class DoubleBufferTest :
    public ::testing::Test
{
    public:

	DoubleBufferTest () :
	    db (vsyncMethods)
	{
	}

	std::list <VSyncMethod::Ptr> vsyncMethods;
	MockDoubleBuffer             db;
	CompRegion	             blitRegion;

};

class CompizOpenGLDoubleBufferDeathTest :
    public DoubleBufferTest
{
};

TEST_F(DoubleBufferTest, TestPaintedFullAlwaysSwaps)
{
    EXPECT_CALL (db, swap ());
    EXPECT_CALL (db, copyFrontToBack ()).Times (0);

    db.render (blitRegion, true);
}

TEST_F(DoubleBufferTest, TestNoPaintedFullscreenOrFBOAlwaysBlitsSubBuffer)
{
    EXPECT_CALL (db, blitAvailable ()).WillOnce (Return (true));
    EXPECT_CALL (db, blit (_));
    EXPECT_CALL (db, copyFrontToBack ()).Times (0);

    db.render (blitRegion, false);
}

TEST_F(DoubleBufferTest, SwapWithoutFBO)
{
    db.set (DoubleBuffer::HAVE_PERSISTENT_BACK_BUFFER, false);
    db.set (DoubleBuffer::NEED_PERSISTENT_BACK_BUFFER, true);

    EXPECT_CALL (db, swap ());
    EXPECT_CALL (db, copyFrontToBack ()).Times (1);

    db.render (blitRegion, true);
}

TEST_F(DoubleBufferTest, BlitWithoutFBO)
{
    db.set (DoubleBuffer::HAVE_PERSISTENT_BACK_BUFFER, false);
    db.set (DoubleBuffer::NEED_PERSISTENT_BACK_BUFFER, false);

    EXPECT_CALL (db, blitAvailable ()).WillRepeatedly (Return (true));
    EXPECT_CALL (db, blit (_));
    EXPECT_CALL (db, swap ()).Times (0);
    EXPECT_CALL (db, copyFrontToBack ()).Times (0);

    db.render (blitRegion, false);
}

TEST_F(DoubleBufferTest, TestNoPaintedFullscreenOrFBODoesNotBlitIfNotSupported)
{

}

TEST_F(DoubleBufferTest, TestBlitExactlyWithRegionSpecified)
{
    CompRegion r1 (0, 0, 100, 100);
    CompRegion r2 (100, 100, 100, 100);
    CompRegion r3 (200, 200, 100, 100);

    EXPECT_CALL (db, blitAvailable ()).WillRepeatedly (Return (true));

    EXPECT_CALL (db, blit (r1));
    db.render (r1, false);

    EXPECT_CALL (db, blit (r2));
    db.render (r2, false);

    EXPECT_CALL (db, blit (r3));
    db.render (r3, false);
}

TEST_F(CompizOpenGLDoubleBufferDeathTest, TestNoPaintedFullscreenOrFBODoesNotBlitOrCopyIfNotSupportedAndDies)
{
    std::list <VSyncMethod::Ptr> vsyncMethods;
    StrictMock <MockDoubleBuffer> dbStrict (vsyncMethods);

    ON_CALL (dbStrict, blitAvailable ()).WillByDefault (Return (false));
    ON_CALL (dbStrict, fallbackBlitAvailable ()).WillByDefault (Return (false));

    ASSERT_DEATH ({
		    dbStrict.render (blitRegion, false);
		  },
		  ".*");
}

TEST_F(DoubleBufferTest, TestSubBufferCopyIfNoFBOAndNoSubBufferBlit)
{
    std::list <VSyncMethod::Ptr> vsyncMethods;
    StrictMock <MockDoubleBuffer> dbStrict (vsyncMethods);

    EXPECT_CALL (dbStrict, blitAvailable ()).WillOnce (Return (false));
    EXPECT_CALL (dbStrict, fallbackBlitAvailable ()).WillOnce (Return (true));
    EXPECT_CALL (dbStrict, fallbackBlit (blitRegion));

    dbStrict.render (blitRegion, false);
}

TEST (DoubleBufferVSyncTest, TestCallWorkingStrategy)
{
    boost::shared_ptr <MockVSyncMethod> mockVSyncMethod (new MockVSyncMethod ());
    std::list <VSyncMethod::Ptr>        vsyncMethods;

    vsyncMethods.push_back (mockVSyncMethod);

    MockDoubleBuffer doubleBuffer (vsyncMethods);

    EXPECT_CALL (*mockVSyncMethod, enableForBufferSwapType (Flip, _))
	    .WillOnce (Return (true));

    doubleBuffer.vsync (Flip);
}

TEST (DoubleBufferVSyncTest, TestCallNextWorkingStrategy)
{
    boost::shared_ptr <MockVSyncMethod> mockVSyncMethod (new MockVSyncMethod ());
    boost::shared_ptr <MockVSyncMethod> mockVSyncMethodSafer (new MockVSyncMethod ());
    std::list <VSyncMethod::Ptr>        vsyncMethods;

    vsyncMethods.push_back (mockVSyncMethod);
    vsyncMethods.push_back (mockVSyncMethodSafer);

    MockDoubleBuffer doubleBuffer (vsyncMethods);

    /* This one fails */
    EXPECT_CALL (*mockVSyncMethod, enableForBufferSwapType (PartialCopy, _))
	    .WillOnce (Return (false));
    /* It needs to be deactivated */
    EXPECT_CALL (*mockVSyncMethod, disable ());
    /* Try the next one */
    EXPECT_CALL (*mockVSyncMethodSafer, enableForBufferSwapType (PartialCopy, _))
	    .WillOnce (Return (true));

    doubleBuffer.vsync (PartialCopy);
}

TEST (DoubleBufferVSyncTest, TestCallPrevCallNextPrevDeactivated)
{
    boost::shared_ptr <MockVSyncMethod> mockVSyncMethod (new MockVSyncMethod ());
    boost::shared_ptr <MockVSyncMethod> mockVSyncMethodSafer (new MockVSyncMethod ());
    std::list <VSyncMethod::Ptr>        vsyncMethods;

    vsyncMethods.push_back (mockVSyncMethod);
    vsyncMethods.push_back (mockVSyncMethodSafer);

    MockDoubleBuffer doubleBuffer (vsyncMethods);

    /* This one fails */
    EXPECT_CALL (*mockVSyncMethod, enableForBufferSwapType (PartialCopy, _))
	    .WillOnce (Return (false));
    /* It needs to be deactivated */
    EXPECT_CALL (*mockVSyncMethod, disable ());
    /* Try the next one */
    EXPECT_CALL (*mockVSyncMethodSafer, enableForBufferSwapType (PartialCopy, _))
	    .WillOnce (Return (true));

    doubleBuffer.vsync (PartialCopy);

    /* Now the previous one works */
    EXPECT_CALL (*mockVSyncMethod, enableForBufferSwapType (PartialCopy, _))
	    .WillOnce (Return (true));
    /* Previous one must be deactivated */
    EXPECT_CALL (*mockVSyncMethodSafer, disable ());

    doubleBuffer.vsync (PartialCopy);
}

TEST (DoubleBufferVSyncTest, TestReportNoHardwareVSyncIfMoreThan5UnthrottledFrames)
{
    boost::shared_ptr <MockVSyncMethod> mockVSyncMethod (new MockVSyncMethod ());
    std::list <VSyncMethod::Ptr>        vsyncMethods;

    vsyncMethods.push_back (mockVSyncMethod);

    /* Always succeed */
    ON_CALL (*mockVSyncMethod, enableForBufferSwapType (PartialCopy, _)).WillByDefault (Return (true));

    MockDoubleBuffer doubleBuffer (vsyncMethods);

    /* This one succeeds but fails to throttle */
    for (unsigned int i = 0; i < 5; ++i)
    {
	EXPECT_CALL (*mockVSyncMethod, enableForBufferSwapType (PartialCopy, _))
		.WillOnce (SetArgReferee <1> (false));

	doubleBuffer.vsync (PartialCopy);
    }

    EXPECT_FALSE (doubleBuffer.hardwareVSyncFunctional ());
}

TEST (DoubleBufferVSyncTest, TestRestoreReportHardwareVSync)
{
    boost::shared_ptr <MockVSyncMethod> mockVSyncMethod (new MockVSyncMethod ());
    std::list <VSyncMethod::Ptr>        vsyncMethods;

    vsyncMethods.push_back (mockVSyncMethod);

    MockDoubleBuffer doubleBuffer (vsyncMethods);

    /* Always succeed */
    ON_CALL (*mockVSyncMethod, enableForBufferSwapType (PartialCopy, _)).WillByDefault (Return (true));

    /* This one succeeds but fails to throttle */
    for (unsigned int i = 0; i < 5; ++i)
    {
	EXPECT_CALL (*mockVSyncMethod, enableForBufferSwapType (PartialCopy, _))
		.WillOnce (SetArgReferee <1> (false));

	EXPECT_TRUE (doubleBuffer.hardwareVSyncFunctional ());

	doubleBuffer.vsync (PartialCopy);
    }

    EXPECT_FALSE (doubleBuffer.hardwareVSyncFunctional ());

    /* It works again */
    EXPECT_CALL (*mockVSyncMethod, enableForBufferSwapType (PartialCopy, _))
	    .WillOnce (SetArgReferee <1> (true));

    doubleBuffer.vsync (PartialCopy);

    /* And should report to work for another 5 bad frames */
    for (unsigned int i = 0; i < 5; ++i)
    {
	EXPECT_CALL (*mockVSyncMethod, enableForBufferSwapType (PartialCopy, _))
		.WillOnce (SetArgReferee <1> (false));

	EXPECT_TRUE (doubleBuffer.hardwareVSyncFunctional ());

	doubleBuffer.vsync (PartialCopy);
    }

    EXPECT_FALSE (doubleBuffer.hardwareVSyncFunctional ());
}
