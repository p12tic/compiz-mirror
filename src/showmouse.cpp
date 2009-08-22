/*
 *
 * Compiz show mouse pointer plugin
 *
 * showmouse.cpp
 *
 * Copyright : (C) 2008 by Dennis Kasprzyk
 * E-mail    : onestone@opencompositing.org
 *
 * Ported to Compiz 0.9 by:
 * Copyright : (C) 2009 by Sam Spilsbury
 * E-mail    : smpillaz@gmail.com
 *
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
 */

#include "showmouse.h"

COMPIZ_PLUGIN_20090315 (showmouse, ShowmousePluginVTable);

void
ShowmouseScreen::genNewParticles (int f_time)
{
    if (!ps)
	return;

    Bool rColor     = optionGetRandom ();
    float life      = optionGetLife ();
    float lifeNeg   = 1 - life;
    float fadeExtra = 0.2f * (1.01 - life);
    float max_new   = ps->particles.size () * ((float)f_time / 50) * (1.05 - life);

    unsigned short *c = optionGetColor ();

    float colr1 = (float)c[0] / 0xffff;
    float colg1 = (float)c[1] / 0xffff;
    float colb1 = (float)c[2] / 0xffff;
    float colr2 = 1.0 / 4.0 * (float)c[0] / 0xffff;
    float colg2 = 1.0 / 4.0 * (float)c[1] / 0xffff;
    float colb2 = 1.0 / 4.0 * (float)c[2] / 0xffff;
    float cola  = (float)c[3] / 0xffff;
    float rVal;

    float partw = optionGetSize () * 5;
    float parth = partw;

    int i, j;

    float pos[10][2];
    int nE       = MIN (10, optionGetEmiters ());
    float rA     = (2 * M_PI) / nE;
    int radius   = optionGetRadius ();
    for (i = 0; i < nE; i++)
    {
	pos[i][0]  = sin (rot + (i * rA)) * radius;
	pos[i][0] += mousePos.x ();
	pos[i][1]  = cos (rot + (i * rA)) * radius;
	pos[i][1] += mousePos.y ();
    }

    for (i = 0; i < ps->particles.size () && max_new > 0; i++)
    {
	Particle *part = ps->particles.at (i);
	if (part->life <= 0.0f)
	{
	    // give gt new life
	    rVal = (float)(random() & 0xff) / 255.0;
	    part->life = 1.0f;
	    part->fade = rVal * lifeNeg + fadeExtra; // Random Fade Value

	    // set size
	    part->width = partw;
	    part->height = parth;
	    rVal = (float)(random() & 0xff) / 255.0;
	    part->w_mod = part->h_mod = -1;

	    // choose random position

	    j        = random() % nE;
	    part->x  = pos[j][0];
	    part->y  = pos[j][1];
	    part->z  = 0.0;
	    part->xo = part->x;
	    part->yo = part->y;
	    part->zo = part->z;

	    // set speed and direction
	    rVal     = (float)(random() & 0xff) / 255.0;
	    part->xi = ((rVal * 20.0) - 10.0f);
	    rVal     = (float)(random() & 0xff) / 255.0;
	    part->yi = ((rVal * 20.0) - 10.0f);
	    part->zi = 0.0f;

	    if (rColor)
	    {
		// Random colors! (aka Mystical Fire)
		rVal    = (float)(random() & 0xff) / 255.0;
		part->r = rVal;
		rVal    = (float)(random() & 0xff) / 255.0;
		part->g = rVal;
		rVal    = (float)(random() & 0xff) / 255.0;
		part->b = rVal;
	    }
	    else
	    {
		rVal    = (float)(random() & 0xff) / 255.0;
		part->r = colr1 - rVal * colr2;
		part->g = colg1 - rVal * colg2;
		part->b = colb1 - rVal * colb2;
	    }
	    // set transparancy
	    part->a = cola;

	    // set gravity
	    part->xg = 0.0f;
	    part->yg = 0.0f;
	    part->zg = 0.0f;

	    ps->active = TRUE;
	    max_new   -= 1;
	}
    }

    fprintf (stderr, "generated new particles at mean position %i %i\n", mousePos.x (), mousePos.y ());

}


void
ShowmouseScreen::damageRegion ()
{
    int          i;
    float        w, h, x1, x2, y1, y2;

    if (!ps)
	return;

    x1 = screen->width ();
    x2 = 0;
    y1 = screen->height ();
    y2 = 0;

    for (i = 0; i < ps->particles.size (); i++)
    {
	Particle *p = ps->particles.at (i);

	if (!p)
	    break;

	w = p->width / 2;
	h = p->height / 2;

	w += (w * p->w_mod) * p->life;
	h += (h * p->h_mod) * p->life;
	
	x1 = MIN (x1, p->x - w);
	x2 = MAX (x2, p->x + w);
	y1 = MIN (y1, p->y - h);
	y2 = MAX (y2, p->y + h);
    }

    CompRegion r (floor (x1), floor (y1), (ceil (x2) - floor (x1)),
					  (ceil (y2) - floor (y1)));
    cScreen->damageRegion (r);
}

