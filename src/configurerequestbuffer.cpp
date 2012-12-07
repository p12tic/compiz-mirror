/*
 * Copyright © 2012 Sam Spilsbury
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Canonical Ltd. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Canonical Ltd. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * CANONICAL, LTD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL CANONICAL, LTD. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authored by: Sam Spilsbury <smspillaz@gmail.com>
 */
#include <cassert>
#include <boost/foreach.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/bind.hpp>
#include "asyncserverwindow.h"
#include "configurerequestbuffer-impl.h"

#ifndef foreach
#define foreach BOOST_FOREACH
#endif

namespace crb = compiz::window::configure_buffers;
namespace cw = compiz::window;

class crb::ConfigureRequestBuffer::Private
{
    public:

	typedef crb::Lockable::Weak LockObserver;

	Private (cw::AsyncServerWindow                          *asyncServerWindow,
		 cw::SyncServerWindow                           *syncServerWindow,
		 const crb::ConfigureRequestBuffer::LockFactory &lockFactory) :
	    clientChangeMask (0),
	    wrapperChangeMask (0),
	    frameChangeMask (0),
	    lockCount (0),
	    asyncServerWindow (asyncServerWindow),
	    syncServerWindow (syncServerWindow),
	    lockFactory (lockFactory)
	{
	}

	void dispatchConfigure (bool force = false);

	XWindowChanges clientChanges;
	unsigned int   clientChangeMask;

	XWindowChanges wrapperChanges;
	unsigned int   wrapperChangeMask;

	XWindowChanges frameChanges;
	unsigned int   frameChangeMask;

	unsigned int   lockCount;

	cw::AsyncServerWindow *asyncServerWindow;
	cw::SyncServerWindow  *syncServerWindow;

	crb::ConfigureRequestBuffer::LockFactory lockFactory;
	std::vector <LockObserver>               locks;
};

void
crb::ConfigureRequestBuffer::Private::dispatchConfigure (bool force)
{
    const unsigned int allEventMasks = 0x7f;
    bool immediate = frameChangeMask & (CWStackMode | CWSibling);
    immediate |= (frameChangeMask & (CWWidth | CWHeight)) &&
		 asyncServerWindow->hasCustomShape ();
    immediate |= force;

    bool clientDispatch = (clientChangeMask & allEventMasks);
    bool wrapperDispatch = (wrapperChangeMask & allEventMasks);
    bool frameDispatch  = (frameChangeMask & allEventMasks);

    bool dispatch = !lockCount && (clientDispatch ||
				   wrapperDispatch ||
				   frameDispatch);

    if (dispatch || immediate)
    {
	/* Immediately make the lockCount zero, as we're going
	 * to be re-locked soon */
	lockCount = 0;

	if (frameDispatch)
	{
	    asyncServerWindow->requestConfigureOnFrame (frameChanges,
							frameChangeMask);
	    frameChangeMask = 0;
	}

	if (wrapperDispatch)
	{
	    asyncServerWindow->requestConfigureOnWrapper (wrapperChanges,
							  wrapperChangeMask);
	    wrapperChangeMask = 0;
	}

	if (clientDispatch)
	{
	    asyncServerWindow->requestConfigureOnClient (clientChanges,
							 clientChangeMask);
	    clientChangeMask = 0;
	}

	foreach (const LockObserver &lock, locks)
	{
	    crb::Lockable::Ptr strongLock (lock.lock ());
	    strongLock->lock ();
	}
    }
}

void
crb::ConfigureRequestBuffer::freeze ()
{
    priv->lockCount++;

    assert (priv->lockCount < priv->locks.size ());
}

void
crb::ConfigureRequestBuffer::release ()
{
    assert (priv->lockCount);

    priv->lockCount--;

    priv->dispatchConfigure ();
}

namespace
{
void applyChangeToXWC (const XWindowChanges &from,
		       XWindowChanges       &to,
		       unsigned int         mask)
{
    if (mask & CWX)
	to.x = from.x;

    if (mask & CWY)
	to.y = from.y;

    if (mask & CWWidth)
	to.width = from.width;

    if (mask & CWHeight)
	to.height = from.height;

    if (mask & CWBorderWidth)
	to.border_width = from.border_width;

    if (mask & CWSibling)
	to.sibling = from.sibling;

    if (mask & CWStackMode)
	to.stack_mode = from.stack_mode;
}
}

