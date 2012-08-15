#ifndef RESIZE_WINDOW_INTERFACE
#define RESIZE_WINDOW_INTERFACE

#include <core/window.h>

namespace resize
{

class ResizeWindowInterface;

/*
 * Interface between a concrete CompWindow
 * and ResizeLogic.
 *
 * An enabler for having ResizeLogic testable.
 */
class CompWindowInterface
{
    public:
	virtual ~CompWindowInterface () {}

	virtual Window id () = 0;
	virtual CompRect outputRect () const = 0;
	virtual XSyncAlarm syncAlarm () = 0;
	virtual XSizeHints & sizeHints () const = 0;
	virtual CompWindow::Geometry & serverGeometry () const = 0;
	virtual CompWindowExtents & border () const = 0;
	virtual CompWindowExtents & output () const = 0;
	virtual bool constrainNewWindowSize (int width,
					     int height,
					     int *newWidth,
					     int *newHeight) = 0;
	virtual bool syncWait () = 0;
	virtual void sendSyncRequest () = 0;
	virtual void configureXWindow (unsigned int valueMask,
				       XWindowChanges *xwc) = 0;
	virtual void grabNotify (int x, int y,
				 unsigned int state, unsigned int mask) = 0;
	virtual void ungrabNotify () = 0;
	virtual bool shaded () = 0;
	virtual CompSize size () const = 0;
	virtual unsigned int actions () = 0;
	virtual unsigned int type () = 0;
	virtual unsigned int & state () = 0;
	virtual bool overrideRedirect () = 0;
	virtual void updateAttributes (CompStackingUpdateMode stackingMode) = 0;
	virtual int outputDevice () = 0;
	virtual const CompSize serverSize () const = 0;
	virtual void maximize (unsigned int state = 0) = 0;

	/* equivalent of CompMatch::evaluate  */
	virtual bool evaluate (CompMatch &match) = 0;

	virtual ResizeWindowInterface *getResizeInterface () = 0;
};

} /* namespace resize */

#endif /* RESIZE_WINDOW_INTERFACE */
