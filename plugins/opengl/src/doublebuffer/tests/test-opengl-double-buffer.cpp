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

	MOCK_CONST_METHOD0 (swapBuffers, void ());
	MOCK_CONST_METHOD0 (subBufferBlitAvailable, bool ());
	MOCK_CONST_METHOD1 (subBufferBlit, void (const CompRegion &));
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

TEST_F(CompizOpenGLDoubleBufferTest, TestPaintedWithFBOAlwaysSwaps)
{
    EXPECT_CALL (mglbb, swapBuffers ());

    blitBuffers (PaintedWithFramebufferObject, blitRegion, mglbb);
}

TEST_F(CompizOpenGLDoubleBufferTest, TestPaintedFullscreenAlwaysSwaps)
{
    EXPECT_CALL (mglbb, swapBuffers ());

    blitBuffers (PaintedFullscreen, blitRegion, mglbb);
}

TEST_F(CompizOpenGLDoubleBufferTest, TestNoPaintedFullscreenOrFBOAlwaysBlitsSubBuffer)
{
    EXPECT_CALL (mglbb, subBufferBlitAvailable ()).WillOnce (Return (true));
    EXPECT_CALL (mglbb, subBufferBlit (_));

    blitBuffers (0, blitRegion, mglbb);
}

TEST_F(CompizOpenGLDoubleBufferTest, TestNoPaintedFullscreenOrFBODoesNotBlitIfNotSupported)
{

}

TEST_F(CompizOpenGLDoubleBufferTest, TestBlitExactlyWithRegionSpecified)
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

TEST_F(CompizOpenGLDoubleBufferDeathTest, TestNoPaintedFullscreenOrFBODoesNotBlitIfNotSupportedAndDies)
{
    StrictMock <MockGLDoubleBuffer> mglbbStrict;

    ON_CALL (mglbbStrict, subBufferBlitAvailable ()).WillByDefault (Return (false));

    ASSERT_DEATH ({
		    blitBuffers (0, blitRegion, mglbbStrict);
		  },
		  ".fatal.");
}
