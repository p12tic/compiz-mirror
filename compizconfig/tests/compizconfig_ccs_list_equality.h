/*
 * Compiz configuration system library
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
#ifndef _COMPIZCONFIG_CCS_LIST_EQUALITY_H
#define _COMPIZCONFIG_CCS_LIST_EQUALITY_H

#include <iosfwd>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::MakeMatcher;
using ::testing::Matcher;

class ListEqualityMatcher :
    public MatcherInterface <CCSSettingValueList>
{
    public:

	ListEqualityMatcher (CCSSettingListInfo info,
			     CCSSettingValueList cmp) :
	    mInfo (info),
	    mCmp (cmp)
	{
	}

	virtual bool MatchAndExplain (CCSSettingValueList x, MatchResultListener *listener) const
	{
	    return ccsCompareLists (x, mCmp, mInfo);
	}

	virtual void DescribeTo (std::ostream *os) const
	{
	    *os << "lists are equal";
	}

	virtual void DescribeNegationTo (std::ostream *os) const
	{
	    *os << "lists are not equal";
	}

    private:

	CCSSettingListInfo mInfo;
	CCSSettingValueList mCmp;
};

Matcher<CCSSettingValueList> ListEqual (CCSSettingListInfo info,
					CCSSettingValueList cmp)
{
    return MakeMatcher (new ListEqualityMatcher (info, cmp));
}

#endif
