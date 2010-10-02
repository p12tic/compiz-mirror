/**
 *
 * Compiz group plugin
 *
 * group.h
 *
 * Copyright : (C) 2006-2007 by Patrick Niklaus, Roi Cohen, Danny Baumann
 * Authors: Patrick Niklaus <patrick.niklaus@googlemail.com>
 *          Roi Cohen       <roico.beryl@gmail.com>
 *          Danny Baumann   <maniac@opencompositing.org>
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
 **/

#ifndef _GROUP_H
#define _GROUP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <X11/Xlib.h>
#include <cairo/cairo-xlib-xrender.h>
#include <core/core.h>
#include <core/atoms.h>
#include <composite/composite.h>
#include <opengl/opengl.h>
#include <mousepoll/mousepoll.h>
#include <text/text.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include <math.h>
#include <limits.h>

class GroupSelection;
class GroupTabBarSlot;
class GroupTabBar;

#include "layers.h"
#include "group_options.h"


/* General TODO:
 * 1) Use std::list/vector etc (done)
 * 2) Use s/XRectangle/CompRect/ (done)
 * 3) Use s/Region/CompRegion/ (done)
 * 5) Derive SelectionObject from CompRect and CompWindowList
 * 6) Make GroupObject from CompWindowList, move all related methods into there
 * 7) Make Queues their own object
 * 8) Move tabbar stuff into it's own object, and move related methods
 * 9) Split rendering into layers
 * 10) Use windowNotify
 * 11) TabList class
 * 12) Rename from groupFoo to just foo
 * 13) Optimize (use const references where appropriate etc)
 */

/*
 * Constants
 *
 */
#define PI 3.1415926535897

/*
 * Helpers
 *
 */

#define WIN_X(w) (w->x ())
#define WIN_Y(w) (w->y ())
#define WIN_WIDTH(w) (w->width ())
#define WIN_HEIGHT(w) (w->height ())

#define WIN_CENTER_X(w) (WIN_X (w) + (WIN_WIDTH (w) / 2))
#define WIN_CENTER_Y(w) (WIN_Y (w) + (WIN_HEIGHT (w) / 2))

/* definitions used for glow painting */
#define WIN_REAL_X(w) (w->x () - w->input ().left)
#define WIN_REAL_Y(w) (w->y () - w->input ().top)
#define WIN_REAL_WIDTH(w) (w->width () + 2 * w->geometry ().border () + \
			   w->input ().left + w->input ().right)
#define WIN_REAL_HEIGHT(w) (w->height () + 2 * w->geometry ().border () + \
			    w->input ().top + w->input ().bottom)

#define TOP_TAB(g) ((g)->mTabBar->mTopTab->mWindow)
#define PREV_TOP_TAB(g) ((g)->mTabBar->mPrevTopTab->mWindow)
#define NEXT_TOP_TAB(g) ((g)->mTabBar->mNextTopTab->mWindow)

#define HAS_TOP_WIN(group) (((group)->mTabBar->mTopTab) && ((group)->mTabBar->mTopTab->mWindow))
#define HAS_PREV_TOP_WIN(group) (((group)->mTabBar->mPrevTopTab) && \
				 ((group)->mTabBar->mPrevTopTab->mWindow))

#define IS_TOP_TAB(w, group) (HAS_TOP_WIN (group) && \
			      ((TOP_TAB (group)->id ()) == (w)->id ()))
#define IS_PREV_TOP_TAB(w, group) (HAS_PREV_TOP_WIN (group) && \
				   ((PREV_TOP_TAB (group)->id ()) == (w)->id ()))

typedef struct _GlowTextureProperties {
    char *textureData;
    int  textureSize;
    int  glowOffset;
} GlowTextureProperties;

/*
 * Structs for pending callbacks
 */
typedef struct _GroupPendingMoves GroupPendingMoves;
struct _GroupPendingMoves {
    CompWindow        *w;
    int               dx;
    int               dy;
    bool              immediate;
    bool              sync;
    GroupPendingMoves *next;
};

typedef struct _GroupPendingGrabs GroupPendingGrabs;
struct _GroupPendingGrabs {
    CompWindow        *w;
    int               x;
    int               y;
    unsigned int      state;
    unsigned int      mask;
    GroupPendingGrabs *next;
};

typedef struct _GroupPendingUngrabs GroupPendingUngrabs;
struct _GroupPendingUngrabs {
    CompWindow          *w;
    GroupPendingUngrabs *next;
};

typedef struct _GroupPendingSyncs GroupPendingSyncs;
struct _GroupPendingSyncs {
    CompWindow        *w;
    GroupPendingSyncs *next;
};

