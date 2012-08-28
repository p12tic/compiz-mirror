/*
 * Compiz opengl plugin, Output class
 *
 * Copyright (c) 2012 Canonical Ltd.
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "gtest/gtest.h"
#include "../output.h"

using namespace compiz::opengl;

TEST (OutputTest, no_windows)
{
    Output monitor (CompRect (0, 0, 1024, 768));
    MockCompWindow desktop (CompWindowTypeDesktopMask, 0, 0, 1024, 768);
    monitor.occlude (&desktop);
    EXPECT_EQ (NULL, monitor.fullscreenWindow());
}

TEST (OutputTest, normal_windows)
{
    Output monitor (CompRect (0, 0, 1024, 768));
    MockCompWindow a (CompWindowTypeNormalMask, 10, 10, 40, 30);
    monitor.occlude (&a);
    MockCompWindow b (CompWindowTypeNormalMask, 20, 20, 50, 20);
    monitor.occlude (&b);
    MockCompWindow desktop (CompWindowTypeDesktopMask, 0, 0, 1024, 768);
    monitor.occlude (&desktop);
    EXPECT_EQ (NULL, monitor.fullscreenWindow());
}

TEST (OutputTest, two_fullscreen)
{
    Output monitor (CompRect (0, 0, 1024, 768));
    MockCompWindow f1 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    monitor.occlude (&f1);
    MockCompWindow a (CompWindowTypeNormalMask, 10, 10, 40, 30);
    monitor.occlude (&a);
    MockCompWindow b (CompWindowTypeNormalMask, 20, 20, 50, 20);
    monitor.occlude (&b);
    MockCompWindow f2 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    monitor.occlude (&f2);
    MockCompWindow desktop (CompWindowTypeDesktopMask, 0, 0, 1024, 768);
    monitor.occlude (&desktop);
    EXPECT_EQ (monitor.fullscreenWindow(), &f1);
}

TEST (OutputTest, offscreen)
{
    Output monitor (CompRect (0, 0, 1024, 768));
    MockCompWindow x (CompWindowTypeNormalMask, -100, -100, 1, 1);
    monitor.occlude (&x);
    MockCompWindow y (CompWindowTypeNormalMask, 2000, 2000, 123, 456);
    monitor.occlude (&y);
    MockCompWindow f1 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    monitor.occlude (&f1);
    MockCompWindow a (CompWindowTypeNormalMask, 10, 10, 40, 30);
    monitor.occlude (&a);
    MockCompWindow b (CompWindowTypeNormalMask, 20, 20, 50, 20);
    monitor.occlude (&b);
    MockCompWindow f2 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    monitor.occlude (&f2);
    MockCompWindow desktop (CompWindowTypeDesktopMask, 0, 0, 1024, 768);
    monitor.occlude (&desktop);
    EXPECT_EQ (monitor.fullscreenWindow(), &f1);
}

TEST (OutputTest, cancel_fullscreen1)
{
    Output monitor (CompRect (0, 0, 1024, 768));
    MockCompWindow z (CompWindowTypeNormalMask, 500, 500, 345, 234);
    monitor.occlude (&z);
    MockCompWindow f1 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    monitor.occlude (&f1);
    MockCompWindow a (CompWindowTypeNormalMask, 10, 10, 40, 30);
    monitor.occlude (&a);
    MockCompWindow b (CompWindowTypeNormalMask, 20, 20, 50, 20);
    monitor.occlude (&b);
    MockCompWindow f2 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    monitor.occlude (&f2);
    MockCompWindow desktop (CompWindowTypeDesktopMask, 0, 0, 1024, 768);
    monitor.occlude (&desktop);
    EXPECT_EQ (NULL, monitor.fullscreenWindow());
}

TEST (OutputTest, cancel_fullscreen2)
{
    Output monitor (CompRect (0, 0, 1024, 768));
    MockCompWindow x (CompWindowTypeNormalMask, -100, -100, 1, 1);
    monitor.occlude (&x);
    MockCompWindow y (CompWindowTypeNormalMask, 2000, 2000, 123, 456);
    monitor.occlude (&y);
    MockCompWindow z (CompWindowTypeNormalMask, 500, 500, 345, 234);
    monitor.occlude (&z);
    MockCompWindow f1 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    monitor.occlude (&f1);
    MockCompWindow a (CompWindowTypeNormalMask, 10, 10, 40, 30);
    monitor.occlude (&a);
    MockCompWindow b (CompWindowTypeNormalMask, 20, 20, 50, 20);
    monitor.occlude (&b);
    MockCompWindow f2 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    monitor.occlude (&f2);
    MockCompWindow desktop (CompWindowTypeDesktopMask, 0, 0, 1024, 768);
    monitor.occlude (&desktop);
    EXPECT_EQ (NULL, monitor.fullscreenWindow());
}

TEST (OutputTest, overflow)
{
    Output monitor (CompRect (0, 0, 1024, 768));
    MockCompWindow a (CompWindowTypeNormalMask, 10, 10, 40, 30);
    monitor.occlude (&a);
    MockCompWindow b (CompWindowTypeNormalMask, -10, -10, 1044, 788);
    monitor.occlude (&b);
    MockCompWindow desktop (CompWindowTypeDesktopMask, 0, 0, 1024, 768);
    monitor.occlude (&desktop);
    EXPECT_EQ (NULL, monitor.fullscreenWindow());
}