void
crb::ConfigureRequestBuffer::pushClientRequest (const XWindowChanges &xwc,
						unsigned int         mask)
{
    applyChangeToXWC (xwc, priv->clientChanges, mask);
    priv->clientChangeMask |= mask;

    priv->dispatchConfigure ();
}

void
crb::ConfigureRequestBuffer::pushWrapperRequest (const XWindowChanges &xwc,
						 unsigned int         mask)
{
    applyChangeToXWC (xwc, priv->wrapperChanges, mask);
    priv->wrapperChangeMask |= mask;

    priv->dispatchConfigure ();
}

void
crb::ConfigureRequestBuffer::pushFrameRequest (const XWindowChanges &xwc,
					       unsigned int         mask)
{
    applyChangeToXWC (xwc, priv->frameChanges, mask);
    priv->frameChangeMask |= mask;

    priv->dispatchConfigure ();
}

crb::Releasable::Ptr
crb::ConfigureRequestBuffer::obtainLock ()
{
    crb::BufferLock::Ptr lock (priv->lockFactory (this));

    lock->lock ();
    priv->locks.push_back (crb::Lockable::Weak (lock));

    return lock;
}

namespace
{
bool isLock (const crb::Lockable::Weak &lockable,
	     crb::BufferLock           *lock)
{
    crb::Lockable::Ptr strongLockable (lockable.lock ());

    /* Asserting that the lock did not go away without telling
     * us first */
    assert (strongLockable.get ());

    if (strongLockable.get () == lock)
	return true;

    return false;
}
}

void
crb::ConfigureRequestBuffer::untrackLock (crb::BufferLock *lock)
{
    std::remove_if (priv->locks.begin (),
		    priv->locks.end (),
		    boost::bind (isLock, _1, lock));
}

bool crb::ConfigureRequestBuffer::queryAttributes (XWindowAttributes &attrib) const
{
    /* This is a little ugly, however the queryAttributes method from
     * the SyncServerWindow interface is const, and the queue really does
     * have to be released before we can query attributes from the server */
    const_cast <crb::ConfigureRequestBuffer::Private *> (priv.get ())->dispatchConfigure (true);
    return priv->syncServerWindow->queryAttributes (attrib);
}

bool crb::ConfigureRequestBuffer::queryFrameAttributes (XWindowAttributes &attrib) const
{
    /* This is a little ugly, however the queryFrameAttributes method from
     * the SyncServerWindow interface is const, and the queue really does
     * have to be released before we can query attributes from the server */
    const_cast <crb::ConfigureRequestBuffer::Private *> (priv.get ())->dispatchConfigure (true);
    return priv->syncServerWindow->queryFrameAttributes (attrib);
}

crb::ConfigureRequestBuffer::ConfigureRequestBuffer (AsyncServerWindow                          *asyncServerWindow,
						     SyncServerWindow                           *syncServerWindow,
						     const crb::ConfigureRequestBuffer::LockFactory &factory) :
    priv (new crb::ConfigureRequestBuffer::Private (asyncServerWindow, syncServerWindow, factory))
{
}

compiz::window::configure_buffers::Buffer::Ptr
crb::ConfigureRequestBuffer::Create (AsyncServerWindow *asyncServerWindow,
				     SyncServerWindow  *syncServerWindow,
				     const LockFactory &factory)
{
    return crb::Buffer::Ptr (new crb::ConfigureRequestBuffer (asyncServerWindow,
							      syncServerWindow,
							      factory));
}

class crb::ConfigureBufferLock::Private
{
    public:

	Private (crb::CountedFreeze *freezable) :
	    freezable (freezable),
	    armed (false)
	{
	}

	crb::CountedFreeze *freezable;
	bool               armed;
};

crb::ConfigureBufferLock::ConfigureBufferLock (crb::CountedFreeze *freezable) :
    priv (new crb::ConfigureBufferLock::Private (freezable))
{
}

void
crb::ConfigureBufferLock::lock ()
{
    if (!priv->armed)
	priv->freezable->freeze ();

    priv->armed = true;
}

void
crb::ConfigureBufferLock::release ()
{
    if (priv->armed)
	priv->freezable->release ();

    priv->armed = false;
}
