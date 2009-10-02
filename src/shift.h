/*
 *
 * Compiz shift switcher plugin
 *
 * shift.h
 *
 * Copyright : (C) 2007 by Dennis Kasprzyk
 * E-mail    : onestone@opencompositing.org
 *
 *
 * Based on ring.c:
 * Copyright : (C) 2007 by Danny Baumann
 * E-mail    : maniac@opencompositing.org
 *
 * Based on scale.c and switcher.c:
 * Copyright : (C) 2007 David Reveman
 * E-mail    : davidr@novell.com
 *
 * Rounded corner drawing taken from wall.c:
 * Copyright : (C) 2007 Robert Carr
 * E-mail    : racarr@beryl-project.org
 *
 * Ported to Compiz 0.9 by:
 * Copyright : (C) 2009 Sam Spilsbury
 * E-mail    : smspillaz@gmail.com
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
#include <core/atoms.h>
#include <composite/composite.h>
#include <opengl/opengl.h>
#include <text/text.h>

#include <cmath>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#include "shift_options.h"

extern bool textAvailable;

class ShiftSlot
{
    public:
	int   x, y;            /* thumb center coordinates */
	float z;
	float scale;           /* size scale (fit to maximal thumb size */
	float opacity;
	float rotation;

	GLfloat tx;
	GLfloat ty;

	Bool    primary;
};

class ShiftDrawSlot
{
    public:
	CompWindow *w;
	ShiftSlot  *slot;
	float      distance;
};

class ShiftScreen :
    public PluginClassHandler <ShiftScreen, CompScreen>,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public ShiftOptions
{
    public:
	typedef enum {
	    ShiftStateNone = 0,
	    ShiftStateOut,
	    ShiftStateSwitching,
	    ShiftStateFinish,
	    ShiftStateIn
	} ShiftState;

	typedef enum {
	    ShiftTypeNormal = 0,
	    ShiftTypeGroup,
	    ShiftTypeAll
	} ShiftType;
    public:
	ShiftScreen (CompScreen *);
	~ShiftScreen ();

	CompositeScreen *cScreen;
	GLScreen	*gScreen;

	CompText	text;

	KeyCode leftKey;
	KeyCode rightKey;
	KeyCode upKey;
	KeyCode downKey;

	CompScreen::GrabHandle grabIndex;

	ShiftState state;
	ShiftType  type;

	bool moreAdjust;
	bool moveAdjust;

	float mvTarget;
	float mvAdjust;
	GLfloat mvVelocity;
	bool	invert;

	Cursor  cursor;

	/* For sorting */

	/* only used for sorting */
	std::vector <CompWindow *> windows;
	std::vector <ShiftDrawSlot> drawSlots;

	ShiftDrawSlot *activeSlot;

	Window clientLeader;
	Window selectedWindow;

	CompMatch match;
	CompMatch currentMatch;

	CompOutput *output;
	int	   usedOutput;

	float	   anim;
	float      animVelocity;

	float      reflectBrightness;
	bool	   reflectActive;

	int	  buttonPressTime;
	Bool      buttonPressed;
	int       startX;
	int       startY;
	float     startTarget;
	float     lastTitle;

	Bool      paintingAbove;

	Bool      canceled;

	void
	handleEvent (XEvent *);

	void
	preparePaint (int);

	void
	paint (CompOutput::ptrList &outputs,
	       unsigned int);

	bool
	glPaintOutput (const GLScreenPaintAttrib &,
		       const GLMatrix		 &,
		       const CompRegion		 &,
		       CompOutput		 *,
		       unsigned int		   );

	void
	donePaint ();

	void
	activateEvent (bool       activating);

	void
	freeWindowTitle ();

	void
	renderWindowTitle ();

	void
	drawWindowTitle ();

	bool
	layoutThumbsCover ();

	bool
	layoutThumbsFlip ();

	bool
	layoutThumbs ();

	void
	addWindowToList (CompWindow *w);

	bool
	updateWindowList ();

	bool
	createWindowList ();

	void
	switchToWindow (bool toNext);

	int
	countWindows ();

	int
	adjustShiftMovement (float chunk);

	bool
	adjustShiftAnimationAttribs (float chunk);

	void
	term (bool cancel);

	bool
	terminate (CompAction         *action,
		   CompAction::State  aState,
		   CompOption::Vector options);

	bool
	initiateScreen (CompAction         *action,
			CompAction::State  aState,
			CompOption::Vector options);

	bool
	doSwitch (CompAction         *action,
		  CompAction::State  aState,
		  CompOption::Vector options,
		  bool		  nextWindow,
		  ShiftType	  type);

	bool
	initiate (CompAction         *action,
		  CompAction::State  aState,
		  CompOption::Vector options);

	bool
	initiateAll (CompAction         *action,
			  CompAction::State  aState,
			  CompOption::Vector options);

	void
	windowRemove (Window id);



};

#define SHIFT_SCREEN(s)							       \
	ShiftScreen *ss = ShiftScreen::get (s)

class ShiftWindow :
    public PluginClassHandler <ShiftWindow, CompWindow>,
    public CompositeWindowInterface,
    public GLWindowInterface
{
    public:
	ShiftWindow (CompWindow *);
	~ShiftWindow ();

	CompWindow	*window;
	CompositeWindow *cWindow;
	GLWindow	*gWindow;

	float opacity;
	float brightness;
	float opacityVelocity;
	float brightnessVelocity;

	ShiftSlot slots[2];

	bool  active;

	bool
	glPaint (const GLWindowPaintAttrib &,
		 const GLMatrix		   &,
		 const CompRegion	   &,
		 unsigned int		     );

	bool
	damageRect (bool,
		    const CompRect &);

	static bool
	compareWindows (CompWindow *w1,
			CompWindow *w2);

	static bool
	compareShiftWindowDistance (ShiftDrawSlot elem1,
				    ShiftDrawSlot elem2);

	bool
	adjustShiftWindowAttribs (float chunk);

	bool
	canStackRelativeTo ();

	bool
	is ();
};

#define SHIFT_WINDOW(w)						       \
	ShiftWindow *sw = ShiftWindow::get (w);

#define PI 3.1415926

class ShiftPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <ShiftScreen, ShiftWindow>
{
    public:

	bool init ();
};
