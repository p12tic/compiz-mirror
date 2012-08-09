/*
 * Copyright © 2012 Canonical Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Canonical Ltd. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Canonical Ltd. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * CANONICAL, LTD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL CANONICAL, LTD. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authored by: Renato Araujo Oliveira Filho <renato@canonical.com>
 */


#include <gtest/gtest.h>

#include "click-threshold.h"

class ExpoClickThresholdTest :
    public ::testing::Test
{
};

TEST(ExpoClickThresholdTest, TestNotMove)
{
    CompPoint first(10, 10);
    EXPECT_TRUE(compiz::expo::clickMovementInThreshold (first, 10, 10));
}

TEST(ExpoClickThresholdTest, TestMoveNearLeft)
{
    CompPoint first(10, 10);
    EXPECT_TRUE(compiz::expo::clickMovementInThreshold (first, 8, 8));
}

TEST(ExpoClickThresholdTest, TestMoveNearRight)
{
    CompPoint first(10, 10);
    EXPECT_TRUE(compiz::expo::clickMovementInThreshold (first, 13, 13));
}

TEST(ExpoClickThresholdTest, TestMoveFarLeft)
{
    CompPoint first(10, 10);
    EXPECT_FALSE(compiz::expo::clickMovementInThreshold (first, 1, 1));
}

TEST(ExpoClickThresholdTest, TestMoveFarRight)
{
    CompPoint first(10, 10);
    EXPECT_FALSE(compiz::expo::clickMovementInThreshold (first, 30, 30));
}

TEST(ExpoClickThresholdTest, TestMoveNearX)
{
    CompPoint first(10, 10);
    EXPECT_TRUE(compiz::expo::clickMovementInThreshold (first, 13, 10));
}

TEST(ExpoClickThresholdTest, TestMoveNearY)
{
    CompPoint first(10, 10);
    EXPECT_TRUE(compiz::expo::clickMovementInThreshold (first, 10, 13));
}

TEST(ExpoClickThresholdTest, TestMoveFarX)
{
    CompPoint first(10, 10);
    EXPECT_FALSE(compiz::expo::clickMovementInThreshold (first, 30, 10));
}

TEST(ExpoClickThresholdTest, TestMoveFarY)
{
    CompPoint first(10, 10);
    EXPECT_FALSE(compiz::expo::clickMovementInThreshold (first, 10, 30));
}
