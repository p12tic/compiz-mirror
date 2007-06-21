/*
 *
 * Compiz scale window title filter plugin
 *
 * scalefilter.c
 *
 * Copyright : (C) 2007 by Danny Baumann
 * E-mail    : maniac@opencompositing.org
 *
 * Copyright : (C) 2006 Diogo Ferreira
 * E-mail    : diogo@underdev.org
 *
 * Rounded corner drawing taken from wall.c:
 * Copyright : (C) 2007 Robert Carr
 * E-mail    : racarr@beryl-project.org
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
 *
 */

#define _GNU_SOURCE
#include <math.h>
#include <string.h>

#include <compiz.h>
#include <scale.h>
#include <text.h>

#include "scalefilter_options.h"

static int displayPrivateIndex;
static int scaleDisplayPrivateIndex;

#define MAX_FILTER_SIZE 32
#define MAX_FILTER_STRING_LEN (MAX_FILTER_SIZE+1)

typedef struct _ScaleFilterInfo {
    CompTimeoutHandle timeoutHandle;

    Pixmap      textPixmap;
    CompTexture textTexture;

    unsigned int outputDevice;

    int textWidth;
    int textHeight;

    CompMatch match;
    CompMatch *origMatch;

    char filterString[MAX_FILTER_STRING_LEN];
    int  filterStringLength;
} ScaleFilterInfo;

typedef struct _ScaleFilterDisplay {
    int screenPrivateIndex;

    HandleEventProc       handleEvent;
    HandleCompizEventProc handleCompizEvent;
} ScaleFilterDisplay;

typedef struct _ScaleFilterScreen {
    PaintOutputProc paintOutput;
    DrawWindowProc drawWindow;

    CompMatch scaleMatch;
    Bool matchApplied;

    ScaleFilterInfo *filterInfo;
} ScaleFilterScreen;

