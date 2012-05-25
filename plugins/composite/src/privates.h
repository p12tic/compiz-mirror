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

#include <memory>
#include <boost/shared_ptr.hpp>

#include <composite/composite.h>
#include <core/atoms.h>

#include "composite_options.h"

extern CompPlugin::VTable *compositeVTable;

extern CompWindow *lastDamagedWindow;

class ServerGrabInterface
{
    public:

	virtual ~ServerGrabInterface () {}

	virtual void grabServer () = 0;
	virtual void ungrabServer () = 0;
	virtual void syncServer () = 0;
};

class ServerLock
{
    public:

	ServerLock (ServerGrabInterface *grab) :
	    mGrab (grab)
	{
	    mGrab->grabServer ();
	    mGrab->syncServer ();
	}

	~ServerLock ()
	{
	    mGrab->ungrabServer ();
	    mGrab->syncServer ();
	}

    private:

	ServerGrabInterface *mGrab;
};

class PrivateCompositeScreen :
    ScreenInterface,
    public ServerGrabInterface,
    public CompositeOptions
{
    public:
	PrivateCompositeScreen (CompositeScreen *cs);
	~PrivateCompositeScreen ();

	bool setOption (const CompString &name, CompOption::Value &value);

	void outputChangeNotify ();

	void handleEvent (XEvent *event);

	void makeOutputWindow ();

	bool init ();

	void handleExposeEvent (XExposeEvent *event);

	void detectRefreshRate ();

	void scheduleRepaint ();

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

	struct timeval lastRedraw;
	int            redrawTime;
	int            optimalRedrawTime;
	bool           scheduled, painting, reschedule;

	bool slowAnimations;

	CompTimer paintTimer;

	compiz::composite::PaintHandler *pHnd;

	CompositeFPSLimiterMode FPSLimiterMode;

	CompWindowList withDestroyedWindows;

	Atom cmSnAtom;
	Window newCmSnOwner;
    private:

	void grabServer ();
	void ungrabServer ();
	void syncServer ();
};

class CompositePixmapRebindInterface
{
    public:

	virtual ~CompositePixmapRebindInterface () {}

	virtual Pixmap pixmap () const = 0;
	virtual bool bind () = 0;
	virtual const CompSize & size () const = 0;
	virtual void release () = 0;
};

class WindowAttributesGetInterface
{
    public:

	virtual ~WindowAttributesGetInterface () {}

	virtual bool getAttributes (XWindowAttributes &) = 0;
};

class WindowPixmapInterface
{
    public:

	virtual ~WindowPixmapInterface () {}

	typedef boost::shared_ptr <WindowPixmapInterface> Ptr;

	virtual Pixmap pixmap () const = 0;
	virtual void releasePixmap ()  = 0;
};

class X11WindowPixmap :
    public WindowPixmapInterface
{
    public:

	X11WindowPixmap (Display *d, Pixmap p) :
	    mDisplay (d),
	    mPixmap (p)
	{
	    printf ("create for : 0x%x\n", (unsigned int) p);
	}

	Pixmap pixmap () const
	{
	    return mPixmap;
	}

	void releasePixmap ()
	{
	    printf ("release for : 0x%x\n", (unsigned int) mPixmap);
	    if (mPixmap)
		XFreePixmap (mDisplay, mPixmap);

	    mPixmap = None;
	}

    private:

	Display *mDisplay;
	Pixmap  mPixmap;
};

class WindowPixmap
{
    public:

	WindowPixmap () :
	    mPixmap ()
	{
	}

	WindowPixmap (WindowPixmapInterface::Ptr &pm) :
	    mPixmap (pm)
	{
	}

	Pixmap pixmap () const
	{
	    if (mPixmap)
		return mPixmap->pixmap ();

	    return None;
	}

	~WindowPixmap ()
	{
	    if (mPixmap)
		mPixmap->releasePixmap ();
	}
    private:

	WindowPixmapInterface::Ptr mPixmap;
};

class WindowPixmapGetInterface
{
    public:

	virtual ~WindowPixmapGetInterface () {}

	virtual WindowPixmapInterface::Ptr getPixmap () = 0;
};


class PrivateCompositeWindow :
    public WindowInterface,
    public CompositePixmapRebindInterface,
    public WindowPixmapGetInterface,
    public WindowAttributesGetInterface
{
    public:
	PrivateCompositeWindow (CompWindow *w, CompositeWindow *cw);
	~PrivateCompositeWindow ();

	void windowNotify (CompWindowNotify n);
	void resizeNotify (int dx, int dy, int dwidth, int dheight);
	void moveNotify (int dx, int dy, bool now);

	Pixmap pixmap () const;
	bool   bind ();
	const CompSize & size () const;
	virtual void release ();

	static void handleDamageRect (CompositeWindow *w,
				      int             x,
				      int             y,
				      int             width,
				      int             height);

    public:
	CompWindow      *window;
	CompositeWindow *cWindow;
	CompositeScreen *cScreen;

	std::auto_ptr <WindowPixmap>  mPixmap;
	CompSize      mSize;
	bool	      needsRebind;
	CompositeWindow::NewPixmapReadyCallback newPixmapReadyCallback;

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
    private:

	bool getAttributes (XWindowAttributes &);
	WindowPixmapInterface::Ptr getPixmap ();
};

#endif
