#include <gtest/gtest.h>

#include "core/core.h"
#include "core/action.h"
#include "core/match.h"
#include "core/option.h"

#include "globals.h"

namespace {
    template<typename T>
    void
    check_type_value(CompOption::Type type, const T & value) {
	CompOption::Value v;
	v.set(value);
	ASSERT_EQ(v.type(),type);
	ASSERT_EQ (v.get<T>(),value);
    }
}



static unsigned short defaultColor[4] = { 0x0, 0x0, 0x0, 0xffff};

TEST(CompOption,Value)
{

    check_type_value<bool> (CompOption::TypeBool, true);
    check_type_value<bool> (CompOption::TypeBool, false);

    check_type_value<int> (CompOption::TypeInt, 1);
    check_type_value<float> (CompOption::TypeFloat, 1.f);
    check_type_value<CompString> (CompOption::TypeString, CompString("Check"));

    check_type_value<CompAction> (CompOption::TypeAction, CompAction());
    check_type_value<CompMatch> (CompOption::TypeMatch, CompMatch());

    CompOption::Value v1, v2;
    ASSERT_EQ (v1,v2);
    v1.set (CompString("SomeString"));
    ASSERT_TRUE(v1 != v2);
}
