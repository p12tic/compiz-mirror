/*
 * Copyright © 2005 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECI<<<<<fAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <dlfcn.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/shape.h>
#include <X11/cursorfont.h>

#include <compiz-core.h>

#include <compscreen.h>
#include <compicon.h>
#include "privatescreen.h"

#define NUM_OPTIONS(s) (sizeof ((s)->priv->opt) / sizeof (CompOption))

CompObject::indices screenPrivateIndices (0);

int
CompScreen::allocPrivateIndex ()
{
    return CompObject::allocatePrivateIndex (COMP_OBJECT_TYPE_SCREEN,
					     &screenPrivateIndices);
}

void
CompScreen::freePrivateIndex (int index)
{
    CompObject::freePrivateIndex (COMP_OBJECT_TYPE_SCREEN,
				  &screenPrivateIndices, index);
}


bool
PrivateScreen::desktopHintEqual (unsigned long *data,
				 int           size,
				 int           offset,
				 int           hintSize)
{
    if (size != desktopHintSize)
	return false;

    if (memcmp (data + offset,
		desktopHintData + offset,
		hintSize * sizeof (unsigned long)) == 0)
	return true;

    return false;
}

void
PrivateScreen::setDesktopHints ()
{
    unsigned long *data;
    int		  dSize, offset, hintSize;
    unsigned int  i;

    dSize = nDesktop * 2 + nDesktop * 2 + nDesktop * 4 + 1;

    data = (unsigned long *) malloc (sizeof (unsigned long) * dSize);
    if (!data)
	return;

    offset   = 0;
    hintSize = nDesktop * 2;

    for (i = 0; i < nDesktop; i++)
    {
	data[offset + i * 2 + 0] = vp.x () * size.width ();
	data[offset + i * 2 + 1] = vp.y () * size.height ();
    }

    if (!desktopHintEqual (data, dSize, offset, hintSize))
	XChangeProperty (display->dpy (), root,
			 display->atoms ().desktopViewport,
			 XA_CARDINAL, 32, PropModeReplace,
			 (unsigned char *) &data[offset], hintSize);

    offset += hintSize;

    for (i = 0; i < nDesktop; i++)
    {
	data[offset + i * 2 + 0] = size.width () * vpSize.width ();
	data[offset + i * 2 + 1] = size.height () * vpSize.height ();
    }

    if (!desktopHintEqual (data, dSize, offset, hintSize))
	XChangeProperty (display->dpy (), root,
			 display->atoms ().desktopGeometry,
			 XA_CARDINAL, 32, PropModeReplace,
			 (unsigned char *) &data[offset], hintSize);

    offset += hintSize;
    hintSize = nDesktop * 4;

    for (i = 0; i < nDesktop; i++)
    {
	data[offset + i * 4 + 0] = workArea.x;
	data[offset + i * 4 + 1] = workArea.y;
	data[offset + i * 4 + 2] = workArea.width;
	data[offset + i * 4 + 3] = workArea.height;
    }

    if (!desktopHintEqual (data, dSize, offset, hintSize))
	XChangeProperty (display->dpy (), root,
			 display->atoms ().workarea,
			 XA_CARDINAL, 32, PropModeReplace,
			 (unsigned char *) &data[offset], hintSize);

    offset += hintSize;

    data[offset] = nDesktop;
    hintSize = 1;

    if (!desktopHintEqual (data, dSize, offset, hintSize))
	XChangeProperty (display->dpy (), root,
			 display->atoms ().numberOfDesktops,
			 XA_CARDINAL, 32, PropModeReplace,
			 (unsigned char *) &data[offset], hintSize);

    if (desktopHintData)
	free (desktopHintData);

    desktopHintData = data;
    desktopHintSize = dSize;
}

void
PrivateScreen::setVirtualScreenSize (int newh, int newv)
{
    vpSize.setWidth (newh);
    vpSize.setHeight (newv);

    setDesktopHints ();
}

void
PrivateScreen::updateOutputDevices ()
{
    CompOption::Value::Vector &list =
	opt[COMP_SCREEN_OPTION_OUTPUTS].value ().list ();

    unsigned int  nOutput = 0;
    int		  x, y, bits;
    unsigned int  width, height;
    int		  x1, y1, x2, y2;
    Region	  region;
    char          str[10];

    foreach (CompOption::Value &value, list)
    {
	x      = 0;
	y      = 0;
	width  = size.width ();
	height = size.height ();

	bits = XParseGeometry (value.s ().c_str (), &x, &y, &width, &height);

	if (bits & XNegative)
	    x = size.width () + x - width;

	if (bits & YNegative)
	    y = size.height () + y - height;

	x1 = x;
	y1 = y;
	x2 = x + width;
	y2 = y + height;

	if (x1 < 0)
	    x1 = 0;
	if (y1 < 0)
	    y1 = 0;
	if (x2 > size.width ())
	    x2 = size.width ();
	if (y2 > size.height ())
	    y2 = size.height ();

	if (x1 < x2 && y1 < y2)
	{
	    if (outputDevs.size () < nOutput + 1)
		outputDevs.resize (nOutput + 1);

	    outputDevs[nOutput].setGeometry (x1, x2, y1, y2);
	    nOutput++;
	}
    }

    /* make sure we have at least one output */
    if (!nOutput)
    {
	if (outputDevs.size () < 1)
	    outputDevs.resize (1);

	outputDevs[0].setGeometry (0, size.width (), 0, size.height ());
	nOutput = 1;
    }

    if (outputDevs.size () > nOutput)
	outputDevs.resize (nOutput);

    /* set name, width, height and update rect pointers in all regions */
    for (unsigned int i = 0; i < nOutput; i++)
    {
	snprintf (str, 10, "Output %d", i);
	outputDevs[i].setId (str, i);
    }


    hasOverlappingOutputs = false;

    screen->setCurrentOutput (currentOutputDev);

    screen->updateWorkarea ();

    screen->setDefaultViewport ();
    screen->damageScreen ();

    region = XCreateRegion ();
    if (region)
    {
	REGION r;

	r.rects = &r.extents;
	r.numRects = 1;

	for (unsigned int i = 0; i < outputDevs.size () - 1; i++)
	    for (unsigned int j = i + 1; j < outputDevs.size (); j++)
            {
		XIntersectRegion (outputDevs[i].region (),
				  outputDevs[j].region (),
				  region);
		if (REGION_NOT_EMPTY (region))
		    hasOverlappingOutputs = true;
	    }
	XSubtractRegion (&emptyRegion, &emptyRegion, region);
	
	if (display->screenInfo ().size ())
	{
	    for (int i = 0; i < display->screenInfo ().size (); i++)
	    {
		r.extents.x1 = display->screenInfo ()[i].x_org;
		r.extents.y1 = display->screenInfo ()[i].y_org;
		r.extents.x2 = r.extents.x1 + display->screenInfo ()[i].width;
		r.extents.y2 = r.extents.y1 + display->screenInfo ()[i].height;

		XUnionRegion (region, &r, region);
	    }
	}
	else
	{
	    r.extents.x1 = 0;
	    r.extents.y1 = 0;
	    r.extents.x2 = size.width ();
	    r.extents.y2 = size.height ();

	    XUnionRegion (region, &r, region);
	}

	/* remove all output regions from visible screen region */
	for (unsigned int i = 0; i < outputDevs.size (); i++)
	    XSubtractRegion (region, outputDevs[i].region (), region);

	/* we should clear color buffers before swapping if we have visible
	   regions without output */
	clearBuffers = REGION_NOT_EMPTY (region);

	XDestroyRegion (region);
    }

    screen->outputChangeNotify ();
}

void
PrivateScreen::detectOutputDevices ()
{
    if (!noDetection && opt[COMP_SCREEN_OPTION_DETECT_OUTPUTS].value ().b ())
    {
	CompString	  name;
	CompOption::Value value;

	if (display->screenInfo ().size ())
	{
	    CompOption::Value::Vector l;
	    foreach (XineramaScreenInfo xi, display->screenInfo ())
	    {
		l.push_back (compPrintf ("%dx%d+%d+%d", xi.width, xi.height,
					 xi.x_org, xi.y_org));
	    }

	    value.set (CompOption::TypeString, l);
	}
	else
	{
	    CompOption::Value::Vector l;
	    l.push_back (compPrintf ("%dx%d+%d+%d", this->size.width(),
				     this->size.height (), 0, 0));
	    value.set (CompOption::TypeString, l);
	}

	name = opt[COMP_SCREEN_OPTION_OUTPUTS].name ();

	opt[COMP_SCREEN_OPTION_DETECT_OUTPUTS].value ().set (false);
	core->setOptionForPlugin (screen, "core", name.c_str (), value);
	opt[COMP_SCREEN_OPTION_DETECT_OUTPUTS].value ().set (true);

    }
    else
    {
	updateOutputDevices ();
    }
}

CompOption::Vector &
CompScreen::getScreenOptions (CompObject *object)
{
    CompScreen *screen = dynamic_cast<CompScreen *> (object);
    if (screen)
	return screen->priv->opt;
    return noOptions;
}

bool
CompScreen::setScreenOption (CompObject        *object,
			     const char        *name,
			     CompOption::Value &value)
{
    CompScreen *screen = dynamic_cast<CompScreen *> (object);
    if (screen)
	return screen->setOption (name, value);
    return false;
}

bool
CompScreen::setOption (const char        *name,
		       CompOption::Value &value)
{
    CompOption   *o;
    unsigned int index;

    o = CompOption::findOption (priv->opt, name, &index);
    if (!o)
	return false;

    switch (index) {
    case COMP_SCREEN_OPTION_DETECT_REFRESH_RATE:
	if (o->set (value))
	{
	    if (value.b ())
		detectRefreshRate ();

	    return true;
	}
	break;
    case COMP_SCREEN_OPTION_DETECT_OUTPUTS:
	if (o->set (value))
	{
	    if (value.b ())
		priv->detectOutputDevices ();

	    return true;
	}
	break;
    case COMP_SCREEN_OPTION_REFRESH_RATE:
	if (priv->opt[COMP_SCREEN_OPTION_DETECT_REFRESH_RATE].value ().b ())
	    return false;

	if (o->set (value))
	{
	    priv->redrawTime = 1000 / o->value ().i ();
	    priv->optimalRedrawTime = priv->redrawTime;
	    return true;
	}
	break;
    case COMP_SCREEN_OPTION_HSIZE:
	if (o->set (value))
	{
	    CompOption *vsize;

	    vsize = CompOption::findOption (priv->opt, "vsize");

	    if (!vsize)
		return false;

	    if (o->value ().i () * priv->size.width () > MAXSHORT)
		return false;

	    priv->setVirtualScreenSize (o->value ().i (), vsize->value ().i ());
	    return true;
	}
	break;
    case COMP_SCREEN_OPTION_VSIZE:
	if (o->set (value))
	{
	    CompOption *hsize;

	    hsize = CompOption::findOption (priv->opt, "hsize");

	    if (!hsize)
		return false;

	    if (o->value ().i () * priv->size.height () > MAXSHORT)
		return false;

	    priv->setVirtualScreenSize (hsize->value ().i (), o->value ().i ());
	    return true;
	}
	break;
    case COMP_SCREEN_OPTION_NUMBER_OF_DESKTOPS:
	if (o->set (value))
	{
	    setNumberOfDesktops (o->value ().i ());
	    return true;
	}
	break;
    case COMP_SCREEN_OPTION_DEFAULT_ICON:
	if (o->set (value))
	    return updateDefaultIcon ();
	break;
    case COMP_SCREEN_OPTION_OUTPUTS:
	if (!noDetection &&
	    priv->opt[COMP_SCREEN_OPTION_DETECT_OUTPUTS].value ().b ())
	    return false;

	if (o->set (value))
	{
	    priv->updateOutputDevices ();
	    return true;
	}
	break;
     case COMP_SCREEN_OPTION_FORCE_INDEPENDENT:
	if (o->set (value))
	{
	    priv->updateOutputDevices ();
	    return true;
	}
	break;
    default:
	if (CompOption::setScreenOption (this, *o, value))
	    return true;
	break;
    }

    return false;
}

const CompMetadata::OptionInfo coreScreenOptionInfo[COMP_SCREEN_OPTION_NUM] = {
    { "detect_refresh_rate", "bool", 0, 0, 0 },
    { "lighting", "bool", 0, 0, 0 },
    { "refresh_rate", "int", "<min>1</min>", 0, 0 },
    { "hsize", "int", "<min>1</min><max>32</max>", 0, 0 },
    { "vsize", "int", "<min>1</min><max>32</max>", 0, 0 },
    { "opacity_step", "int", "<min>1</min>", 0, 0 },
    { "unredirect_fullscreen_windows", "bool", 0, 0, 0 },
    { "default_icon", "string", 0, 0, 0 },
    { "sync_to_vblank", "bool", 0, 0, 0 },
    { "number_of_desktops", "int", "<min>1</min>", 0, 0 },
    { "detect_outputs", "bool", 0, 0, 0 },
    { "outputs", "list", "<type>string</type>", 0, 0 },
    { "overlapping_outputs", "int",
      RESTOSTRING (0, OUTPUT_OVERLAP_MODE_LAST), 0, 0 },
    { "focus_prevention_level", "int",
      RESTOSTRING (0, FOCUS_PREVENTION_LEVEL_LAST), 0, 0 },
    { "focus_prevention_match", "match", 0, 0, 0 },
    { "texture_compression", "bool", 0, 0, 0 },
    { "force_independent_output_painting", "bool", 0, 0, 0 }
};

void
PrivateScreen::updateStartupFeedback ()
{
    if (!startupSequences.empty ())
	XDefineCursor (display->dpy (), root, busyCursor);
    else
	XDefineCursor (display->dpy (), root, normalCursor);
}

#define STARTUP_TIMEOUT_DELAY 15000

bool
PrivateScreen::handleStartupSequenceTimeout()
{
    CompStartupSequence *s;
    struct timeval	now, active;
    double		elapsed;

    gettimeofday (&now, NULL);

    foreach (CompStartupSequence *s, startupSequences)
    {
	sn_startup_sequence_get_last_active_time (s->sequence,
						  &active.tv_sec,
						  &active.tv_usec);

	elapsed = ((((double) now.tv_sec - active.tv_sec) * 1000000.0 +
		    (now.tv_usec - active.tv_usec))) / 1000.0;

	if (elapsed > STARTUP_TIMEOUT_DELAY)
	    sn_startup_sequence_complete (s->sequence);
    }

    return true;
}

void
PrivateScreen::addSequence (SnStartupSequence *sequence)
{
    CompStartupSequence *s;

    s = new CompStartupSequence ();
    if (!s)
	return;

    sn_startup_sequence_ref (sequence);

    s->sequence = sequence;
    s->viewportX = vp.x ();
    s->viewportY = vp.y ();

    startupSequences.push_front (s);

    if (!startupSequenceTimer.active ())
	startupSequenceTimer.start (
	    boost::bind (&PrivateScreen::handleStartupSequenceTimeout, this),
	    1000, 1500);

    updateStartupFeedback ();
}

void
PrivateScreen::removeSequence (SnStartupSequence *sequence)
{
    CompStartupSequence *s = NULL;

    std::list<CompStartupSequence *>::iterator it = startupSequences.begin ();

    while (it != startupSequences.end ())
    {
	if ((*it)->sequence == sequence)
	{
	    s = (*it);
	    break;
	}
    }

    if (!s)
	return;

    sn_startup_sequence_unref (sequence);

    startupSequences.erase (it);

    if (startupSequences.empty () && startupSequenceTimer.active ())
	startupSequenceTimer.stop ();

    updateStartupFeedback ();
}

