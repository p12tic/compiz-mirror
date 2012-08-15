#ifndef RESIZE_COMPOSITE_SCREEN_IMPL_H
#define RESIZE_COMPOSITE_SCREEN_IMPL_H

#include "composite-screen-interface.h"

#include <composite/composite.h>

namespace resize
{

class CompositeScreenImpl : public CompositeScreenInterface
{
    public:
	CompositeScreenImpl (CompositeScreen *impl)
	    : mImpl (impl)
	{
	}

	virtual ~CompositeScreenImpl() {}

	virtual bool compositingActive ()
	{
	    return mImpl->compositingActive ();
	}

	virtual void damageRegion (const CompRegion &r)
	{
	    mImpl->damageRegion (r);
	}

	static CompositeScreenImpl *wrap (CompositeScreen *impl)
	{
	    if (impl)
		return new CompositeScreenImpl (impl);
	    else
		return NULL;
	}

    private:
	CompositeScreen *mImpl;
};

} /* namespace resize */

#endif /* RESIZE_COMPOSITE_SCREEN_IMPL_H */
