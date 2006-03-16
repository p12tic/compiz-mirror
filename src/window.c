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
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xcomposite.h>

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdint.h>

#include <compiz.h>

#define MwmHintsDecorations (1L << 1)

#define PropMotifWmHintElements 3

typedef struct {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
} MwmHints;

static int
reallocWindowPrivates (int  size,
		       void *closure)
{
    CompScreen *s = (CompScreen *) closure;
    CompWindow *w;
    void       *privates;

    for (w = s->windows; w; w = w->next)
    {
	privates = realloc (w->privates, size * sizeof (CompPrivate));
	if (!privates)
	    return FALSE;

	w->privates = (CompPrivate *) privates;
    }

    return TRUE;
}

int
allocateWindowPrivateIndex (CompScreen *screen)
{
    return allocatePrivateIndex (&screen->windowPrivateLen,
				 &screen->windowPrivateIndices,
				 reallocWindowPrivates,
				 (void *) screen);
}

void
freeWindowPrivateIndex (CompScreen *screen,
			int	   index)
{
    freePrivateIndex (screen->windowPrivateLen,
		      screen->windowPrivateIndices,
		      index);
}

static void
recalcNormalHints (CompWindow *window)
{
    int maxSize;

    maxSize = window->screen->maxTextureSize;
    maxSize -= window->attrib.border_width * 2;

    window->sizeHints.x      = window->attrib.x;
    window->sizeHints.y      = window->attrib.y;
    window->sizeHints.width  = window->attrib.width;
    window->sizeHints.height = window->attrib.height;

    if (window->sizeHints.flags & PMinSize)
    {
	window->sizeHints.base_width  = window->sizeHints.min_width;
	window->sizeHints.base_height = window->sizeHints.min_height;
    }
    else
    {
	window->sizeHints.base_width  = 0;
	window->sizeHints.base_height = 0;
    }

    window->sizeHints.flags |= PBaseSize;

    if (window->sizeHints.flags & PBaseSize)
    {
	window->sizeHints.min_width  = window->sizeHints.base_width;
	window->sizeHints.min_height = window->sizeHints.base_height;
    }
    else
    {
	window->sizeHints.min_width  = 0;
	window->sizeHints.min_height = 0;
    }
    window->sizeHints.flags |= PMinSize;

    if (!(window->sizeHints.flags & PMaxSize))
    {
	window->sizeHints.max_width  = 65535;
	window->sizeHints.max_height = 65535;
	window->sizeHints.flags |= PMaxSize;
    }

    if (window->sizeHints.max_width < window->sizeHints.min_width)
	window->sizeHints.max_width = window->sizeHints.min_width;

    if (window->sizeHints.max_height < window->sizeHints.min_height)
	window->sizeHints.max_height = window->sizeHints.min_height;

    if (window->sizeHints.min_width < 1)
	window->sizeHints.min_width = 1;

    if (window->sizeHints.max_width < 1)
	window->sizeHints.max_width = 1;

    if (window->sizeHints.min_height < 1)
	window->sizeHints.min_height = 1;

    if (window->sizeHints.max_height < 1)
	window->sizeHints.max_height = 1;

    if (window->sizeHints.max_width > maxSize)
	window->sizeHints.max_width = maxSize;

    if (window->sizeHints.max_height > maxSize)
	window->sizeHints.max_height = maxSize;

    if (window->sizeHints.min_width > maxSize)
	window->sizeHints.min_width = maxSize;

    if (window->sizeHints.min_height > maxSize)
	window->sizeHints.min_height = maxSize;

    if (window->sizeHints.base_width > maxSize)
	window->sizeHints.base_width = maxSize;

    if (window->sizeHints.base_height > maxSize)
	window->sizeHints.base_height = maxSize;

    if (window->sizeHints.flags & PResizeInc)
    {
	if (window->sizeHints.width_inc == 0)
	    window->sizeHints.width_inc = 1;

	if (window->sizeHints.height_inc == 0)
	    window->sizeHints.height_inc = 1;
    }
    else
    {
	window->sizeHints.width_inc  = 1;
	window->sizeHints.height_inc = 1;
	window->sizeHints.flags |= PResizeInc;
    }

    if (window->sizeHints.flags & PAspect)
    {
	/* don't divide by 0 */
	if (window->sizeHints.min_aspect.y < 1)
	    window->sizeHints.min_aspect.y = 1;

	if (window->sizeHints.max_aspect.y < 1)
	    window->sizeHints.max_aspect.y = 1;
    }
    else
    {
	window->sizeHints.min_aspect.x = 1;
	window->sizeHints.min_aspect.y = 65535;
	window->sizeHints.max_aspect.x = 65535;
	window->sizeHints.max_aspect.y = 1;
	window->sizeHints.flags |= PAspect;
    }

    if (!(window->sizeHints.flags & PWinGravity))
    {
	window->sizeHints.win_gravity = NorthWestGravity;
	window->sizeHints.flags |= PWinGravity;
    }
}

void
updateNormalHints (CompWindow *w)
{
    Status status;
    long   supplied;

    status = XGetWMNormalHints (w->screen->display->display, w->id,
				&w->sizeHints, &supplied);

    if (!status)
	w->sizeHints.flags = 0;

    recalcNormalHints (w);
}

void
updateWmHints (CompWindow *w)
{
    XWMHints *hints;

    hints = XGetWMHints (w->screen->display->display, w->id);
    if (hints)
    {
	if (hints->flags & InputHint)
	    w->inputHint = hints->input;

	XFree (hints);
    }
}

void
updateWindowClassHints (CompWindow *w)
{
    XClassHint classHint;
    int        status;

    status = XGetClassHint (w->screen->display->display, w->id, &classHint);

    if (status)
    {
	if (classHint.res_name)
	{
	    if (w->resName)
		free (w->resName);

	    w->resName = strdup (classHint.res_name);
	    XFree (classHint.res_name);
	}

	if (classHint.res_class)
	{
	    if (w->resClass)
		free (w->resClass);

	    w->resClass = strdup (classHint.res_class);
	    XFree (classHint.res_class);
	}
    }
}

static Window
getClientLeaderOfAncestor (CompWindow *w)
{
    if (w->transientFor)
    {
	w = findWindowAtScreen (w->screen, w->transientFor);
	if (w)
	{
	    if (w->clientLeader)
		return w->clientLeader;

	    return getClientLeaderOfAncestor (w);
	}
    }

    return None;
}

Window
getClientLeader (CompWindow *w)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;

    result = XGetWindowProperty (w->screen->display->display, w->id,
				 w->screen->display->wmClientLeaderAtom,
				 0L, 1L, False, XA_WINDOW, &actual, &format,
				 &n, &left, &data);

    if (result == Success && n && data)
    {
	Window win;

	memcpy (&win, data, sizeof (Window));
	XFree ((void *) data);

	if (win)
	    return win;
    }

    return getClientLeaderOfAncestor (w);
}

int
getWmState (CompDisplay *display,
	    Window      id)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;
    unsigned long state = NormalState;

    result = XGetWindowProperty (display->display, id,
				 display->wmStateAtom, 0L, 2L, FALSE,
				 display->wmStateAtom, &actual, &format,
				 &n, &left, &data);

    if (result == Success && n && data)
    {
	memcpy (&state, data, sizeof (unsigned long));
	XFree ((void *) data);
    }

    return state;
}

void
setWmState (CompDisplay *display,
	    int		state,
	    Window      id)
{
    unsigned long data[2];

    data[0] = state;
    data[1] = None;

    XChangeProperty (display->display, id,
		     display->wmStateAtom, display->wmStateAtom,
		     32, PropModeReplace, (unsigned char *) data, 2);
}

unsigned int
windowStateMask (CompDisplay *display,
		 Atom	     state)
{
    if (state == display->winStateModalAtom)
	return CompWindowStateModalMask;
    else if (state == display->winStateStickyAtom)
	return CompWindowStateStickyMask;
    else if (state == display->winStateMaximizedVertAtom)
	return CompWindowStateMaximizedVertMask;
    else if (state == display->winStateMaximizedHorzAtom)
	return CompWindowStateMaximizedHorzMask;
    else if (state == display->winStateShadedAtom)
	return CompWindowStateShadedMask;
    else if (state == display->winStateSkipTaskbarAtom)
	return CompWindowStateSkipTaskbarMask;
    else if (state == display->winStateSkipPagerAtom)
	return CompWindowStateSkipPagerMask;
    else if (state == display->winStateHiddenAtom)
	return CompWindowStateHiddenMask;
    else if (state == display->winStateFullscreenAtom)
	return CompWindowStateFullscreenMask;
    else if (state == display->winStateAboveAtom)
	return CompWindowStateAboveMask;
    else if (state == display->winStateBelowAtom)
	return CompWindowStateBelowMask;
    else if (state == display->winStateDemandsAttentionAtom)
	return CompWindowStateDemandsAttentationMask;
    else if (state == display->winStateDisplayModalAtom)
	return CompWindowStateDisplayModalMask;

    return 0;
}

unsigned int
getWindowState (CompDisplay *display,
		Window      id)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;
    unsigned int  state = 0;

    result = XGetWindowProperty (display->display, id, display->winStateAtom,
				 0L, 1024L, FALSE, XA_ATOM, &actual, &format,
				 &n, &left, &data);

    if (result == Success && n && data)
    {
	Atom *a = (Atom *) data;

	while (n--)
	    state |= windowStateMask (display, *a++);

	XFree ((void *) data);
    }

    return state;
}

void
setWindowState (CompDisplay  *display,
		unsigned int state,
		Window       id)
{
    Atom data[32];
    int	 i = 0;

    if (state & CompWindowStateModalMask)
	data[i++] = display->winStateModalAtom;
    if (state & CompWindowStateStickyMask)
	data[i++] = display->winStateStickyAtom;
    if (state & CompWindowStateMaximizedVertMask)
	data[i++] = display->winStateMaximizedVertAtom;
    if (state & CompWindowStateMaximizedHorzMask)
	data[i++] = display->winStateMaximizedHorzAtom;
    if (state & CompWindowStateShadedMask)
	data[i++] = display->winStateShadedAtom;
    if (state & CompWindowStateSkipTaskbarMask)
	data[i++] = display->winStateSkipTaskbarAtom;
    if (state & CompWindowStateSkipPagerMask)
	data[i++] = display->winStateSkipPagerAtom;
    if (state & CompWindowStateHiddenMask)
	data[i++] = display->winStateHiddenAtom;
    if (state & CompWindowStateFullscreenMask)
	data[i++] = display->winStateFullscreenAtom;
    if (state & CompWindowStateAboveMask)
	data[i++] = display->winStateAboveAtom;
    if (state & CompWindowStateBelowMask)
	data[i++] = display->winStateBelowAtom;
    if (state & CompWindowStateDemandsAttentationMask)
	data[i++] = display->winStateDemandsAttentionAtom;
    if (state & CompWindowStateDisplayModalMask)
	data[i++] = display->winStateDisplayModalAtom;

    XChangeProperty (display->display, id, display->winStateAtom,
		     XA_ATOM, 32, PropModeReplace,
		     (unsigned char *) data, i);
}

