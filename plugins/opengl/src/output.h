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

#ifndef __COMPIZ_OPENGL_OUTPUT_H
#define __COMPIZ_OPENGL_OUTPUT_H
#include <core/match.h>
#include <core/window.h>
#include <core/rect.h>
#include <core/region.h>

namespace compiz {
namespace opengl {

/*
 * Why mock locally? Because...
 * 1. Code is cleaner without typecasts, so we need to ensure that
 *    Output::Window is-a CompWindow when not mocking.
 * 2. No polymorphism is required = fewer classes, less code, easier to follow.
 */
#ifdef OUTPUTWINDOW
class MockCompWindow
{
public:
    MockCompWindow (unsigned int type, int x, int y, int width, int height) :
	flags (type), reg (x, y, width, height) {}
    unsigned int type () const { return flags; }
    const CompRegion &region () const { return reg; }

private:
    unsigned int flags;
    CompRegion reg;
};
#else
#define OUTPUTWINDOW CompWindow
#endif

class Output
{
public:
    typedef OUTPUTWINDOW Window;
    Output (const CompRect &rect);
    void addWindowToBottom (Window *win); // called FRONT TO BACK
    Window *fullscreenWindow () const;

private:
    Window *fullscreen;
    CompRegion untouched;
};

} // namespace opengl
} // namespace compiz
#endif
