#ifndef RESIZE_SCREEN_IMPL
#define RESIZE_SCREEN_IMPL

#include "screen-interface.h"
#include "window-impl.h"

#include <core/screen.h>

namespace resize
{

class CompScreenImpl : public CompScreenInterface
{
    public:
	CompScreenImpl (CompScreen *impl)
	    : mImpl (impl)
	{
	}

	virtual Window root ()
	{
	    return mImpl->root ();
	}

	virtual CompWindowInterface * findWindow (Window id)
	{
	    return CompWindowImpl::wrap (mImpl->findWindow (id));
	}

	virtual int xkbEvent ()
	{
	    return mImpl->xkbEvent ();
	}

	virtual void handleEvent (XEvent *event)
	{
	    mImpl->handleEvent (event);
	}

	virtual int syncEvent ()
	{
	    return mImpl->syncEvent ();
	}

	virtual Display * dpy ()
	{
	    return mImpl->dpy ();
	}

	virtual void warpPointer (int dx, int dy)
	{
	    mImpl->warpPointer (dx, dy);
	}

	virtual CompOutput::vector & outputDevs ()
	{
	    return mImpl->outputDevs ();
	}

	virtual bool otherGrabExist (const char *n, void *o)
	{
	    return mImpl->otherGrabExist (n, o);
	}

	virtual void updateGrab (CompScreen::GrabHandle handle, Cursor cursor)
	{
	    mImpl->updateGrab (handle, cursor);
	}

	virtual CompScreen::GrabHandle pushGrab (Cursor cursor, const char *name)
	{
	    return mImpl->pushGrab (cursor, name);
	}

	virtual void removeGrab (CompScreen::GrabHandle handle, CompPoint *restorePointer)
	{
	    mImpl->removeGrab (handle, restorePointer);
	}

	/* CompOption::Class */
	virtual CompOption * getOption (const CompString &name)
	{
	    return mImpl->getOption (name);
	}

	/* CompSize */
	virtual int width () const
	{
	    return mImpl->width ();
	}

	virtual int height () const
	{
	    return mImpl->height ();
	}

    private:
	CompScreen *mImpl;
};

} /* namespace resize */

#endif /* RESIZE_SCREEN_IMPL */
