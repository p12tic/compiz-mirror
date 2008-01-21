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

    GLuint texture;
    GLenum target;

    int width;
    int height;

    PositionPollingHandle pollHandle;
	
    PreparePaintScreenProc preparePaintScreen;
    DonePaintScreenProc    donePaintScreen;
    PaintScreenProc        paintScreen;
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

    if (!ms->adjust && ms->zoom == 1.0 && (ms->width || ms->height))
    {
	glEnable (ms->target);

	glBindTexture (ms->target, ms->texture);

	glTexImage2D (ms->target, 0, GL_RGB, 0, 0, 0,
		      GL_RGB, GL_UNSIGNED_BYTE, NULL);

	ms->width = 0;
	ms->height = 0;
	
	glBindTexture (ms->target, 0);

	glDisable (ms->target);
    }

    if (ms->zoom == 1.0 && !ms->adjust && ms->pollHandle)
    {
	(*md->mpFunc->removePositionPolling) (s, ms->pollHandle);
	ms->pollHandle = 0;
    }

    UNWRAP (ms, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (ms, s, donePaintScreen, magDonePaintScreen);
}

static void
magPaintScreen (CompScreen   *s,
		CompOutput   *outputs,
		int          numOutput,
		unsigned int mask)
{
    XRectangle     r;
    float          pw, ph, bw, bh;
    int            x1, x2, y1, y2;
    float          vc[4];
    float          tc[4];
    int            w, h, cw, ch, cx, cy;
    Bool           kScreen;
    unsigned short *color;
    float          tmp;

    MAG_SCREEN (s);

    UNWRAP (ms, s, paintScreen);
    (*s->paintScreen) (s, outputs, numOutput, mask);
    WRAP (ms, s, paintScreen, magPaintScreen);

    if (ms->zoom == 1.0)
	return; 

    r.x      = 0;
    r.y      = 0;
    r.width  = s->width;
    r.height = s->height;

    if (s->lastViewport.x      != r.x     ||
	s->lastViewport.y      != r.y     ||
	s->lastViewport.width  != r.width ||
	s->lastViewport.height != r.height)
    {
	glViewport (r.x, r.y, r.width, r.height);
	s->lastViewport = r;
    }

    w = magGetBoxWidth (s);
    h = magGetBoxHeight (s);

    kScreen = magGetKeepScreen (s);

    x1 = ms->posX - (w / 2);
    if (kScreen)
	x1 = MAX (0, MIN (x1, s->width - w));
    x2 = x1 + w;
    y1 = ms->posY - (h / 2);
    if (kScreen)
	y1 = MAX (0, MIN (y1, s->height - h));
    y2 = y1 + h;
 
    glEnable (ms->target);

    glBindTexture (ms->target, ms->texture);

    cw = ceil ((float)w / (ms->zoom * 2.0)) * 2.0;
    ch = ceil ((float)h / (ms->zoom * 2.0)) * 2.0;
    cw = MIN (w, cw + 2);
    ch = MIN (h, ch + 2);
    cx = (w - cw) / 2;
    cy = (h - ch) / 2;

    cx -= (x1 - (ms->posX - (w / 2))) / ms->zoom;
    cy += (y1 - (ms->posY - (h / 2))) / ms->zoom;

    cx = MAX (0, MIN (ms->width - cw, cx));
    cy = MAX (0, MIN (ms->height - ch, cy));

    if (ms->width != w || ms->height != h)
    {
	glCopyTexImage2D(ms->target, 0, GL_RGB, x1, s->height - y2,
			 w, h, 0);
	ms->width = w;
	ms->height = h;
    }
    else
	glCopyTexSubImage2D (ms->target, 0, cx, cy,
			     x1 + cx, s->height - y2 + cy, cw, ch);

    if (ms->target == GL_TEXTURE_2D)
    {
	pw = 1.0 / ms->width;
	ph = 1.0 / ms->height;
    }
    else
    {
	pw = 1.0;
	ph = 1.0;
    }

    glMatrixMode (GL_PROJECTION);
    glPushMatrix ();
    glLoadIdentity ();
    glMatrixMode (GL_MODELVIEW);
    glPushMatrix ();
    glLoadIdentity ();

    vc[0] = ((x1 * 2.0) / s->width) - 1.0;
    vc[1] = ((x2 * 2.0) / s->width) - 1.0;
    vc[2] = ((y1 * -2.0) / s->height) + 1.0;
    vc[3] = ((y2 * -2.0) / s->height) + 1.0;

    tc[0] = 0.0;
    tc[1] = w * pw;
    tc[2] = h * ph;
    tc[3] = 0.0;

    glColor4usv (defaultColor);

    glPushMatrix ();

    glTranslatef ((float)(ms->posX - (s->width / 2)) * 2 / s->width,
		  (float)(ms->posY - (s->height / 2)) * 2 / -s->height, 0.0);

    glScalef (ms->zoom, ms->zoom, 1.0);

    glTranslatef ((float)((s->width / 2) - ms->posX) * 2 / s->width,
		  (float)((s->height / 2) - ms->posY) * 2 / -s->height, 0.0);

    glScissor (x1, s->height - y2, w, h);

    glEnable (GL_SCISSOR_TEST);

    glBegin (GL_QUADS);
	glTexCoord2f (tc[0], tc[2]);
	glVertex2f (vc[0], vc[2]);
	glTexCoord2f (tc[0], tc[3]);
	glVertex2f (vc[0], vc[3]);
	glTexCoord2f (tc[1], tc[3]);
	glVertex2f (vc[1], vc[3]);
	glTexCoord2f (tc[1], tc[2]);
	glVertex2f (vc[1], vc[2]);
    glEnd ();

    glDisable (GL_SCISSOR_TEST);

    glPopMatrix ();

    glBindTexture (ms->target, 0);

    glDisable (ms->target);

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    tmp = MIN (1.0, (ms->zoom - 1) * 3.0);

    bw = bh = magGetBorder (s);

    bw = bw * 2.0 / s->width;
    bh = bh * 2.0 / s->height;

    bw = bh = magGetBorder (s);

    bw *= 2.0 / (float)s->width;
    bh *= 2.0 / (float)s->height;

    color = magGetBoxColor (s);

    glColor4us (color[0], color[1], color[2], color[3] * tmp);

    glBegin (GL_QUADS);
	glVertex2f (vc[0] - bw, vc[2] + bh);
	glVertex2f (vc[0] - bw, vc[2]);
	glVertex2f (vc[1] + bw, vc[2]);
	glVertex2f (vc[1] + bw, vc[2] + bh);
	glVertex2f (vc[0] - bw, vc[3]);
	glVertex2f (vc[0] - bw, vc[3] - bh);
	glVertex2f (vc[1] + bw, vc[3] - bh);
	glVertex2f (vc[1] + bw, vc[3]);
	glVertex2f (vc[0] - bw, vc[2]);
	glVertex2f (vc[0] - bw, vc[3]);
	glVertex2f (vc[0], vc[3]);
	glVertex2f (vc[0], vc[2]);
	glVertex2f (vc[1], vc[2]);
	glVertex2f (vc[1], vc[3]);
	glVertex2f (vc[1] + bw, vc[3]);
	glVertex2f (vc[1] + bw, vc[2]);
    glEnd();

    glColor4usv (defaultColor);
    glDisable (GL_BLEND);
    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glPopMatrix();
    glMatrixMode (GL_PROJECTION);
    glPopMatrix ();
    glMatrixMode (GL_MODELVIEW);



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

    WRAP (ms, s, paintScreen, magPaintScreen);
    WRAP (ms, s, preparePaintScreen, magPreparePaintScreen);
    WRAP (ms, s, donePaintScreen, magDonePaintScreen);

    ms->zoom = 1.0;
    ms->zVelocity = 0.0;
    ms->zTarget = 1.0;

    ms->pollHandle = 0;

    glGenTextures (1, &ms->texture);

    if (s->textureNonPowerOfTwo)
	ms->target = GL_TEXTURE_2D;
    else
	ms->target = GL_TEXTURE_RECTANGLE_ARB;

    glEnable (ms->target);

    //Bind the texture
    glBindTexture (ms->target, ms->texture);

    //Load the parameters
    glTexParameteri (ms->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (ms->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (ms->target, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri (ms->target, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D (ms->target, 0, GL_RGB, 0, 0, 0,
		  GL_RGB, GL_UNSIGNED_BYTE, NULL);

    ms->width = 0;
    ms->height = 0;

    glBindTexture (ms->target, 0);

    glDisable (ms->target);

    return TRUE;
}


static void
magFiniScreen (CompPlugin *p,
	       CompScreen *s)
{
    MAG_SCREEN (s);
    MAG_DISPLAY (s->display);

    //Restore the original function
    UNWRAP (ms, s, paintScreen);
    UNWRAP (ms, s, preparePaintScreen);
    UNWRAP (ms, s, donePaintScreen);

    if (ms->pollHandle)
	(*md->mpFunc->removePositionPolling) (s, ms->pollHandle);

    if (ms->zoom)
	damageScreen (s);

    glDeleteTextures (1, &ms->target);

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
