/*
 * Copyright © 2006 Novell, Inc.
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
 */

#include <stdarg.h>
#include <stdio.h>
#include <gconf/gconf-client.h>

#include <compiz.h>

#include <gconf-compiz-utils.h>

static FILE *schemaFile;
static int screenDone = 0;
static int displayDone = 0;

static void
gconfPrintf (int level, char *format, ...)
{
    va_list args;

    if (level > 0) {
	int i;

	for (i = 0; i < level * 2; i++)
	    fprintf (schemaFile, "  ");
    }

    va_start (args, format);
    vfprintf (schemaFile, format, args);
    va_end (args);
}

static char *
gconfTypeToString (CompOptionType type)
{
    switch (type) {
    case CompOptionTypeBinding:
	return "string";
    case CompOptionTypeBool:
	return "bool";
    case CompOptionTypeInt:
	return "int";
    case CompOptionTypeFloat:
	return "float";
    case CompOptionTypeString:
	return "string";
    case CompOptionTypeList:
	return "list";
    default:
	break;
    }

    return "unknown";
}

static char *
gconfValueToString (CompDisplay	    *d,
		    CompOptionType  type,
		    CompOptionValue *value)
{
    char *tmp, *escaped;

    switch (type) {
    case CompOptionTypeBinding:
	tmp = gconfBindingToString (d, value);
	escaped = g_markup_escape_text (tmp, -1);
	g_free (tmp);
	return escaped;
    case CompOptionTypeBool:
	return g_strdup (value->b ? "true" : "false");
    case CompOptionTypeInt:
	return g_strdup_printf ("%d", value->i);
    case CompOptionTypeFloat:
	return g_strdup_printf ("%f", value->f);
    case CompOptionTypeString:
	escaped = g_markup_escape_text (value->s, -1);
	return escaped;
    case CompOptionTypeList: {
	char *tmp2, *tmp3;
	int  i;

	tmp = g_strdup_printf ("[");

	for (i = 0; i < value->list.nValue; i++)
	{
	    tmp2 = gconfValueToString (d, value->list.type,
				       &value->list.value[i]);
	    tmp3 = g_strdup_printf ("%s%s%s", tmp, tmp2,
				    ((i + 1) < value->list.nValue) ? "," : "");
	    g_free (tmp);
	    g_free (tmp2);

	    tmp = tmp3;
	}

	tmp2 = g_strdup_printf ("%s]", tmp);
	g_free (tmp);
	escaped = g_markup_escape_text (tmp2, -1);
	g_free (tmp2);
	return escaped;
	}
    default:
	break;
    }

    return g_strdup ("unknown");
}

static void
gconfDumpToSchema (CompDisplay *d,
		   CompOption  *o,
		   char	       *path,
		   char	       *screen)
{
    char *value;

    gconfPrintf (2, "<schema>\n");
    gconfPrintf (3, "<key>/schemas%s/%s/%s/options/%s</key>\n",
		 APP_NAME, path, screen, o->name);
    gconfPrintf (3, "<applyto>%s/%s/%s/options/%s</applyto>\n",
		 APP_NAME, path, screen, o->name);
    gconfPrintf (3, "<owner>compiz</owner>\n");
    gconfPrintf (3, "<type>%s</type>\n", gconfTypeToString (o->type));
    if (o->type == CompOptionTypeList)
	gconfPrintf (3, "<list_type>%s</list_type>\n",
		     gconfTypeToString (o->value.list.type));
    value = gconfValueToString (d, o->type, &o->value);
    gconfPrintf (3, "<default>%s</default>\n", value);
    g_free (value);
    gconfPrintf (3, "<locale name=\"C\">\n");
    gconfPrintf (4, "<short>%s</short>\n", o->shortDesc);
    gconfPrintf (4, "<long>\n");
    gconfPrintf (5, "%s\n", o->longDesc);
    gconfPrintf (4, "</long>\n");
    gconfPrintf (3, "</locale>\n");
    gconfPrintf (2, "</schema>\n\n");
}

static void
gconfTryCloseSchema (void)
{
    if (screenDone && displayDone)
    {
	gconfPrintf (1,
		     "</schemalist>\n");
	gconfPrintf (0,
		     "</gconfschemafile>\n");

	fflush (schemaFile);

	fclose (schemaFile);
    }
}

static Bool
gconfInitDisplay (CompPlugin  *p,
		  CompDisplay *d)
{
    CompOption   *option;
    int	         nOption;

    gconfPrintf (2, "<!-- display options -->\n\n");

    option = compGetDisplayOptions (d, &nOption);
    while (nOption--)
	gconfDumpToSchema (d, option++, "general", "allscreens");

    displayDone = TRUE;

    gconfTryCloseSchema ();

    return TRUE;
}

static Bool
gconfInitScreen (CompPlugin *p,
		 CompScreen *s)
{
    CompOption   *option;
    int	         nOption;
    char         *screenName;

    screenName = g_strdup_printf ("screen%d", s->screenNum);

    gconfPrintf (2, "<!-- %s options -->\n\n", screenName);

    option = compGetScreenOptions (s, &nOption);
    while (nOption--)
	gconfDumpToSchema (s->display, option++, "general", screenName);

    g_free (screenName);

    screenDone = TRUE;

    gconfTryCloseSchema ();

    return TRUE;
}

static Bool
gconfInit (CompPlugin *p)
{
    schemaFile = fopen ("compiz.schemas.dump", "w");

    if (!schemaFile)
	return FALSE;

    gconfPrintf (0,
		 "<!--\n"
		 "      this file is autogenerated "
		 "by gconf-dump compiz plugin\n"
		 "  -->\n\n"
		 "<gconfschemafile>\n");
    gconfPrintf (1,
		 "<schemalist>\n\n");

    fflush (schemaFile);

    return TRUE;
}

static CompPluginDep gconfDeps[] = {
    { CompPluginRuleBefore, "decoration" },
    { CompPluginRuleBefore, "wobbly" },
    { CompPluginRuleBefore, "fade" },
    { CompPluginRuleBefore, "cube" },
    { CompPluginRuleBefore, "expose" }
};

CompPluginVTable gconfVTable = {
    "gconf-dump",
    "GConf dump",
    "GConf dump - dumps gconf schemas",
    gconfInit,
    NULL,
    gconfInitDisplay,
    NULL,
    gconfInitScreen,
    NULL,
    0, /* InitWindow */
    0, /* FiniWindow */
    0, /* GetDisplayOptions */
    0, /* SetDisplayOption */
    0, /* GetScreenOptions */
    0,  /* SetScreenOption */
    gconfDeps,
    sizeof (gconfDeps) / sizeof (gconfDeps[0])
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &gconfVTable;
}
