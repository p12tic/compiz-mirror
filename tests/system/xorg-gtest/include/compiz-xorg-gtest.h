/*
 * Compiz XOrg GTest
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
#ifndef _COMPIZ_XORG_GTEST_H
#define _COMPIZ_XORG_GTEST_H
#include <memory>
#include <list>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <xorg/gtest/xorg-gtest.h>

using ::testing::MatcherInterface;
using ::testing::MatchResultListener;

namespace compiz
{
    namespace testing
    {
	typedef  ::testing::MatcherInterface <const XEvent &> XEventMatcher;

	class PrivatePropertyNotifyXEventMatcher;
	class PropertyNotifyXEventMatcher :
	    public compiz::testing::XEventMatcher
	{
	    public:

		PropertyNotifyXEventMatcher (Display *dpy,
					     const std::string &propertyName);

		virtual bool MatchAndExplain (const XEvent &event, MatchResultListener *listener) const;
		virtual void DescribeTo (std::ostream *os) const;
		virtual void DescribeNegationTo (std::ostream *os) const;

	    private:

		std::auto_ptr <PrivatePropertyNotifyXEventMatcher> priv;
	};

	Window CreateNormalWindow (Display *dpy);

	std::list <Window> NET_CLIENT_LIST_STACKING (Display *);
	bool AdvanceToNextEventOnSuccess (Display *dpy,
					  bool waitResult);
	bool WaitForEventOfTypeOnWindow (Display *dpy,
					 Window  w,
					 int     type,
					 int     ext,
					 int     extType,
					 int     timeout = 1000);
	bool WaitForEventOfTypeOnWindowMatching (Display             *dpy,
						 Window              w,
						 int                 type,
						 int                 ext,
						 int                 extType,
						 const XEventMatcher &matcher,
						 int                 timeout = 1000);

	class PrivateCompizProcess;
	class CompizProcess
	{
	    public:
		typedef enum _StartupFlags
		{
		    ReplaceCurrentWM = (1 << 0),
		    WaitForStartupMessage = (1 << 1),
		    ExpectStartupFailure = (1 << 2)
		} StartupFlags;

		CompizProcess (Display *dpy, StartupFlags, unsigned int waitTimeout);
		~CompizProcess ();
		xorg::testing::Process::State State ();
		pid_t Pid ();

	    private:
		std::auto_ptr <PrivateCompizProcess> priv;
	};

	class PrivateCompizXorgSystemTest;
	class CompizXorgSystemTest :
	    public xorg::testing::Test
	{
	    public:

		CompizXorgSystemTest ();

		virtual void SetUp ();
		virtual void TearDown ();

		xorg::testing::Process::State CompizProcessState ();
		void StartCompiz (CompizProcess::StartupFlags flags);

	    private:
		std::auto_ptr <PrivateCompizXorgSystemTest> priv;
	};

	class AutostartCompizXorgSystemTest :
	    public CompizXorgSystemTest
	{
	    public:

		virtual void SetUp ();
	};
    }
}

#endif
