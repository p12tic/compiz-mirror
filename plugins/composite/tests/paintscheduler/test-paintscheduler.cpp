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
#include <glib.h>
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

#include <cmath>

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

class Worker
{
    public:

	/* Amount of time it takes to complete some work in ms */
	static void setWorkFactor (unsigned int workFactor) { mWorkFactor = workFactor; }
	static void work ()
	{
	    struct timespec ts;

	    ts.tv_sec = 0;
	    ts.tv_nsec = 1000000 * mWorkFactor;

	    nanosleep (&ts, NULL);
	}

    private:

	static unsigned int mWorkFactor;
};

unsigned int Worker::mWorkFactor = 0;

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

	bool addPaintStart (unsigned int time);
	void addPaintDone (const VBlankWaiter::time_value &);

    protected:

	void addVBlank (const VBlankWaiter::time_value &);

	std::vector <float> periods;
	std::vector <VBlankWaiter::time_value> paintPeriods;
	std::vector <VBlankWaiter::time_value> blankPeriods;
	unsigned int        timingCount;
	unsigned int	    paintCount;
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

    protected:

	static gpointer
	drmEventHandlerThread (gpointer);

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
	GThread    	*eventThread;
	GMutex		eventMutex;
	GCond		eventCondition;
	bool            pendingVBlank;
};

class SleepVBlankWaiter :
    public VBlankWaiter
{
    public:

	SleepVBlankWaiter (DummyPaintDispatch *, unsigned int);
	~SleepVBlankWaiter ();

	bool waitVBlank ();
	bool hasVSync () { return true; }

	static VBlankWaiter::time_value start_time;

    private:

	const static int vblank_ms_period = 60;
};

VBlankWaiter::time_value SleepVBlankWaiter::start_time;

class NilVBlankWaiter :
    public VBlankWaiter
{
    public:

	NilVBlankWaiter (DummyPaintDispatch *, unsigned int);
	~NilVBlankWaiter ();

	bool waitVBlank ();

	bool hasVSync () { return false; }
};