/*
 * PaintState
 */

/* Mask values for groupTabSetVisibility */
#define SHOW_BAR_INSTANTLY_MASK (1 << 0)
#define PERMANENT		(1 << 1)

/* Mask values for tabbing animation */
#define IS_ANIMATED		(1 << 0)
#define FINISHED_ANIMATION	(1 << 1)
#define CONSTRAINED_X		(1 << 2)
#define CONSTRAINED_Y		(1 << 3)
#define DONT_CONSTRAIN		(1 << 4)
#define IS_UNGROUPING           (1 << 5)

/*
 * GroupTabBarSlot
 */
class GroupTabBarSlot :
    public GLLayer
{
public:
    class List :
        public std::list <GroupTabBarSlot *>,
	public GLLayer
    {
	public:
	    List (const CompSize &size, GroupSelection *g) :
	        GLLayer::GLLayer (size, g) {};
	
	    void paint (const GLWindowPaintAttrib &attrib,
		        const GLMatrix	          &transform,
			const CompRegion	  &region,
		        const CompRegion	  &clipRegion,
			int			  mask);
    };
public:

    void getDrawOffset (int &hoffset,
			int &voffset);

    void setTargetOpacity (int);
    
    void paint (const GLWindowPaintAttrib &sa,
		const GLMatrix	          &transform,
		const CompRegion	  &paintRegion,
		const CompRegion	  &clipRegion,
		int 			  mask);

public:
    GroupTabBarSlot *mPrev;
    GroupTabBarSlot *mNext;

    CompRegion mRegion;

    CompWindow *mWindow;
    GroupTabBar *mTabBar;

    /* For DnD animations */
    int	  mSpringX;
    int	  mSpeed;
    float mMsSinceLastMove;
    int   mOpacity;
private:

    GroupTabBarSlot (CompWindow *, GroupTabBar *);
    
    friend class GroupTabBar;
};

/*
 * GroupGlow
 */

class GlowQuad {
    public:
	CompRect	  mBox;
	GLTexture::Matrix mMatrix;
};

#define GLOWQUAD_TOPLEFT	 0
#define GLOWQUAD_TOPRIGHT	 1
#define GLOWQUAD_BOTTOMLEFT	 2
#define GLOWQUAD_BOTTOMRIGHT     3
#define GLOWQUAD_TOP		 4
#define GLOWQUAD_BOTTOM		 5
#define GLOWQUAD_LEFT		 6
#define GLOWQUAD_RIGHT		 7
#define NUM_GLOWQUADS		 8

/*
 * Selection
 */

class Selection :
    public CompWindowList
{
public:
    Selection () :
	mPainted (false),
	mVpX (0),
	mVpY (0),
	mX1 (0),
	mY1 (0),
	mX2 (0),
	mY2 (0) {};

    void checkWindow (CompWindow *w);
    void deselect (CompWindow *w);
    void deselect (GroupSelection *group);
    void select (CompWindow *w);
    void select (GroupSelection *g);
    void toGroup ();
    void damage (int, int);
    void paint (const GLScreenPaintAttrib sa,
		const GLMatrix		  transform,
		CompOutput		  *output,
		bool			  transformed);

    /* For selection */
    bool mPainted;
    int  mVpX, mVpY;
    int  mX1, mY1, mX2, mY2;
};

typedef struct _GroupResizeInfo {
    CompWindow *mResizedWindow;
    CompRect    mOrigGeometry;
} GroupResizeInfo;

/*
 * GroupTabBar
 */
class GroupTabBar
{
    public:

	typedef enum {
	    NoTabChange = 0,
	    TabChangeOldOut,
	    TabChangeNewIn
	} TabChangeState;

	/*
	 * Rotation direction for change tab animation
	 */
	typedef enum {
	    RotateUncertain = 0,
	    RotateLeft,
	    RotateRight
	} ChangeAnimationDirection;

    public:

	GroupTabBar (GroupSelection *, CompWindow *);
	~GroupTabBar ();

    public:

	/* Input Prevention */

	void createInputPreventionWindow ();
	void destroyInputPreventionWindow ();

	/* Drawing */

	void paint (const GLWindowPaintAttrib &attrib,
		    const GLMatrix		 &transform,
		    unsigned int		 mask,		
		    CompRegion		 clipRegion);

	void damageRegion ();

	/* Animation */

	void handleTabBarFade (int msSinceLastPaint);
	void handleTextFade (int msSinceLastPaint);
	void handleTabBarAnimation (int msSinceLastPaint);

	/* Region and position management */

	void moveTabBarRegion (int		   dx,
			       int		   dy,
			       bool		   syncIPW);