void
CompScreen::compScreenSnEvent (SnMonitorEvent *event,
			       void           *userData)
{
    CompScreen	      *screen = (CompScreen *) userData;
    SnStartupSequence *sequence;

    sequence = sn_monitor_event_get_startup_sequence (event);

    switch (sn_monitor_event_get_type (event)) {
    case SN_MONITOR_EVENT_INITIATED:
	screen->priv->addSequence (sequence);
	break;
    case SN_MONITOR_EVENT_COMPLETED:
	screen->priv->removeSequence (sn_monitor_event_get_startup_sequence (event));
	break;
    case SN_MONITOR_EVENT_CHANGED:
    case SN_MONITOR_EVENT_CANCELED:
	break;
    }
}

void
PrivateScreen::updateScreenEdges ()
{
    struct screenEdgeGeometry {
	int xw, x0;
	int yh, y0;
	int ww, w0;
	int hh, h0;
    } geometry[SCREEN_EDGE_NUM] = {
	{ 0,  0,   0,  2,   0,  2,   1, -4 }, /* left */
	{ 1, -2,   0,  2,   0,  2,   1, -4 }, /* right */
	{ 0,  2,   0,  0,   1, -4,   0,  2 }, /* top */
	{ 0,  2,   1, -2,   1, -4,   0,  2 }, /* bottom */
	{ 0,  0,   0,  0,   0,  2,   0,  2 }, /* top-left */
	{ 1, -2,   0,  0,   0,  2,   0,  2 }, /* top-right */
	{ 0,  0,   1, -2,   0,  2,   0,  2 }, /* bottom-left */
	{ 1, -2,   1, -2,   0,  2,   0,  2 }  /* bottom-right */
    };
    int i;

    for (i = 0; i < SCREEN_EDGE_NUM; i++)
    {
	if (screenEdge[i].id)
	    XMoveResizeWindow (display->dpy (), screenEdge[i].id,
			       geometry[i].xw * size.width () + geometry[i].x0,
			       geometry[i].yh * size.height () + geometry[i].y0,
			       geometry[i].ww * size.width () + geometry[i].w0,
			       geometry[i].hh * size.height () + geometry[i].h0);
    }
}

static void
frustum (GLfloat *m,
	 GLfloat left,
	 GLfloat right,
	 GLfloat bottom,
	 GLfloat top,
	 GLfloat nearval,
	 GLfloat farval)
{
    GLfloat x, y, a, b, c, d;

    x = (2.0 * nearval) / (right - left);
    y = (2.0 * nearval) / (top - bottom);
    a = (right + left) / (right - left);
    b = (top + bottom) / (top - bottom);
    c = -(farval + nearval) / ( farval - nearval);
    d = -(2.0 * farval * nearval) / (farval - nearval);

#define M(row,col)  m[col*4+row]
    M(0,0) = x;     M(0,1) = 0.0f;  M(0,2) = a;      M(0,3) = 0.0f;
    M(1,0) = 0.0f;  M(1,1) = y;     M(1,2) = b;      M(1,3) = 0.0f;
    M(2,0) = 0.0f;  M(2,1) = 0.0f;  M(2,2) = c;      M(2,3) = d;
    M(3,0) = 0.0f;  M(3,1) = 0.0f;  M(3,2) = -1.0f;  M(3,3) = 0.0f;
#undef M

}

static void
perspective (GLfloat *m,
	     GLfloat fovy,
	     GLfloat aspect,
	     GLfloat zNear,
	     GLfloat zFar)
{
    GLfloat xmin, xmax, ymin, ymax;

    ymax = zNear * tan (fovy * M_PI / 360.0);
    ymin = -ymax;
    xmin = ymin * aspect;
    xmax = ymax * aspect;

    frustum (m, xmin, xmax, ymin, ymax, zNear, zFar);
}

void
CompScreen::setCurrentOutput (unsigned int outputNum)
{
    if (outputNum >= priv->outputDevs.size ())
	outputNum = 0;

    priv->currentOutputDev = outputNum;
}

void
PrivateScreen::reshape (int w, int h)
{

#ifdef USE_COW
    if (useCow)
	XMoveResizeWindow (display->dpy (), overlay, 0, 0, w, h);
#endif

    display->updateScreenInfo();

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();
    glDepthRange (0, 1);
    glViewport (-1, -1, 2, 2);
    glRasterPos2f (0, 0);

    rasterPos = CompPoint (0, 0);

    perspective (projection, 60.0f, 1.0f, 0.1f, 100.0f);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glMultMatrixf (projection);
    glMatrixMode (GL_MODELVIEW);

    region.rects = &region.extents;
    region.numRects = 1;
    region.extents.x1 = 0;
    region.extents.y1 = 0;
    region.extents.x2 = w;
    region.extents.y2 = h;
    region.size = 1;

    size.setWidth (w);
    size.setHeight (h);

    fullscreenOutput.setId ("fullscreen", ~0);
    fullscreenOutput.setGeometry (0, 0, w, h);

    updateScreenEdges ();
}

void
CompScreen::configure (XConfigureEvent *ce)
{
    if (priv->attrib.width  != ce->width ||
	priv->attrib.height != ce->height)
    {
	priv->attrib.width  = ce->width;
	priv->attrib.height = ce->height;

	priv->reshape (ce->width, ce->height);

	priv->detectOutputDevices ();

	damageScreen ();
    }
}

FuncPtr
CompScreen::getProcAddress (const char *name)
{
    static void *dlhand = NULL;
    FuncPtr     funcPtr = NULL;

    if (priv->getProcAddress)
	funcPtr = priv->getProcAddress ((GLubyte *) name);

    if (!funcPtr)
    {
	if (!dlhand)
	    dlhand = dlopen (NULL, RTLD_LAZY);

	if (dlhand)
	{
	    dlerror ();
	    funcPtr = (FuncPtr) dlsym (dlhand, name);
	    if (dlerror () != NULL)
		funcPtr = NULL;
	}
    }

    return funcPtr;
}

