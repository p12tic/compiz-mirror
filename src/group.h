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
 *	    Sam Spilsbury   <smspillaz@gmail.com>
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

#include <cstring>
#include <cmath>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <cairo/cairo-xlib-xrender.h>

#include <core/core.h>
#include <core/atoms.h>
#include <composite/composite.h>
#include <opengl/opengl.h>
#include <text/text.h>
#include <mousepoll/mousepoll.h>

#include <X11/extensions/shape.h>

#include <limits.h>

#include "group_options.h"

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

#define TOP_TAB(g) ((g)->topTab->window)
#define PREV_TOP_TAB(g) ((g)->prevTopTab->window)
#define NEXT_TOP_TAB(g) ((g)->nextTopTab->window)

#define HAS_TOP_WIN(group) (((group)->topTab) && ((group)->topTab->window))
#define HAS_PREV_TOP_WIN(group) (((group)->prevTopTab) && \
				 ((group)->prevTopTab->window))

#define IS_TOP_TAB(w, group) (HAS_TOP_WIN (group) && \
			      ((TOP_TAB (group)->id ()) == (w)->id ()))
#define IS_PREV_TOP_TAB(w, group) (HAS_PREV_TOP_WIN (group) && \
				   ((PREV_TOP_TAB (group)->id ()) == (w)->id ()))

class Tab;
class TabBar;
class Group;

/*
 * Structs
 *
 */

class Layer :
    public CompRect
{
    public:
	typedef enum _PaintState {
	    PaintOff = 0,
	    PaintFadeIn = 1,
	    PaintFadeOut = 2,
	    PaintOn = 3,
	    PaintPermanentOn = 4
	} PaintState;
    public:
    
	Layer (int, int);
        ~Layer ();

	PaintState state;
	
	int animationTime;
	
	GLTexture::List texture;
	
	/* used if layer is used for text drawing */
	int texWidth;
	int texHeight;

/*

	virtual void draw (int, int, float);

    private:
	Layer ();
	~Layer ();
*/
};

class CairoHelper
{
    public :

	/* used if layer is used for cairo drawing */
	unsigned char   *buffer;
	cairo_surface_t *surface;
	cairo_t	    *cairo;
	Pixmap		pixmap;

    public:

	void
	clear ();

	~CairoHelper ();

   private:

    CairoHelper (int, int);
    
    friend class CairoLayer;

};

class CairoLayer :
    public Layer,
    public CairoHelper
{
    public:

	//void draw (); TODO
	//bool render (); TODO

	static CairoLayer *
	rebuildCairoLayer (CairoLayer *,
			   int             width,
			   int             height);

	static CairoLayer *
	createCairoLayer (int        width,
			  int        height);

    private:
	CairoLayer (int, int);
};

class TextLayer :
    public Layer
{
    public:

	TextLayer ();
	~TextLayer ();

	CompText text;
	Pixmap		pixmap;
#if 0
	void draw ();
	bool render ();
	void clear ();

#endif

   private:

};

extern bool textAvailable;

class Tab
{
    public:
    
    Tab (Group *, CompWindow *w);
    ~Tab ();
    
    void
    recalcPos (int slotPos);
    
    void
    getDrawOffset (int &hoffset,
		           int &voffset);
		           
	void
	paint (Group *,
		   const GLMatrix &,
		   int);

	TabBar *bar;

	CompRegion region;

	CompWindow *window;

	/* For DnD animations */
	int	  springX;
	int	  speed;
	float msSinceLastMove;
    private:
};

class TabList :
    public std::list <Tab *>
{
    public:
    
        bool
        getPrevTab (Tab *curr, Tab *&ret);
        
        bool
        getNextTab (Tab *cur, Tab *&ret);
        
        iterator
        getFirstTab () { return begin (); };
        
        reverse_iterator
        getLastTab () { return rend (); };
        
};

class TabBar
{
    public:
	typedef enum _GroupAnimationType {
	    AnimationNone = 0,
	    AnimationPulse,
	    AnimationReflex
	} GroupAnimationType;

	/*
	 * Rotation direction for change tab animation
	 */
	typedef enum _ChangeTabAnimationDirection {
	    RotateUncertain = 0,
	    RotateLeft,
	    RotateRight
	} ChangeTabAnimationDirection;

	typedef enum _TabChangeState {
	    NoTabChange = 0,
	    TabChangeOldOut,
	    TabChangeNewIn
	} TabChangeState;

	typedef enum _TabbingState {
	    NoTabbing = 0,
	    Tabbing,
	    Untabbing
	} TabbingState;

    public:

	TabBar (Group *, CompWindow *);
	~TabBar ();

	TabList	    tabs;

	Tab *hoveredSlot;
	Tab *textSlot;

