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
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <xorg/gtest/xorg-gtest.h>
#include <compiz-xorg-gtest.h>

#include <gtest_shared_tmpenv.h>
#include <gtest_shared_characterwrapper.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

using ::testing::MatchResultListener;
using ::testing::MakeMatcher;
using ::testing::Matcher;

namespace ct = compiz::testing;

namespace
{

void
runtimeErrorWithErrno ()
{
    CharacterWrapper allocatedError (strerror (errno));
    throw std::runtime_error (std::string (allocatedError));
}

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

class AsyncTask
{
    public:

	typedef boost::shared_ptr <AsyncTask> Ptr;

	AsyncTask ()
	{
	    if (pipe2 (testToTaskFd, O_NONBLOCK | O_CLOEXEC) == -1)
		runtimeErrorWithErrno ();

	    if (pipe2 (taskToTestFd, O_NONBLOCK | O_CLOEXEC) == -1)
		runtimeErrorWithErrno ();

	    if (pthread_create (&mThread,
				NULL,
				AsyncTask::ThreadEntry,
				reinterpret_cast<void *> (this)) != 0)
		runtimeErrorWithErrno ();
	}

	~AsyncTask ()
	{
	    void *ret;
	    if (pthread_join (mThread, &ret) != 0)
	    {
		CharacterWrapper allocatedError (strerror (errno));
		throw std::runtime_error (std::string (allocatedError));
	    }

	    close (testToTaskFd[0]);
	    close (testToTaskFd[1]);
	    close (taskToTestFd[0]);
	    close (taskToTestFd[0]);
	}

	void SendMsgToTask (char msg);
	bool ReadMsgFromTask (char msg);

    protected:

	void SendMsgToTest (char msg);
	bool ReadMsgFromTest (char msg);

    private:

	static void * ThreadEntry (void *data);
	virtual void Task () = 0;

	int testToTaskFd[2];
	int taskToTestFd[2];
	pthread_t mThread;
};

bool
AsyncTask::ReadMsgFromTest (char msg)
{
    return checkForMessageOnFd (testToTaskFd[0], 1, msg);
}

bool
AsyncTask::ReadMsgFromTask (char msg)
{
    const int maximumWaitTime = 1000 * 10; // 10 seconds

    return checkForMessageOnFd (taskToTestFd[0], maximumWaitTime, msg);
}

void
AsyncTask::SendMsgToTest (char msg)
{
    char buf[1] = { msg };
    if (write (taskToTestFd[1], reinterpret_cast <void *> (buf), 1) == -1)
	runtimeErrorWithErrno ();
}

void
AsyncTask::SendMsgToTask (char msg)
{
    char buf[1] = { msg };
    if (write (testToTaskFd[1], reinterpret_cast <void *> (buf), 1) == -1)
	runtimeErrorWithErrno ();
}

void *
AsyncTask::ThreadEntry (void *data)
{
    AsyncTask *task = reinterpret_cast <AsyncTask *> (data);
    task->Task ();
    return NULL;
}

class WaitForDeathTask :
    public AsyncTask
{
    public:

	typedef boost::function <xorg::testing::Process::State ()> GetProcessState;

	WaitForDeathTask (const GetProcessState &procState) :
	    mProcessState (procState)
	{
	}

    private:

	GetProcessState mProcessState;

	void Task ();
};

void
WaitForDeathTask::Task ()
{
    do
    {
	if (ReadMsgFromTest (TEST_FAILED_MSG))
	    return;
    } while (mProcessState () != xorg::testing::Process::FINISHED_FAILURE);

    /* The process died, send a message back saying that it did */
    SendMsgToTest (PROCESS_DIED_MSG);
}

class CompizXorgSystemICCCM :
    public ct::CompizXorgSystemTest
{
    public:

	virtual void SetUp ()
	{
	    ct::CompizXorgSystemTest::SetUp ();

	    ::Display *dpy = Display ();

	    XSelectInput (dpy, DefaultRootWindow (dpy),
			  StructureNotifyMask |
			  FocusChangeMask |
			  PropertyChangeMask |
			  SubstructureRedirectMask);
	}

    private:
};

TEST_F (CompizXorgSystemICCCM, SomeoneElseHasSubstructureRedirectMask)
{
    /* XXX: This is a bit stupid, but we have to do it.
     * It seems as though closing the child stdout or
     * stderr will cause the client to hang indefinitely
     * when the child calls XSync (and that can happen
     * implicitly, eg XCloseDisplay) */
    TmpEnv env ("XORG_GTEST_CHILD_STDOUT", "1");

    StartCompiz (static_cast <ct::CompizProcess::StartupFlags> (
		     ct::CompizProcess::ExpectStartupFailure |
		     ct::CompizProcess::ReplaceCurrentWM |
		     ct::CompizProcess::WaitForStartupMessage));

    WaitForDeathTask::GetProcessState processState (boost::bind (&CompizXorgSystemICCCM::CompizProcessState,
								 this));
    AsyncTask::Ptr task (boost::make_shared <WaitForDeathTask> (processState));

    /* Now wait for the thread to tell us the news -
     * this will block for up to ten seconds */
    if (!task->ReadMsgFromTask (PROCESS_DIED_MSG))
    {
	throw std::runtime_error ("compiz process did not exit with failure status");
    }
}
