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

compiz::composite::scheduler::PaintScheduler::PaintScheduler (PaintSchedulerDispatchBase *b) :
    mSchedulerState (0),
    mRedrawTime (1000 / 50),
    mOptimalRedrawTime (1000 / 50),
    mFPSLimiterMode (CompositeFPSLimiterModeDefault),
    mDispatchBase (b)
{
    gettimeofday (&mLastRedraw, 0);
}

compiz::composite::scheduler::PaintScheduler::~PaintScheduler ()
{
    mPaintTimer.stop ();
}

bool
compiz::composite::scheduler::PaintScheduler::schedule ()
{
    int 			    delay = 1;

    if (!mBaseDispatch->compositingActive ())
	return false;

    if (mSchedulerState & paintSchedulerPainting)
    {
	mSchedulerState |= paintSchedulerReschedule;
	return false;
    }

    if (mSchedulerState & paintSchedulerScheduled)
	return false;

    mSchedulerState |= paintSchedulerScheduled;

    if (mFPSLimiterMode == CompositeFPSLimiterModeVSyncLike ||
	(mDispatchBase->hasVSync ()))
    {
	delay = 1;
    }
    else
    {
	struct timeval now;
	gettimeofday (&now, 0);
	int elapsed = TIMEVALDIFF (&now, &mLastRedraw);
	if (elapsed < 0)
	    elapsed = 0;
 	delay = elapsed < mOptimalRedrawTime ? mOptimalRedrawTime - elapsed : 1;
    }
    /*
     * Note the use of delay = 1 instead of 0, even though 0 would be better.
     * A delay of zero is presently broken due to CompTimer bugs;
     *    1. Infinite loop in CompTimeoutSource::callback when a zero
     *       timer is set.
     *    2. Priority set too high in CompTimeoutSource::CompTimeoutSource
     *       causing the glib main event loop to be starved of X events.
     * Fixes for both of these issues are being worked on separately.
     */
    mPaintTimer.start
	(boost::bind (&compiz::composite::scheduler::PaintScheduler::dispatch, this),
	delay);

    return false;
}

int
compiz::composite::scheduler::PaintScheduler::getRedrawTime ()
{
    return mRedrawTime;
}

int
compiz::composite::scheduler::PaintScheduler::getOptimalRedrawTime ()
{
    return mOptimalRedrawTime;
}

bool
compiz::composite::scheduler::PaintScheduler::dispatch ()
{
    struct timeval                  tv;
    int                             timeDiff;

    mSchedulerState |= paintSchedulerPainting;
    mSchedulerState &= ~paintSchedulerReschedule;

    gettimeofday (&tv, 0);

    timeDiff = TIMEVALDIFF (&tv, &mLastRedraw);

    /* handle clock rollback */

    if (timeDiff < 0)
	timeDiff = 0;
    /*
     * Now that we use a "tickless" timing algorithm, timeDiff could be
     * very large if the screen is truely idle.
     * However plugins expect the old behaviour where timeDiff is never
     * larger than the frame rate (optimalRedrawTime).
     * So enforce this to keep animations timed correctly and smooth...
     */

    if (timeDiff > mOptimalRedrawTime)
	timeDiff = mOptimalRedrawTime;

    mRedrawTime = timeDiff;

    mDispatchBase->prepareScheduledPaint (timeDiff);
    mDispatchBase->paintScheduledPaint ();
    mDispatchBase->doneScheduledPaint ();


    mLastRedraw = tv;
    mSchedulerState &= ~(paintSchedulerPainting | paintSchedulerScheduled);

    if (mSchedulerState & paintSchedulerReschedule)
	schedule ();

    return false;
}

void
compiz::composite::scheduler::PaintScheduler::setRefreshRate (unsigned int rate)
{
    mOptimalRedrawTime = mRedrawTime =
	static_cast <int> (1000 / static_cast <float> (rate));
}