#define GET_FILTER_DISPLAY(d)				          \
    ((ScaleFilterDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define FILTER_DISPLAY(d)		          \
    ScaleFilterDisplay *fd = GET_FILTER_DISPLAY (d)

#define GET_FILTER_SCREEN(s, fd)				              \
    ((ScaleFilterScreen *) (s)->privates[(fd)->screenPrivateIndex].ptr)

#define FILTER_SCREEN(s)						               \
    ScaleFilterScreen *fs = GET_FILTER_SCREEN (s, GET_FILTER_DISPLAY (s->display))

static void
scalefilterFreeFilterText (CompScreen *s)
{
    FILTER_SCREEN (s);

    if (!fs->filterInfo)
	return;

    if (!fs->filterInfo->textPixmap)
	return;

    releasePixmapFromTexture(s, &fs->filterInfo->textTexture);
    XFreePixmap (s->display->display, fs->filterInfo->textPixmap);
    initTexture (s, &fs->filterInfo->textTexture);
    fs->filterInfo->textPixmap = None;
}

static void
scalefilterRenderFilterText (CompScreen *s)
{
    CompTextAttrib tA;
    int            stride;
    void*          data;
    int            x1, x2, y1, y2;
    int            width, height;
    REGION         reg;

    FILTER_SCREEN (s);

    if (!fs->filterInfo)
	return;

    x1 = s->outputDev[fs->filterInfo->outputDevice].region.extents.x1;
    x2 = s->outputDev[fs->filterInfo->outputDevice].region.extents.x2;
    y1 = s->outputDev[fs->filterInfo->outputDevice].region.extents.y1;
    y2 = s->outputDev[fs->filterInfo->outputDevice].region.extents.y2;

    reg.rects    = &reg.extents;
    reg.numRects = 1;

    /* damage the old draw rectangle */
    width  = fs->filterInfo->textWidth + (2 * scalefilterGetBorderSize (s));
    height = fs->filterInfo->textHeight + (2 * scalefilterGetBorderSize (s));

    reg.extents.x1 = x1 + ((x2 - x1) / 2) - (width / 2) - 1;
    reg.extents.x2 = reg.extents.x1 + width + 1;
    reg.extents.y1 = y1 + ((y2 - y1) / 2) - (height / 2) - 1;
    reg.extents.y2 = reg.extents.y1 + height + 1;

    damageScreenRegion (s, &reg);

    scalefilterFreeFilterText (s);

    if (!scalefilterGetFilterDisplay (s))
	return;

    if (fs->filterInfo->filterStringLength == 0)
	return;

    tA.maxwidth = x2 - x1 - (2 * scalefilterGetBorderSize (s));
    tA.maxheight = y2 - y1 - (2 * scalefilterGetBorderSize (s));
    tA.screen = s;
    tA.size = scalefilterGetFontSize (s);
    tA.color[0] = scalefilterGetFontColorRed (s);
    tA.color[1] = scalefilterGetFontColorGreen (s);
    tA.color[2] = scalefilterGetFontColorBlue (s);
    tA.color[3] = scalefilterGetFontColorAlpha (s);
    tA.style = (scalefilterGetFontBold (s)) ?
	       TEXT_STYLE_BOLD : TEXT_STYLE_NORMAL;
    tA.family = "Sans";
    tA.ellipsize = TRUE;

    tA.renderMode = TextRenderNormal;
    tA.data = (void*)fs->filterInfo->filterString;

    if ((*s->display->fileToImage) (s->display, TEXT_ID, (char *)&tA,
				    &fs->filterInfo->textWidth,
				    &fs->filterInfo->textHeight,
				    &stride, &data))
    {
	fs->filterInfo->textPixmap = (Pixmap)data;
	bindPixmapToTexture (s, &fs->filterInfo->textTexture,
			     fs->filterInfo->textPixmap,
			     fs->filterInfo->textWidth,
			     fs->filterInfo->textHeight, 32);
    }
    else
    {
	fs->filterInfo->textPixmap = None;
	fs->filterInfo->textWidth = 0;
	fs->filterInfo->textHeight = 0;
    }

    /* damage the new draw rectangle */
    width  = fs->filterInfo->textWidth + (2 * scalefilterGetBorderSize (s));
    height = fs->filterInfo->textHeight + (2 * scalefilterGetBorderSize (s));

    reg.extents.x1 = x1 + ((x2 - x1) / 2) - (width / 2) - 1;
    reg.extents.x2 = reg.extents.x1 + width + 1;
    reg.extents.y1 = y1 + ((y2 - y1) / 2) - (height / 2) - 1;
    reg.extents.y2 = reg.extents.y1 + height + 1;

    damageScreenRegion (s, &reg);
}

static void
scalefilterDrawFilterText (CompScreen *s,
			   CompOutput *output)
{
    FILTER_SCREEN (s);

    GLboolean wasBlend;
    GLint     oldBlendSrc, oldBlendDst;
    int       ox1, ox2, oy1, oy2;

    float width = fs->filterInfo->textWidth;
    float height = fs->filterInfo->textHeight;
    float border = scalefilterGetBorderSize (s);

    ox1 = output->region.extents.x1;
    ox2 = output->region.extents.x2;
    oy1 = output->region.extents.y1;
    oy2 = output->region.extents.y2;
    float x = ox1 + ((ox2 - ox1) / 2) - (width / 2);
    float y = oy1 + ((oy2 - oy1) / 2) + (height / 2);

    x = floor (x);
    y = floor (y);

    wasBlend = glIsEnabled (GL_BLEND);
    glGetIntegerv (GL_BLEND_SRC, &oldBlendSrc);
    glGetIntegerv (GL_BLEND_DST, &oldBlendDst);

    if (!wasBlend)
	glEnable (GL_BLEND);

    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glColor4us (scalefilterGetBackColorRed (s),
		scalefilterGetBackColorGreen (s),
		scalefilterGetBackColorBlue (s),
		scalefilterGetBackColorAlpha (s));

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
	float rad = k* (3.14159 / 180.0f);\
	glVertex2f (0.0f, 0.0f);\
	glVertex2f (cos (rad) * border, sin (rad) * border);\
	glVertex2f (cos ((k - 1) * (3.14159 / 180.0f)) * border, \
		    sin ((k - 1) * (3.14159 / 180.0f)) * border);\
    }

    /* Rounded corners */
    int k;

    glTranslatef (border, border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (180, 270) glEnd();
    glTranslatef (-border, -border, 0.0f);

    glTranslatef (width + border, border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (270, 360) glEnd();
    glTranslatef (-(width + border), -border, 0.0f);

    glTranslatef (border, height + border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (90, 180) glEnd();
    glTranslatef (-border, -(height + border), 0.0f);

    glTranslatef (width + border, height + border, 0.0f);
    glBegin (GL_TRIANGLES);
    CORNER (0, 90) glEnd();
    glTranslatef (-(width + border), -(height + border), 0.0f);

    glPopMatrix ();

#undef CORNER

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f (1.0, 1.0, 1.0, 1.0);

    enableTexture (s, &fs->filterInfo->textTexture, COMP_TEXTURE_FILTER_GOOD);

    CompMatrix *m = &fs->filterInfo->textTexture.matrix;

    glBegin (GL_QUADS);

    glTexCoord2f (COMP_TEX_COORD_X(m, 0),COMP_TEX_COORD_Y(m ,0));
    glVertex2f (x, y - height);
    glTexCoord2f (COMP_TEX_COORD_X(m, 0),COMP_TEX_COORD_Y(m, height));
    glVertex2f (x, y);
    glTexCoord2f (COMP_TEX_COORD_X(m, width),COMP_TEX_COORD_Y(m, height));
    glVertex2f (x + width, y);
    glTexCoord2f (COMP_TEX_COORD_X(m, width),COMP_TEX_COORD_Y(m, 0));
    glVertex2f (x + width, y - height);

    glEnd ();

    disableTexture (s, &fs->filterInfo->textTexture);
    glColor4usv (defaultColor);

    if (!wasBlend)
	glDisable (GL_BLEND);
    glBlendFunc (oldBlendSrc, oldBlendDst);
}


/*
 * FIXME:
 * we should not need to duplicate these three
 * functions from scale.c
 *
 */
static Bool
scalefilterIsNeverScaleWin (CompWindow *w)
{
    if (w->attrib.override_redirect)
	return TRUE;

    if (w->wmType & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	return TRUE;

    return FALSE;
}

static Bool
scalefilterIsScaleWin (CompWindow *w)
{
    SCALE_SCREEN (w->screen);

    if (scalefilterIsNeverScaleWin (w))
	return FALSE;

    if (!ss->type || ss->type == ScaleTypeOutput)
    {
	if (!(*w->screen->focusWindow) (w))
	    return FALSE;
    }

    if (w->state & CompWindowStateSkipPagerMask)
	return FALSE;

    if (w->state & CompWindowStateShadedMask)
	return FALSE;

    if (!w->mapNum || w->attrib.map_state != IsViewable)
	return FALSE;

    switch (ss->type) {
    case ScaleTypeGroup:
	if (ss->clientLeader != w->clientLeader &&
	    ss->clientLeader != w->id)
	    return FALSE;
	break;
    case ScaleTypeOutput:
	if (outputDeviceForWindow(w) != w->screen->currentOutputDev)
	    return FALSE;
    default:
	break;
    }

    if (!matchEval (ss->currentMatch, w))
	return FALSE;

    return TRUE;
}

static Bool
scalefilterLayoutThumbs (CompScreen *s)
{
    CompWindow *w;

    SCALE_SCREEN (s);

    ss->nWindows = 0;

    /* add windows scale list, top most window first */
    for (w = s->reverseWindows; w; w = w->prev)
    {
        SCALE_WINDOW (w);

        if (sw->slot)
            sw->adjust = TRUE;

        sw->slot = 0;

        if (!scalefilterIsScaleWin (w))
            continue;

        if (ss->windowsSize <= ss->nWindows)
        {
            ss->windows = realloc (ss->windows,
                                   sizeof (CompWindow *) * (ss->nWindows + 32));
            if (!ss->windows)
                return FALSE;

            ss->windowsSize = ss->nWindows + 32;
        }

        ss->windows[ss->nWindows++] = w;
    }

    if (ss->nWindows == 0)
        return FALSE;

    if (ss->slotsSize < ss->nWindows)
    {
        ss->slots = realloc (ss->slots, sizeof (ScaleSlot) * ss->nWindows);
        if (!ss->slots)
            return FALSE;

        ss->slotsSize = ss->nWindows;
    }
    return TRUE;
}

static void
scalefilterRelayout (CompScreen *s)
{
    SCALE_SCREEN (s);
    SCALE_DISPLAY (s->display);

    sd->selectedWindow = None;
    sd->hoveredWindow = None;

    if (scalefilterLayoutThumbs (s)) {
	ss->state = SCALE_STATE_OUT;
	(*ss->layoutSlotsAndAssignWindows) (s);
    }

    damageScreen (s);
}

static void
scalefilterInitFilterInfo (CompScreen *s)
{
    FILTER_SCREEN (s);
    SCALE_SCREEN (s);

    ScaleFilterInfo *info = fs->filterInfo;

    memset (info->filterString, 0, sizeof (info->filterString));
    info->filterStringLength = 0;

    info->textPixmap = None;
    info->textWidth  = 0;
    info->textHeight = 0;

    info->timeoutHandle = 0;

    info->outputDevice = s->currentOutputDev;

    initTexture (s, &info->textTexture);

    matchInit (&info->match);
    matchCopy (&info->match, ss->currentMatch);

    info->origMatch  = ss->currentMatch;
    ss->currentMatch = &info->match;
}

static void
scalefilterFiniFilterInfo (CompScreen *s,
			   Bool freeTimeout)
{
    FILTER_SCREEN (s);

    scalefilterFreeFilterText (s);

    matchFini (&fs->filterInfo->match);

    if (freeTimeout && fs->filterInfo->timeoutHandle)
	compRemoveTimeout (fs->filterInfo->timeoutHandle);

    free (fs->filterInfo);
    fs->filterInfo = NULL;
}

static Bool
scalefilterFilterTimeout (void *closure)
{
    CompScreen *s = (CompScreen *) closure;

    FILTER_SCREEN (s);
    SCALE_SCREEN (s);

    if (fs->filterInfo)
    {
	ss->currentMatch = fs->filterInfo->origMatch;
	scalefilterFiniFilterInfo (s, FALSE);
	scalefilterRelayout (s);
    }

    return FALSE;
}

static void
scalefilterHandleEvent (CompDisplay *d,
	 		XEvent      *event)
{
    FILTER_DISPLAY (d);

    switch (event->type)
    {
    case KeyPress:
    	{
	    CompScreen *s;
	    s = findScreenAtDisplay (d, event->xkey.root);
	    if (s)
	    {
	        SCALE_SCREEN (s);
		if (ss->grabIndex)
		{
		    Bool needRelayout = FALSE;
		    Bool dropKeyEvent = FALSE;
		    char buffer[1];
		    KeySym ks;
		    int count, timeout;
		    ScaleFilterInfo *info;

		    FILTER_SCREEN (s);

		    info = fs->filterInfo;
		    count = XLookupString (&(event->xkey), buffer, 1, &ks, NULL);

		    if (info && ks == XK_Escape)
		    {
			/* Escape key - drop current filter */
			ss->currentMatch = info->origMatch;
			scalefilterFiniFilterInfo (s, TRUE);
			needRelayout = TRUE;
			dropKeyEvent = TRUE;
		    }
		    else if (info && ks == XK_Return)
		    {
			/* Return key - apply current filter persistently */
			matchFini (&ss->match);
			matchInit (&ss->match);
			matchCopy (&ss->match, &info->match);
			matchUpdate (s->display, &ss->match);
			ss->currentMatch = &ss->match;
			fs->matchApplied = TRUE;
			dropKeyEvent = TRUE;
			needRelayout = TRUE;
			scalefilterFiniFilterInfo (s, TRUE);
		    }
		    else if (ks == XK_BackSpace)
		    {
			if (info && info->filterStringLength > 0)
			{
			    /* remove last character in string */
			    info->filterString[--(info->filterStringLength)] = '\0';
			    needRelayout = TRUE;
			}
			else if (fs->matchApplied)
			{
			    /* remove filter applied previously
			       if currently not in input mode */
    			    matchFini (&ss->match);
    			    matchInit (&ss->match);
    			    matchCopy (&ss->match, &fs->scaleMatch);
			    matchUpdate (s->display, &ss->match);
    			    ss->currentMatch = &ss->match;
			    fs->matchApplied = FALSE;
			    needRelayout = TRUE;
			}
		    }
		    else if (count > 0)
		    {
			if (!info)
			{
			    fs->filterInfo = info = malloc (sizeof (ScaleFilterInfo));
			    scalefilterInitFilterInfo (s);
			}
			else if (info->timeoutHandle)
			    compRemoveTimeout (info->timeoutHandle);

			timeout = scalefilterGetTimeout (s);
			if (timeout > 0)
			    info->timeoutHandle =
				compAddTimeout (timeout,
				    		scalefilterFilterTimeout, s);

			if (info->filterStringLength < MAX_FILTER_SIZE)
			{
    			    info->filterString[info->filterStringLength++] = buffer[0];
    			    info->filterString[info->filterStringLength] = '\0';
			    needRelayout = TRUE;
			}
		    }

		    /* set the event type invalid if we
		       don't want other plugins see it */
		    if (dropKeyEvent)
			event->type = LASTEvent+1;

		    if (needRelayout)
		    {
			char *matchTitle;
			info = fs->filterInfo;

			scalefilterRenderFilterText (s);

			if (info)
			{
			    matchFini (&info->match);
			    matchInit (&info->match);

			    if (info->origMatch->nOp > 0)
				asprintf (&matchTitle, "(%s) & (title=%s)",
					  matchToString (info->origMatch),
					  info->filterString);
			    else
				asprintf (&matchTitle, "title=%s", info->filterString);
		
			    matchAddFromString (&info->match, matchTitle);
			    free (matchTitle);

			    matchUpdate (s->display, &info->match);
			}
			scalefilterRelayout (s);
		    }
		}
	    }
	}
	break;
    default:
	break;
    }

    UNWRAP (fd, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (fd, d, handleEvent, scalefilterHandleEvent);
}

static void
scalefilterHandleCompizEvent (CompDisplay *d,
	 		      char        *pluginName,
	 		      char        *eventName,
	 		      CompOption  *option,
	 		      int         nOption)
{
    FILTER_DISPLAY (d);

    UNWRAP (fd, d, handleCompizEvent);
    (*d->handleCompizEvent) (d, pluginName, eventName, option, nOption);
    WRAP (fd, d, handleCompizEvent, scalefilterHandleCompizEvent);

    if ((strcmp (pluginName, "scale") == 0) &&
	(strcmp (eventName, "activate") == 0))
    {
	Window xid = getIntOptionNamed (option, nOption, "root", 0);
	Bool activated = getBoolOptionNamed (option, nOption, "active", FALSE);
	CompScreen *s = findScreenAtDisplay (d, xid);

	if (s)
	{
	    FILTER_SCREEN (s);
	    SCALE_SCREEN (s);

	    if (activated)
	    {
		matchFini (&fs->scaleMatch);
		matchInit (&fs->scaleMatch);
		matchCopy (&fs->scaleMatch, ss->currentMatch);
		matchUpdate (d, &fs->scaleMatch);
		fs->matchApplied = FALSE;
	    }

	    if (!activated)
	    {
		if (fs->filterInfo)
		{
		    ss->currentMatch = fs->filterInfo->origMatch;
		    scalefilterFiniFilterInfo (s, TRUE);
		}
		fs->matchApplied = FALSE;
	    }
	}
    }
}

static Bool
scalefilterPaintOutput (CompScreen              *s,
			const ScreenPaintAttrib *sAttrib,
			const CompTransform     *transform,
			Region                  region,
			CompOutput              *output,
			unsigned int            mask)
{
    Bool status;

    FILTER_SCREEN (s);

    UNWRAP (fs, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
    WRAP (fs, s, paintOutput, scalefilterPaintOutput);

    if (status && fs->filterInfo)
    {
	if ((output->id == ~0 || output->id == fs->filterInfo->outputDevice) &&
	    fs->filterInfo->textPixmap)
	{
	    CompTransform sTransform = *transform;
	    transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);

	    glPushMatrix ();
	    glLoadMatrixf (sTransform.m);

	    scalefilterDrawFilterText (s, output);

	    glPopMatrix ();
	}
    }

    return status;
}

static Bool
scalefilterDrawWindow (CompWindow	     *w,
	     	       const CompTransform  *transform,
       		       const FragmentAttrib *attrib,
		       Region		     region,
		       unsigned int	     mask)
{
    CompScreen *s = w->screen;
    Bool       status;

    FILTER_SCREEN (s);

    if (fs->matchApplied ||
	(fs->filterInfo && fs->filterInfo->filterStringLength))
    {
	FragmentAttrib fA = *attrib;

	SCALE_WINDOW (w);

	if (!sw->slot)
    	    fA.opacity = 0;

	UNWRAP (fs, s, drawWindow);
	status = (*s->drawWindow) (w, transform, &fA, region, mask);
	WRAP (fs, s, drawWindow, scalefilterDrawWindow);
    }
    else
    {
	UNWRAP (fs, s, drawWindow);
	status = (*s->drawWindow) (w, transform, attrib, region, mask);
	WRAP (fs, s, drawWindow, scalefilterDrawWindow);
    }

    return status;
}

static void
scalefilterScreenOptionChanged (CompScreen               *s,
				CompOption               *opt,
	 			ScalefilterScreenOptions num)
{
    switch (num)
    {
	case ScalefilterScreenOptionFontBold:
	case ScalefilterScreenOptionFontSize:
	case ScalefilterScreenOptionFontColor:
	case ScalefilterScreenOptionBackColor:
	    {
		FILTER_SCREEN (s);

		if (fs->filterInfo)
		    scalefilterRenderFilterText (s);
	    }
	    break;
	default:
	    break;
    }
}

static Bool
scalefilterInitDisplay (CompPlugin  *p,
	    		CompDisplay *d)
{
    ScaleFilterDisplay *fd;
    CompPlugin         *scale = findActivePlugin ("scale");
    CompOption         *option;
    int                nOption;

    if (!scale || !scale->vTable->getDisplayOptions)
	return FALSE;

    option = (*scale->vTable->getDisplayOptions) (scale, d, &nOption);

    if (getIntOptionNamed (option, nOption, "abi", 0) != SCALE_ABIVERSION)
    {
	compLogMessage (d, "scalefilter", CompLogLevelError,
			"scale ABI version mismatch");
	return FALSE;
    }

    scaleDisplayPrivateIndex = getIntOptionNamed (option, nOption, "index", -1);
    if (scaleDisplayPrivateIndex < 0)
	return FALSE;

    fd = malloc (sizeof (ScaleFilterDisplay));
    if (!fd)
	return FALSE;

    fd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (fd->screenPrivateIndex < 0)
    {
	free (fd);
	return FALSE;
    }

    WRAP (fd, d, handleEvent, scalefilterHandleEvent);
    WRAP (fd, d, handleCompizEvent, scalefilterHandleCompizEvent);

    d->privates[displayPrivateIndex].ptr = fd;

    return TRUE;
}

static void
scalefilterFiniDisplay (CompPlugin  *p,
	    		CompDisplay *d)
{
    FILTER_DISPLAY (d);

    UNWRAP (fd, d, handleEvent);
    UNWRAP (fd, d, handleCompizEvent);

    freeScreenPrivateIndex (d, fd->screenPrivateIndex);

    free (fd);
}

static Bool
scalefilterInitScreen (CompPlugin *p,
	    	       CompScreen *s)
{
    ScaleFilterScreen *fs;

    FILTER_DISPLAY (s->display);

    fs = malloc (sizeof (ScaleFilterScreen));
    if (!fs)
	return FALSE;

    fs->filterInfo = NULL;
    matchInit (&fs->scaleMatch);
    fs->matchApplied = FALSE;

    WRAP (fs, s, paintOutput, scalefilterPaintOutput);
    WRAP (fs, s, drawWindow, scalefilterDrawWindow);

    scalefilterSetFontBoldNotify (s, scalefilterScreenOptionChanged);
    scalefilterSetFontSizeNotify (s, scalefilterScreenOptionChanged);
    scalefilterSetFontColorNotify (s, scalefilterScreenOptionChanged);
    scalefilterSetBackColorNotify (s, scalefilterScreenOptionChanged);

    s->privates[fd->screenPrivateIndex].ptr = fs;

    return TRUE;
}

static void
scalefilterFiniScreen (CompPlugin *p,
	    	       CompScreen *s)
{
    FILTER_SCREEN (s);
    SCALE_SCREEN (s);

    UNWRAP (fs, s, paintOutput);
    UNWRAP (fs, s, drawWindow);

    if (fs->filterInfo)
    {
	ss->currentMatch = fs->filterInfo->origMatch;
	scalefilterFiniFilterInfo (s, TRUE);
    }

    free (fs);
}

static Bool
scalefilterInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
scalefilterFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}

static int
scalefilterGetVersion (CompPlugin *plugin,
	    	       int	 version)
{
    return ABIVERSION;
}

CompPluginVTable scalefilterVTable = {
    "scalefilter",
    scalefilterGetVersion,
    0,
    scalefilterInit,
    scalefilterFini,
    scalefilterInitDisplay,
    scalefilterFiniDisplay,
    scalefilterInitScreen,
    scalefilterFiniScreen,
    0,
    0,
    0,
    0,
    0,
    0,
    0, /* Deps */
    0, /* nDeps */
    0, /* Features */
    0  /* nFeatures */
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &scalefilterVTable;
}