static void
setWindowActions (CompDisplay  *display,
		  unsigned int actions,
		  Window       id)
{
    Atom data[32];
    int	 i = 0;

    if (actions & CompWindowActionMoveMask)
	data[i++] = display->winActionMoveAtom;
    if (actions & CompWindowActionResizeMask)
	data[i++] = display->winActionResizeAtom;
    if (actions & CompWindowActionStickMask)
	data[i++] = display->winActionStickAtom;
    if (actions & CompWindowActionMinimizeMask)
	data[i++] = display->winActionMinimizeAtom;
    if (actions & CompWindowActionMaximizeHorzMask)
	data[i++] = display->winActionMaximizeHorzAtom;
    if (actions & CompWindowActionMaximizeVertMask)
	data[i++] = display->winActionMaximizeVertAtom;
    if (actions & CompWindowActionFullscreenMask)
	data[i++] = display->winActionFullscreenAtom;
    if (actions & CompWindowActionCloseMask)
	data[i++] = display->winActionCloseAtom;

    XChangeProperty (display->display, id, display->wmAllowedActionsAtom,
		     XA_ATOM, 32, PropModeReplace,
		     (unsigned char *) data, i);
}

static void
recalcWindowActions (CompWindow *w)
{
    unsigned int actions = 0;

    switch (w->type) {
    case CompWindowTypeFullscreenMask:
    case CompWindowTypeNormalMask:
	actions |=
	    CompWindowActionMinimizeMask     |
	    CompWindowActionMaximizeHorzMask |
	    CompWindowActionMaximizeVertMask |
	    CompWindowActionFullscreenMask;
	/* fall-through */
    case CompWindowTypeDialogMask:
    case CompWindowTypeModalDialogMask:
	actions |=
	    CompWindowActionMoveMask   |
	    CompWindowActionResizeMask |
	    CompWindowActionStickMask  |
	    CompWindowActionCloseMask;
	break;
    case CompWindowTypeUtilMask:
    case CompWindowTypeToolbarMask:
    case CompWindowTypeMenuMask:
    case CompWindowTypeSplashMask:
    case CompWindowTypeDesktopMask:
    case CompWindowTypeDockMask:
    case CompWindowTypeUnknownMask:
    default:
	break;
    }

    if (actions != w->actions)
    {
	w->actions = actions;
	setWindowActions (w->screen->display, actions, w->id);
    }
}

unsigned int
getWindowType (CompDisplay *display,
	       Window      id)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;

    result = XGetWindowProperty (display->display, id, display->winTypeAtom,
				 0L, 1L, FALSE, XA_ATOM, &actual, &format,
				 &n, &left, &data);

    if (result == Success && n && data)
    {
	Atom a;

	memcpy (&a, data, sizeof (Atom));
	XFree ((void *) data);

	if (a == display->winTypeNormalAtom)
	    return CompWindowTypeNormalMask;
	else if (a == display->winTypeMenuAtom)
	    return CompWindowTypeMenuMask;
	else if (a == display->winTypeDesktopAtom)
	    return CompWindowTypeDesktopMask;
	else if (a == display->winTypeDockAtom)
	    return CompWindowTypeDockMask;
	else if (a == display->winTypeToolbarAtom)
	    return CompWindowTypeToolbarMask;
	else if (a == display->winTypeUtilAtom)
	    return CompWindowTypeUtilMask;
	else if (a == display->winTypeSplashAtom)
	    return CompWindowTypeSplashMask;
	else if (a == display->winTypeDialogAtom)
	    return CompWindowTypeDialogMask;
    }

    return CompWindowTypeUnknownMask;
}

void
recalcWindowType (CompWindow *w)
{
    unsigned int type;

    type = w->wmType;

    if (!w->attrib.override_redirect && w->wmType == CompWindowTypeUnknownMask)
	type = CompWindowTypeNormalMask;

    if (w->state & CompWindowStateFullscreenMask)
	type = CompWindowTypeFullscreenMask;

    if (type == CompWindowTypeNormalMask)
    {
	if (w->transientFor)
	    type = CompWindowTypeDialogMask;
    }

    if ((type & (CompWindowTypeNormalMask | CompWindowTypeDialogMask)) &&
	(w->state & CompWindowStateModalMask))
	type = CompWindowTypeModalDialogMask;

    if (type != w->type)
    {
	w->type = type;
	recalcWindowActions (w);
    }
}

unsigned int
getMwmDecor (CompDisplay *display,
	     Window      id)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    MwmHints	  *mwmHints;
    unsigned int  decor = MwmDecorAll;

    result = XGetWindowProperty (display->display, id, display->mwmHintsAtom,
				 0L, 20L, FALSE, display->mwmHintsAtom,
				 &actual, &format, &n, &left,
				 (unsigned char **) &mwmHints);

    if (result == Success && n && mwmHints)
    {
	if (n >= PropMotifWmHintElements)
	{
	    if (mwmHints->flags & MwmHintsDecorations)
		decor = mwmHints->decorations;
	}

	XFree (mwmHints);
    }

    return decor;
}

unsigned int
getProtocols (CompDisplay *display,
	      Window      id)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    Atom	  *protocol;
    unsigned int  protocols = 0;

    result = XGetWindowProperty (display->display, id, display->wmProtocolsAtom,
				 0L, 20L, FALSE, XA_ATOM,
				 &actual, &format, &n, &left,
				 (unsigned char **) &protocol);

    if (result == Success && n && protocol)
    {
	int i;

	for (i = 0; i < n; i++)
	{
	    if (protocol[i] == display->wmDeleteWindowAtom)
		protocols |= CompWindowProtocolDeleteMask;
	    else if (protocol[i] == display->wmTakeFocusAtom)
		protocols |= CompWindowProtocolTakeFocusMask;
	    else if (protocol[i] == display->wmPingAtom)
		protocols |= CompWindowProtocolPingMask;
	    else if (protocol[i] == display->wmSyncRequestAtom)
		protocols |= CompWindowProtocolSyncRequestMask;
	}

	XFree (protocol);
    }

    return protocols;
}

unsigned short
getWindowProp32 (CompDisplay	*display,
		 Window		id,
		 Atom		property,
		 unsigned short defaultValue)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned char *data;

    result = XGetWindowProperty (display->display, id, property,
				 0L, 1L, FALSE, XA_CARDINAL, &actual, &format,
				 &n, &left, &data);

    if (result == Success && n && data)
    {
	CARD32 value;

	memcpy (&value, data, sizeof (CARD32));

	XFree (data);

	return value >> 16;
    }

    return defaultValue;
}

void
setWindowProp32 (CompDisplay    *display,
		 Window         id,
		 Atom		property,
		 unsigned short value)
{
    CARD32 value32;

    value32 = value << 16 | value;

    XChangeProperty (display->display, id, property,
		     XA_CARDINAL, 32, PropModeReplace,
		     (unsigned char *) &value32, 1);
}


static void
updateFrameWindow (CompWindow *w)
{
    if (w->input.left || w->input.right || w->input.top || w->input.bottom)
    {
	XRectangle rects[4];
	int	   x, y, width, height;

	x      = w->serverX - w->input.left;
	y      = w->serverY - w->input.top;
	width  = w->width  + w->input.left + w->input.right;
	height = w->height + w->input.top  + w->input.bottom;

	if (!w->frame)
	{
	    XSetWindowAttributes attr;
	    XWindowChanges	 xwc;

	    attr.event_mask	   = 0;
	    attr.override_redirect = TRUE;

	    w->frame = XCreateWindow (w->screen->display->display,
				      w->screen->root,
				      x, y, width, height, 0,
				      CopyFromParent,
				      InputOnly,
				      CopyFromParent,
				      CWOverrideRedirect | CWEventMask, &attr);

	    XGrabButton (w->screen->display->display, AnyButton,
			 AnyModifier, w->frame, TRUE, ButtonPressMask |
			 ButtonReleaseMask | ButtonMotionMask,
			 GrabModeSync, GrabModeSync, None, None);

	    xwc.stack_mode = Below;
	    xwc.sibling    = w->id;

	    XConfigureWindow (w->screen->display->display, w->frame,
			      CWSibling | CWStackMode, &xwc);

	    if (w->mapNum)
		XMapWindow (w->screen->display->display, w->frame);

	    XChangeProperty (w->screen->display->display, w->id,
			     w->screen->display->frameWindowAtom,
			     XA_WINDOW, 32, PropModeReplace,
			     (unsigned char *) &w->frame, 1);
	}

	XResizeWindow (w->screen->display->display, w->frame, width, height);

	rects[0].x	= 0;
	rects[0].y	= 0;
	rects[0].width  = width;
	rects[0].height = w->input.top;

	rects[1].x	= 0;
	rects[1].y	= w->input.top;
	rects[1].width  = w->input.left;
	rects[1].height = height - w->input.top - w->input.bottom;

	rects[2].x	= width - w->input.right;
	rects[2].y	= w->input.top;
	rects[2].width  = w->input.right;
	rects[2].height = height - w->input.top - w->input.bottom;

	rects[3].x	= 0;
	rects[3].y	= height - w->input.bottom;
	rects[3].width  = width;
	rects[3].height = w->input.bottom;

	XShapeCombineRectangles (w->screen->display->display,
				 w->frame,
				 ShapeInput,
				 0,
				 0,
				 rects,
				 4,
				 ShapeSet,
				 YXBanded);
    }
    else
    {
	if (w->frame)
	{
	    XDestroyWindow (w->screen->display->display, w->frame);
	    w->frame = None;
	}
    }
}

