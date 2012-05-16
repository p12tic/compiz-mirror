#include <stdlib.h>
#include <iostream>
#include <string>
#include <grabhandler.h>

/* FIXME: Not entirely portable, but we can't
 * include window.h without pulling in bunch of
 * static initalizers */

#define CompWindowGrabKeyMask         (1 << 0)
#define CompWindowGrabButtonMask      (1 << 1)
#define CompWindowGrabMoveMask        (1 << 2)
#define CompWindowGrabResizeMask      (1 << 3)
#define CompWindowGrabExternalAppMask (1 << 4)

void pass (const std::string &message)
{
    std::cout << "PASS: " << message << std::endl;
}

void fail (const std::string &message)
{
    std::cout << "FAIL: " << message << std::endl;
    exit (1);
}

int main (void)
{
    compiz::grid::window::GrabWindowHandler moveHandler (CompWindowGrabMoveMask |
							 CompWindowGrabButtonMask);
    compiz::grid::window::GrabWindowHandler resizeHandler (CompWindowGrabButtonMask |
						    	   CompWindowGrabResizeMask);

    std::cout << "TEST: compiz::grid::window::GrabHandler" << std::endl;

    if (moveHandler.track () && !moveHandler.resetResize ())
	pass ("compiz::grid::window::GrabHandler CompWindowGrabMoveMask | CompWindowGrabButtonMask");
    else
	fail ("compiz::grid::window::GrabHandler CompWindowGrabMoveMask | CompWindowGrabButtonMask");

    if (!resizeHandler.track () && resizeHandler.resetResize ())
        pass ("compiz::grid::window::GrabHandler CompWindowGrabResizeMask | CompWindowGrabButtonMask");
    else
        fail ("compiz::grid::window::GrabHandler CompWindowGrabResizeMask | CompWindowGrabButtonMask");

    return 0;
}
