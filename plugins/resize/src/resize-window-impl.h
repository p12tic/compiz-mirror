#ifndef RESIZE_RESIZE_WINDOW_IMPL_H
#define RESIZE_RESIZE_WINDOW_IMPL_H

#include "resize-window-interface.h"

namespace resize
{

class ResizeWindowImpl : public ResizeWindowInterface
{
    public:
	ResizeWindowImpl (ResizeWindow *impl)
	    : mImpl (impl)
	{
	}

	ResizeWindow *impl()
	{
	    return mImpl;
	}

	static ResizeWindowImpl *wrap (ResizeWindow *impl)
	{
	    if (impl)
		return new ResizeWindowImpl (impl);
	    else
		return NULL;
	}

	virtual void getStretchScale (BoxPtr pBox, float *xScale, float *yScale)
	{
	    return mImpl->getStretchScale(pBox, xScale, yScale);
	}

    private:
	ResizeWindow *mImpl;
};

} /* namespace resize */

#endif /* RESIZE_RESIZE_WINDOW_IMPL_H */
