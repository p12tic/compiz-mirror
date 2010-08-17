#ifndef ANIMATION_GRIDTRANSFORM_H
#define ANIMATION_GRIDTRANSFORM_H
#include "animation.h"
class GridTransformAnim :
public GridAnim,
virtual public TransformAnim
{
public:
    GridTransformAnim (CompWindow *w,
		       WindowEvent curWindowEvent,
		       float duration,
		       const AnimEffect info,
		       const CompRect &icon);
protected:
    void init ();
    void updateTransform (GLMatrix &wTransform);
    void updateBB (CompOutput &output);
    bool updateBBUsed () { return true; }
    
    bool mUsingTransform; ///< whether transform matrix is used (default: true)
};
#endif
