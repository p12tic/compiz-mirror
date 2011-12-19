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

class DummyPaintDispatch;

void pass (const std::string &message)
{
    std::cout << "PASS: " << message << std::endl;
}

void fail (const std::string &message)
{
    std::cout << "FAIL: " << message << std::endl;
    exit (1);
}

class VBlankWaiter
{
    public:

	typedef timeval time_value;

	VBlankWaiter (DummyPaintDispatch *owner, unsigned int n);
	virtual ~VBlankWaiter ();

	virtual bool waitVBlank () = 0;
	virtual bool hasVSync () = 0;

	bool checkTimings (float req, float threshold);
	float averagePeriod ();
	float averagePhase ();

	void addPaintStart (unsigned int time);

    protected:

	std::vector <float> periods;
	std::vector <VBlankWaiter::time_value> paintPeriods;
	std::vector <VBlankWaiter::time_value> blankPeriods;
	unsigned int        timingCount;
	DummyPaintDispatch  *owner;
};

class DRMVBlankWaiter :
    public VBlankWaiter
{
    public:

	DRMVBlankWaiter (DummyPaintDispatch *owner, unsigned int n);
	~DRMVBlankWaiter ();

	bool waitVBlank ();
	bool hasVSync ();

	class DRMInfo
	{
	    public:

		DRMInfo (DRMVBlankWaiter *w) :
		    self (w)
		{
		}

		DRMVBlankWaiter *self;
	};

	static float threshold;

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
	bool		done;
};

class NilVBlankWaiter :
    public VBlankWaiter
{
    public:

	NilVBlankWaiter (DummyPaintDispatch *, unsigned int);
	~NilVBlankWaiter ();

	bool waitVBlank ()
	{
	    timingCount++;
	    return timingCount < periods.size ();
	}

	bool hasVSync () { return false; }
};

class DummyPaintDispatch :
    public PaintSchedulerDispatchBase
{
    public:

	DummyPaintDispatch (Glib::RefPtr <Glib::MainLoop> loop);

	void prepareScheduledPaint (unsigned int timeDiff)
	{
	    waiter->addPaintStart (timeDiff);
	}

	void paintScheduledPaint ()
	{
	    if (waiter)
	    {
		success = waiter->waitVBlank ();
	    }
	    else
		success = false;
	}

	void doneScheduledPaint ()
	{
	    if (waiter && success)
		sched.schedule ();
	    else
		ml->quit ();
	}

	bool schedulerCompositingActive ()
	{
	    return true;
	}

	bool schedulerHasVsync ()
	{
	    if (waiter)
		return waiter->hasVSync ();

	    return true;
	}

	void setWaiter (VBlankWaiter *w)
	{
	    if (waiter)
		delete waiter;

	    waiter = w;
	}

	void setRefreshRate (unsigned int r)
	{
	    sched.setRefreshRate (r);
	}

	bool isDone () { return !(waiter && success); }

    private:

	PaintScheduler sched;
	VBlankWaiter   *waiter;
	bool           success;
	Glib::RefPtr <Glib::MainLoop> ml;
};


float DRMVBlankWaiter::threshold = 7.0f;

float
VBlankWaiter::averagePeriod ()
{
    float buf = 0.0f;

    for (std::vector <float>::iterator it = periods.begin ();
	 it != periods.end (); it++)
	buf += (*it);

    buf = buf / (periods.size ());

    return buf;
}

/* Returns false if a value exists outside
 * the threshold */
bool
VBlankWaiter::checkTimings (float req, float threshold)
{
    /* Ignore the first ten as they are synchronization bits */
    std::vector <float>::iterator it = periods.begin ();

    assert (periods.size () > 10);
    std::advance (it, 10);

    for (; it != periods.end (); it++)
    {
	if ((((*it) <= req - threshold) ||
	     ((*it) >= req + threshold)))
	{
	    std::cout << "DEBUG: failing value " << (*it) << std::endl;
	    return false;
	}
    }

    std::vector <VBlankWaiter::time_value>::iterator pit = paintPeriods.begin ();
    std::vector <VBlankWaiter::time_value>::iterator vit = blankPeriods.begin ();

    assert (paintPeriods.size () > 10);
    assert (blankPeriods.size () > 10);

    std::advance (pit, 10);
    std::advance (vit, 10);

    /* Check if any blank periods fall outside of -4ms
     * of the paint period, that means we're out of phase. */

    for (; vit != blankPeriods.end (); vit++, pit++)
    {
	if (pit->tv_usec - vit->tv_usec > 4000)
	{
	   std::cout << "DEBUG: failing values blank: " << vit->tv_usec << " paint: " << pit->tv_usec << " out of phase!" << std::endl;
	   return false;
	}
    }

    return true;
}

void
VBlankWaiter::addPaintStart (unsigned int time)
{
    if (timingCount < periods.size ())
	periods[timingCount]= time;
}

VBlankWaiter::VBlankWaiter (DummyPaintDispatch *o, unsigned int n) :
    periods (n),
    paintPeriods (n),
    blankPeriods (n),
    timingCount (0),
    owner (o)
{
}

VBlankWaiter::~VBlankWaiter  ()
{
}

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

    if (info->self->timingCount == info->self->periods.size ())
	info->self->done = true;
    else
    {
	info->self->pendingVBlank = true;
	info->self->timingCount++;
    }

    VBlankWaiter::time_value tv;

    gettimeofday (&tv, NULL);

    info->self->blankPeriods.push_back (tv);
	
    pthread_cond_signal (&info->self->eventCondition);
    pthread_mutex_unlock (&info->self->eventMutex);
}

