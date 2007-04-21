/*
 * winrules plugin for compiz
 *
 * Copyright (C) 2007 Bellegarde Cedric (gnumdk (at) gmail.com)
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <compiz.h>

#include <X11/Xatom.h>

#define WINRULES_TARGET_WINDOWS (CompWindowTypeNormalMask |	\
				 CompWindowTypeDialogMask |	\
				 CompWindowTypeModalDialogMask |\
				 CompWindowTypeFullscreenMask |	\
				 CompWindowTypeUnknownMask)


#define WINRULES_SCREEN_OPTION_SKIPTASKBAR_MATCH  0
#define WINRULES_SCREEN_OPTION_SKIPPAGER_MATCH	  1
#define WINRULES_SCREEN_OPTION_ABOVE_MATCH	  2
#define WINRULES_SCREEN_OPTION_BELOW_MATCH        3
#define WINRULES_SCREEN_OPTION_STICKY_MATCH       4
#define WINRULES_SCREEN_OPTION_FULLSCREEN_MATCH   5
#define WINRULES_SCREEN_OPTION_WIDGET_MATCH       6
#define WINRULES_SCREEN_OPTION_MOVE_MATCH         7
#define WINRULES_SCREEN_OPTION_RESIZE_MATCH       8
#define WINRULES_SCREEN_OPTION_MINIMIZE_MATCH     9
#define WINRULES_SCREEN_OPTION_MAXIMIZE_MATCH     10
#define WINRULES_SCREEN_OPTION_CLOSE_MATCH        11
#define WINRULES_SCREEN_OPTION_NOFOCUS_MATCH      12
#define WINRULES_SCREEN_OPTION_SIZE_MATCHES	  13
#define WINRULES_SCREEN_OPTION_SIZE_WIDTH_VALUES  14
#define WINRULES_SCREEN_OPTION_SIZE_HEIGHT_VALUES 15
#define WINRULES_SCREEN_OPTION_NUM		  16

#define WINRULES_SIMPLE_MATCH_OPTION_NUM	  12

static int displayPrivateIndex;

typedef struct _WinrulesWindow {

    unsigned int allowedActions;

    /* only remove if set by us*/
    Bool shouldRemoveSkipTaskbar;
    Bool shouldRemoveSkipPager;
    Bool shouldRemoveAbove;
    Bool shouldRemoveBelow;
    Bool shouldRemoveSticky;
    Bool shouldRemoveFullscreen;
    Bool shouldRemoveNofocus;
    Bool shouldRemoveWidget;

    Bool firstMap;
} WinrulesWindow;

typedef struct _WinrulesDisplay {
    int screenPrivateIndex;
    HandleEventProc handleEvent;
} WinrulesDisplay;

typedef struct _WinrulesScreen {
    int windowPrivateIndex;
    GetAllowedActionsForWindowProc getAllowedActionsForWindow;
    CompOption opt[WINRULES_SCREEN_OPTION_NUM];
} WinrulesScreen;

#define GET_WINRULES_DISPLAY(d)				     			   \
    ((WinrulesDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define WINRULES_DISPLAY(d)			   				   \
    WinrulesDisplay *wd = GET_WINRULES_DISPLAY (d)

#define GET_WINRULES_SCREEN(s, wd)					 	   \
    ((WinrulesScreen *) (s)->privates[(wd)->screenPrivateIndex].ptr)

#define WINRULES_SCREEN(s)							   \
    WinrulesScreen *ws = GET_WINRULES_SCREEN (s, GET_WINRULES_DISPLAY (s->display))

#define GET_WINRULES_WINDOW(w, ws)                                   		   \
    ((WinrulesWindow *) (w)->privates[(ws)->windowPrivateIndex].ptr)
    
#define WINRULES_WINDOW(w)							   \
    WinrulesWindow *ww = GET_WINRULES_WINDOW  (w,				   \
					    GET_WINRULES_SCREEN  (w->screen,	   \
					    GET_WINRULES_DISPLAY (w->screen->display)))

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))


