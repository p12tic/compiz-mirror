/*
 * Compiz, opengl plugin, DoubleBuffer class
 *
 * Copyright (c) 2012 Canonical Ltd.
 * Authors: Sam Spilsbury <sam.spilsbury@canonical.com>
 *          Daniel van Vugt <daniel.van.vugt@canonical.com>
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

#include <cstdlib>
#include <cassert>
#include "opengl/doublebuffer.h"

using namespace compiz::opengl;

char programName[] = "compiz_test_opengl_double_buffer";
bool debugOutput = false;

namespace compiz
{
namespace opengl
{

DoubleBuffer::DoubleBuffer (const std::list <VSyncMethod::Ptr> &vsyncMethods) :
    unthrottledFrames (0),
    vsyncMethods (vsyncMethods)
{
    setting[VSYNC] = true;
    setting[HAVE_PERSISTENT_BACK_BUFFER] = false;
    setting[NEED_PERSISTENT_BACK_BUFFER] = false;
}

DoubleBuffer::~DoubleBuffer ()
{
}

void
DoubleBuffer::set (Setting name, bool value)
{
    setting[name] = value;
}

void
DoubleBuffer::render (const CompRegion &region,
                      bool fullscreen)
{
    if (fullscreen)
    {
	if (setting[VSYNC])
	    vsync (Flip);

	swap ();

	if (setting[NEED_PERSISTENT_BACK_BUFFER] &&
	    !setting[HAVE_PERSISTENT_BACK_BUFFER])
	{
	    copyFrontToBack ();
	}
    }
    else
    {
	if (setting[VSYNC])
	    vsync (PartialCopy);

	if (blitAvailable ())
	    blit (region);
	else if (fallbackBlitAvailable ())
	    fallbackBlit (region);
	else
	{
	    // This will never happen unless you make a porting mistake...
	    assert (false);
	    abort ();
	}
    }
}

void
DoubleBuffer::vsync (BufferSwapType swapType)
{
    bool throttled = true;

    for (std::list <VSyncMethod::Ptr>::iterator it = vsyncMethods.begin ();
	 it != vsyncMethods.end ();
	 ++it)
    {
	VSyncMethod::Ptr &method (*it);

	/* Try and use this method, check if this method
	 * throttled us too */
	if (method->enableForBufferSwapType (swapType, throttled))
	{
	    if (lastSuccessfulVSyncMethod &&
		lastSuccessfulVSyncMethod != method)
		lastSuccessfulVSyncMethod->disable ();

	    lastSuccessfulVSyncMethod = method;
	    break;
	}
	else
	{
	    throttled = false;
	    method->disable ();
	}
    }

    if (!throttled)
	unthrottledFrames++;
    else
	unthrottledFrames = 0;
}

bool
DoubleBuffer::hardwareVSyncFunctional ()
{
    return unthrottledFrames < 5;
}

} // namespace opengl
} // namespace compiz
