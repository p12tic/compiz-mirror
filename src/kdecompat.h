/*
 *
 * Compiz KDE compatibility plugin
 *
 * kdecompat.cpp
 *
 * Copyright : (C) 2007 by Danny Baumann
 * E-mail    : maniac@opencompositing.org
 *
 * Based on scale.c and switcher.c:
 * Copyright : (C) 2007 David Reveman
 * E-mail    : davidr@novell.com
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
 
#include <core/core.h>
#include <core/pluginclasshandler.h>
#include <composite/composite.h>
#include <opengl/opengl.h>

#include <X11/Xatom.h>
#include <core/atoms.h>

#include "kdecompat_options.h"

#include <cmath>
 
class KDECompatScreen :
    public PluginClassHandler <KDECompatScreen, CompScreen>,
    public ScreenInterface,
    public KdecompatOptions
{
    public:

	KDECompatScreen (CompScreen *);
	~KDECompatScreen ();

    public:

	void
	handleEvent (XEvent *);
	
	void
	advertiseThumbSupport (bool supportThumbs);
	
	void
	optionChanged (CompOption                *option,
		       KdecompatOptions::Options num);
	
	CompositeScreen *cScreen;
	GLScreen	*gScreen;

	Atom mKdePreviewAtom;
};

#define KDECOMPAT_SCREEN(s)						       \
    KDECompatScreen *ks = KDECompatScreen::get (s)

class KDECompatWindow :
    public PluginClassHandler <KDECompatWindow, CompWindow>,
    public CompositeWindowInterface,
    public GLWindowInterface
{
    public:

	KDECompatWindow (CompWindow *);
	~KDECompatWindow ();
	
	CompWindow      *window;
	CompositeWindow *cWindow;
	GLWindow	*gWindow;

    public:

	typedef struct {
	    Window   id;
	    CompRect thumb;
	} Thumb;

	std::list <Thumb> mPreviews;
	bool		  mIsPreview;
	
    public:
    
	bool
	glPaint (const GLWindowPaintAttrib &,
		 const GLMatrix		   &,
		 const CompRegion	   &,
		 unsigned int);
		 
	bool
	damageRect (bool,
		    const CompRect &);
		    
	void
	updatePreviews ();
};

#define KDECOMPAT_WINDOW(w)						       \
    KDECompatWindow *kw = KDECompatWindow::get(w)

class KDECompatPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <KDECompatScreen,
						 KDECompatWindow>
{
    public:

	bool
	init ();
};