static void
winrulesSetProtocols (CompDisplay *display,
		      unsigned int protocols,
		      Window id)
{
    Atom *protocol = NULL;
    int count = 0;

    if (protocols & CompWindowProtocolDeleteMask)
    {
	protocol = realloc (protocol, (count + 1) * sizeof(Atom));
	protocol[count++] = display->wmDeleteWindowAtom;
    }
    if (protocols & CompWindowProtocolTakeFocusMask)
    {
	protocol = realloc (protocol, (count + 1) * sizeof(Atom));
	protocol[count++] = display->wmTakeFocusAtom;
    }
    if (protocols & CompWindowProtocolPingMask)
    {
	protocol = realloc (protocol, (count + 1) * sizeof(Atom));
	protocol[count++] = display->wmPingAtom;
    }
    if (protocols & CompWindowProtocolSyncRequestMask)
    {
	protocol = realloc (protocol, (count + 1) * sizeof(Atom));
	protocol[count] = display->wmSyncRequestAtom;
    }
    XSetWMProtocols (display->display,
		     id,
		     protocol,
		     count);

    XFree (protocol);
}

/* FIXME? Directly set inputHint, not a problem for now */
static void
winrulesSetNoFocus (CompWindow *w,
		    int optNum,
		    Bool *shouldRemove)
{
    Bool protocolRemoved = FALSE;

    WINRULES_SCREEN (w->screen);

    if (matchEval (&ws->opt[optNum].value.match, w))
    {
	w->protocols &= ~CompWindowProtocolTakeFocusMask;
	w->inputHint = FALSE;
	protocolRemoved = TRUE;
    }
    else if (*shouldRemove)
    {
	w->protocols |= CompWindowProtocolTakeFocusMask;
	w->inputHint = TRUE;
    }

   if (protocolRemoved || *shouldRemove)
   {
	winrulesSetProtocols (w->screen->display,
		      w->protocols,
		      w->id);
	
	*shouldRemove = protocolRemoved;
   }
}

static void
winrulesUpdateState (CompWindow *w,
		     int optNum,
		     int mask,
		     Bool *shouldRemove)
{
    Bool stateRemoved = FALSE;

    WINRULES_SCREEN (w->screen);

    if (matchEval (&ws->opt[optNum].value.match, w))
    {
	w->state |= mask;
	stateRemoved = TRUE;
    }
    else if (*shouldRemove)
	w->state &= ~mask;

    if (stateRemoved || shouldRemove)
    {
	changeWindowState (w, w->state);
	*shouldRemove = stateRemoved;
    }
}

static void
winrulesUpdateWidget (CompWindow *w)
{
    Atom compizWidget = XInternAtom (w->screen->display->display,
				     "_COMPIZ_WIDGET",
				     FALSE);

    WINRULES_SCREEN (w->screen);
    WINRULES_WINDOW (w);

    if (matchEval
	(&ws->opt[WINRULES_SCREEN_OPTION_WIDGET_MATCH].value.match, w))
    {
	if (w->inShowDesktopMode || w->mapNum ||
                w->attrib.map_state == IsViewable || w->minimized)
	{
	    if (w->minimized || w->inShowDesktopMode)
		unminimizeWindow (w);
	    XChangeProperty (w->screen->display->display, w->id, compizWidget,
			     XA_STRING, 8, PropModeReplace,
			     (unsigned char *)(int[]){-2}, 1);
	    ww->shouldRemoveWidget = TRUE;
	}
    }
    else if (ww->shouldRemoveWidget)
    {
	XDeleteProperty (w->screen->display->display, w->id, compizWidget);
	ww->shouldRemoveWidget = FALSE;
    }
}

static void
winrulesSetAllowedActions (CompWindow *w,
			   int optNum,
			   int action)
{
    WINRULES_SCREEN (w->screen);
    WINRULES_WINDOW (w);

    if (matchEval (&ws->opt[optNum].value.match, w))
	ww->allowedActions &= ~action;
    else if ( !(ww->allowedActions & action))
	ww->allowedActions |= action;

    recalcWindowActions (w);
}

