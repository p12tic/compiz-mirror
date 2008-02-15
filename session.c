/**
 *
 * Compiz session plugin
 *
 * session.c
 *
 * Copyright (c) 2007 Travis Watkins <amaranth@ubuntu.com>
 * Copyright (c) 2006 Patrick Niklaus
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
 * Authors: Travis Watkins <amaranth@ubuntu.com>
 *          Patrick Niklaus
 **/

#define _GNU_SOURCE
#include <X11/Xatom.h>

#include <compiz-core.h>

#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>


static int corePrivateIndex;
static int displayPrivateIndex;
static CompMetadata sessionMetadata;

typedef void (* SessionWindowFunc) (CompWindow *w, char *clientId, char *name,
				    void *user_data);

typedef struct _SessionCore
{
    SessionSaveYourselfProc sessionSaveYourself;
    ObjectAddProc           objectAdd;
} SessionCore;

typedef struct _SessionDisplay
{
    Atom visibleNameAtom;
    Atom clientIdAtom;
    Atom embedInfoAtom;
} SessionDisplay;

#define GET_SESSION_CORE(c)				     \
    ((SessionCore *) (c)->base.privates[corePrivateIndex].ptr)

#define SESSION_CORE(c)		       \
    SessionCore *sc = GET_SESSION_CORE (c)

