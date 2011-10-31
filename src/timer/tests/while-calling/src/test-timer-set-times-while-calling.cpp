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
CompTimerTestSetCalling::cb (int timernum, CompTimer *t1, CompTimer *t2, CompTimer *t3)
{
    std::cout << "INFO: triggering timer " << timernum << std::endl;

    if (lastTimerTriggered == 0 && timernum == 1)
    {
	/* Change the timeout time of the second timer to be after the third */
	t2->setTimes (4000, 4100);

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

void
CompTimerTestSetCalling::precallback ()
{
    CompTimer      *t1, *t2, *t3;

    std::cout << "-= TEST: changing timeout time" << std::endl;

    t1 = new CompTimer ();
    t2 = new CompTimer ();
    t3 = new CompTimer ();

    timers.push_back (t1);
    timers.push_back (t2);
    timers.push_back (t3);

    t1->setCallback (boost::bind (&CompTimerTestSetCalling::cb, this, 1, t1, t2, t3));
    t1->setTimes (1000, 1100);
    t1->start ();
    t2->setCallback (boost::bind (&CompTimerTestSetCalling::cb, this, 2, t1, t2, t3));
    t2->setTimes (2000, 2100);
    t2->start ();
    t3->setCallback (boost::bind (&CompTimerTestSetCalling::cb, this, 3, t1, t2, t3));
    t3->setTimes (3000, 3100);
    t3->start ();
}

CompTimerTest *
getTestObject ()
{
    return new CompTimerTestSetCalling ();
}
