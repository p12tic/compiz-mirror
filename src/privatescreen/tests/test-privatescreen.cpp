#include "privatescreen.h"


// Get rid of stupid macro from X.h
// Why, oh why, are we including X.h?
#undef None

#include <gtest/gtest.h>
#include <gmock/gmock.h>


namespace {


} // (abstract) namespace



TEST(PrivateScreenTest, dummy)
{
    using namespace testing;


    ASSERT_EQ(0, __LINE__);
}

