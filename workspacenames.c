/*
 *
 * Compiz workspace name display plugin
 *
 * workspacenames.c
 *
 * Copyright : (C) 2008 by Danny Baumann
 * E-mail    : maniac@compiz-fusion.org
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <compiz-core.h>
#include <compiz-text.h>
#include "workspacenames_options.h"

#define PI 3.1415926

static int WSNamesDisplayPrivateIndex;

typedef struct _WSNamesDisplay {
    int		    screenPrivateIndex;
    HandleEventProc handleEvent;
} WSNamesDisplay;

typedef struct _WSNamesScreen {
    PreparePaintScreenProc preparePaintScreen;
    DonePaintScreenProc    donePaintScreen;
    PaintOutputProc        paintOutput;

    CompTexture textTexture;
    Pixmap      textPixmap;
    int         textWidth;
    int         textHeight;

    CompTimeoutHandle timeoutHandle;
    int               timer;
} WSNamesScreen;

#define WSNAMES_DISPLAY(d) PLUGIN_DISPLAY(d, WSNames, w)
#define WSNAMES_SCREEN(s) PLUGIN_SCREEN(s, WSNames, w)

static void 
wsnamesFreeText (CompScreen *s)
{
    WSNAMES_SCREEN(s);

    if (!ws->textPixmap)
	return;

    releasePixmapFromTexture (s, &ws->textTexture);
    initTexture (s, &ws->textTexture);
    XFreePixmap (s->display->display, ws->textPixmap);
    ws->textPixmap = None;
}

static char *
wsnamesGetCurrentWSName (CompScreen *s)
{
    int           currentVp;
    int           listSize, i;
    CompListValue *names;
    CompListValue *vpNumbers;

    vpNumbers = workspacenamesGetViewports (s);
    names     = workspacenamesGetNames (s);

    currentVp = s->y * s->hsize + s->x + 1;
    listSize  = MIN (vpNumbers->nValue, names->nValue);

    for (i = 0; i < listSize; i++)
	if (vpNumbers->value[i].i == currentVp)
	    return names->value[i].s;

    return NULL;
}

static void
wsnamesRenderNameText (CompScreen *s)
{
    CompTextAttrib tA;
    int            stride;
    void           *data;
    char           *name;
    int            ox1, ox2, oy1, oy2;

    WSNAMES_SCREEN (s);

    wsnamesFreeText (s);

    name = wsnamesGetCurrentWSName (s);
    if (!name)
	return;

    getCurrentOutputExtents (s, &ox1, &oy1, &ox2, &oy2);

    /* 75% of the output device as maximum width */
    tA.maxWidth = (ox2 - ox1) * 3 / 4;
    tA.maxHeight = 100;
    tA.screen = s;
    tA.size = workspacenamesGetTextFontSize (s);
    tA.color[0] = workspacenamesGetFontColorRed (s);
    tA.color[1] = workspacenamesGetFontColorGreen (s);
    tA.color[2] = workspacenamesGetFontColorBlue (s);
    tA.color[3] = workspacenamesGetFontColorAlpha (s);
    tA.style = (workspacenamesGetBoldText (s)) ?
	       TEXT_STYLE_BOLD : TEXT_STYLE_NORMAL;
    tA.family = "Sans";
    tA.ellipsize = TRUE;

    tA.renderMode = TextRenderNormal;
    tA.data = (void *) name;

    initTexture (s, &ws->textTexture);

    if ((*s->display->fileToImage) (s->display, TEXT_ID, (char *)&tA,
			 	    &ws->textWidth, &ws->textHeight,
				    &stride, &data))
    {
	ws->textPixmap = (Pixmap) data;

	bindPixmapToTexture (s, &ws->textTexture, ws->textPixmap,
			     ws->textWidth, ws->textHeight, 32);
    }
    else 
    {
	ws->textPixmap = None;
	ws->textWidth  = 0;
	ws->textHeight = 0;
    }
}

