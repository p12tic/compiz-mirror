#include <gtest/gtest.h>

#include <grabhandler.h>

/* FIXME: Not entirely portable, but we can't
 * include window.h without pulling in bunch of
 * static initalizers */

#define CompWindowGrabKeyMask         (1 << 0)
#define CompWindowGrabButtonMask      (1 << 1)
#define CompWindowGrabMoveMask        (1 << 2)
#define CompWindowGrabResizeMask      (1 << 3)
#define CompWindowGrabExternalAppMask (1 << 4)

class GridGrabHandlerTest :
    public ::testing::Test
{
};

TEST(GridGrabHandlerTest, TestMoveHandler)
{
    compiz::grid::window::GrabWindowHandler moveHandler (CompWindowGrabMoveMask |
							 CompWindowGrabButtonMask);

    EXPECT_TRUE (moveHandler.track ());
    EXPECT_FALSE (moveHandler.resetResize ());
}

TEST(GridGrabHandlerTest, TestResizeHandler)
{
    compiz::grid::window::GrabWindowHandler resizeHandler (CompWindowGrabButtonMask |
						    	   CompWindowGrabResizeMask);

    EXPECT_FALSE (resizeHandler.track ());
    EXPECT_TRUE (resizeHandler.resetResize ());
}