static Bool
winrulesMatchSizeValue (CompWindow *w,
			CompOption *matches,
			CompOption *widthValues,
			CompOption *heightValues,
			int	   *width,
			int	   *height)
{
    int i, min;

    if (w->type & CompWindowTypeDesktopMask)
	return FALSE;

    min = MIN (matches->value.list.nValue, widthValues->value.list.nValue);
    min = MIN (min, heightValues->value.list.nValue);

    for (i = 0; i < min; i++)
    {
	if (matchEval (&matches->value.list.value[i].match, w))
	{
	    *width = widthValues->value.list.value[i].i;
	    *height = heightValues->value.list.value[i].i;
	
	    return TRUE;
	}
    }

    return FALSE;
}

static Bool
winrulesMatchSize (CompWindow *w,
		   int	      *width,
		   int	      *height)
{
    WINRULES_SCREEN (w->screen);

    return winrulesMatchSizeValue (w,
				   &ws->opt[WINRULES_SCREEN_OPTION_SIZE_MATCHES],
				   &ws->opt[WINRULES_SCREEN_OPTION_SIZE_WIDTH_VALUES],
				   &ws->opt[WINRULES_SCREEN_OPTION_SIZE_HEIGHT_VALUES],
				   width,
				   height);
}

static void
winrulesUpdateWindowSize (CompWindow *w,
			  int width,
			  int height)
{
    XWindowChanges xwc;
    unsigned int xwcm = 0;

    if (width != w->serverWidth)
	xwcm |= CWWidth;
    if (height != w->serverHeight)
	xwcm |= CWHeight;

    xwc.x = w->attrib.x;
    xwc.y = w->attrib.y;
    xwc.width = width;
    xwc.height = height;

    configureXWindow (w, xwcm, &xwc);
}


