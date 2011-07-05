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

bool
CompTimerTestCallbacks::cb (int timernum)
{
    if (lastTimerTriggered == 0 && timernum == 1)
    {
	std::cout << "FAIL: timer with a higher timeout value triggered before the timer with the lower timeout" << std::endl;
	exit (1);
	return false;
    }
    else if (lastTimerTriggered == 2 && timernum != 1)
    {
	std::cout << "FAIL: the second timer should have triggered" << std::endl;
	exit (1);
	return false;
    }

    std::cout << "INFO: triggering timer " << timernum << std::endl;

    lastTimerTriggered = timernum;

    if (timernum == 1)
    {
	std::cout << "PASS: basic timers" << std::endl;
	ml->quit ();
    }

    return false;
}

void
CompTimerTestCallbacks::precallback ()
{
    /* Test 2: Adding timers */
    std::cout << "-= TEST: adding timers and callbacks" << std::endl;
    timers.push_back (new CompTimer ());
    timers.front ()->setTimes (100, 110);
    timers.front ()->setCallback (boost::bind (&CompTimerTestCallbacks::cb, this, 1));

    /* TimeoutHandler::timers should be empty */
    if (!TimeoutHandler::Default ()->timers ().empty ())
    {
	std::cout << "FAIL: timers list is not empty" << std::endl;
	exit (1);
    }

    timers.push_back (new CompTimer ());
    timers.back ()->setTimes (50, 90);
    timers.back ()->setCallback (boost::bind (&CompTimerTestCallbacks::cb, this, 2));

    /* Start both timers */
    timers.front ()->start ();
    timers.back ()->start ();

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