#define GET_SESSION_DISPLAY(d)                                 \
    ((SessionDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define SESSION_DISPLAY(d)                  \
    SessionDisplay *sd = GET_SESSION_DISPLAY (d)

//list stuff
typedef struct _SessionWindowList
{
    struct _SessionWindowList *next;

    char *clientId;
    char *name;

    XRectangle   geometry;
    Bool         geometryValid;
    unsigned int state;
    Bool         minimized;
    int          workspace;
} SessionWindowList;

static SessionWindowList *windowList = NULL;

static void
freeWindowList (SessionWindowList *item)
{
    if (item->next != NULL)
    {
	freeWindowList (item->next);
    }
    if (item->clientId != NULL)
	free (item->clientId);
    if (item->name != NULL)
	free (item->name);
    free (item);
}



static char*
sessionGetUtf8Property (CompDisplay *d,
			Window      id,
			Atom        atom)
{
    Atom          type;
    int           format;
    unsigned long nitems;
    unsigned long bytesAfter;
    char          *val;
    int           result;
    char          *retval;

    result = XGetWindowProperty (d->display, id, atom, 0L, 65536, False,
                                 d->utf8StringAtom, &type, &format, &nitems,
                                 &bytesAfter, (unsigned char **)&val);

    if (result != Success)
	return NULL;

    if (type != d->utf8StringAtom || format != 8 || nitems == 0)
    {
	if (val)
	    XFree (val);
	return NULL;
    }

    retval = strndup (val, nitems);
    XFree (val);

    return retval;
}

static char*
sessionGetTextProperty (CompDisplay *d,
			Window      id,
			Atom        atom)
{
    XTextProperty text;
    char          *retval = NULL;

    text.nitems = 0;
    if (XGetTextProperty (d->display, id, &text, atom))
    {
	if (text.value) {
	    retval = strndup ((char *)text.value,text.nitems);
	    XFree (text.value);
	}
    }

    return retval;
}

static char*
sessionGetWindowName (CompDisplay *d,
		      Window      id)
{
    char *name;

    SESSION_DISPLAY (d);

    name = sessionGetUtf8Property (d, id, sd->visibleNameAtom);

    if (!name)
	name = sessionGetUtf8Property(d, id, d->wmNameAtom);

    if (!name)
	name = sessionGetTextProperty (d, id, XA_WM_NAME);

    return name;
}

static Bool
sessionGetIsEmbedded (CompDisplay *d, Window id)
{
    SESSION_DISPLAY (d);
    Atom actual_type_return;
    int actual_format_return;
    unsigned long nitems_return;
    unsigned long bytes_after_return;
    unsigned char *prop_return = 0;
    if (XGetWindowProperty(d->display, id, sd->embedInfoAtom, 0, 2,
			   FALSE, XA_CARDINAL, &actual_type_return,
			   &actual_format_return, &nitems_return,
			   &bytes_after_return, &prop_return) == Success)
    {
	if (nitems_return > 1)
	    return TRUE;
    }
    return FALSE;
}

static char*
sessionGetClientId (CompWindow *w)
{
    SESSION_DISPLAY (w->screen->display);
    Window clientLeader;
    char  *clientId;
    XTextProperty text;
    text.nitems = 0;

    clientId = NULL;
    clientLeader = w->clientLeader;

    //window is its own client leader so it's a leader for something else
    if (clientLeader == w->id)
	return NULL;

    //try to find clientLeader on transient parents
    if (clientLeader == None)
    {
	CompWindow *window;
	window = w;
	while (window->transientFor != None)
	{
	    if (window->transientFor == window->id)
		break;

	    window = findWindowAtScreen (w->screen, window->transientFor);
	    if (window->clientLeader != None)
	    {
		clientLeader = window->clientLeader;
		break;
	    }
	}
    }

    if (clientLeader != None)
    {
	if (XGetTextProperty (w->screen->display->display, clientLeader, &text,
			      sd->clientIdAtom))
	{
	    if (text.value) {
		clientId = strndup ((char *)text.value, text.nitems);
		XFree (text.value);
	    }
	}
    }
    else
    {
	//some apps set SM_CLIENT_ID on the app
	if (XGetTextProperty (w->screen->display->display, w->id, &text,
			      sd->clientIdAtom))
	{
	    if (text.value) {
		clientId = strndup ((char *)text.value, text.nitems);
		XFree (text.value);
	    }
	}
    }
    return clientId;
}

static int
sessionGetIntForProp (xmlNodePtr node, char *prop)
{
    xmlChar *temp;
    int      num;

    temp = xmlGetProp (node, BAD_CAST prop);
    if (temp != NULL)
    {
	num = xmlXPathCastStringToNumber (temp);
	xmlFree (temp);
	return num;
    }
    return -1;
}

static void
sessionForeachWindow (CompDisplay *d, SessionWindowFunc func, void *user_data)
{
    CompScreen *s;
    CompWindow *w;
    char       *clientId = NULL;
    char       *name = NULL;

    for (s = d->screens; s; s = s->next)
    {
	for (w = s->windows; w; w = w->next)
	{
	    //filter out embedded windows (notification icons)
	    if (sessionGetIsEmbedded (d, w->id))
		continue;

	    clientId = sessionGetClientId (w);

	    if (clientId == NULL)
		continue;

	    name = sessionGetWindowName (d, w->id);

	    (* func) (w, clientId, name, user_data);

	    if (name)
		free (name);
	    free (clientId);
	}
    }
}

static void
sessionWriteWindow (CompWindow *w, char *clientId, char *name, void *user_data)
{
    FILE *outfile = (FILE*) user_data;
    int x, y, width, height;

    fprintf (outfile, "  <window id=\"%s\" title=\"%s\" class=\"%s\" name=\"%s\">\n",
	     clientId,
	     name ? name : "",
	     w->resClass ? w->resClass : "",
	     w->resName ? w->resName : "");

    //save geometry
    x = w->attrib.x - w->input.left;
    y = w->attrib.y - w->input.top;
    width = w->attrib.width;
    height = w->attrib.height;
    if (w->state & CompWindowStateMaximizedVertMask)
    {
	y = w->saveWc.y;
	height = w->saveWc.height;
    }
    if (w->state & CompWindowStateMaximizedHorzMask)
    {
	x = w->saveWc.x;
	width = w->saveWc.width;
    }

    fprintf (outfile, "    <geometry x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\"/>\n",
	     x, y, width, height);

    //save shaded
    if (w->state & CompWindowStateShadedMask)
	fprintf (outfile, "    <shaded/>\n");

    //save sticky
    if (w->state & CompWindowStateStickyMask ||
	w->type & CompWindowTypeDesktopMask ||
	w->type & CompWindowTypeDockMask)
	    fprintf (outfile, "    <sticky/>\n");

    //save minimized
    if (w->minimized)
	fprintf (outfile, "    <minimized/>\n");

    //save maximized
    if (w->state & MAXIMIZE_STATE)
    {
	fprintf (outfile, "    <maximized ");
	if (w->state & CompWindowStateMaximizedVertMask)
	    fprintf (outfile, "vert=\"yes\" ");
	if (w->state & CompWindowStateMaximizedHorzMask)
	    fprintf (outfile, "horiz=\"yes\"");
	fprintf (outfile, "/>\n");
    }

    //save workspace
    if (!(w->type & CompWindowTypeDesktopMask ||
	  w->type & CompWindowTypeDockMask))
	    fprintf (outfile, "    <workspace index=\"%d\"/>\n",
		     w->desktop);

    fprintf (outfile, "  </window>\n");
}

static void
saveState (const char *clientId,
	   CompDisplay *d)
{
    char           filename[1024];
    FILE          *outfile;
    struct passwd *p = getpwuid(geteuid());

    //setup filename and create directories as needed
    memset (filename, '\0', 1024);
    strncat (filename, p->pw_dir, 1024);
    strncat (filename, "/.compiz", 1024);
    if (mkdir (filename, 0700) == 0 || errno == EEXIST)
    {
	strncat (filename, "/session", 1024);
	if (mkdir (filename, 0700) == 0 || errno == EEXIST)
	{
	    strncat (filename, "/", 1024);
	    strncat (filename, clientId, 1024);
	}
	else
	{
	    return;
	}
    }
    else
    {
	return;
    }

    outfile = fopen (filename, "w");
    if (outfile == NULL)
    {
	return;
    }

    fprintf (outfile, "<compiz_session id=\"%s\">\n", clientId);

    sessionForeachWindow (d, sessionWriteWindow, outfile);

    fprintf (outfile, "</compiz_session>\n");
    fclose (outfile);
}

static void
sessionReadWindow (CompWindow *w, char *clientId, char *name, void *user_data)
{
    XWindowChanges     xwc;
    Bool               foundWindow = FALSE;
    unsigned int       xwcm = 0;
    SessionWindowList *cur;

    memset (&xwc, 0, sizeof (xwc));

    if (clientId == NULL)
	return;

    for (cur = windowList; cur; cur = cur->next)
    {
	if (cur->clientId != NULL && strcmp (clientId, cur->clientId) == 0)
	{
	    foundWindow = TRUE;
	    break;
	}
	if (cur->name != NULL && strcmp (name, cur->name) == 0)
	{
	    foundWindow = TRUE;
	    break;
	}
    }

    if (foundWindow)
    {
	if (cur->geometryValid)
	{
	    xwcm = CWX | CWY | CWWidth | CWHeight;

	    xwc.x = cur->geometry.x;
	    xwc.y = cur->geometry.y;
	    xwc.width = cur->geometry.width;
	    xwc.height = cur->geometry.height;

	    configureXWindow (w, xwcm, &xwc);
	}

	if (cur->minimized)
	    minimizeWindow (w);

	if (cur->workspace != -1)
	    setDesktopForWindow (w, cur->workspace);

	if (cur->state)
	{
	    changeWindowState (w, w->state | cur->state);
	    updateWindowAttributes (w, CompStackingUpdateModeNone);
	}

	//remove item from list
	if (cur == windowList)
	{
	    windowList = cur->next;
	}
	else
	{
	    SessionWindowList *prev;
	    for (prev = windowList; prev; prev = prev->next)
	    {
		if (prev->next == cur)
		{
		    prev->next = cur->next;
		}
	    }
	}
	if (cur->clientId != NULL)
	    free (cur->clientId);
	if (cur->name != NULL)
	    free (cur->name);
	free (cur);
    }
}

static void
readState (xmlNodePtr root)
{
    xmlNodePtr         cur, attrib;
    xmlChar           *newClientId;
    xmlChar           *newName;
    SessionWindowList *current_window;
    int                first = 1;

    for (cur = root->xmlChildrenNode; cur; cur = cur->next)
    {
	SessionWindowList *item = calloc (1, sizeof (SessionWindowList));
	item->geometryValid = FALSE;

	if (xmlStrcmp (cur->name, BAD_CAST "window") == 0)
	{
	    newClientId = xmlGetProp (cur, BAD_CAST "id");
	    if (newClientId != NULL)
	    {
		item->clientId = strdup ((char*) newClientId);
		xmlFree (newClientId);
	    }

	    newName = xmlGetProp (cur, BAD_CAST "title");
	    if (newName != NULL)
	    {
		item->name = strdup ((char*) newName);
		xmlFree (newName);
	    }
	}

	if (item->clientId == NULL && item->name == NULL)
	{
	    free (item);
	    continue;
	}

	for (attrib = cur->xmlChildrenNode; attrib; attrib = attrib->next)
	{
	    if (xmlStrcmp (attrib->name, BAD_CAST "geometry") == 0)
	    {
		item->geometryValid = TRUE;
		item->geometry.x = sessionGetIntForProp (attrib, "x");
		item->geometry.y = sessionGetIntForProp (attrib, "y");
		item->geometry.width = sessionGetIntForProp (attrib, "width");
		item->geometry.height = sessionGetIntForProp (attrib, "height");
	    }

	    if (xmlStrcmp (attrib->name, BAD_CAST "shaded") == 0)
		item->state |= CompWindowStateShadedMask;
	    if (xmlStrcmp (attrib->name, BAD_CAST "sticky") == 0)
		item->state |= CompWindowStateStickyMask;
	    if (xmlStrcmp (attrib->name, BAD_CAST "minimized") == 0)
		item->minimized = TRUE;

	    if (xmlStrcmp (attrib->name, BAD_CAST "maximized") == 0)
	    {
		xmlChar *vert, *horiz;
		vert = xmlGetProp (attrib, BAD_CAST "vert");
		if (vert != NULL)
		{
		    item->state |= CompWindowStateMaximizedVertMask;
		    xmlFree (vert);
		}
		horiz = xmlGetProp (attrib, BAD_CAST "horiz");
		if (horiz != NULL)
		{
		    item->state |= CompWindowStateMaximizedHorzMask;
		    xmlFree (horiz);
		}
	    }

	    if (xmlStrcmp (attrib->name, BAD_CAST "workspace") == 0)
	    {
		int desktop = sessionGetIntForProp (attrib, "index");
		item->workspace = desktop;
	    }
	}

	if (first)
	{
	    windowList = item;
	    current_window = item;
	    first = 0;
	}
	else
	{
	    current_window->next = item;
	    current_window = item;
	}
    }
}

static void
loadState (CompDisplay *d, char *previousId)
{
    xmlDocPtr          doc;
    xmlNodePtr         root;
    char               filename[1024];
    struct passwd *p = getpwuid(geteuid());

    //setup filename and create directories as needed
    memset (filename, '\0', 1024);
    strncat (filename, p->pw_dir, 1024);
    strncat (filename, "/.compiz/", 1024);
    strncat (filename, "session/", 1024);
    strncat (filename, previousId, 1024);

    doc = xmlParseFile (filename);
    if (doc == NULL)
	return;

    root = xmlDocGetRootElement (doc);
    if (root == NULL)
	goto out;

    if (xmlStrcmp (root->name, BAD_CAST "compiz_session") != 0)
	goto out;

    readState (root);

    //load state for windows that appeared before compiz
    sessionForeachWindow (d, sessionReadWindow, NULL);


   out:
    xmlFreeDoc(doc);
    xmlCleanupParser();
}


static void
sessionWindowAdd (CompScreen *s,
		  CompWindow *w)
{
    //just go through every window again instead of duplicating the filter
    sessionForeachWindow (s->display, sessionReadWindow, NULL);
}

static void
sessionSessionSaveYourself (CompCore   *c,
			    const char *clientId,
			    int         saveType,
			    int         interactStyle,
			    Bool        shutdown,
			    Bool        fast)
{
    CompObject *object;

    object = compObjectFind (&c->base, COMP_OBJECT_TYPE_DISPLAY, NULL);
    if (object)
    {
	CompDisplay *d = (CompDisplay *) object;
	saveState (clientId, d);
    }
}

static void
sessionObjectAdd (CompObject *parent,
		  CompObject *object)
{
    static ObjectAddProc dispTab[] = {
	(ObjectAddProc) 0, /* CoreAdd */
	(ObjectAddProc) 0, /* DisplayAdd */
	(ObjectAddProc) 0, /* ScreenAdd */
	(ObjectAddProc) sessionWindowAdd
	};

    SESSION_CORE (&core);

    UNWRAP (sc, &core, objectAdd);
    (*core.objectAdd) (parent, object);
    WRAP (sc, &core, objectAdd, sessionObjectAdd);
	
    DISPATCH (object, dispTab, ARRAY_SIZE (dispTab), (parent, object));
}

static int
sessionInit (CompPlugin *p)
{
    if (!compInitPluginMetadataFromInfo (&sessionMetadata, p->vTable->name,
					 0, 0, 0, 0))
	return FALSE;

    corePrivateIndex = allocateCorePrivateIndex ();
    if (corePrivateIndex < 0)
    {
	compFiniMetadata (&sessionMetadata);
	return FALSE;
    }

    compAddMetadataFromFile (&sessionMetadata, p->vTable->name);

    return TRUE;
}

static void
sessionFini (CompPlugin *p)
{
    freeCorePrivateIndex(corePrivateIndex);
    compFiniMetadata (&sessionMetadata);
}

static Bool
sessionInitCore (CompPlugin *p,
		 CompCore   *c)
{
    SessionCore *sc;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    sc = malloc (sizeof (SessionCore));
    if (!sc)
	return FALSE;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
    {
	free (sc);
	return FALSE;
    }

    WRAP (sc, c, objectAdd, sessionObjectAdd);
    WRAP(sc, c, sessionSaveYourself, sessionSessionSaveYourself);

    c->base.privates[corePrivateIndex].ptr = sc;

    return TRUE;
}

static void
sessionFiniCore (CompPlugin *p,
		 CompCore   *c)
{
    SESSION_CORE (c);

    freeDisplayPrivateIndex (displayPrivateIndex);
    UNWRAP(sc, c, objectAdd);
    UNWRAP(sc, c, sessionSaveYourself);
    free (sc);
}

static int
sessionInitDisplay (CompPlugin *p, CompDisplay *d)
{
    SessionDisplay *sd;
    char *previousId = NULL;
    int i;

    sd = malloc (sizeof (SessionDisplay));
    if (!sd)
	return FALSE;

    d->base.privates[displayPrivateIndex].ptr = sd;

    sd->visibleNameAtom = XInternAtom (d->display,
				       "_NET_WM_VISIBLE_NAME", 0);
    sd->clientIdAtom = XInternAtom (d->display,
				    "SM_CLIENT_ID", 0);
    sd->embedInfoAtom = XInternAtom (d->display,
				     "_XEMBED_INFO", 0);

    for (i = 0; i < programArgc; i++)
    {
	if (strcmp (programArgv[i], "--sm-disable") == 0)
	{
	    if (previousId)
	    {
		free (previousId);
		previousId = NULL;
		break;
	    }
	}
	else if (strcmp (programArgv[i], "--sm-client-id") == 0)
	{
	    previousId = strdup (programArgv[i + 1]);
	}
    }

    if (previousId)
    {
	loadState (d, previousId);
	free (previousId);
    }

    return TRUE;
}

static void
sessionFiniDisplay (CompPlugin *p, CompDisplay *d)
{
    SESSION_DISPLAY (d);

    if (windowList != NULL)
	freeWindowList (windowList);
    free (sd);
}

static CompBool
sessionInitObject (CompPlugin *p,
		   CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
    	(InitPluginObjectProc) sessionInitCore,
	(InitPluginObjectProc) sessionInitDisplay
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
sessionFiniObject (CompPlugin *p,
		   CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) sessionFiniCore,
	(FiniPluginObjectProc) sessionFiniDisplay
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static CompMetadata *
sessionGetMetadata (CompPlugin *plugin)
{
    return &sessionMetadata;
}

static CompPluginVTable sessionVTable =
{
    "session",
    sessionGetMetadata,
    sessionInit,
    sessionFini,
    sessionInitObject,
    sessionFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo20070830 (void)
{
    return &sessionVTable;
}