void
setWindowFrameExtents (CompWindow	 *w,
		       CompWindowExtents *input,
		       CompWindowExtents *output)
{
    if (input->left   != w->input.left  ||
	input->right  != w->input.right ||
	input->top    != w->input.top   ||
	input->bottom != w->input.bottom)
    {
	unsigned long data[4];

	w->input = *input;

	data[0] = input->left;
	data[1] = input->right;
	data[2] = input->top;
	data[3] = input->bottom;

	updateFrameWindow (w);

	XChangeProperty (w->screen->display->display, w->id,
			 w->screen->display->frameExtentsAtom,
			 XA_CARDINAL, 32, PropModeReplace,
			 (unsigned char *) data, 4);
    }

    if (output->left   != w->output.left  ||
	output->right  != w->output.right ||
	output->top    != w->output.top   ||
	output->bottom != w->output.bottom)
    {
	w->output = *output;

	(*w->screen->windowResizeNotify) (w);
    }
}

static void
setWindowMatrix (CompWindow *w)
{
    w->matrix = w->texture.matrix;
    w->matrix.x0 -= (w->attrib.x * w->matrix.xx);
    w->matrix.y0 -= (w->attrib.y * w->matrix.yy);
}

void
bindWindow (CompWindow *w)
{
    if (testMode)
    {
	unsigned int width, height;

	if (readImageToTexture (w->screen, &w->texture,
				windowImage, &width, &height))
	{
	    XResizeWindow (w->screen->display->display, w->id, width, height);

	    w->width  = width;
	    w->height = height;
	}

	w->pixmap	  = 1;
	w->texture.pixmap = 1;
    }
    else
    {
	if (!w->pixmap)
	{
	    w->pixmap =
		XCompositeNameWindowPixmap (w->screen->display->display,
					    w->id);
	    if (!w->pixmap)
	    {
		fprintf (stderr, "%s: XCompositeNameWindowPixmap failed\n",
			 programName);
	    }
	}

	if (!bindPixmapToTexture (w->screen, &w->texture, w->pixmap,
				  w->width, w->height,
				  w->attrib.depth))
	{
	    fprintf (stderr, "%s: Couldn't bind redirected window 0x%x to "
		     "texture\n", programName, (int) w->id);
	}
    }

    setWindowMatrix (w);
}

void
releaseWindow (CompWindow *w)
{
    if (w->pixmap)
    {
	releasePixmapFromTexture (w->screen, &w->texture);

	if (!testMode)
	    XFreePixmap (w->screen->display->display, w->pixmap);

	w->pixmap = None;
    }
}

static void
freeWindow (CompWindow *w)
{
    releaseWindow (w);

    if (w->syncAlarm)
	XSyncDestroyAlarm (w->screen->display->display, w->syncAlarm);

    if (w->syncWaitHandle)
	compRemoveTimeout (w->syncWaitHandle);

    if (w->texture.name)
	finiTexture (w->screen, &w->texture);

    if (w->clip)
	XDestroyRegion (w->clip);

    if (w->region)
	XDestroyRegion (w->region);

    if (w->privates)
	free (w->privates);

    if (w->sizeDamage)
	free (w->damageRects);

    if (w->vertices)
	free (w->vertices);

    if (w->indices)
	free (w->indices);

    if (lastFoundWindow == w)
	lastFoundWindow = 0;

    if (lastDamagedWindow == w)
	lastDamagedWindow = 0;

    if (w->struts)
	free (w->struts);

    if (w->startupId)
	free (w->startupId);

    if (w->resName)
	free (w->resName);

    if (w->resClass)
	free (w->resClass);

    free (w);
}

void
damageWindowRegion (CompWindow *w,
		    Region     region)
{
    if (w->scaled)
    {
	REGION reg;
	int    x1, y1, x2, y2;

	reg.rects    = &reg.extents;
	reg.numRects = 1;

	x1 = region->extents.x1 - w->attrib.x;
	y1 = region->extents.y1 - w->attrib.y;
	x2 = region->extents.x2 - w->attrib.x;
	y2 = region->extents.y2 - w->attrib.y;

	reg.extents.x1 = (x1 * w->paint.xScale) + w->attrib.x;
	reg.extents.y1 = (y1 * w->paint.yScale) + w->attrib.y;
	reg.extents.x2 = (x2 * w->paint.xScale + 0.5f) + w->attrib.x;
	reg.extents.y2 = (y2 * w->paint.yScale + 0.5f) + w->attrib.y;

	if (reg.extents.x2 > reg.extents.x1 && reg.extents.y2 > reg.extents.y1)
	    damageScreenRegion (w->screen, &reg);
    }
    else
    {
	damageScreenRegion (w->screen, region);
    }
}

void
damageWindowOutputExtents (CompWindow *w)
{
    if (w->screen->damageMask & COMP_SCREEN_DAMAGE_ALL_MASK)
	return;

    if (w->attrib.map_state == IsViewable && w->damaged)
    {
	REGION reg;

	reg.rects = &reg.extents;
	reg.numRects = reg.size = 1;

	/* top */
	reg.extents.x1 = w->attrib.x - w->output.left;
	reg.extents.y1 = w->attrib.y - w->output.top;
	reg.extents.x2 = w->attrib.x + w->width + w->output.right;
	reg.extents.y2 = w->attrib.y;

	if (reg.extents.x1 < reg.extents.x2 && reg.extents.y1 < reg.extents.y2)
	    damageWindowRegion (w, &reg);

	/* bottom */
	reg.extents.y1 = w->attrib.y + w->height;
	reg.extents.y2 = reg.extents.y1 + w->output.bottom;

	if (reg.extents.x1 < reg.extents.x2 && reg.extents.y1 < reg.extents.y2)
	    damageWindowRegion (w, &reg);

	/* left */
	reg.extents.x1 = w->attrib.x - w->output.left;
	reg.extents.y1 = w->attrib.y;
	reg.extents.x2 = w->attrib.x;
	reg.extents.y2 = w->attrib.y + w->height;

	if (reg.extents.x1 < reg.extents.x2 && reg.extents.y1 < reg.extents.y2)
	    damageWindowRegion (w, &reg);

	/* right */
	reg.extents.x1 = w->attrib.x + w->width;
	reg.extents.x2 = reg.extents.x1 + w->output.right;

	if (reg.extents.x1 < reg.extents.x2 && reg.extents.y1 < reg.extents.y2)
	    damageWindowRegion (w, &reg);
    }
}

Bool
damageWindowRect (CompWindow *w,
		  Bool       initial,
		  BoxPtr     rect)
{
    return FALSE;
}

void
addWindowDamage (CompWindow *w)
{
    if (w->screen->damageMask & COMP_SCREEN_DAMAGE_ALL_MASK)
	return;

    if (w->attrib.map_state == IsViewable && w->damaged)
    {
	REGION region;

	region.extents.x1 = -w->output.left - w->attrib.border_width;
	region.extents.y1 = -w->output.top - w->attrib.border_width;
	region.extents.x2 = w->width + w->output.right;
	region.extents.y2 = w->height + w->output.bottom;

	if (!(*w->screen->damageWindowRect) (w, FALSE, &region.extents))
	{
	    region.extents.x1 += w->attrib.x + w->attrib.border_width;
	    region.extents.y1 += w->attrib.y + w->attrib.border_width;
	    region.extents.x2 += w->attrib.x + w->attrib.border_width;
	    region.extents.y2 += w->attrib.y + w->attrib.border_width;

	    region.rects = &region.extents;
	    region.numRects = region.size = 1;

	    damageWindowRegion (w, &region);
	}
    }
}

void
updateWindowRegion (CompWindow *w)
{
    REGION     rect;
    XRectangle r, *rects, *shapeRects = 0;
    int	       i, n = 0;

    EMPTY_REGION (w->region);

    if (w->screen->display->shapeExtension)
    {
	int order;

	shapeRects = XShapeGetRectangles (w->screen->display->display, w->id,
					  ShapeBounding, &n, &order);
    }

    if (n < 2)
    {
	r.x      = -w->attrib.border_width;
	r.y      = -w->attrib.border_width;
	r.width  = w->width;
	r.height = w->height;

	rects = &r;
	n = 1;
    }
    else
    {
	rects = shapeRects;
    }

    rect.rects = &rect.extents;
    rect.numRects = rect.size = 1;

    for (i = 0; i < n; i++)
    {
	rect.extents.x1 = rects[i].x + w->attrib.border_width;
	rect.extents.y1 = rects[i].y + w->attrib.border_width;
	rect.extents.x2 = rect.extents.x1 + rects[i].width;
	rect.extents.y2 = rect.extents.y1 + rects[i].height;

	if (rect.extents.x1 < 0)
	    rect.extents.x1 = 0;
	if (rect.extents.y1 < 0)
	    rect.extents.y1 = 0;
	if (rect.extents.x2 > w->width)
	    rect.extents.x2 = w->width;
	if (rect.extents.y2 > w->height)
	    rect.extents.y2 = w->height;

	if (rect.extents.y1 < rect.extents.y2 &&
	    rect.extents.x1 < rect.extents.x2)
	{
	    rect.extents.x1 += w->attrib.x;
	    rect.extents.y1 += w->attrib.y;
	    rect.extents.x2 += w->attrib.x;
	    rect.extents.y2 += w->attrib.y;

	    XUnionRegion (&rect, w->region, w->region);
	}
    }

    if (shapeRects)
	XFree (shapeRects);
}

