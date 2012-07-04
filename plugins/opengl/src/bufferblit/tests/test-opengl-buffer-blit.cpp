#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <opengl/bufferblit.h>

using namespace compiz::opengl;
using testing::_;

class CompizOpenGLBufferBlitTest :
    public ::testing::Test
{
};

class MockGLBufferBlit :
    public GLBufferBlitInterface
{
    public:

	MOCK_CONST_METHOD0 (swapBuffers, void ());
	MOCK_CONST_METHOD0 (subBufferBlitAvailable, bool ());
	MOCK_CONST_METHOD1 (subBufferBlit, void (const CompRegion &));
};

TEST(CompizOpenGLBufferBlitTest, TestPaintedWithFBOAlwaysSwaps)
{
    MockGLBufferBlit mglbb;
    CompRegion	     blitRegion (infiniteRegion);

    EXPECT_CALL (mglbb, swapBuffers ());

    blitBuffers (PaintedWithFramebufferObject, blitRegion, mglbb);
}
