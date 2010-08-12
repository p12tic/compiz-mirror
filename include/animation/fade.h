#include "animation.h"
class FadeAnim :
virtual public Animation
{
public:
     FadeAnim (CompWindow *w,
	       WindowEvent curWindowEvent,
	       float duration,
	       const AnimEffect info,
	       const CompRect &icon);
public:
     void updateBB (CompOutput &output);
     bool updateBBUsed () { return true; }
     void updateAttrib (GLWindowPaintAttrib &wAttrib);
     virtual float getFadeProgress () { return progressLinear (); }
};