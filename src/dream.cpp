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

const float DreamAnim::kDurationFactor = 1.67;

// =====================  Effect: Dream  =========================

DreamAnim::DreamAnim (CompWindow *w,
		      WindowEvent curWindowEvent,
		      float duration,
		      const AnimEffect info,
		      const CompRect &icon) :
    Animation::Animation (w, curWindowEvent, duration, info, icon),
    TransformAnim::TransformAnim (w, curWindowEvent, duration, info, icon),
    GridZoomAnim::GridZoomAnim (w, curWindowEvent, duration, info, icon)
{
}

void
DreamAnim::init ()
{
    GridZoomAnim::init ();

    if (!zoomToIcon ())
	mUsingTransform = false;
}

void
DreamAnim::adjustDuration ()
{
    if (zoomToIcon ())
	mTotalTime *= ZoomAnim::kDurationFactor;
    else
	mTotalTime *= kDurationFactor;

    mRemainingTime = mTotalTime;
}

void
DreamAnim::initGrid ()
{
    mGridWidth = 2;
    mGridHeight = optValI (AnimationOptions::MagicLampWavyGridRes); // TODO new option
}

void
DreamAnim::step ()
{
    GridZoomAnim::step ();

    float forwardProgress = getActualProgress ();

    CompRect winRect (mAWindow->savedRectsValid () ?
		      mAWindow->saveWinRect () :
		      mWindow->geometry ());
    CompRect outRect (mAWindow->savedRectsValid () ?
		      mAWindow->savedOutRect () :
		      mWindow->outputRect ());
    CompWindowExtents outExtents (mAWindow->savedRectsValid () ?
				  mAWindow->savedOutExtents () :
				  mWindow->output ());

    float waveAmpMax = MIN (outRect.height (), outRect.width ()) * 0.125f;
    float waveWidth = 10.0f;
    float waveSpeed = 7.0f;

    GridModel::GridObject *object = mModel->objects ();
    for (int i = 0; i < mModel->numObjects (); i++, object++)
    {
	float origx = (winRect.x () +
		       (outRect.width () * object->gridPosition ().x () -
			outExtents.left) * mModel->scale ().x ());
	float origy = (winRect.y () +
		       (outRect.height () * object->gridPosition ().y () -
			outExtents.top) * mModel->scale ().y ());

	object->position ().setX (
	    origx +
	    forwardProgress * waveAmpMax * mModel->scale ().x () *
	    sin (object->gridPosition ().y () * M_PI * waveWidth +
		waveSpeed * forwardProgress));
	object->position ().setY (origy);
    }
}

float
DreamAnim::getFadeProgress ()
{
    if (zoomToIcon ())
	return ZoomAnim::getFadeProgress ();

    return progressLinear ();
}

bool
DreamAnim::zoomToIcon ()
{
    return ((mCurWindowEvent == WindowEventMinimize ||
	     mCurWindowEvent == WindowEventUnminimize) &&
	    optValB (AnimationOptions::DreamZoomToTaskbar));
}