Bool
updateWindowStruts (CompWindow *w)
{
    Atom	  actual;
    int		  result, format;
    unsigned long n, left;
    unsigned long *struts = NULL;
    Bool	  hasOld, hasNew;
    CompStruts    old, new;

#define MIN_EMPTY 76

    if (w->struts)
    {
	hasOld = TRUE;

	old.left   = w->struts->left;
	old.right  = w->struts->right;
	old.top    = w->struts->top;
	old.bottom = w->struts->bottom;
    }
    else
    {
	hasOld = FALSE;
    }

    hasNew = FALSE;

    new.left.x	    = 0;
    new.left.y	    = 0;
    new.left.width  = 0;
    new.left.height = w->screen->height;

    new.right.x      = w->screen->width;
    new.right.y      = 0;
    new.right.width  = 0;
    new.right.height = w->screen->height;

    new.top.x	   = 0;
    new.top.y	   = 0;
    new.top.width  = w->screen->width;
    new.top.height = 0;

    new.bottom.x      = 0;
    new.bottom.y      = w->screen->height;
    new.bottom.width  = w->screen->width;
    new.bottom.height = 0;

    result = XGetWindowProperty (w->screen->display->display, w->id,
				 w->screen->display->wmStrutPartialAtom,
				 0L, 12L, FALSE, XA_CARDINAL, &actual, &format,
				 &n, &left, (unsigned char **) &struts);

    if (result == Success && n && struts)
    {
	if (n == 12)
	{
	    int gap;

	    hasNew = TRUE;

	    gap = w->screen->width - struts[0] - struts[1];
	    gap -= MIN_EMPTY;

	    new.left.width  = (int) struts[0] + MIN (0, gap / 2);
	    new.right.width = (int) struts[1] + MIN (0, gap / 2);

	    gap = w->screen->height - struts[2] - struts[3];
	    gap -= MIN_EMPTY;

	    new.top.height    = (int) struts[2] + MIN (0, gap / 2);
	    new.bottom.height = (int) struts[3] + MIN (0, gap / 2);

	    new.right.x  = w->screen->width  - new.right.width;
	    new.bottom.y = w->screen->height - new.bottom.height;

	    new.left.y       = struts[4];
	    new.left.height  = struts[5] - new.left.y + 1;
	    new.right.y      = struts[6];
	    new.right.height = struts[7] - new.right.y + 1;

	    new.top.x        = struts[8];
	    new.top.width    = struts[9] - new.top.x + 1;
	    new.bottom.x     = struts[10];
	    new.bottom.width = struts[11] - new.bottom.x + 1;
	}

	XFree (struts);
    }

    if (!hasNew)
    {
	result = XGetWindowProperty (w->screen->display->display, w->id,
				     w->screen->display->wmStrutAtom,
				     0L, 4L, FALSE, XA_CARDINAL,
				     &actual, &format, &n, &left,
				     (unsigned char **) &struts);

	if (result == Success && n && struts)
	{
	    if (n == 4)
	    {
		int gap;

		hasNew = TRUE;

		gap = w->screen->width - struts[0] - struts[1];
		gap -= MIN_EMPTY;

		new.left.width  = (int) struts[0] + MIN (0, gap / 2);
		new.right.width = (int) struts[1] + MIN (0, gap / 2);

		gap = w->screen->height - struts[2] - struts[3];
		gap -= MIN_EMPTY;

		new.top.height    = (int) struts[2] + MIN (0, gap / 2);
		new.bottom.height = (int) struts[3] + MIN (0, gap / 2);

		new.left.x  = 0;
		new.right.x = w->screen->width - new.right.width;

		new.top.y    = 0;
		new.bottom.y = w->screen->height - new.bottom.height;
	    }

	    XFree (struts);
	}
    }

    if (hasOld != hasNew || (hasNew && hasOld &&
			     memcmp (&new, &old, sizeof (CompStruts))))
    {
	if (hasNew)
	{
	    if (!w->struts)
		w->struts = malloc (sizeof (CompStruts));

	    *w->struts = new;
	}
	else
	{
	    free (w->struts);
	    w->struts = NULL;
	}

	return TRUE;
    }

    return FALSE;
}

void
addWindow (CompScreen *screen,
	   Window     id,
	   Window     aboveId)
{
    CompWindow *w;

    w = (CompWindow *) malloc (sizeof (CompWindow));
    if (!w)
	return;

    w->next = NULL;
    w->prev = NULL;

    w->mapNum	 = 0;
    w->activeNum = 0;

    w->frame = None;

    w->placed    = FALSE;
    w->minimized = FALSE;

    w->pendingUnmaps = 0;

    w->startupId = NULL;
    w->resName   = NULL;
    w->resClass  = NULL;

    initTexture (screen, &w->texture);

    w->screen    = screen;
    w->pixmap    = None;
    w->destroyed = FALSE;
    w->damaged   = FALSE;

    w->destroyRefCnt = 1;
    w->unmapRefCnt   = 1;

    w->group = NULL;

    w->damageRects = 0;
    w->sizeDamage  = 0;
    w->nDamage	   = 0;

    w->vertices   = 0;
    w->vertexSize = 0;
    w->indices    = 0;
    w->indexSize  = 0;
    w->vCount     = 0;

    w->struts = 0;

    w->input.left   = 0;
    w->input.right  = 0;
    w->input.top    = 0;
    w->input.bottom = 0;

    w->output.left   = 0;
    w->output.right  = 0;
    w->output.top    = 0;
    w->output.bottom = 0;

    w->paint.opacity	= w->opacity    = OPAQUE;
    w->paint.brightness = w->brightness = 0xffff;
    w->paint.saturation = w->saturation = COLOR;
    w->paint.xScale	= 1.0f;
    w->paint.yScale	= 1.0f;

    w->lastPaint = w->paint;

    w->alive = TRUE;

    w->scaled = FALSE;

    w->mwmDecor = MwmDecorAll;

    w->syncAlarm      = None;
    w->syncCounter    = 0;
    w->syncWaitHandle = 0;

    if (screen->windowPrivateLen)
    {
	w->privates = malloc (screen->windowPrivateLen * sizeof (CompPrivate));
	if (!w->privates)
	{
	    free (w);
	    return;
	}
    }
    else
	w->privates = 0;

    w->region = XCreateRegion ();
    if (!w->region)
    {
	freeWindow (w);
	return;
    }

    w->clip = XCreateRegion ();
    if (!w->clip)
    {
	freeWindow (w);
	return;
    }

    if (!XGetWindowAttributes (screen->display->display, id, &w->attrib))
    {
	freeWindow (w);
	return;
    }

    w->width  = w->attrib.width  + w->attrib.border_width * 2;
    w->height = w->attrib.height + w->attrib.border_width * 2;

    w->sizeHints.flags = 0;

    recalcNormalHints (w);

    w->transientFor = None;
    w->clientLeader = None;

    w->serverX = w->attrib.x;
    w->serverY = w->attrib.y;

    w->syncWait	       = FALSE;
    w->syncX	       = w->attrib.x;
    w->syncY	       = w->attrib.y;
    w->syncWidth       = w->attrib.width;
    w->syncHeight      = w->attrib.height;
    w->syncBorderWidth = w->attrib.border_width;

    w->saveMask = 0;

    XSelectInput (screen->display->display, id,
		  PropertyChangeMask |
		  EnterWindowMask    |
		  FocusChangeMask);

    w->id = id;

    XGrabButton (screen->display->display, AnyButton,
		 AnyModifier, w->id, TRUE, ButtonPressMask |
		 ButtonReleaseMask | ButtonMotionMask,
		 GrabModeSync, GrabModeSync, None, None);

    w->inputHint = TRUE;
    w->alpha     = (w->attrib.depth == 32);
    w->wmType    = 0;
    w->state     = 0;
    w->actions   = 0;
    w->protocols = 0;
    w->type      = CompWindowTypeUnknownMask;
    w->lastPong  = screen->display->lastPing;

    if (screen->display->shapeExtension)
	XShapeSelectInput (screen->display->display, id, ShapeNotifyMask);

    insertWindowIntoScreen (screen, w, aboveId);

    EMPTY_REGION (w->region);

    if (w->attrib.class != InputOnly)
    {
	REGION rect;

	rect.rects = &rect.extents;
	rect.numRects = rect.size = 1;

	rect.extents.x1 = w->attrib.x;
	rect.extents.y1 = w->attrib.y;
	rect.extents.x2 = w->attrib.x + w->width;
	rect.extents.y2 = w->attrib.y + w->height;

	XUnionRegion (&rect, w->region, w->region);

	initTexture (screen, &w->texture);

	w->damage = XDamageCreate (screen->display->display, id,
				   XDamageReportRawRectangles);

	/* need to check for DisplayModal state on all windows */
	w->state = getWindowState (w->screen->display, w->id);

	updateWindowClassHints (w);
    }
    else
    {
	w->damage = None;
	w->attrib.map_state = IsUnmapped;
    }

    if (testMode)
    {
	static int useAlpha = 0;

	w->attrib.map_state = IsViewable;
	w->damaged	    = TRUE;

	w->attrib.width  = 0;
	w->attrib.height = 0;

	bindWindow (w);

	w->alpha = useAlpha;
	useAlpha = !useAlpha;
    }

    w->invisible = TRUE;

    if (!w->attrib.override_redirect)
    {
	updateNormalHints (w);
	updateWindowStruts (w);

	updateWmHints (w);

	XGetTransientForHint (w->screen->display->display,
			      w->id, &w->transientFor);

	w->clientLeader = getClientLeader (w);

	w->wmType = getWindowType (w->screen->display, w->id);

	recalcWindowType (w);

	w->mwmDecor  = getMwmDecor (w->screen->display, w->id);
	w->protocols = getProtocols (w->screen->display, w->id);

	if (!(w->type & CompWindowTypeDesktopMask))
	    w->opacity =
		getWindowProp32 (w->screen->display, w->id,
				 w->screen->display->winOpacityAtom,
				 OPAQUE);

	w->brightness =
		getWindowProp32 (w->screen->display, w->id,
				 w->screen->display->winBrightnessAtom,
				 BRIGHT);

	if (w->alive)
	{
	    w->paint.opacity    = w->opacity;
	    w->paint.brightness = w->brightness;
	}

	if (w->screen->canDoSaturated)
	{
	    w->saturation =
		getWindowProp32 (w->screen->display, w->id,
				 w->screen->display->winSaturationAtom,
				 COLOR);
	    if (w->alive)
		w->paint.saturation = w->saturation;
	}
    }

    if (w->attrib.map_state == IsViewable)
    {
	w->attrib.map_state = IsUnmapped;

	mapWindow (w);

	if (!w->attrib.override_redirect)
	    updateWindowAttributes (w);
    }

    windowInitPlugins (w);
}

void
removeWindow (CompWindow *w)
{
    unhookWindowFromScreen (w->screen, w);

    if (w->attrib.map_state == IsViewable && w->damaged)
    {
	if (w->type == CompWindowTypeDesktopMask)
	    w->screen->desktopWindowCount--;

	if (w->struts)
	    updateWorkareaForScreen (w->screen);

	updateClientListForScreen (w->screen);
    }
    else if (w->state & CompWindowStateHiddenMask)
    {
	updateClientListForScreen (w->screen);
    }

    windowFiniPlugins (w);

    freeWindow (w);
}

