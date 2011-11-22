/*
 * Copyright © 2011 Canonical Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Dennis Kasprzyk not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Dennis Kasprzyk makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DENNIS KASPRZYK DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DENNIS KASPRZYK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * drm_vblank handling code adapted from libdrm
 * Copyright 2008 Tungsten Graphics
 *   Jakob Bornecrantz <jakob@tungstengraphics.com>
 * Copyright 2008 Intel Corporation
 *   Jesse Barnes <jesse.barnes@intel.com>
 *
 * Authors: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include <iostream>
#include <string>
#include <core/timeouthandler.h>
#include <paintscheduler.h>
#include <privatetimeouthandler.h>
#include <privatetimer.h>
#include <privatetimeoutsource.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/time.h>

#include <pthread.h>

extern "C"
{

#include "test-paintscheduler-set-drmvblanktype.h"

}

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

using namespace compiz::composite::scheduler;

class VBlankWaiter
{
    public:

	virtual bool waitVBlank () = 0;
};

class DRMVBlankWaiter :
    public VBlankWaiter
{
    public:

	DRMVBlankWaiter ();
	~DRMVBlankWaiter ();

	bool waitVBlank ();

	class DRMInfo
	{
	    public:

		DRMInfo (DRMVBlankWaiter *w) :
		    self (w)
		{
		}

		DRMVBlankWaiter *self;
	};

    protected:

	static void *
	drmEventHandlerThread (void *);

	static void
	vblankHandler (int          fd,
		       unsigned int frame,
		       unsigned int sec,
		       unsigned int usec,
		       void         *data);
    private:

	drmVBlank       mVbl;
	drmEventContext evctx;
	int             drmFD;
	DRMInfo         *info;
	pthread_t       eventThread;
	pthread_mutex_t eventMutex;
	pthread_cond_t  eventCondition;
	bool            pendingVBlank;
};

void
DRMVBlankWaiter::vblankHandler (int fd, unsigned int frame, unsigned int sec, unsigned int usc, void *data)
{
    drmVBlank vbl;
    DRMInfo   *info = static_cast <DRMInfo *> (data);

    setDRMVBlankType (&vbl.request.type, DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT);
    vbl.request.sequence = 1;
    vbl.request.signal = (unsigned long) data;

    drmWaitVBlank (fd, &vbl);

    pthread_mutex_lock (&info->self->eventMutex);
    info->self->pendingVBlank = true;
    pthread_cond_signal (&info->self->eventCondition);
    pthread_mutex_unlock (&info->self->eventMutex);
}

void *
DRMVBlankWaiter::drmEventHandlerThread (void *data)
{
    DRMVBlankWaiter *self = static_cast <DRMVBlankWaiter *> (data);
    std::cout << "DEBUG: libdrm event handler thread" << std::endl;

    while (true)
    {
	int    ret;
	struct pollfd pfd;

	pfd.fd = self->drmFD;
	pfd.events = POLLIN | POLLHUP;
	pfd.revents = 0;

	ret = poll (&pfd, 1, -1);

	if (ret <= 0)
	{
	    std::cout << "DEBUG: poll () failed" << std::endl;
	    perror ("poll");
	    return NULL;
	}

	ret = drmHandleEvent (self->drmFD, &self->evctx);
	if (ret != 0)
	{
	    drmError (ret, "DRMVBlankWaiter::setupEventContextThread");
	    return NULL;
	}
    }

    return NULL;
}

DRMVBlankWaiter::DRMVBlankWaiter () :
    info (new DRMInfo (this)),
    pendingVBlank (false)
{
    const char  *modules[] = { "i915", "radeon", "nouveau", "vmwgfx" };

    pthread_mutex_init (&eventMutex, NULL);
    pthread_cond_init  (&eventCondition, NULL);

    for (unsigned int i = 0; i < ARRAY_SIZE (modules); i++)
    {
	std::cout << "DEBUG: attempting to load platform " << modules[i];
	drmFD = drmOpen (modules[i], NULL);

	if (drmFD < 0)
	    std::cout << " ... failed" << std::endl;
	else
	{
	    std::cout << " ... success" << std::endl;;
	    break;
	}
    }

    if (drmFD < 0)
	throw std::exception ();

    memset (&mVbl, 0, sizeof (drmVBlank));
    mVbl.request.type = DRM_VBLANK_RELATIVE;
    mVbl.request.sequence = 0;

    int ret = drmWaitVBlank (drmFD, &mVbl);

    if (ret != 0)
    {
	drmError (ret, "DRMVBlankWaiter::DRMVBlankWaiter (RELATIVE)");
	throw std::exception ();
    }

    setDRMVBlankType (&mVbl.request.type, DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT);
    mVbl.request.sequence = 1;
    mVbl.request.signal = (unsigned long) info;

    ret = drmWaitVBlank (drmFD, &mVbl);

    if (ret != 0)
    {
	drmError (ret, "DRMVBlankWaiter::DRMVBlankWaiter (RELATIVE EVENT)");
	throw std::exception ();
    }

    memset (&evctx, 0, sizeof (drmEventContext));
    evctx.version = DRM_EVENT_CONTEXT_VERSION;
    evctx.vblank_handler = DRMVBlankWaiter::vblankHandler;
    evctx.page_flip_handler = NULL;

    if (pthread_create (&eventThread, NULL, &DRMVBlankWaiter::drmEventHandlerThread, (void *) this))
    {
	std::cout << "DEBUG: could not create thread" << std::endl;
	throw std::exception ();
    }
}

bool
DRMVBlankWaiter::waitVBlank ()
{
    pthread_mutex_lock (&eventMutex);
    if (!pendingVBlank)
	pthread_cond_wait (&eventCondition, &eventMutex);

    pendingVBlank = false;
    pthread_mutex_unlock (&eventMutex);

    return true;
}

class DummyPaintDispatch :
    public PaintSchedulerDispatchBase
{
    public:

	DummyPaintDispatch ();

	void prepareScheduledPaint (unsigned int timeDiff)
	{
	    std::cout << "DEBUG: scheduled paint time diff: " << timeDiff << std::endl;
	}

	void paintScheduledPaint ()
	{
	    std::cout << "DEBUG: paint scheduled paint" << std::endl;

	    if (waiter)
	    {
		if (!waiter->waitVBlank ())
		{
		    delete waiter;
		    waiter = NULL;
		}
	    }
	}

	void doneScheduledPaint ()
	{
	    std::cout << "DEBUG: done scheduled paint" << std::endl;

	    sched.schedule ();
	}

	bool schedulerCompositingActive ()
	{
	    return true;
	}

	bool schedulerHasVsync ()
	{
	    return true;
	}

    private:

	PaintScheduler sched;
	VBlankWaiter   *waiter;
};

DummyPaintDispatch::DummyPaintDispatch () :
    sched (this)
{
    sched.schedule ();

    try
    {
	waiter = new DRMVBlankWaiter ();
    }
    catch (std::exception &e)
    {
	throw e;
    }
}

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
    TimeoutHandler     *th = new TimeoutHandler ();
    TimeoutHandler::SetDefault (th);

    Glib::RefPtr <Glib::MainContext> ctx = Glib::MainContext::get_default ();
    Glib::RefPtr <Glib::MainLoop> mainloop = Glib::MainLoop::create (ctx, false);
    Glib::RefPtr <CompTimeoutSource> timeout = CompTimeoutSource::create (ctx);

    DummyPaintDispatch *dbp = new DummyPaintDispatch ();

    ctx->iteration (false);

    mainloop->run ();

    delete dbp;

    return 0;
}
