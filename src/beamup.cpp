#if 0
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

// =====================  Effect: Beam Up  =========================

BeamUpAnim::BeamUpAnim (CompWindow *w,
                        WindowEvent curWindowEvent,
                        float duration,
                        const AnimEffect info,
                        const CompRect &icon) :
    ParticleAnim::ParticleAnim (w, curWindowEvent, duration, info, icon)
{
}

void
BurnAnim::init ()
{
    int winWidth = w->width () + w->output ().left + w->output ().right;
    float slowDown = optValF (AnimationaddonOptions::BeamSlowdown);

    initLightDarkParticles (winWidth / 10, winWidth,
                            slowDown / 2.0f, slowDown);
}

static void
fxBeamUpGenNewBeam (CompWindow * w,
		   ParticleSystem * ps,
		   int x,
		   int y,
		   int width,
		   int height,
		   float size,
		   float time)
{
    ps->numParticles =
	width / animGetI (w, ANIMADDON_SCREEN_OPTION_BEAMUP_SPACING);

    float beaumUpLife = animGetF (w, ANIMADDON_SCREEN_OPTION_FIRE_LIFE);
    float beaumUpLifeNeg = 1 - beaumUpLife;
    float fadeExtra = 0.2f * (1.01 - beaumUpLife);
    float max_new = ps->numParticles * (time / 50) * (1.05 - beaumUpLife);

    // set color ABAB ANIMADDON_SCREEN_OPTION_BEAMUP_COLOR
    unsigned short *c =
	animGetC (w, ANIMADDON_SCREEN_OPTION_BEAMUP_COLOR);
    float colr1 = (float)c[0] / 0xffff;
    float colg1 = (float)c[1] / 0xffff;
    float colb1 = (float)c[2] / 0xffff;
    float colr2 = 1 / 1.7 * (float)c[0] / 0xffff;
    float colg2 = 1 / 1.7 * (float)c[1] / 0xffff;
    float colb2 = 1 / 1.7 * (float)c[2] / 0xffff;
    float cola = (float)c[3] / 0xffff;
    float rVal;

    float partw = 2.5 * animGetF (w, ANIMADDON_SCREEN_OPTION_BEAMUP_SIZE);

    Particle *part = ps->particles;

    for (int i = 0; i < ps->numParticles && max_new > 0; i++, part++)
    {
	if (part->life <= 0.0f)
	{
	    // give gt new life
	    rVal = (float)(random () & 0xff) / 255.0;
	    part->life = 1.0f;
	    part->fade = rVal * beaumUpLifeNeg + fadeExtra; // Random Fade Value

	    // set size
	    part->width = partw;
	    part->height = height;
	    part->w_mod = size * 0.2;
	    part->h_mod = size * 0.02;

	    // choose random x position
	    rVal = (float)(random () & 0xff) / 255.0;
	    part->x = x + ((width > 1) ? (rVal * width) : 0);
	    part->y = y;
	    part->z = 0.0;
	    part->xo = part->x;
	    part->yo = part->y;
	    part->zo = part->z;

	    // set speed and direction
	    part->xi = 0.0f;
	    part->yi = 0.0f;
	    part->zi = 0.0f;

	    part->r = colr1 - rVal * colr2;
	    part->g = colg1 - rVal * colg2;
	    part->b = colb1 - rVal * colb2;
	    part->a = cola;

	    // set gravity
	    part->xg = 0.0f;
	    part->yg = 0.0f;
	    part->zg = 0.0f;

	    ps->active = true;
	    max_new -= 1;
	}
	else
	{
	    part->xg = (part->x < part->xo) ? 1.0 : -1.0;
	}
    }

}

void
BeamUpAnim::step ()
{
    CompScreen *s = w->screen;

    ANIMADDON_DISPLAY (s->display);
    ANIMADDON_WINDOW (w);

    float timestep = (s->slowAnimations ? 2 :	// For smooth slow-mo (refer to display.c)
		      getIntenseTimeStep (s));

    aw->com->timestep = timestep;

    bool creating = (aw->com->curWindowEvent == WindowEventOpen ||
		     aw->com->curWindowEvent == WindowEventUnminimize ||
		     aw->com->curWindowEvent == WindowEventUnshade);

    mRemainingTime -= timestep;
    if (mRemainingTime <= 0)
	mRemainingTime = 0;	// avoid sub-zero values
    float new = 1 - (mRemainingTime) / (mTotalTime - timestep);

    if (creating)
	new = 1 - new;

    if (!aw->com->drawRegion)
	aw->com->drawRegion = XCreateRegion ();
    if (mRemainingTime > 0)
    {
	XRectangle rect;

	rect.x = outRect.x () + ((new / 2) * outRect.width ());
	rect.width = (1 - new) * outRect.width ();
	rect.y = outRect.y () + ((new / 2) * outRect.height ());
	rect.height = (1 - new) * outRect.height ();
	XUnionRectWithRegion (&rect, &emptyRegion, aw->com->drawRegion);
    }
    else
    {
	XUnionRegion (&emptyRegion, &emptyRegion, aw->com->drawRegion);
    }

    aw->com->useDrawRegion = (fabs (new) > 1e-5);

    if (mRemainingTime > 0 && aw->eng.numPs)
    {
	fxBeamUpGenNewBeam (w, &aw->eng.ps[1], 
			   outRect.x (), outRect.y () + (outRect.height () / 2), outRect.width (),
			   creating ?
			   (1 - new / 2) * outRect.height () : 
			   (1 - new) * outRect.height (),
			   outRect.width () / 40.0, time);

    }
    if (mRemainingTime <= 0 && aw->eng.numPs
	&& (aw->eng.ps[0].active || aw->eng.ps[1].active))
	mRemainingTime = 0.001f;

    if (!aw->eng.numPs || !aw->eng.ps)
    {
	if (aw->eng.ps)
	{
	    finiParticles (aw->eng.ps);
	    free (aw->eng.ps);
	    aw->eng.ps = NULL;
	}
	// Abort animation
	mRemainingTime = 0;
	return;
    }

    aw->eng.ps[0].x = outRect.x ();
    aw->eng.ps[0].y = outRect.y ();

    if (mRemainingTime > 0)
    {
	int nParticles = aw->eng.ps[1].numParticles;
	Particle *part = aw->eng.ps[1].particles;

	for (int i = 0; i < nParticles; i++, part++)
	    part->xg = (part->x < part->xo) ? 1.0 : -1.0;
    }
    aw->eng.ps[1].x = outRect.x ();
    aw->eng.ps[1].y = outRect.y ();
}

void
fxBeamupUpdateWindowAttrib (CompWindow *w,
			    WindowPaintAttrib * wAttrib)
{
    ANIMADDON_WINDOW (w);

    float forwardProgress = 0;
    if (mTotalTime - aw->com->timestep != 0)
	forwardProgress =
	    1 - mRemainingTime /
	    (mTotalTime - aw->com->timestep);
    forwardProgress = MIN (forwardProgress, 1);
    forwardProgress = MAX (forwardProgress, 0);

    if (aw->com->curWindowEvent == WindowEventOpen ||
	aw->com->curWindowEvent == WindowEventUnminimize)
    {
	forwardProgress = forwardProgress * forwardProgress;
	forwardProgress = forwardProgress * forwardProgress;
	forwardProgress = 1 - forwardProgress;
    }

    wAttrib->opacity = (GLushort) (aw->com->storedOpacity * (1 - forwardProgress));
}
#endif
