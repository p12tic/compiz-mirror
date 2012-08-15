#ifndef RESIZE_SCREEN_INTERFACE_H
#define RESIZE_SCREEN_INTERFACE_H

#include <core/screen.h>

namespace resize
{

class CompWindowInterface;

/*
 * Interface between a concrete CompScreen
 * and ResizeLogic.
 *
 * An enabler for having ResizeLogic testable.
 */
class CompScreenInterface
{
    public:
	virtual ~CompScreenInterface () {}

	virtual Window root () = 0;
	virtual CompWindowInterface * findWindow (Window id) = 0;
	virtual int xkbEvent () = 0;
	virtual void handleEvent (XEvent *event) = 0;
	virtual int syncEvent () = 0;
	virtual Display * dpy () = 0;
	virtual void warpPointer (int dx, int dy) = 0;
	virtual CompOutput::vector & outputDevs () = 0;
	virtual bool otherGrabExist (const char *, void *) = 0;
	virtual void updateGrab (CompScreen::GrabHandle handle, Cursor cursor) = 0;
	virtual CompScreen::GrabHandle pushGrab (Cursor cursor, const char *name) = 0;
	virtual void removeGrab (CompScreen::GrabHandle handle, CompPoint *restorePointer) = 0;

	/* CompOption::Class */
	virtual CompOption * getOption (const CompString &name) = 0;

	/* CompSize */
	virtual int width () const = 0;
	virtual int height () const = 0;
};

} /* namespace resize */

#endif /* RESIZE_SCREEN_INTERFACE_H */
