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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>

#define XK_MISCELLANY
#include <X11/keysymdef.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/shape.h>

#include <compiz.h>

static unsigned int virtualModMask[] = {
    CompAltMask, CompMetaMask, CompSuperMask, CompHyperMask,
    CompModeSwitchMask, CompNumLockMask, CompScrollLockMask
};

typedef struct _CompTimeout {
    struct _CompTimeout *next;
    int			time;
    int			left;
    CallBackProc	callBack;
    void		*closure;
    CompTimeoutHandle   handle;
} CompTimeout;

static CompTimeout       *timeouts = 0;
static struct timeval    lastTimeout;
static CompTimeoutHandle lastTimeoutHandle = 1;

#define CLICK_TO_FOCUS_DEFAULT TRUE

#define AUTORAISE_DEFAULT TRUE

#define AUTORAISE_DELAY_DEFAULT 1000
#define AUTORAISE_DELAY_MIN	0
#define AUTORAISE_DELAY_MAX	10000

#define NUM_OPTIONS(d) (sizeof ((d)->opt) / sizeof (CompOption))

static char *textureFilter[] = { "Fast", "Good", "Best" };

#define NUM_TEXTURE_FILTER (sizeof (textureFilter) / sizeof (textureFilter[0]))

CompDisplay *compDisplays = 0;

static CompDisplay compDisplay;

static char *displayPrivateIndices = 0;
static int  displayPrivateLen = 0;

static int
reallocDisplayPrivate (int  size,
		       void *closure)
{
    CompDisplay *d = compDisplays;
    void        *privates;

    if (d)
    {
	privates = realloc (d->privates, size * sizeof (CompPrivate));
	if (!privates)
	    return FALSE;

	d->privates = (CompPrivate *) privates;
    }

    return TRUE;
}

int
allocateDisplayPrivateIndex (void)
{
    return allocatePrivateIndex (&displayPrivateLen,
				 &displayPrivateIndices,
				 reallocDisplayPrivate,
				 0);
}

void
freeDisplayPrivateIndex (int index)
{
    freePrivateIndex (displayPrivateLen, displayPrivateIndices, index);
}

static void
compDisplayInitOptions (CompDisplay *display,
			char	    **plugin,
			int	    nPlugin)
{
    CompOption *o;
    int        i;

    o = &display->opt[COMP_DISPLAY_OPTION_ACTIVE_PLUGINS];
    o->name	         = "active_plugins";
    o->shortDesc         = "Active Plugins";
    o->longDesc	         = "List of currently active plugins";
    o->type	         = CompOptionTypeList;
    o->value.list.type   = CompOptionTypeString;
    o->value.list.nValue = nPlugin;
    o->value.list.value  = malloc (sizeof (CompOptionValue) * nPlugin);
    for (i = 0; i < nPlugin; i++)
	o->value.list.value[i].s = strdup (plugin[i]);
    o->rest.s.string     = 0;
    o->rest.s.nString    = 0;

    display->dirtyPluginList = TRUE;

    o = &display->opt[COMP_DISPLAY_OPTION_TEXTURE_FILTER];
    o->name	      = "texture_filter";
    o->shortDesc      = "Texture Filter";
    o->longDesc	      = "Texture filtering";
    o->type	      = CompOptionTypeString;
    o->value.s	      = strdup (defaultTextureFilter);
    o->rest.s.string  = textureFilter;
    o->rest.s.nString = NUM_TEXTURE_FILTER;

    o = &display->opt[COMP_DISPLAY_OPTION_CLICK_TO_FOCUS];
    o->name	      = "click_to_focus";
    o->shortDesc      = "Click To Focus";
    o->longDesc	      = "Click on window moves input focus to it";
    o->type	      = CompOptionTypeBool;
    o->value.b	      = CLICK_TO_FOCUS_DEFAULT;

    o = &display->opt[COMP_DISPLAY_OPTION_AUTORAISE];
    o->name	      = "autoraise";
    o->shortDesc      = "Auto-Raise";
    o->longDesc	      = "Raise selected windows after interval";
    o->type	      = CompOptionTypeBool;
    o->value.b	      = AUTORAISE_DEFAULT;

    o = &display->opt[COMP_DISPLAY_OPTION_AUTORAISE_DELAY];
    o->name	  = "autoraise_delay";
    o->shortDesc  = "Auto-Raise Delay";
    o->longDesc	  = "Interval before raising selected windows";
    o->type	  = CompOptionTypeInt;
    o->value.i	  = AUTORAISE_DELAY_DEFAULT;
    o->rest.i.min = AUTORAISE_DELAY_MIN;
    o->rest.i.max = AUTORAISE_DELAY_MAX;
}

CompOption *
compGetDisplayOptions (CompDisplay *display,
		       int	   *count)
{
    *count = NUM_OPTIONS (display);
    return display->opt;
}

static Bool
setDisplayOption (CompDisplay     *display,
		  char	          *name,
		  CompOptionValue *value)
{
    CompOption *o;
    int	       index;

    o = compFindOption (display->opt, NUM_OPTIONS (display), name, &index);
    if (!o)
	return FALSE;

    switch (index) {
    case COMP_DISPLAY_OPTION_ACTIVE_PLUGINS:
	if (compSetOptionList (o, value))
	{
	    display->dirtyPluginList = TRUE;
	    return TRUE;
	}
	break;
    case COMP_DISPLAY_OPTION_TEXTURE_FILTER:
	if (compSetStringOption (o, value))
	{
	    CompScreen *s;

	    for (s = display->screens; s; s = s->next)
		damageScreen (s);

	    if (strcmp (o->value.s, "Fast") == 0)
		display->textureFilter = GL_NEAREST;
	    else
		display->textureFilter = GL_LINEAR;

	    return TRUE;
	}
	break;
    case COMP_DISPLAY_OPTION_CLICK_TO_FOCUS:
	if (compSetBoolOption (o, value))
	    return TRUE;
	break;
    case COMP_DISPLAY_OPTION_AUTORAISE:
	if (compSetBoolOption (o, value))
	    return TRUE;
	break;
    case COMP_DISPLAY_OPTION_AUTORAISE_DELAY:
	if (compSetIntOption (o, value))
	    return TRUE;
    default:
	break;
    }

    return FALSE;
}

static Bool
setDisplayOptionForPlugin (CompDisplay     *display,
			   char	           *plugin,
			   char	           *name,
			   CompOptionValue *value)
{
    CompPlugin *p;

    p = findActivePlugin (plugin);
    if (p && p->vTable->setDisplayOption)
	return (*p->vTable->setDisplayOption) (display, name, value);

    return FALSE;
}

