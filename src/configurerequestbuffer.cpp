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
	    valueMask (0),
	    lockCount (0),
	    asyncServerWindow (asyncServerWindow),
	    syncServerWindow (syncServerWindow),
	    lockFactory (lockFactory)
	{
	}

	void dispatchConfigure (bool force = false);

	XWindowChanges xwc;
	unsigned int   valueMask;

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
    bool immediate = valueMask & (CWStackMode | CWSibling);
    immediate |= (valueMask & (CWWidth | CWHeight)) &&
		 asyncServerWindow->HasCustomShape ();
    immediate |= force;

    bool dispatch = !lockCount && (valueMask & allEventMasks);

    if (dispatch || immediate)
    {
	/* Immediately make the lockCount zero, as we're going
	 * to be re-locked soon */
	lockCount = 0;

	asyncServerWindow->Configure (xwc, valueMask);

	/* Reset value mask */
	valueMask = 0;

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
crb::ConfigureRequestBuffer::pushConfigureRequest (const XWindowChanges &xwc,
						   unsigned int         mask)
{
    applyChangeToXWC (xwc, priv->xwc, mask);
    priv->valueMask |= mask;

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

crb::ConfigureRequestBuffer::ConfigureRequestBuffer (cw::AsyncServerWindow                          *asyncServerWindow,
						     SyncServerWindow                               *syncServerWindow,
						     const crb::ConfigureRequestBuffer::LockFactory &factory) :
    priv (new crb::ConfigureRequestBuffer::Private (asyncServerWindow, syncServerWindow, factory))
{
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
