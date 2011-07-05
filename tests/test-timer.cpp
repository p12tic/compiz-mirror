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

    bool test1cb (int timernum);
    bool test2cb (int timernum, CompTimer *t1, CompTimer *t2, CompTimer *t3);

    int lastTimerTriggered;
};

bool
CompTimerTest::test1cb (int timernum)
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

bool
CompTimerTest::test2cb (int timernum, CompTimer *t1, CompTimer *t2, CompTimer *t3)
{
    std::cout << "INFO: triggering timer " << timernum << std::endl;

    if (lastTimerTriggered == 0 && timernum == 1)
    {
	/* Change the timeout time of the second timer to be after the third */
	t2->setTimes (400, 410);

	/* Check if it is now at the back of the timeout list */
	if (TimeoutHandler::Default ()->timers ().back () != t2)
	{
	    std::cout << "FAIL: timer with higher timeout time is not at the back of the list" << std::endl;

	    for (std::list <CompTimer *>::iterator it = TimeoutHandler::Default ()->timers ().begin ();
		 it != TimeoutHandler::Default ()->timers ().end (); it++)
	    {
		CompTimer *t = (*it);
		std::cout << "INFO: t->minLeft " << t->minLeft () << std::endl << \
			     "INFO: t->maxLeft " << t->maxLeft () << std::endl << \
			     "INFO: t->minTime " << t->minTime () << std::endl << \
			     "INFO: t->maxTime " << t->maxTime () << std::endl;
	    }

	    exit (1);
	}
    }
    else if (lastTimerTriggered == 1 && timernum == 2)
    {
	std::cout << "FAIL: timer with a higher timeout time got triggered before a timer with a lower timeout time" << std::endl;

	for (std::list <CompTimer *>::iterator it = TimeoutHandler::Default ()->timers ().begin ();
	     it != TimeoutHandler::Default ()->timers ().end (); it++)
	{
	    CompTimer *t = (*it);
	    std::cout << "INFO: t->minLeft " << t->minLeft () << std::endl << \
			 "INFO: t->maxLeft " << t->maxLeft () << std::endl << \
			 "INFO: t->minTime " << t->minTime () << std::endl << \
			 "INFO: t->maxTime " << t->maxTime () << std::endl;
	}

	exit (1);
    }
    else if (lastTimerTriggered == 2 && timernum != 1)
    {
	std::cout << "FAIL: timer with higher timeout time didn't get triggered after a lower timeout time" << std::endl;

	for (std::list <CompTimer *>::iterator it = TimeoutHandler::Default ()->timers ().begin ();
	     it != TimeoutHandler::Default ()->timers ().end (); it++)
	{
	    CompTimer *t = (*it);
	    std::cout << "INFO: t->minLeft " << t->minLeft () << std::endl << \
			 "INFO: t->maxLeft " << t->maxLeft () << std::endl << \
			 "INFO: t->minTime " << t->minTime () << std::endl << \
			 "INFO: t->maxTime " << t->maxTime () << std::endl;
	}

	exit (1);
    }

    lastTimerTriggered = timernum;

    if (timernum == 2)
    {
	std::cout << "PASS: retiming" << std::endl;
	ml->quit ();
    }

    return false;
}

int
main (int argc, char **argv)
{
    CompTimerTest  *ctt = new CompTimerTest ();
    TimeoutHandler *th = new TimeoutHandler ();
    TimeoutHandler::SetDefault (th);

    /* Set up the main loop */
    ctt->mc = Glib::MainContext::get_default ();
    ctt->ml = Glib::MainLoop::create (ctt->mc, false);
    ctt->ts = CompTimeoutSource::create (ctt->mc);    
    ctt->lastTimerTriggered = 0;

    /* Test 1: Adding timers */
    std::cout << "-= TEST 1: adding timers" << std::endl;
    ctt->timers.push_back (new CompTimer ());
    ctt->timers.front ()->setTimes (100, 110);
    ctt->timers.front ()->setCallback (boost::bind (&CompTimerTest::test1cb, ctt, 1));

    /* TimeoutHandler::timers should be empty */
    if (!TimeoutHandler::Default ()->timers ().empty ())
    {
	std::cout << "FAIL: timers list is not empty" << std::endl;
	exit (1);
    }

    ctt->timers.push_back (new CompTimer ());
    ctt->timers.back ()->setTimes (50, 90);
    ctt->timers.back ()->setCallback (boost::bind (&CompTimerTest::test1cb, ctt, 2));

    /* Start both timers */
    ctt->timers.front ()->start ();
    ctt->timers.back ()->start ();

    /* TimeoutHandler::timers should have the timer that
     * is going to trigger first at the front of the
     * list and the last timer at the back */
    if (TimeoutHandler::Default ()->timers ().front () != ctt->timers.back ())
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

    if (TimeoutHandler::Default ()->timers ().back () != ctt->timers.front ())
    {
	std::cout << "FAIL: timer with the most time is not at the back" << std::endl;
	exit (1);
    }

    ctt->ml->run ();

    while (ctt->timers.size ())
    {
	CompTimer *t = ctt->timers.front ();

	ctt->timers.pop_front ();
	delete t;
    }

    delete ctt;
    ctt = new CompTimerTest ();

    ctt->mc = Glib::MainContext::get_default ();
    ctt->ml = Glib::MainLoop::create (ctt->mc, false);
    ctt->ts = CompTimeoutSource::create (ctt->mc);    
    ctt->lastTimerTriggered = 0;

    ctt->lastTimerTriggered = 0;

    /* Test 2: Changing timeout time while timer is running */

    std::cout << "-= TEST 2: changing timeout time" << std::endl;

    CompTimer *t1 = new CompTimer ();
    CompTimer *t2 = new CompTimer ();
    CompTimer *t3 = new CompTimer ();

    ctt->timers.push_back (t1);
    ctt->timers.push_back (t2);
    ctt->timers.push_back (t3);

    t1->setCallback (boost::bind (&CompTimerTest::test2cb, ctt, 1, t1, t2, t3));
    t1->setTimes (100, 110);
    t1->start ();
    t2->setCallback (boost::bind (&CompTimerTest::test2cb, ctt, 2, t1, t2, t3));
    t2->setTimes (200, 210);
    t2->start ();
    t3->setCallback (boost::bind (&CompTimerTest::test2cb, ctt, 3, t1, t2, t3));
    t3->setTimes (300, 310);
    t3->start ();

    ctt->ml->run ();

    delete th;

    return 0;
}