static Bool
initPluginForDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    return (*p->vTable->initDisplay) (p, d);
}

static void
finiPluginForDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    (*p->vTable->finiDisplay) (p, d);
}

static void
updatePlugins (CompDisplay *d)
{
    CompOption *o;
    CompPlugin *p, **pop = 0;
    int	       nPop, i, j;

    d->dirtyPluginList = FALSE;

    o = &d->opt[COMP_DISPLAY_OPTION_ACTIVE_PLUGINS];
    for (i = 0; i < d->plugin.list.nValue && i < o->value.list.nValue; i++)
    {
	if (strcmp (d->plugin.list.value[i].s, o->value.list.value[i].s))
	    break;
    }

    nPop = d->plugin.list.nValue - i;

    if (nPop)
    {
	pop = malloc (sizeof (CompPlugin *) * nPop);
	if (!pop)
	{
	    (*d->setDisplayOption) (d, o->name, &d->plugin);
	    return;
	}
    }

    for (j = 0; j < nPop; j++)
    {
	pop[j] = popPlugin ();
	d->plugin.list.nValue--;
	free (d->plugin.list.value[d->plugin.list.nValue].s);
    }

    for (; i < o->value.list.nValue; i++)
    {
	p = 0;
	for (j = 0; j < nPop; j++)
	{
	    if (pop[j] && strcmp (pop[j]->vTable->name,
				  o->value.list.value[i].s) == 0)
	    {
		if (pushPlugin (pop[j]))
		{
		    p = pop[j];
		    pop[j] = 0;
		    break;
		}
	    }
	}

	if (p == 0)
	{
	    p = loadPlugin (o->value.list.value[i].s);
	    if (p)
	    {
		if (!pushPlugin (p))
		{
		    unloadPlugin (p);
		    p = 0;
		}
	    }
	}

	if (p)
	{
	    CompOptionValue *value;

	    value = realloc (d->plugin.list.value, sizeof (CompOption) *
			     (d->plugin.list.nValue + 1));
	    if (value)
	    {
		value[d->plugin.list.nValue].s = strdup (p->vTable->name);

		d->plugin.list.value = value;
		d->plugin.list.nValue++;
	    }
	    else
	    {
		p = popPlugin ();
		unloadPlugin (p);
	    }
	}
    }

    for (j = 0; j < nPop; j++)
    {
	if (pop[j])
	    unloadPlugin (pop[j]);
    }

    if (nPop)
	free (pop);

    (*d->setDisplayOption) (d, o->name, &d->plugin);
}

static void
addTimeout (CompTimeout *timeout)
{
    CompTimeout *p = 0, *t;

    for (t = timeouts; t; t = t->next)
    {
	if (timeout->time < t->left)
	    break;

	p = t;
    }

    timeout->next = t;
    timeout->left = timeout->time;

    if (p)
	p->next = timeout;
    else
	timeouts = timeout;
}

CompTimeoutHandle
compAddTimeout (int	     time,
		CallBackProc callBack,
		void	     *closure)
{
    CompTimeout *timeout;

    timeout = malloc (sizeof (CompTimeout));
    if (!timeout)
	return 0;

    timeout->time     = time;
    timeout->callBack = callBack;
    timeout->closure  = closure;
    timeout->handle   = lastTimeoutHandle++;

    if (lastTimeoutHandle == MAXSHORT)
	lastTimeoutHandle = 1;

    if (!timeouts)
	gettimeofday (&lastTimeout, 0);

    addTimeout (timeout);

    return timeout->handle;
}

void
compRemoveTimeout (CompTimeoutHandle handle)
{
    CompTimeout *p = 0, *t;

    for (t = timeouts; t; t = t->next)
    {
	if (t->handle == handle)
	    break;

	p = t;
    }

    if (t)
    {
	if (p)
	    p->next = t->next;
	else
	    timeouts = t->next;

	free (t);
    }
}

#define TIMEVALDIFF(tv1, tv2)						   \
    ((tv1)->tv_sec == (tv2)->tv_sec || (tv1)->tv_usec >= (tv2)->tv_usec) ? \
    ((((tv1)->tv_sec - (tv2)->tv_sec) * 1000000) +			   \
     ((tv1)->tv_usec - (tv2)->tv_usec)) / 1000 :			   \
    ((((tv1)->tv_sec - 1 - (tv2)->tv_sec) * 1000000) +			   \
     (1000000 + (tv1)->tv_usec - (tv2)->tv_usec)) / 1000

static int
getTimeToNextRedraw (CompScreen     *s,
		     struct timeval *lastTv,
		     Bool	    idle)
{
    struct timeval tv;
    int		   diff, next;
    static int     timeMult = 1;

    gettimeofday (&tv, 0);

    diff = TIMEVALDIFF (&tv, lastTv);

    /* handle clock rollback */
    if (diff < 0)
	diff = 0;

    if (idle)
    {
	if (timeMult > 1)
	{
	    s->frameStatus = -1;
	    s->redrawTime = s->optimalRedrawTime;
	    timeMult--;
	}
    }
    else
    {
	if (diff > s->redrawTime)
	{
	    if (s->frameStatus > 0)
		s->frameStatus = 0;

	    next = s->optimalRedrawTime * (timeMult + 1);
	    if (diff > next)
	    {
		s->frameStatus--;
		if (s->frameStatus < -1)
		{
		    timeMult++;
		    s->redrawTime = diff = next;
		}
	    }
	}
	else if (diff < s->redrawTime)
	{
	    if (s->frameStatus < 0)
		s->frameStatus = 0;

	    if (timeMult > 1)
	    {
		next = s->optimalRedrawTime * (timeMult - 1);
		if (diff < next)
		{
		    s->frameStatus++;
		    if (s->frameStatus > 4)
		    {
			timeMult--;
			s->redrawTime = next;
		    }
		}
	    }
	}
    }

    if (diff > s->redrawTime)
	return 0;

    return s->redrawTime - diff;
}

static CompWindow *
findWindowAt (CompDisplay *d,
	      Window      root,
	      int	  x,
	      int	  y)
{
    CompScreen *s;
    CompWindow *w;

    for (s = d->screens; s; s = s->next)
    {
	if (s->root == root && s->maxGrab == 0)
	{
	    for (w = s->reverseWindows; w; w = w->prev)
	    {
		if (x >= w->attrib.x &&
		    y >= w->attrib.y &&
		    x <  w->attrib.x + w->width &&
		    y <  w->attrib.y + w->height)
		    return w;
	    }
	}
    }

    return 0;
}