void
destroyWindow (CompWindow *w)
{
    w->id = 1;
    w->mapNum = 0;

    w->destroyRefCnt--;
    if (w->destroyRefCnt)
	return;

    if (!w->destroyed)
    {
	w->destroyed = TRUE;
	w->screen->pendingDestroys++;
    }
}

void
sendConfigureNotify (CompWindow *w)
{
    XConfigureEvent xev;

    xev.type	     = ConfigureNotify;
    xev.event	     = w->id;
    xev.window	     = w->id;
    xev.x	     = w->attrib.x;
    xev.y	     = w->attrib.y;
    xev.width	     = w->attrib.width;
    xev.height	     = w->attrib.height;
    xev.border_width = w->attrib.border_width;

    xev.above		  = (w->prev) ? w->prev->id : None;
    xev.override_redirect = w->attrib.override_redirect;

    XSendEvent (w->screen->display->display, w->id, FALSE,
		StructureNotifyMask, (XEvent *) &xev);
}

void
mapWindow (CompWindow *w)
{
    if (w->attrib.class == InputOnly)
	return;

    if (w->attrib.map_state == IsViewable)
	return;

    w->unmapRefCnt = 1;

    w->attrib.map_state = IsViewable;

    setWmState (w->screen->display, NormalState, w->id);

    w->invisible = TRUE;
    w->damaged   = FALSE;
    w->alive     = TRUE;

    w->lastPong = w->screen->display->lastPing;

    w->mapNum = w->screen->mapNum++;

    updateWindowRegion (w);
    updateWindowSize (w);

    if (w->frame)
	XMapWindow (w->screen->display->display, w->frame);

    if (w->struts)
	updateWorkareaForScreen (w->screen);

    updateClientListForScreen (w->screen);

    if (w->type & CompWindowTypeDesktopMask)
	w->screen->desktopWindowCount++;

    if (w->attrib.override_redirect)
    {
	if (w->protocols & CompWindowProtocolSyncRequestMask)
	    sendSyncRequest (w);

	/* initial map is handled same as configure request */
	sendConfigureNotify (w);
    }
}

void
unmapWindow (CompWindow *w)
{
    if (w->mapNum)
    {
	if (w->frame)
	    XUnmapWindow (w->screen->display->display, w->frame);

	w->mapNum = 0;
    }

    w->unmapRefCnt--;
    if (w->unmapRefCnt > 0)
	return;

    if (w->attrib.map_state != IsViewable)
	return;

    if (w->type == CompWindowTypeDesktopMask)
	w->screen->desktopWindowCount--;

    addWindowDamage (w);

    w->attrib.map_state = IsUnmapped;

    w->invisible = TRUE;

    releaseWindow (w);

    if (w->struts)
	updateWorkareaForScreen (w->screen);

    updateClientListForScreen (w->screen);
}

static int
restackWindow (CompWindow *w,
	       Window     aboveId)
{
    if (w->prev)
    {
	if (aboveId && aboveId == w->prev->id)
	    return 0;
    }
    else if (aboveId == None)
	return 0;

    unhookWindowFromScreen (w->screen, w);
    insertWindowIntoScreen (w->screen, w, aboveId);

    updateClientListForScreen (w->screen);

    return 1;
}

Bool
resizeWindow (CompWindow *w,
	      int	 x,
	      int	 y,
	      int	 width,
	      int	 height,
	      int	 borderWidth)
{
    Window frame = None;

    if (x != w->serverX)
    {
	frame	   = w->frame;
	w->serverX = x;
    }
    else
	x = w->attrib.x;

    if (y != w->serverY)
    {
	frame	   = w->frame;
	w->serverY = y;
    }
    else
	y = w->attrib.y;

    if (frame)
	XMoveWindow (w->screen->display->display, frame,
		     w->serverX - w->input.left,
		     w->serverY - w->input.top);

    if (w->attrib.width        != width  ||
	w->attrib.height       != height ||
	w->attrib.border_width != borderWidth)
    {
	unsigned int pw, ph, actualWidth, actualHeight, ui;
	Pixmap	     pixmap = None;
	Window	     root;
	Status	     result;
	int	     i;

	pw = width  + borderWidth * 2;
	ph = height + borderWidth * 2;

	if (w->mapNum)
	{
	    pixmap = XCompositeNameWindowPixmap (w->screen->display->display,
						 w->id);
	    if (!pixmap)
	    {
		fprintf (stderr, "%s: XCompositeNameWindowPixmap failed\n",
			 programName);

		return FALSE;
	    }

	    result = XGetGeometry (w->screen->display->display, pixmap, &root,
				   &i, &i, &actualWidth, &actualHeight,
				   &ui, &ui);

	    if (!result || actualWidth != pw || actualHeight != ph)
	    {
		XFreePixmap (w->screen->display->display, pixmap);

		return FALSE;
	    }
	}

	addWindowDamage (w);

	w->attrib.x	       = x;
	w->attrib.y	       = y;
	w->attrib.width	       = width;
	w->attrib.height       = height;
	w->attrib.border_width = borderWidth;

	w->width  = pw;
	w->height = ph;

	releaseWindow (w);

	w->pixmap = pixmap;

	if (w->mapNum)
	    updateWindowRegion (w);

	(*w->screen->windowResizeNotify) (w);

	addWindowDamage (w);

	w->invisible = WINDOW_INVISIBLE (w);

	updateFrameWindow (w);
    }
    else if (w->attrib.x != x || w->attrib.y != y)
    {
	int dx, dy;

	dx = x - w->attrib.x;
	dy = y - w->attrib.y;

	moveWindow (w, dx, dy, TRUE);
    }

    return TRUE;
}

static void
syncValueIncrement (XSyncValue *value)
{
    XSyncValue one;
    int	       overflow;

    XSyncIntToValue (&one, 1);
    XSyncValueAdd (value, *value, one, &overflow);
}

static Bool
initializeSyncCounter (CompWindow *w)
{
    XSyncAlarmAttributes values;
    Atom		 actual;
    int			 result, format;
    unsigned long	 n, left;
    unsigned long	 *counter;

    if (w->syncCounter)
	return w->syncAlarm != None;

    if (!(w->protocols & CompWindowProtocolSyncRequestMask))
	return FALSE;

    result = XGetWindowProperty (w->screen->display->display, w->id,
				 w->screen->display->wmSyncRequestCounterAtom,
				 0L, 1L, FALSE, XA_CARDINAL, &actual, &format,
				 &n, &left, (unsigned char **) &counter);

    if (result == Success && n && counter)
    {
	w->syncCounter = *counter;

	XFree (counter);

	XSyncIntsToValue (&w->syncValue, (unsigned int) rand (), 0);
	XSyncSetCounter (w->screen->display->display,
			 w->syncCounter,
			 w->syncValue);

	syncValueIncrement (&w->syncValue);

	values.events = TRUE;

	values.trigger.counter    = w->syncCounter;
	values.trigger.wait_value = w->syncValue;

	values.trigger.value_type = XSyncAbsolute;
	values.trigger.test_type  = XSyncPositiveComparison;

	XSyncIntToValue (&values.delta, 1);

	values.events = TRUE;

	compCheckForError (w->screen->display->display);

	/* Note that by default, the alarm increments the trigger value
	 * when it fires until the condition (counter.value < trigger.value)
	 * is FALSE again.
	 */
	w->syncAlarm = XSyncCreateAlarm (w->screen->display->display,
					 XSyncCACounter   |
					 XSyncCAValue     |
					 XSyncCAValueType |
					 XSyncCATestType  |
					 XSyncCADelta     |
					 XSyncCAEvents,
					 &values);

	if (!compCheckForError (w->screen->display->display))
	    return TRUE;

	XSyncDestroyAlarm (w->screen->display->display, w->syncAlarm);
	w->syncAlarm = None;
    }

    return FALSE;
}

static Bool
syncWaitTimeout (void *closure)
{
    CompWindow *w = closure;

    w->syncWaitHandle = 0;
    handleSyncAlarm (w);

    return FALSE;
}

void
sendSyncRequest (CompWindow *w)
{
    XClientMessageEvent xev;

    if (w->syncWait)
	return;

    if (!initializeSyncCounter (w))
	return;

    xev.type	     = ClientMessage;
    xev.window	     = w->id;
    xev.message_type = w->screen->display->wmProtocolsAtom;
    xev.format	     = 32;
    xev.data.l[0]    = w->screen->display->wmSyncRequestAtom;
    xev.data.l[1]    = CurrentTime;
    xev.data.l[2]    = XSyncValueLow32 (w->syncValue);
    xev.data.l[3]    = XSyncValueHigh32 (w->syncValue);
    xev.data.l[4]    = 0;

    syncValueIncrement (&w->syncValue);

    XSendEvent (w->screen->display->display, w->id, FALSE, 0, (XEvent *) &xev);

    w->syncWait	       = TRUE;
    w->syncX	       = w->serverX;
    w->syncY	       = w->serverY;
    w->syncWidth       = w->attrib.width;
    w->syncHeight      = w->attrib.height;
    w->syncBorderWidth = w->attrib.border_width;

    if (!w->syncWaitHandle)
	w->syncWaitHandle = compAddTimeout (1000, syncWaitTimeout, w);
}

void
configureWindow (CompWindow	 *w,
		 XConfigureEvent *ce)
{
    if (w->syncWait)
    {
	w->syncX	   = ce->x;
	w->syncY	   = ce->y;
	w->syncWidth       = ce->width;
	w->syncHeight      = ce->height;
	w->syncBorderWidth = ce->border_width;
    }
    else
    {
	resizeWindow (w, ce->x, ce->y, ce->width, ce->height,
		      ce->border_width);
    }

    w->attrib.override_redirect = ce->override_redirect;

    if (restackWindow (w, ce->above))
	addWindowDamage (w);
}

void
circulateWindow (CompWindow	 *w,
		 XCirculateEvent *ce)
{
    Window newAboveId;

    if (ce->place == PlaceOnTop && w->screen->reverseWindows)
	newAboveId = w->screen->reverseWindows->id;
    else
	newAboveId = 0;

    if (restackWindow (w, newAboveId))
	addWindowDamage (w);
}