static void
winrulesScreenInitOptions (WinrulesScreen *ws)
{
    CompOption *o;

    o = &ws->opt[WINRULES_SCREEN_OPTION_SKIPTASKBAR_MATCH];
    o->name	         = "skiptaskbar_match";
    o->shortDesc         = N_("Skip taskbar");
    o->longDesc	         = N_("Don't show application in taskbar");
    o->type	         = CompOptionTypeMatch;
    matchInit (&o->value.match);
    matchAddFromString (&o->value.match, "");

    o = &ws->opt[WINRULES_SCREEN_OPTION_SKIPPAGER_MATCH];
    o->name	         = "skippager_match";
    o->shortDesc         = N_("Skip pager");
    o->longDesc	         = N_("Don't show application in pager");
    o->type	         = CompOptionTypeMatch;
    matchInit (&o->value.match);
    matchAddFromString (&o->value.match, "");

    o = &ws->opt[WINRULES_SCREEN_OPTION_ABOVE_MATCH];
    o->name	         = "above_match";
    o->shortDesc         = N_("Above");
    o->longDesc	         = N_("Above others windows");
    o->type	         = CompOptionTypeMatch;
    matchInit (&o->value.match);
    matchAddFromString (&o->value.match, "");

    o = &ws->opt[WINRULES_SCREEN_OPTION_BELOW_MATCH];
    o->name	         = "below_match";
    o->shortDesc         = N_("Below");
    o->longDesc	         = N_("Below others windows");
    o->type	         = CompOptionTypeMatch;
    matchInit (&o->value.match);
    matchAddFromString (&o->value.match, "");

    o = &ws->opt[WINRULES_SCREEN_OPTION_STICKY_MATCH];
    o->name	         = "sticky_match";
    o->shortDesc         = N_("Sticky");
    o->longDesc	         = N_("Sticky windows");
    o->type	         = CompOptionTypeMatch;
    matchInit (&o->value.match);
    matchAddFromString (&o->value.match, "");

    o = &ws->opt[WINRULES_SCREEN_OPTION_FULLSCREEN_MATCH];
    o->name	         = "fullscreen_match";
    o->shortDesc         = N_("Fullscreen");
    o->longDesc	         = N_("Fullscreen windows");
    o->type	         = CompOptionTypeMatch;
    matchInit (&o->value.match);
    matchAddFromString (&o->value.match, "");

    o = &ws->opt[WINRULES_SCREEN_OPTION_MOVE_MATCH];
    o->name	         = "no_move_match";
    o->shortDesc         = N_("Non movable windows");
    o->longDesc	         = N_("Set window as non movable");
    o->type	         = CompOptionTypeMatch;
    matchInit (&o->value.match);
    matchAddFromString (&o->value.match, "");

    o = &ws->opt[WINRULES_SCREEN_OPTION_RESIZE_MATCH];
    o->name	         = "no_resize_match";
    o->shortDesc         = N_("Non resizable windows");
    o->longDesc	         = N_("Set window as non resizable");
    o->type	         = CompOptionTypeMatch;
    matchInit (&o->value.match);
    matchAddFromString (&o->value.match, "");

    o = &ws->opt[WINRULES_SCREEN_OPTION_MINIMIZE_MATCH];
    o->name	         = "no_minimize_match";
    o->shortDesc         = N_("Non minimizable windows");
    o->longDesc	         = N_("Set window as non minimizable");
    o->type	         = CompOptionTypeMatch;
    matchInit (&o->value.match);
    matchAddFromString (&o->value.match, "");

    o = &ws->opt[WINRULES_SCREEN_OPTION_MAXIMIZE_MATCH];
    o->name	         = "no_maximize_match";
    o->shortDesc         = N_("Non maximizable windows");
    o->longDesc	         = N_("Set window as non maximizable");
    o->type	         = CompOptionTypeMatch;
    matchInit (&o->value.match);
    matchAddFromString (&o->value.match, "");

    o = &ws->opt[WINRULES_SCREEN_OPTION_CLOSE_MATCH];
    o->name	         = "no_close_match";
    o->shortDesc         = N_("Non closable windows");
    o->longDesc	         = N_("Set window as non closable");
    o->type	         = CompOptionTypeMatch;
    matchInit (&o->value.match);
    matchAddFromString (&o->value.match, "");

    o = &ws->opt[WINRULES_SCREEN_OPTION_NOFOCUS_MATCH];
    o->name	         = "no_focus_match";
    o->shortDesc         = N_("No focus");
    o->longDesc	         = N_("Windows will not have focus");
    o->type	         = CompOptionTypeMatch;
    matchInit (&o->value.match);
    matchAddFromString (&o->value.match, "");
    
    o = &ws->opt[WINRULES_SCREEN_OPTION_WIDGET_MATCH];
    o->name	         = "widget_match";
    o->shortDesc         = N_("Widget");
    o->longDesc	         = N_("Set window as widget");
    o->type	         = CompOptionTypeMatch;
    matchInit (&o->value.match);
    matchAddFromString (&o->value.match, "");
    
    o = &ws->opt[WINRULES_SCREEN_OPTION_SIZE_MATCHES];
    o->name	         = "size_matches";
    o->shortDesc         = N_("sized windows");
    o->longDesc	         = N_("Windows that should be resized by default");
    o->type	         = CompOptionTypeList;
    o->value.list.type   = CompOptionTypeMatch;
    o->value.list.nValue = 0;
    o->value.list.value  = NULL;
    o->rest.s.string     = NULL;
    o->rest.s.nString    = 0;

    o = &ws->opt[WINRULES_SCREEN_OPTION_SIZE_WIDTH_VALUES];
    o->name	         = "size_width_values";
    o->shortDesc         = N_("Width");
    o->longDesc	         = N_("Width values");
    o->type	         = CompOptionTypeList;
    o->value.list.type   = CompOptionTypeInt;
    o->value.list.nValue = 0;
    o->value.list.value  = NULL;
    o->rest.i.min	 = MINSHORT;
    o->rest.i.max	 = MAXSHORT;

    o = &ws->opt[WINRULES_SCREEN_OPTION_SIZE_HEIGHT_VALUES];
    o->name	         = "size_height_values";
    o->shortDesc         = N_("Height");
    o->longDesc	         = N_("Height values");
    o->type	         = CompOptionTypeList;
    o->value.list.type   = CompOptionTypeInt;
    o->value.list.nValue = 0;
    o->value.list.value  = NULL;
    o->rest.i.min	 = MINSHORT;
    o->rest.i.max	 = MAXSHORT;
}

static CompOption *
winrulesGetScreenOptions (CompPlugin *plugin,
			  CompScreen *screen,
                          int        *count)
{
    WINRULES_SCREEN (screen);

    *count = NUM_OPTIONS (ws);
    return ws->opt;
}