void
ShowmouseScreen::positionUpdate (const CompPoint &p)
{
    mousePos = p;
}


void
ShowmouseScreen::preparePaint (int f_time)
{
    if (active && !pollHandle.active ())
    {
	mousePos = MousePoller::getCurrentPosition ();
	pollHandle.start ();
    }

    if (active && !ps)
    {
	ps = new ParticleSystem (optionGetNumParticles ());
	if (!ps)
	{
	    cScreen->preparePaint (f_time);
	    return;
	}

	ps->slowdown = optionGetSlowdown ();
	ps->darken = optionGetDarken ();
	ps->blendMode = (optionGetBlend()) ? GL_ONE :
			    GL_ONE_MINUS_SRC_ALPHA;

	glGenTextures(1, &ps->tex);
	glBindTexture(GL_TEXTURE_2D, ps->tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, starTex);
	glBindTexture(GL_TEXTURE_2D, 0);
    }

    rot = fmod (rot + (((float)f_time / 1000.0) * 2 * M_PI *
		    optionGetRotationSpeed ()), 2 * M_PI);

    if (ps && ps->active)
    {
	ps->updateParticles (f_time);
	damageRegion ();
    }

    if (ps && active)
	genNewParticles (f_time);

    cScreen->preparePaint (f_time);
}

void
ShowmouseScreen::donePaint ()
{
    if (active || (ps && ps->active))
	damageRegion ();

    if (!active && pollHandle.active ())
    {
	pollHandle.stop ();
    }

    if (!active && ps && !ps->active)
    {
	ps->finiParticles ();
	delete ps;
	ps = NULL;
    }

    cScreen->donePaint ();
}

bool
ShowmouseScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
				const GLMatrix		  &transform,
				const CompRegion	  &region,
				CompOutput		  *output,
				unsigned int		  mask)
{

    bool           status;
    GLMatrix       sTransform;

    status = gScreen->glPaintOutput (attrib, transform, region, output, mask);

    if (!ps || !ps->active)
	return status;

    //sTransform.reset ();

    sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

    glPushMatrix ();
    glLoadMatrixf (sTransform.getMatrix ());

    ps->drawParticles ();

    fprintf (stderr, "drawing particles\n");

    glPopMatrix();

    glColor4usv (defaultColor);

    return status;
}

bool
ShowmouseScreen::terminate (CompAction         *action,
			    CompAction::State  state,
			    CompOption::Vector options)
{
    active = false;
    damageRegion ();

    return true;
}

bool
ShowmouseScreen::initiate (CompAction         *action,
			   CompAction::State  state,
			   CompOption::Vector options)
{
    if (active)
	return terminate (action, state, options);

    active = true;

    return true;
}

ShowmouseScreen::ShowmouseScreen (CompScreen *screen) :
    PluginClassHandler <ShowmouseScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    active (false),
    ps (NULL),
    rot (0.0f)
{
    CompositeScreenInterface::setHandler (cScreen);
    GLScreenInterface::setHandler (gScreen);

    pollHandle.setCallback (boost::bind (&ShowmouseScreen::positionUpdate, this,
					 _1));

    optionSetInitiateInitiate (boost::bind (&ShowmouseScreen::initiate, this,
					    _1, _2, _3));
    optionSetInitiateTerminate (boost::bind (&ShowmouseScreen::terminate, this,
					     _1, _2, _3));

    optionSetInitiateButtonInitiate (boost::bind (&ShowmouseScreen::initiate,
						  this,  _1, _2, _3));
    optionSetInitiateButtonTerminate (boost::bind (&ShowmouseScreen::terminate,
						   this,  _1, _2, _3));

    optionSetInitiateEdgeInitiate (boost::bind (&ShowmouseScreen::initiate,
						this,  _1, _2, _3));
    optionSetInitiateEdgeTerminate (boost::bind (&ShowmouseScreen::terminate,
						 this,  _1, _2, _3));
}

ShowmouseScreen::~ShowmouseScreen ()
{
    if (ps)
    {
	ps->finiParticles ();
	delete ps;
    }

    if (pollHandle.active ())
	pollHandle.stop ();
}

bool
ShowmousePluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI) ||
	!CompPlugin::checkPluginABI ("particlesystem", COMPIZ_PARTICLESYSTEM_ABI) ||
	!CompPlugin::checkPluginABI ("mousepoll", COMPIZ_MOUSEPOLL_ABI))
	return false;

    return true;
}
