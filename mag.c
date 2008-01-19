/*
 *
 * Compiz magnifier plugin
 *
 * mag.c
 *
 * Copyright : (C) 2008 by Dennis Kasprzyk
 * E-mail    : onestone@opencompositing.org
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

#include <compiz-core.h>
#include <compiz-mousepoll.h>

#include "mag_options.h"

#define GET_MAG_DISPLAY(d)                                  \
    ((MagDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define MAG_DISPLAY(d)                      \
    MagDisplay *md = GET_MAG_DISPLAY (d)

#define GET_MAG_SCREEN(s, md)                                   \
    ((MagScreen *) (s)->base.privates[(md)->screenPrivateIndex].ptr)

#define MAG_SCREEN(s)                                                      \
    MagScreen *ms = GET_MAG_SCREEN (s, GET_MAG_DISPLAY (s->display))

static int displayPrivateIndex = 0;

typedef struct _MagDisplay
{
    int  screenPrivateIndex;

    MousePollFunc *mpFunc;
}
MagDisplay;

typedef struct _MagScreen
{
    int posX;
    int posY;

    Bool adjust;

    GLfloat zVelocity;
    GLfloat zTarget;
    GLfloat zoom;

    PositionPollingHandle pollHandle;
	
    PreparePaintScreenProc preparePaintScreen;
    DonePaintScreenProc    donePaintScreen;
    PaintOutputProc        paintOutput;
}
MagScreen;

static void
damageRegion (CompScreen *s)
{
    REGION r;
    float  w, h, b;

    MAG_SCREEN (s);

    w = magGetBoxWidth (s);
    h = magGetBoxHeight (s);
    b = magGetBorder (s);
    w += 2 * b;
    h += 2 * b;

    r.rects = &r.extents;
    r.numRects = r.size = 1;

    r.extents.x1 = MAX (0, MIN (ms->posX - (w / 2), s->width - w));
    r.extents.x2 = r.extents.x1 + w;
    r.extents.y1 = MAX (0, MIN (ms->posY - (h / 2), s->height - h));
    r.extents.y2 = r.extents.y1 + h;

    damageScreenRegion (s, &r);
}

static void
positionUpdate (CompScreen *s,
		int        x,
		int        y)
{
    MAG_SCREEN (s);

    damageRegion (s);

    ms->posX = x;
    ms->posY = y;

    damageRegion (s);
}

static int
adjustZoom (CompScreen *s, float chunk)
{
    float dx, adjust, amount;
    float change;

    MAG_SCREEN(s);

    dx = ms->zTarget - ms->zoom;

    adjust = dx * 0.15f;
    amount = fabs(dx) * 1.5f;
    if (amount < 0.2f)
	amount = 0.2f;
    else if (amount > 2.0f)
	amount = 2.0f;

    ms->zVelocity = (amount * ms->zVelocity + adjust) / (amount + 1.0f);

    if (fabs (dx) < 0.002f && fabs (ms->zVelocity) < 0.004f)
    {
	ms->zVelocity = 0.0f;
	ms->zoom = ms->zTarget;
	return FALSE;
    }

    change = ms->zVelocity * chunk;
    if (!change)
    {
	if (ms->zVelocity)
	    change = (dx > 0) ? 0.01 : -0.01;
    }

    ms->zoom += change;

    return TRUE;
}

static void
magPreparePaintScreen (CompScreen *s,
		       int        time)
{
    MAG_SCREEN (s);
    MAG_DISPLAY (s->display);

    if (ms->adjust)
    {
	int   steps;
	float amount, chunk;

	amount = time * 0.35f * magGetSpeed (s);
	steps  = amount / (0.5f * magGetTimestep (s));

	if (!steps)
	    steps = 1;

	chunk  = amount / (float) steps;

	while (steps--)
	{
	    ms->adjust = adjustZoom (s, chunk);
	    if (ms->adjust)
		break;
	}
    }

    if (ms->zoom != 1.0)
    {
	if (!ms->pollHandle)
	{
	    (*md->mpFunc->getCurrentPosition) (s, &ms->posX, &ms->posY);
	    ms->pollHandle =
		(*md->mpFunc->addPositionPolling) (s, positionUpdate);
	}
	damageRegion (s);
    }

    UNWRAP (ms, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, time);
    WRAP (ms, s, preparePaintScreen, magPreparePaintScreen);
}

static void
magDonePaintScreen (CompScreen *s)
{
    MAG_SCREEN (s);
    MAG_DISPLAY (s->display);

    if (ms->adjust)
	damageRegion (s);

    if (ms->zoom == 1.0 && !ms->adjust && ms->pollHandle)
    {
	(*md->mpFunc->removePositionPolling) (s, ms->pollHandle);
	ms->pollHandle = 0;
    }

    UNWRAP (ms, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (ms, s, donePaintScreen, magDonePaintScreen);
}

static Bool
magPaintOutput (CompScreen              *s,
	        const ScreenPaintAttrib *sa,
	        const CompTransform     *transform,
	        Region                  region,
	        CompOutput              *output,
	        unsigned int            mask)
{
    Bool           status;
    CompTransform  sTransform = *transform;
    int            w, h, b;
    int            x1, x2, y1, y2;
    Bool           kScreen;
    unsigned short *color;
    float          tmp;
    
    MAG_SCREEN (s);

    UNWRAP (ms, s, paintOutput);
    status = (*s->paintOutput) (s, sa, transform, region, output, mask);
    WRAP (ms, s, paintOutput, magPaintOutput);

    if (ms->zoom == 1.0)
	return status;

    w = magGetBoxWidth (s);
    h = magGetBoxHeight (s);
    b = magGetBorder (s);

    kScreen = magGetKeepScreen (s);
    
    x1 = ms->posX - (w / 2);
    if (kScreen)
	x1 = MAX (0, MIN (x1, s->width - w));
    x2 = x1 + w;
    y1 = ms->posY - (h / 2);
    if (kScreen)
	y1 = MAX (0, MIN (y1, s->height - h));
    y2 = y1 + h;
    
    matrixTranslate (&sTransform,
		     (float)(ms->posX - (output->width / 2) -
		     output->region.extents.x1) / output->width,
		     (float)(ms->posY - (output->height / 2) -
		     output->region.extents.y1) / -output->height,
		     0.0);
    matrixScale (&sTransform, ms->zoom, ms->zoom, 1.0);
    
    matrixTranslate (&sTransform,
		     (float)((output->width / 2) + output->region.extents.x1 -
		     ms->posX) / output->width,
		     (float)((output->height / 2) + output->region.extents.y1 -
		     ms->posY) / -output->height,
		     0.0);

    mask |= PAINT_SCREEN_TRANSFORMED_MASK;

    glScissor (x1, s->height - y2, w, h);

    glEnable (GL_SCISSOR_TEST);

    UNWRAP (ms, s, paintOutput);
    status = (*s->paintOutput) (s, sa, &sTransform, region, output, mask);
    WRAP (ms, s, paintOutput, magPaintOutput);

    glDisable (GL_SCISSOR_TEST);

    glScissor (0, 0, s->width, s->height);

    matrixGetIdentity (&sTransform);

    transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);

    glPushMatrix ();
    glLoadMatrixf (sTransform.m);

    tmp = MIN (0.5, ms->zoom - 1) * 2;
    
    color = magGetBoxColor (s);

    glColor4us (color[0], color[1], color[2], color[3] * tmp);

    if (tmp < 1.0)
	glEnable (GL_BLEND);

    glBegin (GL_QUADS);
    glVertex2f (x1 - b, y1 - b);
    glVertex2f (x1 - b, y1);
    glVertex2f (x2 + b, y1);
    glVertex2f (x2 + b, y1 - b);
    glVertex2f (x1 - b, y2);
    glVertex2f (x1 - b, y2 + b);
    glVertex2f (x2 + b, y2 + b);
    glVertex2f (x2 + b, y2);
    glVertex2f (x1 - b, y1);
    glVertex2f (x1 - b, y2);
    glVertex2f (x1, y2);
    glVertex2f (x1, y1);
    glVertex2f (x2, y1);
    glVertex2f (x2, y2);
    glVertex2f (x2 + b, y2);
    glVertex2f (x2 + b, y1);
    glEnd();

    glDisable (GL_BLEND);

    glPopMatrix();

    glColor4usv (defaultColor);

    return status;
}

static Bool
magTerminate (CompDisplay     *d,
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
	MAG_SCREEN (s);

	ms->zTarget = 1.0;
	ms->adjust  = TRUE;
	damageScreen (s);

	return TRUE;
    }
    return FALSE;
}

static Bool
magInitiate (CompDisplay     *d,
	     CompAction      *action,
	     CompActionState state,
	     CompOption      *option,
	     int             nOption)
{
    CompScreen *s;
    Window     xid;
    float      factor;

    xid    = getIntOptionNamed (option, nOption, "root", 0);
    factor = getFloatOptionNamed (option, nOption, "factor", 0.0);

    s = findScreenAtDisplay (d, xid);
    if (s)
    {
	MAG_SCREEN (s);

	if (factor == 0.0 && ms->zTarget != 1.0)
	    return magTerminate (d, action, state, option, nOption);
	
	if (factor != 1.0)
	    factor = magGetZoomFactor (s);
	
	ms->zTarget = MAX (1.0, MIN (64.0, factor));
	ms->adjust  = TRUE;
	damageScreen (s);

	return TRUE;
    }
    return FALSE;
}

static Bool
magZoomIn (CompDisplay     *d,
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
	MAG_SCREEN (s);

	ms->zTarget = MIN (64.0, ms->zTarget * 1.2);
	ms->adjust  = TRUE;
	damageScreen (s);

	return TRUE;
    }
    return FALSE;
}

static Bool
magZoomOut (CompDisplay     *d,
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
	MAG_SCREEN (s);

	ms->zTarget = MAX (1.0, ms->zTarget / 1.2);
	ms->adjust  = TRUE;
	damageScreen (s);

	return TRUE;
    }
    return FALSE;
}


static Bool
magInitScreen (CompPlugin *p,
	       CompScreen *s)
{
    MAG_DISPLAY (s->display);

    MagScreen *ms = (MagScreen *) calloc (1, sizeof (MagScreen) );

    if (!ms)
	return FALSE;

    s->base.privates[md->screenPrivateIndex].ptr = ms;

    WRAP (ms, s, paintOutput, magPaintOutput);
    WRAP (ms, s, preparePaintScreen, magPreparePaintScreen);
    WRAP (ms, s, donePaintScreen, magDonePaintScreen);

    ms->zoom = 1.0;
    ms->zVelocity = 0.0;
    ms->zTarget = 1.0;

    ms->pollHandle = 0;

    return TRUE;
}


static void
magFiniScreen (CompPlugin *p,
	       CompScreen *s)
{
    MAG_SCREEN (s);
    MAG_DISPLAY (s->display);

    //Restore the original function
    UNWRAP (ms, s, paintOutput);
    UNWRAP (ms, s, preparePaintScreen);
    UNWRAP (ms, s, donePaintScreen);

    if (ms->pollHandle)
	(*md->mpFunc->removePositionPolling) (s, ms->pollHandle);

    if (ms->zoom)
	damageScreen (s);

    //Free the pointer
    free (ms);
}

static Bool
magInitDisplay (CompPlugin  *p,
	        CompDisplay *d)
{
    //Generate a mag display
    MagDisplay *md;
    int        index;

    if (!checkPluginABI ("core", CORE_ABIVERSION) ||
        !checkPluginABI ("mousepoll", MOUSEPOLL_ABIVERSION))
	return FALSE;

    if (!getPluginDisplayIndex (d, "mousepoll", &index))
	return FALSE;

    md = (MagDisplay *) malloc (sizeof (MagDisplay));

    if (!md)
	return FALSE;
 
    //Allocate a private index
    md->screenPrivateIndex = allocateScreenPrivateIndex (d);

    //Check if its valid
    if (md->screenPrivateIndex < 0)
    {
	//Its invalid so free memory and return
	free (md);
	return FALSE;
    }

    md->mpFunc = d->base.privates[index].ptr;

    magSetInitiateInitiate (d, magInitiate);
    magSetInitiateTerminate (d, magTerminate);

    magSetZoomInButtonInitiate (d, magZoomIn);
    magSetZoomOutButtonInitiate (d, magZoomOut);

    //Record the display
    d->base.privates[displayPrivateIndex].ptr = md;
    return TRUE;
}

static void
magFiniDisplay (CompPlugin  *p,
	        CompDisplay *d)
{
    MAG_DISPLAY (d);
    //Free the private index
    freeScreenPrivateIndex (d, md->screenPrivateIndex);
    //Free the pointer
    free (md);
}



static Bool
magInit (CompPlugin * p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex();

    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
magFini (CompPlugin * p)
{
    if (displayPrivateIndex >= 0)
	freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompBool
magInitObject (CompPlugin *p,
	       CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) magInitDisplay,
	(InitPluginObjectProc) magInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
magFiniObject (CompPlugin *p,
	       CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) magFiniDisplay,
	(FiniPluginObjectProc) magFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable magVTable = {
    "mag",
    0,
    magInit,
    magFini,
    magInitObject,
    magFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &magVTable;
}
