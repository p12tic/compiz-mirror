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
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <X11/SM/SMlib.h>
#include <X11/ICE/ICElib.h>

static int displayPrivateIndex;

typedef struct _SessionDisplay
{
    Atom visibleNameAtom;
    Atom clientIdAtom;

    SmcConn           smcConnection;
    CompWatchFdHandle iceWatchFdHandle;
    Bool              connected;
    Bool              iceConnected;
    Bool              iceInitialized;
    char             *smClientId;
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

static void
sessionSaveState (CompDisplay *d)
{
    SESSION_DISPLAY (d);
    CompScreen  *s;
    CompWindow  *w;
    char        *name;
    FILE        *outfile;

    outfile = fopen ("/home/travis/.config/compiz/session", "w");
    if (outfile == NULL)
    {
	return;
    }

    fprintf (outfile, "<compiz_session id=\"%s\">\n", sd->smClientId);

    for (s = d->screens; s; s = s->next)
    {
	for (w = s->windows; w; w = w->next)
	{
	    char  *clientId = NULL;
	    Window clientLeader;
	    clientLeader = w->clientLeader;

	    if (w->clientLeader == w->id)
		continue;

	    //try to find clientLeader on transient parents
	    if (clientLeader == None)
	    {
		CompWindow *window;
		window = w;
		while (window->transientFor != None)
		{
		    if (window->transientFor == window->id)
			break;

		    window = findWindowAtScreen (s, window->transientFor);
		    if (window->clientLeader != None)
		    {
			clientLeader = window->clientLeader;
			break;
		    }
		}
	    }

	    if (clientLeader != None)
	    {
		XTextProperty text;
		text.nitems = 0;

		if (XGetTextProperty (d->display, clientLeader, &text, sd->clientIdAtom))
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
		XTextProperty text;
		text.nitems = 0;

		if (XGetTextProperty (d->display, w->id, &text, sd->clientIdAtom))
		{
		    if (text.value) {
			clientId = strndup ((char *)text.value, text.nitems);
			XFree (text.value);
		    }
		}
	    }

	    if (clientId == NULL)
		continue;

	    name = sessionGetWindowName (d, w->id);

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
		fprintf (outfile, "    <maximized/>\n");

	    //save workspace
	    if (!(w->type & CompWindowTypeDesktopMask ||
		  w->type & CompWindowTypeDockMask))
		    fprintf (outfile, "    <workspace index=\"%d\"/>\n", w->desktop);

	    //save geometry
	    fprintf (outfile, "    <geometry x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\"/>\n",
		     w->serverX, w->serverY, w->width, w->height);

	    fprintf (outfile, "  </window>\n");
	}
    }

    fprintf (outfile, "</compiz_session>\n");
    fclose (outfile);
}

static void
sessionSetCloneRestartCommands (SmcConn connection, CompDisplay *d)
{
    SESSION_DISPLAY (d);
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
    restartv[i] = sd->smClientId;
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
sessionSetRestartStyle (SmcConn connection, char hint)
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
sessionCloseSession (CompDisplay *d)
{
    SESSION_DISPLAY (d);

    if (sd->connected)
    {
	sessionSetRestartStyle (sd->smcConnection, SmRestartIfRunning);

	if (SmcCloseConnection (sd->smcConnection, 0, NULL) != SmcConnectionInUse)
	    sd->connected = FALSE;
	if (sd->smClientId) {
	    free (sd->smClientId);
	    sd->smClientId = NULL;
	}
    }
}

static void
sessionSaveYourselfCallback (SmcConn	connection,
			     SmPointer  client_data,
			     int	saveType,
			     Bool	shutdown,
			     int	interact_Style,
			     Bool	fast)
{
    sessionSaveState ((CompDisplay*) client_data);

    sessionSetRestartStyle (connection, SmRestartIfRunning);
    sessionSetCloneRestartCommands (connection, (CompDisplay*) client_data);
    SmcSaveYourselfDone (connection, 1);
}

static void
sessionDieCallback (SmcConn   connection,
		    SmPointer clientData)
{
    sessionCloseSession ((CompDisplay*) clientData);
    exit (0);
}

/* ice connection handling taken and updated from gnome-ice.c
 * original gnome-ice.c code written by Tom Tromey <tromey@cygnus.com>
 */

/* This is called when data is available on an ICE connection. */
static Bool
sessionIceProcessMessages (void *data)
{
    IceConn		     connection = (IceConn) data;
    IceProcessMessagesStatus status;

    status = IceProcessMessages (connection, NULL, NULL);

    if (status == IceProcessMessagesIOError)
    {
	IceSetShutdownNegotiation (connection, False);
	IceCloseConnection (connection);
    }

    return 1;
}

/* This is called when a new ICE connection is made.  It arranges for
   the ICE connection to be handled via the event loop.  */
static void
sessionIceNewConnection (IceConn    connection,
			 IcePointer clientData,
			 Bool	     opening,
			 IcePointer *watchData)
{
    SESSION_DISPLAY ((CompDisplay*) clientData);

    if (opening)
    {
	/* Make sure we don't pass on these file descriptors to any
	   exec'ed children */
	fcntl (IceConnectionNumber (connection), F_SETFD,
	       fcntl (IceConnectionNumber (connection),
		      F_GETFD,0) | FD_CLOEXEC);

	sd->iceWatchFdHandle = compAddWatchFd (IceConnectionNumber (connection),
					       POLLIN | POLLPRI | POLLHUP | POLLERR,
					       sessionIceProcessMessages, connection);

	sd->iceConnected = 1;
    }
    else
    {
	if (sd->iceConnected)
	{
	    compRemoveWatchFd (sd->iceWatchFdHandle);

	    sd->iceWatchFdHandle = 0;
	    sd->iceConnected = 0;
	}
    }
}

static IceIOErrorHandler oldIceHandler;

static void
sessionIceErrorHandler (IceConn connection)
{
    if (oldIceHandler)
	(*oldIceHandler) (connection);
}

/* We call any handler installed before (or after) iceInit but
   avoid calling the default libICE handler which does an exit() */
static void
sessionIceInit (CompDisplay *d)
{
    SESSION_DISPLAY (d);

    if (!sd->iceInitialized)
    {
	IceIOErrorHandler defaultIceHandler;

	oldIceHandler	  = IceSetIOErrorHandler (NULL);
	defaultIceHandler = IceSetIOErrorHandler (sessionIceErrorHandler);

	if (oldIceHandler == defaultIceHandler)
	    oldIceHandler = NULL;

	IceAddConnectionWatch (sessionIceNewConnection, d);

	sd->iceInitialized = 1;
    }
}

static int
sessionGetVersion(CompPlugin * p, int version)
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
    static SmcCallbacks callbacks;
    SessionDisplay     *sd;
    char               *previousId = NULL;;
    int                 i;

    sd = malloc (sizeof (SessionDisplay));
    if (!sd)
	return FALSE;

    sd->visibleNameAtom = XInternAtom (d->display,
				       "_NET_WM_VISIBLE_NAME", 0);
    sd->clientIdAtom = XInternAtom (d->display,
				    "SM_CLIENT_ID", 0);

    for (i = 0; i < programArgc; i++)
    {
	if (strcmp(programArgv[i], "--sm-client-id") == 0)
	{
	    previousId = programArgv[++i];
	    break;
	}
    }

    if (getenv ("SESSION_MANAGER"))
    {
	char errorBuffer[1024];

	sessionIceInit (d);

	callbacks.save_yourself.callback    = sessionSaveYourselfCallback;
	callbacks.save_yourself.client_data = d;

	callbacks.die.callback	  = sessionDieCallback;
	callbacks.die.client_data = NULL;

	sd->smcConnection = SmcOpenConnection (NULL,
					       NULL,
					       SmProtoMajor,
					       SmProtoMinor,
					       SmcSaveYourselfProcMask |
					       SmcDieProcMask,
					       &callbacks,
					       previousId,
					       &sd->smClientId,
					       sizeof (errorBuffer),
					       errorBuffer);
	if (!sd->smcConnection)
	{
	    compLogMessage (NULL, "session", CompLogLevelWarn,
			    "SmcOpenConnection failed: %s",
			    errorBuffer);
	    return FALSE;
	}
	else
	    sd->connected = FALSE;
    }

    sd->iceConnected = FALSE;
    sd->iceInitialized = FALSE;

    d->privates[displayPrivateIndex].ptr = sd;

    return TRUE;
}

static void
sessionFiniDisplay (CompPlugin *p, CompDisplay *d)
{
    SESSION_DISPLAY (d);
    sessionCloseSession (d);
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

