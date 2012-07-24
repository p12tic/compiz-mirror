#include <iostream>

#include <core/logmessage.h>
#include <opengl/doublebuffer.h>

#include <cstdlib>

using namespace compiz::opengl;

char programName[] = "compiz_test_opengl_double_buffer";
bool debugOutput = false;

void
compiz::opengl::GLDoubleBufferInterface::render (const CompRegion &region,
                                                 bool fullscreen,
                                                 bool persistentBackBuffer)
{
    if (fullscreen)
	swap (persistentBackBuffer);
    else if (blitAvailable ())
	blit (region);
    else if (fallbackBlitAvailable ())
	fallbackBlit (region);
    else
    {
	/* FIXME: We need to use compLogMessage here, but for some
	 * reason it just crashes in the tests */
	std::cerr << "(compiz) - fatal: no back to front flip methods supported" << std::endl;
	abort ();
    }
}
