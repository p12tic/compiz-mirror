#ifndef RESIZE_GL_SCREEN_IMPL
#define RESIZE_GL_SCREEN_IMPL

#include "gl-screen-interface.h"

#include <opengl/opengl.h>

namespace resize
{

class GLScreenImpl : public GLScreenInterface
{
    public:
	GLScreenImpl (GLScreen *impl)
	    : mImpl (impl)
	{
	}

	virtual ~GLScreenImpl() {}

	virtual void glPaintOutputSetEnabled (GLScreenInterface *,
					      bool enable)
	{
	    mImpl->glPaintOutputSetEnabled(mImpl, enable);
	}

	static GLScreenImpl *wrap (GLScreen *impl)
	{
	    if (impl)
		return new GLScreenImpl (impl);
	    else
		return NULL;
	}

    private:
	GLScreen *mImpl;
};

} /* namespace resize */

#endif /* RESIZE_GL_SCREEN_INTERFACE */