	TextLayer  *textLayer;
	CairoLayer *bgLayer;
	CairoLayer *selectionLayer;

	Group	   *group;

	/* For animations */
	int                bgAnimationTime;
	GroupAnimationType bgAnimation;

	CairoLayer::PaintState state;
	int        animationTime;
	CompRegion     region;
	int        oldWidth;

	Window inputPrevention;
	bool   ipwMapped;

	CompTimer  timeoutHandle;

	/* For DnD animations */
	int   leftSpringX, rightSpringX;
	int   leftSpeed, rightSpeed;
	float leftMsSinceLastMove, rightMsSinceLastMove;
	
	void
	draw (const GLWindowPaintAttrib  &wAttrib,
	      const GLMatrix		 &transform,
	      unsigned int		 mask,
	      CompRegion		 clipRegion);

	void
	renderWindowTitle ();

	void
	renderTopTabHighlight ();

	void
	renderTabBarBackground ();

	void handleFade (int ms);

	void handleTextFade (int ms);

	void handleAnimation (int ms);

	void drawTabAnimation (int ms);

	void setVisibility (bool, unsigned int);

	void recalcPos (int, int, int);

	void insertTabAfter (Tab *tab, Tab *prev);

	void insertTabBefore (Tab *tab, Tab *next);

	void insertTab (Tab *tab);

	void unhookTab (Tab *tab, bool temporary);

	void deleteTab (Tab *tab);

	Tab *
	createTab (CompWindow *);

	void applyForces (Tab *draggedSlot);

	void applySpeeds (int ms);

	void startTabbingAnimation (bool);

		
	bool changeTab (Tab *topTab, ChangeTabAnimationDirection direction);

	void switchTopTabInput (bool enable);

	void createIPW ();
	void destroyIPW ();

	void moveRegion (int dx, int dy, bool syncIPW);

	void resizeRegion (CompRect box, bool syncIPW);

	void damageRegion ();

    private:
};

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

class Group
{
    public:
	/*
	 * Ungrouping states
	 */
	typedef enum _UngroupState {
	    UngroupNone = 0,
	    UngroupAll,
	    UngroupSingle
	} UngroupState;

    public:

	static Group *
	create (unsigned int);
	
	void
	destroy (bool);

	CompWindowList windows;

	/* Unique identifier for this group */
	long int identifier;

	Tab* topTab;
	Tab* prevTopTab;

/* Move this section to TabBar */

	/* needed for untabbing animation */
	CompWindow *lastTopTab;

	/* Those two are only for the change-tab animation,
	when the tab was changed again during animation.
	Another animation should be started again,
	switching for this window. */
	TabBar::ChangeTabAnimationDirection nextDirection;
	Tab		             *nextTopTab;

	/* check focus stealing prevention after changing tabs */
	bool checkFocusAfterTabChange;

/**/

	TabBar *tabBar;

	int            changeAnimationTime;
	int            changeAnimationDirection;
	TabBar::TabChangeState changeState;

	TabBar::TabbingState tabbingState;

	UngroupState ungroupState;

	Window       grabWindow;
	unsigned int grabMask;

	GLushort color[4];

	void
	addWindow (CompWindow *w);

	void
	minimizeWindows (CompWindow *top, bool);

	void
	shadeWindows (CompWindow *top, bool);

	void
	raiseWindows (CompWindow *top);



	void
	handleAnimation ();

	void
	handleHoverDetection ();

	void
	getDrawOffset (CompPoint &p);

	void tab (CompWindow *main);
	void untab ();
	
	void
	finishTabbing ();
	
	void
	drawTabAnimation (int            msSinceLastPaint);
	
	void
	applyConstraining (CompRegion         constrainRegion,
			   Window         constrainedWindow,
			   int            dx,
			   int            dy);
					   
	void
	startTabbingAnimation (Bool           tab);

    private:
    
	Group (unsigned int);
	~Group ();
};

class Selection :
    public CompWindowList
{
    public:
	Group *
	toGroup ();

	void
	push_back (Selection &);

	void
	push_back (CompWindow *w);
};