void
PrivateScreen::updateScreenBackground (CompTexture *texture)
{
    Display	  *dpy = display->dpy ();
    Atom	  pixmapAtom, actualType;
    int		  actualFormat, i, status;
    unsigned int  width = 1, height = 1, depth = 0;
    unsigned long nItems;
    unsigned long bytesAfter;
    unsigned char *prop;
    Pixmap	  pixmap = 0;

    pixmapAtom = XInternAtom (dpy, "PIXMAP", FALSE);

    for (i = 0; pixmap == 0 && i < 2; i++)
    {
	status = XGetWindowProperty (dpy, root,
				     display->atoms ().xBackground[i],
				     0, 4, FALSE, AnyPropertyType,
				     &actualType, &actualFormat, &nItems,
				     &bytesAfter, &prop);

	if (status == Success && nItems && prop)
	{
	    if (actualType   == pixmapAtom &&
		actualFormat == 32         &&
		nItems	     == 1)
	    {
		Pixmap p;

		memcpy (&p, prop, 4);

		if (p)
		{
		    unsigned int ui;
		    int		 i;
		    Window	 w;

		    if (XGetGeometry (dpy, p, &w, &i, &i,
				      &width, &height, &ui, &depth))
		    {
			if (depth == attrib.depth)
			    pixmap = p;
		    }
		}
	    }

	    XFree (prop);
	}
    }

    if (pixmap)
    {
/* FIXME:
	if (pixmap == texture->pixmap)
	    return;
*/
	texture->reset ();

	if (!texture->bindPixmap (pixmap, width, height, depth))
	{
	    compLogMessage (NULL, "core", CompLogLevelWarn,
			    "Couldn't bind background pixmap 0x%x to "
			    "texture", (int) pixmap);
	}
    }
    else
    {
	texture->reset ();
    }

    if (!texture->name () && backgroundImage)
	CompTexture::readImageToTexture (screen, texture, backgroundImage,
					 &width, &height);

    if (texture->target () == GL_TEXTURE_2D)
    {
	glBindTexture (texture->target (), texture->name ());
	glTexParameteri (texture->target (), GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (texture->target (), GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture (texture->target (), 0);
    }
}

void
CompScreen::detectRefreshRate ()
{
    if (!noDetection &&
	priv->opt[COMP_SCREEN_OPTION_DETECT_REFRESH_RATE].value ().b ())
    {
	CompString        name;
	CompOption::Value value;

	value.set ((int) 0);

	if (priv->display->XRandr())
	{
	    XRRScreenConfiguration *config;

	    config  = XRRGetScreenInfo (priv->display->dpy (), priv->root);
	    value.set ((int) XRRConfigCurrentRate (config));

	    XRRFreeScreenConfigInfo (config);
	}

	if (value.i () == 0)
	    value.set ((int) defaultRefreshRate);

	name = priv->opt[COMP_SCREEN_OPTION_REFRESH_RATE].name ();

	priv->opt[COMP_SCREEN_OPTION_DETECT_REFRESH_RATE].value ().set (false);
	core->setOptionForPlugin (this, "core", name.c_str (), value);
	priv->opt[COMP_SCREEN_OPTION_DETECT_REFRESH_RATE].value ().set (true);
    }
    else
    {
	priv->redrawTime = 1000 /
	    priv->opt[COMP_SCREEN_OPTION_REFRESH_RATE].value ().i ();
	priv->optimalRedrawTime = priv->redrawTime;
    }
}

void
PrivateScreen::setSupportingWmCheck ()
{
    XChangeProperty (display->dpy (), grabWindow,
		     display->atoms ().supportingWmCheck,
		     XA_WINDOW, 32, PropModeReplace,
		     (unsigned char *) &grabWindow, 1);

    XChangeProperty (display->dpy (), grabWindow, display->atoms ().wmName,
		     display->atoms ().utf8String, 8, PropModeReplace,
		     (unsigned char *) PACKAGE, strlen (PACKAGE));
    XChangeProperty (display->dpy (), grabWindow, display->atoms ().winState,
		     XA_ATOM, 32, PropModeReplace,
		     (unsigned char *) &display->atoms ().winStateSkipTaskbar,
		     1);
    XChangeProperty (display->dpy (), grabWindow, display->atoms ().winState,
		     XA_ATOM, 32, PropModeAppend,
		     (unsigned char *) &display->atoms ().winStateSkipPager, 1);
    XChangeProperty (display->dpy (), grabWindow, display->atoms ().winState,
		     XA_ATOM, 32, PropModeAppend,
		     (unsigned char *) &display->atoms ().winStateHidden, 1);

    XChangeProperty (display->dpy (), root, display->atoms ().supportingWmCheck,
		     XA_WINDOW, 32, PropModeReplace,
		     (unsigned char *) &grabWindow, 1);
}

void
PrivateScreen::setSupported ()
{
    Atom	data[256];
    int		i = 0;

    data[i++] = display->atoms ().supported;
    data[i++] = display->atoms ().supportingWmCheck;

    data[i++] = display->atoms ().utf8String;

    data[i++] = display->atoms ().clientList;
    data[i++] = display->atoms ().clientListStacking;

    data[i++] = display->atoms ().winActive;

    data[i++] = display->atoms ().desktopViewport;
    data[i++] = display->atoms ().desktopGeometry;
    data[i++] = display->atoms ().currentDesktop;
    data[i++] = display->atoms ().numberOfDesktops;
    data[i++] = display->atoms ().showingDesktop;

    data[i++] = display->atoms ().workarea;

    data[i++] = display->atoms ().wmName;
/*
    data[i++] = display->atoms ().wmVisibleName;
*/

    data[i++] = display->atoms ().wmStrut;
    data[i++] = display->atoms ().wmStrutPartial;

/*
    data[i++] = display->atoms ().wmPid;
*/

    data[i++] = display->atoms ().wmUserTime;
    data[i++] = display->atoms ().frameExtents;
    data[i++] = display->atoms ().frameWindow;

    data[i++] = display->atoms ().winState;
    data[i++] = display->atoms ().winStateModal;
    data[i++] = display->atoms ().winStateSticky;
    data[i++] = display->atoms ().winStateMaximizedVert;
    data[i++] = display->atoms ().winStateMaximizedHorz;
    data[i++] = display->atoms ().winStateShaded;
    data[i++] = display->atoms ().winStateSkipTaskbar;
    data[i++] = display->atoms ().winStateSkipPager;
    data[i++] = display->atoms ().winStateHidden;
    data[i++] = display->atoms ().winStateFullscreen;
    data[i++] = display->atoms ().winStateAbove;
    data[i++] = display->atoms ().winStateBelow;
    data[i++] = display->atoms ().winStateDemandsAttention;

    data[i++] = display->atoms ().winOpacity;
    data[i++] = display->atoms ().winBrightness;

    if (canDoSaturated)
    {
	data[i++] = display->atoms ().winSaturation;
	data[i++] = display->atoms ().winStateDisplayModal;
    }

    data[i++] = display->atoms ().wmAllowedActions;

    data[i++] = display->atoms ().winActionMove;
    data[i++] = display->atoms ().winActionResize;
    data[i++] = display->atoms ().winActionStick;
    data[i++] = display->atoms ().winActionMinimize;
    data[i++] = display->atoms ().winActionMaximizeHorz;
    data[i++] = display->atoms ().winActionMaximizeVert;
    data[i++] = display->atoms ().winActionFullscreen;
    data[i++] = display->atoms ().winActionClose;
    data[i++] = display->atoms ().winActionShade;
    data[i++] = display->atoms ().winActionChangeDesktop;
    data[i++] = display->atoms ().winActionAbove;
    data[i++] = display->atoms ().winActionBelow;

    data[i++] = display->atoms ().winType;
    data[i++] = display->atoms ().winTypeDesktop;
    data[i++] = display->atoms ().winTypeDock;
    data[i++] = display->atoms ().winTypeToolbar;
    data[i++] = display->atoms ().winTypeMenu;
    data[i++] = display->atoms ().winTypeSplash;
    data[i++] = display->atoms ().winTypeDialog;
    data[i++] = display->atoms ().winTypeUtil;
    data[i++] = display->atoms ().winTypeNormal;

    data[i++] = display->atoms ().wmDeleteWindow;
    data[i++] = display->atoms ().wmPing;

    data[i++] = display->atoms ().wmMoveResize;
    data[i++] = display->atoms ().moveResizeWindow;
    data[i++] = display->atoms ().restackWindow;

    XChangeProperty (display->dpy (), root, display->atoms ().supported,
		     XA_ATOM, 32, PropModeReplace, (unsigned char *) data, i);
}

void
PrivateScreen::getDesktopHints ()
{
    unsigned long data[2];
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *propData;

    result = XGetWindowProperty (display->dpy (), root,
				 display->atoms ().numberOfDesktops,
				 0L, 1L, FALSE, XA_CARDINAL, &actual, &format,
				 &n, &left, &propData);

    if (result == Success && n && propData && useDesktopHints)
    {
	memcpy (data, propData, sizeof (unsigned long));
	XFree (propData);

	if (data[0] > 0 && data[0] < 0xffffffff)
	    nDesktop = data[0];
    }

    result = XGetWindowProperty (display->dpy (), root,
				 display->atoms ().desktopViewport, 0L, 2L,
				 FALSE, XA_CARDINAL, &actual, &format,
				 &n, &left, &propData);

    if (result == Success && n && propData)
    {
	if (n == 2)
	{
	    memcpy (data, propData, sizeof (unsigned long) * 2);

	    if (data[0] / size.width () < vpSize.width () - 1)
		vp.setX (data[0] / size.width ());

	    if (data[1] / size.height () < vpSize.height () - 1)
		vp.setY (data[1] / size.height ());
	}

	XFree (propData);
    }

    result = XGetWindowProperty (display->dpy (), root,
				 display->atoms ().currentDesktop,
				 0L, 1L, FALSE, XA_CARDINAL, &actual, &format,
				 &n, &left, &propData);

    if (result == Success && n && propData && useDesktopHints)
    {
	memcpy (data, propData, sizeof (unsigned long));
	XFree (propData);

	if (data[0] < nDesktop)
	    currentDesktop = data[0];
    }

    result = XGetWindowProperty (display->dpy (), root,
				 display->atoms ().showingDesktop,
				 0L, 1L, FALSE, XA_CARDINAL, &actual, &format,
				 &n, &left, &propData);

    if (result == Success && n && propData)
    {
	memcpy (data, propData, sizeof (unsigned long));
	XFree (propData);

	if (data[0])
	    screen->enterShowDesktopMode ();
    }

    data[0] = currentDesktop;

    XChangeProperty (display->dpy (), root, display->atoms ().currentDesktop,
		     XA_CARDINAL, 32, PropModeReplace,
		     (unsigned char *) data, 1);

    data[0] = showingDesktopMask ? TRUE : FALSE;

    XChangeProperty (display->dpy (), root, display->atoms ().showingDesktop,
		     XA_CARDINAL, 32, PropModeReplace,
		     (unsigned char *) data, 1);
}

void
CompScreen::showOutputWindow ()
{

#ifdef USE_COW
    if (useCow)
    {
	Display       *dpy = priv->display->dpy ();
	XserverRegion region;

	region = XFixesCreateRegion (dpy, NULL, 0);

	XFixesSetWindowShapeRegion (dpy,
				    priv->output,
				    ShapeBounding,
				    0, 0, 0);
	XFixesSetWindowShapeRegion (dpy,
				    priv->output,
				    ShapeInput,
				    0, 0, region);

	XFixesDestroyRegion (dpy, region);

	damageScreen ();
    }
#endif

}

void
CompScreen::hideOutputWindow ()
{

#ifdef USE_COW
    if (useCow)
    {
	Display       *dpy = priv->display->dpy ();
	XserverRegion region;

	region = XFixesCreateRegion (dpy, NULL, 0);

	XFixesSetWindowShapeRegion (dpy,
				    priv->output,
				    ShapeBounding,
				    0, 0, region);

	XFixesDestroyRegion (dpy, region);
    }
#endif

}

void
CompScreen::updateOutputWindow ()
{

#ifdef USE_COW
    if (useCow)
    {
	Display       *dpy = priv->display->dpy ();
	XserverRegion region;
	static Region tmpRegion = NULL;

	if (!tmpRegion)
	{
	    tmpRegion = XCreateRegion ();
	    if (!tmpRegion)
		return;
	}

	XSubtractRegion (&priv->region, &emptyRegion, tmpRegion);

	
	for (CompWindowList::reverse_iterator rit = priv->windows.rbegin ();
	     rit != priv->windows.rend (); rit++)
	    if ((*rit)->overlayWindow ())
	    {
		XSubtractRegion (tmpRegion, (*rit)->region (), tmpRegion);
	    }
	
	XShapeCombineRegion (dpy, priv->output, ShapeBounding,
			     0, 0, tmpRegion, ShapeSet);


	region = XFixesCreateRegion (dpy, NULL, 0);

	XFixesSetWindowShapeRegion (dpy,
				    priv->output,
				    ShapeInput,
				    0, 0, region);

	XFixesDestroyRegion (dpy, region);
    }
#endif

}

void
PrivateScreen::makeOutputWindow ()
{

#ifdef USE_COW
    if (useCow)
    {
	overlay = XCompositeGetOverlayWindow (display->dpy (), root);
	output  = overlay;

	XSelectInput (display->dpy (), output, ExposureMask);
    }
    else
#endif

	output = overlay = root;

    screen->showOutputWindow ();
}

void
CompScreen::enterShowDesktopMode ()
{
    WRAPABLE_HND_FUNC(enterShowDesktopMode)

    CompDisplay   *d = priv->display;
    unsigned long data = 1;
    int		  count = 0;
    CompOption    *st = d->getOption ("hide_skip_taskbar_windows");

    priv->showingDesktopMask = ~(CompWindowTypeDesktopMask |
				CompWindowTypeDockMask);

    foreach (CompWindow *w, priv->windows)
    {
	if ((priv->showingDesktopMask & w->wmType ()) &&
	    (!(w->state () & CompWindowStateSkipTaskbarMask) ||
	    (st && st->value ().b ())))
	{
	    if (!w->inShowDesktopMode () && !w->grabbed () &&
		w->managed () && w->focus ())
	    {
		w->setShowDesktopMode (true);
		w->hide ();
	    }
	}

	if (w->inShowDesktopMode ())
	    count++;
    }

    if (!count)
    {
	priv->showingDesktopMask = 0;
	data = 0;
    }

    XChangeProperty (priv->display->dpy (), priv->root,
		     priv->display->atoms ().showingDesktop,
		     XA_CARDINAL, 32, PropModeReplace,
		     (unsigned char *) &data, 1);
}

void
CompScreen::leaveShowDesktopMode (CompWindow *window)
{
    WRAPABLE_HND_FUNC(leaveShowDesktopMode, window)

    unsigned long data = 0;

    if (window)
    {
	if (!window->inShowDesktopMode ())
	    return;

	window->setShowDesktopMode (false);
	window->show ();

	/* return if some other window is still in show desktop mode */
	foreach (CompWindow *w, priv->windows)
	    if (w->inShowDesktopMode ())
		return;

	priv->showingDesktopMask = 0;
    }
    else
    {
	priv->showingDesktopMask = 0;

	foreach (CompWindow *w, priv->windows)
	{
	    if (!w->inShowDesktopMode ())
		continue;

	    w->setShowDesktopMode (false);
	    w->show ();
	}

	/* focus default window - most likely this will be the window
	   which had focus before entering showdesktop mode */
	focusDefaultWindow ();
    }

    XChangeProperty (priv->display->dpy (), priv->root,
		     priv->display->atoms ().showingDesktop,
		     XA_CARDINAL, 32, PropModeReplace,
		     (unsigned char *) &data, 1);
}

CompWindowList
CompScreen::getWindowPaintList ()
{
    WRAPABLE_HND_FUNC_RETURN (CompWindowList, getWindowPaintList)

    return priv->windows;
}

CompScreen::CompScreen ():
    CompObject (COMP_OBJECT_TYPE_SCREEN, "screen", &screenPrivateIndices)
{
    WRAPABLE_INIT_HND(preparePaint);
    WRAPABLE_INIT_HND(donePaint);
    WRAPABLE_INIT_HND(paint);
    WRAPABLE_INIT_HND(paintOutput);
    WRAPABLE_INIT_HND(paintTransformedOutput);
    WRAPABLE_INIT_HND(enableOutputClipping);
    WRAPABLE_INIT_HND(disableOutputClipping);
    WRAPABLE_INIT_HND(applyTransform);

    WRAPABLE_INIT_HND(enterShowDesktopMode);
    WRAPABLE_INIT_HND(leaveShowDesktopMode);

    WRAPABLE_INIT_HND(outputChangeNotify);

    WRAPABLE_INIT_HND(getWindowPaintList);

    priv = new PrivateScreen (this);
    assert (priv);
    next = NULL;

    windowPrivateIndices = 0;
    windowPrivateLen     = 0;
}

PrivateScreen::PrivateScreen (CompScreen *screen) :
    screen(screen),
    display (0),
    windows (),
    size (0, 0),
    vp (0, 0),
    vpSize (1, 1),
    nDesktop (1),
    currentDesktop (0),
    damageMask (COMP_SCREEN_DAMAGE_ALL_MASK),
    root (None),
    overlay (None),
    output (None),
    grabWindow (None),
    textureRectangle (false),
    textureNonPowerOfTwo (false),
    textureEnvCombine (false),
    textureEnvCrossbar (false),
    textureBorderClamp (false),
    textureCompression (false),
    maxTextureSize (0),
    fbo (false),
    fragmentProgram (false),
    maxTextureUnits (1),
    exposeRects (0),
    backgroundTexture (screen),
    backgroundLoaded (false),
    pendingDestroys (0),
    desktopWindowCount (0),
    mapNum (1),
    activeNum (1),
    outputDevs (0),
    currentOutputDev (0),
    hasOverlappingOutputs (false),
    windowPaintOffset (0, 0),
    currentHistory (0),
    overlayWindowCount (0),
    snContext (0),
    startupSequences (0),
    startupSequenceTimer (),
    groups (0),
    defaultIcon (0),
    canDoSaturated (false),
    canDoSlightlySaturated (false),
    clientList (0),
    nClientList (0),
    buttonGrabs (0),
    keyGrabs (0),
    grabs (0),
    rasterPos (0, 0),
    nextRedraw (0),
    redrawTime (1000 / defaultRefreshRate),
    optimalRedrawTime (1000 / defaultRefreshRate),
    frameStatus (0),
    timeMult (1),
    idle (true),
    timeLeft (0),
    pendingCommands (true),
    fragmentStorage (),
    clearBuffers (true),
    lighting (false),
    slowAnimations (false),
    showingDesktopMask (0),
    desktopHintData (0),
    desktopHintSize (0),
    opt (COMP_SCREEN_OPTION_NUM),
    paintTimer (),
    getProcAddress (0)
{
    memset (history, 0, sizeof (history));
    gettimeofday (&lastRedraw, 0);
}

PrivateScreen::~PrivateScreen ()
{
    CompOption::finiScreenOptions (screen, opt);
}

bool
CompScreen::init (CompDisplay *display, int screenNum)
{
    Display              *dpy = display->dpy ();
    Window               newWmSnOwner = None, newCmSnOwner = None;
    Atom                 wmSnAtom = 0, cmSnAtom = 0;
    Time                 wmSnTimestamp = 0;
    XEvent               event;
    XSetWindowAttributes attr;
    Window               currentWmSnOwner, currentCmSnOwner;
    char                 buf[128];
    bool                 rv;

    sprintf (buf, "WM_S%d", screenNum);
    wmSnAtom = XInternAtom (dpy, buf, 0);

    currentWmSnOwner = XGetSelectionOwner (dpy, wmSnAtom);

    if (currentWmSnOwner != None)
    {
	if (!replaceCurrentWm)
	{
	    compLogMessage (display, "core", CompLogLevelError,
			    "Screen %d on display \"%s\" already "
			    "has a window manager; try using the "
			    "--replace option to replace the current "
			    "window manager.",
			    screenNum, DisplayString (dpy));

	    return false;
	}

	XSelectInput (dpy, currentWmSnOwner, StructureNotifyMask);
    }

    sprintf (buf, "_NET_WM_CM_S%d", screenNum);
    cmSnAtom = XInternAtom (dpy, buf, 0);

    currentCmSnOwner = XGetSelectionOwner (dpy, cmSnAtom);

    if (currentCmSnOwner != None)
    {
	if (!replaceCurrentWm)
	{
	    compLogMessage (display, "core", CompLogLevelError,
			    "Screen %d on display \"%s\" already "
			    "has a compositing manager; try using the "
			    "--replace option to replace the current "
			    "compositing manager.",
			    screenNum, DisplayString (dpy));

	    return false;
	}
    }

    attr.override_redirect = TRUE;
    attr.event_mask	       = PropertyChangeMask;

    newCmSnOwner = newWmSnOwner =
	XCreateWindow (dpy, XRootWindow (dpy, screenNum),
		       -100, -100, 1, 1, 0,
		       CopyFromParent, CopyFromParent,
		       CopyFromParent,
		       CWOverrideRedirect | CWEventMask,
		       &attr);

    XChangeProperty (dpy,
		     newWmSnOwner,
		     display->atoms ().wmName,
		     display->atoms ().utf8String, 8,
		     PropModeReplace,
		     (unsigned char *) PACKAGE,
		     strlen (PACKAGE));

    XWindowEvent (dpy, newWmSnOwner, PropertyChangeMask, &event);

    wmSnTimestamp = event.xproperty.time;

    XSetSelectionOwner (dpy, wmSnAtom, newWmSnOwner, wmSnTimestamp);

    if (XGetSelectionOwner (dpy, wmSnAtom) != newWmSnOwner)
    {
	compLogMessage (display, "core", CompLogLevelError,
		        "Could not acquire window manager "
		        "selection on screen %d display \"%s\"",
		        screenNum, DisplayString (dpy));

	XDestroyWindow (dpy, newWmSnOwner);

	return false;
    }

    /* Send client message indicating that we are now the WM */
    event.xclient.type	       = ClientMessage;
    event.xclient.window       = XRootWindow (dpy, screenNum);
    event.xclient.message_type = display->atoms ().manager;
    event.xclient.format       = 32;
    event.xclient.data.l[0]    = wmSnTimestamp;
    event.xclient.data.l[1]    = wmSnAtom;
    event.xclient.data.l[2]    = 0;
    event.xclient.data.l[3]    = 0;
    event.xclient.data.l[4]    = 0;

    XSendEvent (dpy, XRootWindow (dpy, screenNum), FALSE,
		StructureNotifyMask, &event);

    /* Wait for old window manager to go away */
    if (currentWmSnOwner != None)
    {
	do {
	    XWindowEvent (dpy, currentWmSnOwner,
			  StructureNotifyMask, &event);
	} while (event.type != DestroyNotify);
    }

    compCheckForError (dpy);

    XCompositeRedirectSubwindows (dpy, XRootWindow (dpy, screenNum),
				  CompositeRedirectManual);

    if (compCheckForError (dpy))
    {
	compLogMessage (display, "core", CompLogLevelError,
			"Another composite manager is already "
			"running on screen: %d", screenNum);

	return false;
    }

    XSetSelectionOwner (dpy, cmSnAtom, newCmSnOwner, wmSnTimestamp);

    if (XGetSelectionOwner (dpy, cmSnAtom) != newCmSnOwner)
    {
	compLogMessage (display, "core", CompLogLevelError,
			"Could not acquire compositing manager "
			"selection on screen %d display \"%s\"",
			screenNum, DisplayString (dpy));

	return false;
    }

    XGrabServer (dpy);

    XSelectInput (dpy, XRootWindow (dpy, screenNum),
		  SubstructureRedirectMask |
		  SubstructureNotifyMask   |
		  StructureNotifyMask      |
		  PropertyChangeMask       |
		  LeaveWindowMask          |
		  EnterWindowMask          |
		  KeyPressMask             |
		  KeyReleaseMask           |
		  ButtonPressMask          |
		  ButtonReleaseMask        |
		  FocusChangeMask          |
		  ExposureMask);

    if (compCheckForError (dpy))
    {
	compLogMessage (display, "core", CompLogLevelError,
		        "Another window manager is "
		        "already running on screen: %d", screenNum);

	XUngrabServer (dpy);
	return false;
    }

    rv = init (display, screenNum, newWmSnOwner, wmSnAtom, wmSnTimestamp);

    XUngrabServer (dpy);
    return rv;
}


bool
CompScreen::init (CompDisplay *display,
		  int         screenNum,
		  Window      wmSnSelectionWindow,
		  Atom        wmSnAtom,
		  Time        wmSnTimestamp)
{
    Display		 *dpy = display->dpy ();
    static char		 data = 0;
    XColor		 black;
    Pixmap		 bitmap;
    XVisualInfo		 templ;
    XVisualInfo		 *visinfo;
    GLXFBConfig		 *fbConfigs;
    Window		 rootReturn, parentReturn;
    Window		 *children;
    unsigned int	 nchildren;
    int			 defaultDepth, nvisinfo, nElements, value, i;
    const char		 *glxExtensions, *glExtensions;
    XSetWindowAttributes attrib;
    GLfloat		 globalAmbient[]  = { 0.1f, 0.1f,  0.1f, 0.1f };
    GLfloat		 ambientLight[]   = { 0.0f, 0.0f,  0.0f, 0.0f };
    GLfloat		 diffuseLight[]   = { 0.9f, 0.9f,  0.9f, 0.9f };
    GLfloat		 light0Position[] = { -0.5f, 0.5f, -9.0f, 1.0f };

    priv->display = display;

    if (!coreMetadata->initScreenOptions (this, coreScreenOptionInfo,
					  COMP_SCREEN_OPTION_NUM, priv->opt))
	return false;

    priv->damage = XCreateRegion ();
    if (!priv->damage)
	return false;

    priv->vpSize.setWidth (priv->opt[COMP_SCREEN_OPTION_HSIZE].value ().i ());
    priv->vpSize.setHeight (priv->opt[COMP_SCREEN_OPTION_VSIZE].value ().i ());

    for (i = 0; i < SCREEN_EDGE_NUM; i++)
    {
	priv->screenEdge[i].id    = None;
	priv->screenEdge[i].count = 0;
    }

    priv->screenNum = screenNum;
    priv->colormap  = DefaultColormap (dpy, screenNum);
    priv->root	    = XRootWindow (dpy, screenNum);

    priv->snContext = sn_monitor_context_new (display->snDisplay (),
					      screenNum,
					      compScreenSnEvent, this,
					      NULL);

    priv->wmSnSelectionWindow = wmSnSelectionWindow;
    priv->wmSnAtom            = wmSnAtom;
    priv->wmSnTimestamp       = wmSnTimestamp;

    if (!XGetWindowAttributes (dpy, priv->root, &priv->attrib))
	return false;

    priv->workArea.x      = 0;
    priv->workArea.y      = 0;
    priv->workArea.width  = priv->attrib.width;
    priv->workArea.height = priv->attrib.height;
    priv->grabWindow = None;

    priv->makeOutputWindow ();

    templ.visualid = XVisualIDFromVisual (priv->attrib.visual);

    visinfo = XGetVisualInfo (dpy, VisualIDMask, &templ, &nvisinfo);
    if (!nvisinfo)
    {
	compLogMessage (display, "core", CompLogLevelFatal,
			"Couldn't get visual info for default visual");
	return false;
    }

    defaultDepth = visinfo->depth;

    black.red = black.green = black.blue = 0;

    if (!XAllocColor (dpy, priv->colormap, &black))
    {
	compLogMessage (display, "core", CompLogLevelFatal,
			"Couldn't allocate color");
	XFree (visinfo);
	return false;
    }

    bitmap = XCreateBitmapFromData (dpy, priv->root, &data, 1, 1);
    if (!bitmap)
    {
	compLogMessage (display, "core", CompLogLevelFatal,
			"Couldn't create bitmap");
	XFree (visinfo);
	return false;
    }

    priv->invisibleCursor = XCreatePixmapCursor (dpy, bitmap, bitmap,
						 &black, &black, 0, 0);
    if (!priv->invisibleCursor)
    {
	compLogMessage (display, "core", CompLogLevelFatal,
			"Couldn't create invisible cursor");
	XFree (visinfo);
	return false;
    }

    XFreePixmap (dpy, bitmap);
    XFreeColors (dpy, priv->colormap, &black.pixel, 1, 0);

    glXGetConfig (dpy, visinfo, GLX_USE_GL, &value);
    if (!value)
    {
	compLogMessage (display, "core", CompLogLevelFatal,
			"Root visual is not a GL visual");
	XFree (visinfo);
	return false;
    }

    glXGetConfig (dpy, visinfo, GLX_DOUBLEBUFFER, &value);
    if (!value)
    {
	compLogMessage (display, "core", CompLogLevelFatal,
			"Root visual is not a double buffered GL visual");
	XFree (visinfo);
	return false;
    }

    priv->ctx = glXCreateContext (dpy, visinfo, NULL, !indirectRendering);
    if (!priv->ctx)
    {
	compLogMessage (display, "core", CompLogLevelFatal,
			"glXCreateContext failed");
	XFree (visinfo);

	return false;
    }

    glxExtensions = glXQueryExtensionsString (dpy, screenNum);
    if (!strstr (glxExtensions, "GLX_EXT_texture_from_pixmap"))
    {
	compLogMessage (display, "core", CompLogLevelFatal,
			"GLX_EXT_texture_from_pixmap is missing");
	XFree (visinfo);

	return false;
    }

    XFree (visinfo);

    if (!strstr (glxExtensions, "GLX_SGIX_fbconfig"))
    {
	compLogMessage (display, "core", CompLogLevelFatal,
			"GLX_SGIX_fbconfig is missing");
	return false;
    }

    priv->getProcAddress = (GLXGetProcAddressProc)
	getProcAddress ("glXGetProcAddressARB");
    bindTexImage = (GLXBindTexImageProc)
	getProcAddress ("glXBindTexImageEXT");
    releaseTexImage = (GLXReleaseTexImageProc)
	getProcAddress ("glXReleaseTexImageEXT");
    queryDrawable = (GLXQueryDrawableProc)
	getProcAddress ("glXQueryDrawable");
    getFBConfigs = (GLXGetFBConfigsProc)
	getProcAddress ("glXGetFBConfigs");
    getFBConfigAttrib = (GLXGetFBConfigAttribProc)
	getProcAddress ("glXGetFBConfigAttrib");
    createPixmap = (GLXCreatePixmapProc)
	getProcAddress ("glXCreatePixmap");

    if (!bindTexImage)
    {
	compLogMessage (display, "core", CompLogLevelFatal,
			"glXBindTexImageEXT is missing");
	return false;
    }

    if (!releaseTexImage)
    {
	compLogMessage (display, "core", CompLogLevelFatal,
			"glXReleaseTexImageEXT is missing");
	return false;
    }

    if (!queryDrawable     ||
	!getFBConfigs      ||
	!getFBConfigAttrib ||
	!createPixmap)
    {
	compLogMessage (display, "core", CompLogLevelFatal,
			"fbconfig functions missing");
	return false;
    }

    copySubBuffer = NULL;
    if (strstr (glxExtensions, "GLX_MESA_copy_sub_buffer"))
	copySubBuffer = (GLXCopySubBufferProc)
	    getProcAddress ("glXCopySubBufferMESA");

    getVideoSync = NULL;
    waitVideoSync = NULL;
    if (strstr (glxExtensions, "GLX_SGI_video_sync"))
    {
	getVideoSync = (GLXGetVideoSyncProc)
	    getProcAddress ("glXGetVideoSyncSGI");

	waitVideoSync = (GLXWaitVideoSyncProc)
	    getProcAddress ("glXWaitVideoSyncSGI");
    }

    glXMakeCurrent (dpy, priv->output, priv->ctx);
    currentRoot = priv->root;

    glExtensions = (const char *) glGetString (GL_EXTENSIONS);
    if (!glExtensions)
    {
	compLogMessage (display, "core", CompLogLevelFatal,
			"No valid GL extensions string found.");
	return false;
    }

    priv->textureNonPowerOfTwo = false;
    if (strstr (glExtensions, "GL_ARB_texture_non_power_of_two"))
	priv->textureNonPowerOfTwo = true;

    glGetIntegerv (GL_MAX_TEXTURE_SIZE, &priv->maxTextureSize);

    priv->textureRectangle = false;
    if (strstr (glExtensions, "GL_NV_texture_rectangle")  ||
	strstr (glExtensions, "GL_EXT_texture_rectangle") ||
	strstr (glExtensions, "GL_ARB_texture_rectangle"))
    {
	priv->textureRectangle = true;

	if (!priv->textureNonPowerOfTwo)
	{
	    GLint maxTextureSize;

	    glGetIntegerv (GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, &maxTextureSize);
	    if (maxTextureSize > priv->maxTextureSize)
		priv->maxTextureSize = maxTextureSize;
	}
    }

    if (!(priv->textureRectangle || priv->textureNonPowerOfTwo))
    {
	compLogMessage (display, "core", CompLogLevelFatal,
			"Support for non power of two textures missing");
	return false;
    }

    priv->textureEnvCombine = priv->textureEnvCrossbar = false;
    if (strstr (glExtensions, "GL_ARB_texture_env_combine"))
    {
	priv->textureEnvCombine = true;

	/* XXX: GL_NV_texture_env_combine4 need special code but it seams to
	   be working anyway for now... */
	if (strstr (glExtensions, "GL_ARB_texture_env_crossbar") ||
	    strstr (glExtensions, "GL_NV_texture_env_combine4"))
	    priv->textureEnvCrossbar = true;
    }

    priv->textureBorderClamp = false;
    if (strstr (glExtensions, "GL_ARB_texture_border_clamp") ||
	strstr (glExtensions, "GL_SGIS_texture_border_clamp"))
	priv->textureBorderClamp = true;

    priv->maxTextureUnits = 1;
    if (strstr (glExtensions, "GL_ARB_multitexture"))
    {
	activeTexture = (GLActiveTextureProc)
	    getProcAddress ("glActiveTexture");
	clientActiveTexture = (GLClientActiveTextureProc)
	    getProcAddress ("glClientActiveTexture");
	multiTexCoord2f = (GLMultiTexCoord2fProc)
	    getProcAddress ("glMultiTexCoord2f");

	if (activeTexture && clientActiveTexture && multiTexCoord2f)
	    glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &priv->maxTextureUnits);
    }

    priv->fragmentProgram = false;
    if (strstr (glExtensions, "GL_ARB_fragment_program"))
    {
	genPrograms = (GLGenProgramsProc)
	    getProcAddress ("glGenProgramsARB");
	deletePrograms = (GLDeleteProgramsProc)
	    getProcAddress ("glDeleteProgramsARB");
	bindProgram = (GLBindProgramProc)
	    getProcAddress ("glBindProgramARB");
	programString = (GLProgramStringProc)
	    getProcAddress ("glProgramStringARB");
	programEnvParameter4f = (GLProgramParameter4fProc)
	    getProcAddress ("glProgramEnvParameter4fARB");
	programLocalParameter4f = (GLProgramParameter4fProc)
	    getProcAddress ("glProgramLocalParameter4fARB");
	getProgramiv = (GLGetProgramivProc)
	    getProcAddress ("glGetProgramivARB");

	if (genPrograms             &&
	    deletePrograms          &&
	    bindProgram             &&
	    programString           &&
	    programEnvParameter4f   &&
	    programLocalParameter4f &&
	    getProgramiv)
	    priv->fragmentProgram = true;
    }

    priv->fbo = false;
    if (strstr (glExtensions, "GL_EXT_framebuffer_object"))
    {
	genFramebuffers = (GLGenFramebuffersProc)
	    getProcAddress ("glGenFramebuffersEXT");
	deleteFramebuffers = (GLDeleteFramebuffersProc)
	    getProcAddress ("glDeleteFramebuffersEXT");
	bindFramebuffer = (GLBindFramebufferProc)
	    getProcAddress ("glBindFramebufferEXT");
	checkFramebufferStatus = (GLCheckFramebufferStatusProc)
	    getProcAddress ("glCheckFramebufferStatusEXT");
	framebufferTexture2D = (GLFramebufferTexture2DProc)
	    getProcAddress ("glFramebufferTexture2DEXT");
	generateMipmap = (GLGenerateMipmapProc)
	    getProcAddress ("glGenerateMipmapEXT");

	if (genFramebuffers        &&
	    deleteFramebuffers     &&
	    bindFramebuffer        &&
	    checkFramebufferStatus &&
	    framebufferTexture2D   &&
	    generateMipmap)
	    priv->fbo = true;
    }

    priv->textureCompression = false;
    if (strstr (glExtensions, "GL_ARB_texture_compression"))
	priv->textureCompression = true;

    fbConfigs = (*getFBConfigs) (dpy, screenNum, &nElements);

    for (i = 0; i <= MAX_DEPTH; i++)
    {
	int j, db, stencil, depth, alpha, mipmap, rgba;

	priv->glxPixmapFBConfigs[i].fbConfig       = NULL;
	priv->glxPixmapFBConfigs[i].mipmap         = 0;
	priv->glxPixmapFBConfigs[i].yInverted      = 0;
	priv->glxPixmapFBConfigs[i].textureFormat  = 0;
	priv->glxPixmapFBConfigs[i].textureTargets = 0;

	db      = MAXSHORT;
	stencil = MAXSHORT;
	depth   = MAXSHORT;
	mipmap  = 0;
	rgba    = 0;

	for (j = 0; j < nElements; j++)
	{
	    XVisualInfo *vi;
	    int		visualDepth;

	    vi = glXGetVisualFromFBConfig (dpy, fbConfigs[j]);
	    if (vi == NULL)
		continue;

	    visualDepth = vi->depth;

	    XFree (vi);

	    if (visualDepth != i)
		continue;

	    (*getFBConfigAttrib) (dpy,
				  fbConfigs[j],
				  GLX_ALPHA_SIZE,
				  &alpha);
	    (*getFBConfigAttrib) (dpy,
				  fbConfigs[j],
				  GLX_BUFFER_SIZE,
				  &value);
	    if (value != i && (value - alpha) != i)
		continue;

	    value = 0;
	    if (i == 32)
	    {
		(*getFBConfigAttrib) (dpy,
				      fbConfigs[j],
				      GLX_BIND_TO_TEXTURE_RGBA_EXT,
				      &value);

		if (value)
		{
		    rgba = 1;

		    priv->glxPixmapFBConfigs[i].textureFormat =
			GLX_TEXTURE_FORMAT_RGBA_EXT;
		}
	    }

	    if (!value)
	    {
		if (rgba)
		    continue;

		(*getFBConfigAttrib) (dpy,
				      fbConfigs[j],
				      GLX_BIND_TO_TEXTURE_RGB_EXT,
				      &value);
		if (!value)
		    continue;

		priv->glxPixmapFBConfigs[i].textureFormat =
		    GLX_TEXTURE_FORMAT_RGB_EXT;
	    }

	    (*getFBConfigAttrib) (dpy,
				  fbConfigs[j],
				  GLX_DOUBLEBUFFER,
				  &value);
	    if (value > db)
		continue;

	    db = value;

	    (*getFBConfigAttrib) (dpy,
				  fbConfigs[j],
				  GLX_STENCIL_SIZE,
				  &value);
	    if (value > stencil)
		continue;

	    stencil = value;

	    (*getFBConfigAttrib) (dpy,
				  fbConfigs[j],
				  GLX_DEPTH_SIZE,
				  &value);
	    if (value > depth)
		continue;

	    depth = value;

	    if (priv->fbo)
	    {
		(*getFBConfigAttrib) (dpy,
				      fbConfigs[j],
				      GLX_BIND_TO_MIPMAP_TEXTURE_EXT,
				      &value);
		if (value < mipmap)
		    continue;

		mipmap = value;
	    }

	    (*getFBConfigAttrib) (dpy,
				  fbConfigs[j],
				  GLX_Y_INVERTED_EXT,
				  &value);

	    priv->glxPixmapFBConfigs[i].yInverted = value;

	    (*getFBConfigAttrib) (dpy,
				  fbConfigs[j],
				  GLX_BIND_TO_TEXTURE_TARGETS_EXT,
				  &value);

	    priv->glxPixmapFBConfigs[i].textureTargets = value;

	    priv->glxPixmapFBConfigs[i].fbConfig = fbConfigs[j];
	    priv->glxPixmapFBConfigs[i].mipmap   = mipmap;
	}
    }

    if (nElements)
	XFree (fbConfigs);

    if (!priv->glxPixmapFBConfigs[defaultDepth].fbConfig)
    {
	compLogMessage (display, "core", CompLogLevelFatal,
			"No GLXFBConfig for default depth, "
			"this isn't going to work.");
	return false;
    }

    glClearColor (0.0, 0.0, 0.0, 1.0);
    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable (GL_CULL_FACE);
    glDisable (GL_BLEND);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glColor4usv (defaultColor);
    glEnableClientState (GL_VERTEX_ARRAY);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);

    priv->canDoSaturated = priv->canDoSlightlySaturated = false;
    if (priv->textureEnvCombine && priv->maxTextureUnits >= 2)
    {
	priv->canDoSaturated = true;
	if (priv->textureEnvCrossbar && priv->maxTextureUnits >= 4)
	    priv->canDoSlightlySaturated = true;
    }

    priv->reshape (priv->attrib.width, priv->attrib.height);

    detectRefreshRate ();
    priv->detectOutputDevices ();
    priv->updateOutputDevices ();

    glLightModelfv (GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    glEnable (GL_LIGHT0);
    glLightfv (GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv (GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv (GL_LIGHT0, GL_POSITION, light0Position);

    glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    glNormal3f (0.0f, 0.0f, -1.0f);

    priv->lighting	      = false;
    priv->slowAnimations = false;

    priv->display->addScreenActions (this);

    priv->getDesktopHints ();

    display->addChild (this);

    /* TODO: bailout properly when objectInitPlugins fails */
    assert (CompPlugin::objectInitPlugins (this));



    XQueryTree (dpy, priv->root,
		&rootReturn, &parentReturn,
		&children, &nchildren);

    for (i = 0; i < nchildren; i++)
	new CompWindow (this, children[i], i ? children[i - 1] : 0);

    foreach (CompWindow *w, priv->windows)
    {
	if (w->attrib ().map_state == IsViewable)
	{
	    w->setActiveNum (priv->activeNum++);
	}
    }

    XFree (children);

    attrib.override_redirect = 1;
    attrib.event_mask	     = PropertyChangeMask;

    priv->grabWindow = XCreateWindow (dpy, priv->root, -100, -100, 1, 1, 0,
				      CopyFromParent, InputOnly, CopyFromParent,
				      CWOverrideRedirect | CWEventMask,
				      &attrib);
    XMapWindow (dpy, priv->grabWindow);

    for (i = 0; i < SCREEN_EDGE_NUM; i++)
    {
	long xdndVersion = 3;

	priv->screenEdge[i].id = XCreateWindow (dpy, priv->root, -100, -100, 1, 1, 0,
					        CopyFromParent, InputOnly,
					        CopyFromParent, CWOverrideRedirect,
					        &attrib);

	XChangeProperty (dpy, priv->screenEdge[i].id, display->atoms ().xdndAware,
			 XA_ATOM, 32, PropModeReplace,
			 (unsigned char *) &xdndVersion, 1);

	XSelectInput (dpy, priv->screenEdge[i].id,
		      EnterWindowMask   |
		      LeaveWindowMask   |
		      ButtonPressMask   |
		      ButtonReleaseMask |
		      PointerMotionMask);
    }

    priv->updateScreenEdges ();

    priv->setDesktopHints ();
    priv->setSupportingWmCheck ();
    priv->setSupported ();

    priv->normalCursor = XCreateFontCursor (dpy, XC_left_ptr);
    priv->busyCursor   = XCreateFontCursor (dpy, XC_watch);

    XDefineCursor (dpy, priv->root, priv->normalCursor);

    priv->filter[NOTHING_TRANS_FILTER] = CompTexture::Fast;
    priv->filter[SCREEN_TRANS_FILTER]  = CompTexture::Good;
    priv->filter[WINDOW_TRANS_FILTER]  = CompTexture::Good;

    priv->paintTimer.start (boost::bind(&CompScreen::handlePaintTimeout, this),
			    priv->optimalRedrawTime, MAXSHORT);
    return true;
}

CompScreen::~CompScreen ()
{
    priv->paintTimer.stop ();

    while (!priv->windows.empty ())
	delete priv->windows.front ();

    CompPlugin::objectFiniPlugins (this);

    XUngrabKey (priv->display->dpy (), AnyKey, AnyModifier, priv->root);

    for (int i = 0; i < SCREEN_EDGE_NUM; i++)
	XDestroyWindow (priv->display->dpy (), priv->screenEdge[i].id);

    XDestroyWindow (priv->display->dpy (), priv->grabWindow);

    if (priv->defaultIcon)
	delete priv->defaultIcon;

    glXDestroyContext (priv->display->dpy (), priv->ctx);

    XFreeCursor (priv->display->dpy (), priv->invisibleCursor);

#ifdef USE_COW
    if (useCow)
	XCompositeReleaseOverlayWindow (priv->display->dpy (), priv->root);
#endif

    if (priv->clientList)
	free (priv->clientList);

    if (priv->desktopHintData)
	free (priv->desktopHintData);

    if (priv->snContext)
	sn_monitor_context_unref (priv->snContext);

    if (priv->damage)
	XDestroyRegion (priv->damage);

    delete priv;
}

CompString
CompScreen::name ()
{
    char tmp[256];

    snprintf (tmp, 256, "%d", priv->screenNum);

    return CompString (tmp);

}

void
CompScreen::damageRegion (Region region)
{
    if (priv->damageMask & COMP_SCREEN_DAMAGE_ALL_MASK)
	return;

    if (priv->damageMask == 0)
	priv->paintTimer.setTimes (priv->paintTimer.minLeft ());

    XUnionRegion (priv->damage, region, priv->damage);

    priv->damageMask |= COMP_SCREEN_DAMAGE_REGION_MASK;

}

void
CompScreen::damageScreen ()
{
    if (priv->damageMask == 0)
	priv->paintTimer.setTimes (priv->paintTimer.minLeft ());

    priv->damageMask |= COMP_SCREEN_DAMAGE_ALL_MASK;
    priv->damageMask &= ~COMP_SCREEN_DAMAGE_REGION_MASK;
}

void
CompScreen::damagePending ()
{
    if (priv->damageMask == 0)
	priv->paintTimer.setTimes (priv->paintTimer.minLeft ());

    priv->damageMask |= COMP_SCREEN_DAMAGE_PENDING_MASK;
}

void
CompScreen::forEachWindow (ForEachWindowProc proc, void *closure)
{
    foreach (CompWindow *w, priv->windows)
	(*proc) (w, closure);
}

void
CompScreen::focusDefaultWindow ()
{
    CompDisplay *d = priv->display;
    CompWindow  *w;
    CompWindow  *focus = NULL;

    if (!d->getOption ("click_to_focus")->value ().b ())
    {
	w = d->findTopLevelWindow (d->below ());
	if (w && !(w->type () & (CompWindowTypeDesktopMask |
				 CompWindowTypeDockMask)))
	{
	    if (w->focus ())
		focus = w;
	}
    }

    if (!focus)
    {
	for (CompWindowList::reverse_iterator rit = priv->windows.rbegin ();
	     rit != priv->windows.rend (); rit++)
	{
	    w = (*rit);

	    if (w->type () & CompWindowTypeDockMask)
		continue;

	    if (w->focus ())
	    {
		if (focus)
		{
		    if (w->type () & (CompWindowTypeNormalMask |
				      CompWindowTypeDialogMask |
				      CompWindowTypeModalDialogMask))
		    {
			if (CompWindow::compareWindowActiveness (focus, w) < 0)
			    focus = w;
		    }
		}
		else
		    focus = w;
	    }
	}
    }

    if (focus)
    {
	if (focus->id () != d->activeWindow ())
	    focus->moveInputFocusTo ();
    }
    else
    {
	XSetInputFocus (d->dpy (), priv->root, RevertToPointerRoot,
			CurrentTime);
    }
}

CompWindow *
CompScreen::findWindow (Window id)
{
    if (lastFoundWindow && lastFoundWindow->id () == id)
    {
	return lastFoundWindow;
    }
    else
    {
	foreach (CompWindow *w, priv->windows)
	    if (w->id () == id)
		return (lastFoundWindow = w);
    }

    return 0;
}

CompWindow *
CompScreen::findTopLevelWindow (Window id)
{
    CompWindow *w;

    w = findWindow (id);
    if (!w)
	return NULL;

    if (w->attrib ().override_redirect)
    {
	/* likely a frame window */
	if (w->attrib ().c_class == InputOnly)
	{
	    foreach (w, priv->windows)
		if (w->frame () == id)
		    return w;
	}

	return NULL;
    }

    return w;
}

void
CompScreen::insertWindow (CompWindow *w, Window	aboveId)
{
    if (!aboveId || priv->windows.empty ())
    {
	if (!priv->windows.empty ())
	{
	    priv->windows.front ()->prev = w;
	    w->next = priv->windows.front ();
	}
	priv->windows.push_front (w);

	return;
    }

    CompWindowList::iterator it = priv->windows.begin ();

    while (it != priv->windows.end ())
    {
	if ((*it)->id () == aboveId)
	    break;
	it++;
    }

    if (it == priv->windows.end ())
    {
#ifdef DEBUG
	abort ();
#endif
	return;
    }

    w->next = (*it)->next;
    w->prev = (*it);
    (*it)->next = w;

    if (w->next)
    {
	w->next->prev = w;
    }

    priv->windows.insert (++it, w);
}

void
CompScreen::unhookWindow (CompWindow *w)
{
    CompWindow *prev, *next;
    CompWindowList::iterator it =
	std::find (priv->windows.begin (), priv->windows.end (), w);

    priv->windows.erase (it);

    if (w->next)
	w->next->prev = w->prev;

    if (w->prev)
	w->prev->next = w->next;

    w->next = NULL;
    w->prev = NULL;

    if (w == lastFoundWindow)
	lastFoundWindow = NULL;
    if (w == lastDamagedWindow)
	lastDamagedWindow = NULL;
}

#define POINTER_GRAB_MASK (ButtonReleaseMask | \
			   ButtonPressMask   | \
			   PointerMotionMask)
CompScreen::grabHandle
CompScreen::pushGrab (Cursor cursor, const char *name)
{
    if (priv->grabs.empty())
    {
	int status;

	status = XGrabPointer (priv->display->dpy (), priv->grabWindow, TRUE,
			       POINTER_GRAB_MASK,
			       GrabModeAsync, GrabModeAsync,
			       priv->root, cursor,
			       CurrentTime);

	if (status == GrabSuccess)
	{
	    status = XGrabKeyboard (priv->display->dpy (),
				    priv->grabWindow, TRUE,
				    GrabModeAsync, GrabModeAsync,
				    CurrentTime);
	    if (status != GrabSuccess)
	    {
		XUngrabPointer (priv->display->dpy (), CurrentTime);
		return NULL;
	    }
	}
	else
	    return NULL;
    }
    else
    {
	XChangeActivePointerGrab (priv->display->dpy (), POINTER_GRAB_MASK,
				  cursor, CurrentTime);
    }

    PrivateScreen::Grab *grab = new PrivateScreen::Grab ();
    grab->cursor = cursor;
    grab->name   = name;

    priv->grabs.push_back (grab);

    return grab;
}

void
CompScreen::updateGrab (CompScreen::grabHandle handle, Cursor cursor)
{
    if (!handle)
	return;

    XChangeActivePointerGrab (priv->display->dpy (), POINTER_GRAB_MASK,
			      cursor, CurrentTime);

    ((PrivateScreen::Grab *) handle)->cursor = cursor;
}

void
CompScreen::removeGrab (CompScreen::grabHandle handle,
			CompPoint *restorePointer)
{
    if (!handle)
	return;

    std::list<PrivateScreen::Grab *>::iterator it;

    it = std::find (priv->grabs.begin (), priv->grabs.end (), handle);

    if (it != priv->grabs.end ())
    {
	priv->grabs.erase (it);
	delete (static_cast<PrivateScreen::Grab *> (handle));
    }
    if (!priv->grabs.empty ())
    {
	XChangeActivePointerGrab (priv->display->dpy (),
				  POINTER_GRAB_MASK,
				  priv->grabs.back ()->cursor,
				  CurrentTime);
    }
    else
    {
	    if (restorePointer)
		warpPointer (restorePointer->x () - pointerX,
			     restorePointer->y () - pointerY);

	    XUngrabPointer (priv->display->dpy (), CurrentTime);
	    XUngrabKeyboard (priv->display->dpy (), CurrentTime);
    }
}

/* otherScreenGrabExist takes a series of strings terminated by a NULL.
   It returns TRUE if a grab exists but it is NOT held by one of the
   plugins listed, returns FALSE otherwise. */

bool
CompScreen::otherGrabExist (const char *first, ...)
{
    va_list    ap;
    const char *name;

    std::list<PrivateScreen::Grab *>::iterator it;

    for (it = priv->grabs.begin (); it != priv->grabs.end (); it++)
    {
	va_start (ap, first);

	name = first;
	while (name)
	{
	    if (strcmp (name, (*it)->name) == 0)
		break;

	    name = va_arg (ap, const char *);
	}

	va_end (ap);

	if (!name)
	return true;
    }

    return false;
}

void
PrivateScreen::grabUngrabOneKey (unsigned int modifiers,
				 int          keycode,
				 bool         grab)
{
    if (grab)
    {
	XGrabKey (display->dpy (),
		  keycode,
		  modifiers,
		  root,
		  TRUE,
		  GrabModeAsync,
		  GrabModeAsync);
    }
    else
    {
	XUngrabKey (display->dpy (),
		    keycode,
		    modifiers,
		    root);
    }
}

bool
PrivateScreen::grabUngrabKeys (unsigned int modifiers,
			       int          keycode,
			       bool         grab)
{
    XModifierKeymap *modMap = display->modMap ();
    int ignore, mod, k;

    compCheckForError (display->dpy ());

    for (ignore = 0; ignore <= display->ignoredModMask (); ignore++)
    {
	if (ignore & ~display->ignoredModMask ())
	    continue;

	if (keycode != 0)
	{
	    grabUngrabOneKey (modifiers | ignore, keycode, grab);
	}
	else
	{
	    for (mod = 0; mod < 8; mod++)
	    {
		if (modifiers & (1 << mod))
		{
		    for (k = mod * modMap->max_keypermod;
			 k < (mod + 1) * modMap->max_keypermod;
			 k++)
		    {
			if (modMap->modifiermap[k])
			{
			    grabUngrabOneKey ((modifiers & ~(1 << mod)) |
					      ignore,
					      modMap->modifiermap[k],
					      grab);
			}
		    }
		}
	    }
	}

	if (compCheckForError (display->dpy ()))
	    return false;
    }

    return true;
}

bool
PrivateScreen::addPassiveKeyGrab (CompAction::KeyBinding &key)
{
    KeyGrab                      newKeyGrab;
    unsigned int                 mask;
    std::list<KeyGrab>::iterator it;

    mask = display->virtualToRealModMask (key.modifiers ());

    for (it = keyGrabs.begin (); it != keyGrabs.end (); it++)
    {
	if (key.keycode () == (*it).keycode &&
	    mask           == (*it).modifiers)
	{
	    (*it).count++;
	    return true;
	}
    }



    if (!(mask & CompNoMask))
    {
	if (!grabUngrabKeys (mask, key.keycode (), true))
	    return false;
    }

    newKeyGrab.keycode   = key.keycode ();
    newKeyGrab.modifiers = mask;
    newKeyGrab.count     = 1;

    keyGrabs.push_back (newKeyGrab);

    return true;
}

void
PrivateScreen::removePassiveKeyGrab (CompAction::KeyBinding &key)
{
    unsigned int                 mask;
    std::list<KeyGrab>::iterator it;

    mask = display->virtualToRealModMask (key.modifiers ());

    for (it = keyGrabs.begin (); it != keyGrabs.end (); it++)
    {
	if (key.keycode () == (*it).keycode &&
	    mask           == (*it).modifiers)
	{
	    (*it).count--;
	    if ((*it).count)
		return;

	    it = keyGrabs.erase (it);

	    if (!(mask & CompNoMask))
		grabUngrabKeys (mask, key.keycode (), false);
	}
    }
}

void
PrivateScreen::updatePassiveKeyGrabs ()
{
    std::list<KeyGrab>::iterator it;

    XUngrabKey (display->dpy (), AnyKey, AnyModifier, root);

    for (it = keyGrabs.begin (); it != keyGrabs.end (); it++)
    {
	if (!((*it).modifiers & CompNoMask))
	{
	    grabUngrabKeys ((*it).modifiers,
			    (*it).keycode, true);
	}
    }
}

bool
PrivateScreen::addPassiveButtonGrab (CompAction::ButtonBinding &button)
{
    ButtonGrab                      newButtonGrab;
    std::list<ButtonGrab>::iterator it;

    for (it = buttonGrabs.begin (); it != buttonGrabs.end (); it++)
    {
	if (button.button ()    == (*it).button &&
	    button.modifiers () == (*it).modifiers)
	{
	    (*it).count++;
	    return true;
	}
    }

    newButtonGrab.button    = button.button ();
    newButtonGrab.modifiers = button.modifiers ();
    newButtonGrab.count     = 1;

    buttonGrabs.push_back (newButtonGrab);

    return true;
}

void
PrivateScreen::removePassiveButtonGrab (CompAction::ButtonBinding &button)
{
    std::list<ButtonGrab>::iterator it;

    for (it = buttonGrabs.begin (); it != buttonGrabs.end (); it++)
    {
	if (button.button ()    == (*it).button &&
	    button.modifiers () == (*it).modifiers)
	{
	    (*it).count--;
	    if ((*it).count)
		return;

	    it = buttonGrabs.erase (it);
	}
    }
}

bool
CompScreen::addAction (CompAction *action)
{
    if (action->type () & CompAction::BindingTypeKey)
    {
	if (!priv->addPassiveKeyGrab (action->key ()))
	    return true;
    }

    if (action->type () & CompAction::BindingTypeButton)
    {
	if (!priv->addPassiveButtonGrab (action->button ()))
	{
	    if (action->type () & CompAction::BindingTypeKey)
		priv->removePassiveKeyGrab (action->key ());

	    return true;
	}
    }

    if (action->edgeMask ())
    {
	int i;

	for (i = 0; i < SCREEN_EDGE_NUM; i++)
	    if (action->edgeMask () & (1 << i))
		enableEdge (i);
    }

    return true;
}

void
CompScreen::removeAction (CompAction *action)
{
    if (action->type () & CompAction::BindingTypeKey)
	priv->removePassiveKeyGrab (action->key ());

    if (action->type () & CompAction::BindingTypeButton)
	priv->removePassiveButtonGrab (action->button ());

    if (action->edgeMask ())
    {
	int i;

	for (i = 0; i < SCREEN_EDGE_NUM; i++)
	    if (action->edgeMask () & (1 << i))
		disableEdge (i);
    }
}

void
CompScreen::updatePassiveGrabs ()
{
    priv->updatePassiveKeyGrabs ();
}

void
PrivateScreen::computeWorkareaForBox (BoxPtr     pBox,
				      XRectangle *area)
{
    Region     region;
    REGION     r;
    int	       x1, y1, x2, y2;

    region = XCreateRegion ();
    if (!region)
    {
	area->x      = pBox->x1;
	area->y      = pBox->y1;
	area->width  = pBox->x1 - pBox->x1;
	area->height = pBox->y2 - pBox->y1;

	return;
    }

    r.rects    = &r.extents;
    r.numRects = r.size = 1;
    r.extents  = *pBox;

    XUnionRegion (&r, region, region);

    foreach (CompWindow *w, windows)
    {
	if (!w->mapNum ())
	    continue;

	if (w->struts ())
	{
	    r.extents.y1 = pBox->y1;
	    r.extents.y2 = pBox->y2;

	    x1 = w->struts ()->left.x;
	    y1 = w->struts ()->left.y;
	    x2 = x1 + w->struts ()->left.width;
	    y2 = y1 + w->struts ()->left.height;

	    if (y1 < pBox->y2 && y2 > pBox->y1)
	    {
		r.extents.x1 = x1;
		r.extents.x2 = x2;

		XSubtractRegion (region, &r, region);
	    }

	    x1 = w->struts ()->right.x;
	    y1 = w->struts ()->right.y;
	    x2 = x1 + w->struts ()->right.width;
	    y2 = y1 + w->struts ()->right.height;

	    if (y1 < pBox->y2 && y2 > pBox->y1)
	    {
		r.extents.x1 = x1;
		r.extents.x2 = x2;

		XSubtractRegion (region, &r, region);
	    }

	    r.extents.x1 = pBox->x1;
	    r.extents.x2 = pBox->x2;

	    x1 = w->struts ()->top.x;
	    y1 = w->struts ()->top.y;
	    x2 = x1 + w->struts ()->top.width;
	    y2 = y1 + w->struts ()->top.height;

	    if (x1 < pBox->x2 && x2 > pBox->x1)
	    {
		r.extents.y1 = y1;
		r.extents.y2 = y2;

		XSubtractRegion (region, &r, region);
	    }

	    x1 = w->struts ()->bottom.x;
	    y1 = w->struts ()->bottom.y;
	    x2 = x1 + w->struts ()->bottom.width;
	    y2 = y1 + w->struts ()->bottom.height;

	    if (x1 < pBox->x2 && x2 > pBox->x1)
	    {
		r.extents.y1 = y1;
		r.extents.y2 = y2;

		XSubtractRegion (region, &r, region);
	    }
	}
    }

    area->x      = region->extents.x1;
    area->y      = region->extents.y1;
    area->width  = region->extents.x2 - region->extents.x1;
    area->height = region->extents.y2 - region->extents.y1;

    XDestroyRegion (region);
}

void
CompScreen::updateWorkarea ()
{
    XRectangle workArea;
    BoxRec     box;
    bool       workAreaChanged = false;

    for (unsigned int i = 0; i < priv->outputDevs.size (); i++)
    {
	XRectangle oldWorkArea = priv->outputDevs[i].workArea ();

	priv->computeWorkareaForBox (&priv->outputDevs[i].region ()->extents,
				     &workArea);

	if (memcmp (&workArea, &oldWorkArea, sizeof (XRectangle)))
	{
	    workAreaChanged = true;
	    priv->outputDevs[i].setWorkArea (workArea);
	}
    }

    box.x1 = 0;
    box.y1 = 0;
    box.x2 = priv->size.width ();
    box.y2 = priv->size.height ();

    priv->computeWorkareaForBox (&box, &workArea);

    if (memcmp (&workArea, &priv->workArea, sizeof (XRectangle)))
    {
	workAreaChanged = true;
	priv->workArea = workArea;

	priv->setDesktopHints ();
    }

    if (workAreaChanged)
    {
	/* as work area changed, update all maximized windows on this
	   screen to snap to the new work area */
	foreach (CompWindow *w, priv->windows)
	    w->updateSize ();
    }
}

static bool
isClientListWindow (CompWindow *w)
{
    /* windows with client id less than 2 have been destroyed and only exists
       because some plugin keeps a reference to them. they should not be in
       client lists */
    if (w->id () < 2)
	return false;

    if (w->attrib ().override_redirect)
	return false;

    if (w->attrib ().map_state != IsViewable)
    {
	if (!(w->state () & CompWindowStateHiddenMask))
	    return false;
    }

    return true;
}

static void
countClientListWindow (CompWindow *w,
		       void       *closure)
{
    if (isClientListWindow (w))
    {
	int *num = (int *) closure;

	*num = *num + 1;
    }
}

static int
compareMappingOrder (const void *w1,
		     const void *w2)
{
    return (*((CompWindow **) w1))->mapNum () -
	   (*((CompWindow **) w2))->mapNum ();
}

void
CompScreen::updateClientList ()
{
    Window *clientList;
    Window *clientListStacking;
    Bool   updateClientList = true;
    Bool   updateClientListStacking = true;
    int	   i, n = 0;

    forEachWindow (countClientListWindow, (void *) &n);

    if (n == 0)
    {
	if (n != priv->nClientList)
	{
	    free (priv->clientList);

	    priv->clientList  = NULL;
	    priv->nClientList = 0;

	    XChangeProperty (priv->display->dpy (), priv->root,
			     priv->display->atoms ().clientList,
			     XA_WINDOW, 32, PropModeReplace,
			     (unsigned char *) &priv->grabWindow, 1);
	    XChangeProperty (priv->display->dpy (), priv->root,
			     priv->display->atoms ().clientListStacking,
			     XA_WINDOW, 32, PropModeReplace,
			     (unsigned char *) &priv->grabWindow, 1);
	}

	return;
    }

    if (n != priv->nClientList)
    {
	CompWindow **list;

	list = (CompWindow **)
	    realloc (priv->clientList, (sizeof (CompWindow *) +
		     sizeof (Window) * 2) * n);
	if (!list)
	    return;

	priv->clientList  = list;
	priv->nClientList = n;

	updateClientList = updateClientListStacking = true;
    }

    clientList	       = (Window *) (priv->clientList + n);
    clientListStacking = clientList + n;

    i = 0;
    foreach (CompWindow *w, priv->windows)
	if (isClientListWindow (w))
	{
	    priv->clientList[i] = w;
	    i++;
	}

    for (i = 0; i < n; i++)
    {
	if (!updateClientListStacking)
	{
	    if (clientListStacking[i] != priv->clientList[i]->id ())
		updateClientListStacking = true;
	}

	clientListStacking[i] = priv->clientList[i]->id ();
    }

    /* sort window list in mapping order */
    qsort (priv->clientList, n, sizeof (CompWindow *), compareMappingOrder);

    for (i = 0; i < n; i++)
    {
	if (!updateClientList)
	{
	    if (clientList[i] != priv->clientList[i]->id ())
		updateClientList = true;
	}

	clientList[i] = priv->clientList[i]->id ();
    }

    if (updateClientList)
	XChangeProperty (priv->display->dpy (), priv->root,
			 priv->display->atoms ().clientList,
			 XA_WINDOW, 32, PropModeReplace,
			 (unsigned char *) clientList, priv->nClientList);

    if (updateClientListStacking)
	XChangeProperty (priv->display->dpy (), priv->root,
			 priv->display->atoms ().clientListStacking,
			 XA_WINDOW, 32, PropModeReplace,
			 (unsigned char *) clientListStacking,
			 priv->nClientList);
}

void
CompScreen::toolkitAction (Atom   toolkitAction,
			   Time   eventTime,
			   Window window,
			   long   data0,
			   long   data1,
			   long   data2)
{
    XEvent ev;

    ev.type		    = ClientMessage;
    ev.xclient.window	    = window;
    ev.xclient.message_type = priv->display->atoms ().toolkitAction;
    ev.xclient.format	    = 32;
    ev.xclient.data.l[0]    = toolkitAction;
    ev.xclient.data.l[1]    = eventTime;
    ev.xclient.data.l[2]    = data0;
    ev.xclient.data.l[3]    = data1;
    ev.xclient.data.l[4]    = data2;

    XUngrabPointer (priv->display->dpy (), CurrentTime);
    XUngrabKeyboard (priv->display->dpy (), CurrentTime);

    XSendEvent (priv->display->dpy (), priv->root, FALSE,
		StructureNotifyMask, &ev);
}

void
CompScreen::runCommand (CompString command)
{
    if (command.size () == 0)
	return;

    if (fork () == 0)
    {
	unsigned int pos;
	CompString   env = priv->display->displayString ();

	setsid ();
	
	pos = env.find (':');
	if (pos != std::string::npos)
	{
	    if (env.find ('.', pos) != std::string::npos)
	    {
		env.erase (env.find ('.', pos));
	    }
	    else
	    {
		env.erase (pos);
		env.append (":0");
	    }
	}

	env.append (compPrintf (".%d", priv->screenNum));
	
	putenv (const_cast<char *> (env.c_str ()));

	exit (execl ("/bin/sh", "/bin/sh", "-c", command.c_str (), NULL));
    }
}

void
CompScreen::moveViewport (int tx, int ty, bool sync)
{
    int         wx, wy;

    tx = priv->vp.x () - tx;
    tx = MOD (tx, priv->vpSize.width ());
    tx -= priv->vp.x ();

    ty = priv->vp.y () - ty;
    ty = MOD (ty, priv->vpSize.height ());
    ty -= priv->vp.y ();

    if (!tx && !ty)
	return;

    priv->vp.setX (priv->vp.x () + tx);
    priv->vp.setY (priv->vp.y () + ty);

    tx *= -priv->size.width ();
    ty *= -priv->size.height ();

    foreach (CompWindow *w, priv->windows)
    {
	if (w->onAllViewports ())
	    continue;

	w->getMovementForOffset (tx, ty, &wx, &wy);

	if (w->saveMask () & CWX)
	    w->saveWc ().x += wx;

	if (w->saveMask () & CWY)
	    w->saveWc ().y += wy;

	/* move */
	w->move (wx, wy, sync, true);

	if (sync)
	    w->syncPosition ();
    }

    if (sync)
    {
	CompWindow *w;

	priv->setDesktopHints ();

	setCurrentActiveWindowHistory (priv->vp.x (), priv->vp.y ());

	w = priv->display->findWindow (priv->display->activeWindow ());
	if (w)
	{
	    int x, y;

	    w->defaultViewport (&x, &y);

	    /* add window to current history if it's default viewport is
	       still the current one. */
	    if (priv->vp.x () == x && priv->vp.y () == y)
		addToCurrentActiveWindowHistory (w->id ());
	}
    }
}

CompGroup *
CompScreen::addGroup (Window id)
{
    CompGroup *group = new CompGroup ();

    group->refCnt = 1;
    group->id     = id;

    priv->groups.push_back (group);

    return group;
}

void
CompScreen::removeGroup (CompGroup *group)
{
    group->refCnt--;
    if (group->refCnt)
	return;

    std::list<CompGroup *>::iterator it =
	std::find (priv->groups.begin (), priv->groups.end (), group);

    if (it != priv->groups.end ())
    {
	priv->groups.erase (it);
    }

    delete group;
}

CompGroup *
CompScreen::findGroup (Window id)
{
    foreach (CompGroup *g, priv->groups)
	if (g->id == id)
	    return g;

    return NULL;
}

void
CompScreen::applyStartupProperties (CompWindow *window)
{
    CompStartupSequence *s = NULL;
    const char	        *startupId = window->startupId ();

    if (!startupId)
    {
	CompWindow *leader;

	leader = findWindow (window->clientLeader ());
	if (leader)
	    startupId = leader->startupId ();

	if (!startupId)
	    return;
    }

    foreach (CompStartupSequence *ss, priv->startupSequences)
    {
	const char *id;

	id = sn_startup_sequence_get_id (ss->sequence);
	if (strcmp (id, startupId) == 0)
	{
	    s = ss;
	    break;
	}
    }

    if (s)
	window->applyStartupProperties (s);
}

void
CompScreen::sendWindowActivationRequest (Window id)
{
    XEvent xev;

    xev.xclient.type    = ClientMessage;
    xev.xclient.display = priv->display->dpy ();
    xev.xclient.format  = 32;

    xev.xclient.message_type = priv->display->atoms ().winActive;
    xev.xclient.window	     = id;

    xev.xclient.data.l[0] = 2;
    xev.xclient.data.l[1] = 0;
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 0;
    xev.xclient.data.l[4] = 0;

    XSendEvent (priv->display->dpy (),
		priv->root,
		FALSE,
		SubstructureRedirectMask | SubstructureNotifyMask,
		&xev);
}

void
CompScreen::setTexEnvMode (GLenum mode)
{
    if (priv->lighting)
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    else
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
}

void
CompScreen::setLighting (bool lighting)
{
    if (priv->lighting != lighting)
    {
	if (!priv->opt[COMP_SCREEN_OPTION_LIGHTING].value ().b ())
	    lighting = false;

	if (lighting)
	{
	    glEnable (GL_COLOR_MATERIAL);
	    glEnable (GL_LIGHTING);
	}
	else
	{
	    glDisable (GL_COLOR_MATERIAL);
	    glDisable (GL_LIGHTING);
	}

	priv->lighting = lighting;

	setTexEnvMode (GL_REPLACE);
    }
}

void
CompScreen::enableEdge (int edge)
{
    priv->screenEdge[edge].count++;
    if (priv->screenEdge[edge].count == 1)
	XMapRaised (priv->display->dpy (), priv->screenEdge[edge].id);
}

void
CompScreen::disableEdge (int edge)
{
    priv->screenEdge[edge].count--;
    if (priv->screenEdge[edge].count == 0)
	XUnmapWindow (priv->display->dpy (), priv->screenEdge[edge].id);
}

Window
CompScreen::getTopWindow ()
{
    /* return first window that has not been destroyed */
    for (CompWindowList::reverse_iterator rit = priv->windows.rbegin ();
	     rit != priv->windows.rend (); rit++)
    {
	if ((*rit)->id () > 1)
	    return (*rit)->id ();
    }

    return None;
}

void
CompScreen::makeCurrent ()
{
    if (currentRoot != priv->root)
    {
	glXMakeCurrent (priv->display->dpy (), priv->output, priv->ctx);
	currentRoot = priv->root;
    }

    priv->pendingCommands = true;
}

void
CompScreen::finishDrawing ()
{
    if (priv->pendingCommands)
    {
	makeCurrent ();
	glFinish ();

	priv->pendingCommands = true;
    }
}

int
CompScreen::outputDeviceForPoint (int x, int y)
{
    return outputDeviceForGeometry (CompWindow::Geometry (x, y, 1, 1, 0));
}

void
CompScreen::getCurrentOutputExtents (int *x1, int *y1, int *x2, int *y2)
{
    if (x1)
	*x1 = priv->outputDevs[priv->currentOutputDev].x1 ();

    if (y1)
	*y1 = priv->outputDevs[priv->currentOutputDev].y1 ();

    if (x2)
	*x2 = priv->outputDevs[priv->currentOutputDev].x2 ();

    if (y2)
	*y2 = priv->outputDevs[priv->currentOutputDev].y2 ();
}

void
CompScreen::setNumberOfDesktops (unsigned int nDesktop)
{
    if (nDesktop < 1 || nDesktop >= 0xffffffff)
	return;

    if (nDesktop == priv->nDesktop)
	return;

    if (priv->currentDesktop >= nDesktop)
	priv->currentDesktop = nDesktop - 1;

    foreach (CompWindow *w, priv->windows)
    {
	if (w->desktop () == 0xffffffff)
	    continue;

	if (w->desktop () >= nDesktop)
	    w->setDesktop (nDesktop - 1);
    }

    priv->nDesktop = nDesktop;

    priv->setDesktopHints ();
}

void
CompScreen::setCurrentDesktop (unsigned int desktop)
{
    unsigned long data;

    if (desktop >= priv->nDesktop)
	return;

    if (desktop == priv->currentDesktop)
	return;

    priv->currentDesktop = desktop;

    foreach (CompWindow *w, priv->windows)
    {
	if (w->desktop () == 0xffffffff)
	    continue;

	if (w->desktop () == desktop)
	    w->show ();
	else
	    w->hide ();
    }

    data = desktop;

    XChangeProperty (priv->display->dpy (), priv->root,
		     priv->display->atoms ().currentDesktop,
		     XA_CARDINAL, 32, PropModeReplace,
		     (unsigned char *) &data, 1);
}

void
CompScreen::getWorkareaForOutput (int output, XRectangle *area)
{
    *area = priv->outputDevs[output].workArea ();
}

void
CompScreen::setDefaultViewport ()
{
    priv->lastViewport.x      = priv->outputDevs[0].x1 ();
    priv->lastViewport.y      = priv->size.height () - priv->outputDevs[0].y2 ();
    priv->lastViewport.width  = priv->outputDevs[0].width ();
    priv->lastViewport.height = priv->outputDevs[0].height ();

    glViewport (priv->lastViewport.x,
		priv->lastViewport.y,
		priv->lastViewport.width,
		priv->lastViewport.height);
}

void
CompScreen::outputChangeNotify ()
    WRAPABLE_HND_FUNC(outputChangeNotify)

void
CompScreen::clearOutput (CompOutput   *output,
			 unsigned int mask)
{
    BoxPtr pBox = &output->region ()->extents;

    if (pBox->x1 != 0	     ||
	pBox->y1 != 0	     ||
	pBox->x2 != priv->size.width () ||
	pBox->y2 != priv->size.height ())
    {
	glPushAttrib (GL_SCISSOR_BIT);

	glEnable (GL_SCISSOR_TEST);
	glScissor (pBox->x1,
		   priv->size.height () - pBox->y2,
		   pBox->x2 - pBox->x1,
		   pBox->y2 - pBox->y1);
	glClear (mask);

	glPopAttrib ();
    }
    else
    {
	glClear (mask);
    }
}

/* Returns default viewport for some window geometry. If the window spans
   more than one viewport the most appropriate viewport is returned. How the
   most appropriate viewport is computed can be made optional if necessary. It
   is currently computed as the viewport where the center of the window is
   located, except for when the window is visible in the current viewport as
   the current viewport is then always returned. */
void
CompScreen::viewportForGeometry (CompWindow::Geometry gm,
				 int                  *viewportX,
				 int                  *viewportY)
{
    int	centerX;
    int	centerY;

    gm.setWidth  (gm.width () + (gm.border () * 2));
    gm.setHeight (gm.height () + (gm.border () * 2));

    if ((gm.x () < priv->size.width ()  && gm.x () + gm.width () > 0) &&
	(gm.y () < priv->size.height () && gm.y ()+ gm.height () > 0))
    {
	if (viewportX)
	    *viewportX = priv->vp.x ();

	if (viewportY)
	    *viewportY = priv->vp.y ();

	return;
    }

    if (viewportX)
    {
	centerX = gm.x () + (gm.width () >> 1);
	if (centerX < 0)
	    *viewportX = priv->vp.x () + ((centerX / priv->size.width ()) - 1) %
		priv->vpSize.width ();
	else
	    *viewportX = priv->vp.x () + (centerX / priv->size.width ()) %
		priv->vpSize.width ();
    }

    if (viewportY)
    {
	centerY = gm.y () + (gm.height () >> 1);
	if (centerY < 0)
	    *viewportY = priv->vp.y () +
		((centerY / priv->size.height ()) - 1) % priv->vpSize.height ();
	else
	    *viewportY = priv->vp.y () + (centerY / priv->size.height ()) %
		priv->vpSize.height ();
    }
}

static int
rectangleOverlapArea (BOX *rect1,
		      BOX *rect2)
{
    int left, right, top, bottom;

    /* extents of overlapping rectangle */
    left = MAX (rect1->x1, rect2->x1);
    right = MIN (rect1->x2, rect2->x2);
    top = MAX (rect1->y1, rect2->y1);
    bottom = MIN (rect1->y2, rect2->y2);

    if (left > right || top > bottom)
    {
	/* no overlap */
	return 0;
    }

    return (right - left) * (bottom - top);
}

int
CompScreen::outputDeviceForGeometry (CompWindow::Geometry gm)
{
    int          overlapAreas[priv->outputDevs.size ()];
    int          highest, seen, highestScore;
    int          strategy;
    unsigned int i;
    BOX          geomRect;

    if (priv->outputDevs.size () == 1)
	return 0;

    strategy = priv->opt[COMP_SCREEN_OPTION_OVERLAPPING_OUTPUTS].value ().i ();

    if (strategy == OUTPUT_OVERLAP_MODE_SMART)
    {
	/* for smart mode, calculate the overlap of the whole rectangle
	   with the output device rectangle */
	geomRect.x2 = gm.width () + 2 * gm.border ();
	geomRect.y2 = gm.height () + 2 * gm.border ();

	geomRect.x1 = gm.x () % priv->size.width ();
	if ((geomRect.x1 + geomRect.x2 / 2) < 0)
	    geomRect.x1 += priv->size.width ();
	geomRect.y1 = gm.y () % priv->size.height ();
	if ((geomRect.y1 + geomRect.y2 / 2) < 0)
	    geomRect.y1 += priv->size.height ();

	geomRect.x2 += geomRect.x1;
	geomRect.y2 += geomRect.y1;
    }
    else
    {
	/* for biggest/smallest modes, only use the window center to determine
	   the correct output device */
	geomRect.x1 = (gm.x () + (gm.width () / 2) + gm.border ()) % priv->size.width ();
	if (geomRect.x1 < 0)
	    geomRect.x1 += priv->size.width ();
	geomRect.y1 = (gm.y () + (gm.height () / 2) + gm.border()) % priv->size.height ();
	if (geomRect.y1 < 0)
	    geomRect.y1 += priv->size.height ();

	geomRect.x2 = geomRect.x1 + 1;
	geomRect.y2 = geomRect.y1 + 1;
    }

    /* get amount of overlap on all output devices */
    for (i = 0; i < priv->outputDevs.size (); i++)
	overlapAreas[i] =
		rectangleOverlapArea (&priv->outputDevs[i].region ()->extents,
				      &geomRect);

    /* find output with largest overlap */
    for (i = 0, highest = 0, highestScore = 0; i < priv->outputDevs.size (); i++)
	if (overlapAreas[i] > highestScore)
	{
	    highest = i;
	    highestScore = overlapAreas[i];
	}

    /* look if the highest score is unique */
    for (i = 0, seen = 0; i < priv->outputDevs.size (); i++)
	if (overlapAreas[i] == highestScore)
	    seen++;

    if (seen > 1)
    {
	/* it's not unique, select one output of the matching ones and use the
	   user preferred strategy for that */
	unsigned int currentSize, bestOutputSize;
	Bool         searchLargest;
	
	searchLargest = (strategy != OUTPUT_OVERLAP_MODE_PREFER_SMALLER);
	if (searchLargest)
	    bestOutputSize = 0;
	else
	    bestOutputSize = UINT_MAX;

	for (i = 0, highest = 0; i < priv->outputDevs.size (); i++)
	    if (overlapAreas[i] == highestScore)
	    {
		BOX  *box = &priv->outputDevs[i].region ()->extents;
		Bool bestFit;

		currentSize = (box->x2 - box->x1) * (box->y2 - box->y1);

		if (searchLargest)
		    bestFit = (currentSize > bestOutputSize);
		else
		    bestFit = (currentSize < bestOutputSize);

		if (bestFit)
		{
		    highest = i;
		    bestOutputSize = currentSize;
		}
	    }
    }

    return highest;
}

bool
CompScreen::updateDefaultIcon ()
{
    CompString file = priv->opt[COMP_SCREEN_OPTION_DEFAULT_ICON].value ().s ();
    void       *data;
    int        width, height;

    if (priv->defaultIcon)
    {
	delete priv->defaultIcon;
	priv->defaultIcon = NULL;
    }

    if (!priv->display->readImageFromFile (file.c_str (), &width,
					   &height, &data))
	return false;

    priv->defaultIcon = new CompIcon (this, width, height);

    memcpy (priv->defaultIcon->data (), data, width * height * sizeof (CARD32));

    free (data);

    return true;
}

void
CompScreen::setCurrentActiveWindowHistory (int x, int y)
{
    int	i, min = 0;

    for (i = 0; i < ACTIVE_WINDOW_HISTORY_NUM; i++)
    {
	if (priv->history[i].x == x && priv->history[i].y == y)
	{
	    priv->currentHistory = i;
	    return;
	}
    }

    for (i = 1; i < ACTIVE_WINDOW_HISTORY_NUM; i++)
	if (priv->history[i].activeNum < priv->history[min].activeNum)
	    min = i;

    priv->currentHistory = min;

    priv->history[min].activeNum = priv->activeNum;
    priv->history[min].x         = x;
    priv->history[min].y         = y;

    memset (priv->history[min].id, 0, sizeof (priv->history[min].id));
}

void
CompScreen::addToCurrentActiveWindowHistory (Window id)
{
    CompActiveWindowHistory *history = &priv->history[priv->currentHistory];
    Window		    tmp, next = id;
    int			    i;

    /* walk and move history */
    for (i = 0; i < ACTIVE_WINDOW_HISTORY_SIZE; i++)
    {
	tmp = history->id[i];
	history->id[i] = next;
	next = tmp;

	/* we're done when we find an old instance or an empty slot */
	if (tmp == id || tmp == None)
	    break;
    }

    history->activeNum = priv->activeNum;
}

void
CompScreen::setWindowPaintOffset (int x, int y)
{
    priv->windowPaintOffset = CompPoint (x, y);
}


ScreenInterface::ScreenInterface ()
{
    WRAPABLE_INIT_FUNC(preparePaint);
    WRAPABLE_INIT_FUNC(donePaint);
    WRAPABLE_INIT_FUNC(paint);
    WRAPABLE_INIT_FUNC(paintOutput);
    WRAPABLE_INIT_FUNC(paintTransformedOutput);
    WRAPABLE_INIT_FUNC(enableOutputClipping);
    WRAPABLE_INIT_FUNC(disableOutputClipping);
    WRAPABLE_INIT_FUNC(applyTransform);

    WRAPABLE_INIT_FUNC(enterShowDesktopMode);
    WRAPABLE_INIT_FUNC(leaveShowDesktopMode);

    WRAPABLE_INIT_FUNC(outputChangeNotify);

    WRAPABLE_INIT_FUNC(getWindowPaintList);
}

void
ScreenInterface::preparePaint (int msSinceLastPaint)
    WRAPABLE_DEF_FUNC(preparePaint, msSinceLastPaint)

void
ScreenInterface::donePaint ()
    WRAPABLE_DEF_FUNC(donePaint)

void
ScreenInterface::paint (CompOutput::ptrList &outputs,
			unsigned int        mask)
    WRAPABLE_DEF_FUNC(paint, outputs, mask)

bool
ScreenInterface::paintOutput (const CompScreenPaintAttrib *sAttrib,
			      const CompTransform         *transform,
			      Region	                  region,
			      CompOutput                  *output,
			      unsigned int                mask)
    WRAPABLE_DEF_FUNC_RETURN(paintOutput, sAttrib, transform, region,
			     output, mask)

void
ScreenInterface::paintTransformedOutput (const CompScreenPaintAttrib *sAttrib,
					 const CompTransform         *transform,
					 Region                      region,
					 CompOutput                  *output,
					 unsigned int                mask)
    WRAPABLE_DEF_FUNC(paintTransformedOutput, sAttrib, transform, region,
		      output, mask)

void
ScreenInterface::applyTransform (const CompScreenPaintAttrib *sAttrib,
				 CompOutput                  *output,
				 CompTransform               *transform)
    WRAPABLE_DEF_FUNC(applyTransform, sAttrib, output, transform)

void
ScreenInterface::enableOutputClipping (const CompTransform *transform,
				       Region              region,
				       CompOutput          *output)
    WRAPABLE_DEF_FUNC(enableOutputClipping, transform, region, output)

void
ScreenInterface::disableOutputClipping ()
    WRAPABLE_DEF_FUNC(disableOutputClipping)

void
ScreenInterface::enterShowDesktopMode ()
    WRAPABLE_DEF_FUNC(enterShowDesktopMode)

void
ScreenInterface::leaveShowDesktopMode (CompWindow *window)
    WRAPABLE_DEF_FUNC(leaveShowDesktopMode, window)

void
ScreenInterface::outputChangeNotify ()
    WRAPABLE_DEF_FUNC(outputChangeNotify)

CompWindowList
ScreenInterface::getWindowPaintList ()
    WRAPABLE_DEF_FUNC_WITH_RETURN(CompWindowList (), getWindowPaintList)

CompDisplay *
CompScreen::display ()
{
    return priv->display;
}

Window
CompScreen::root ()
{
    return priv->root;
}

CompOption *
CompScreen::getOption (const char *name)
{
    CompOption *o = CompOption::findOption (priv->opt, name);
    return o;
}

bool
CompScreen::hasGrab ()
{
    return !priv->grabs.empty ();
}

unsigned int
CompScreen::showingDesktopMask ()
{
    return priv->showingDesktopMask;
}

bool
paintTimeout (void *closure)
{
    CompScreen *s = (CompScreen *) closure;
    return s->handlePaintTimeout ();
}


int
CompScreen::getTimeToNextRedraw (struct timeval *tv)
{
    int diff, next;

    diff = TIMEVALDIFF (tv, &priv->lastRedraw);

    /* handle clock rollback */
    if (diff < 0)
	diff = 0;

    if (priv->idle || (getVideoSync &&
	priv->opt[COMP_SCREEN_OPTION_SYNC_TO_VBLANK].value ().b ()))
    {
	if (priv->timeMult > 1)
	{
	    priv->frameStatus = -1;
	    priv->redrawTime = priv->optimalRedrawTime;
	    priv->timeMult--;
	}
    }
    else
    {
	if (diff > priv->redrawTime)
	{
	    if (priv->frameStatus > 0)
		priv->frameStatus = 0;

	    next = priv->optimalRedrawTime * (priv->timeMult + 1);
	    if (diff > next)
	    {
		priv->frameStatus--;
		if (priv->frameStatus < -1)
		{
		    priv->timeMult++;
		    priv->redrawTime = diff = next;
		}
	    }
	}
	else if (diff < priv->redrawTime)
	{
	    if (priv->frameStatus < 0)
		priv->frameStatus = 0;

	    if (priv->timeMult > 1)
	    {
		next = priv->optimalRedrawTime * (priv->timeMult - 1);
		if (diff < next)
		{
		    priv->frameStatus++;
		    if (priv->frameStatus > 4)
		    {
			priv->timeMult--;
			priv->redrawTime = next;
		    }
		}
	    }
	}
    }
    if (diff >= priv->redrawTime)
	return 1;

    return priv->redrawTime - diff;
}

void
CompScreen::waitForVideoSync ()
{
    unsigned int sync;

    if (!priv->opt[COMP_SCREEN_OPTION_SYNC_TO_VBLANK].value ().b ())
	return;

    if (getVideoSync)
    {
	glFlush ();

	(*getVideoSync) (&sync);
	(*waitVideoSync) (2, (sync + 1) % 2, &sync);
    }
}

void
CompScreen::paint (CompOutput::ptrList &outputs,
		   unsigned int        mask)
{
    WRAPABLE_HND_FUNC(paint, outputs, mask)

    XRectangle                    r;
    CompOutput::ptrList::iterator it;
    CompOutput                    *output;

    for (it = outputs.begin (); it != outputs.end (); it++)
    {
	output = (*it);
	targetScreen = this;
	targetOutput = output;

	r.x	 = output->x1 ();
	r.y	 = priv->size.height () - output->y2 ();
	r.width  = output->width ();
	r.height = output->height ();

	if (priv->lastViewport.x	   != r.x     ||
	    priv->lastViewport.y	   != r.y     ||
	    priv->lastViewport.width  != r.width ||
	    priv->lastViewport.height != r.height)
	{
	    glViewport (r.x, r.y, r.width, r.height);
	    priv->lastViewport = r;
	}

	if (mask & COMP_SCREEN_DAMAGE_ALL_MASK)
	{
	    CompMatrix identity;

	    paintOutput (&defaultScreenPaintAttrib,
			 &identity,
			 output->region (), output,
			 PAINT_SCREEN_REGION_MASK |
			 PAINT_SCREEN_FULL_MASK);
	}
	else if (mask & COMP_SCREEN_DAMAGE_REGION_MASK)
	{
	    CompMatrix identity;

	    XIntersectRegion (priv->display->mTmpRegion,
			      output->region (),
			      priv->display->mOutputRegion);

	    if (!paintOutput (&defaultScreenPaintAttrib,
			      &identity,
			      priv->display->mOutputRegion, output,
			      PAINT_SCREEN_REGION_MASK))
	    {
		identity.reset ();

		paintOutput (&defaultScreenPaintAttrib,
			     &identity,
			     output->region (), output,
			     PAINT_SCREEN_FULL_MASK);

		XUnionRegion (priv->display->mTmpRegion,
			      output->region (),
			      priv->display->mTmpRegion);

	    }
	}
    }
}

void
CompScreen::warpPointer (int dx, int dy)
{
    CompDisplay *display = priv->display;
    XEvent      event;

    pointerX += dx;
    pointerY += dy;

    if (pointerX >= priv->size.width ())
	pointerX = priv->size.width () - 1;
    else if (pointerX < 0)
	pointerX = 0;

    if (pointerY >= priv->size.height ())
	pointerY = priv->size.height () - 1;
    else if (pointerY < 0)
	pointerY = 0;

    XWarpPointer (display->dpy (),
		  None, priv->root,
		  0, 0, 0, 0,
		  pointerX, pointerY);

    XSync (display->dpy (), FALSE);

    while (XCheckMaskEvent (display->dpy (),
			    LeaveWindowMask |
			    EnterWindowMask |
			    PointerMotionMask,
			    &event));

    if (!inHandleEvent)
    {
	lastPointerX = pointerX;
	lastPointerY = pointerY;
    }
}

CompWindowList &
CompScreen::windows ()
{
    return priv->windows;
}

Time
CompScreen::getCurrentTime ()
{
    XEvent event;

    XChangeProperty (priv->display->dpy (), priv->grabWindow,
		     XA_PRIMARY, XA_STRING, 8,
		     PropModeAppend, NULL, 0);
    XWindowEvent (priv->display->dpy (), priv->grabWindow,
		  PropertyChangeMask,
		  &event);

    return event.xproperty.time;
}

Atom
CompScreen::selectionAtom ()
{
    return priv->wmSnAtom;
}

Window
CompScreen::selectionWindow ()
{
    return priv->wmSnSelectionWindow;
}

Time
CompScreen::selectionTimestamp ()
{
    return priv->wmSnTimestamp;
}

int
CompScreen::screenNum ()
{
    return priv->screenNum;
}

bool
CompScreen::handlePaintTimeout ()
{
    int         timeDiff;
    struct      timeval tv;
    CompDisplay *d = priv->display;

    gettimeofday (&tv, 0);

    if (priv->damageMask)
    {
	finishDrawing ();
 
	targetScreen = this;

	timeDiff = TIMEVALDIFF (&tv, &priv->lastRedraw);

	/* handle clock rollback */
	if (timeDiff < 0)
	    timeDiff = 0;

	makeCurrent ();

	if (priv->slowAnimations)
	{
	    preparePaint (priv->idle ? 2 : (timeDiff * 2) / priv->redrawTime);
	}
	else
	    preparePaint (priv->idle ? priv->redrawTime : timeDiff);

	/* substract top most overlay window region */
	if (priv->overlayWindowCount)
	{
	    for (CompWindowList::reverse_iterator rit = priv->windows.rbegin ();
	         rit != priv->windows.rend (); rit++)
	    {
		CompWindow *w = (*rit);

		if (w->destroyed () || w->invisible ())
		    continue;

		if (!w->redirected ())
		    XSubtractRegion (priv->damage, w->region (),
				     priv->damage);

		break;
	    }

	    if (priv->damageMask & COMP_SCREEN_DAMAGE_ALL_MASK)
	    {
		priv->damageMask &= ~COMP_SCREEN_DAMAGE_ALL_MASK;
		priv->damageMask |= COMP_SCREEN_DAMAGE_REGION_MASK;
	    }
	}

	if (priv->damageMask & COMP_SCREEN_DAMAGE_REGION_MASK)
	{
	    XIntersectRegion (priv->damage, &priv->region,
			      d->mTmpRegion);

	    if (d->mTmpRegion->numRects  == 1	  &&
		d->mTmpRegion->rects->x1 == 0	  &&
		d->mTmpRegion->rects->y1 == 0	  &&
		d->mTmpRegion->rects->x2 == priv->size.width () &&
		d->mTmpRegion->rects->y2 == priv->size.height ())
		damageScreen ();
	}

	EMPTY_REGION (priv->damage);

	int mask = priv->damageMask;
	priv->damageMask = 0;

	if (priv->clearBuffers)
	{
	    if (mask & COMP_SCREEN_DAMAGE_ALL_MASK)
		glClear (GL_COLOR_BUFFER_BIT);
	}

	CompOutput::ptrList outputs (0);
	
	if (priv->opt[COMP_SCREEN_OPTION_FORCE_INDEPENDENT].value ().b ()
	    || !priv->hasOverlappingOutputs)
	{
	    for (unsigned int i = 0; i < priv->outputDevs.size (); i++)
		outputs.push_back (&priv->outputDevs[i]);
	}
	else
	    outputs.push_back (&priv->fullscreenOutput);

	paint (outputs, mask);
	
	targetScreen = NULL;
	targetOutput = &priv->outputDevs[0];

	waitForVideoSync ();

	if (mask & COMP_SCREEN_DAMAGE_ALL_MASK)
	{
	    glXSwapBuffers (d->dpy (), priv->output);
	}
	else
	{
	    BoxPtr pBox;
	    int    nBox, y;

	    pBox = d->mTmpRegion->rects;
	    nBox = d->mTmpRegion->numRects;

	    if (copySubBuffer)
	    {
		while (nBox--)
		{
		    y = priv->size.height () - pBox->y2;

		    (*copySubBuffer) (d->dpy (),
				      priv->output,
				      pBox->x1, y,
				      pBox->x2 - pBox->x1,
				      pBox->y2 - pBox->y1);

		    pBox++;
		}
	    }
	    else
	    {
		glEnable (GL_SCISSOR_TEST);
		glDrawBuffer (GL_FRONT);

		while (nBox--)
		{
		    y = priv->size.height () - pBox->y2;

		    glBitmap (0, 0, 0, 0,
			      pBox->x1 - priv->rasterPos.x (),
			      y - priv->rasterPos.y (),
			      NULL);

		    priv->rasterPos = CompPoint (pBox->x1, y);

		    glScissor (pBox->x1, y,
			       pBox->x2 - pBox->x1,
			       pBox->y2 - pBox->y1);

		    glCopyPixels (pBox->x1, y,
				  pBox->x2 - pBox->x1,
				  pBox->y2 - pBox->y1,
				  GL_COLOR);

		    pBox++;
		}

		glDrawBuffer (GL_BACK);
		glDisable (GL_SCISSOR_TEST);
		glFlush ();
	    }
	}

	priv->lastRedraw = tv;

	donePaint ();

	/* remove destroyed windows */
	while (priv->pendingDestroys)
	{
	    foreach (CompWindow *w, priv->windows)
	    {
		if (w->destroyed ())
		{
		    w->addDamage ();
		    delete w;
		    break;
		}
	    }

	    priv->pendingDestroys--;
	}

	priv->idle = false;
    }
    else
    {
	priv->idle = true;
    }

    gettimeofday (&tv, 0);

    priv->paintTimer.setTimes (getTimeToNextRedraw (&tv), MAXSHORT);
    return true;
}

int
CompScreen::maxTextureSize ()
{
    return priv->maxTextureSize;
}

unsigned int
CompScreen::damageMask ()
{
    return priv->damageMask;
}

CompPoint
CompScreen::vp ()
{
    return priv->vp;
}

CompSize
CompScreen::vpSize ()
{
    return priv->vpSize;
}

CompSize
CompScreen::size ()
{
    return priv->size;
}

unsigned int &
CompScreen::pendingDestroys ()
{
    return priv->pendingDestroys;
}

unsigned int &
CompScreen::mapNum ()
{
    return priv->mapNum;
}

int &
CompScreen::desktopWindowCount ()
{
    return priv->desktopWindowCount;
}

int &
CompScreen::overlayWindowCount ()
{
    return priv->overlayWindowCount;
}

CompOutput::vector &
CompScreen::outputDevs ()
{
    return priv->outputDevs;
}

XRectangle
CompScreen::workArea ()
{
    return priv->workArea;
}


unsigned int
CompScreen::currentDesktop ()
{
    return priv->currentDesktop;
}

unsigned int
CompScreen::nDesktop ()
{
    return priv->nDesktop;
}

CompActiveWindowHistory *
CompScreen::currentHistory ()
{
    return &priv->history[priv->currentHistory];
}

CompScreenEdge &
CompScreen::screenEdge (int edge)
{
    return priv->screenEdge[edge];
}

Window
CompScreen::overlay ()
{
    return priv->overlay;
}

unsigned int &
CompScreen::activeNum ()
{
    return priv->activeNum;
}

void
CompScreen::updateBackground ()
{
    priv->backgroundTexture.reset ();

    if (priv->backgroundLoaded)
    {
	priv->backgroundLoaded = false;
	damageScreen ();
    }
}

bool
CompScreen::textureNonPowerOfTwo ()
{
    return priv->textureNonPowerOfTwo;
}

bool
CompScreen::textureCompression ()
{
    return priv->textureCompression;
}

bool
CompScreen::canDoSaturated ()
{
    return priv->canDoSaturated;
}

bool
CompScreen::canDoSlightlySaturated ()
{
    return priv->canDoSlightlySaturated;
}

bool
CompScreen::lighting ()
{
    return priv->lighting;
}

CompTexture::Filter
CompScreen::filter (int filter)
{
    return priv->filter[filter];
}

CompFragment::Storage *
CompScreen::fragmentStorage ()
{
    return &priv->fragmentStorage;
}

bool
CompScreen::fragmentProgram ()
{
    return priv->fragmentProgram;
}

CompFBConfig*
CompScreen::glxPixmapFBConfig (unsigned int depth)
{
    return &priv->glxPixmapFBConfigs[depth];
}

bool
CompScreen::framebufferObject ()
{
    return priv->fbo;
}