static Bool
winrulesSetScreenOption (CompPlugin *plugin,
			 CompScreen      *screen,
                         char            *name, 
                         CompOptionValue *value)
{
    CompOption *o;
    CompWindow *w;
    int index;

    WINRULES_SCREEN (screen);

    o = compFindOption (ws->opt, NUM_OPTIONS (ws), name, &index);
    if (!o)
        return FALSE;

    switch (index)
    {
    case WINRULES_SCREEN_OPTION_SKIPTASKBAR_MATCH:
	if (compSetMatchOption (o, value))
	{
	    for (w = screen->windows; w; w = w->next)
	    {
		if (! w->type & WINRULES_TARGET_WINDOWS)
		    continue;

		WINRULES_WINDOW (w);
		winrulesUpdateState (w,
				     WINRULES_SCREEN_OPTION_SKIPTASKBAR_MATCH,
				     CompWindowStateSkipTaskbarMask,
				     &ww->shouldRemoveSkipTaskbar);
	    }
				
	    return TRUE;
	}
	break;

    case WINRULES_SCREEN_OPTION_SKIPPAGER_MATCH:
	if (compSetMatchOption (o, value))
	{
	    for (w = screen->windows; w; w = w->next)
	    {
		if (! w->type & WINRULES_TARGET_WINDOWS)
		    continue;

		WINRULES_WINDOW (w);
		winrulesUpdateState (w,
				     WINRULES_SCREEN_OPTION_SKIPPAGER_MATCH,
				     CompWindowStateSkipPagerMask,
				     &ww->shouldRemoveSkipPager);
	    }
	    return TRUE;
	}
	break;

    case WINRULES_SCREEN_OPTION_ABOVE_MATCH:
	if (compSetMatchOption (o, value))
	{
	    for (w = screen->windows; w; w = w->next)
	    {
		if (! w->type & WINRULES_TARGET_WINDOWS)
		    continue;

		WINRULES_WINDOW (w);
		winrulesUpdateState (w,
				     WINRULES_SCREEN_OPTION_ABOVE_MATCH,
				     CompWindowStateAboveMask,
				     &ww->shouldRemoveAbove);
		if (ww->shouldRemoveAbove)
		    raiseWindow (w);
		
	    }
	    return TRUE;
	}
	break;
	
    case WINRULES_SCREEN_OPTION_BELOW_MATCH:
	if (compSetMatchOption (o, value))
	{
	    for (w = screen->windows; w; w = w->next)
	    {
		if (! w->type & WINRULES_TARGET_WINDOWS)
		    continue;

		WINRULES_WINDOW (w);
		winrulesUpdateState (w,
				     WINRULES_SCREEN_OPTION_BELOW_MATCH,
				     CompWindowStateBelowMask,
				     &ww->shouldRemoveBelow);
		if (ww->shouldRemoveBelow)
		    lowerWindow (w);
	    }
	    return TRUE;
	}
	break;
	
    case WINRULES_SCREEN_OPTION_STICKY_MATCH:
	if (compSetMatchOption (o, value))
	{
	    for (w = screen->windows; w; w = w->next)
	    {
		if (! w->type & WINRULES_TARGET_WINDOWS)
		    continue;

		WINRULES_WINDOW (w);
		winrulesUpdateState (w,
				     WINRULES_SCREEN_OPTION_STICKY_MATCH,
				     CompWindowStateStickyMask,
				     &ww->shouldRemoveSticky);
	    }
	    return TRUE;
	}
	break;
	
    case WINRULES_SCREEN_OPTION_FULLSCREEN_MATCH:
	if (compSetMatchOption (o, value))
	{
	    for (w = screen->windows; w; w = w->next)
	    {
		if (! w->type & WINRULES_TARGET_WINDOWS)
		    continue;

		WINRULES_WINDOW (w);
		winrulesUpdateState (w,
				     WINRULES_SCREEN_OPTION_FULLSCREEN_MATCH,
				     CompWindowStateFullscreenMask,
				     &ww->shouldRemoveFullscreen);
		if (ww->shouldRemoveFullscreen)
		    maximizeWindow (w, MAXIMIZE_STATE);
	    }
	    return TRUE;
	}
	break;

    case WINRULES_SCREEN_OPTION_MOVE_MATCH:
	if (compSetMatchOption (o, value))
	{
	    for (w = screen->windows; w; w = w->next)
	    {
		if (! w->type & WINRULES_TARGET_WINDOWS)
		    continue;

		winrulesSetAllowedActions (w,
					   WINRULES_SCREEN_OPTION_MOVE_MATCH,
					   CompWindowActionMoveMask);
	    }
	    return TRUE;
	}
	break;

    case WINRULES_SCREEN_OPTION_RESIZE_MATCH:
	if (compSetMatchOption (o, value))
	{
	    for (w = screen->windows; w; w = w->next)
	    {
		if (! w->type & WINRULES_TARGET_WINDOWS)
		    continue;

		winrulesSetAllowedActions (w,
					   WINRULES_SCREEN_OPTION_RESIZE_MATCH,
					   CompWindowActionResizeMask);
	    }
	    return TRUE;
	}
	break;

    case WINRULES_SCREEN_OPTION_MINIMIZE_MATCH:
	if (compSetMatchOption (o, value))
	{
	    for (w = screen->windows; w; w = w->next)
	    {
		if (! w->type & WINRULES_TARGET_WINDOWS)
		    continue;

		winrulesSetAllowedActions (w, 
					   WINRULES_SCREEN_OPTION_MINIMIZE_MATCH,
					   CompWindowActionMinimizeMask);
	    }
	    return TRUE;
	}
	break;

    case WINRULES_SCREEN_OPTION_MAXIMIZE_MATCH:
	if (compSetMatchOption (o, value))
	{
	    for (w = screen->windows; w; w = w->next)
	    {
		if (! w->type & WINRULES_TARGET_WINDOWS)
		    continue;

		winrulesSetAllowedActions (w, 
					   WINRULES_SCREEN_OPTION_MAXIMIZE_MATCH,
					   CompWindowActionMaximizeVertMask|
					   CompWindowActionMaximizeHorzMask);
	    }
	    return TRUE;
	}
	break;

    case WINRULES_SCREEN_OPTION_CLOSE_MATCH:
	if (compSetMatchOption (o, value))
	{
	    for (w = screen->windows; w; w = w->next)
	    {
		if (! w->type & WINRULES_TARGET_WINDOWS)
		    continue;

		winrulesSetAllowedActions (w, 
					   WINRULES_SCREEN_OPTION_CLOSE_MATCH,
					   CompWindowActionCloseMask);
	    }
	    return TRUE;
	}
	break;
    case WINRULES_SCREEN_OPTION_NOFOCUS_MATCH:
	if (compSetMatchOption (o, value))
	{
	    for (w = screen->windows; w; w = w->next)
	    {
		if (! w->type & WINRULES_TARGET_WINDOWS)
		    continue;
		
		WINRULES_WINDOW (w);
		winrulesSetNoFocus (w,
				    WINRULES_SCREEN_OPTION_NOFOCUS_MATCH,
				    &ww->shouldRemoveNofocus);
	    }
	    return TRUE;
	}
	break;
    case WINRULES_SCREEN_OPTION_WIDGET_MATCH:
	if (compSetMatchOption (o, value))
	{
	    for (w = screen->windows; w; w = w->next)
	    {
		if (! w->type & WINRULES_TARGET_WINDOWS)
		    continue;

	    	winrulesUpdateWidget (w);
	    }
	    return TRUE;
	}
	break;
    case WINRULES_SCREEN_OPTION_SIZE_MATCHES:
	if (compSetOptionList (o, value))
	{
	    int i;

	    for (i = 0; i < o->value.list.nValue; i++)
		matchUpdate (screen->display, &o->value.list.value[i].match);

	    return TRUE;
	}
	break;
    default:
	if (compSetOption (o, value))
	    return TRUE;
        break;
    }

    return FALSE;
}

