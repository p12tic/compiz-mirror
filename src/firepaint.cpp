/*
 * Compiz fire effect plugin
 *
 * firepaint.c
 *
 * Copyright : (C) 2007 by Dennis Kasprzyk
 * E-mail    : onestone@beryl-project.org
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

#include "firepaint.h"

COMPIZ_PLUGIN_20090315 (firepaint, FirePluginVTable);

ParticleSystem::ParticleSystem (int n) :
    particles (NULL),
    vertices_cache (NULL),
    coords_cache (NULL),
    colors_cache (NULL),
    dcolors_cache (NULL)
{
    initParticles (n);
}

ParticleSystem::ParticleSystem () :
    particles (NULL),
    vertices_cache (NULL),
    coords_cache (NULL),
    colors_cache (NULL),
    dcolors_cache (NULL)
{
}

ParticleSystem::~ParticleSystem ()
{
    finiParticles ();
}

void
ParticleSystem::initParticles (int            f_numParticles)
{
    particles.clear ();

    tex = 0;
    slowdown = 1;
    active = FALSE;

    // Initialize cache
    vertices_cache = NULL;
    colors_cache   = NULL;
    coords_cache   = NULL;
    dcolors_cache  = NULL;

    vertex_cache_count  = 0;
    color_cache_count   = 0;
    coords_cache_count  = 0;
    dcolors_cache_count = 0;

    int i;

    for (i = 0; i < f_numParticles; i++)
    {
	Particle *p = new Particle ();
	p->life = 0.0f;
	particles.push_back (p);
    }
}

void
ParticleSystem::drawParticles ()
{
    GLfloat *dcolors;
    GLfloat *vertices;
    GLfloat *coords;
    GLfloat *colors;

    glPushMatrix ();

    glEnable (GL_BLEND);

    if (tex)
    {
	glBindTexture (GL_TEXTURE_2D, tex);
	glEnable (GL_TEXTURE_2D);
    }

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    /* Check that the cache is big enough */

    if (particles.size () > vertex_cache_count)
    {
	vertices_cache = (GLfloat *) realloc (vertices_cache,
				      	      particles.size () * 4 * 3 *
				      	      sizeof (GLfloat));
	vertex_cache_count = particles.size ();
    }

    if (particles.size () > coords_cache_count)
    {
	coords_cache = (GLfloat *) realloc (coords_cache,
				    	    particles.size () * 4 * 2 *
				    	    sizeof (GLfloat));
	coords_cache_count = particles.size ();
    }

    if (particles.size () > color_cache_count)
    {
	colors_cache = (GLfloat *) realloc (colors_cache,
				    	    particles.size () * 4 * 4 *
				    	    sizeof (GLfloat));
	color_cache_count = particles.size ();
    }

    if (darken > 0)
    {
	if (dcolors_cache_count < particles.size ())
	{
	    dcolors_cache = (GLfloat *) realloc (dcolors_cache,
					 	 particles.size () * 4 * 4 *
					 	 sizeof (GLfloat));
	    dcolors_cache_count = particles.size ();
	}
    }

    dcolors  = dcolors_cache;
    vertices = vertices_cache;
    coords   = coords_cache;
    colors   = colors_cache;

    int numActive = 0;

    foreach (Particle *part, particles)
    {
	if (part->life > 0.0f)
	{
	    numActive += 4;

	    float w = part->width / 2;
	    float h = part->height / 2;

	    w += (w * part->w_mod) * part->life;
	    h += (h * part->h_mod) * part->life;

	    vertices[0]  = part->x - w;
	    vertices[1]  = part->y - h;
	    vertices[2]  = part->z;

	    vertices[3]  = part->x - w;
	    vertices[4]  = part->y + h;
	    vertices[5]  = part->z;

	    vertices[6]  = part->x + w;
	    vertices[7]  = part->y + h;
	    vertices[8]  = part->z;

	    vertices[9]  = part->x + w;
	    vertices[10] = part->y - h;
	    vertices[11] = part->z;

	    vertices += 12;

	    coords[0] = 0.0;
	    coords[1] = 0.0;

	    coords[2] = 0.0;
	    coords[3] = 1.0;

	    coords[4] = 1.0;
	    coords[5] = 1.0;

	    coords[6] = 1.0;
	    coords[7] = 0.0;

	    coords += 8;

	    colors[0]  = part->r;
	    colors[1]  = part->g;
	    colors[2]  = part->b;
	    colors[3]  = part->life * part->a;
	    colors[4]  = part->r;
	    colors[5]  = part->g;
	    colors[6]  = part->b;
	    colors[7]  = part->life * part->a;
	    colors[8]  = part->r;
	    colors[9]  = part->g;
	    colors[10] = part->b;
	    colors[11] = part->life * part->a;
	    colors[12] = part->r;
	    colors[13] = part->g;
	    colors[14] = part->b;
	    colors[15] = part->life * part->a;

	    colors += 16;

	    if (darken > 0)
	    {

		dcolors[0]  = part->r;
		dcolors[1]  = part->g;
		dcolors[2]  = part->b;
		dcolors[3]  = part->life * part->a * darken;
		dcolors[4]  = part->r;
		dcolors[5]  = part->g;
		dcolors[6]  = part->b;
		dcolors[7]  = part->life * part->a * darken;
		dcolors[8]  = part->r;
		dcolors[9]  = part->g;
		dcolors[10] = part->b;
		dcolors[11] = part->life * part->a * darken;
		dcolors[12] = part->r;
		dcolors[13] = part->g;
		dcolors[14] = part->b;
		dcolors[15] = part->life * part->a * darken;

		dcolors += 16;
	    }
	}
    }

    glEnableClientState (GL_COLOR_ARRAY);

    glTexCoordPointer (2, GL_FLOAT, 2 * sizeof (GLfloat), coords_cache);
    glVertexPointer (3, GL_FLOAT, 3 * sizeof (GLfloat), vertices_cache);

    // darken the background

    if (darken > 0)
    {
	glBlendFunc (GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
	glColorPointer (4, GL_FLOAT, 4 * sizeof (GLfloat), dcolors_cache);
	glDrawArrays (GL_QUADS, 0, numActive);
    }

    // draw particles
    glBlendFunc (GL_SRC_ALPHA, blendMode);

    glColorPointer (4, GL_FLOAT, 4 * sizeof (GLfloat), colors_cache);
    glDrawArrays (GL_QUADS, 0, numActive);
    glDisableClientState (GL_COLOR_ARRAY);

    glPopMatrix ();
    glColor4usv (defaultColor);

    GLScreen::get(screen)->setTexEnvMode (GL_REPLACE); // ??? 

    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable (GL_TEXTURE_2D);
    glDisable (GL_BLEND);
}

