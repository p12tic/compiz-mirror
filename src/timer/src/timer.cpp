/*
 * Copyright © 2008 Dennis Kasprzyk
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
 * Authors: Dennis Kasprzyk <onestone@compiz-fusion.org>
 */

#include <boost/foreach.hpp>
#include <cmath>

#include "privatetimeoutsource.h"
#include "privatetimer.h"

#define foreach BOOST_FOREACH

CompTimeoutSource::CompTimeoutSource (Glib::RefPtr <Glib::MainContext> &ctx) :
    Glib::Source ()
{
    set_priority (G_PRIORITY_HIGH);
    attach (ctx);

    mLastTime = get_time ();
    
    connect (sigc::mem_fun <bool, CompTimeoutSource> (this, &CompTimeoutSource::callback));
}

CompTimeoutSource::~CompTimeoutSource ()
{
}

sigc::connection
CompTimeoutSource::connect (const sigc::slot <bool> &slot)
{
    return connect_generic (slot);
}

Glib::RefPtr <CompTimeoutSource>
CompTimeoutSource::create (Glib::RefPtr <Glib::MainContext> &ctx)
{
    return Glib::RefPtr <CompTimeoutSource> (new CompTimeoutSource (ctx));
}

#define COMPIZ_TIMEOUT_WAIT 15

bool
CompTimeoutSource::prepare (int &timeout)
{
    /* Determine time to wait */

    if (TimeoutHandler::Default ()->timers ().empty ())
    {
	/* This kind of sucks, but we have to do it, considering
	 * that glib provides us no safe way to remove the source -
	 * thankfully we shouldn't ever be hitting this case since
	 * we create the source after we start pingTimer
	 * and that doesn't stop until compiz does
	 */

	timeout = COMPIZ_TIMEOUT_WAIT;

	return true;
    }

    if (TimeoutHandler::Default ()->timers ().front ()->minLeft () > 0)
    {
	std::list<CompTimer *>::iterator it = TimeoutHandler::Default ()->timers ().begin ();

	CompTimer *t = (*it);
	timeout = t->maxLeft ();
	while (it != TimeoutHandler::Default ()->timers ().end ())
	{
	    t = (*it);
	    if (t->minLeft () >= timeout)
		break;
	    if (t->maxLeft () < timeout)
		timeout = t->maxLeft ();
	    it++;
	}

	mLastTime = get_time ();

	return false;
    }
    else
    {
	timeout = 0;

	mLastTime = get_time ();

	return true;
    }
}

bool
CompTimeoutSource::check ()
{
    gint64		    fixedTimeDiff;
    gdouble		    timeDiff;
    gint64		    currentTime = get_time ();
    bool		    ready = false;

    timeDiff = (currentTime - mLastTime) / 1000.0;

    mLastTime = currentTime;

    /* prefer over-estimating rather than waking up */
    fixedTimeDiff = ceil (timeDiff);

    /* Handle clock rollback */
    if (fixedTimeDiff < 0)
    {
	fixedTimeDiff = 0;
    }

    foreach (CompTimer *t, TimeoutHandler::Default ()->timers ())
    {
	t->decrement (fixedTimeDiff);
    }

    ready = (!TimeoutHandler::Default ()->timers ().empty () &&
	     TimeoutHandler::Default ()->timers ().front ()->minLeft () <= 0);

    return ready;
}

bool
CompTimeoutSource::dispatch (sigc::slot_base *slot)
{
    (*static_cast <sigc::slot <bool> *> (slot)) ();
    return true;
}

bool
CompTimeoutSource::callback ()
{
    while (TimeoutHandler::Default ()->timers ().begin () != TimeoutHandler::Default ()->timers ().end () &&
	   TimeoutHandler::Default ()->timers ().front ()->minLeft () <= 0)
    {
	CompTimer *t = TimeoutHandler::Default ()->timers ().front ();
	TimeoutHandler::Default ()->timers ().pop_front ();

	t->setActive (false);
	if (t->triggerCallback ())
	{
	    TimeoutHandler::Default ()->addTimer (t);
	    t->setActive (true);
	}
    }

    return !TimeoutHandler::Default ()->timers ().empty ();
}

PrivateTimer::PrivateTimer () :
    mActive (false),
    mMinTime (0),
    mMaxTime (0),
    mMinLeft (0),
    mMaxLeft (0),
    mCallBack (NULL)
{
}

PrivateTimer::~PrivateTimer ()
{
}

CompTimer::CompTimer () :
    priv (new PrivateTimer ())
{
    assert (priv);
}

CompTimer::~CompTimer ()
{
    TimeoutHandler::Default ()->removeTimer (this);
    delete priv;
}

void
CompTimer::setTimes (unsigned int min, unsigned int max)
{
    bool wasActive = priv->mActive;
    if (priv->mActive)
	stop ();
    priv->mMinTime = min;
    priv->mMaxTime = (min <= max)? max : min;

    if (wasActive)
	start ();
}

void
CompTimer::setExpiryTimes (unsigned int min, unsigned int max)
{
    priv->mMinLeft = min;
    priv->mMaxLeft = (min <= max) ? max : min;
}

void
CompTimer::decrement (unsigned int diff)
{
    priv->mMinLeft -= diff;
    priv->mMaxLeft -= diff;
}

void
CompTimer::setActive (bool active)
{
    priv->mActive = active;
}

bool
CompTimer::triggerCallback ()
{
    return priv->mCallBack ();
}

void
CompTimer::setCallback (CompTimer::CallBack callback)
{
    bool wasActive = priv->mActive;
    if (priv->mActive)
	stop ();

    priv->mCallBack = callback;

    if (wasActive)
	start ();
}


void
CompTimer::start ()
{
    stop ();

    if (priv->mCallBack.empty ())
    {
#warning compLogMessage needs to be testable
#if 0
	compLogMessage ("core", CompLogLevelWarn,
			"Attempted to start timer without callback.");
#endif
	return;
    }

    priv->mActive = true;
    TimeoutHandler::Default ()->addTimer (this);
}

void
CompTimer::start (unsigned int min, unsigned int max)
{
    setTimes (min, max);
    start ();
}

void
CompTimer::start (CompTimer::CallBack callback,
		  unsigned int min, unsigned int max)
{
    setTimes (min, max);
    setCallback (callback);
    start ();
}

void
CompTimer::stop ()
{
    priv->mActive = false;
    TimeoutHandler::Default ()->removeTimer (this);
}

unsigned int
CompTimer::minTime ()
{
    return priv->mMinTime;
}

unsigned int
CompTimer::maxTime ()
{
    return priv->mMaxTime;
}

unsigned int
CompTimer::minLeft ()
{
    return (priv->mMinLeft < 0)? 0 : priv->mMinLeft;
}

unsigned int
CompTimer::maxLeft ()
{
    return (priv->mMaxLeft < 0)? 0 : priv->mMaxLeft;
}

bool
CompTimer::active ()
{
    return priv->mActive;
}
