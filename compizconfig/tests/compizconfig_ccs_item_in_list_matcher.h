#ifndef _COMPIZCONFIG_CCS_ITEM_IN_LIST_MATCHER_H
#define _COMPIZCONFIG_CCS_ITEM_IN_LIST_MATCHER_H

using ::testing::Matcher;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::MakeMatcher;

template <typename I, typename L>
class ItemInCCSListMatcher :
    public ::testing::MatcherInterface <L>
{
    public:

	ItemInCCSListMatcher (const Matcher<I> &matcher) :
	    mMatcher (matcher)
	{
	}

	virtual bool MatchAndExplain (L list, MatchResultListener *listener) const
	{
	    L iter = list;

	    while (iter)
	    {
		if (mMatcher.MatchAndExplain ((*(reinterpret_cast <I *> (iter->data))), listener))
		    return true;

		iter = iter->next;
	    }

	    return false;
	}

	virtual void DescribeTo (std::ostream *os) const
	{
	    *os << "found in list (";
	    mMatcher.DescribeTo (os);
	    *os << ")";
	}

	virtual void DescribeNegationTo (std::ostream *os) const
	{
	    *os << "not found in list (";
	    mMatcher.DescribeNegationTo (os);
	    *os << ")";
	}

    private:

	const Matcher<I> & mMatcher;
};

template <typename I, typename L>
Matcher<L> IsItemInCCSList (const Matcher<I> &matcher)
{
    return MakeMatcher (new ItemInCCSListMatcher <I, L> (matcher));
}

#endif