void
ParticleSystem::updateParticles (float          time)
{
    float speed = (time / 50.0);
    float f_slowdown = slowdown * (1 - MAX (0.99, time / 1000.0) ) * 1000;

    active = FALSE;

    foreach (Particle *part, particles)
    {
	if (part->life > 0.0f)
	{
	    // move particle
	    part->x += part->xi / f_slowdown;
	    part->y += part->yi / f_slowdown;
	    part->z += part->zi / f_slowdown;

	    // modify speed
	    part->xi += part->xg * speed;
	    part->yi += part->yg * speed;
	    part->zi += part->zg * speed;

	    // modify life
	    part->life -= part->fade * speed;
	    active = TRUE;
	}
    }
}

void
ParticleSystem::finiParticles ()
{
    particles.clear ();

    if (tex)
	glDeleteTextures (1, &tex);

    if (vertices_cache)
    {
	free (vertices_cache);
	vertices_cache = NULL;
    }

    if (colors_cache)
    {
	free (colors_cache);
	colors_cache = NULL;
    }

    if (coords_cache)
    {
	free (coords_cache);
	coords_cache = NULL;
    }

    if (dcolors_cache)
    {
	free (dcolors_cache);
	dcolors_cache = NULL;
    }
}

static void
toggleFunctions (bool enabled)
{
    FIRE_SCREEN (screen);
    screen->handleEventSetEnabled (fs, enabled);
    fs->cScreen->preparePaintSetEnabled (fs, enabled);
    fs->gScreen->glPaintOutputSetEnabled (fs, enabled);
    fs->cScreen->donePaintSetEnabled (fs, enabled);
}

void
FireScreen::fireAddPoint (int        x,
		          int        y,
		          bool       requireGrab)
{

    if (!requireGrab || grabIndex)
    {
	if (pointsSize < numPoints + 1)
	{
	    points = (XPoint *) realloc (points,
				         (pointsSize + NUM_ADD_POINTS) *
				         sizeof (XPoint));
	    pointsSize += NUM_ADD_POINTS;
	}

	points[numPoints].x = x;
	points[numPoints].y = y;
	numPoints++;
    }
}


