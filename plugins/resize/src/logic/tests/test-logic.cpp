#include <gtest/gtest.h>

#include "mock-screen.h"
#include "mock-window.h"
#include "fake-property-writer.h"

#include <core/core.h>

#include "resize-logic.h"
#include "resize_options.h"

using namespace resize;
using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::StrEq;
using ::testing::_;

class ResizeLogicTest : public ::testing::Test
{
public:
    virtual void SetUp ()
    {
	SetUpMockWindow ();
	SetUpMockScreenExpectations ();
	SetUpFakePropertyWriter ();
	InjectMocksAndFakesIntoTestSubject ();
    }

    void SetUpMockWindow ()
    {
	EXPECT_CALL (mockWindow, id ())
	    .WillRepeatedly (Return (123));

	EXPECT_CALL (mockWindow, actions ())
	    .WillRepeatedly (Return (CompWindowActionResizeMask));

	mockWindowServerGeometry.set (100 /* x */, 100 /* y */,
				      100 /* w */, 100 /* h */,
				      2 /* border */);
	EXPECT_CALL (mockWindow, serverGeometry ())
	    .WillRepeatedly (ReturnRef (mockWindowServerGeometry));

	EXPECT_CALL (mockWindow, serverSize ())
	    .WillRepeatedly (Invoke (this, &ResizeLogicTest::GetMockWindowServerSize));

	EXPECT_CALL (mockWindow, outputDevice ())
	    .WillRepeatedly (Return (0));

	EXPECT_CALL (mockWindow, evaluate (_))
	    .WillRepeatedly (Return (false));

	EXPECT_CALL (mockWindow, type ())
	    .WillRepeatedly (Return (0));

	EXPECT_CALL (mockWindow, overrideRedirect ())
	    .WillRepeatedly (Return (false));

	EXPECT_CALL (mockWindow, shaded ())
	    .WillRepeatedly (Return (false));

	mockWindowState = 0;
	EXPECT_CALL (mockWindow, state ())
	    .WillRepeatedly (ReturnPointee (&mockWindowState));

	EXPECT_CALL (mockWindow, border ())
	    .WillRepeatedly (ReturnRef (mockWindowBorder));

	EXPECT_CALL (mockWindow, grabNotify (_,_,_,_))
	    .Times(AnyNumber());

	EXPECT_CALL (mockWindow, ungrabNotify ())
	    .Times(AnyNumber());

	EXPECT_CALL (mockWindow, sendSyncRequest ())
	    .Times(AnyNumber());

	EXPECT_CALL (mockWindow, syncWait ())
	    .WillRepeatedly (Return (false));

	EXPECT_CALL (mockWindow, constrainNewWindowSize (_,_,_,_))
	    .WillRepeatedly (Return (false));

	EXPECT_CALL (mockWindow, configureXWindow (_,_))
	    .Times(AnyNumber());
    }

    void SetUpMockScreenExpectations ()
    {
	optionRaiseOnClick.setName ("raise_on_click", CompOption::TypeBool);
	optionRaiseOnClick.value ().set ((bool) false);

	EXPECT_CALL (mockScreen, getOption (StrEq ("raise_on_click")))
	    .WillRepeatedly (Return (&optionRaiseOnClick));

	outputDevices.push_back (CompOutput ());
	outputDevices[0].setGeometry(0, 0, 1280, 1024);

	EXPECT_CALL (mockScreen, outputDevs ())
	    .WillRepeatedly (ReturnRef (outputDevices));

	EXPECT_CALL (mockScreen, findWindow (123))
	    .WillRepeatedly (Return (&mockWindow));

	EXPECT_CALL (mockScreen, compositingActive ())
	    .WillRepeatedly (Return (true));

	EXPECT_CALL (mockScreen, damageRegion (_));

	EXPECT_CALL (mockScreen, syncEvent ())
	    .WillRepeatedly (Return (-XSyncAlarmNotify));

	EXPECT_CALL (mockScreen, handleEvent (_));

	EXPECT_CALL (mockScreen, dpy ())
	    .WillRepeatedly (Return ((Display*)NULL));

	EXPECT_CALL (mockScreen, xkbEvent ())
	    .WillRepeatedly (Return (0));
    }

