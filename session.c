/**
 *
 * Compiz session plugin
 *
 * session.c
 *
 * Copyright (c) 2007 Travis Watkins <amaranth@ubuntu.com>
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

#define _GNU_SOURCE
#include <X11/Xatom.h>

#include <compiz.h>

#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pwd.h>
#include <X11/SM/SMlib.h>
#include <X11/ICE/ICElib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

#define SM_DEBUG(x)

static SmcConn		 smcConnection;
static CompWatchFdHandle iceWatchFdHandle;
static Bool		 connected = 0;
static Bool		 iceConnected = 0;
static char		 *smClientId;

static void iceInit (void);

static int displayPrivateIndex;

typedef void (* SessionWindowFunc) (CompWindow *w, char *clientId, char *name,
				    void *user_data);

typedef struct _SessionDisplay
{
    Atom visibleNameAtom;
    Atom clientIdAtom;
    Atom embedInfoAtom;
} SessionDisplay;

#define GET_SESSION_DISPLAY(d)                                 \
    ((SessionDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define SESSION_DISPLAY(d)                  \
    SessionDisplay *sd = GET_SESSION_DISPLAY (d)

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
    return 0;
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
	}
    }
}

static void
sessionWriteWindow (CompWindow *w, char *clientId, char *name, void *user_data)
{
    FILE *outfile = (FILE*) user_data;

    fprintf (outfile, "  <window id=\"%s\" title=\"%s\" class=\"%s\" name=\"%s\">\n",
	     clientId,
	     name ? name : "",
	     w->resClass ? w->resClass : "",
	     w->resName ? w->resName : "");

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

    //save geometry
    if (!(w->state & MAXIMIZE_STATE))
	fprintf (outfile, "    <geometry x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\"/>\n",
		 w->attrib.x - w->input.left, w->attrib.y - w->input.top,
		 w->width + 2 * w->attrib.border_width + w->input.left + w->input.right,
		 w->height + 2 * w->attrib.border_width + w->input.top + w->input.bottom);

    fprintf (outfile, "  </window>\n");
}

static void
saveState (CompDisplay *d)
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
	    strncat (filename, smClientId, 1024);
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

    fprintf (outfile, "<compiz_session id=\"%s\">\n", smClientId);

    sessionForeachWindow (d, sessionWriteWindow, outfile);

    fprintf (outfile, "</compiz_session>\n");
    fclose (outfile);
}

static void
sessionReadWindow (CompWindow *w, char *clientId, char *name, void *user_data)
{
    xmlNodePtr     cur;
    xmlChar       *newName;
    xmlChar       *newClientId;
    XWindowChanges xwc;
    Bool           foundWindow = FALSE;
    unsigned int   xwcm = 0;
    xmlNodePtr     root = (xmlNodePtr) user_data;

    memset (&xwc, 0, sizeof (xwc));

    if (clientId == NULL)
	return;

    for (cur = root->xmlChildrenNode; cur; cur = cur->next)
    {
	if (xmlStrcmp (cur->name, BAD_CAST "window") == 0)
	{
	    newClientId = xmlGetProp (cur, BAD_CAST "id");
	    if (newClientId != NULL)
	    {
		if (strcmp (clientId, (char*) newClientId) == 0)
		{
		    foundWindow = TRUE;
		    break;
		}
		xmlFree (newClientId);
	    }

	    newName = xmlGetProp (cur, BAD_CAST "title");
	    if (newName != NULL)
	    {
		if (strcmp (name, (char*) newName) == 0)
		{
		    foundWindow = TRUE;
		    break;
		}
		xmlFree (newName);
	    }
	}
    }

    if (foundWindow)
    {
	printf ("Found window %s\n", name);
	for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	{
	    if (xmlStrcmp (cur->name, BAD_CAST "geometry") == 0)
	    {
		xwcm = CWX | CWY | CWWidth | CWHeight;

		xwc.x = sessionGetIntForProp (cur, "x");
		xwc.y = sessionGetIntForProp (cur, "y");
		xwc.width = sessionGetIntForProp (cur, "width");
		xwc.height = sessionGetIntForProp (cur, "height");

		configureXWindow (w, xwcm, &xwc);
	    }
	    if (xmlStrcmp (cur->name, BAD_CAST "sticky") == 0)
	    {
		changeWindowState (w, w->state | CompWindowStateStickyMask);
	    }
	    if (xmlStrcmp (cur->name, BAD_CAST "minimized") == 0)
	    {
		minimizeWindow (w);
	    }
	    if (xmlStrcmp (cur->name, BAD_CAST "maximized") == 0)
	    {
		int state = 0;
		xmlChar *vert, *horiz;
		vert = xmlGetProp (cur, BAD_CAST "vert");
		if (vert != NULL)
		{
		    state |= CompWindowStateMaximizedVertMask;
		    xmlFree (vert);
		}
		horiz = xmlGetProp (cur, BAD_CAST "horiz");
		if (horiz != NULL)
		{
		    state |= CompWindowStateMaximizedHorzMask;
		    xmlFree (horiz);
		}
		maximizeWindow (w, state);
	    }
	}
    }
}

static void
loadState (CompDisplay *d, char *previousId)
{
    xmlDocPtr      doc;
    xmlNodePtr     root;
    char           filename[1024];
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

    sessionForeachWindow (d, sessionReadWindow, root);

   out:
    xmlFreeDoc(doc);
    xmlCleanupParser();
}




static void
setCloneRestartCommands (SmcConn connection)
{
    char *restartv[10];
    char *clonev[10];
    SmProp prop1, prop2, *props[2];
    int    i;

    prop1.name = SmRestartCommand;
    prop1.type = SmLISTofARRAY8;

    i = 0;
    restartv[i] = "compiz";
    ++i;
    restartv[i] = "--sm-client-id";
    ++i;
    restartv[i] = smClientId;
    ++i;
    restartv[i] = NULL;

    prop1.vals = malloc (i * sizeof (SmPropValue));
    if (!prop1.vals)
	return;

    i = 0;
    while (restartv[i])
    {
        prop1.vals[i].value = restartv[i];
	prop1.vals[i].length = strlen (restartv[i]);
	++i;
    }
    prop1.num_vals = i;


    prop2.name = SmCloneCommand;
    prop2.type = SmLISTofARRAY8;

    i = 0;
    clonev[i] = "compiz";
    ++i;
    clonev[i] = NULL;

    prop2.vals = malloc (i * sizeof (SmPropValue));
    if (!prop2.vals)
	return;

    i = 0;
    while (clonev[i])
    {
        prop2.vals[i].value = clonev[i];
	prop2.vals[i].length = strlen (clonev[i]);
	++i;
    }
    prop2.num_vals = i;


    props[0] = &prop1;
    props[1] = &prop2;
    
    SmcSetProperties (connection, 2, props);
}

static void
setRestartStyle (SmcConn connection, char hint)
{
    SmProp	prop, *pProp;
    SmPropValue propVal;

    prop.name = SmRestartStyleHint;
    prop.type = SmCARD8;
    prop.num_vals = 1;
    prop.vals = &propVal;
    propVal.value = &hint;
    propVal.length = 1;

    pProp = &prop;

    SmcSetProperties (connection, 1, &pProp);
}

static void
saveYourselfGotProps (SmcConn   connection,
		      SmPointer client_data,
		      int       num_props,
		      SmProp    **props)
{
    setRestartStyle (connection, SmRestartIfRunning);
    setCloneRestartCommands (connection);
    SmcSaveYourselfDone (connection, 1);
}

static void
saveYourselfCallback (SmcConn	connection,
		      SmPointer client_data,
		      int	saveType,
		      Bool	shutdown,
		      int	interact_Style,
		      Bool	fast)
{
    saveState ((CompDisplay*) client_data);

    if (!SmcGetProperties (connection, saveYourselfGotProps, NULL))
	SmcSaveYourselfDone (connection, 1);
}

static void
dieCallback (SmcConn   connection,
	     SmPointer clientData)
{
    closeSession ();
    exit (0);
}

static void
saveCompleteCallback (SmcConn	connection,
		      SmPointer clientData)
{
}

static void
shutdownCancelledCallback (SmcConn   connection,
			   SmPointer clientData)
{
}

static void
initSession2 (CompDisplay *d, char *smPrevClientId)
{
    static SmcCallbacks callbacks;

    if (getenv ("SESSION_MANAGER"))
    {
	char errorBuffer[1024];

	iceInit ();

	callbacks.save_yourself.callback    = saveYourselfCallback;
	callbacks.save_yourself.client_data = d;

	callbacks.die.callback	  = dieCallback;
	callbacks.die.client_data = NULL;

	callbacks.save_complete.callback    = saveCompleteCallback;
	callbacks.save_complete.client_data = NULL;

	callbacks.shutdown_cancelled.callback	 = shutdownCancelledCallback;
	callbacks.shutdown_cancelled.client_data = NULL;

	smcConnection = SmcOpenConnection (NULL,
					   NULL,
					   SmProtoMajor,
					   SmProtoMinor,
					   SmcSaveYourselfProcMask |
					   SmcDieProcMask	   |
					   SmcSaveCompleteProcMask |
					   SmcShutdownCancelledProcMask,
					   &callbacks,
					   smPrevClientId,
					   &smClientId,
					   sizeof (errorBuffer),
					   errorBuffer);
	if (!smcConnection)
	    compLogMessage (NULL, "session", CompLogLevelWarn,
			    "SmcOpenConnection failed: %s",
			    errorBuffer);
	else
	    connected = TRUE;
    }
}

void
closeSession (void)
{
    if (connected)
    {
	setRestartStyle (smcConnection, SmRestartIfRunning);

	if (SmcCloseConnection (smcConnection, 0, NULL) != SmcConnectionInUse)
	    connected = FALSE;
	if (smClientId) {
	    free (smClientId);
	    smClientId = NULL;
	}
    }
}

/* ice connection handling taken and updated from gnome-ice.c
 * original gnome-ice.c code written by Tom Tromey <tromey@cygnus.com>
 */

