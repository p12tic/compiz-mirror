/*
 * Compiz, opengl plugin, vsync methods
 *
 * Copyright © 2012 Canonical Ltd.
 * Copyright © 2008 Dennis Kasprzyk
 * Copyright © 2007 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Dennis Kasprzyk not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Dennis Kasprzyk makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DENNIS KASPRZYK DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DENNIS KASPRZYK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Dennis Kasprzyk <onestone@compiz-fusion.org>
 *          David Reveman <davidr@novell.com>
 *	    Daniel van Vugt <daniel.van.vugt@canonical.com>
 *	    Sam Spilsbury <sam.spilsbury@canonical.com>
 */
#include <vsyncmethod.h>
#include <vsync-method-wait-video-sync.h>

namespace compiz
{
    namespace opengl
    {
	namespace impl
	{
	    class PrivateWaitVSyncMethod
	    {
		public:

		    PrivateWaitVSyncMethod (const GLXWaitVideoSyncSGIFunc &);

		    GLXWaitVideoSyncSGIFunc waitVideoSync;
		    unsigned int            lastVSyncCounter;
	    };
	}
    }
}

namespace cgl = compiz::opengl;
namespace cgli = compiz::opengl::impl;

cgli::PrivateWaitVSyncMethod::PrivateWaitVSyncMethod (const cgli::GLXWaitVideoSyncSGIFunc &waitVideoSync) :
    waitVideoSync (waitVideoSync),
    lastVSyncCounter (0)
{
}

cgli::WaitVSyncMethod::WaitVSyncMethod (const cgli::GLXWaitVideoSyncSGIFunc &waitVideoSync) :
    priv (new cgli::PrivateWaitVSyncMethod (waitVideoSync))
{
}

bool
cgli::WaitVSyncMethod::enableForBufferSwapType (cgl::BufferSwapType swapType,
						bool		    &throttledFrame)
{
    unsigned int oldVideoSyncCounter = priv->lastVSyncCounter;
    priv->waitVideoSync (1, 0, &priv->lastVSyncCounter);

    /* Check if this frame was actually throttled */
    if (priv->lastVSyncCounter == oldVideoSyncCounter)
	throttledFrame = false;
    else
	throttledFrame = true;

    return true;
}

void
cgli::WaitVSyncMethod::disable ()
{
}