    void SetUpFakePropertyWriter ()
    {
	fakePropWriter.mPropertyValues.resize(4);
	for (int i = 0; i < 4; i++)
	{
	    char buf[4];
	    snprintf (buf, 4, "%i", i);
	    CompString tmpName (buf);

	    fakePropWriter.mPropertyValues.at (i).setName (tmpName, CompOption::TypeInt);
	}
    }

    void InjectMocksAndFakesIntoTestSubject ()
    {
	logic.resizeInformationAtom = &fakePropWriter;
	logic.mScreen = &mockScreen;
	logic.cScreen = &mockScreen;
	logic.gScreen = &mockScreen;
	logic.options = &resizeOptions;
    }

    CompSize GetMockWindowServerSize ()
    {
	CompSize size (mockWindowServerGeometry.width(),
		       mockWindowServerGeometry.height());
	return size;
    }

    ResizeLogic logic;
    MockWindow mockWindow;
    MockScreen mockScreen;
    FakePropertyWriter fakePropWriter;
    ResizeOptions resizeOptions;
    CompOption optionRaiseOnClick;
    CompOutput::vector outputDevices;
    CompWindow::Geometry mockWindowServerGeometry;
    unsigned int mockWindowState;
    CompWindowExtents mockWindowBorder;
};


TEST_F (ResizeLogicTest, MaximizeVerticallyWhenResizedTopEdgeScreen)
{
    CompAction		action;
    CompAction::State	state = 0;
    CompOption::Vector	options;

    options.push_back (CompOption ("window", CompOption::TypeInt));
    options[0].value ().set ((int) 123);

    options.push_back (CompOption ("external", CompOption::TypeBool));
    options[1].value ().set (true);

    EXPECT_CALL (mockScreen, otherGrabExist (StrEq ("resize"), _))
	.WillOnce (Return (false));

    EXPECT_CALL (mockScreen, pushGrab (_, StrEq ("resize")))
	.WillOnce (Return ((CompScreen::GrabHandle)1));

    /* hit the middle of the top edge of the window  */
    pointerX = mockWindowServerGeometry.centerX ();
    pointerY = mockWindowServerGeometry.top();

    logic.initiateResizeDefaultMode(&action, state, options);

    /* move the pointer to the top of the screen */
    pointerY = 1;

    XEvent event;
    event.type = MotionNotify;
    event.xmotion.root = 345;

    EXPECT_CALL (mockScreen, root ())
	.WillRepeatedly (Return (event.xmotion.root));

    logic.handleEvent(&event);

    EXPECT_CALL (mockScreen, removeGrab (_, _));

    EXPECT_CALL (mockWindow, maximize (CompWindowStateMaximizedVertMask))
	.Times(1)
        .WillRepeatedly(SaveArg<0>(&mockWindowState));

    logic.terminateResize(&action, state, options);
}

TEST_F (ResizeLogicTest, DontMaximizeVerticallyIfFarFromTopEdge)
{
    CompAction		action;
    CompAction::State	state = 0;
    CompOption::Vector	options;

    options.push_back (CompOption ("window", CompOption::TypeInt));
    options[0].value ().set ((int) 123);

    options.push_back (CompOption ("external", CompOption::TypeBool));
    options[1].value ().set (true);

    EXPECT_CALL (mockScreen, otherGrabExist (StrEq ("resize"), _))
	.WillOnce (Return (false));

    EXPECT_CALL (mockScreen, pushGrab (_, StrEq ("resize")))
	.WillOnce (Return ((CompScreen::GrabHandle)1));

    /* hit the middle of the top edge of the window  */
    pointerX = mockWindowServerGeometry.centerX ();
    pointerY = mockWindowServerGeometry.top();

    logic.initiateResizeDefaultMode(&action, state, options);

    /* move the pointer just a bit upwards. Still far away from the top edge of
     * the screen */
    pointerY -= 10;

    XEvent event;
    event.type = MotionNotify;
    event.xmotion.root = 345;

    EXPECT_CALL (mockScreen, root ())
	.WillRepeatedly (Return (event.xmotion.root));

    logic.handleEvent(&event);

    EXPECT_CALL (mockScreen, removeGrab (_, _));

    EXPECT_CALL (mockWindow, maximize (_))
	.Times(0);

    logic.terminateResize(&action, state, options);

}
