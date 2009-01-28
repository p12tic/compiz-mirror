/**
 *
 * Compiz fade to desktop plugin
 *
 * fadedesktop.c
 *
 * Copyright (c) 2006 Robert Carr <racarr@beryl-project.org>
 * 		 2007 Danny Baumann <maniac@beryl-project.org>
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
 **/

#include <core/core.h>
#include <core/privatehandler.h>

#include <composite/composite.h>
#include <opengl/opengl.h>

#include "fadedesktop_options.h"

class FadedesktopScreen :
    public PrivateHandler <FadedesktopScreen, CompScreen>,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public FadedesktopOptions
{
    public:

	typedef enum
	{
	    FD_STATE_OFF = 0,
	    FD_STATE_OUT,
	    FD_STATE_ON,
	    FD_STATE_IN
	} FadeDesktopState;
    public:

	FadedesktopScreen (CompScreen *);

	void preparePaint (int);
	void donePaint ();

	void enterShowDesktopMode ();
	void leaveShowDesktopMode (CompWindow *w);

	CompositeScreen *cScreen;
	GLScreen	*gScreen;

	FadeDesktopState state;
	int		  fadeTime;
};

class FadedesktopWindow :
    public PrivateHandler <FadedesktopWindow, CompWindow>,
    public WindowInterface,
    public GLWindowInterface
{
    public:

	FadedesktopWindow (CompWindow *);

	bool glPaint (const GLWindowPaintAttrib &,
		      const GLMatrix &,
		      const CompRegion &,
		      unsigned int);

	CompWindow *window;
	CompositeWindow *cWindow;
	GLWindow   *gWindow;

	bool fading;
	bool isHidden;

	GLushort opacity;
};

#define FD_SCREEN(s)							       \
    FadedesktopScreen *fs = FadedesktopScreen::get (s);

#define FD_WINDOW(w)							       \
    FadedesktopWindow *fw = FadedesktopWindow::get (w);

class FadedesktopPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <FadedesktopScreen, FadedesktopWindow>
{
    public:

	bool init ();

	PLUGIN_OPTION_HELPER (FadedesktopScreen);
};
