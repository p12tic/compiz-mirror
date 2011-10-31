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
CompTimerTestSetValues::cb (int timernum)
{
    if (timernum == 3)
    {
	ml->quit ();
	std::cout << "PASS: testing values" << std::endl;
    }
    return false;
}

void
CompTimerTestSetValues::precallback ()
{
    CompTimer      *t1, *t2, *t3;

    std::cout << "-= TEST: testing values" << std::endl;

    t1 = new CompTimer ();

    t1->setTimes (100, 90);
    t1->setCallback (boost::bind (&CompTimerTestSetValues::cb, this, 1));
    t1->start ();

    if (t1->minTime () != 100)
    {
	std::cout << "FAIL: min time was not the min value passed" << std::endl;
	exit (1);
    }

    if (t1->maxTime () != 100)
    {
	std::cout << "FAIL: max time was not the min value passed" << std::endl;
	exit (1);
    }

    if (t1->minLeft () != 100)
    {
	std::cout << "FAIL: min left was not the min value passed " << std::endl;
	exit (1);
    }

    if (t1->maxLeft () != 100)
    {
	std::cout << "FAIL: max left was not the min value passed" << std::endl;
	exit (1);
    }

    t2 = new CompTimer ();

    t2->setTimes (100, 110);
    t2->setCallback (boost::bind (&CompTimerTestSetValues::cb, this, 2));
    t2->start ();

    if (t2->minTime () != 100)
    {
	std::cout << "FAIL: min time was not the min value passed" << std::endl;
	exit (1);
    }

    if (t2->maxTime () != 110)
    {
	std::cout << "FAIL: max time was not the max value passed" << std::endl;
	exit (1);
    }

    if (t2->minLeft () != 100)
    {
	std::cout << "FAIL: min left was not the min value passed " << std::endl;
	exit (1);
    }

    if (t2->maxLeft () != 110)
    {
	std::cout << "FAIL: max left was not the max value passed" << std::endl;
	exit (1);
    }

    t3 = new CompTimer ();

    t3->setTimes (100);
    t3->setCallback (boost::bind (&CompTimerTestSetValues::cb, this, 3));
    t3->start ();

    if (t3->minTime () != 100)
    {
	std::cout << "FAIL: min time was not the value passed" << std::endl;
	exit (1);
    }

    if (t3->maxTime () != 100)
    {
	std::cout << "FAIL: max time was not the value passed" << std::endl;
	exit (1);
    }

    if (t3->minLeft () != 100)
    {
	std::cout << "FAIL: min left was not the value passed" << t3->minLeft () << std::endl;
	exit (1);
    }

    if (t3->maxLeft () != 100)
    {
	std::cout << "FAIL: max left was not the value passed" << std::endl;
	exit (1);
    }

    timers.push_back (t1);
    timers.push_back (t2);
    timers.push_back (t3);
}

CompTimerTest *
getTestObject ()
{
    return new CompTimerTestSetValues ();
}