class DummyPaintDispatch :
    public PaintSchedulerDispatchBase
{
    public:

	DummyPaintDispatch (Glib::RefPtr <Glib::MainLoop> loop);

	void prepareScheduledPaint (unsigned int timeDiff)
	{
	    success = waiter->addPaintStart (timeDiff);
	}

	void paintScheduledPaint ()
	{
	    Worker::work ();
	}

	bool syncScheduledPaint ()
	{
	    if (waiter && !isDone ())
		return waiter->waitVBlank ();
	    else
		return false;
	}

	void doneScheduledPaint ()
	{
	    VBlankWaiter::time_value tv;
	    gettimeofday (&tv, NULL);

	    waiter->addPaintDone (tv);

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
	    if (isDone ())
		return false;

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

namespace
{
CompTimeoutSource *timeout;
}

float
VBlankWaiter::averagePeriod ()
{
    float buf = 0.0f;

    for (std::vector <float>::iterator it = periods.begin ();
	 it != periods.end (); it++)
    {
	buf += (*it);
    }

    buf = buf / (periods.size ());

    return buf;
}

/* Returns false if a value exists outside
 * the threshold */
bool
VBlankWaiter::checkTimings (float req, float threshold)
{
    std::sort (periods.begin (), periods.end ());

    float median = periods[periods.size () / 2];
    std::vector <float> deviationPeriods (periods.size ());

    unsigned int i = 0;

    for (std::vector <float>::iterator it = periods.begin ();
	it != periods.end (); it++, i++)
	deviationPeriods[i] = fabsf ((*it) - median);

    std::sort (deviationPeriods.begin (), deviationPeriods.end ());

    float medianDeviation = deviationPeriods[deviationPeriods.size () / 2];

    std::cout << "DEBUG: median absolute deviation of the periods was " << medianDeviation << std::endl;

    if (medianDeviation > threshold)
	return false;

    /* No phase checks if there is no vblanking */
    if (hasVSync ())
    {

	/* The phase needs to be sensitive to outliers
	* so use the standard deviation with an acceptable
	* deviation of 1 */

	std::vector <VBlankWaiter::time_value>::iterator pit = paintPeriods.begin ();
	std::vector <VBlankWaiter::time_value>::iterator vit = blankPeriods.begin ();
	float sum = 0.0f;
	float mean = 0.0f;

	/* Check if any blank periods fall outside of -4ms
	* of the paint period, that means we're out of phase. */

	for (; vit != blankPeriods.end (); vit++, pit++)
	    sum += ((pit->tv_usec - vit->tv_usec) / 1000.0f);

	mean = sum / blankPeriods.size ();

	pit = paintPeriods.begin ();
	vit = blankPeriods.begin ();

	sum = 0.0f;

	for (; vit != blankPeriods.end (); vit++, pit++)
	    sum += pow (((pit->tv_usec - vit->tv_usec) / 1000.0f) - mean, 2);

	sum /= blankPeriods.size ();

	std::cout << "DEBUG: st. dev of the phases was " << sqrt (sum) << std::endl;

	if (sqrt (sum) > 0.025f)
	    return false;
    }

    return true;
}

bool
VBlankWaiter::addPaintStart (unsigned int time)
{
    if (timingCount < periods.size ())
	periods[timingCount]= time;
    else
	return false;

    ++timingCount;

    return true;
}

void
VBlankWaiter::addPaintDone (const VBlankWaiter::time_value &tv)
{
    if (paintCount < paintPeriods.size ())
        paintPeriods[paintCount] = tv;
    ++paintCount;
}

void
VBlankWaiter::addVBlank (const VBlankWaiter::time_value &tv)
{
    /* There may be multiple vblanks per paint
     * because of throttling, so use the last slot
     * available for the paint period */
    if (paintCount < blankPeriods.size ())
	blankPeriods[paintCount] = tv;
}

VBlankWaiter::VBlankWaiter (DummyPaintDispatch *o, unsigned int n) :
    periods (n),
    paintPeriods (n),
    blankPeriods (n),
    timingCount (0),
    paintCount (0),
    owner (o)
{
}

VBlankWaiter::~VBlankWaiter  ()
{
}

SleepVBlankWaiter::SleepVBlankWaiter (DummyPaintDispatch *o, unsigned int n) :
    VBlankWaiter (o, n)
{
}

SleepVBlankWaiter::~SleepVBlankWaiter ()
{
}

bool
SleepVBlankWaiter::waitVBlank ()
{
    /* Wait until the next interval of 16ms */
    VBlankWaiter::time_value tv;
    gettimeofday (&tv, NULL);

    unsigned long usecIntervalWait = 16000 - (tv.tv_usec % 16000);

    struct timespec slp;

    slp.tv_sec = 0;
    slp.tv_nsec = usecIntervalWait * 1000;

    nanosleep (&slp, NULL);

    gettimeofday (&tv, NULL);

    addVBlank (tv);

    return timingCount < periods.size ();
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

    g_mutex_lock (&info->self->eventMutex);

    if (info->self->timingCount != info->self->periods.size ())
	info->self->pendingVBlank = true;

    g_cond_signal (&info->self->eventCondition);
    g_mutex_unlock (&info->self->eventMutex);
}

gpointer
DRMVBlankWaiter::drmEventHandlerThread (gpointer data)
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
    pendingVBlank (false)
{
    const char  *modules[] = { "i915", "radeon", "nouveau", "vmwgfx" };

    g_mutex_init (&eventMutex);
    g_cond_init  (&eventCondition);

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

    eventThread = g_thread_try_new ("drm_wait_t", &DRMVBlankWaiter::drmEventHandlerThread, (void *) this, NULL);

    if (!eventThread)
    {
	std::cout << "DEBUG: could not create thread" << std::endl;
	throw std::exception ();
    }
}

DRMVBlankWaiter::~DRMVBlankWaiter ()
{
    g_thread_join (eventThread);

    delete info;

    drmClose (drmFD);
}

NilVBlankWaiter::NilVBlankWaiter (DummyPaintDispatch *o, unsigned int n) :
    VBlankWaiter (o, n)
{
}

bool
NilVBlankWaiter::waitVBlank ()
{
    /* Just add a new "vblank" */
    VBlankWaiter::time_value tv;
    gettimeofday (&tv, NULL);

    addVBlank (tv);

    return timingCount < periods.size ();
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

    g_mutex_lock (&eventMutex);
    if (!pendingVBlank && !owner->isDone ())
	g_cond_wait (&eventCondition, &eventMutex);

    pendingVBlank = false;
    g_mutex_unlock (&eventMutex);

    gettimeofday (&tv, NULL);

    addVBlank (tv);

    return true;
}

DummyPaintDispatch::DummyPaintDispatch (Glib::RefPtr <Glib::MainLoop> mainloop) :
    sched (this),
    waiter (NULL),
    success (true),
    ml (mainloop)
{
    sched.schedule ();
}

bool doTest (const std::string &testName, int refreshRate, float workFactor, bool vsync, const Glib::RefPtr <Glib::MainLoop> &ml)
{
    DummyPaintDispatch *dpb = new DummyPaintDispatch (ml);
    VBlankWaiter       *vbwaiter = NULL;

    if (vsync)
    {
	try
	{
	    vbwaiter = new DRMVBlankWaiter (dpb, 200);
	}
	catch (std::exception &e)
	{
	    std::cout << "WARN: can't test DRM vblanking! Using hardcoded nanosleep () based waiter" << std::endl;
	    vbwaiter = new SleepVBlankWaiter (dpb, 200);
	}
    }
    else
	vbwaiter = new NilVBlankWaiter (dpb, 200);

    dpb->setWaiter (vbwaiter);
    dpb->setRefreshRate (refreshRate);

    Worker::setWorkFactor ((1000 / refreshRate) * workFactor);

    std::cout << "INFO: " << testName << " with refresh rate of " << refreshRate << "Hz and work factor of " << workFactor << (vsync ? " with vertical sync" : " without vertical sync") << std::endl;

    ml->run ();

    float averagePeriod = vbwaiter->averagePeriod ();

    std::cout << "DEBUG: average vblank wait time was " << averagePeriod << std::endl;
    std::cout << "DEBUG: average frame rate " << 1000 / averagePeriod << " Hz" << std::endl;
    std::cout << "TEST: " << testName << " time " << averagePeriod << " within threshold of " << 10.0f << std::endl;

    if (vbwaiter->checkTimings (averagePeriod, 10.0f) &&
	1000 / averagePeriod < refreshRate)
	pass (testName);
    else
	fail (testName);

    dpb->setWaiter (NULL);

    delete dpb;

    return true;
}
   
int main (void)
{
    gettimeofday (&SleepVBlankWaiter::start_time, NULL);

    TimeoutHandler     *th = new TimeoutHandler ();
    TimeoutHandler::SetDefault (th);

    Glib::RefPtr <Glib::MainContext> ctx = Glib::MainContext::get_default ();
    Glib::RefPtr <Glib::MainLoop> mainloop = Glib::MainLoop::create (ctx, false);
    timeout = CompTimeoutSource::create (ctx);

    doTest ("unthrottled vblank timings", 60, 0.0f, true, mainloop);
    doTest ("no vsync 60 Hz refresh rate", 60, 0.0f, false, mainloop);
    doTest ("unthrottled vblank timings", 60, 0.8f, true, mainloop);
    doTest ("no vsync 60 Hz refresh rate", 60, 0.8f, false, mainloop);
    doTest ("vsync 50 Hz refresh rate", 50, 0.0f, true, mainloop);
    doTest ("no vsync 20 Hz refresh rate", 30, 0.0f,false, mainloop);
    doTest ("vsync 50 Hz refresh rate", 50, 1.6f, true, mainloop);
    doTest ("no vsync 20 Hz refresh rate", 30, 1.6f,false, mainloop);
    doTest ("vsync 30 Hz refresh rate", 30, 0.0f, true, mainloop);
    doTest ("no vsync 100 Hz refresh rate", 100, 0.0f, false, mainloop);
    doTest ("vsync 100 Hz refresh rate", 100, 0.0f, true, mainloop);
    doTest ("no vsync 100 Hz refresh rate", 100, 2.8f, false, mainloop);
    doTest ("vsync 100 Hz refresh rate", 100, 2.8f, true, mainloop);

    delete timeout;
    delete th;

    return 0;
}
