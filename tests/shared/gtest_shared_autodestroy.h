#ifndef _COMPIZ_GTEST_SHARED_AUTODESTROY_H
#define _COMPIZ_GTEST_SHARED_AUTODESTROY_H

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

namespace
{
    template <typename T, typename TDel>
    boost::shared_ptr <T>
    AutoDestroy (T *t, TDel d)
    {
	return boost::shared_ptr <T> (t, boost::bind (d, _1));
    }
}

#endif