bool
FireScreen::addParticle (CompAction         *action,
			 CompAction::State  state,
			 CompOption::Vector options)
{
    float x, y;

    x = CompOption::getFloatOptionNamed (options, "x", 0);
    y = CompOption::getFloatOptionNamed (options, "y", 0);

    fireAddPoint (x, y, FALSE);

    cScreen->damageScreen ();

    return true;
}


bool
FireScreen::initiate (CompAction         *action,
		      CompAction::State  state,
		      CompOption::Vector options)
{
    if (screen->otherGrabExist (0) )
        return false;

    if (!grabIndex)
        grabIndex = screen->pushGrab (None, "firepaint");

    if (state & CompAction::StateInitButton)
        action->setState (action->state () | CompAction::StateTermButton);

    if (state & CompAction::StateInitKey)
        action->setState (action->state () | CompAction::StateTermKey);

    fireAddPoint (pointerX, pointerY, TRUE);

    return TRUE;
}

bool
FireScreen::terminate (CompAction         *action,
		       CompAction::State  state,
		       CompOption::Vector options)
{

    if (grabIndex)
    {
	screen->removeGrab (grabIndex, NULL);
	grabIndex = 0;
    }

    action->setState (action->state () & ~(CompAction::StateTermKey |
				           CompAction::StateTermButton));

    return FALSE;
}


bool
FireScreen::clear (CompAction         *action,
		   CompAction::State  state,
		   CompOption::Vector options)
{
    numPoints = 0;
    return true;
}


void
FireScreen::preparePaint (int      time)
{
    int i;
    Particle *part;
    float size = 4;
    float bg = (float) optionGetBgBrightness () / 100.0;

    if (init && numPoints)
    {
	ps->initParticles (optionGetNumParticles ());
	init = FALSE;

	glGenTextures (1, &ps->tex);
	glBindTexture (GL_TEXTURE_2D, ps->tex);

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0,
		      GL_RGBA, GL_UNSIGNED_BYTE, fireTex);
	glBindTexture (GL_TEXTURE_2D, 0);

	ps->slowdown  = optionGetFireSlowdown ();
	ps->darken    = 0.5;
	ps->blendMode = GL_ONE;

    }

    if (!init)
	ps->updateParticles (time);

    if (numPoints)
    {
	float max_new = MIN (ps->particles.size (),  numPoints * 2) *
			((float) time / 50.0) *
			(1.05 -	optionGetFireLife());
	float rVal;
	int rVal2;

	for (i = 0; i < ps->particles.size () && max_new > 0; i++)
	{
	    part = ps->particles.at (i);
	    if (part->life <= 0.0f)
	    {
		/* give gt new life */
		rVal = (float) (random () & 0xff) / 255.0;
		part->life = 1.0f;
		/* Random Fade Value */
		part->fade = (rVal * (1 - optionGetFireLife ()) +
			      (0.2f * (1.01 - optionGetFireLife ())));

		/* set size */
		part->width  = optionGetFireSize ();
		part->height = optionGetFireSize () * 1.5;
		rVal = (float) (random () & 0xff) / 255.0;
		part->w_mod = size * rVal;
		part->h_mod = size * rVal;

		/* choose random position */
		rVal2 = random () % numPoints;
		part->x = points[rVal2].x;
		part->y = points[rVal2].y;
		part->z = 0.0;
		part->xo = part->x;
		part->yo = part->y;
		part->zo = part->z;

		/* set speed and direction */
		rVal = (float) (random () & 0xff) / 255.0;
		part->xi = ( (rVal * 20.0) - 10.0f);
		rVal = (float) (random () & 0xff) / 255.0;
		part->yi = ( (rVal * 20.0) - 15.0f);
		part->zi = 0.0f;
		rVal = (float) (random () & 0xff) / 255.0;

		if (optionGetFireMystical () )
		{
		    /* Random colors! (aka Mystical Fire) */
		    rVal = (float) (random () & 0xff) / 255.0;
		    part->r = rVal;
		    rVal = (float) (random () & 0xff) / 255.0;
		    part->g = rVal;
		    rVal = (float) (random () & 0xff) / 255.0;
		    part->b = rVal;
		}
		else
		{
		    part->r = (float) optionGetFireColorRed () / 0xffff -
			      (rVal / 1.7 *
			       (float) optionGetFireColorRed () / 0xffff);
		    part->g = (float) optionGetFireColorGreen () / 0xffff -
			      (rVal / 1.7 *
			      (float) optionGetFireColorGreen () / 0xffff);
		    part->b = (float) optionGetFireColorBlue () / 0xffff -
			      (rVal / 1.7 *
			      (float) optionGetFireColorBlue () / 0xffff);
		}

		/* set transparancy */
		part->a = (float) optionGetFireColorAlpha () / 0xffff;

		/* set gravity */
		part->xg = (part->x < part->xo) ? 1.0 : -1.0;
		part->yg = -3.0f;
		part->zg = 0.0f;

		ps->active = TRUE;

		max_new -= 1;
	    }
	    else
	    {
		part->xg = (part->x < part->xo) ? 1.0 : -1.0;
	    }
	}
    }

    if (numPoints && brightness != bg)
    {
	float div = 1.0 - bg;
	div *= (float) time / 500.0;
	brightness = MAX (bg, brightness - div);

    }

    if (!numPoints && brightness != 1.0)
    {
	float div = 1.0 - bg;
	div *= (float) time / 500.0;
	brightness = MIN (1.0, brightness + div);

    }

    if (!init && !numPoints && !ps->active)
    {
	ps->finiParticles ();
	init = TRUE;
    }

    cScreen->preparePaint (time);
}

