#include <iostream>

#include <core/logmessage.h>
#include <opengl/bufferblit.h>

#include <cstdlib>

using namespace compiz::opengl;

char programName[] = "compiz_test_opengl_buffer_blit";
bool debugOutput = false;

void
compiz::opengl::blitBuffers (unsigned int flags,
			     const CompRegion &tmpRegion,
			     GLBufferBlitInterface &blit)
{
    if (flags & (PaintedFullscreen |
		 PaintedWithFramebufferObject))
    {
	blit.swapBuffers ();
    }
    else if (blit.subBufferBlitAvailable ())
    {
	blit.subBufferBlit (tmpRegion);
    }
    else
    {
	/* FIXME: We need to use compLogMessage here, but for some
	 * reason it just crashes in the tests */
	std::cerr << "(compiz) - fatal: no back to front flip methods supported" << std::endl;
	abort ();
    }
}
