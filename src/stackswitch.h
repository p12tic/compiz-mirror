/*
 *
 * Compiz stackswitch switcher plugin
 *
 * stackswitch.h
 *
 * Copyright : (C) 2008 by Dennis Kasprzyk
 * E-mail    : onestone@beryl-project.org
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

#include <cmath>
#include <core/atoms.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#include <core/core.h>
#include <core/pluginclasshandler.h>
#include <opengl/opengl.h>
#include <text/text.h>
#include <composite/composite.h>

#include "stackswitch_options.h"

typedef struct _StackswitchSlot {
    int   x, y;            /* thumb center coordinates */
    float scale;           /* size scale (fit to maximal thumb size) */
} StackswitchSlot;

typedef struct _StackswitchDrawSlot {
    CompWindow      *w;
    StackswitchSlot **slot;
} StackswitchDrawSlot;

class StackswitchScreen :
    public PluginClassHandler <StackswitchScreen, CompScreen>,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public StackswitchOptions
{
    public:
    
	StackswitchScreen (CompScreen *);
	~StackswitchScreen ();
	
    public:
    
	typedef enum {
	    StateNone = 0,
	    StateOut,
	    StateSwitching,
	    StateIn
	} StackswitchState;

	typedef enum {
	    TypeNormal = 0,
	    TypeGroup,
	    TypeAll
	} StackswitchType;
	
    public:

	CompositeScreen *cScreen;
	GLScreen	*gScreen;
	
	/* text display support */
	CompText	mText;

	void
	handleEvent (XEvent *);
	
	void
	preparePaint (int);
	
	bool
	glPaintOutput (const GLScreenPaintAttrib &,
		       const GLMatrix            &,
		       const CompRegion		 &,
		       CompOutput		 *,
		       unsigned int		   );
		       
	void
	donePaint ();

	void
	renderWindowTitle ();

	void
	drawWindowTitle (GLMatrix &transform,
					    CompWindow *w);
					    
	bool
	layoutThumbs ();

	bool
	updateWindowList ();

	bool
	createWindowList ();

	void
	switchToWindow (bool toNext);

	int
	countWindows ();

	int
	adjustStackswitchRotation (float chunk);

	bool
	terminate (CompAction *action,
		   CompAction::State state,
		   CompOption::Vector options);
				      
	bool
	initiate (CompAction         *action,
		  CompAction::State  state,
		   CompOption::Vector options);
				     
	bool
	doSwitch (CompAction         *action,
		  CompAction::State  state,
		  CompOption::Vector options,
		  bool		nextWindow,
		  StackswitchType type);

	void
	windowRemove (Window id);
	
	CompScreen::GrabHandle mGrabIndex;
	
	StackswitchState mState;
	StackswitchType  mType;
	bool		 mMoreAdjust;
	bool		 mRotateAdjust;
	
	bool		 mPaintingSwitcher;
	
	GLfloat		 mRVelocity;
	GLfloat		 mRotation;
	
	/* only used for sorting */
	std::vector <CompWindow *>   mWindows;
	std::vector <StackswitchDrawSlot *> mDrawSlots;
	
	Window		 mClientLeader;
	Window		 mSelectedWindow;
	
	CompMatch	 mMatch;
	CompMatch	 mCurrentMatch;
	
};

#define STACKSWITCH_SCREEN(s)						       \
    StackswitchScreen *ss = StackswitchScreen::get (s)
	
class StackswitchWindow :
    public PluginClassHandler <StackswitchWindow, CompWindow>,
    public CompositeWindowInterface,
    public GLWindowInterface
{
    public:
    
	StackswitchWindow (CompWindow *w);
	~StackswitchWindow ();
	
    public:

	CompWindow	*window;
	CompositeWindow *cWindow;
	GLWindow	*gWindow;
	
	bool
	damageRect (bool, const CompRect &);
	
	bool
	glPaint (const GLWindowPaintAttrib &,
		 const GLMatrix		   &,
		 const CompRegion	   &,
		 unsigned int		     );
		 
	int
	adjustStackswitchVelocity ();

	void
	addToList ();

	static bool
	compareStackswitchWindowDepth (StackswitchDrawSlot *a1,
				       StackswitchDrawSlot *a2);
							  
	static bool
	compareWindows (CompWindow *w1,
					   CompWindow *w2);
					   
	bool
	isStackswitchable ();

	StackswitchSlot *mSlot;
	
	GLfloat 	mXVelocity;
	GLfloat		mYVelocity;
	GLfloat		mScaleVelocity;
	GLfloat		mRotVelocity;
	
	GLfloat		mTx;
	GLfloat		mTy;
	GLfloat		mScale;
	GLfloat		mRotation;
	bool		mAdjust;
};

#define STACKSWITCH_WINDOW(w)						       \
    StackswitchWindow *sw = StackswitchWindow::get (w)

class StackswitchPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <StackswitchScreen, StackswitchWindow>
{
    public:

	bool init ();
};