/* This is called when data is available on an ICE connection. */
static Bool
iceProcessMessages (void *data)
{
    IceConn		     connection = (IceConn) data;
    IceProcessMessagesStatus status;

    SM_DEBUG (printf ("ICE connection process messages\n"));

    status = IceProcessMessages (connection, NULL, NULL);

    if (status == IceProcessMessagesIOError)
    {
	SM_DEBUG (printf ("ICE connection process messages"
			  " - error => shutting down the connection\n"));

	IceSetShutdownNegotiation (connection, False);
	IceCloseConnection (connection);
    }

    return 1;
}

/* This is called when a new ICE connection is made.  It arranges for
   the ICE connection to be handled via the event loop.  */
static void
iceNewConnection (IceConn    connection,
		  IcePointer clientData,
		  Bool	     opening,
		  IcePointer *watchData)
{
    if (opening)
    {
	SM_DEBUG (printf ("ICE connection opening\n"));

	/* Make sure we don't pass on these file descriptors to any
	   exec'ed children */
	fcntl (IceConnectionNumber (connection), F_SETFD,
	       fcntl (IceConnectionNumber (connection),
		      F_GETFD,0) | FD_CLOEXEC);

	iceWatchFdHandle = compAddWatchFd (IceConnectionNumber (connection),
					   POLLIN | POLLPRI | POLLHUP | POLLERR,
					   iceProcessMessages, connection);

	iceConnected = 1;
    }
    else
    {
	SM_DEBUG (printf ("ICE connection closing\n"));

	if (iceConnected)
	{
	    compRemoveWatchFd (iceWatchFdHandle);

	    iceWatchFdHandle = 0;
	    iceConnected = 0;
	}
    }
}

