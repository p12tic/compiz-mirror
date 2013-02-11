#include "pixmap-requests.h"
#include <boost/foreach.hpp>
#include <algorithm>

#ifndef foreach
#define foreach BOOST_FOREACH
#endif

DecorPixmap::DecorPixmap (Pixmap pixmap, PixmapDestroyQueue::Ptr d) :
    mPixmap (pixmap),
    mDeletor (d)
{
}

DecorPixmap::~DecorPixmap ()
{
    mDeletor->postDeletePixmap (mPixmap);
}

Pixmap
DecorPixmap::getPixmap ()
{
    return mPixmap;
}

X11DecorPixmapReceiver::X11DecorPixmapReceiver (DecorPixmapRequestorInterface *requestor,
						DecorationInterface *decor) :
    mUpdateState (0),
    mDecorPixmapRequestor (requestor),
    mDecoration (decor)
{
}

void
X11DecorPixmapReceiver::pending ()
{
    if (mUpdateState & X11DecorPixmapReceiver::UpdateRequested)
	mUpdateState |= X11DecorPixmapReceiver::UpdatesPending;
    else
    {
	mUpdateState |= X11DecorPixmapReceiver::UpdateRequested;

	mDecorPixmapRequestor->postGenerateRequest (mDecoration->getFrameType (),
						    mDecoration->getFrameState (),
						    mDecoration->getFrameActions ());
    }
}

void X11DecorPixmapReceiver::update ()
{
    if (mUpdateState & X11DecorPixmapReceiver::UpdatesPending)
	mDecorPixmapRequestor->postGenerateRequest (mDecoration->getFrameType (),
						    mDecoration->getFrameState (),
						    mDecoration->getFrameActions ());

    mUpdateState = 0;
}

X11DecorPixmapRequestor::X11DecorPixmapRequestor (Display *dpy,
						  Window  window,
						  DecorationListFindMatchingInterface *listFinder) :
    mDpy (dpy),
    mWindow (window),
    mListFinder (listFinder)
{
}

int
X11DecorPixmapRequestor::postGenerateRequest (unsigned int frameType,
					      unsigned int frameState,
					      unsigned int frameActions)
{
    return decor_post_generate_request (mDpy,
					 mWindow,
					 frameType,
					 frameState,
					 frameActions);
}

void
X11DecorPixmapRequestor::handlePending (long *data)
{
    DecorationInterface::Ptr d = mListFinder->findMatchingDecoration (static_cast <unsigned int> (data[0]),
								      static_cast <unsigned int> (data[1]),
								      static_cast <unsigned int> (data[2]));

    if (d)
	d->receiverInterface ().pending ();
    else
	postGenerateRequest (static_cast <unsigned int> (data[0]),
			     static_cast <unsigned int> (data[1]),
			     static_cast <unsigned int> (data[2]));
}

namespace
{
typedef PixmapReleasePool::FreePixmapFunc FreePixmapFunc;
}

PixmapReleasePool::PixmapReleasePool (const FreePixmapFunc &freePixmap) :
    mFreePixmap (freePixmap)
{
}

void
PixmapReleasePool::markUnused (Pixmap pixmap)
{
}

int
PixmapReleasePool::postDeletePixmap (Pixmap pixmap)
{
    return 0;
}

namespace cd = compiz::decor;
namespace cdp = compiz::decor::protocol;

cd::IterationHandlerBase::IterationHandlerBase (const DecorationListFindMatchingInterface &fm) :
    listFinder (fm)
{
}

cd::PendingHandler::PendingHandler (const DecorationListFindMatchingInterface &fm) :
    IterationHandlerBase (fm)
{
}

void
cd::PendingHandler::handleMessage (unsigned int frameType,
				   unsigned int frameState,
				   unsigned int frameActions)
{
}

cd::UnusedHandler::UnusedHandler (const DecorationListFindMatchingInterface &fm,
				  const UnusedPixmapQueue::Ptr &queue,
				  const FreePixmapFunc         &freePixmap) :
    IterationHandlerBase (fm),
    mQueue (queue),
    mFreePixmap (freePixmap)
{
}

void
cd::UnusedHandler::handleMessage (Pixmap)
{
}

cdp::Communicator::Communicator (const PendingMessage      &pending,
				 const PixmapUnusedMessage &pixmapUnused) :
    mPendingHandler (pending),
    mPixmapUnusedHander (pixmapUnused)
{
}

void
cdp::Communicator::handleClientMessage (const XClientMessageEvent &xce)
{
}
