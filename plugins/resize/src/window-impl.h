#ifndef RESIZE_WINDOW_IMPL_H
#define RESIZE_WINDOW_IMPL_H

#include <core/window.h>
#include "window-interface.h"

#include "resize-window-impl.h"

namespace resize
{

class CompWindowImpl : public CompWindowInterface
{
    public:
	CompWindowImpl (CompWindow *impl)
	    : mImpl (impl)
	{
	    mResizeImpl = ResizeWindowImpl::wrap(ResizeWindow::get (impl));
	}

	CompWindow *impl()
	{
	    return mImpl;
	}

	static CompWindowImpl *wrap (CompWindow *impl)
	{
	    if (impl)
		return new CompWindowImpl (impl);
	    else
		return NULL;
	}

	virtual Window id ()
	{
	    return mImpl->id ();
	}

	virtual CompRect outputRect () const
	{
	    return mImpl->outputRect ();
	}

	virtual XSyncAlarm syncAlarm ()
	{
	    return mImpl->syncAlarm ();
	}

	virtual XSizeHints & sizeHints () const
	{
	    return mImpl->sizeHints ();
	}

	virtual CompWindow::Geometry & serverGeometry () const
	{
	    return mImpl->serverGeometry ();
	}

	virtual CompWindowExtents & border () const
	{
	    return mImpl->border ();
	}

	virtual CompWindowExtents & output () const
	{
	    return mImpl->output ();
	}

	virtual bool constrainNewWindowSize (int width,
					     int height,
					     int *newWidth,
					     int *newHeight)
	{
	    return mImpl->constrainNewWindowSize (width,
						 height,
						 newWidth,
						 newHeight);
	}

	virtual bool syncWait ()
	{
	    return mImpl->syncWait ();
	}

	virtual void sendSyncRequest ()
	{
	    mImpl->sendSyncRequest ();
	}

	virtual void configureXWindow (unsigned int valueMask,
				       XWindowChanges *xwc)
	{
	    mImpl->configureXWindow (valueMask, xwc);
	}

	virtual void grabNotify (int x, int y,
				 unsigned int state, unsigned int mask)
	{
	    mImpl->grabNotify (x, y, state, mask);
	}

	virtual void ungrabNotify ()
	{
	    mImpl->ungrabNotify ();
	}

	virtual bool shaded ()
	{
	    return mImpl->shaded ();
	}

	virtual CompSize size () const
	{
	    return mImpl->size ();
	}

	virtual unsigned int actions ()
	{
	    return mImpl->actions ();
	}

	virtual unsigned int type ()
	{
	    return mImpl->type ();
	}

	virtual unsigned int & state ()
	{
	    return mImpl->state ();
	}

	virtual bool overrideRedirect ()
	{
	    return mImpl->overrideRedirect ();
	}

	virtual void updateAttributes (CompStackingUpdateMode stackingMode)
	{
	    mImpl->updateAttributes (stackingMode);
	}

	virtual int outputDevice ()
	{
	    return mImpl->outputDevice ();
	}

	virtual const CompSize serverSize () const
	{
	    return mImpl->serverSize ();
	}

	virtual void maximize (unsigned int state = 0)
	{
	    mImpl->maximize (state);
	}

	virtual bool evaluate (CompMatch &match)
	{
	    return match.evaluate (mImpl);
	}

	virtual ResizeWindowInterface *getResizeInterface ()
	{
	    return mResizeImpl;
	}

    private:
	CompWindow *mImpl;
	ResizeWindowImpl *mResizeImpl;
};

} /* namespace resize */

#endif /* RESIZE_WINDOW_IMPL_H */
