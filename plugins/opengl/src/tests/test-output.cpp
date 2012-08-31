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
#include "output.h"

using namespace compiz::opengl;

namespace
{

class MockCompWindow
{
public:
    MockCompWindow (bool canBeOnTop, int x, int y, int width, int height) :
	raisable (canBeOnTop), reg (x, y, width, height) {}
    bool allowedOnTop () const { return raisable; }
    const CompRegion &region () const { return reg; }

private:
    bool raisable;
    CompRegion reg;
};

} // namespace

TEST (OpenGLOutput, NoWindows)
{
    Output monitor (CompRect (0, 0, 1024, 768));
    MockCompWindow desktop (false, 0, 0, 1024, 768);
    monitor.addToBottom (desktop.region (), desktop.allowedOnTop (), &desktop);
    EXPECT_EQ (NULL, monitor.fullscreenWindow());
}

TEST (OpenGLOutput, NormalWindows)
{
    Output monitor (CompRect (0, 0, 1024, 768));
    MockCompWindow a (true, 10, 10, 40, 30);
    monitor.addToBottom (a.region (), a.allowedOnTop (), &a);
    MockCompWindow b (true, 20, 20, 50, 20);
    monitor.addToBottom (b.region (), b.allowedOnTop (), &b);
    MockCompWindow desktop (false, 0, 0, 1024, 768);
    monitor.addToBottom (desktop.region (), desktop.allowedOnTop (), &desktop);
    EXPECT_EQ (NULL, monitor.fullscreenWindow());
}

TEST (OpenGLOutput, TwoFullscreen)
{
    Output monitor (CompRect (0, 0, 1024, 768));
    MockCompWindow f1 (true, 0, 0, 1024, 768);
    monitor.addToBottom (f1.region (), f1.allowedOnTop (), &f1);
    MockCompWindow a (true, 10, 10, 40, 30);
    monitor.addToBottom (a.region (), a.allowedOnTop (), &a);
    MockCompWindow b (true, 20, 20, 50, 20);
    monitor.addToBottom (b.region (), b.allowedOnTop (), &b);
    MockCompWindow f2 (true, 0, 0, 1024, 768);
    monitor.addToBottom (f2.region (), f2.allowedOnTop (), &f2);
    MockCompWindow desktop (false, 0, 0, 1024, 768);
    monitor.addToBottom (desktop.region (), desktop.allowedOnTop (), &desktop);
    EXPECT_EQ (monitor.fullscreenWindow(), &f1);
}

TEST (OpenGLOutput, Offscreen)
{
    Output monitor (CompRect (0, 0, 1024, 768));
    MockCompWindow x (true, -100, -100, 1, 1);
    monitor.addToBottom (x.region (), x.allowedOnTop (), &x);
    MockCompWindow y (true, 2000, 2000, 123, 456);
    monitor.addToBottom (y.region (), y.allowedOnTop (), &y);
    MockCompWindow f1 (true, 0, 0, 1024, 768);
    monitor.addToBottom (f1.region (), f1.allowedOnTop (), &f1);
    MockCompWindow a (true, 10, 10, 40, 30);
    monitor.addToBottom (a.region (), a.allowedOnTop (), &a);
    MockCompWindow b (true, 20, 20, 50, 20);
    monitor.addToBottom (b.region (), b.allowedOnTop (), &b);
    MockCompWindow f2 (true, 0, 0, 1024, 768);
    monitor.addToBottom (f2.region (), f2.allowedOnTop (), &f2);
    MockCompWindow desktop (false, 0, 0, 1024, 768);
    monitor.addToBottom (desktop.region (), desktop.allowedOnTop (), &desktop);
    EXPECT_EQ (monitor.fullscreenWindow(), &f1);
}

TEST (OpenGLOutput, CancelFullscreen1)
{
    Output monitor (CompRect (0, 0, 1024, 768));
    MockCompWindow z (true, 500, 500, 345, 234);
    monitor.addToBottom (z.region (), z.allowedOnTop (), &z);
    MockCompWindow f1 (true, 0, 0, 1024, 768);
    monitor.addToBottom (f1.region (), f1.allowedOnTop (), &f1);
    MockCompWindow a (true, 10, 10, 40, 30);
    monitor.addToBottom (a.region (), a.allowedOnTop (), &a);
    MockCompWindow b (true, 20, 20, 50, 20);
    monitor.addToBottom (b.region (), b.allowedOnTop (), &b);
    MockCompWindow f2 (true, 0, 0, 1024, 768);
    monitor.addToBottom (f2.region (), f2.allowedOnTop (), &f2);
    MockCompWindow desktop (false, 0, 0, 1024, 768);
    monitor.addToBottom (desktop.region (), desktop.allowedOnTop (), &desktop);
    EXPECT_EQ (NULL, monitor.fullscreenWindow());
}

TEST (OpenGLOutput, CancelFullscreen2)
{
    Output monitor (CompRect (0, 0, 1024, 768));
    MockCompWindow x (true, -100, -100, 1, 1);
    monitor.addToBottom (x.region (), x.allowedOnTop (), &x);
    MockCompWindow y (true, 2000, 2000, 123, 456);
    monitor.addToBottom (y.region (), y.allowedOnTop (), &y);
    MockCompWindow z (true, 500, 500, 345, 234);
    monitor.addToBottom (z.region (), z.allowedOnTop (), &z);
    MockCompWindow f1 (true, 0, 0, 1024, 768);
    monitor.addToBottom (f1.region (), f1.allowedOnTop (), &f1);
    MockCompWindow a (true, 10, 10, 40, 30);
    monitor.addToBottom (a.region (), a.allowedOnTop (), &a);
    MockCompWindow b (true, 20, 20, 50, 20);
    monitor.addToBottom (b.region (), b.allowedOnTop (), &b);
    MockCompWindow f2 (true, 0, 0, 1024, 768);
    monitor.addToBottom (f2.region (), f2.allowedOnTop (), &f2);
    MockCompWindow desktop (false, 0, 0, 1024, 768);
    monitor.addToBottom (desktop.region (), desktop.allowedOnTop (), &desktop);
    EXPECT_EQ (NULL, monitor.fullscreenWindow());
}

TEST (OpenGLOutput, Overflow)
{
    Output monitor (CompRect (0, 0, 1024, 768));
    MockCompWindow a (true, 10, 10, 40, 30);
    monitor.addToBottom (a.region (), a.allowedOnTop (), &a);
    MockCompWindow b (true, -10, -10, 1044, 788);
    monitor.addToBottom (b.region (), b.allowedOnTop (), &b);
    MockCompWindow desktop (false, 0, 0, 1024, 768);
    monitor.addToBottom (desktop.region (), desktop.allowedOnTop (), &desktop);
    EXPECT_EQ (NULL, monitor.fullscreenWindow());
}