class SelectionRect :
   public CompRect
{
    public:

	void
	damageRect (CompScreen *s);

	Selection
	toSelection ();

	void
	paint (const GLScreenPaintAttrib &,
	       const GLMatrix &,
	       CompOutput *,
	       bool);

	void
	damage (int, int);

	
    private:
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

/* XXX: cleanup */

class GlowTexture
{
    public:
	class Properties {
	    public:

		Properties ();
		~Properties ();

		char *textureData;
		int  textureSize;
		int  glowOffset;
	};

	class Quads {
	    public:

		Quads ();
		~Quads ();

		CompRect box;
		GLTexture::Matrix matrix;
	};

    public:

	GLTexture::List	      texture;

	Properties properties[2];
};

class GroupScreen :
    public PluginClassHandler <GroupScreen, CompScreen>,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public GroupOptions
{
    public:

	typedef enum _GrabState {
	    ScreenGrabNone = 0,
	    ScreenGrabSelect,
	    ScreenGrabTabDrag
	} GrabState;

	typedef struct _GroupResizeInfo  {
	    CompWindow *resizedWindow;
	    CompRect   origGeometry;
	} GroupResizeInfo;

	/* These form queues:
	 * They are required because when moveNotify is called for some
	 * window, other plugins may be unwrapped (their function is not
	 * in the call chain), so we cannot call these functions directly
	 * with other windows because other plugins functions would not be
	 * called for those windows. Rather, we put them in a queue and set
	 * a zero-time timer such that they are called at the beginning
	 * of handleEvent (XEvent *) so that these functions are wrapped
	 * by all plugins again */

	typedef struct _PendingMoves {
	    CompWindow        *w;
	    int               dx;
	    int               dy;
	    Bool              immediate;
	    Bool              sync;
	} PendingMoves;

	typedef struct _PendingGrabs  {
	    CompWindow        *w;
	    int               x;
	    int               y;
	    unsigned int      state;
	    unsigned int      mask;
	} PendingGrabs;

	typedef struct _PendingUngrabs  {
	    CompWindow          *w;
	} PendingUngrabs;

	typedef struct _PendingSyncs  {
	    CompWindow        *w;
	} PendingSyncs;

	typedef struct _PendingNotifies {
	    CompWindow	      *w;
	    CompWindowNotify  n;
	} PendingNotifies;


    public:

	GroupScreen (CompScreen *);
	~GroupScreen ();

	CompositeScreen *cScreen;
	GLScreen	*gScreen;

	bool ignoreMode;
	GroupResizeInfo *resizeInfo;
	GlowTexture	glowTexture;
	Group		*lastRestackedGroup;

	Atom groupWinPropertyAtom;
	Atom resizeNotifyAtom;
	
	CompPoint mouse;
	

	void
	damagePaintRectangle (CompRect);

	void
	handleEvent (XEvent *);

	void preparePaint (int);

	bool glPaintOutput (const GLScreenPaintAttrib &,
			const GLMatrix		  &,
			const CompRegion	  &,
			CompOutput		  *,
			unsigned int		    );

	void glPaintTransformedOutput (const GLScreenPaintAttrib &,
				    const GLMatrix	     &,
				    const CompRegion	      &,
				    CompOutput		      *,
				    unsigned int		);

	void donePaint ();

	void handleMotionEvent (int, int);
	void handleButtonPressEvent (XEvent *);
	void handleButtonReleaseEvent (XEvent *);

	bool
	dequeue ();

	void
	enqueueMoveNotify (CompWindow *,
			   int, int, bool, bool);

	void
	dequeueMoveNotifies ();

	void
	dequeueSyncs ();

	void
	enqueueGrabNotify (CompWindow *,
			   int, int, unsigned int, unsigned int);

	void
	dequeueGrabNotifies ();

	void
	enqueueUngrabNotify (CompWindow *);

	void
	dequeueUngrabNotifies ();

	void
	enqueueWindowNotify (CompWindow *w, CompWindowNotify n);

	void
	dequeueWindowNotifies ();
	
	bool
        tabBarTimeout (TabBar *bar);
        
        bool
        dragHoverTimeout (CompWindow *w);
    
        void
        tabChangeActivateEvent (bool activating);

	bool
	selectSingle (CompAction         *action,
		      CompAction::State  state,
		      CompOption::Vector &options);

	bool
	select (CompAction         *action,
		CompAction::State  state,
		CompOption::Vector &options);

	bool
	selectTerminate (CompAction         *action,
			 CompAction::State  state,
			 CompOption::Vector &options);

	bool
	groupWindows (CompAction         *action,
		      CompAction::State  state,
		      CompOption::Vector &options);

	bool
	ungroupWindows (CompAction         *action,
			CompAction::State  state,
			CompOption::Vector &options);

	bool
	removeWindow (CompAction         *action,
		      CompAction::State  state,
		      CompOption::Vector &options);

	bool
	closeWindows (CompAction         *action,
		      CompAction::State  state,
		      CompOption::Vector &options);
		      
	bool
	changeColor (CompAction	        *action,
		     CompAction::State  state,
		     CompOption::Vector &option);
	bool
	setIgnore (CompAction         *action,
		   CompAction::State  state,
		   CompOption::Vector &options);

	bool
	unsetIgnore (CompAction         *action,
		     CompAction::State  state,
		     CompOption::Vector &options);

	bool
	initTab (CompAction         *action,
		 CompAction::State  state,
		 CompOption::Vector &options);

	Bool
	changeTabLeft (CompAction         *action,
		       CompAction::State  state,
		       CompOption::Vector &options);

	bool
	changeTabRight (CompAction         *action,
			CompAction::State  state,
			CompOption::Vector &options);

	std::list <PendingMoves *>    pendingMoves;
	std::list <PendingSyncs *>    pendingSyncs;
	std::list <PendingGrabs *>    pendingGrabs;
	std::list <PendingUngrabs *>  pendingUngrabs;
	std::list <PendingNotifies *> pendingNotifies;

	CompTimer		     dequeueTimeoutHandle;

	std::list <Group *>	     groups;
	SelectionRect                masterSelectionRect;

	/* should rather be replaced with a list once we get mpx */
	Selection 		     masterSelection;

	bool			     queued;

	GrabState	     grabState;
	CompScreen::GrabHandle	     grabIndex;

	Group			     *lastHoveredGroup;

	CompTimer			     showDelayTimeoutHandle;

	/* For selection */
	Bool painted;
	int  vpX, vpY;
	int  x1, y1, x2, y2;

	/* For d&d */
	Tab		     *draggedSlot;
	CompTimer         dragHoverTimeoutHandle;
	Bool              dragged;
	int               prevX, prevY; /* Buffer for mouse coordinates */
	MousePoller       poller;

	void positionUpdate (const CompPoint &); // mousepoll callback function

	CompTimer	      initialActionsTimeoutHandle;

    public:

	void
	optionChanged (CompOption            *opt,
		       GroupOptions::Options num);

	bool
	applyInitialActions ();

	void grabScreen (GrabState);

	void updateTabBars (Window);

	bool
	showDelayTimeout (TabBar *);
	
	CompRegion
	getConstrainRegion ();

};

#define GROUP_SCREEN(s)							       \
    GroupScreen *gs = GroupScreen::get (s);

class GroupWindow :
    public PluginClassHandler <GroupWindow, CompWindow>,
    public WindowInterface,
    public CompositeWindowInterface,
    public GLWindowInterface
{
    public:

	typedef struct _HideInfo {
	    Window shapeWindow;

	    unsigned long skipState;
	    unsigned long shapeMask;

	    XRectangle *inputRects;
	    int        nInputRects;
	    int        inputRectOrdering;
	} HideInfo;

	typedef enum _State {
	    WindowNormal = 0,
	    WindowMinimized,
	    WindowShaded
	} State;

    public:

	GroupWindow (CompWindow *);
	~GroupWindow ();


	CompWindow *window;
	CompositeWindow *cWindow;
	GLWindow	*gWindow;

	GlowTexture::Quads *glowQuads;

	Selection *selection;
	Group	   *group;
	Bool inSelection;

	/* To prevent freeing the group
	property in groupFiniWindow. */
	bool readOnlyProperty;

	/* For the tab bar */
	Tab	 *tab;

	bool needsPosSync;

	State    windowState;
	HideInfo *windowHideInfo;

	CompRect  resizeGeometry;

	/* For tab animation */
	int       animateState;
	CompPoint mainTabOffset;
	CompPoint destination;
	CompPoint orgPos;

	float tx,ty;
	float xVelocity, yVelocity;
	
	void
	updateProperty ();

	void
	select ();

	bool
	is ();

	bool
	inRegion (CompRegion, float);

	void
	moveNotify (int, int, bool);

	void
	resizeNotify (int, int, int, int);


	void
	getOutputExtents (CompWindowExtents &);

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

	bool checkProperty (long int &id,
			    bool     &tabbed,
			    GLushort *color);


	void deleteGroupWindow ();
	void removeFromGroup ();

	void
	setVisibility (bool       visible);

	void
	groupClearWindowInputShape (HideInfo *hideInfo);

	CompRegion getClippingRegion ();


	void
	getStretchRectangle (CompRect     &pBox,
			     float        *xScale,
			     float        *yScale);


	unsigned int
	updateResizeRectangle (CompRect, bool);

	void
	computeGlowQuads (GLTexture::Matrix *matrix);
	
	void
	clearInputShape (HideInfo *);
	
	void
	restoreInputShape (HideInfo *);
	
	int
	adjustTabVelocity ();
    
	bool
	constrainMovement (CompRegion     constrainRegion,
				int        dx,
				int        dy,
				int        &new_dx,
				int        &new_dy);	
};

#define GROUP_WINDOW(w)							       \
     GroupWindow *gw = GroupWindow::get (w);

class GroupPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <GroupScreen, GroupWindow>
{
    public:

	bool init ();
};



#endif
