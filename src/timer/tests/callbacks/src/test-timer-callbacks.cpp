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
#include <pthread.h>

using ::testing::AtLeast;
using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;

class CompTimerTestCallback: public CompTimerTest
{
public:
    CompTimerTestCallback ()
    {
	pthread_mutex_init (&mlistGuard, NULL);
    }
protected:

    pthread_mutex_t mlistGuard;

    static void* run (void* cb)
    {
	if (cb == NULL)
	{
	    return NULL;
	}
	static_cast <CompTimerTestCallback *> (cb)->ml->run ();
	return NULL;
    }

    pthread_t mmainLoopThread;
    std::vector<int> mtriggeredTimers;

    bool cb (int num)
    {
	std::cout << "cb: " << num << std::endl;
	pthread_mutex_lock (&mlistGuard);
	mtriggeredTimers.push_back (num);
	pthread_mutex_unlock (&mlistGuard);
	return (true);
    }

    void SetUp ()
    {
	CompTimerTest::SetUp ();
	mtriggeredTimers.clear ();

	/* Test 2: Adding timers */
	timers.push_back (new CompTimer ());
	timers.front ()->setTimes (100, 110);
	timers.front ()->setCallback (
		boost::bind(&CompTimerTestCallback::cb, this, 1));

	/* TimeoutHandler::timers should be empty */
	if (!TimeoutHandler::Default ()->timers().empty())
	{
	    FAIL () << "timers list is not empty";
	}

	timers.push_back (new CompTimer());
	timers.back ()->setTimes (50, 90);
	timers.back ()->setCallback (
		boost::bind(&CompTimerTestCallback::cb, this, 2));

	timers.push_back (new CompTimer());
	timers.back ()->setTimes (0, 0);
	timers.back ()->setCallback (
		boost::bind(&CompTimerTestCallback::cb, this, 3));

	/* Start both timers */
	timers[0]->start ();
	timers[1]->start ();
	timers[2]->start ();

	/* TimeoutHandler::timers should have the timer that
	 * is going to trigger first at the front of the
	 * list and the last timer at the back */
	if (TimeoutHandler::Default ()->timers ().front () != timers.back ())
	{
	    RecordProperty ("TimeoutHandler::Default ().size",
		    TimeoutHandler::Default ()->timers ().size ());
	    RecordProperty ("TimeoutHandler::Default ().front->minLeft",
		    TimeoutHandler::Default ()->timers ().front ()->minLeft());
	    RecordProperty ("TimeoutHandler::Default ().front->maxLeft",
		    TimeoutHandler::Default ()->timers ().front ()->maxLeft());
	    RecordProperty ("TimeoutHandler::Default ().front->minTime",
		    TimeoutHandler::Default ()->timers ().front ()->minTime());
	    RecordProperty ("TimeoutHandler::Default ().front->maxTime",
		    TimeoutHandler::Default ()->timers ().front ()->maxTime());
	    RecordProperty ("TimeoutHandler::Default ().back->minLeft",
		    TimeoutHandler::Default ()->timers ().back ()->minLeft());
	    RecordProperty ("TimeoutHandler::Default ().back->maxLeft",
		    TimeoutHandler::Default ()->timers ().back ()->maxLeft());
	    RecordProperty ("TimeoutHandler::Default ().back->minTime",
		    TimeoutHandler::Default ()->timers ().back ()->minTime());
	    RecordProperty ("TimeoutHandler::Default ().back->maxTime",
		    TimeoutHandler::Default ()->timers ().back ()->maxTime());
	    FAIL() << "timer with the least time is not at the front";
	}

	if (TimeoutHandler::Default ()->timers ().back () != timers.front ())
	{
	    FAIL() << "timer with the most time is not at the back";
	}

	ASSERT_EQ(
		0,
		pthread_create(&mmainLoopThread, NULL, CompTimerTestCallback::run, this));

	::sleep(1);
    }

    void TearDown ()
    {
	ml->quit();
	pthread_join(mmainLoopThread, NULL);

	CompTimerTest::TearDown();
    }
};

TEST_F (CompTimerTestCallback, TimerOrder)
{
    RecordProperty ("mtriggeredTimers.front()", mtriggeredTimers.front());
    RecordProperty("mtriggeredTimers.back()", mtriggeredTimers.back());

    std::vector<int>::iterator it = mtriggeredTimers.begin();
    ++it;

    int lastTimer = mtriggeredTimers.front ();

    while (it != mtriggeredTimers.end ())
    {
	std::cout << *it;
	switch (lastTimer)
	{
	case 0:
	    ASSERT_EQ (3, *it);
	    break;
	case 2:
	    ASSERT_EQ (3, *it);
	    break;
	case 1:
	    ASSERT_EQ (2, *it);
	    break;
	}
	lastTimer = *it;
	++it;
    }
    ASSERT_EQ (mtriggeredTimers.front(), 1);
}
