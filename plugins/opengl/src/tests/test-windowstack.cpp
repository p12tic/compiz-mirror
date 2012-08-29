/*
 * Compiz opengl plugin, WindowStack class
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
#include "windowstack.h"

using namespace compiz::opengl;

namespace
{

class MockCompWindow : public Stackable
{
public:
    MockCompWindow (unsigned int type, int x, int y, int width, int height) :
	flags (type), reg (x, y, width, height) {}
    unsigned int type () { return flags; }
    const CompRegion &region () const { return reg; }

private:
    unsigned int flags;
    CompRegion reg;
};

} // namespace

TEST (OpenGLWindowStack, NoWindows)
{
    WindowStack s (CompRect (0, 0, 1024, 768));
    MockCompWindow desktop (CompWindowTypeDesktopMask, 0, 0, 1024, 768);
    s.addWindowToBottom (&desktop);
    EXPECT_EQ (NULL, s.fullscreenWindow());
}

TEST (OpenGLWindowStack, NormalWindows)
{
    WindowStack s (CompRect (0, 0, 1024, 768));
    MockCompWindow a (CompWindowTypeNormalMask, 10, 10, 40, 30);
    s.addWindowToBottom (&a);
    MockCompWindow b (CompWindowTypeNormalMask, 20, 20, 50, 20);
    s.addWindowToBottom (&b);
    MockCompWindow desktop (CompWindowTypeDesktopMask, 0, 0, 1024, 768);
    s.addWindowToBottom (&desktop);
    EXPECT_EQ (NULL, s.fullscreenWindow());
}

TEST (OpenGLWindowStack, TwoFullscreen)
{
    WindowStack s (CompRect (0, 0, 1024, 768));
    MockCompWindow f1 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    s.addWindowToBottom (&f1);
    MockCompWindow a (CompWindowTypeNormalMask, 10, 10, 40, 30);
    s.addWindowToBottom (&a);
    MockCompWindow b (CompWindowTypeNormalMask, 20, 20, 50, 20);
    s.addWindowToBottom (&b);
    MockCompWindow f2 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    s.addWindowToBottom (&f2);
    MockCompWindow desktop (CompWindowTypeDesktopMask, 0, 0, 1024, 768);
    s.addWindowToBottom (&desktop);
    EXPECT_EQ (s.fullscreenWindow(), &f1);
}

TEST (OpenGLWindowStack, Offscreen)
{
    WindowStack s (CompRect (0, 0, 1024, 768));
    MockCompWindow x (CompWindowTypeNormalMask, -100, -100, 1, 1);
    s.addWindowToBottom (&x);
    MockCompWindow y (CompWindowTypeNormalMask, 2000, 2000, 123, 456);
    s.addWindowToBottom (&y);
    MockCompWindow f1 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    s.addWindowToBottom (&f1);
    MockCompWindow a (CompWindowTypeNormalMask, 10, 10, 40, 30);
    s.addWindowToBottom (&a);
    MockCompWindow b (CompWindowTypeNormalMask, 20, 20, 50, 20);
    s.addWindowToBottom (&b);
    MockCompWindow f2 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    s.addWindowToBottom (&f2);
    MockCompWindow desktop (CompWindowTypeDesktopMask, 0, 0, 1024, 768);
    s.addWindowToBottom (&desktop);
    EXPECT_EQ (s.fullscreenWindow(), &f1);
}

TEST (OpenGLWindowStack, CancelFullscreen1)
{
    WindowStack s (CompRect (0, 0, 1024, 768));
    MockCompWindow z (CompWindowTypeNormalMask, 500, 500, 345, 234);
    s.addWindowToBottom (&z);
    MockCompWindow f1 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    s.addWindowToBottom (&f1);
    MockCompWindow a (CompWindowTypeNormalMask, 10, 10, 40, 30);
    s.addWindowToBottom (&a);
    MockCompWindow b (CompWindowTypeNormalMask, 20, 20, 50, 20);
    s.addWindowToBottom (&b);
    MockCompWindow f2 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    s.addWindowToBottom (&f2);
    MockCompWindow desktop (CompWindowTypeDesktopMask, 0, 0, 1024, 768);
    s.addWindowToBottom (&desktop);
    EXPECT_EQ (NULL, s.fullscreenWindow());
}

TEST (OpenGLWindowStack, CancelFullscreen2)
{
    WindowStack s (CompRect (0, 0, 1024, 768));
    MockCompWindow x (CompWindowTypeNormalMask, -100, -100, 1, 1);
    s.addWindowToBottom (&x);
    MockCompWindow y (CompWindowTypeNormalMask, 2000, 2000, 123, 456);
    s.addWindowToBottom (&y);
    MockCompWindow z (CompWindowTypeNormalMask, 500, 500, 345, 234);
    s.addWindowToBottom (&z);
    MockCompWindow f1 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    s.addWindowToBottom (&f1);
    MockCompWindow a (CompWindowTypeNormalMask, 10, 10, 40, 30);
    s.addWindowToBottom (&a);
    MockCompWindow b (CompWindowTypeNormalMask, 20, 20, 50, 20);
    s.addWindowToBottom (&b);
    MockCompWindow f2 (CompWindowTypeNormalMask, 0, 0, 1024, 768);
    s.addWindowToBottom (&f2);
    MockCompWindow desktop (CompWindowTypeDesktopMask, 0, 0, 1024, 768);
    s.addWindowToBottom (&desktop);
    EXPECT_EQ (NULL, s.fullscreenWindow());
}

TEST (OpenGLWindowStack, Overflow)
{
    WindowStack s (CompRect (0, 0, 1024, 768));
    MockCompWindow a (CompWindowTypeNormalMask, 10, 10, 40, 30);
    s.addWindowToBottom (&a);
    MockCompWindow b (CompWindowTypeNormalMask, -10, -10, 1044, 788);
    s.addWindowToBottom (&b);
    MockCompWindow desktop (CompWindowTypeDesktopMask, 0, 0, 1024, 768);
    s.addWindowToBottom (&desktop);
    EXPECT_EQ (NULL, s.fullscreenWindow());
}


