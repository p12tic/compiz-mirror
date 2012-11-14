/*
 * Compiz Core: Output class: Unit tests
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
#include "core/output.h"

TEST (Output, ConstrainWindowInternal)
{
    CompOutput output;
    int w, h;

    output.setGeometry (0, 0, 1024, 768);
    output.setWorkArea (CompRect (48, 24, 976, 744));

    w = 123;
    h = 123;
    output.constrainWindowSize (&w, &h);
    EXPECT_EQ (w, 123);
    EXPECT_EQ (h, 123);

    w = 0;
    h = 0;
    output.constrainWindowSize (&w, &h);
    EXPECT_EQ (w, 0);
    EXPECT_EQ (h, 0);

    w = 1;
    h = 1;
    output.constrainWindowSize (&w, &h);
    EXPECT_EQ (w, 1);
    EXPECT_EQ (h, 1);
}

TEST (Output, ConstrainWindowVertMax)
{
    CompOutput output;
    int w, h;

    output.setGeometry (0, 0, 1024, 768);
    output.setWorkArea (CompRect (48, 24, 976, 744));

    w = 123;
    h = CompOutput::Maximized;
    output.constrainWindowSize (&w, &h);
    EXPECT_EQ (w, 123);
    EXPECT_EQ (h, output.workArea ().height ());
}

TEST (Output, ConstrainWindowHorzMax)
{
    CompOutput output;
    int w, h;

    output.setGeometry (0, 0, 1024, 768);
    output.setWorkArea (CompRect (48, 24, 976, 744));

    w = CompOutput::Maximized;
    h = 456;
    output.constrainWindowSize (&w, &h);
    EXPECT_EQ (w, output.workArea ().width ());
    EXPECT_EQ (h, 456);
}

TEST (Output, ConstrainWindowMaximized)
{
    CompOutput output;
    int w, h;

    output.setGeometry (0, 0, 1024, 768);
    output.setWorkArea (CompRect (48, 24, 976, 744));

    w = CompOutput::Maximized;
    h = CompOutput::Maximized;
    output.constrainWindowSize (&w, &h);
    EXPECT_EQ (w, output.workArea ().width ());
    EXPECT_EQ (h, output.workArea ().height ());
}

TEST (Output, ConstrainWindowFullscreen)
{
    CompOutput output;
    int w, h;

    output.setGeometry (0, 0, 1024, 768);
    output.setWorkArea (CompRect (48, 24, 976, 744));

    w = CompOutput::Fullscreen;
    h = CompOutput::Fullscreen;
    output.constrainWindowSize (&w, &h);
    EXPECT_EQ (w, output.width ());
    EXPECT_EQ (h, output.height ());
}

TEST (Output, ConstrainWindowOversized)
{
    CompOutput output;
    int w, h;

    output.setGeometry (0, 0, 1920, 1200);

    w = 4000;
    h = 3000;
    output.constrainWindowSize (&w, &h);
    EXPECT_EQ (w, 4000);
    EXPECT_EQ (h, 3000);
}

