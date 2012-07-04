#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <opengl/bufferblit.h>

using namespace compiz::opengl;
using testing::_;
using testing::StrictMock;
using testing::Return;

class MockGLBufferBlit :
    public GLBufferBlitInterface
{
    public:

	MOCK_CONST_METHOD0 (swapBuffers, void ());
	MOCK_CONST_METHOD0 (subBufferBlitAvailable, bool ());
	MOCK_CONST_METHOD1 (subBufferBlit, void (const CompRegion &));
};

class CompizOpenGLBufferBlitTest :
    public ::testing::Test
{
    public:

	MockGLBufferBlit mglbb;
	CompRegion	 blitRegion;

};

class CompizOpenGLBufferBlitDeathTest :
    public CompizOpenGLBufferBlitTest
{
};

TEST_F(CompizOpenGLBufferBlitTest, TestPaintedWithFBOAlwaysSwaps)
{
    EXPECT_CALL (mglbb, swapBuffers ());

    blitBuffers (PaintedWithFramebufferObject, blitRegion, mglbb);
}

TEST_F(CompizOpenGLBufferBlitTest, TestPaintedFullscreenAlwaysSwaps)
{
    EXPECT_CALL (mglbb, swapBuffers ());

    blitBuffers (PaintedFullscreen, blitRegion, mglbb);
}

TEST_F(CompizOpenGLBufferBlitTest, TestNoPaintedFullscreenOrFBOAlwaysBlitsSubBuffer)
{
    EXPECT_CALL (mglbb, subBufferBlitAvailable ()).WillOnce (Return (true));
    EXPECT_CALL (mglbb, subBufferBlit (_));

    blitBuffers (0, blitRegion, mglbb);
}

TEST_F(CompizOpenGLBufferBlitTest, TestNoPaintedFullscreenOrFBODoesNotBlitIfNotSupported)
{

}

TEST_F(CompizOpenGLBufferBlitTest, TestBlitExactlyWithRegionSpecified)
{
    CompRegion r1 (0, 0, 100, 100);
    CompRegion r2 (100, 100, 100, 100);
    CompRegion r3 (200, 200, 100, 100);

    EXPECT_CALL (mglbb, subBufferBlitAvailable ()).WillRepeatedly (Return (true));

    EXPECT_CALL (mglbb, subBufferBlit (r1));
    blitBuffers (0, r1, mglbb);

    EXPECT_CALL (mglbb, subBufferBlit (r2));
    blitBuffers (0, r2, mglbb);

    EXPECT_CALL (mglbb, subBufferBlit (r3));
    blitBuffers (0, r3, mglbb);
}

TEST_F(CompizOpenGLBufferBlitDeathTest, TestNoPaintedFullscreenOrFBODoesNotBlitIfNotSupportedAndDies)
{
    StrictMock <MockGLBufferBlit> mglbbStrict;

    ON_CALL (mglbbStrict, subBufferBlitAvailable ()).WillByDefault (Return (false));

    ASSERT_DEATH ({
		    blitBuffers (0, blitRegion, mglbbStrict);
		  },
		  ".fatal.");
}