	void resizeTabBarRegion (CompRect       &box,
			         bool           syncIPW);

	void recalcTabBarPos (int		  middleX,
			      int		  minX1,
			      int		  maxX2);

	/* Slot management */
	void insertTabBarSlotBefore (GroupTabBarSlot *slot,
				     GroupTabBarSlot *nextSlot);

	void insertTabBarSlotAfter (GroupTabBarSlot *slot,
				    GroupTabBarSlot *prevSlot);

	void insertTabBarSlot (GroupTabBarSlot *slot);

	void unhookTabBarSlot (GroupTabBarSlot *slot,
			       bool            temporary);

	void deleteTabBarSlot (GroupTabBarSlot *slot);

	void createSlot (CompWindow      *w);

	void applyForces (GroupTabBarSlot *);

	void applySpeeds (int            msSinceLastRepaint);


    public:
	GroupTabBarSlot::List mSlots;

	GroupSelection  *mGroup;

	GroupTabBarSlot* mTopTab;
	GroupTabBarSlot* mPrevTopTab;

	/* needed for untabbing animation */
	CompWindow *mLastTopTab;

	/* Those two are only for the change-tab animation,
	when the tab was changed again during animation.
	Another animation should be started again,
	switching for this window. */
	GroupTabBar::ChangeAnimationDirection mNextDirection;
	GroupTabBarSlot             *mNextTopTab;

	/* check focus stealing prevention after changing tabs */
	bool mCheckFocusAfterTabChange;

	int            mChangeAnimationTime;
	int            mChangeAnimationDirection;
	GroupTabBar::TabChangeState mChangeState;

	GroupTabBarSlot *mHoveredSlot;
	GroupTabBarSlot *mTextSlot;

	TextLayer *mTextLayer;
	BackgroundLayer *mBgLayer;
	SelectionLayer *mSelectionLayer;

	/* For animations */
	int                mBgAnimationTime;
	BackgroundLayer::AnimationType mBgAnimation;

	PaintState mState;
	int        mAnimationTime;
	CompRegion mRegion;
	int        mOldWidth;

	CompTimer mTimeoutHandle;

	/* For DnD animations */
	int   mLeftSpringX, mRightSpringX;
	int   mLeftSpeed, mRightSpeed;
	float mLeftMsSinceLastMove, mRightMsSinceLastMove;

	Window mInputPrevention;
	bool   mIpwMapped;
};

/*
 * GroupSelection
 */
class GroupSelection
{
    public:
	/*
	 * Ungrouping states
	 */
	typedef enum {
	    UngroupNone = 0,
	    UngroupAll,
	    UngroupSingle
	} UngroupState;
	
	typedef enum {
	    NoTabbing = 0,
	    Tabbing,
	    Untabbing
	} TabbingState;

    public:

	typedef std::list <GroupSelection *> List;

	GroupSelection (CompWindow *, long int);
	~GroupSelection ();

    public:

	void tabGroup (CompWindow *main);
	void untabGroup ();

	void raiseWindows (CompWindow *top);
	void minimizeWindows (CompWindow *top,
			  bool	     minimize);
	void shadeWindows (CompWindow  *top,
		       bool	   shade);
	void moveWindows (CompWindow *top,
			  int 	 dx,
			  int 	 dy,
			  bool 	 immediate,
			  bool	 viewportChange = false);
	void prepareResizeWindows (CompRect &resizeRect);
	void resizeWindows (CompWindow *top);
	void maximizeWindows (CompWindow *top);

	void
	applyConstraining (CompRegion  constrainRegion,
			   Window	   constrainedWindow,
			   int	   dx,
			   int	   dy);
		       
	bool tabBarTimeout ();
	bool showDelayTimeout ();

	void
	tabSetVisibility (bool           visible,
			  unsigned int   mask);

	void
	handleHoverDetection (const CompPoint &);

	void
	handleAnimation ();

	void
	finishTabbing ();

	void
	drawTabAnimation (int	      msSinceLastPaint);

	void
	startTabbingAnimation (bool           tab);

	void fini ();

public:
    CompScreen *mScreen;
    CompWindowList mWindows;
    
    MousePoller	mPoller;

    /* Unique identifier for this group */
    long int mIdentifier;

    GroupTabBar *mTabBar;

    GroupSelection::TabbingState mTabbingState;

    UngroupState mUngroupState;

    Window       mGrabWindow;
    unsigned int mGrabMask;

    GLushort mColor[4];
    
    GroupResizeInfo *mResizeInfo;
};

/*
 * GroupScreen structure
 */
