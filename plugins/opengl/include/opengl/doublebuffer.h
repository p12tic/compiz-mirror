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

	virtual void swap () const = 0;
	virtual bool blitAvailable () const = 0;
	virtual void blit (const CompRegion &region) const = 0;
	virtual bool fallbackBlitAvailable () const = 0;
	virtual void fallbackBlit (const CompRegion &region) const = 0;
};

void blitBuffers (unsigned int flags,
		  const CompRegion &blitRegion,
		  GLDoubleBufferInterface &);

}
}
#endif