static IceIOErrorHandler oldIceHandler;

static void
iceErrorHandler (IceConn connection)
{
    if (oldIceHandler)
	(*oldIceHandler) (connection);
}

/* We call any handler installed before (or after) iceInit but
   avoid calling the default libICE handler which does an exit() */
static void
iceInit (void)
{
    static Bool iceInitialized = 0;

    if (!iceInitialized)
    {
	IceIOErrorHandler defaultIceHandler;

	oldIceHandler	  = IceSetIOErrorHandler (NULL);
	defaultIceHandler = IceSetIOErrorHandler (iceErrorHandler);

	if (oldIceHandler == defaultIceHandler)
	    oldIceHandler = NULL;

	IceAddConnectionWatch (iceNewConnection, NULL);

	iceInitialized = 1;
    }
}




static int
sessionGetVersion(CompPlugin * p,
		    int version)
{
    return ABIVERSION;
}

static int
sessionInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
sessionFini (CompPlugin *p)
{
    freeDisplayPrivateIndex(displayPrivateIndex);
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

    d->privates[displayPrivateIndex].ptr = sd;

    sd->visibleNameAtom = XInternAtom (d->display,
				       "_NET_WM_VISIBLE_NAME", 0);
    sd->clientIdAtom = XInternAtom (d->display,
				    "SM_CLIENT_ID", 0);
    sd->embedInfoAtom = XInternAtom (d->display,
				     "_XEMBED_INFO", 0);

    for (i = 0; i < programArgc; i++)
    {
	if (strcmp(programArgv[i], "--sm-client-id") == 0)
	{
	    previousId = malloc (strlen (programArgv[++i]) + 1);
	    previousId = strdup (programArgv[i]);
	    break;
	}
    }

    initSession2 (d, previousId);

    if (previousId != NULL)
	loadState (d, previousId);

    free (previousId);

    return TRUE;
}

static void
sessionFiniDisplay (CompPlugin *p, CompDisplay *d)
{
    SESSION_DISPLAY (d);
    closeSession ();
    free (sd);
}

static CompPluginVTable sessionVTable =
{
    "session",
    sessionGetVersion,
    0,
    sessionInit,
    sessionFini,
    sessionInitDisplay,
    sessionFiniDisplay,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};

CompPluginVTable * getCompPluginInfo(void)
{
    return &sessionVTable;
}