void *
DRMVBlankWaiter::drmEventHandlerThread (void *data)
{
    DRMVBlankWaiter *self = static_cast <DRMVBlankWaiter *> (data);

    while (!self->owner->isDone ())
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

DRMVBlankWaiter::DRMVBlankWaiter (DummyPaintDispatch *o, unsigned int n) :
    VBlankWaiter (o, n),
    info (new DRMInfo (this)),
    pendingVBlank (false),
    done (false)
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

DRMVBlankWaiter::~DRMVBlankWaiter ()
{
    pthread_join (eventThread, NULL);

    delete info;

    drmClose (drmFD);
}

NilVBlankWaiter::NilVBlankWaiter (DummyPaintDispatch *o, unsigned int n) :
    VBlankWaiter (o, n)
{
}

NilVBlankWaiter::~NilVBlankWaiter ()
{
}

bool
DRMVBlankWaiter::hasVSync ()
{
    return true;
}

bool
DRMVBlankWaiter::waitVBlank ()
{
    VBlankWaiter::time_value tv;

    pthread_mutex_lock (&eventMutex);
    if (!pendingVBlank)
	pthread_cond_wait (&eventCondition, &eventMutex);

    pendingVBlank = false;
    pthread_mutex_unlock (&eventMutex);

    gettimeofday (&tv, NULL);

    info->self->paintPeriods.push_back (tv);

    return !done;
}

DummyPaintDispatch::DummyPaintDispatch (Glib::RefPtr <Glib::MainLoop> mainloop) :
    sched (this),
    waiter (NULL),
    success (true),
    ml (mainloop)
{
    sched.schedule ();
}

int main (void)
{
    TimeoutHandler     *th = new TimeoutHandler ();
    TimeoutHandler::SetDefault (th);

    Glib::RefPtr <Glib::MainContext> ctx = Glib::MainContext::get_default ();
    Glib::RefPtr <Glib::MainLoop> mainloop = Glib::MainLoop::create (ctx, false);
    Glib::RefPtr <CompTimeoutSource> timeout = CompTimeoutSource::create (ctx);

    DummyPaintDispatch *dpb = new DummyPaintDispatch (mainloop);

    try
    {

	DRMVBlankWaiter    *dvbwaiter = new DRMVBlankWaiter (dpb, 300);

	dpb->setWaiter (dvbwaiter);

	mainloop->run ();

	float averagePeriod = dvbwaiter->averagePeriod ();

	std::cout << "DEBUG: average vblank wait time was " << averagePeriod << std::endl;
	std::cout << "DEBUG: average frame rate " << 1000 / averagePeriod << " Hz" << std::endl;
	std::cout << "TEST: time " << averagePeriod << " within threshold of " << DRMVBlankWaiter::threshold << std::endl;

	if (dvbwaiter->checkTimings (averagePeriod, DRMVBlankWaiter::threshold))
	    pass ("hardware vblank timings");
	else
	    fail ("hardware vblank timings");
    }
    catch (std::exception &e)
    {
	std::cout << "WARN: can't test DRM vblanking!" << std::endl;
    }

    dpb->setWaiter (NULL);

    delete dpb;
    delete th;

    th = new TimeoutHandler ();
    TimeoutHandler::SetDefault (th);

    mainloop = Glib::MainLoop::create (ctx, false);
    timeout = CompTimeoutSource::create (ctx);

    dpb = new DummyPaintDispatch (mainloop);

    NilVBlankWaiter *nwaiter = new NilVBlankWaiter (dpb, 100);

    dpb->setRefreshRate (50);
    dpb->setWaiter (nwaiter);

    mainloop->run ();

    std::cout << "TEST: refreshRate " << 50 << " within threshold of " << 10 << std::endl;

    if (nwaiter->checkTimings (1000 / 50, 10))
	pass ("50 Hz refresh rate");
    else
	fail ("50 Hz refresh rate");

    dpb->setWaiter (NULL);

    delete dpb;
    delete th;

    th = new TimeoutHandler ();
    TimeoutHandler::SetDefault (th);

    mainloop = Glib::MainLoop::create (ctx, false);
    timeout = CompTimeoutSource::create (ctx);

    dpb = new DummyPaintDispatch (mainloop);

    NilVBlankWaiter *nwaiter2 = new NilVBlankWaiter (dpb, 100);

    dpb->setRefreshRate (30);
    dpb->setWaiter (nwaiter2);

    mainloop->run ();

    std::cout << "TEST: refreshRate " << 30 << " within threshold of " << 10 << std::endl;

    if (nwaiter2->checkTimings (1000 / 30, 10))
	pass ("30 Hz refresh rate");
    else
	fail ("30 Hz refresh rate");

    dpb->setWaiter (NULL);

    delete dpb;
    delete th;

    th = new TimeoutHandler ();
    TimeoutHandler::SetDefault (th);

    mainloop = Glib::MainLoop::create (ctx, false);
    timeout = CompTimeoutSource::create (ctx);

    dpb = new DummyPaintDispatch (mainloop);

    NilVBlankWaiter *nwaiter3 = new NilVBlankWaiter (dpb, 100);

    dpb->setRefreshRate (100);
    dpb->setWaiter (nwaiter3);

    mainloop->run ();

    std::cout << "TEST: refreshRate " << 100 << " within threshold of " << 10 << std::endl;

    if (nwaiter3->checkTimings (1000 / 100, 10))
	pass ("100 Hz refresh rate");
    else
	fail ("100 Hz refresh rate");

    dpb->setWaiter (NULL);

    delete dpb;
    delete th;

    return 0;
}