class GroupScreen :
    public PluginClassHandler <GroupScreen, CompScreen>,
    public GroupOptions,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface
{
    public:

	/*
	 * Screengrab states
	 */
	typedef enum {
	    ScreenGrabNone = 0,
	    ScreenGrabSelect,
	    ScreenGrabTabDrag
	} GrabState;

    public:

	GroupScreen (CompScreen *);
	~GroupScreen ();

    public:

	CompositeScreen *cScreen;
	GLScreen	*gScreen;

    public:

	void
	handleEvent (XEvent *);

	void
	preparePaint (int);

	void
	donePaint ();

	bool
	glPaintOutput (const GLScreenPaintAttrib &attrib,
		       const GLMatrix	      &transform,
		       const CompRegion	      &region,
		       CompOutput	      *output,
		       unsigned int	      mask);

	void
	glPaintTransformedOutput (const GLScreenPaintAttrib &,
				  const GLMatrix	    &,
				  const CompRegion	    &,
				  CompOutput		    *,
				  unsigned int		      );

	

    public:

	void
	optionChanged (CompOption *opt,
		       Options    num);

	bool
	applyInitialActions ();

	/* cairo.c */

	void
	damagePaintRectangle (const CompRect &box);

	/* queues.c */

	void
	dequeueSyncs (GroupPendingSyncs *syncs);

	void
	dequeueMoveNotifies ();

	void
	dequeueGrabNotifies ();

	void
	dequeueUngrabNotifies ();

	bool
	dequeueTimer ();

	/* selection.c */

	bool
	selectSingle (CompAction         *action,
			CompAction::State  state,
			CompOption::Vector options);
	bool
	select (CompAction         *action,
		  CompAction::State  state,
		  CompOption::Vector options);

	bool
	selectTerminate (CompAction         *action,
			   CompAction::State  state,
			   CompOption::Vector options);

	/* group.c */

	void
	grabScreen (GroupScreen::GrabState newState);

	bool
	groupWindows (CompAction         *action,
			CompAction::State  state,
			CompOption::Vector options);

	bool
	ungroupWindows (CompAction          *action,
			  CompAction::State   state,
			  CompOption::Vector  options);

	bool
	removeWindow (CompAction         *action,
			CompAction::State  state,
			CompOption::Vector options);

	bool
	closeWindows (CompAction           *action,
			CompAction::State    state,
			CompOption::Vector   options);

	bool
	changeColor (CompAction           *action,
		       CompAction::State    state,
		       CompOption::Vector   options);

	bool
	setIgnore (CompAction         *action,
		     CompAction::State  state,
		     CompOption::Vector options);

	bool
	unsetIgnore (CompAction          *action,
		       CompAction::State   state,
		       CompOption::Vector  options);

	void
	handleButtonPressEvent (XEvent *event);

	void
	handleButtonReleaseEvent (XEvent *event);

	void
	handleMotionEvent (int xRoot,
			     int yRoot);

	/* tab.c */

	bool
	getCurrentMousePosition (int &x, int &y);

	void
	tabChangeActivateEvent (bool activating);

	void
	updateTabBars (Window enteredWin);

	CompRegion
	getConstrainRegion ();

	bool
	changeTab (GroupTabBarSlot             *topTab,
		        GroupTabBar::ChangeAnimationDirection direction);

	void
	recalcSlotPos (GroupTabBarSlot *slot,
		       int		 slotPos);

	bool
	initTab (CompAction         *aciton,
		 CompAction::State  state,
		 CompOption::Vector options);

	bool
	changeTabLeft (CompAction          *action,
		       CompAction::State   state,
		       CompOption::Vector  options);
	bool
	changeTabRight (CompAction         *action,
			CompAction::State  state,
			CompOption::Vector options);

	void
	switchTopTabInput (GroupSelection *group,
			   bool		  enable);

    public:

	bool		mIgnoreMode;
	GlowTextureProperties *mGlowTextureProperties;
	GroupSelection	*mLastRestackedGroup;
	Atom		mGroupWinPropertyAtom;
	Atom		mResizeNotifyAtom;

	CompText	mText;

	GroupPendingMoves   *mPendingMoves;
	GroupPendingGrabs   *mPendingGrabs;
	GroupPendingUngrabs *mPendingUngrabs;
	CompTimer	    mDequeueTimeoutHandle;

	GroupSelection::List mGroups;
	Selection	     mTmpSel;

	bool mQueued;

	GroupScreen::GrabState   mGrabState;
	CompScreen::GrabHandle mGrabIndex;

	GroupSelection *mLastHoveredGroup;

	CompTimer       mShowDelayTimeoutHandle;

	/* For d&d */
	GroupTabBarSlot   *mDraggedSlot;
	CompTimer	  mDragHoverTimeoutHandle;
	bool              mDragged;
	int               mPrevX, mPrevY; /* Buffer for mouse coordinates */

	CompTimer	  mInitialActionsTimeoutHandle;

	GLTexture::List   mGlowTexture;
	
	Window		  mLastGrabbedWindow;
};

