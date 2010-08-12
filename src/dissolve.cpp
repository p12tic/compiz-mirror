/* Compiz Dissolve animation
 * dissolve.cpp
 *
 * Copyright (c) 2010 Jay Catherwood <jay.catherwood@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "private.h"

DissolveAnim::DissolveAnim (CompWindow *w,
		    WindowEvent curWindowEvent,
		    float duration,
		    const AnimEffect info,
		    const CompRect &icon) :
    Animation::Animation (w, curWindowEvent, duration, info, icon)
{
    mRadius = 3;
}

bool DissolveAnim::paintWindow (GLWindow                  *gWindow,
			  	      const GLWindowPaintAttrib &attrib,
				      const GLMatrix            &transform,
				      const CompRegion          &region,
				      unsigned int              mask)
{
    bool ret = false;

    GLWindowPaintAttrib a (attrib);
    float o = 0.2;

    a.opacity = attrib.opacity * o/(1.-4*o);
    ret |= gWindow->glPaint (a, transform, region, mask);

    GLMatrix t (transform);

    a.opacity = attrib.opacity * o/(1.-3*o);
    t.translate (mRadius*getDissolveProgress (), 0.f, 0.f);
    ret |= gWindow->glPaint (a, t, region, mask);

    a.opacity = attrib.opacity * o/(1.-2*o);
    t = transform;
    t.translate (-mRadius*getDissolveProgress (), 0.f, 0.f);
    ret |= gWindow->glPaint (a, t, region, mask);

    a.opacity = attrib.opacity * o/(1.-o);
    t = transform;
    t.translate (0.f, mRadius*getDissolveProgress (), 0.f);
    ret |= gWindow->glPaint (a, t, region, mask);

    a.opacity = attrib.opacity * o;
    t = transform;
    t.translate (0.f, -mRadius*getDissolveProgress (), 0.f);
    ret |= gWindow->glPaint (a, t, region, mask);

    return ret;
}

void
DissolveAnim::updateBB (CompOutput &output)
{
    mAWindow->expandBBWithWindow ();
}
