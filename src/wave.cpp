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

const float WaveAnim::kMinDuration = 400;

// =====================  Effect: Wave  =========================

WaveAnim::WaveAnim (CompWindow *w,
		    WindowEvent curWindowEvent,
		    float duration,
		    const AnimEffect info,
		    const CompRect &icon) :
    Animation::Animation (w, curWindowEvent, duration, info, icon),
    TransformAnim::TransformAnim (w, curWindowEvent, duration, info, icon),
    GridTransformAnim::GridTransformAnim (w, curWindowEvent, duration, info,
					  icon)
{
}

void
WaveAnim::adjustDuration ()
{
    if (mTotalTime < kMinDuration)
    {
	mTotalTime = kMinDuration;
	mRemainingTime = mTotalTime;
    }
}

void
WaveAnim::initGrid ()
{
    mGridWidth = 2;
    mGridHeight = optValI (AnimationOptions::MagicLampWavyGridRes); // TODO new option
}

void
WaveAnim::step ()
{
    float forwardProgress = 1 - progressLinear ();
    if (mCurWindowEvent == WindowEventClose)
	forwardProgress = 1 - forwardProgress;

    CompRect winRect (mAWindow->savedRectsValid () ?
		      mAWindow->saveWinRect () :
		      mWindow->geometry ());
    CompRect outRect (mAWindow->savedRectsValid () ?
		      mAWindow->savedOutRect () :
		      mWindow->outputRect ());
    CompWindowExtents outExtents (mAWindow->savedRectsValid () ?
				  mAWindow->savedOutExtents () :
				  mWindow->output ());

    float waveHalfWidth = (outRect.height () * mModel->scale ().y () *
			   optValF (AnimationOptions::WaveWidth) / 2);

    float waveAmp = (pow ((float)outRect.height () / ::screen->height (), 0.4) *
		     0.04 * optValF (AnimationOptions::WaveAmpMult));

    float wavePosition =
	outRect.y () - waveHalfWidth +
	forwardProgress * (outRect.height () * mModel->scale ().y () +
			   2 * waveHalfWidth);

    GridModel::GridObject *object = mModel->objects ();
    for (int i = 0; i < mModel->numObjects (); i++, object++)
    {
	float origx = winRect.x () + mModel->scale ().x () *
	    (outRect.width () * object->gridPosition ().x () -
	     outExtents.left);
	float origy = winRect.y () + mModel->scale ().y () *
	    (outRect.height () * object->gridPosition ().y () -
	     outExtents.top);

	object->position ().setX (origx);
	object->position ().setY (origy);
	float distFromWaveCenter =
	    fabs (object->position ().y () - wavePosition);

	if (distFromWaveCenter < waveHalfWidth)
	    object->position ().
		setZ (waveAmp * (cos (distFromWaveCenter *
				      M_PI / waveHalfWidth) + 1) / 2);
	else
	    object->position ().setZ (0);
    }
}

