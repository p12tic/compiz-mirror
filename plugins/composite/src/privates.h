/*
 * Copyright © 2008 Dennis Kasprzyk
 * Copyright © 2007 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Dennis Kasprzyk not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Dennis Kasprzyk makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DENNIS KASPRZYK DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DENNIS KASPRZYK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Dennis Kasprzyk <onestone@compiz-fusion.org>
 *          David Reveman <davidr@novell.com>
 */

#ifndef _COMPOSITE_PRIVATES_H
#define _COMPOSITE_PRIVATES_H

#include <composite/composite.h>
#include <core/atoms.h>

#include "paintscheduler.h"

#include "composite_options.h"

extern CompPlugin::VTable *compositeVTable;

extern CompWindow *lastDamagedWindow;
	

class PrivateCompositeScreen :
    ScreenInterface,
    public compiz::composite::scheduler::PaintSchedulerDispatchBase,
    public CompositeOptions
{
    public:
	PrivateCompositeScreen (CompositeScreen *cs);
	~PrivateCompositeScreen ();

	void outputChangeNotify ();

	void handleEvent (XEvent *event);

	void makeOutputWindow ();

	bool init ();

	void handleExposeEvent (XExposeEvent *event);

	void detectRefreshRate ();

    protected:

	void prepareScheduledPaint (unsigned int timeDiff);

	void paintScheduledPaint ();

	bool syncScheduledPaint ();

	void doneScheduledPaint ();

	bool schedulerCompositingActive ();

	bool schedulerHasVsync ();

    public:

	CompositeScreen *cScreen;

	int compositeEvent, compositeError, compositeOpcode;
	int damageEvent, damageError;
	int fixesEvent, fixesError, fixesVersion;

	bool shapeExtension;
	int  shapeEvent, shapeError;

	bool randrExtension;
	int  randrEvent, randrError;

	CompRegion    damage;
	unsigned long damageMask;

	CompRegion    tmpRegion;

	Window	      overlay;
	Window	      output;

	std::list <CompRect> exposeRects;

	CompPoint windowPaintOffset;

	int overlayWindowCount;

	bool slowAnimations;

	compiz::composite::PaintHandler 	     *pHnd;
	compiz::composite::scheduler::PaintScheduler scheduler;

	CompWindowList withDestroyedWindows;
};

class PrivateCompositeWindow : WindowInterface
{
    public:
	PrivateCompositeWindow (CompWindow *w, CompositeWindow *cw);
	~PrivateCompositeWindow ();

	void windowNotify (CompWindowNotify n);
	void resizeNotify (int dx, int dy, int dwidth, int dheight);
	void moveNotify (int dx, int dy, bool now);

	static void handleDamageRect (CompositeWindow *w,
				      int             x,
				      int             y,
				      int             width,
				      int             height);

    public:
	CompWindow      *window;
	CompositeWindow *cWindow;
	CompositeScreen *cScreen;

	Pixmap	      pixmap;
	CompSize      size;

	Damage	      damage;

	bool	      damaged;
	bool	      redirected;
	bool          overlayWindow;
	bool          bindFailed;

	unsigned short opacity;
	unsigned short brightness;
	unsigned short saturation;

	XRectangle *damageRects;
	int        sizeDamage;
	int        nDamage;
};

#endif
