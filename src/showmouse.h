/*
 *
 * Compiz show mouse pointer plugin
 *
 * showmouse.c
 *
 * Copyright : (C) 2008 by Dennis Kasprzyk
 * E-mail    : onestone@opencompositing.org
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

#include <cmath>

#include <core/core.h>
#include <composite/composite.h>
#include <opengl/opengl.h>
#include <mousepoll/mousepoll.h>
#include <particlesystem/particlesystem.h>

#include "showmouse_options.h"
#include "showmouse_tex.h"

class ShowmouseScreen :
    public PluginClassHandler <ShowmouseScreen, CompScreen>,
    public ShowmouseOptions,
    public CompositeScreenInterface,
    public GLScreenInterface
{
    public:

	ShowmouseScreen (CompScreen *);
	~ShowmouseScreen ();

	CompositeScreen *cScreen;
	GLScreen	*gScreen;

	CompPoint mousePos;

	bool	       active;

	ParticleSystem *ps;

	float	       rot;

	MousePoller    pollHandle;

	void
	preparePaint (int);

	bool
	glPaintOutput (const GLScreenPaintAttrib &,
		       const GLMatrix		 &,
		       const CompRegion		 &,
		       CompOutput		 *,
		       unsigned int		 );

	void
	donePaint ();

	void
	genNewParticles (int);

	void
	damageRegion ();

	void
	positionUpdate (const CompPoint &p);

	bool
	terminate (CompAction         *action,
		   CompAction::State  state,
		   CompOption::Vector options);

	bool
	initiate (CompAction         *action,
		  CompAction::State  state,
		  CompOption::Vector options);

};

class ShowmousePluginVTable :
    public CompPlugin::VTableForScreen <ShowmouseScreen>
{
    bool init ();
};
