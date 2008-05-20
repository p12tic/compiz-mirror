/*
 *
 * Compiz show repainted regions plugin
 *
 * showrepainted.c
 *
 * Copyright : (C) 2008 by Dennis Kasprzyk
 * E-mail    : onestone@compiz-fusion.org
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
 */

#include <math.h>
#include <string.h>

#include <compiz-core.h>

#include "showrepaint_options.h"

#define SHOWREPAINT_DISPLAY(d) PLUGIN_DISPLAY(d, Showrepaint, s)
#define SHOWREPAINT_SCREEN(s) PLUGIN_SCREEN(s, Showrepaint, s)

static int ShowrepaintDisplayPrivateIndex = 0;

typedef struct _ShowrepaintDisplay
{
    int  screenPrivateIndex;
}
ShowrepaintDisplay;

typedef struct _ShowrepaintScreen
{
    Bool   active;
    Region tmpRegion;

    PaintOutputProc        paintOutput;
}
ShowrepaintScreen;


static Bool
showrepaintPaintOutput (CompScreen              *s,
			const ScreenPaintAttrib *sa,
			const CompTransform     *transform,
			Region                  region,
			CompOutput              *output,
			unsigned int            mask)
{
    Bool           status;
    CompTransform  sTransform;
    unsigned short color[4];
    int            i;
    BoxPtr         box;

    SHOWREPAINT_SCREEN (s);

    UNWRAP (ss, s, paintOutput);
    status = (*s->paintOutput) (s, sa, transform, region, output, mask);
    WRAP (ss, s, paintOutput, showrepaintPaintOutput);

    if (!ss->active)
	return status;

    XIntersectRegion (region, &output->region, ss->tmpRegion);

    if (!REGION_NOT_EMPTY (ss->tmpRegion))
	return status;

    matrixGetIdentity (&sTransform);

    transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);

    color[3] = showrepaintGetIntensity (s->display) * 0xffff / 100;
    color[0] = (rand () & 7) * color[3] / 8;
    color[1] = (rand () & 7) * color[3] / 8;
    color[2] = (rand () & 7) * color[3] / 8;

    glColor4usv (color);
    glPushMatrix ();
    glLoadMatrixf (sTransform.m);
    glEnable (GL_BLEND);

    glBegin (GL_QUADS);
    for (i = 0; i < ss->tmpRegion->numRects; i++)
    {
	box = &ss->tmpRegion->rects[i];
	glVertex2i (box->x1, box->y1);
	glVertex2i (box->x1, box->y2);
	glVertex2i (box->x2, box->y2);
	glVertex2i (box->x2, box->y1);
    }
    glEnd ();

    glDisable (GL_BLEND);
    glPopMatrix();

    glColor4usv (defaultColor);

    return status;
}

static Bool
showrepaintTerminate (CompDisplay     *d,
		      CompAction      *action,
		      CompActionState state,
		      CompOption      *option,
		      int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    s = findScreenAtDisplay (d, xid);
    if (s)
    {
	SHOWREPAINT_SCREEN (s);

	ss->active = FALSE;
	damageScreen (s);

	return TRUE;
    }
    return FALSE;
}

static Bool
showrepaintInitiate (CompDisplay     *d,
		     CompAction      *action,
		     CompActionState state,
		     CompOption      *option,
		     int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid    = getIntOptionNamed (option, nOption, "root", 0);

    s = findScreenAtDisplay (d, xid);
    if (s)
    {
	SHOWREPAINT_SCREEN (s);

	if (ss->active)
	    return showrepaintTerminate (d, action, state, option, nOption);

	ss->active = TRUE;

	return TRUE;
    }
    return FALSE;
}


static Bool
showrepaintInitScreen (CompPlugin *p,
		       CompScreen *s)
{
    SHOWREPAINT_DISPLAY (s->display);

    ShowrepaintScreen *ss;
    ss = (ShowrepaintScreen *) calloc (1, sizeof (ShowrepaintScreen));

    if (!ss)
	return FALSE;

    ss->tmpRegion = XCreateRegion ();
    if (!ss->tmpRegion)
    {
	free (ss);
	return FALSE;
    }

    s->base.privates[sd->screenPrivateIndex].ptr = ss;

    WRAP (ss, s, paintOutput, showrepaintPaintOutput);

    ss->active = FALSE;

    return TRUE;
}


static void
showrepaintFiniScreen (CompPlugin *p,
		       CompScreen *s)
{
    SHOWREPAINT_SCREEN (s);

    //Restore the original function
    UNWRAP (ss, s, paintOutput);

    damageScreen (s);

    XDestroyRegion (ss->tmpRegion);

    //Free the pointer
    free (ss);
}

static Bool
showrepaintInitDisplay (CompPlugin  *p,
			CompDisplay *d)
{
    //Generate a showrepaint display
    ShowrepaintDisplay *sd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    sd = (ShowrepaintDisplay *) malloc (sizeof (ShowrepaintDisplay));

    if (!sd)
	return FALSE;
 
    //Allocate a private index
    sd->screenPrivateIndex = allocateScreenPrivateIndex (d);

    //Check if its valid
    if (sd->screenPrivateIndex < 0)
    {
	//Its invalid so free memory and return
	free (sd);
	return FALSE;
    }

    showrepaintSetInitiateInitiate (d, showrepaintInitiate);
    showrepaintSetInitiateTerminate (d, showrepaintTerminate);

    //Record the display
    d->base.privates[ShowrepaintDisplayPrivateIndex].ptr = sd;
    return TRUE;
}

static void
showrepaintFiniDisplay (CompPlugin  *p,
			CompDisplay *d)
{
    SHOWREPAINT_DISPLAY (d);
    //Free the private index
    freeScreenPrivateIndex (d, sd->screenPrivateIndex);
    //Free the pointer
    free (sd);
}

static Bool
showrepaintInit (CompPlugin * p)
{
    ShowrepaintDisplayPrivateIndex = allocateDisplayPrivateIndex();

    if (ShowrepaintDisplayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
showrepaintFini (CompPlugin * p)
{
    if (ShowrepaintDisplayPrivateIndex >= 0)
	freeDisplayPrivateIndex (ShowrepaintDisplayPrivateIndex);
}

static CompBool
showrepaintInitObject (CompPlugin *p,
		       CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) showrepaintInitDisplay,
	(InitPluginObjectProc) showrepaintInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
showrepaintFiniObject (CompPlugin *p,
		       CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) showrepaintFiniDisplay,
	(FiniPluginObjectProc) showrepaintFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable showrepaintVTable = {
    "showrepaint",
    0,
    showrepaintInit,
    showrepaintFini,
    showrepaintInitObject,
    showrepaintFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &showrepaintVTable;
}