bool
FireScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
			   const GLMatrix	     &transform,
			   const CompRegion	     &region,
			   CompOutput 		     *output,
			   unsigned int		     mask)
{
    bool status;

    status = gScreen->glPaintOutput (attrib, transform, region, output, mask);

    if ( (!init && ps->active) || brightness < 1.0)
    {
	GLMatrix sTransform = transform;

	sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

	glPushMatrix ();
	glLoadMatrixf (sTransform.getMatrix ());

	if (brightness < 1.0)
	{
	    glColor4f (0.0, 0.0, 0.0, 1.0 - brightness);
	    glEnable (GL_BLEND);
	    glBegin (GL_QUADS);
	    glVertex2d (output->region ()->extents.x1,
			output->region ()->extents.y1);
	    glVertex2d (output->region ()->extents.x1,
			output->region ()->extents.y2);
	    glVertex2d (output->region ()->extents.x2,
			output->region ()->extents.y2);
	    glVertex2d (output->region ()->extents.x2,
			output->region ()->extents.y1);
	    glEnd ();
	    glDisable (GL_BLEND);
	    glColor4usv (defaultColor);
	}

	if (!init && ps->active)
	    ps->drawParticles ();

	glPopMatrix ();
    }

    return status;
}



void
FireScreen::donePaint ()
{
    if ( (!init && ps->active) || numPoints || brightness < 1.0)
    {
	cScreen->damageScreen ();
    }

    cScreen->donePaint ();
}

void
FireScreen::handleEvent (XEvent *event)
{
    switch (event->type)
    {

    case MotionNotify:
	fireAddPoint (pointerX, pointerY, true);
	break;

    case EnterNotify:
    case LeaveNotify:
	fireAddPoint (pointerX, pointerY, true);
    default:
	break;
    }

    screen->handleEvent (event);
}

FireScreen::FireScreen (CompScreen *screen) :
    PluginClassHandler <FireScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    ps (new ParticleSystem ()),
    init (true),
    points (NULL),
    pointsSize (0),
    numPoints (0),
    brightness (1.0),
    grabIndex (0)
{
    ScreenInterface::setHandler (screen);
    CompositeScreenInterface::setHandler (cScreen);
    GLScreenInterface::setHandler (gScreen);

    optionSetInitiateKeyInitiate (boost::bind (&FireScreen::initiate, this, _1,
						_2, _3));
    optionSetInitiateButtonInitiate (boost::bind (&FireScreen::initiate, this,
						   _1, _2, _3));
    optionSetInitiateKeyTerminate (boost::bind (&FireScreen::terminate, this,
						 _1, _2, _3));
    optionSetInitiateButtonTerminate (boost::bind (&FireScreen::terminate, this,
						   _1, _2, _3));

    optionSetClearKeyInitiate (boost::bind (&FireScreen::clear, this, _1, _2,
					    _3));
    optionSetClearButtonInitiate (boost::bind (&FireScreen::clear, this, _1, _2,
					       _3));

    optionSetAddParticleInitiate (boost::bind (&FireScreen::addParticle, this,
						_1, _2, _3));
}

FireScreen::~FireScreen ()
{
    if (!init)
	ps->finiParticles ();

    if (ps)
	delete ps;

    if (points)
	free (points);
}

bool
FirePluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	return false;

    return true;
}
