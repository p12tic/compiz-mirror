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

extern "C"
{

#include <xf86drm.h>
#include <xf86drmMode.h>

}

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

    private:

	drmVBlank       mVbl;
	unsigned int    drmFD;
};

DRMVBlankWaiter::DRMVBlankWaiter ()
{
    const char  *modules[] = { "i915", "radeon", "nouveau", "vmwgfx" };

    for (unsigned int i = 0; i < sizeof (modules) / sizeof (modules[0]); i++)
    {
	std::cout << "DEBUG: attempting to load platform " << modules[i];
	drmFD = drmOpen (modules[i], NULL);

	if (drmFD < 0)
	    std::cout << " ... failed";
	else
	{
	    std::cout << " ... success";
	    break;
	}
    }

    if (drmFD < 0)
	throw std::exception ();

    mVbl.request.type = DRM_VBLANK_RELATIVE;
    mVbl.request.sequence = 0;
}

bool
DRMVBlankWaiter::waitVBlank ()
{
    unsigned int ret = drmWaitVBlank (drmFD, &mVbl);

    if (ret != 0)
    {
	std::cout << "DEBUG: drmWaitVBlank failed! Error code: " << ret << std::endl;
	return false;
    }

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