void
moveWindow (CompWindow *w,
	    int        dx,
	    int        dy,
	    Bool       damage)
{
    if (dx || dy)
    {
	if (damage)
	    addWindowDamage (w);

	w->attrib.x += dx;
	w->attrib.y += dy;

	XOffsetRegion (w->region, dx, dy);

	setWindowMatrix (w);

	w->invisible = WINDOW_INVISIBLE (w);

	(*w->screen->windowMoveNotify) (w, dx, dy);

	if (damage)
	    addWindowDamage (w);
    }
}

void
syncWindowPosition (CompWindow *w)
{
    if (w->attrib.x != w->serverX || w->attrib.y != w->serverY)
    {
	XMoveWindow (w->screen->display->display, w->id,
		     w->attrib.x,
		     w->attrib.y);

	if (w->frame)
	    XMoveWindow (w->screen->display->display, w->frame,
			 w->attrib.x - w->input.left,
			 w->attrib.y - w->input.top);

	if (!w->attrib.override_redirect)
	    sendConfigureNotify (w);
    }
}

void
setWindowScale (CompWindow *w,
		float      xScale,
		float      yScale)
{
    if (xScale > 0.999f && xScale < 1.001f &&
	yScale > 0.999f && yScale < 1.001f)
    {
	w->paint.xScale = 1.0f;
	w->paint.yScale = 1.0f;

	w->scaled = FALSE;
    }
    else
    {
	w->paint.xScale = xScale;
	w->paint.yScale = yScale;

	w->scaled = TRUE;
    }
}

Bool
focusWindow (CompWindow *w)
{
    if (w->attrib.override_redirect)
	return FALSE;

    if (!w->mapNum || w->attrib.map_state != IsViewable)
	return FALSE;

    if (w->attrib.x + w->width  <= 0	||
	w->attrib.y + w->height <= 0	||
	w->attrib.x >= w->screen->width ||
	w->attrib.y >= w->screen->height)
	return FALSE;

    return TRUE;
}

void
windowResizeNotify (CompWindow *w)
{
}

void
windowMoveNotify (CompWindow *w,
		  int	     dx,
		  int	     dy)
{
}

void
windowGrabNotify (CompWindow   *w,
		  int	       x,
		  int	       y,
		  unsigned int state,
		  unsigned int mask)
{
}

void
windowUngrabNotify (CompWindow *w)
{
}

static Bool
isGroupTransient (CompWindow *w,
		  Window     clientLeader)
{
    if (!clientLeader)
	return FALSE;

    if (w->transientFor == None || w->transientFor == w->screen->root)
    {
	if (w->type & (CompWindowTypeDialogMask |
		       CompWindowTypeModalDialogMask))
	{
	    if (w->clientLeader == clientLeader)
		return TRUE;
	}
    }

    return FALSE;
}

static CompWindow *
getModalTransient (CompWindow *window)
{
    CompWindow *w, *modalTransient;

    modalTransient = window;

    for (w = window->screen->reverseWindows; w; w = w->prev)
    {
	if (w == modalTransient || w->mapNum == 0)
	    continue;

	if (w->transientFor == modalTransient->id)
	{
	    if (w->state & CompWindowStateModalMask)
	    {
		modalTransient = w;
		w = window->screen->reverseWindows;
	    }
	}
    }

    if (modalTransient == window)
    {
	for (w = window->screen->reverseWindows; w; w = w->prev)
	{
	    if (w == modalTransient || w->mapNum == 0)
		continue;

	    if (isGroupTransient (w, modalTransient->clientLeader))
	    {
		if (w->state & CompWindowStateModalMask)
		{
		    modalTransient = w;
		    break;
		}
	    }
	}
    }

    if (modalTransient == window)
	modalTransient = NULL;

    return modalTransient;
}

void
moveInputFocusToWindow (CompWindow *w)
{
    CompDisplay *d = w->screen->display;
    CompWindow  *modalTransient;

    modalTransient = getModalTransient (w);
    if (modalTransient)
	w = modalTransient;

    if (w->id != d->activeWindow)
	XSetInputFocus (d->display, w->id, RevertToPointerRoot,
			CurrentTime);
}

static Bool
isAncestorTo (CompWindow *transient,
	      CompWindow *ancestor)
{
    if (transient->transientFor)
    {
	if (transient->transientFor == ancestor->id)
	    return TRUE;

	transient = findWindowAtScreen (transient->screen,
					transient->transientFor);
	if (transient)
	    return isAncestorTo (transient, ancestor);
    }

    return FALSE;
}


static Bool
stackLayerCheck (CompWindow *w,
		 Window	    clientLeader,
		 CompWindow *below)
{
    if (w->transientFor == below->id)
	return TRUE;

    if (isAncestorTo (below, w))
	return FALSE;

    if (clientLeader && below->clientLeader == clientLeader)
	if (isGroupTransient (below, clientLeader))
	    return FALSE;

    if (w->state & CompWindowStateAboveMask)
    {
	return TRUE;
    }
    else if (w->state & CompWindowStateBelowMask)
    {
	if (below->state & CompWindowStateBelowMask)
	    return TRUE;
    }
    else if (!(below->state & CompWindowStateAboveMask))
    {
	return TRUE;
    }

    return FALSE;
}

/* goes through the stack, top-down until we find a window we should
   stack above, normal windows can be stacked above fullscreen windows. */
static CompWindow *
findSiblingBelow (CompWindow *w)
{
    CompWindow   *below;
    Window	 clientLeader = w->clientLeader;
    unsigned int type = w->type;

    /* normal stacking fullscreen windows with below state */
    if ((type & CompWindowTypeFullscreenMask) &&
	(w->state & CompWindowStateBelowMask))
	type = CompWindowTypeNormalMask;

    if (w->transientFor || isGroupTransient (w, clientLeader))
	clientLeader = None;

    for (below = w->screen->reverseWindows; below; below = below->prev)
    {
	if (below == w)
	    continue;

	if (below->attrib.override_redirect)
	    continue;

	if (below->attrib.map_state != IsViewable || below->mapNum == 0)
	    continue;

	/* always above desktop windows */
	if (below->type & CompWindowTypeDesktopMask)
	    return below;

	switch (type) {
	case CompWindowTypeDesktopMask:
	    /* desktop window layer */
	    break;
	case CompWindowTypeFullscreenMask:
	case CompWindowTypeDockMask:
	    /* fullscreen and dock layer */
	    if (below->type & (CompWindowTypeFullscreenMask |
			       CompWindowTypeDockMask))
	    {
		if (stackLayerCheck (w, clientLeader, below))
		    return below;
	    }
	    else
	    {
		return below;
	    }
	    break;
	default:
	    /* fullscreen and normal layer */
	    if (!(below->type & CompWindowTypeDockMask))
	    {
		if (stackLayerCheck (w, clientLeader, below))
		    return below;
	    }
	    break;
	}
    }

    return NULL;
}

/* goes through the stack, top-down and returns the lowest window we
   can stack above. */
static CompWindow *
findLowestSiblingBelow (CompWindow *w)
{
    CompWindow   *below, *lowest = w->screen->reverseWindows;
    Window	 clientLeader = w->clientLeader;
    unsigned int type = w->type;

    /* normal stacking fullscreen windows with below state */
    if ((type & CompWindowTypeFullscreenMask) &&
	(w->state & CompWindowStateBelowMask))
	type = CompWindowTypeNormalMask;

    if (w->transientFor || isGroupTransient (w, clientLeader))
	clientLeader = None;

    for (below = w->screen->reverseWindows; below; below = below->prev)
    {
	if (below == w)
	    continue;

	if (below->attrib.override_redirect)
	    continue;

	if (below->attrib.map_state != IsViewable || below->mapNum == 0)
	    continue;

	/* always above desktop windows */
	if (below->type & CompWindowTypeDesktopMask)
	    return below;

	/* always above ancestor */
	if (isAncestorTo (w, below))
	    return below;

	switch (type) {
	case CompWindowTypeDesktopMask:
	    /* desktop window layer */
	    break;
	case CompWindowTypeFullscreenMask:
	case CompWindowTypeDockMask:
	    /* fullscreen and dock layer */
	    if (below->type & (CompWindowTypeFullscreenMask |
			       CompWindowTypeDockMask))
	    {
		if (!stackLayerCheck (below, clientLeader, w))
		    return lowest;
	    }
	    else
	    {
		return lowest;
	    }
	    break;
	default:
	    /* fullscreen and normal layer */
	    if (!(below->type & CompWindowTypeDockMask))
	    {
		if (!stackLayerCheck (below, clientLeader, w))
		    return lowest;
	    }
	    break;
	}

	lowest = below;
    }

    return lowest;
}

static Bool
validSiblingBelow (CompWindow *w,
		   CompWindow *sibling)
{
    Window	 clientLeader = w->clientLeader;
    unsigned int type = w->type;

    /* normal stacking fullscreen windows with below state */
    if ((type & CompWindowTypeFullscreenMask) &&
	(w->state & CompWindowStateBelowMask))
	type = CompWindowTypeNormalMask;

    if (w->transientFor || isGroupTransient (w, clientLeader))
	clientLeader = None;

    if (sibling == w)
	return FALSE;

    if (sibling->attrib.override_redirect)
	return FALSE;

    if (sibling->attrib.map_state != IsViewable || sibling->mapNum == 0)
	return FALSE;

    /* always above desktop windows */
    if (sibling->type & CompWindowTypeDesktopMask)
	return TRUE;

    switch (type) {
    case CompWindowTypeDesktopMask:
	/* desktop window layer */
	break;
    case CompWindowTypeFullscreenMask:
    case CompWindowTypeDockMask:
	/* fullscreen and dock layer */
	if (sibling->type & (CompWindowTypeFullscreenMask |
			     CompWindowTypeDockMask))
	{
	    if (stackLayerCheck (w, clientLeader, sibling))
		return TRUE;
	}
	else
	{
	    return TRUE;
	}
	break;
    default:
	/* fullscreen and normal layer */
	if (!(sibling->type & CompWindowTypeDockMask))
	{
	    if (stackLayerCheck (w, clientLeader, sibling))
		return TRUE;
	}
	break;
    }

    return FALSE;
}