static void
winrulesHandleEvent (CompDisplay *d,
                     XEvent      *event)
{
    CompWindow *w;

    WINRULES_DISPLAY (d);

    if (event->type == MapNotify)
    {
	w = findWindowAtDisplay (d, event->xmap.window);
	if (w && w->type & WINRULES_TARGET_WINDOWS)
	{
	    WINRULES_WINDOW (w);
	    /* Only apply at window creation.
	     * Using CreateNotify not working.
	     */
	    if (ww->firstMap)
	    {
		int width, height;
		
		winrulesUpdateState (w,
				     WINRULES_SCREEN_OPTION_SKIPTASKBAR_MATCH,
				     CompWindowStateSkipTaskbarMask,
				     &ww->shouldRemoveSkipTaskbar);

		winrulesUpdateState (w,
				     WINRULES_SCREEN_OPTION_SKIPPAGER_MATCH,
				     CompWindowStateSkipPagerMask,
				     &ww->shouldRemoveSkipPager);

		winrulesUpdateState (w,
				     WINRULES_SCREEN_OPTION_ABOVE_MATCH,
				     CompWindowStateAboveMask,
				     &ww->shouldRemoveAbove);

		winrulesUpdateState (w,
				     WINRULES_SCREEN_OPTION_BELOW_MATCH,
				     CompWindowStateBelowMask,
				     &ww->shouldRemoveBelow);

		winrulesUpdateState (w,
				     WINRULES_SCREEN_OPTION_STICKY_MATCH,
				     CompWindowStateStickyMask,
				     &ww->shouldRemoveSticky);

		winrulesUpdateState (w,
				     WINRULES_SCREEN_OPTION_FULLSCREEN_MATCH,
				     CompWindowStateFullscreenMask,
				     &ww->shouldRemoveFullscreen);

		winrulesUpdateWidget (w);

		if (ww->shouldRemoveAbove)
		    raiseWindow (w);
		if (ww->shouldRemoveBelow)
		    lowerWindow (w);
		if (ww->shouldRemoveFullscreen)
		    maximizeWindow (w, MAXIMIZE_STATE);


		winrulesSetAllowedActions (w,
					   WINRULES_SCREEN_OPTION_MOVE_MATCH,
					   CompWindowActionMoveMask);


		winrulesSetAllowedActions (w,
					   WINRULES_SCREEN_OPTION_RESIZE_MATCH,
					   CompWindowActionResizeMask);


		winrulesSetAllowedActions (w,
					   WINRULES_SCREEN_OPTION_MINIMIZE_MATCH,
					   CompWindowActionMinimizeMask);


		winrulesSetAllowedActions (w,
					   WINRULES_SCREEN_OPTION_MAXIMIZE_MATCH,
					   CompWindowActionMaximizeVertMask|
					   CompWindowActionMaximizeHorzMask);
	
		winrulesSetAllowedActions (w,
					   WINRULES_SCREEN_OPTION_CLOSE_MATCH,
					   CompWindowActionCloseMask);
		
		if (winrulesMatchSize (w, &width, &height))
		    winrulesUpdateWindowSize (w, width, height);
	    }

	    ww->firstMap = FALSE;
	}
    }
    else if (event->type == MapRequest)
    {
	w = findWindowAtDisplay (d, event->xmap.window);
	if (w && w->type & WINRULES_TARGET_WINDOWS)
	{

	    WINRULES_WINDOW (w);
	    winrulesSetNoFocus (w,
		                WINRULES_SCREEN_OPTION_NOFOCUS_MATCH,
			        &ww->shouldRemoveNofocus);

	}
    }
    UNWRAP (wd, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (wd, d, handleEvent, winrulesHandleEvent);
}

static unsigned int
winrulesGetAllowedActionsForWindow (CompWindow *w)
{
    unsigned int actions;

    WINRULES_SCREEN (w->screen);
    WINRULES_WINDOW (w);

    UNWRAP (ws, w->screen, getAllowedActionsForWindow);
    actions = (*w->screen->getAllowedActionsForWindow) (w);
    WRAP (ws, w->screen, getAllowedActionsForWindow,
          winrulesGetAllowedActionsForWindow);

    return actions & ww->allowedActions;

}

static Bool
winrulesInitDisplay (CompPlugin  *p, 
		     CompDisplay *d)
{
    WinrulesDisplay *wd;

    wd = malloc (sizeof (WinrulesDisplay));
    if (!wd)
        return FALSE;

    wd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (wd->screenPrivateIndex < 0)
    {
        free (wd);
        return FALSE;
    }
    WRAP (wd, d, handleEvent, winrulesHandleEvent);
    d->privates[displayPrivateIndex].ptr = wd;

    return TRUE;
}

static void
winrulesFiniDisplay (CompPlugin  *p, 
		     CompDisplay *d)
{
    WINRULES_DISPLAY (d);

    freeScreenPrivateIndex (d, wd->screenPrivateIndex);
    UNWRAP (wd, d, handleEvent);
    free (wd);
}

static Bool
winrulesInitScreen (CompPlugin *p, 
		    CompScreen *s)
{
    WinrulesScreen *ws;
    int i;

    WINRULES_DISPLAY (s->display);

    ws = malloc (sizeof (WinrulesScreen));
    if (!ws)
        return FALSE;

    ws->windowPrivateIndex = allocateWindowPrivateIndex(s);
    if (ws->windowPrivateIndex < 0)
    {
	free(ws);
	return FALSE;
    }


    winrulesScreenInitOptions (ws);
    for (i=0; i< WINRULES_SIMPLE_MATCH_OPTION_NUM; i++)
    {
	matchUpdate (s->display, &ws->opt[i].value.match);
    }

    WRAP (ws, s, getAllowedActionsForWindow,
	  winrulesGetAllowedActionsForWindow);
    s->privates[wd->screenPrivateIndex].ptr = ws;

    return TRUE;
}

static void
winrulesFiniScreen (CompPlugin *p, 
                    CompScreen *s)
{
    int i;
    WINRULES_SCREEN (s);

    for (i=0; i< WINRULES_SIMPLE_MATCH_OPTION_NUM; i++)
    {
	matchFini (&ws->opt[i].value.match);
    }

    UNWRAP (ws, s, getAllowedActionsForWindow);

    freeWindowPrivateIndex(s, ws->windowPrivateIndex);

    free (ws);
}

static Bool
winrulesInitWindow (CompPlugin *p, 
		    CompWindow *w)
{
    WINRULES_SCREEN (w->screen);

    WinrulesWindow *ww = malloc (sizeof (WinrulesWindow));
    if (!ww)
    {
        return FALSE;
    }

    ww->shouldRemoveSkipTaskbar  = FALSE;
    ww->shouldRemoveSkipPager    = FALSE;
    ww->shouldRemoveAbove        = FALSE;
    ww->shouldRemoveBelow        = FALSE;
    ww->shouldRemoveSticky       = FALSE;
    ww->shouldRemoveFullscreen   = FALSE;
    ww->shouldRemoveNofocus	 = FALSE;
    ww->shouldRemoveWidget       = FALSE;

    ww->allowedActions = ~0;

    ww->firstMap = TRUE;

    w->privates[ws->windowPrivateIndex].ptr = ww;

    return TRUE;
}

static void
winrulesFiniWindow (CompPlugin *p, 
                    CompWindow *w)
{
    WINRULES_WINDOW (w);

    free (ww);
}

static Bool
winrulesInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
        return FALSE;

    return TRUE;
}

static void
winrulesFini (CompPlugin *p)
{
    if (displayPrivateIndex >= 0)
	freeDisplayPrivateIndex (displayPrivateIndex);
}

CompPluginDep winrulesDeps[] = {
    {CompPluginRuleBefore, "widget"}
};

static int
winrulesGetVersion (CompPlugin *plugin,
		int	    version)
{
    return ABIVERSION;
}

static CompPluginVTable winrulesVTable = {
    "winrules",
    "Window Rules",
    "Set windows rules",
    winrulesGetVersion,
    0,
    winrulesInit,
    winrulesFini,
    winrulesInitDisplay,
    winrulesFiniDisplay,
    winrulesInitScreen,
    winrulesFiniScreen,
    winrulesInitWindow,
    winrulesFiniWindow,
    0, /* winrulesGetDisplayOptions, */
    0, /* winrulesSetDisplayOption,  */
    winrulesGetScreenOptions,
    winrulesSetScreenOption,
    winrulesDeps,
    sizeof (winrulesDeps) / sizeof (winrulesDeps[0]),
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &winrulesVTable;
}