static Window
translateToRootWindow (CompDisplay *d,
		       Window      child)
{
    CompScreen *s;

    for (s = d->screens; s; s = s->next)
    {
	if (s->root == child || s->grabWindow == child)
	    return s->root;
    }

    return child;
}

void
updateModifierMappings (CompDisplay *d)
{
    XModifierKeymap *modmap;
    unsigned int    modMask[CompModNum];
    int		    i, minKeycode, maxKeycode, keysymsPerKeycode = 0;

    for (i = 0; i < CompModNum; i++)
	modMask[i] = 0;

    XDisplayKeycodes (d->display, &minKeycode, &maxKeycode);
    XGetKeyboardMapping (d->display, minKeycode, (maxKeycode - minKeycode + 1),
			 &keysymsPerKeycode);

    modmap = XGetModifierMapping (d->display);
    if (modmap && modmap->max_keypermod > 0)
    {
	static int maskTable[] = {
	    ShiftMask, LockMask, ControlMask, Mod1Mask,
	    Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
	};
	KeySym keysym;
	int    index, size, mask;

	size = (sizeof (maskTable) / sizeof (int)) * modmap->max_keypermod;

	for (i = 0; i < size; i++)
	{
	    if (!modmap->modifiermap[i])
		continue;

	    index = 0;
	    do
	    {
		keysym = XKeycodeToKeysym (d->display,
					   modmap->modifiermap[i],
					   index++);
	    } while (!keysym && index < keysymsPerKeycode);

	    if (keysym)
	    {
		mask = maskTable[i / modmap->max_keypermod];

		if (keysym == XK_Alt_L ||
		    keysym == XK_Alt_R)
		{
		    modMask[CompModAlt] |= mask;
		}
		else if (keysym == XK_Meta_L ||
			 keysym == XK_Meta_R)
		{
		    modMask[CompModMeta] |= mask;
		}
		else if (keysym == XK_Super_L ||
			 keysym == XK_Super_R)
		{
		    modMask[CompModSuper] |= mask;
		}
		else if (keysym == XK_Hyper_L ||
			 keysym == XK_Hyper_R)
		{
		    modMask[CompModHyper] |= mask;
		}
		else if (keysym == XK_Mode_switch)
		{
		    modMask[CompModModeSwitch] |= mask;
		}
		else if (keysym == XK_Scroll_Lock)
		{
		    modMask[CompModScrollLock] |= mask;
		}
		else if (keysym == XK_Num_Lock)
		{
		    modMask[CompModNumLock] |= mask;
		}
	    }
	}

	if (modmap)
	    XFreeModifiermap (modmap);

	for (i = 0; i < CompModNum; i++)
	{
	    if (!modMask[i])
		modMask[i] = CompNoMask;
	}

	if (memcmp (modMask, d->modMask, sizeof (modMask)))
	{
	    CompScreen *s;

	    memcpy (d->modMask, modMask, sizeof (modMask));

	    d->ignoredModMask = LockMask |
		(modMask[CompModNumLock]    & ~CompNoMask) |
		(modMask[CompModScrollLock] & ~CompNoMask);

	    for (s = d->screens; s; s = s->next)
		updatePassiveGrabs (s);
	}
    }
}

unsigned int
virtualToRealModMask (CompDisplay  *d,
		      unsigned int modMask)
{
    int i;

    for (i = 0; i < CompModNum; i++)
    {
	if (modMask & virtualModMask[i])
	{
	    modMask &= ~virtualModMask[i];
	    modMask |= d->modMask[i];
	}
    }

    return (modMask & ~(CompPressMask | CompReleaseMask));
}

static unsigned int
realToVirtualModMask (CompDisplay  *d,
		      unsigned int modMask)
{
    int i;

    for (i = 0; i < CompModNum; i++)
    {
	if (modMask & d->modMask[i])
	    modMask |= virtualModMask[i];
    }

    return modMask;
}

