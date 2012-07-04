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

	MOCK_METHOD0 (swapBuffers, void ());
	MOCK_METHOD0 (subBufferBlitAvailable, bool ());
	MOCK_METHOD1 (subBufferBlit, void (const CompRegion &));
};

TEST(CompizOpenGLBufferBlitTest, TestTest)
{
}
