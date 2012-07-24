#ifndef _COMPIZ_OPENGL_BUFFERBLIT_H
#define _COMPIZ_OPENGL_BUFFERBLIT_H

#include <core/region.h>

namespace compiz
{
namespace opengl
{

class DoubleBuffer
{
    public:

	virtual ~DoubleBuffer () {}

	virtual void swap (bool persistentBackBuffer=false) const = 0;
	virtual bool blitAvailable () const = 0;
	virtual void blit (const CompRegion &region) const = 0;
	virtual bool fallbackBlitAvailable () const = 0;
	virtual void fallbackBlit (const CompRegion &region) const = 0;

	void render (const CompRegion &region, bool fullscreen,
	             bool persistentBackBuffer=false);

};

}
}
#endif
