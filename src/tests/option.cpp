#include <gtest/gtest.h>

#include "core/core.h"
#include "core/action.h"
#include "core/match.h"
#include "core/option.h"

#include "globals.h"

#define CHECK_TYPE_AND_VALUE(type, compiz_type, value) \
    v.set (value); \
    try { \
      ASSERT_EQ (v.get<type>(),value); \
    } catch(...) {\
      FAIL() << #type;\
    }\


static unsigned short defaultColor[4] = { 0x0, 0x0, 0x0, 0xffff};

TEST(CompOption,Value)
{
    CompOption::Value v;

    CHECK_TYPE_AND_VALUE(bool,CompOption::TypeBool, true);
    CHECK_TYPE_AND_VALUE(bool,CompOption::TypeBool, false);

    CHECK_TYPE_AND_VALUE(int, CompOption::TypeInt, 1);
    CHECK_TYPE_AND_VALUE(float, CompOption::TypeFloat, 1.f);
    CHECK_TYPE_AND_VALUE(CompString, CompOption::TypeString, CompString("Check"));
    CHECK_TYPE_AND_VALUE(unsigned short*, CompOption::TypeColor, defaultColor);
    CHECK_TYPE_AND_VALUE(CompAction, CompOption::TypeAction, CompAction());
    CHECK_TYPE_AND_VALUE(CompMatch, CompOption::TypeMatch, CompMatch());

}
