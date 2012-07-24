#include <iostream>

#include <core/logmessage.h>
#include <opengl/doublebuffer.h>

#include <cstdlib>

using namespace compiz::opengl;

char programName[] = "compiz_test_opengl_double_buffer";
bool debugOutput = false;

namespace compiz
{
namespace opengl
{

DoubleBuffer::DoubleBuffer ()
{
    setting[VSYNC] = true;
    setting[PERSISTENT_BACK_BUFFER] = false;
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
	swap ();
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

} // namespace opengl
} // namespace compiz
