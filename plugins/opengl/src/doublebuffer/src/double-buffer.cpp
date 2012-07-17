#include <iostream>

#include <core/logmessage.h>
#include <opengl/doublebuffer.h>

#include <cstdlib>

using namespace compiz::opengl;

char programName[] = "compiz_test_opengl_double_buffer";
bool debugOutput = false;

void
compiz::opengl::render (unsigned int flags,
                        const CompRegion &region,
                        GLDoubleBufferInterface &impl)
{
    if (flags & (PaintedFullscreen |
		 PaintedWithFramebufferObject))
    {
	impl.swap ();
    }
    else if (impl.blitAvailable ())
    {
	impl.blit (region);
    }
    else if (impl.fallbackBlitAvailable ())
    {
	impl.fallbackBlit (region);
    }
    else
    {
	/* FIXME: We need to use compLogMessage here, but for some
	 * reason it just crashes in the tests */
	std::cerr << "(compiz) - fatal: no back to front flip methods supported" << std::endl;
	abort ();
    }
}
