#include <libnotify/notify.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <compiz.h>

#define NOTIFY_DISPLAY_OPTION_TIMEOUT 0
#define NOTIFY_DISPLAY_OPTION_NUM     1

#define IMAGE_DIR ".compiz/images"

#define NOTIFY_TIMEOUT_DEFAULT -1
#define NOTIFY_TIMEOUT_NEVER    0

static int displayPrivateIndex;

static CompMetadata notifyMetadata;

typedef struct _NotifyDisplay {
    LogMessageProc logMessage;

    int        timeout;
    CompOption opt[NOTIFY_DISPLAY_OPTION_NUM];
} NotifyDisplay;

#define GET_NOTIFY_DISPLAY(d)				       \
    ((NotifyDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define NOTIFY_DISPLAY(d)		       \
    NotifyDisplay *nd = GET_NOTIFY_DISPLAY (d)

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))


void
notifyLogMessage (CompDisplay *d,
		  char *component,
		  CompLogLevel level,
		  char *message)
{
    NotifyNotification *n;
    char               *logLevel, iconFile[256], *iconUri, *homeDir;

    NOTIFY_DISPLAY (d);

    homeDir = getenv ("HOME");
    if (!homeDir)
	return;

    snprintf (iconFile, 256, "%s/%s/%s", homeDir, IMAGE_DIR, "compiz.png");

    iconUri = malloc (sizeof (char) * strlen (iconFile) + 8);
    if (!iconUri)
	return;

    sprintf (iconUri, "file://%s", iconFile);

    logLevel = logLevelToString (level);

    n = notify_notification_new (logLevel,
                                 message,
                                 iconUri, NULL);

    notify_notification_set_timeout (n, nd->timeout);

    switch (level)
    {
    case CompLogLevelFatal:
    case CompLogLevelError:
	notify_notification_set_urgency (n, NOTIFY_URGENCY_CRITICAL);
	break;
    case CompLogLevelWarn:
	notify_notification_set_urgency (n, NOTIFY_URGENCY_NORMAL);
	break;
    default:
	notify_notification_set_urgency (n, NOTIFY_URGENCY_LOW);
	break;
    }

    if (!notify_notification_show (n, NULL))
	fprintf (stderr, "failed to send notification\n");

    g_object_unref (G_OBJECT (n));
    free (logLevel);
    free (iconUri);

    UNWRAP (nd, d, logMessage);
    (*d->logMessage) (d, component, level, message);
    WRAP (nd, d, logMessage, notifyLogMessage);
}

static const CompMetadataOptionInfo notifyDisplayOptionInfo[] = {
    { "timeout", "int", "<min>-1</min><max>30</max><default>-1</default>", 0, 0 }
};

static Bool
notifyInitDisplay (CompPlugin  *p,
		   CompDisplay *d)
{
    NotifyDisplay *nd;

    nd = malloc (sizeof (NotifyDisplay));
    if (!nd)
	return FALSE;

    if (!compInitDisplayOptionsFromMetadata (d,
					     &notifyMetadata,
					     notifyDisplayOptionInfo,
					     nd->opt,
					     NOTIFY_DISPLAY_OPTION_NUM))
    {
	free (nd);
	return FALSE;
    }

    notify_init ("compiz");
    nd->timeout = 2000;

    d->privates[displayPrivateIndex].ptr = nd;

    WRAP (nd, d, logMessage, notifyLogMessage);

    return TRUE;
}

static void
notifyFiniDisplay (CompPlugin  *p,
		   CompDisplay *d)
{
    NOTIFY_DISPLAY (d);

    UNWRAP (nd, d, logMessage);

    if (notify_is_initted ())
	notify_uninit ();

    free (nd);
}
static Bool
notifyInit (CompPlugin *p)
{
    if (!compInitPluginMetadataFromInfo (&notifyMetadata,
					 p->vTable->name,
					 notifyDisplayOptionInfo,
					 NOTIFY_DISPLAY_OPTION_NUM,
					 0, 0))
	return FALSE;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
    {
	compFiniMetadata (&notifyMetadata);
	return FALSE;
    }

    //compAddMetadataFromFile (&notifyMetadata, p->vTable->name);

    return TRUE;
}

static CompOption *
notifyGetDisplayOptions (CompPlugin   *p,
			 CompDisplay  *display,
			 int	      *count)
{
    NOTIFY_DISPLAY (display);

    *count = NUM_OPTIONS (nd);
    return nd->opt;
}

static Bool
notifySetDisplayOption (CompPlugin      *p,
			CompDisplay     *display,
			char	        *name,
			CompOptionValue *value)
{
    CompOption *o;
    int	       index;

    NOTIFY_DISPLAY (display);

    o = compFindOption (nd->opt, NUM_OPTIONS (nd), name, &index);
    if (!o)
	return FALSE;

    switch (index) {
    case NOTIFY_DISPLAY_OPTION_TIMEOUT:
	if (compSetIntOption (o, value))
	{
	    if (value->i == -1)
		nd->timeout = value->i;
	    else
		nd->timeout = value->i * 1000;
	    return TRUE;
	}
    default:
	break;
    }

    return FALSE;
}

static void
notifyFini (CompPlugin *p)
{
    if (displayPrivateIndex >= 0)
	freeDisplayPrivateIndex (displayPrivateIndex);

    compFiniMetadata (&notifyMetadata);
}
static int
notifyGetVersion (CompPlugin *plugin,
		  int	     version)
{
    return ABIVERSION;
}

static CompMetadata *
notifyGetMetadata (CompPlugin *plugin)
{
    return &notifyMetadata;
}

static CompPluginVTable notifyVTable = {
    "notification",
    notifyGetVersion,
    notifyGetMetadata,
    notifyInit,
    notifyFini,
    notifyInitDisplay,
    notifyFiniDisplay,
    0,
    0,
    0, /* InitWindow */
    0, /* FiniWindow */
    notifyGetDisplayOptions,
    notifySetDisplayOption,
    0, /* GetScreenOptions */
    0, /* SetScreenOption */
    NULL,
    0,
    NULL,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &notifyVTable;
}
