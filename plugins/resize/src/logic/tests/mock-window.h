#ifndef RESIZE_MOCK_WINDOW_H
#define RESIZE_MOCK_WINDOW_H

#include <gmock/gmock.h>

#include "window-interface.h"

namespace resize
{

class MockWindow : public CompWindowInterface
{
public:
    MOCK_METHOD0(id, Window());
    MOCK_CONST_METHOD0(outputRect, CompRect());
    MOCK_METHOD0(syncAlarm, XSyncAlarm());
    MOCK_CONST_METHOD0(sizeHints, XSizeHints&());
    MOCK_CONST_METHOD0(serverGeometry, CompWindow::Geometry &());
    MOCK_CONST_METHOD0(border, CompWindowExtents&());
    MOCK_CONST_METHOD0(output, CompWindowExtents&());
    MOCK_METHOD4(constrainNewWindowSize, bool (int width,
					       int height,
					       int *newWidth,
					       int *newHeight));
    MOCK_METHOD0(syncWait, bool());
    MOCK_METHOD0(sendSyncRequest, void());
    MOCK_METHOD2(configureXWindow, void(unsigned int valueMask,
					XWindowChanges *xwc));
    MOCK_METHOD4(grabNotify, void (int x, int y,
				   unsigned int state,
				   unsigned int mask));
    MOCK_METHOD0(ungrabNotify, void());
    MOCK_METHOD0(shaded, bool());
    MOCK_CONST_METHOD0(size, CompSize());
    MOCK_METHOD0(actions, unsigned int());
    MOCK_METHOD0(type, unsigned int());
    MOCK_METHOD0(state, unsigned int &());
    MOCK_METHOD0(overrideRedirect, bool());
    MOCK_METHOD1(updateAttributes, void(CompStackingUpdateMode stackingMode));
    MOCK_METHOD0(outputDevice, int());
    MOCK_CONST_METHOD0(serverSize, const CompSize());
    MOCK_METHOD1(maximize, void(unsigned int state));
    MOCK_METHOD1(evaluate, bool(CompMatch &match));

    MOCK_METHOD0(getResizeInterface, ResizeWindowInterface*());
};

} /* namespace resize */

#endif /* RESIZE_MOCK_WINDOW_H */