extern bool textAvailable;

#define GROUP_SCREEN(s)							       \
    GroupScreen *gs = GroupScreen::get (s);

/*
 * GroupWindow structure
 */
class GroupWindow :
    public PluginClassHandler <GroupWindow, CompWindow>,
    public WindowInterface,
    public CompositeWindowInterface,
    public GLWindowInterface
{
    public:

	/*
	 * Window states
	 */
	typedef enum {
	    WindowNormal = 0,
	    WindowMinimized,
	    WindowShaded
	} State;

	class HideInfo {
	    public:
		Window mShapeWindow;

		unsigned long mSkipState;
		unsigned long mShapeMask;

		XRectangle *mInputRects;
		int        mNInputRects;
		int        mInputRectOrdering;
	};

    public:

	GroupWindow (CompWindow *);
	~GroupWindow ();

    public:

	CompWindow *window;
	CompositeWindow *cWindow;
	GLWindow   *gWindow;

    public:

	void
	moveNotify (int, int, bool);

	void
	resizeNotify (int, int, int, int);

	void
	grabNotify (int, int,
		    unsigned int,
		    unsigned int);

	void
	ungrabNotify ();

	void
	windowNotify (CompWindowNotify n);

	void
	stateChangeNotify (unsigned int);


	void
	activate ();

	void
	getOutputExtents (CompWindowExtents &);

	bool
	glDraw (const GLMatrix &,
		GLFragment::Attrib &,
		const CompRegion	&,
		unsigned int); 

	bool
	glPaint (const GLWindowPaintAttrib &,
		 const GLMatrix		   &,
		 const CompRegion	   &,
		 unsigned int		     );

	bool
	damageRect (bool,
		    const CompRect &);

    public:

	/* paint.c */

	void
	computeGlowQuads (GLTexture::Matrix *matrix);

	void
	getStretchRectangle (CompRect &box,
			     float  &xScaleRet,
			     float  &yScaleRet);

	/* queues.c */

	void
	enqueueMoveNotify (int  dx,
			   int  dy,
			   bool immediate,
			   bool sync);

	void
	enqueueGrabNotify (int          x,
			   int          y,
			   unsigned int state,
			   unsigned int mask);

	void
	enqueueUngrabNotify ();

	/* selection.c */

	bool
	windowInRegion (CompRegion src,
			  float  precision);

	/* group.c */

	bool
	isGroupWindow ();

	bool
	dragHoverTimeout ();

	bool
	checkWindowProperty (CompWindow *w,
			       long int   *id,
			       bool       *tabbed,
			       GLushort   *color);

	void
	updateWindowProperty ();

	unsigned int
	updateResizeRectangle (CompRect	    masterGeometry,
				    bool	    damage);

	void
	deleteGroupWindow ();

	void
	removeWindowFromGroup ();

	void
	addWindowToGroup (GroupSelection *group,
			    long int       initialIdent);

	/* tab.cpp */

	CompRegion
	getClippingRegion ();

	void
	clearWindowInputShape (GroupWindow::HideInfo *hideInfo);

	void
	setWindowVisibility (bool visible);

	int
	adjustTabVelocity ();

	bool
	constrainMovement (CompRegion constrainRegion,
			   int        dx,
			   int        dy,
			   int        &new_dx,
			   int        &new_dy);



    public:

	GroupSelection *mGroup;
	bool mInSelection;

	/* To prevent freeing the group
	property in groupFiniWindow. */
	bool mReadOnlyProperty;

	/* For the tab bar */
	GroupTabBarSlot *mSlot;

	bool mNeedsPosSync;

	GlowQuad *mGlowQuads;

	GroupWindow::State    mWindowState;
	GroupWindow::HideInfo   *mWindowHideInfo;

	CompRect	    mResizeGeometry;

	/* For tab animation */
	int       mAnimateState;
	CompPoint mMainTabOffset;
	CompPoint mDestination;
	CompPoint mOrgPos;

	float mTx,mTy;
	float mXVelocity, mYVelocity;
};

#define GROUP_WINDOW(w)							       \
    GroupWindow *gw = GroupWindow::get (w);

class GroupPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <GroupScreen, GroupWindow>
{
    public:

	bool init ();
};

/*
 * Pre-Definitions
 *
 */
#endif
