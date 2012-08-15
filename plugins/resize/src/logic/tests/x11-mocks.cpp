#include <X11/Xlib.h>
#include <X11/Xutil.h>

Status XSendEvent(Display*		/* display */,
		  Window		/* w */,
		  Bool		/* propagate */,
		  long		/* event_mask */,
		  XEvent*		/* event_send */)
{
    return Success;
}
