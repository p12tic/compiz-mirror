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

#include <glibmm/main.h>
#include <core/timer.h>
#include <privatetimeouthandler.h>
#include <privatetimeoutsource.h>
#include <iostream>
#include <boost/bind.hpp>

class CompTimerTest
{
public:
    Glib::RefPtr <Glib::MainLoop> ml;
    Glib::RefPtr <Glib::MainContext> mc;
    Glib::RefPtr <CompTimeoutSource> ts;
    std::list <CompTimer *> timers;

    bool test1 ();
    bool test1cb (int timernum);

    int lastTimerTriggered;
};

bool
CompTimerTest::test1cb (int timernum)
{
    if (lastTimerTriggered == 0 && timernum == 1)
    {
	std::cout << "fail: timer with a higher timeout value triggered before the timer with the lower timeout" << std::endl;
	return false;
    }
    else if (lastTimerTriggered == 2 && timernum != 1)
    {
	std::cout << "fail: the second timer should have triggered" << std::endl;
	return false;
    }

    std::cout << "info: triggering timer " << timernum << std::endl;

    lastTimerTriggered = timernum;

    if (timernum == 1)
    {
	std::cout << "pass: basic timers" << std::endl;
	ml->quit ();
    }

    return false;
}

int
main (int argc, char **argv)
{
    CompTimerTest  ctt;
    TimeoutHandler *th = new TimeoutHandler ();
    TimeoutHandler::SetDefault (th);

    /* Set up the main loop */
    ctt.mc = Glib::MainContext::get_default ();
    ctt.ml = Glib::MainLoop::create (ctt.mc, false);
    ctt.ts = CompTimeoutSource::create (ctt.mc);    
    ctt.lastTimerTriggered = 0;

    /* Test 1: Adding timers */
    std::cout << "test 1: adding timers" << std::endl;
    ctt.timers.push_back (new CompTimer ());
    ctt.timers.front ()->setTimes (100, 110);
    ctt.timers.front ()->setCallback (boost::bind (&CompTimerTest::test1cb, &ctt, 1));

    /* TimeoutHandler::timers should be empty */
    if (!TimeoutHandler::Default ()->timers ().empty ())
	std::cout << "fail: timers list is not empty" << std::endl;

    ctt.timers.push_back (new CompTimer ());
    ctt.timers.back ()->setTimes (50, 90);
    ctt.timers.back ()->setCallback (boost::bind (&CompTimerTest::test1cb, &ctt, 2));

    /* Start both timers */
    ctt.timers.front ()->start ();
    ctt.timers.back ()->start ();

    /* TimeoutHandler::timers should have the timer that
     * is going to trigger first at the front of the
     * list and the last timer at the back */
    if (TimeoutHandler::Default ()->timers ().front () != ctt.timers.back ())
    {
	std::cout << "fail: timer with the least time is not at the front" << std::endl;
	std::cout << "info: TimeoutHandler::Default ().size " << TimeoutHandler::Default ()->timers ().size () << std::endl;

	std::cout << "info: TimeoutHandler::Default ().front->minLeft " << TimeoutHandler::Default ()->timers ().front ()->minLeft () << std::endl << \
	"info: TimeoutHandler::Default ().front->maxLeft " << TimeoutHandler::Default ()->timers ().front ()->maxLeft () << std::endl << \
	"info: TimeoutHandler::Default ().front->minTime " << TimeoutHandler::Default ()->timers ().front ()->minTime () << std::endl << \
	"info: TimeoutHandler::Default ().front->maxTime " << TimeoutHandler::Default ()->timers ().front ()->maxTime () << std::endl;

	std::cout << "info: TimeoutHandler::Default ().back->minLeft " << TimeoutHandler::Default ()->timers ().back ()->minLeft () <<  std::endl << \
	"info: TimeoutHandler::Default ().back->maxLeft " << TimeoutHandler::Default ()->timers ().back ()->maxLeft () << std::endl << \
	"info: TimeoutHandler::Default ().back->minTime " << TimeoutHandler::Default ()->timers ().back ()->minTime () << std::endl << \
	"info: TimeoutHandler::Default ().back->maxTime " << TimeoutHandler::Default ()->timers ().back ()->maxTime () << std::endl;
    }

    if (TimeoutHandler::Default ()->timers ().back () != ctt.timers.front ())
	std::cout << "fail: timer with the most time is not at the back" << std::endl;

    ctt.ml->run ();

    delete th;

    return 0;
}
