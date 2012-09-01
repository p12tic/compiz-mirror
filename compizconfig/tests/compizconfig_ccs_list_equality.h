#ifndef _COMPIZCONFIG_CCS_LIST_EQUALITY_H
#define _COMPIZCONFIG_CCS_LIST_EQUALITY_H

#include <iosfwd>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::MakeMatcher;

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
