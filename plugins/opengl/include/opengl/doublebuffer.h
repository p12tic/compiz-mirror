#ifndef _COMPIZ_OPENGL_BUFFERBLIT_H
#define _COMPIZ_OPENGL_BUFFERBLIT_H

#include <core/region.h>
#include <vsyncmethod.h>

namespace compiz
{
namespace opengl
{

class DoubleBuffer
{
    public:
	DoubleBuffer (const std::list <VSyncMethod::Ptr> &vsyncMethods);
	virtual ~DoubleBuffer ();

	virtual void swap () const = 0;
	virtual bool blitAvailable () const = 0;
	virtual void blit (const CompRegion &region) const = 0;
	virtual bool fallbackBlitAvailable () const = 0;
	virtual void fallbackBlit (const CompRegion &region) const = 0;
	virtual void copyFrontToBack () const = 0;

	typedef enum
	{
	    VSYNC,
	    HAVE_PERSISTENT_BACK_BUFFER,
	    NEED_PERSISTENT_BACK_BUFFER,
	    _NSETTINGS
	} Setting;

	void set (Setting name, bool value);
	void render (const CompRegion &region, bool fullscreen);
	void vsync (BufferSwapType swapType);

	bool hardwareVSyncFunctional ();

    protected:
	bool setting[_NSETTINGS];

    private:
	unsigned int                 unthrottledFrames;
	std::list <VSyncMethod::Ptr> vsyncMethods;
	VSyncMethod::Ptr	     lastSuccessfulVSyncMethod;
};

}
}
#endif