static void
saveWindowGeometry (CompWindow *w,
		    int	       mask)
{
    int m = mask & ~w->saveMask;

    /* only save geometry if window has been placed */
    if (!w->placed)
	return;

    if (m & CWX)
	w->saveWc.x = w->attrib.x;

    if (m & CWY)
	w->saveWc.y = w->attrib.y;

    if (m & CWWidth)
	w->saveWc.width = w->attrib.width;

    if (m & CWHeight)
	w->saveWc.height = w->attrib.height;

    if (m & CWBorderWidth)
	w->saveWc.border_width = w->attrib.border_width;

    w->saveMask |= m;
}

static int
restoreWindowGeometry (CompWindow     *w,
		       XWindowChanges *xwc,
		       int	      mask)
{
    int m = mask & w->saveMask;

    if (m & CWX)
	xwc->x = w->saveWc.x;

    if (m & CWY)
	xwc->y = w->saveWc.y;

    if (m & CWWidth)
	xwc->width = w->saveWc.width;

    if (m & CWHeight)
	xwc->height = w->saveWc.height;

    if (m & CWBorderWidth)
	xwc->border_width = w->saveWc.border_width;

    w->saveMask &= ~mask;

    return m;
}

static void
configureXWindow (Display	 *dpy,
		  CompWindow	 *w,
		  unsigned int	 valueMask,
		  XWindowChanges *xwc)
{
    XConfigureWindow (w->screen->display->display, w->id,
		      valueMask, xwc);

    if (w->frame && (valueMask & (CWSibling | CWStackMode)))
	XConfigureWindow (w->screen->display->display, w->frame,
			  valueMask & (CWSibling | CWStackMode), xwc);
}

static Bool
stackTransients (CompWindow	*w,
		 CompWindow	*avoid,
		 XWindowChanges *xwc)
{
    CompWindow *t;
    Window     clientLeader = w->clientLeader;

    if (w->transientFor || isGroupTransient (w, clientLeader))
	clientLeader = None;

    for (t = w->screen->reverseWindows; t; t = t->prev)
    {
	if (t == w || t == avoid)
	    continue;

	if (t->transientFor == w->id || isGroupTransient (t, clientLeader))
	{
	    if (!stackTransients (t, avoid, xwc))
		return FALSE;

	    if (xwc->sibling == t->id)
		return FALSE;

	    if (t->mapNum)
		configureXWindow (w->screen->display->display, t,
				  CWSibling | CWStackMode, xwc);
	}
    }

    return TRUE;
}

static void
stackAncestors (CompWindow     *w,
		XWindowChanges *xwc)
{
    if (w->transientFor && xwc->sibling != w->transientFor)
    {
	CompWindow *ancestor;

	ancestor = findWindowAtScreen (w->screen, w->transientFor);
	if (ancestor)
	{
	    if (!stackTransients (ancestor, w, xwc))
		return;

	    if (ancestor->type & CompWindowTypeDesktopMask)
		return;

	    if (ancestor->mapNum)
		configureXWindow (w->screen->display->display, ancestor,
				  CWSibling | CWStackMode,
				  xwc);

	    stackAncestors (ancestor, xwc);
	}
    }
    else if (isGroupTransient (w, w->clientLeader))
    {
	CompWindow *a;

	for (a = w->screen->reverseWindows; a; a = a->prev)
	{
	    if (a->clientLeader == w->clientLeader &&
		a->transientFor == None		   &&
		!isGroupTransient (a, w->clientLeader))
	    {
		if (xwc->sibling == a->id)
		    break;

		if (!stackTransients (a, w, xwc))
		    break;

		if (a->type & CompWindowTypeDesktopMask)
		    continue;

		if (a->mapNum)
		    configureXWindow (w->screen->display->display, a,
				      CWSibling | CWStackMode,
				      xwc);
	    }
	}
    }
}

static int
addWindowSizeChanges (CompWindow     *w,
		      XWindowChanges *xwc)
{
    int mask = 0;

    if (w->type & CompWindowTypeFullscreenMask)
    {
	saveWindowGeometry (w,
			    CWX | CWY | CWWidth | CWHeight |
			    CWBorderWidth);

	xwc->width	  = w->screen->width;
	xwc->height	  = w->screen->height;
	xwc->border_width = 0;

	mask |= CWWidth | CWHeight | CWBorderWidth;
    }
    else
    {
	mask |= restoreWindowGeometry (w, xwc, CWBorderWidth);

	if (w->state & CompWindowStateMaximizedVertMask)
	{
	    saveWindowGeometry (w, CWY | CWHeight);

	    xwc->height = w->screen->workArea.height - w->input.top -
		w->input.bottom - w->attrib.border_width * 2;

	    mask |= CWHeight;
	}
	else
	{
	    mask |= restoreWindowGeometry (w, xwc, CWY | CWHeight);
	}

	if (w->state & CompWindowStateMaximizedHorzMask)
	{
	    saveWindowGeometry (w, CWX | CWWidth);

	    xwc->width = w->screen->workArea.width - w->input.left -
		w->input.right - w->attrib.border_width * 2;

	    mask |= CWWidth;
	}
	else
	{
	    mask |= restoreWindowGeometry (w, xwc, CWX | CWWidth);
	}
    }

    if (mask & (CWWidth | CWHeight))
    {
	if (w->type & CompWindowTypeFullscreenMask)
	{
	    xwc->x = 0;
	    xwc->y = 0;

	    mask |= CWX | CWY;
	}
	else
	{
	    int width, height, max;

	    width  = (mask & CWWidth)  ? xwc->width  : w->attrib.width;
	    height = (mask & CWHeight) ? xwc->height : w->attrib.height;

	    xwc->width  = w->attrib.width;
	    xwc->height = w->attrib.height;

	    if (constrainNewWindowSize (w, width, height, &width, &height))
	    {
		xwc->width  = width;
		xwc->height = height;
	    }
	    else
		mask &= ~(CWWidth | CWHeight);

	    if (w->state & CompWindowStateMaximizedVertMask)
	    {
		if (w->attrib.y < w->screen->workArea.y + w->input.top)
		{
		    xwc->y = w->screen->workArea.y + w->input.top;
		    mask |= CWY;
		}
		else
		{
		    height = xwc->height + w->attrib.border_width * 2;

		    max = w->screen->workArea.y + w->screen->workArea.height;
		    if (w->attrib.y + height + w->input.bottom > max)
		    {
			xwc->y = max - height - w->input.bottom;
			mask |= CWY;
		    }
		}
	    }

	    if (w->state & CompWindowStateMaximizedHorzMask)
	    {
		if (w->attrib.x < w->screen->workArea.x + w->input.left)
		{
		    xwc->x = w->screen->workArea.x + w->input.left;
		    mask |= CWX;
		}
		else
		{
		    width = xwc->width + w->attrib.border_width * 2;

		    max = w->screen->workArea.x + w->screen->workArea.width;
		    if (w->attrib.x + width + w->input.right > max)
		    {
			xwc->x = max - width - w->input.right;
			mask |= CWX;
		    }
		}
	    }
	}
    }

    return mask;
}

void
updateWindowSize (CompWindow *w)
{
    XWindowChanges xwc;
    int		   mask;

    mask = addWindowSizeChanges (w, &xwc);
    if (mask)
	configureXWindow (w->screen->display->display, w, mask, &xwc);
}

static int
addWindowStackChanges (CompWindow     *w,
		       XWindowChanges *xwc,
		       CompWindow     *sibling)
{
    int	mask = 0;

    if (!sibling || sibling->id != w->id)
    {
	if (w->prev)
	{
	    if (!sibling)
	    {
		XLowerWindow (w->screen->display->display, w->id);
		if (w->frame)
		    XLowerWindow (w->screen->display->display, w->frame);
	    }
	    else if (sibling->id != w->prev->id)
	    {
		mask |= CWSibling | CWStackMode;

		xwc->stack_mode = Above;
		xwc->sibling    = sibling->id;
	    }
	}
	else if (sibling)
	{
	    mask |= CWSibling | CWStackMode;

	    xwc->stack_mode = Above;
	    xwc->sibling    = sibling->id;
	}
    }

    if (sibling && mask)
    {
	/* a normal window can be stacked above fullscreen windows but we
	   don't wont normal windows to be stacked above dock window so if
	   the sibling we're stacking above is a fullscreen window we also
	   update all dock windows. */
	if ((sibling->type & CompWindowTypeFullscreenMask) &&
	    (!(w->type & (CompWindowTypeFullscreenMask |
			  CompWindowTypeDockMask))))
	{
	    CompWindow *dw;

	    for (dw = w->screen->reverseWindows; dw; dw = dw->prev)
		if (dw == sibling)
		    break;

	    for (; dw; dw = dw->prev)
		if (dw->type & CompWindowTypeDockMask)
		    configureXWindow (w->screen->display->display, dw,
				      mask, xwc);
	}
    }

    return mask;
}

void
raiseWindow (CompWindow *w)
{
    XWindowChanges xwc;
    int		   mask;

    mask = addWindowStackChanges (w, &xwc, findSiblingBelow (w));
    if (mask)
	configureXWindow (w->screen->display->display, w, mask, &xwc);
}

void
lowerWindow (CompWindow *w)
{
    XWindowChanges xwc;
    int		   mask;

    mask = addWindowStackChanges (w, &xwc, findLowestSiblingBelow (w));
    if (mask)
	configureXWindow (w->screen->display->display, w, mask, &xwc);
}

void
restackWindowAbove (CompWindow *w,
		    CompWindow *sibling)
{
    if (validSiblingBelow (w, sibling))
    {
	XWindowChanges xwc;
	int	       mask;

	mask = addWindowStackChanges (w, &xwc, sibling);
	if (mask)
	    configureXWindow (w->screen->display->display, w, mask, &xwc);
    }
}

void
restackWindowBelow (CompWindow *w,
		    CompWindow *sibling)
{
    for (sibling = sibling->prev; sibling; sibling = sibling->prev)
    {
	if (sibling->attrib.override_redirect)
	    continue;

	if (sibling->mapNum == 0)
	    continue;

	break;
    }
    
    if (sibling)
	restackWindowAbove (w, sibling);
}

