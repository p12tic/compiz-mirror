#ifndef _COMPIZ_OPENGL_BUFFERBLIT_H
#define _COMPIZ_OPENGL_BUFFERBLIT_H

#include <core/region.h>

namespace compiz
{
namespace opengl
{

const unsigned int  PaintedWithFramebufferObject = (1 << 0);
const unsigned int  PaintedFullscreen = (1 << 1);

class GLDoubleBufferInterface
{
    public:

	virtual ~GLDoubleBufferInterface () {}

	virtual void swapBuffers () const = 0;
	virtual bool subBufferBlitAvailable () const = 0;
	virtual void subBufferBlit (const CompRegion &region) const = 0;
	virtual bool subBufferCopyAvailable () const = 0;
	virtual void subBufferCopy (const CompRegion &region) const = 0;
};

void blitBuffers (unsigned int flags,
		  const CompRegion &blitRegion,
		  GLDoubleBufferInterface &);

}
}
#endif