void
eventLoop (void)
{
    XEvent	   event;
    struct pollfd  ufd;
    int		   timeDiff;
    struct timeval tv;
    Region	   tmpRegion;
    CompDisplay    *display = compDisplays;
    CompScreen	   *s = display->screens;
    int		   timeToNextRedraw = 0;
    CompWindow	   *move = 0;
    int		   px = 0, py = 0;
    int		   moveX = 0, moveY = 0;
    CompTimeout    *t;
    Bool	   idle = TRUE;

    tmpRegion = XCreateRegion ();
    if (!tmpRegion)
    {
	fprintf (stderr, "%s: Couldn't create region\n", programName);
	return;
    }

    ufd.fd = ConnectionNumber (display->display);
    ufd.events = POLLIN;

    for (;;)
    {
	if (display->dirtyPluginList)
	    updatePlugins (display);

	if (restartSignal)
	{
	     execvp (programName, programArgv);
	     exit (1);
	}

	while (XPending (display->display))
	{
	    XNextEvent (display->display, &event);

	    /* translate root window coordinates */
	    if (testMode)
	    {
		Window root, child;

		switch (event.type) {
		case ButtonPress:
		    if (!move)
		    {
			px = event.xbutton.x;
			py = event.xbutton.y;

			move = findWindowAt (display, event.xbutton.window,
					     px, py);
			if (move)
			{
			    moveX = move->attrib.x;
			    moveY = move->attrib.y;
			    XRaiseWindow (display->display, move->id);
			    continue;
			}
		    }
		    /* fall-through */
		case ButtonRelease:
		    move = 0;

		    root = translateToRootWindow (display,
						  event.xbutton.window);
		    XTranslateCoordinates (display->display,
					   event.xbutton.root, root,
					   event.xbutton.x_root,
					   event.xbutton.y_root,
					   &event.xbutton.x_root,
					   &event.xbutton.y_root,
					   &child);
		    event.xbutton.root = root;
		    break;
		case KeyPress:
		case KeyRelease:
		    root = translateToRootWindow (display, event.xkey.window);
		    XTranslateCoordinates (display->display,
					   event.xkey.root, root,
					   event.xkey.x_root,
					   event.xkey.y_root,
					   &event.xkey.x_root,
					   &event.xkey.y_root,
					   &child);
		    event.xkey.root = root;
		    break;
		case MotionNotify:
		    if (move)
		    {
			moveX += event.xbutton.x - px;
			moveY += event.xbutton.y - py;
			px = event.xbutton.x;
			py = event.xbutton.y;

			XMoveWindow (display->display, move->id, moveX, moveY);

			continue;
		    }

		    root = translateToRootWindow (display,
						  event.xmotion.window);
		    XTranslateCoordinates (display->display,
					   event.xmotion.root, root,
					   event.xmotion.x_root,
					   event.xmotion.y_root,
					   &event.xmotion.x_root,
					   &event.xmotion.y_root,
					   &child);
		    event.xmotion.root = root;
		default:
		    break;
		}
	    }

	    /* add virtual modifiers */
	    switch (event.type) {
	    case ButtonPress:
		event.xbutton.state |= CompPressMask;
		event.xbutton.state =
		    realToVirtualModMask (display, event.xbutton.state);
		break;
	    case ButtonRelease:
		event.xbutton.state |= CompReleaseMask;
		event.xbutton.state =
		    realToVirtualModMask (display, event.xbutton.state);
		break;
	    case KeyPress:
		event.xkey.state |= CompPressMask;
		event.xkey.state = realToVirtualModMask (display,
							 event.xkey.state);
		break;
	    case KeyRelease:
		event.xkey.state |= CompReleaseMask;
		event.xkey.state = realToVirtualModMask (display,
							 event.xkey.state);
		break;
	    case MotionNotify:
		event.xmotion.state =
		    realToVirtualModMask (display, event.xmotion.state);
		break;
	    default:
		break;
	    }

	    sn_display_process_event (display->snDisplay, &event);

	    (*display->handleEvent) (display, &event);
	}

	if (s->damageMask)
	{
	    /* sync with server */
	    glFinish ();

	    timeToNextRedraw = getTimeToNextRedraw (s, &s->lastRedraw, idle);
	    if (timeToNextRedraw)
		timeToNextRedraw = poll (&ufd, 1, timeToNextRedraw);

	    if (timeToNextRedraw == 0)
	    {
		gettimeofday (&tv, 0);

		timeDiff = TIMEVALDIFF (&tv, &s->lastRedraw);

		s->stencilRef = 0;

		if (s->slowAnimations)
		{
		    (*s->preparePaintScreen) (s, idle ? 2 :
					      (timeDiff << 1) / s->redrawTime);
		}
		else
		    (*s->preparePaintScreen) (s, idle ? s->redrawTime :
					      timeDiff);

		if (s->damageMask & COMP_SCREEN_DAMAGE_REGION_MASK)
		{
		    XIntersectRegion (s->damage, &s->region, tmpRegion);

		    if (tmpRegion->numRects  == 1	 &&
			tmpRegion->rects->x1 == 0	 &&
			tmpRegion->rects->y1 == 0	 &&
			tmpRegion->rects->x2 == s->width &&
			tmpRegion->rects->y2 == s->height)
			damageScreen (s);
		}

		EMPTY_REGION (s->damage);

		if (s->damageMask & COMP_SCREEN_DAMAGE_ALL_MASK)
		{
		    s->damageMask = 0;

		    (*s->paintScreen) (s,
				       &defaultScreenPaintAttrib,
				       &s->region,
				       PAINT_SCREEN_REGION_MASK |
				       PAINT_SCREEN_FULL_MASK);

		    glXSwapBuffers (s->display->display, s->root);
		}
		else if (s->damageMask & COMP_SCREEN_DAMAGE_REGION_MASK)
		{
		    s->damageMask = 0;

		    if ((*s->paintScreen) (s,
					   &defaultScreenPaintAttrib,
					   tmpRegion,
					   PAINT_SCREEN_REGION_MASK))
		    {
			BoxPtr pBox;
			int    nBox, y;

			/*
			pBox = tmpRegion->rects;
			nBox = tmpRegion->numRects;
			while (nBox--)
			{
			    y = s->height - pBox->y2;

			    glXCopySubBufferMESA (s->display->display,
						  s->root,
						  pBox->x1, y,
						  pBox->x2 - pBox->x1,
						  pBox->y2 - pBox->y1);

			    pBox++;
			}

			*/ /* ugly empty rect flush hack */ /*
			glXCopySubBufferMESA (s->display->display, s->root,
					      0, 0, 0, 0);
			*/

			glEnable (GL_SCISSOR_TEST);
			glDrawBuffer (GL_FRONT);

			pBox = tmpRegion->rects;
			nBox = tmpRegion->numRects;
			while (nBox--)
			{
			    y = s->height - pBox->y2;

			    glBitmap (0, 0, 0, 0,
				      pBox->x1 - s->rasterX, y - s->rasterY,
				      NULL);

			    s->rasterX = pBox->x1;
			    s->rasterY = y;

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
		    else
		    {
			(*s->paintScreen) (s,
					   &defaultScreenPaintAttrib,
					   &s->region,
					   PAINT_SCREEN_FULL_MASK);

			glXSwapBuffers (s->display->display, s->root);
		    }
		}

		s->lastRedraw = tv;

		(*s->donePaintScreen) (s);

		/* remove destroyed windows */
		while (s->pendingDestroys)
		{
		    CompWindow *w;

		    for (w = s->windows; w; w = w->next)
		    {
			if (w->destroyed)
			{
			    addWindowDamage (w);
			    removeWindow (w);
			    break;
			}
		    }

		    s->pendingDestroys--;
		}
	    }

	    idle = FALSE;
	}
	else
	{
	    if (timeouts)
	    {
		if (timeouts->left > 0)
		    poll (&ufd, 1, timeouts->left);

		gettimeofday (&tv, 0);

		timeDiff = TIMEVALDIFF (&tv, &lastTimeout);

		/* handle clock rollback */
		if (timeDiff < 0)
		    timeDiff = 0;

		for (t = timeouts; t; t = t->next)
		    t->left -= timeDiff;

		while (timeouts && timeouts->left <= 0)
		{
		    t = timeouts;
		    if ((*t->callBack) (t->closure))
		    {
			timeouts = t->next;
			addTimeout (t);
		    }
		    else
		    {
			timeouts = t->next;
			free (t);
		    }
		}

		lastTimeout = tv;
	    }
	    else
	    {
		poll (&ufd, 1, 1000);
	    }

	    idle = TRUE;
	}
    }
}

static int errors = 0;

static int
errorHandler (Display     *dpy,
	      XErrorEvent *e)
{

#ifdef DEBUG
    char str[128];
    char *name = 0;
    int  o;
#endif

    errors++;

#ifdef DEBUG
    XGetErrorDatabaseText (dpy, "XlibMessage", "XError", "", str, 128);
    fprintf (stderr, "%s", str);

    o = e->error_code - compDisplays->damageError;
    switch (o) {
    case BadDamage:
	name = "BadDamage";
	break;
    default:
	break;
    }

    if (name)
    {
	fprintf (stderr, ": %s\n  ", name);
    }
    else
    {
	XGetErrorText (dpy, e->error_code, str, 128);
	fprintf (stderr, ": %s\n  ", str);
    }

    XGetErrorDatabaseText (dpy, "XlibMessage", "MajorCode", "%d", str, 128);
    fprintf (stderr, str, e->request_code);

    sprintf (str, "%d", e->request_code);
    XGetErrorDatabaseText (dpy, "XRequest", str, "", str, 128);
    if (strcmp (str, ""))
	fprintf (stderr, " (%s)", str);
    fprintf (stderr, "\n  ");

    XGetErrorDatabaseText (dpy, "XlibMessage", "MinorCode", "%d", str, 128);
    fprintf (stderr, str, e->minor_code);
    fprintf (stderr, "\n  ");

    XGetErrorDatabaseText (dpy, "XlibMessage", "ResourceID", "%d", str, 128);
    fprintf (stderr, str, e->resourceid);
    fprintf (stderr, "\n");

    /* abort (); */
#endif

    return 0;
}

int
compCheckForError (Display *dpy)
{
    int e;

    XSync (dpy, FALSE);

    e = errors;
    errors = 0;

    return e;
}

#define PING_DELAY 5000

static Bool
pingTimeout (void *closure)
{
    CompDisplay *d = closure;
    CompScreen  *s;
    CompWindow  *w;
    XEvent      ev;
    int		ping = d->lastPing + 1;

    ev.type		    = ClientMessage;
    ev.xclient.window	    = 0;
    ev.xclient.message_type = d->wmProtocolsAtom;
    ev.xclient.format	    = 32;
    ev.xclient.data.l[0]    = d->wmPingAtom;
    ev.xclient.data.l[1]    = ping;
    ev.xclient.data.l[2]    = 0;
    ev.xclient.data.l[3]    = 0;
    ev.xclient.data.l[4]    = 0;

    for (s = d->screens; s; s = s->next)
    {
	for (w = s->windows; w; w = w->next)
	{
	    if (w->attrib.map_state != IsViewable)
		continue;

	    if (!(w->type & CompWindowTypeNormalMask))
		continue;

	    if (w->protocols & CompWindowProtocolPingMask)
	    {
		if (w->transientFor)
		    continue;

		if (w->lastPong < d->lastPing)
		{
		    if (w->alive)
		    {
			w->alive	    = FALSE;
			w->paint.brightness = 0xa8a8;
			w->paint.saturation = 0;

			addWindowDamage (w);
		    }
		}

		ev.xclient.window    = w->id;
		ev.xclient.data.l[2] = w->id;

		XSendEvent (d->display, w->id, FALSE, NoEventMask, &ev);
	    }
	}
    }

    d->lastPing = ping;

    return TRUE;
}

Bool
addDisplay (char *name,
	    char **plugin,
	    int  nPlugin)
{
    CompDisplay *d;
    Display	*dpy;
    Window	focus;
    int		revertTo, i;

    d = &compDisplay;

    if (displayPrivateLen)
    {
	d->privates = malloc (displayPrivateLen * sizeof (CompPrivate));
	if (!d->privates)
	    return FALSE;
    }
    else
	d->privates = 0;

    d->screenPrivateIndices = 0;
    d->screenPrivateLen     = 0;

    for (i = 0; i < CompModNum; i++)
	d->modMask[i] = CompNoMask;

    d->ignoredModMask = LockMask;

    d->plugin.list.type   = CompOptionTypeString;
    d->plugin.list.nValue = 0;
    d->plugin.list.value  = 0;

    compDisplayInitOptions (d, plugin, nPlugin);

    d->textureFilter = GL_LINEAR;
    d->below	     = None;

    d->activeWindow = 0;

    d->autoRaiseHandle = 0;
    d->autoRaiseWindow = None;

    d->display = dpy = XOpenDisplay (name);
    if (!d->display)
    {
	fprintf (stderr, "%s: Couldn't open display %s\n",
		 programName, XDisplayName (name));
	return FALSE;
    }

    snprintf (d->displayString, 255, "DISPLAY=%s", DisplayString (dpy));

#ifdef DEBUG
    XSynchronize (dpy, TRUE);
#endif

    XSetErrorHandler (errorHandler);

    updateModifierMappings (d);

    d->setDisplayOption		 = setDisplayOption;
    d->setDisplayOptionForPlugin = setDisplayOptionForPlugin;

    d->initPluginForDisplay = initPluginForDisplay;
    d->finiPluginForDisplay = finiPluginForDisplay;

    d->handleEvent = handleEvent;

    d->supportedAtom	     = XInternAtom (dpy, "_NET_SUPPORTED", 0);
    d->supportingWmCheckAtom = XInternAtom (dpy, "_NET_SUPPORTING_WM_CHECK", 0);

    d->utf8StringAtom = XInternAtom (dpy, "UTF8_STRING", 0);

    d->wmNameAtom = XInternAtom (dpy, "_NET_WM_NAME", 0);

    d->winTypeAtom	  = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE", 0);
    d->winTypeDesktopAtom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DESKTOP",
					 0);
    d->winTypeDockAtom    = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DOCK", 0);
    d->winTypeToolbarAtom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR",
					 0);
    d->winTypeMenuAtom    = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_MENU", 0);
    d->winTypeUtilAtom    = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_UTILITY",
					 0);
    d->winTypeSplashAtom  = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_SPLASH", 0);
    d->winTypeDialogAtom  = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DIALOG", 0);
    d->winTypeNormalAtom  = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_NORMAL", 0);

    d->winOpacityAtom	 = XInternAtom (dpy, "_NET_WM_WINDOW_OPACITY", 0);
    d->winBrightnessAtom = XInternAtom (dpy, "_NET_WM_WINDOW_BRIGHTNESS", 0);
    d->winSaturationAtom = XInternAtom (dpy, "_NET_WM_WINDOW_SATURATION", 0);

    d->winActiveAtom = XInternAtom (dpy, "_NET_ACTIVE_WINDOW", 0);

    d->workareaAtom = XInternAtom (dpy, "_NET_WORKAREA", 0);

    d->desktopViewportAtom  = XInternAtom (dpy, "_NET_DESKTOP_VIEWPORT", 0);
    d->desktopGeometryAtom  = XInternAtom (dpy, "_NET_DESKTOP_GEOMETRY", 0);
    d->currentDesktopAtom   = XInternAtom (dpy, "_NET_CURRENT_DESKTOP", 0);
    d->numberOfDesktopsAtom = XInternAtom (dpy, "_NET_NUMBER_OF_DESKTOPS", 0);

    d->winStateAtom		    = XInternAtom (dpy, "_NET_WM_STATE", 0);
    d->winStateModalAtom	    =
	XInternAtom (dpy, "_NET_WM_STATE_MODAL", 0);
    d->winStateStickyAtom	    =
	XInternAtom (dpy, "_NET_WM_STATE_STICKY", 0);
    d->winStateMaximizedVertAtom    =
	XInternAtom (dpy, "_NET_WM_STATE_MAXIMIZED_VERT", 0);
    d->winStateMaximizedHorzAtom    =
	XInternAtom (dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", 0);
    d->winStateShadedAtom	    =
	XInternAtom (dpy, "_NET_WM_STATE_SHADED", 0);
    d->winStateSkipTaskbarAtom	    =
	XInternAtom (dpy, "_NET_WM_STATE_SKIP_TASKBAR", 0);
    d->winStateSkipPagerAtom	    =
	XInternAtom (dpy, "_NET_WM_STATE_SKIP_PAGER", 0);
    d->winStateHiddenAtom	    =
	XInternAtom (dpy, "_NET_WM_STATE_HIDDEN", 0);
    d->winStateFullscreenAtom	    =
	XInternAtom (dpy, "_NET_WM_STATE_FULLSCREEN", 0);
    d->winStateAboveAtom	    =
	XInternAtom (dpy, "_NET_WM_STATE_ABOVE", 0);
    d->winStateBelowAtom	    =
	XInternAtom (dpy, "_NET_WM_STATE_BELOW", 0);
    d->winStateDemandsAttentionAtom =
	XInternAtom (dpy, "_NET_WM_STATE_DEMANDS_ATTENTION", 0);
    d->winStateDisplayModalAtom	    =
	XInternAtom (dpy, "_NET_WM_STATE_DISPLAY_MODAL", 0);

    d->winActionMoveAtom	 = XInternAtom (dpy, "_NET_WM_ACTION_MOVE", 0);
    d->winActionResizeAtom	 =
	XInternAtom (dpy, "_NET_WM_ACTION_RESIZE", 0);
    d->winActionStickAtom	 = XInternAtom (dpy, "_NET_WM_ACTION_STICK", 0);
    d->winActionMinimizeAtom	 =
	XInternAtom (dpy, "_NET_WM_ACTION_MINIMIZE", 0);
    d->winActionMaximizeHorzAtom =
	XInternAtom (dpy, "_NET_WM_ACTION_MAXIMIZE_HORZ", 0);
    d->winActionMaximizeVertAtom =
	XInternAtom (dpy, "_NET_WM_ACTION_MAXIMIZE_VERT", 0);
    d->winActionFullscreenAtom	 =
	XInternAtom (dpy, "_NET_WM_ACTION_FULLSCREEN", 0);
    d->winActionCloseAtom	 = XInternAtom (dpy, "_NET_WM_ACTION_CLOSE", 0);

    d->wmAllowedActionsAtom = XInternAtom (dpy, "_NET_WM_ALLOWED_ACTIONS", 0);

    d->wmStrutAtom	  = XInternAtom (dpy, "_NET_WM_STRUT", 0);
    d->wmStrutPartialAtom = XInternAtom (dpy, "_NET_WM_STRUT_PARTIAL", 0);

    d->clientListAtom	      = XInternAtom (dpy, "_NET_CLIENT_LIST", 0);
    d->clientListStackingAtom =
	XInternAtom (dpy, "_NET_CLIENT_LIST_STACKING", 0);

    d->frameExtentsAtom = XInternAtom (dpy, "_NET_FRAME_EXTENTS", 0);
    d->frameWindowAtom  = XInternAtom (dpy, "_NET_FRAME_WINDOW", 0);

    d->wmStateAtom	  = XInternAtom (dpy, "WM_STATE", 0);
    d->wmChangeStateAtom  = XInternAtom (dpy, "WM_CHANGE_STATE", 0);
    d->wmProtocolsAtom	  = XInternAtom (dpy, "WM_PROTOCOLS", 0);
    d->wmClientLeaderAtom = XInternAtom (dpy, "WM_CLIENT_LEADER", 0);

    d->wmDeleteWindowAtom = XInternAtom (dpy, "WM_DELETE_WINDOW", 0);
    d->wmTakeFocusAtom	  = XInternAtom (dpy, "WM_TAKE_FOCUS", 0);
    d->wmPingAtom	  = XInternAtom (dpy, "_NET_WM_PING", 0);
    d->wmSyncRequestAtom  = XInternAtom (dpy, "_NET_WM_SYNC_REQUEST", 0);

    d->wmSyncRequestCounterAtom =
	XInternAtom (dpy, "_NET_WM_SYNC_REQUEST_COUNTER", 0);

    d->closeWindowAtom	    = XInternAtom (dpy, "_NET_CLOSE_WINDOW", 0);
    d->wmMoveResizeAtom	    = XInternAtom (dpy, "_NET_WM_MOVERESIZE", 0);
    d->moveResizeWindowAtom = XInternAtom (dpy, "_NET_MOVERESIZE_WINDOW", 0);
    d->restackWindowAtom    = XInternAtom (dpy, "_NET_RESTACK_WINDOW", 0);

    d->showingDesktopAtom = XInternAtom (dpy, "_NET_SHOWING_DESKTOP", 0);

    d->xBackgroundAtom[0] = XInternAtom (dpy, "_XSETROOT_ID", 0);
    d->xBackgroundAtom[1] = XInternAtom (dpy, "_XROOTPMAP_ID", 0);

    d->panelActionAtom	        = XInternAtom (dpy, "_GNOME_PANEL_ACTION", 0);
    d->panelActionMainMenuAtom  =
	XInternAtom (dpy, "_GNOME_PANEL_ACTION_MAIN_MENU", 0);
    d->panelActionRunDialogAtom =
	XInternAtom (dpy, "_GNOME_PANEL_ACTION_RUN_DIALOG", 0);

    d->mwmHintsAtom = XInternAtom (dpy, "_MOTIF_WM_HINTS", 0);

    d->managerAtom   = XInternAtom (dpy, "MANAGER", 0);
    d->targetsAtom   = XInternAtom (dpy, "TARGETS", 0);
    d->multipleAtom  = XInternAtom (dpy, "MULTIPLE", 0);
    d->timestampAtom = XInternAtom (dpy, "TIMESTAMP", 0);
    d->versionAtom   = XInternAtom (dpy, "VERSION", 0);
    d->atomPairAtom  = XInternAtom (dpy, "ATOM_PAIR", 0);

    d->snDisplay = sn_display_new (dpy, NULL, NULL);
    if (!d->snDisplay)
	return FALSE;

    d->lastPing = 1;

    if (testMode)
    {
	d->compositeOpcode = MAXSHORT;
	d->compositeEvent  = MAXSHORT;
	d->compositeError  = MAXSHORT;

	d->damageEvent = MAXSHORT;
	d->damageError = MAXSHORT;
    }
    else
    {
	int compositeMajor, compositeMinor;

	if (!XQueryExtension (dpy,
			      COMPOSITE_NAME,
			      &d->compositeOpcode,
			      &d->compositeEvent,
			      &d->compositeError))
	{
	    fprintf (stderr, "%s: No composite extension\n", programName);
	    return FALSE;
	}

	XCompositeQueryVersion (dpy, &compositeMajor, &compositeMinor);
	if (compositeMajor == 0 && compositeMinor < 2)
	{
	    fprintf (stderr, "%s: Old composite extension\n", programName);
	    return FALSE;
	}

	if (!XDamageQueryExtension (dpy, &d->damageEvent, &d->damageError))
	{
	    fprintf (stderr, "%s: No damage extension\n", programName);
	    return FALSE;
	}

	if (!XRRQueryExtension (dpy, &d->randrEvent, &d->randrError))
	{
	    fprintf (stderr, "%s: No RandR extension\n", programName);
	    return FALSE;
	}

	if (!XSyncQueryExtension (dpy, &d->syncEvent, &d->syncError))
	{
	    fprintf (stderr, "%s: No sync extension\n", programName);
	    return FALSE;
	}
    }

    d->shapeExtension = XShapeQueryExtension (dpy,
					      &d->shapeEvent,
					      &d->shapeError);

    compDisplays = d;

    if (testMode)
    {
	addScreen (d, 0, None, 0, 0);
    }
    else
    {
	for (i = 0; i < ScreenCount (dpy); i++)
	{
	    Window		 newWmSnOwner = None;
	    Atom		 wmSnAtom = 0;
	    Time		 wmSnTimestamp = 0;
	    XEvent		 event;
	    XSetWindowAttributes attr;
	    Window		 currentWmSnOwner;
	    char		 buf[128];

	    sprintf (buf, "WM_S%d", i);
	    wmSnAtom = XInternAtom (dpy, buf, 0);

	    currentWmSnOwner = XGetSelectionOwner (dpy, wmSnAtom);

	    if (currentWmSnOwner != None)
	    {
		if (!replaceCurrentWm)
		{
		    fprintf (stderr,
			     "%s: Screen %d on display \"%s\" already "
			     "has a window manager; try using the "
			     "--replace option to replace the current "
			     "window manager.\n",
			     programName, i, DisplayString (dpy));

		    continue;
		}

		XSelectInput (dpy, currentWmSnOwner,
			      StructureNotifyMask);
	    }

	    attr.override_redirect = TRUE;
	    attr.event_mask	   = PropertyChangeMask;

	    newWmSnOwner =
		XCreateWindow (dpy, XRootWindow (dpy, i),
			       -100, -100, 1, 1, 0,
			       CopyFromParent, CopyFromParent,
			       CopyFromParent,
			       CWOverrideRedirect | CWEventMask,
			       &attr);

	    XChangeProperty (dpy,
			     newWmSnOwner,
			     d->wmNameAtom,
			     d->utf8StringAtom, 8,
			     PropModeReplace,
			     (unsigned char *) PACKAGE,
			     strlen (PACKAGE));

	    XWindowEvent (dpy,
			  newWmSnOwner,
			  PropertyChangeMask,
			  &event);

	    wmSnTimestamp = event.xproperty.time;

	    XSetSelectionOwner (dpy, wmSnAtom, newWmSnOwner,
				wmSnTimestamp);

	    if (XGetSelectionOwner (dpy, wmSnAtom) != newWmSnOwner)
	    {
		fprintf (stderr,
			 "%s: Could not acquire window manager "
			 "selection on screen %d display \"%s\"\n",
			 programName, i, DisplayString (dpy));

		XDestroyWindow (dpy, newWmSnOwner);

		continue;
	    }

	    /* Send client message indicating that we are now the WM */
	    event.xclient.type	   = ClientMessage;
	    event.xclient.window       = XRootWindow (dpy, i);
	    event.xclient.message_type = d->managerAtom;
	    event.xclient.format       = 32;
	    event.xclient.data.l[0]    = wmSnTimestamp;
	    event.xclient.data.l[1]    = wmSnAtom;
	    event.xclient.data.l[2]    = 0;
	    event.xclient.data.l[3]    = 0;
	    event.xclient.data.l[4]    = 0;

	    XSendEvent (dpy, XRootWindow (dpy, i), FALSE,
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

	    XCompositeRedirectSubwindows (dpy, XRootWindow (dpy, i),
					  CompositeRedirectManual);

	    if (compCheckForError (dpy))
	    {
		fprintf (stderr, "%s: Another composite manager is already "
			 "running on screen: %d\n", programName, i);

		continue;
	    }

	    XSelectInput (dpy, XRootWindow (dpy, i),
			  SubstructureRedirectMask |
			  SubstructureNotifyMask   |
			  StructureNotifyMask      |
			  PropertyChangeMask       |
			  LeaveWindowMask	   |
			  EnterWindowMask	   |
			  KeyPressMask		   |
			  KeyReleaseMask	   |
			  FocusChangeMask	   |
			  ExposureMask);

	    if (compCheckForError (dpy))
	    {
		fprintf (stderr, "%s: Another window manager is "
			 "already running on screen: %d\n",
			 programName, i);

		continue;
	    }

	    if (!addScreen (d, i, newWmSnOwner, wmSnAtom, wmSnTimestamp))
	    {
		fprintf (stderr, "%s: Failed to manage screen: %d\n",
			 programName, i);
	    }
	}
    }

    if (!d->screens)
    {
	fprintf (stderr, "%s: No managable screens found on display %s\n",
		 programName, XDisplayName (name));
	return FALSE;
    }

    XGetInputFocus (dpy, &focus, &revertTo);

    if (focus == None || focus == PointerRoot)
    {
	focusDefaultWindow (d);
    }
    else
    {
	CompWindow *w;

	w = findWindowAtDisplay (d, focus);
	if (w)
	{
	    moveInputFocusToWindow (w);
	}
	else
	    focusDefaultWindow (d);
    }

    d->pingHandle = compAddTimeout (PING_DELAY, pingTimeout, d);

    return TRUE;
}

void
focusDefaultWindow (CompDisplay *d)
{
    CompScreen *s;
    CompWindow *w;
    CompWindow *focus = NULL;

    for (s = d->screens; s; s = s->next)
    {
	for (w = s->reverseWindows; w; w = w->prev)
	{
	    if (w->type & CompWindowTypeDockMask)
		continue;

	    if ((*s->focusWindow) (w))
	    {
		if (focus)
		{
		    if (w->type & (CompWindowTypeNormalMask |
				   CompWindowTypeDialogMask |
				   CompWindowTypeModalDialogMask))
		    {
			if (w->activeNum > focus->activeNum)
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
	if (focus->id != d->activeWindow)
	    moveInputFocusToWindow (focus);
    }
    else
    {
	XSetInputFocus (d->display, d->screens->root, RevertToPointerRoot,
			CurrentTime);
    }
}

CompScreen *
findScreenAtDisplay (CompDisplay *d,
		     Window      root)
{
    CompScreen *s;

    for (s = d->screens; s; s = s->next)
    {
	if (s->root == root)
	    return s;
    }

    return 0;
}

void
forEachWindowOnDisplay (CompDisplay	  *display,
			ForEachWindowProc proc,
			void		  *closure)
{
    CompScreen *s;

    for (s = display->screens; s; s = s->next)
	forEachWindowOnScreen (s, proc, closure);
}

CompWindow *
findWindowAtDisplay (CompDisplay *d,
		     Window      id)
{
    if (lastFoundWindow && lastFoundWindow->id == id)
    {
	return lastFoundWindow;
    }
    else
    {
	CompScreen *s;
	CompWindow *w;

	for (s = d->screens; s; s = s->next)
	{
	    w = findWindowAtScreen (s, id);
	    if (w)
		return w;
	}
    }

    return 0;
}

static CompScreen *
findScreenForSelection (CompDisplay *display,
			Window       owner,
			Atom         selection)
{
    CompScreen *s;

    for (s = display->screens; s; s = s->next)
    {
	if (s->wmSnSelectionWindow == owner && s->wmSnAtom == selection)
	    return s;
    }

    return NULL;
}

/* from fvwm2, Copyright Matthias Clasen, Dominik Vogt */
static Bool
convertProperty (CompDisplay *display,
		 CompScreen  *screen,
		 Window      w,
		 Atom        target,
		 Atom        property)
{

#define N_TARGETS 4

    Atom conversionTargets[N_TARGETS];
    long icccmVersion[] = { 2, 0 };

    conversionTargets[0] = display->targetsAtom;
    conversionTargets[1] = display->multipleAtom;
    conversionTargets[2] = display->timestampAtom;
    conversionTargets[3] = display->versionAtom;

    if (target == display->targetsAtom)
	XChangeProperty (display->display, w, property,
			 XA_ATOM, 32, PropModeReplace,
			 (unsigned char *) conversionTargets, N_TARGETS);
    else if (target == display->timestampAtom)
	XChangeProperty (display->display, w, property,
			 XA_INTEGER, 32, PropModeReplace,
			 (unsigned char *) &screen->wmSnTimestamp, 1);
    else if (target == display->versionAtom)
	XChangeProperty (display->display, w, property,
			 XA_INTEGER, 32, PropModeReplace,
			 (unsigned char *) icccmVersion, 2);
    else
	return FALSE;

    /* Be sure the PropertyNotify has arrived so we
     * can send SelectionNotify
     */
    XSync (display->display, FALSE);

    return TRUE;
}

/* from fvwm2, Copyright Matthias Clasen, Dominik Vogt */
void
handleSelectionRequest (CompDisplay *display,
			XEvent      *event)
{
    XSelectionEvent reply;
    CompScreen      *screen;

    screen = findScreenForSelection (display,
				     event->xselectionrequest.owner,
				     event->xselectionrequest.selection);
    if (!screen)
	return;

    reply.type	    = SelectionNotify;
    reply.display   = display->display;
    reply.requestor = event->xselectionrequest.requestor;
    reply.selection = event->xselectionrequest.selection;
    reply.target    = event->xselectionrequest.target;
    reply.property  = None;
    reply.time	    = event->xselectionrequest.time;

    if (event->xselectionrequest.target == display->multipleAtom)
    {
	if (event->xselectionrequest.property != None)
	{
	    Atom	  type, *adata;
	    int		  i, format;
	    unsigned long num, rest;
	    unsigned char *data;

	    if (XGetWindowProperty (display->display,
				    event->xselectionrequest.requestor,
				    event->xselectionrequest.property,
				    0, 256, FALSE,
				    display->atomPairAtom,
				    &type, &format, &num, &rest,
				    &data) != Success)
		return;

	    /* FIXME: to be 100% correct, should deal with rest > 0,
	     * but since we have 4 possible targets, we will hardly ever
	     * meet multiple requests with a length > 8
	     */
	    adata = (Atom *) data;
	    i = 0;
	    while (i < (int) num)
	    {
		if (!convertProperty (display, screen,
				      event->xselectionrequest.requestor,
				      adata[i], adata[i + 1]))
		    adata[i + 1] = None;

		i += 2;
	    }

	    XChangeProperty (display->display,
			     event->xselectionrequest.requestor,
			     event->xselectionrequest.property,
			     display->atomPairAtom,
			     32, PropModeReplace, data, num);
	}
    }
    else
    {
	if (event->xselectionrequest.property == None)
	    event->xselectionrequest.property = event->xselectionrequest.target;

	if (convertProperty (display, screen,
			     event->xselectionrequest.requestor,
			     event->xselectionrequest.target,
			     event->xselectionrequest.property))
	    reply.property = event->xselectionrequest.property;
    }

    XSendEvent (display->display,
		event->xselectionrequest.requestor,
		FALSE, 0L, (XEvent *) &reply);
}

void
handleSelectionClear (CompDisplay *display,
		      XEvent      *event)
{
    /* We need to unmanage the screen on which we lost the selection */
    CompScreen *screen;

    screen = findScreenForSelection (display,
				     event->xselectionclear.window,
				     event->xselectionclear.selection);

    if (!screen)
	return;

    /* removeScreen (screen); */

    exit (0);
}
