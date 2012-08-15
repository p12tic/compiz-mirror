#ifndef RESIZE_GL_SCREEN_INTERFACE
#define RESIZE_GL_SCREEN_INTERFACE

namespace resize
{

class GLScreenInterface
{
    public:
	virtual ~GLScreenInterface () {}
	virtual void glPaintOutputSetEnabled (GLScreenInterface *screen,
					      bool enable) = 0;
};

} /* namespace resize */

#endif /* RESIZE_GL_SCREEN_INTERFACE */
