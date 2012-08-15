#ifndef RESIZE_MOCK_SCREEN_H
#define RESIZE_MOCK_SCREEN_H

#include <gmock/gmock.h>

#include "screen-interface.h"
#include "gl-screen-interface.h"
#include "composite-screen-interface.h"

namespace resize
{

class MockScreen :  public CompScreenInterface,
		    public GLScreenInterface,
		    public CompositeScreenInterface
{
    public:
	MOCK_METHOD0(root, Window());
	MOCK_METHOD1(findWindow, CompWindowInterface*(Window id));
	MOCK_METHOD0(xkbEvent, int());
	MOCK_METHOD1(handleEvent, void(XEvent *event));
	MOCK_METHOD0(syncEvent, int());
	MOCK_METHOD0(dpy, Display*());
	MOCK_METHOD2(warpPointer, void(int dx, int dy));
	MOCK_METHOD0(outputDevs, CompOutput::vector&());
	MOCK_METHOD2(otherGrabExist, bool(const char *, void *));
	MOCK_METHOD2(updateGrab, void(CompScreen::GrabHandle handle, Cursor cursor));
	MOCK_METHOD2(pushGrab, CompScreen::GrabHandle(Cursor cursor, const char *name));
	MOCK_METHOD2(removeGrab, void(CompScreen::GrabHandle handle, CompPoint *restorePointer));

	MOCK_METHOD1(getOption, CompOption*(const CompString &name));

	MOCK_CONST_METHOD0(width, int());
	MOCK_CONST_METHOD0(height, int());

	/* from GLSCreenInterface  */
	MOCK_METHOD2(glPaintOutputSetEnabled, void(GLScreenInterface *screen, bool enable));

	/* from CompositeScreenInterface */
	MOCK_METHOD0(compositingActive, bool());
	MOCK_METHOD1(damageRegion, void(const CompRegion &r));
};

} /* namespace resize */

#endif /* RESIZE_MOCK_SCREEN_H */
