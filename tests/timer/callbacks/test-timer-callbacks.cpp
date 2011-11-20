/*
 * Copyright © 2011 Canonical Ltd.
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
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include "test-timer.h"
#include <ctime>

static time_t starttime = 0;

bool
CompTimerTestCallbacks::cb (int timernum)
{
    static bool complete = false;
    static int count[4] = {0, 0, 0, 0};

    if (timernum < 4)
	count[timernum]++;

    if (complete)
	return false;

    if (time (NULL) - starttime > 5)  /* Wait no more than 5 seconds */
    {
	std::cout << "FAIL: some timers are never being triggered" << std::endl;
	exit (1);
	return false;
    }
    else if (lastTimerTriggered == 0 && timernum != 3)
    {
	std::cout << "FAIL: timer 3 was not triggered first" << std::endl;
	exit (1);
	return false;
    }
    else if (lastTimerTriggered == 2 && timernum != 3)
    {
	std::cout << "FAIL: timer 3 was not after 2" << std::endl;
	exit (1);
	return false;
    }
    else if (timernum == 1 && count[2] < 1)
    {
	std::cout << "FAIL: timer 1 was not preceded by 2" << std::endl;
	exit (1);
	return false;
    }

    if (timernum < 3 || lastTimerTriggered != 3)
    {
	std::cout
	    << "INFO: triggering timer "
	    << timernum
	    << ((timernum == 3) ? " (many times)" : "")
	    << std::endl;
    }

    lastTimerTriggered = timernum;

    if (timernum == 1)
    {
	complete = true;
	std::cout
	    << "PASS: basic timers. Total calls: "
	    << count[1]
	    << ", "
	    << count[2]
	    << ", "
	    << count[3]
	    << std::endl;
	ml->quit ();
    }

    return !complete;
}

void
CompTimerTestCallbacks::precallback ()
{
    starttime = time (NULL);

    /* Test 2: Adding timers */
    std::cout << "-= TEST: adding timers and callbacks" << std::endl;

    CompTimer *t1 = new CompTimer ();
    timers.push_back (t1);
    timers.front ()->setTimes (100, 110);
    timers.front ()->setCallback (boost::bind (&CompTimerTestCallbacks::cb, this, 1));

    /* TimeoutHandler::timers should be empty */
    if (!TimeoutHandler::Default ()->timers ().empty ())
    {
	std::cout << "FAIL: timers list is not empty" << std::endl;
	exit (1);
    }

    CompTimer *t2 = new CompTimer ();
    timers.push_back (t2);
    timers.back ()->setTimes (50, 90);
    timers.back ()->setCallback (boost::bind (&CompTimerTestCallbacks::cb, this, 2));

    CompTimer *t3 = new CompTimer ();
    timers.push_back (t3);
    timers.back ()->setTimes (0, 0);
    timers.back ()->setCallback (boost::bind (&CompTimerTestCallbacks::cb, this, 3));

    /* Start all timers */
    t1->start ();
    t2->start ();
    t3->start ();

    /* TimeoutHandler::timers should have the timer that
     * is going to trigger first at the front of the
     * list and the last timer at the back */
    if (TimeoutHandler::Default ()->timers ().front () != timers.back ())
    {
	std::cout << "FAIL: timer with the least time is not at the front" << std::endl;
	std::cout << "INFO: TimeoutHandler::Default ().size " << TimeoutHandler::Default ()->timers ().size () << std::endl;

	std::cout << "INFO: TimeoutHandler::Default ().front->minLeft " << TimeoutHandler::Default ()->timers ().front ()->minLeft () << std::endl << \
	"INFO: TimeoutHandler::Default ().front->maxLeft " << TimeoutHandler::Default ()->timers ().front ()->maxLeft () << std::endl << \
	"INFO: TimeoutHandler::Default ().front->minTime " << TimeoutHandler::Default ()->timers ().front ()->minTime () << std::endl << \
	"INFO: TimeoutHandler::Default ().front->maxTime " << TimeoutHandler::Default ()->timers ().front ()->maxTime () << std::endl;

	std::cout << "INFO: TimeoutHandler::Default ().back->minLeft " << TimeoutHandler::Default ()->timers ().back ()->minLeft () <<  std::endl << \
	"INFO: TimeoutHandler::Default ().back->maxLeft " << TimeoutHandler::Default ()->timers ().back ()->maxLeft () << std::endl << \
	"INFO: TimeoutHandler::Default ().back->minTime " << TimeoutHandler::Default ()->timers ().back ()->minTime () << std::endl << \
	"INFO: TimeoutHandler::Default ().back->maxTime " << TimeoutHandler::Default ()->timers ().back ()->maxTime () << std::endl;
	exit (1);
    }

    if (TimeoutHandler::Default ()->timers ().back () != timers.front ())
    {
	std::cout << "FAIL: timer with the most time is not at the back" << std::endl;
	exit (1);
    }
}