void
updateWindowAttributes (CompWindow *w)
{
    XWindowChanges xwc;
    int		   mask;

    if (w->state & CompWindowStateHiddenMask)
	return;

    mask = addWindowStackChanges (w, &xwc, findSiblingBelow (w));

    /* only update fullscreen and maximized size if window is visible on
       current viewport. Size is updated once we switch to the windows
       viewport. */
    if (w->attrib.x < w->screen->width && w->attrib.x + w->width > 0)
	mask |= addWindowSizeChanges (w, &xwc);

    if (!mask)
	return;

    if (mask & (CWSibling | CWStackMode))
    {
	/* transient children above */
	if (stackTransients (w, NULL, &xwc))
	{
	    configureXWindow (w->screen->display->display, w, mask, &xwc);

	    /* ancestors, sibilings and sibiling transients below */
	    stackAncestors (w, &xwc);
	}
    }
    else
    {
	configureXWindow (w->screen->display->display, w, mask, &xwc);
    }
}

static void
ensureWindowVisibility (CompWindow *w)
{
    int x1, y1, x2, y2;
    int dx = 0;
    int dy = 0;

    if (w->struts || w->attrib.override_redirect)
	return;

    if (w->type & (CompWindowTypeDockMask | CompWindowTypeUnknownMask))
	return;

    x1 = w->screen->workArea.x - w->screen->width * w->screen->x;
    y1 = w->screen->workArea.y;
    x2 = x1 + w->screen->workArea.width + w->screen->size * w->screen->width;
    y2 = y1 + w->screen->workArea.height;

    if (w->attrib.x - w->input.left >= x2)
	dx = (x2 - 25) - w->attrib.x;
    else if (w->attrib.x + w->width + w->input.right <= x1)
	dx = (x1 + 25) - (w->attrib.x + w->width);

    if (w->attrib.y - w->input.top >= y2)
	dy = (y2 - 25) - w->attrib.y;
    else if (w->attrib.y + w->height + w->input.bottom <= y1)
	dy = (y1 + 25) - (w->attrib.y + w->height);

    if (dx || dy)
    {
	moveWindow (w, dx, dy, TRUE);
	syncWindowPosition (w);
    }
}

void
activateWindow (CompWindow *w)
{
    if (w->state & CompWindowStateHiddenMask)
    {
	if (w->minimized)
	    unminimizeWindow (w);

	if (w->screen->showingDesktopMask)
	    leaveShowDesktopMode (w->screen);

	showWindow (w);
    }

    if (w->state & CompWindowStateHiddenMask)
	return;

    ensureWindowVisibility (w);
    updateWindowAttributes (w);
    moveInputFocusToWindow (w);
}

void
closeWindow (CompWindow *w)
{
    CompDisplay *display = w->screen->display;

    if (w->protocols & CompWindowProtocolDeleteMask)
    {
	XEvent ev;

	ev.type			= ClientMessage;
	ev.xclient.window	= w->id;
	ev.xclient.message_type = display->wmProtocolsAtom;
	ev.xclient.format	= 32;
	ev.xclient.data.l[0]    = display->wmDeleteWindowAtom;
	ev.xclient.data.l[1]    = CurrentTime;
	ev.xclient.data.l[2]    = 0;
	ev.xclient.data.l[3]    = 0;
	ev.xclient.data.l[4]    = 0;

	XSendEvent (display->display, w->id, FALSE, NoEventMask, &ev);
    }
    else
    {
	XKillClient (display->display, w->id);
    }
}

void
getOuterRectOfWindow (CompWindow *w,
		      XRectangle *r)
{
    r->x      = w->attrib.x - w->input.left;
    r->y      = w->attrib.y - w->input.top;
    r->width  = w->width  + w->input.left + w->input.right;
    r->height = w->height + w->input.top  + w->input.bottom;
}

Bool
constrainNewWindowSize (CompWindow *w,
			int        width,
			int        height,
			int        *newWidth,
			int        *newHeight)
{
    const XSizeHints *hints = &w->sizeHints;
    int		     min_width = 0;
    int		     min_height = 0;
    int		     base_width = 0;
    int		     base_height = 0;
    int		     xinc = 1;
    int		     yinc = 1;
    int		     max_width = MAXSHORT;
    int		     max_height = MAXSHORT;

    /* Ater gdk_window_constrain_size(), which is partially borrowed from fvwm.
     *
     * Copyright 1993, Robert Nation
     *     You may use this code for any purpose, as long as the original
     *     copyright remains in the source code and all documentation
     *
     * which in turn borrows parts of the algorithm from uwm
     */

#define FLOOR(value, base) (((int) ((value) / (base))) * (base))
#define FLOOR64(value, base) (((uint64_t) ((value) / (base))) * (base))
#define CLAMP(v, min, max) ((v) <= (min) ? (min) : (v) >= (max) ? (max) : (v))

    if ((hints->flags & PBaseSize) && (hints->flags & PMinSize))
    {
	base_width = hints->base_width;
	base_height = hints->base_height;
	min_width = hints->min_width;
	min_height = hints->min_height;
    }
    else if (hints->flags & PBaseSize)
    {
	base_width = hints->base_width;
	base_height = hints->base_height;
	min_width = hints->base_width;
	min_height = hints->base_height;
    }
    else if (hints->flags & PMinSize)
    {
	base_width = hints->min_width;
	base_height = hints->min_height;
	min_width = hints->min_width;
	min_height = hints->min_height;
    }

    if (hints->flags & PMaxSize)
    {
	max_width = hints->max_width;
	max_height = hints->max_height;
    }

    if (hints->flags & PResizeInc)
    {
	xinc = MAX (xinc, hints->width_inc);
	yinc = MAX (yinc, hints->height_inc);
    }

    /* clamp width and height to min and max values */
    width  = CLAMP (width, min_width, max_width);
    height = CLAMP (height, min_height, max_height);

    /* shrink to base + N * inc */
    width  = base_width + FLOOR (width - base_width, xinc);
    height = base_height + FLOOR (height - base_height, yinc);

    /* constrain aspect ratio, according to:
     *
     * min_aspect.x       width      max_aspect.x
     * ------------  <= -------- <=  -----------
     * min_aspect.y       height     max_aspect.y
     */
    if (hints->flags & PAspect &&
	hints->min_aspect.y > 0 && hints->max_aspect.x > 0)
    {
	/* Use 64 bit arithmetic to prevent overflow */

	uint64_t min_aspect_x = hints->min_aspect.x;
	uint64_t min_aspect_y = hints->min_aspect.y;
	uint64_t max_aspect_x = hints->max_aspect.x;
	uint64_t max_aspect_y = hints->max_aspect.y;
	uint64_t delta;

	if (min_aspect_x * height > width * min_aspect_y)
	{
	    delta = FLOOR64 (height - width * min_aspect_y / min_aspect_x,
			     yinc);
	    if (height - delta >= min_height)
		height -= delta;
	    else
	    {
		delta = FLOOR64 (height * min_aspect_x / min_aspect_y - width,
				 xinc);
		if (width + delta <= max_width)
		    width += delta;
	    }
	}

	if (width * max_aspect_y > max_aspect_x * height)
	{
	    delta = FLOOR64 (width - height * max_aspect_x / max_aspect_y,
			     xinc);
	    if (width - delta >= min_width)
		width -= delta;
	    else
	    {
		delta = FLOOR64 (width * min_aspect_y / max_aspect_y - height,
				 yinc);
		if (height + delta <= max_height)
		    height += delta;
	    }
	}
    }

#undef FLOOR
#undef FLOOR64

    if (width != w->attrib.width || height != w->attrib.height)
    {
	*newWidth  = width;
	*newHeight = height;

	return TRUE;
    }

    return FALSE;
}

void
hideWindow (CompWindow *w)
{
    if (w->attrib.map_state != IsViewable)
	return;

    if (w->state & CompWindowStateHiddenMask)
	return;

    if (!w->minimized && !(w->type & w->screen->showingDesktopMask))
	return;

    w->state |= CompWindowStateHiddenMask;

    w->pendingUnmaps++;

    XUnmapWindow (w->screen->display->display, w->id);

    setWindowState (w->screen->display, w->state, w->id);
}

void
showWindow (CompWindow *w)
{
    if (!(w->state & CompWindowStateHiddenMask))
	return;

    if (w->minimized || (w->type & w->screen->showingDesktopMask))
	return;

    w->state &= ~CompWindowStateHiddenMask;

    XMapWindow (w->screen->display->display, w->id);

    setWindowState (w->screen->display, w->state, w->id);
}

static void
minimizeTransients (CompWindow *w,
		    void       *closure)
{
    CompWindow *ancestor = closure;

    if (w->transientFor == ancestor->id ||
	isGroupTransient (w, ancestor->clientLeader))
	minimizeWindow (w);
}

void
minimizeWindow (CompWindow *w)
{
    if (!(w->actions & CompWindowActionMinimizeMask))
	return;

    if (!w->minimized)
    {
	w->minimized = TRUE;

	forEachWindowOnScreen (w->screen, minimizeTransients, (void *) w);

	hideWindow (w);
    }
}

static void
unminimizeTransients (CompWindow *w,
		      void       *closure)
{
    CompWindow *ancestor = closure;

    if (w->transientFor == ancestor->id ||
	isGroupTransient (w, ancestor->clientLeader))
	unminimizeWindow (w);
}

void
unminimizeWindow (CompWindow *w)
{
    if (w->minimized)
    {
	w->minimized = FALSE;

	showWindow (w);

	forEachWindowOnScreen (w->screen, unminimizeTransients, (void *) w);
    }
}

void
maximizeWindow (CompWindow *w)
{
    int state = 0;

    if (w->attrib.override_redirect)
	return;

    if (w->actions & CompWindowActionMaximizeHorzMask)
	state |= CompWindowStateMaximizedHorzMask;

    if (w->actions & CompWindowActionMaximizeVertMask)
	state |= CompWindowStateMaximizedVertMask;

    state &= ~w->state;

    if (!state)
	return;

    w->state |= state;

    recalcWindowType (w);

    updateWindowAttributes (w);

    setWindowState (w->screen->display, w->state, w->id);
}

void
unmaximizeWindow (CompWindow *w)
{
    if (w->attrib.override_redirect)
	return;

    if (!(w->state & (CompWindowStateMaximizedHorzMask |
		      CompWindowStateMaximizedVertMask)))
	return;

    w->state &= ~(CompWindowStateMaximizedHorzMask |
		  CompWindowStateMaximizedVertMask);

    recalcWindowType (w);

    updateWindowAttributes (w);

    setWindowState (w->screen->display, w->state, w->id);
}
