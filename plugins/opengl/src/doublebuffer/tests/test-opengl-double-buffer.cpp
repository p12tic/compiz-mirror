#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <opengl/doublebuffer.h>

using namespace compiz::opengl;
using testing::_;
using testing::StrictMock;
using testing::Return;

class MockGLDoubleBuffer :
    public GLDoubleBufferInterface
{
    public:

	MOCK_CONST_METHOD1 (swap, void (bool));
	MOCK_CONST_METHOD0 (blitAvailable, bool ());
	MOCK_CONST_METHOD1 (blit, void (const CompRegion &));
	MOCK_CONST_METHOD0 (fallbackBlitAvailable, bool ());
	MOCK_CONST_METHOD1 (fallbackBlit, void (const CompRegion &));
};

class CompizOpenGLDoubleBufferTest :
    public ::testing::Test
{
    public:

	MockGLDoubleBuffer mglbb;
	CompRegion	 blitRegion;

};

class CompizOpenGLDoubleBufferDeathTest :
    public CompizOpenGLDoubleBufferTest
{
};

TEST_F(CompizOpenGLDoubleBufferTest, TestPaintedFullAlwaysSwaps)
{
    EXPECT_CALL (mglbb, swap (false));

    mglbb.render (blitRegion, true);
}

TEST_F(CompizOpenGLDoubleBufferTest, TestNoPaintedFullscreenOrFBOAlwaysBlitsSubBuffer)
{
    EXPECT_CALL (mglbb, blitAvailable ()).WillOnce (Return (true));
    EXPECT_CALL (mglbb, blit (_));

    mglbb.render (blitRegion, false);
}

TEST_F(CompizOpenGLDoubleBufferTest, TestNoPaintedFullscreenOrFBODoesNotBlitIfNotSupported)
{

}

TEST_F(CompizOpenGLDoubleBufferTest, TestBlitExactlyWithRegionSpecified)
{
    CompRegion r1 (0, 0, 100, 100);
    CompRegion r2 (100, 100, 100, 100);
    CompRegion r3 (200, 200, 100, 100);

    EXPECT_CALL (mglbb, blitAvailable ()).WillRepeatedly (Return (true));

    EXPECT_CALL (mglbb, blit (r1));
    mglbb.render (r1, false);

    EXPECT_CALL (mglbb, blit (r2));
    mglbb.render (r2, false);

    EXPECT_CALL (mglbb, blit (r3));
    mglbb.render (r3, false);
}

TEST_F(CompizOpenGLDoubleBufferDeathTest, TestNoPaintedFullscreenOrFBODoesNotBlitOrCopyIfNotSupportedAndDies)
{
    StrictMock <MockGLDoubleBuffer> mglbbStrict;

    ON_CALL (mglbbStrict, blitAvailable ()).WillByDefault (Return (false));
    ON_CALL (mglbbStrict, fallbackBlitAvailable ()).WillByDefault (Return (false));

    ASSERT_DEATH ({
		    mglbbStrict.render (blitRegion, false);
		  },
		  ".fatal.");
}

TEST_F(CompizOpenGLDoubleBufferTest, TestSubBufferCopyIfNoFBOAndNoSubBufferBlit)
{
    StrictMock <MockGLDoubleBuffer> mglbbStrict;

    EXPECT_CALL (mglbbStrict, blitAvailable ()).WillOnce (Return (false));
    EXPECT_CALL (mglbbStrict, fallbackBlitAvailable ()).WillOnce (Return (true));
    EXPECT_CALL (mglbbStrict, fallbackBlit (blitRegion));

    mglbbStrict.render (blitRegion, false);
}
