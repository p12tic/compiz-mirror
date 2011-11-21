/*
 * Copyright © 2008 Dennis Kasprzyk
 * Copyright © 2007 Novell, Inc.
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
 * Authors: Dennis Kasprzyk <onestone@compiz-fusion.org>
 *          David Reveman <davidr@novell.com>
 */

#include <compiz.h>
#include <core/timer.h>
#include <composite/fpslimiter.h>

#ifndef _COMPIZ_COMPOSITE_PAINTSCHEDULER_H
#define _COMPIZ_COMPOSITE_PAINTSCHEDULER_H

namespace compiz
{
namespace composite
{
namespace scheduler
{

const unsigned int paintSchedulerScheduled = 1 << 0;
const unsigned int paintSchedulerPainting = 1 << 1;
const unsigned int paintSchedulerReschedule = 1 << 2;

class PaintSchedulerDispatchBase
{
    public:

	virtual void prepareScheduledPaint (unsigned int timeDiff) = 0;
	virtual void paintScheduledPaint () = 0;
	virtual void doneScheduledPaint () = 0;
	virtual bool schedulerCompositingActive () = 0;
	virtual bool schedulerHasVsync () = 0;
};

class PaintScheduler
{
    public:

	PaintScheduler (PaintSchedulerDispatchBase *);
	~PaintScheduler ();

	bool schedule ();
	void setFPSLimiterMode (CompositeFPSLimiterMode mode);
	CompositeFPSLimiterMode getFPSLimiterMode ();

	int getRedrawTime ();
	int getOptimalRedrawTime ();

	void setRefreshRate (unsigned int);

    protected:

	bool dispatch ();

    private:

	unsigned int   mSchedulerState;

	struct timeval mLastRedraw;
	int            mRedrawTime;
	int            mOptimalRedrawTime;

	CompositeFPSLimiterMode mFPSLimiterMode;

	CompTimer mPaintTimer;

	PaintSchedulerDispatchBase *mDispatchBase;
};

}
}
}

#endif
