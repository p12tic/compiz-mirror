/*
 * Compiz XOrg GTest, window stacking
 *
 * Copyright (C) 2012 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authored By:
 * Sam Spilsbury <sam.spilsbury@canonical.com>
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <list>
#include <string>
#include <stdexcept>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <xorg/gtest/xorg-gtest.h>
#include <compiz-xorg-gtest.h>

#include <gtest_shared_tmpenv.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

using ::testing::MatchResultListener;
using ::testing::MakeMatcher;
using ::testing::Matcher;

namespace ct = compiz::testing;

class CompizXorgSystemICCCM :
    public ct::XorgSystemTest
{
    public:

	CompizXorgSystemICCCM ()
	{
	    if (pipe2 (testToWaitThreadPipeFd, O_NONBLOCK | O_CLOEXEC) == -1)
	    {
		perror ("pipe2");
		exit (EXIT_FAILURE);
	    }

	    if (pipe2 (waitThreadToTestPipeFd, O_NONBLOCK | O_CLOEXEC) == -1)
	    {
		perror ("pipe2");
		exit (EXIT_FAILURE);
	    }
	}

	virtual void SetUp ()
	{
	    ct::XorgSystemTest::SetUp ();

	    ::Display *dpy = Display ();

	    XSelectInput (dpy, DefaultRootWindow (dpy),
			  StructureNotifyMask |
			  FocusChangeMask |
			  PropertyChangeMask |
			  SubstructureRedirectMask);
	}

	~CompizXorgSystemICCCM ()
	{
	    close (testToWaitThreadPipeFd[0]);
	    close (testToWaitThreadPipeFd[1]);
	    close (waitThreadToTestPipeFd[0]);
	    close (waitThreadToTestPipeFd[0]);
	}

	static void * WaitForDeathEntry (void *data);
	void WaitForDeath ();

	void SendGotDeathToTest ();
	void SendTestFailedToThread ();
	bool CheckIfTestTimedOut ();
	bool CheckIfProcessDied ();

    private:

	int testToWaitThreadPipeFd[2];
	int waitThreadToTestPipeFd[2];
};

namespace
{

bool
checkForMessageOnFd (int fd, int timeout, char expected)
{
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    /* Wait 1ms */
    poll (&pfd, 1, timeout);

    if (pfd.revents == POLLIN)
    {
	char msg[1];
	size_t sz = read (pfd.fd, reinterpret_cast <void *> (msg), 1);
	if (sz == 1 &&
	    msg[0] == expected)
	    return true;
    }

    return false;
}

char TEST_FAILED_MSG = 's';
char PROCESS_DIED_MSG = 'd';

}

bool
CompizXorgSystemICCCM::CheckIfTestTimedOut ()
{
    return checkForMessageOnFd (testToWaitThreadPipeFd[0], 1, TEST_FAILED_MSG);
}

bool
CompizXorgSystemICCCM::CheckIfProcessDied ()
{
    const int maximumWaitTime = 1000 * 10; // 10 seconds

    return checkForMessageOnFd (waitThreadToTestPipeFd[0], maximumWaitTime, PROCESS_DIED_MSG);
}

void
CompizXorgSystemICCCM::SendGotDeathToTest ()
{
    char buf[1] = { PROCESS_DIED_MSG };
    if (write (waitThreadToTestPipeFd[1], reinterpret_cast <void *> (buf), 1) == -1)
    {
	FAIL ();
	perror ("write");
    }
}

void
CompizXorgSystemICCCM::SendTestFailedToThread ()
{
    char buf[1] = { TEST_FAILED_MSG };
    if (write (testToWaitThreadPipeFd[1], reinterpret_cast <void *> (buf), 1) == -1)
    {
	FAIL ();
	perror ("write");
    }
}

void
CompizXorgSystemICCCM::WaitForDeath ()
{
    do
    {
	if (CheckIfTestTimedOut ())
	    return;
    } while (CompizProcessState () != xorg::testing::Process::FINISHED_FAILURE);

    /* The process died, send a message back saying that it did */
    SendGotDeathToTest ();
}

void *
CompizXorgSystemICCCM::WaitForDeathEntry (void *data)
{
    CompizXorgSystemICCCM *self = reinterpret_cast <CompizXorgSystemICCCM *> (data);
    self->WaitForDeath ();
    return NULL;
}

TEST_F (CompizXorgSystemICCCM, SomeoneElseHasSubstructureRedirectMask)
{
    pthread_t waitingForDeathThread;

    if (pthread_create (&waitingForDeathThread,
			NULL,
			CompizXorgSystemICCCM::WaitForDeathEntry,
			this) != 0)
    {
	FAIL ();
	perror ("pthread_create");
    }

    /* XXX: This is a bit stupid, but we have to do it.
     * It seems as though closing the child stdout or
     * stderr will cause the client to hang indefinitely
     * when the child calls XSync (and that can happen
     * implicitly, eg XCloseDisplay) */
    TmpEnv env ("XORG_GTEST_CHILD_STDOUT", "1");

    StartCompiz ();

    /* Now wait for the thread to tell us the news -
     * this will block for up to ten seconds */
    if (!CheckIfProcessDied ())
    {
	FAIL () << "compiz process did not exit with failure status";
    }

    void *ret;
    if (pthread_join (waitingForDeathThread, &ret) != 0)
    {
	FAIL ();
	perror ("pthread_join");
    }
}