static void
wsnamesDrawText (CompScreen *s)
{
    GLboolean wasBlend;
    GLint     oldBlendSrc, oldBlendDst;
    GLfloat   alpha;
    float     width, height, border;
    int       ox1, ox2, oy1, oy2;
    int       k;
    float     x, y;

    WSNAMES_SCREEN(s);

    width  = ws->textWidth;
    height = ws->textHeight;
    border = 10.0f;

    getCurrentOutputExtents (s, &ox1, &oy1, &ox2, &oy2);

    x = ox1 + ((ox2 - ox1) / 2) - (ws->textWidth / 2);

    /* assign y (for the lower corner!) according to the setting */
    switch (workspacenamesGetTextPlacement (s))
    {
	case TextPlacementCenteredOnScreen:
	    y = oy1 + ((oy2 - oy1) / 2) + (height / 2);
	    break;
	case TextPlacementTopOfScreen:
	case TextPlacementBottomOfScreen:
	    {
		XRectangle workArea;
		getWorkareaForOutput (s, s->currentOutputDev, &workArea);

	    	if (workspacenamesGetTextPlacement (s) == 
		    TextPlacementTopOfScreen)
    		    y = oy1 + workArea.y + (2 * border) + height;
		else
		    y = oy1 + workArea.y + workArea.height - (2 * border);
	    }
	    break;
	default:
	    return;
	    break;
    }

    x = floor (x);
    y = floor (y);

    glGetIntegerv (GL_BLEND_SRC, &oldBlendSrc);
    glGetIntegerv (GL_BLEND_DST, &oldBlendDst);
    wasBlend = glIsEnabled (GL_BLEND);

    if (!wasBlend)
	glEnable (GL_BLEND);
    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    if (ws->timer)
	alpha = ws->timer / (workspacenamesGetFadeTime (s) * 1000.0f);
    else
	alpha = 1.0f;

    glColor4us (workspacenamesGetBackColorRed (s),
		workspacenamesGetBackColorGreen (s),
		workspacenamesGetBackColorBlue (s),
		workspacenamesGetBackColorAlpha (s) * alpha);

    glPushMatrix ();

    glTranslatef (x, y - height, 0.0f);
    glRectf (0.0f, height, width, 0.0f);
    glRectf (0.0f, 0.0f, width, -border);
    glRectf (0.0f, height + border, width, height);
    glRectf (-border, height, 0.0f, 0.0f);
    glRectf (width, height, width + border, 0.0f);
    glTranslatef (-border, -border, 0.0f);

#define CORNER(a,b) \
    for (k = a; k < b; k++) \
    {\
	float rad = k * (PI / 180.0f);\
	glVertex2f (0.0f, 0.0f);\
	glVertex2f (cos (rad) * border, sin (rad) * border);\
	glVertex2f (cos ((k - 1) * (PI / 180.0f)) * border, \
		    sin ((k - 1) * (PI / 180.0f)) * border);\
    }

    /* Rounded corners */

    glTranslatef (border, border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (180, 270) glEnd ();
    glTranslatef (-border, -border, 0.0f);

    glTranslatef (width + border, border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (270, 360) glEnd ();
    glTranslatef (-(width + border), -border, 0.0f);

    glTranslatef (border, height + border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (90, 180) glEnd ();
    glTranslatef (-border, -(height + border), 0.0f);

    glTranslatef (width + border, height + border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (0, 90) glEnd ();
    glTranslatef (-(width + border), -(height + border), 0.0f);

    glPopMatrix ();

#undef CORNER

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f (alpha, alpha, alpha, alpha);

    enableTexture (s, &ws->textTexture,COMP_TEXTURE_FILTER_GOOD);

    CompMatrix *m = &ws->textTexture.matrix;

    glBegin (GL_QUADS);

    glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m ,0));
    glVertex2f (x, y - height);
    glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m, height));
    glVertex2f (x, y);
    glTexCoord2f (COMP_TEX_COORD_X (m, width), COMP_TEX_COORD_Y (m, height));
    glVertex2f (x + width, y);
    glTexCoord2f (COMP_TEX_COORD_X (m, width), COMP_TEX_COORD_Y (m, 0));
    glVertex2f (x + width, y - height);

    glEnd ();

    disableTexture (s, &ws->textTexture);
    glColor4usv (defaultColor);

    if (!wasBlend)
	glDisable (GL_BLEND);
    glBlendFunc (oldBlendSrc, oldBlendDst);
}

static Bool
wsnamesPaintOutput (CompScreen		    *s,
		    const ScreenPaintAttrib *sAttrib,
		    const CompTransform	    *transform,
		    Region		    region,
		    CompOutput		    *output,
		    unsigned int	    mask)
{
    Bool status;

    WSNAMES_SCREEN (s);

    UNWRAP (ws, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
    WRAP (ws, s, paintOutput, wsnamesPaintOutput);

    if (ws->textPixmap)
    {
	CompTransform sTransform = *transform;

	transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);
	glPushMatrix ();
	glLoadMatrixf (sTransform.m);

	wsnamesDrawText (s);

	glPopMatrix ();
    }
	
    return status;
}

static void
wsnamesPreparePaintScreen (CompScreen *s,
			   int	      msSinceLastPaint)
{
    WSNAMES_SCREEN (s);

    if (ws->timer)
    {
	ws->timer -= msSinceLastPaint;
	ws->timer = MAX (ws->timer, 0);

	if (!ws->timer)
	    wsnamesFreeText (s);
    }

    UNWRAP (ws, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, msSinceLastPaint);
    WRAP (ws, s, preparePaintScreen, wsnamesPreparePaintScreen);
}

