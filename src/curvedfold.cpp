/*
 * Animation plugin for compiz/beryl
 *
 * animation.c
 *
 * Copyright : (C) 2006 Erkin Bahceci
 * E-mail    : erkinbah@gmail.com
 *
 * Based on Wobbly and Minimize plugins by
 *           : David Reveman
 * E-mail    : davidr@novell.com>
 *
 * Particle system added by : (C) 2006 Dennis Kasprzyk
 * E-mail                   : onestone@beryl-project.org
 *
 * Beam-Up added by : Florencio Guimaraes
 * E-mail           : florencio@nexcorp.com.br
 *
 * Hexagon tessellator added by : Mike Slegeir
 * E-mail                       : mikeslegeir@mail.utexas.edu>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "private.h"

// =====================  Effect: Curved Fold  =========================

FoldAnim::FoldAnim (CompWindow *w,
		    WindowEvent curWindowEvent,
		    float duration,
		    const AnimEffect info,
		    const CompRect &icon) :
    Animation::Animation (w, curWindowEvent, duration, info, icon),
    TransformAnim::TransformAnim (w, curWindowEvent, duration, info, icon),
    GridZoomAnim::GridZoomAnim (w, curWindowEvent, duration, info, icon)
{
}

float
FoldAnim::getFadeProgress ()
{
    // if shade/unshade, don't do anything
    if (mCurWindowEvent == WindowEventShade ||
	mCurWindowEvent == WindowEventUnshade)
	return 0;

    if (zoomToIcon ())
	return ZoomAnim::getFadeProgress ();

    return progressLinear ();
}

CurvedFoldAnim::CurvedFoldAnim (CompWindow *w,
				WindowEvent curWindowEvent,
				float duration,
				const AnimEffect info,
				const CompRect &icon) :
    Animation::Animation (w, curWindowEvent, duration, info, icon),
    TransformAnim::TransformAnim (w, curWindowEvent, duration, info, icon),
    FoldAnim::FoldAnim (w, curWindowEvent, duration, info, icon)
{
}

void
CurvedFoldAnim::initGrid ()
{
    mGridWidth = 2;
    mGridHeight = optValI (AnimationOptions::MagicLampWavyGridRes); // TODO new option
}

float
CurvedFoldAnim::getObjectZ (GridAnim::GridModel *mModel,
			    float forwardProgress,
			    float sinForProg,
			    float relDistToCenter,
			    float curveMaxAmp)
{
    return -(sinForProg *
	     (1 - pow (pow (2 * relDistToCenter, 1.3), 2)) *
	     curveMaxAmp *
	     mModel->scale ().x ());
}

void
CurvedFoldAnim::step ()
{
    GridZoomAnim::step ();

    float forwardProgress = getActualProgress ();

    CompRect winRect (mAWindow->savedRectsValid () ?
		      mAWindow->saveWinRect () :
		      mWindow->geometry ());
    CompRect inRect (mAWindow->savedRectsValid () ?
		     mAWindow->savedInRect () :
		     mWindow->inputRect ());
    CompRect outRect (mAWindow->savedRectsValid () ?
		      mAWindow->savedOutRect () :
		      mWindow->outputRect ());
    CompWindowExtents outExtents (mAWindow->savedRectsValid () ?
				  mAWindow->savedOutExtents () :
				  mWindow->output ());

    float curveMaxAmp = (0.4 * pow ((float)outRect.height () /
				    ::screen->height (), 0.4) *
			 optValF (AnimationOptions::CurvedFoldAmpMult));

    float sinForProg = sin (forwardProgress * M_PI / 2);

    GridModel::GridObject *object = mModel->objects ();
    for (int i = 0; i < mModel->numObjects (); i++, object++)
    {
	if (i % 2 == 0) // object is at the left side
	{
	    float origy = (winRect.y () +
			   (outRect.height () * object->gridPosition ().y () -
			    outExtents.top) * mModel->scale ().y ());

	    if (mCurWindowEvent == WindowEventShade ||
		mCurWindowEvent == WindowEventUnshade)
	    {
		// Execute shade mode

		// find position in window contents
		// (window contents correspond to 0.0-1.0 range)
		float relPosInWinContents =
		    (object->gridPosition ().y () * outRect.height () -
		     mDecorTopHeight) / winRect.height ();
		float relDistToCenter = fabs (relPosInWinContents - 0.5);

		if (object->gridPosition ().y () == 0)
		{
		    object->position ().setY (outRect.y ());
		    object->position ().setZ (0);
		}
		else if (object->gridPosition ().y () == 1)
		{
		    object->position ().setY (
			(1 - forwardProgress) * origy +
			forwardProgress *
			(outRect.y () + mDecorTopHeight + mDecorBottomHeight));
		    object->position ().setZ (0);
		}
		else
		{
		    object->position ().setY (
			(1 - forwardProgress) * origy +
			forwardProgress * (outRect.y () + mDecorTopHeight));
		    object->position ().setZ (
			getObjectZ (mModel, forwardProgress, sinForProg, relDistToCenter,
				    curveMaxAmp));
		}
	    }
	    else
	    {
		// Execute normal mode

		// find position within window borders
		// (border contents correspond to 0.0-1.0 range)
		float relPosInWinBorders =
		    (object->gridPosition ().y () * outRect.height () -
		     (inRect.y () - outRect.y ())) / inRect.height ();
		float relDistToCenter = fabs (relPosInWinBorders - 0.5);

		// prevent top & bottom shadows from extending too much
		if (relDistToCenter > 0.5)
		    relDistToCenter = 0.5;

		object->position ().setY (
		    (1 - forwardProgress) * origy +
		    forwardProgress * (inRect.y () + inRect.height () / 2.0));
		object->position ().setZ (
		    getObjectZ (mModel, forwardProgress, sinForProg, relDistToCenter,
				curveMaxAmp));
	    }
	}
	else // object is at the right side
	{
	    // Set y/z position to the y/z position of the object at the left
	    // on the same row (previous object)
	    object->position ().setY ((object - 1)->position ().y ());
	    object->position ().setZ ((object - 1)->position ().z ());
	}

	float origx = (winRect.x () +
		       (outRect.width () * object->gridPosition ().x () -
			outExtents.left) * mModel->scale ().x ());
	object->position ().setX (origx);
    }
}

bool
CurvedFoldAnim::zoomToIcon ()
{
    return ((mCurWindowEvent == WindowEventMinimize ||
	     mCurWindowEvent == WindowEventUnminimize) &&
	    optValB (AnimationOptions::CurvedFoldZoomToTaskbar));
}