static void
wsnamesDonePaintScreen (CompScreen *s)
{
    WSNAMES_SCREEN (s);

    /* FIXME: better only damage paint region */
    if (ws->timer)
	damageScreen (s);

    UNWRAP (ws, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (ws, s, donePaintScreen, wsnamesDonePaintScreen);
}

static Bool
wsnamesHideTimeout (void *closure)
{
    CompScreen *s = (CompScreen *) closure;

    WSNAMES_SCREEN (s);

    ws->timer = workspacenamesGetFadeTime (s) * 1000;
    if (!ws->timer)
	wsnamesFreeText (s);

    damageScreen (s);

    ws->timeoutHandle = 0;

    return FALSE;
}

static void
wsnamesHandleEvent (CompDisplay *d,
		    XEvent      *event)
{
    WSNAMES_DISPLAY (d);

    UNWRAP (wd, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (wd, d, handleEvent, wsnamesHandleEvent);

    switch (event->type) {
    case PropertyNotify:
	if (event->xproperty.atom == d->desktopViewportAtom)
	{
	    CompScreen *s;
	    s = findScreenAtDisplay (d, event->xproperty.window);
	    if (s)
	    {
		int timeout;

		WSNAMES_SCREEN (s);

		ws->timer = 0;
		if (ws->timeoutHandle)
		    compRemoveTimeout (ws->timeoutHandle);

		wsnamesRenderNameText (s);
		timeout = workspacenamesGetDisplayTime (s) * 1000;
		ws->timeoutHandle = compAddTimeout (timeout,
						    wsnamesHideTimeout, s);

		damageScreen (s);
	    }
	}
	break;
    }
}

static Bool
wsnamesInitDisplay (CompPlugin  *p,
		    CompDisplay *d)
{
    WSNamesDisplay *wd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    if (!checkPluginABI ("text", TEXT_ABIVERSION))
	return FALSE;

    wd = malloc (sizeof (WSNamesDisplay));
    if (!wd)
	return FALSE;

    wd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (wd->screenPrivateIndex < 0)
    {
	free (wd);
	return FALSE;
    }

    WRAP (wd, d, handleEvent, wsnamesHandleEvent);

    d->base.privates[WSNamesDisplayPrivateIndex].ptr = wd;

    return TRUE;
}

static void
wsnamesFiniDisplay (CompPlugin  *p,
		    CompDisplay *d)
{
    WSNAMES_DISPLAY (d);

    freeScreenPrivateIndex (d, wd->screenPrivateIndex);

    UNWRAP (wd, d, handleEvent);

    free (wd);
}

static Bool
wsnamesInitScreen (CompPlugin *p,
		   CompScreen *s)
{
    WSNamesScreen *ws;

    WSNAMES_DISPLAY (s->display);

    ws = malloc (sizeof (WSNamesScreen));
    if (!ws)
	return FALSE;

    ws->textPixmap = None;
    ws->textHeight = 0;
    ws->textWidth  = 0;

    ws->timeoutHandle = 0;
    ws->timer         = 0;

    WRAP (ws, s, preparePaintScreen, wsnamesPreparePaintScreen);
    WRAP (ws, s, donePaintScreen, wsnamesDonePaintScreen);
    WRAP (ws, s, paintOutput, wsnamesPaintOutput);

    s->base.privates[wd->screenPrivateIndex].ptr = ws;

    return TRUE;
}

static void
wsnamesFiniScreen (CompPlugin *p,
		   CompScreen *s)
{
    WSNAMES_SCREEN (s);

    UNWRAP (ws, s, preparePaintScreen);
    UNWRAP (ws, s, donePaintScreen);
    UNWRAP (ws, s, paintOutput);

    wsnamesFreeText (s);

    free (ws);
}

static CompBool
wsnamesInitObject (CompPlugin *p,
		   CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) wsnamesInitDisplay,
	(InitPluginObjectProc) wsnamesInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
wsnamesFiniObject (CompPlugin *p,
		   CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) wsnamesFiniDisplay,
	(FiniPluginObjectProc) wsnamesFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
wsnamesInit (CompPlugin *p)
{
    WSNamesDisplayPrivateIndex = allocateDisplayPrivateIndex ();
    if (WSNamesDisplayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
wsnamesFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (WSNamesDisplayPrivateIndex);
}

CompPluginVTable wsnamesVTable = {
    "workspacenames",
    0,
    wsnamesInit,
    wsnamesFini,
    wsnamesInitObject,
    wsnamesFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &wsnamesVTable;
}
