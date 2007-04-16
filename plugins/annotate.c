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
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <cairo-xlib-xrender.h>

#include <compiz.h>

#define ANNO_INITIATE_BUTTON_DEFAULT	       Button1
#define ANNO_INITIATE_BUTTON_MODIFIERS_DEFAULT (CompSuperMask | CompAltMask)

#define ANNO_ERASE_BUTTON_DEFAULT	    Button3
#define ANNO_ERASE_BUTTON_MODIFIERS_DEFAULT (CompSuperMask | CompAltMask)

#define ANNO_CLEAR_KEY_DEFAULT	         "k"
#define ANNO_CLEAR_KEY_MODIFIERS_DEFAULT (CompSuperMask | CompAltMask)

#define ANNO_FILL_COLOR_RED_DEFAULT   0xffff
#define ANNO_FILL_COLOR_GREEN_DEFAULT 0x0000
#define ANNO_FILL_COLOR_BLUE_DEFAULT  0x0000

#define ANNO_STROKE_COLOR_RED_DEFAULT   0x0000
#define ANNO_STROKE_COLOR_GREEN_DEFAULT 0xffff
#define ANNO_STROKE_COLOR_BLUE_DEFAULT  0x0000

#define ANNO_LINE_WIDTH_MIN        0.1f
#define ANNO_LINE_WIDTH_MAX        100.0f
#define ANNO_LINE_WIDTH_DEFAULT    3.0f
#define ANNO_LINE_WIDTH_PRECISION  0.1f

#define ANNO_STROKE_WIDTH_MIN        0.1f
#define ANNO_STROKE_WIDTH_MAX        20.0f
#define ANNO_STROKE_WIDTH_DEFAULT    1.0f
#define ANNO_STROKE_WIDTH_PRECISION  0.1f

static int displayPrivateIndex;

static int annoLastPointerX = 0;
static int annoLastPointerY = 0;

#define ANNO_DISPLAY_OPTION_INITIATE     0
#define ANNO_DISPLAY_OPTION_DRAW	 1
#define ANNO_DISPLAY_OPTION_ERASE        2
#define ANNO_DISPLAY_OPTION_CLEAR        3
#define ANNO_DISPLAY_OPTION_FILL_COLOR   4
#define ANNO_DISPLAY_OPTION_STROKE_COLOR 5
#define ANNO_DISPLAY_OPTION_LINE_WIDTH   6
#define ANNO_DISPLAY_OPTION_STROKE_WIDTH 7
#define ANNO_DISPLAY_OPTION_NUM	         8

typedef struct _AnnoDisplay {
    int		    screenPrivateIndex;
    HandleEventProc handleEvent;

    CompOption opt[ANNO_DISPLAY_OPTION_NUM];
} AnnoDisplay;

typedef struct _AnnoScreen {
    PaintScreenProc paintScreen;
    int		    grabIndex;

    Pixmap	    pixmap;
    CompTexture	    texture;
    cairo_surface_t *surface;
    cairo_t	    *cairo;
    Bool            content;

    Bool eraseMode;
} AnnoScreen;

#define GET_ANNO_DISPLAY(d)				     \
    ((AnnoDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define ANNO_DISPLAY(d)			   \
    AnnoDisplay *ad = GET_ANNO_DISPLAY (d)

#define GET_ANNO_SCREEN(s, ad)					 \
    ((AnnoScreen *) (s)->privates[(ad)->screenPrivateIndex].ptr)

#define ANNO_SCREEN(s)							\
    AnnoScreen *as = GET_ANNO_SCREEN (s, GET_ANNO_DISPLAY (s->display))

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))


#define NUM_TOOLS (sizeof (tools) / sizeof (tools[0]))

static void
annoCairoClear (CompScreen *s,
		cairo_t    *cr)
{
    ANNO_SCREEN (s);

    cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);
    cairo_restore (cr);

    as->content = FALSE;
}

static cairo_t *
annoCairoContext (CompScreen *s)
{
    ANNO_SCREEN (s);

    if (!as->cairo)
    {
	XRenderPictFormat *format;
	Screen		  *screen;
	int		  w, h;

	screen = ScreenOfDisplay (s->display->display, s->screenNum);

	w = s->width;
	h = s->height;

	format = XRenderFindStandardFormat (s->display->display,
					    PictStandardARGB32);

	as->pixmap = XCreatePixmap (s->display->display, s->root, w, h, 32);

	if (!bindPixmapToTexture (s, &as->texture, as->pixmap, w, h, 32))
	{
	    fprintf (stderr, "%s: Couldn't bind annotate pixmap 0x%x to "
		     "texture\n", programName, (int) as->pixmap);

	    XFreePixmap (s->display->display, as->pixmap);

	    return NULL;
	}

	as->surface =
	    cairo_xlib_surface_create_with_xrender_format (s->display->display,
							   as->pixmap, screen,
							   format, w, h);

	as->cairo = cairo_create (as->surface);

	annoCairoClear (s, as->cairo);
    }

    return as->cairo;
}

static void
annoSetSourceColor (cairo_t	   *cr,
		    unsigned short *color)
{
    cairo_set_source_rgba (cr,
			   (double) color[0] / 0xffff,
			   (double) color[1] / 0xffff,
			   (double) color[2] / 0xffff,
			   (double) color[3] / 0xffff);
}

static void
annoDrawCircle (CompScreen     *s,
		double	       xc,
		double	       yc,
		double	       radius,
		unsigned short *fillColor,
		unsigned short *strokeColor,
		double	       strokeWidth)
{
    REGION  reg;
    cairo_t *cr;

    ANNO_SCREEN (s);

    cr = annoCairoContext (s);
    if (cr)
    {
	double  ex1, ey1, ex2, ey2;

	annoSetSourceColor (cr, fillColor);
	cairo_arc (cr, xc, yc, radius, 0, 2 * M_PI);
	cairo_fill_preserve (cr);
	cairo_set_line_width (cr, strokeWidth);
	cairo_stroke_extents (cr, &ex1, &ey1, &ex2, &ey2);
	annoSetSourceColor (cr, strokeColor);
	cairo_stroke (cr);

	reg.rects    = &reg.extents;
	reg.numRects = 1;

	reg.extents.x1 = ex1;
	reg.extents.y1 = ey1;
	reg.extents.x2 = ex2;
	reg.extents.y2 = ey2;

	as->content = TRUE;
	damageScreenRegion (s, &reg);
    }
}

static void
annoDrawRectangle (CompScreen	  *s,
		   double	  x,
		   double	  y,
		   double	  w,
		   double	  h,
		   unsigned short *fillColor,
		   unsigned short *strokeColor,
		   double	  strokeWidth)
{
    REGION reg;
    cairo_t *cr;

    ANNO_SCREEN (s);

    cr = annoCairoContext (s);
    if (cr)
    {
	double  ex1, ey1, ex2, ey2;

	annoSetSourceColor (cr, fillColor);
	cairo_rectangle (cr, x, y, w, h);
	cairo_fill_preserve (cr);
	cairo_set_line_width (cr, strokeWidth);
	cairo_stroke_extents (cr, &ex1, &ey1, &ex2, &ey2);
	annoSetSourceColor (cr, strokeColor);
	cairo_stroke (cr);

	reg.rects    = &reg.extents;
	reg.numRects = 1;

	reg.extents.x1 = ex1;
	reg.extents.y1 = ey1;
	reg.extents.x2 = ex2 + 2.0;
	reg.extents.y2 = ey2 + 2.0;

	as->content = TRUE;
	damageScreenRegion (s, &reg);
    }
}

static void
annoDrawLine (CompScreen     *s,
	      double	     x1,
	      double	     y1,
	      double	     x2,
	      double	     y2,
	      double	     width,
	      unsigned short *color)
{
    REGION reg;
    cairo_t *cr;

    ANNO_SCREEN (s);

    cr = annoCairoContext (s);
    if (cr)
    {
	double ex1, ey1, ex2, ey2;

	cairo_set_line_width (cr, width);
	cairo_move_to (cr, x1, y1);
	cairo_line_to (cr, x2, y2);
	cairo_stroke_extents (cr, &ex1, &ey1, &ex2, &ey2);
	annoSetSourceColor (cr, color);
	cairo_stroke (cr);

	reg.rects    = &reg.extents;
	reg.numRects = 1;

	reg.extents.x1 = ex1;
	reg.extents.y1 = ey1;
	reg.extents.x2 = ex2;
	reg.extents.y2 = ey2;

	as->content = TRUE;
	damageScreenRegion (s, &reg);
    }
}

static void
annoDrawText (CompScreen     *s,
	      double	     x,
	      double	     y,
	      char	     *text,
	      char	     *fontFamily,
	      double	     fontSize,
	      int	     fontSlant,
	      int	     fontWeight,
	      unsigned short *fillColor,
	      unsigned short *strokeColor,
	      double	     strokeWidth)
{
    REGION  reg;
    cairo_t *cr;

    ANNO_SCREEN (s);

    cr = annoCairoContext (s);
    if (cr)
    {
	cairo_text_extents_t extents;

	cairo_set_line_width (cr, strokeWidth);
	annoSetSourceColor (cr, fillColor);
	cairo_select_font_face (cr, fontFamily, fontSlant, fontWeight);
	cairo_set_font_size (cr, fontSize);
	cairo_text_extents (cr, text, &extents);
	cairo_save (cr);
	cairo_move_to (cr, x, y);
	cairo_text_path (cr, text);
	cairo_fill_preserve (cr);
	annoSetSourceColor (cr, strokeColor);
	cairo_stroke (cr);
	cairo_restore (cr);

	reg.rects    = &reg.extents;
	reg.numRects = 1;

	reg.extents.x1 = x;
	reg.extents.y1 = y + extents.y_bearing - 2.0;
	reg.extents.x2 = x + extents.width + 20.0;
	reg.extents.y2 = y + extents.height;

	as->content = TRUE;
	damageScreenRegion (s, &reg);
    }
}

static Bool
annoDraw (CompDisplay     *d,
	  CompAction      *action,
	  CompActionState state,
	  CompOption      *option,
	  int		  nOption)
{
    CompScreen *s;
    Window     xid;

    xid  = getIntOptionNamed (option, nOption, "root", 0);

    s = findScreenAtDisplay (d, xid);
    if (s)
    {
	cairo_t *cr;

	cr = annoCairoContext (s);
	if (cr)
	{
	    char	   *tool;
	    unsigned short *fillColor, *strokeColor;
	    double	   lineWidth, strokeWidth;

	    ANNO_DISPLAY (d);

	    tool = getStringOptionNamed (option, nOption, "tool", "line");

	    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	    cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);

	    fillColor = ad->opt[ANNO_DISPLAY_OPTION_FILL_COLOR].value.c;
	    fillColor = getColorOptionNamed (option, nOption, "fill_color",
					     fillColor);

	    strokeColor = ad->opt[ANNO_DISPLAY_OPTION_STROKE_COLOR].value.c;
	    strokeColor = getColorOptionNamed (option, nOption,
					       "stroke_color", strokeColor);

	    strokeWidth = ad->opt[ANNO_DISPLAY_OPTION_STROKE_WIDTH].value.f;
	    strokeWidth = getFloatOptionNamed (option, nOption, "stroke_width",
					       strokeWidth);

	    lineWidth = ad->opt[ANNO_DISPLAY_OPTION_LINE_WIDTH].value.f;
	    lineWidth = getFloatOptionNamed (option, nOption, "line_width",
					     lineWidth);

	    if (strcasecmp (tool, "rectangle") == 0)
	    {
		double x, y, w, h;

		x = getFloatOptionNamed (option, nOption, "x", 0);
		y = getFloatOptionNamed (option, nOption, "y", 0);
		w = getFloatOptionNamed (option, nOption, "w", 100);
		h = getFloatOptionNamed (option, nOption, "h", 100);

		annoDrawRectangle (s, x, y, w, h, fillColor, strokeColor,
				   strokeWidth);
	    }
	    else if (strcasecmp (tool, "circle") == 0)
	    {
		double xc, yc, r;

		xc = getFloatOptionNamed (option, nOption, "xc", 0);
		yc = getFloatOptionNamed (option, nOption, "yc", 0);
		r  = getFloatOptionNamed (option, nOption, "radius", 100);

		annoDrawCircle (s, xc, yc, r, fillColor, strokeColor,
				strokeWidth);
	    }
	    else if (strcasecmp (tool, "line") == 0)
	    {
		double x1, y1, x2, y2;

		x1 = getFloatOptionNamed (option, nOption, "x1", 0);
		y1 = getFloatOptionNamed (option, nOption, "y1", 0);
		x2 = getFloatOptionNamed (option, nOption, "x2", 100);
		y2 = getFloatOptionNamed (option, nOption, "y2", 100);

		annoDrawLine (s, x1, y1, x2, y2, lineWidth, fillColor);
	    }
	    else if (strcasecmp (tool, "text") == 0)
	    {
		double	     x, y, size;
		char	     *text, *family;
		unsigned int slant, weight;
		char	     *str;

		str = getStringOptionNamed (option, nOption, "slant", "");
		if (strcasecmp (str, "oblique") == 0)
		    slant = CAIRO_FONT_SLANT_OBLIQUE;
		else if (strcasecmp (str, "italic") == 0)
		    slant = CAIRO_FONT_SLANT_ITALIC;
		else
		    slant = CAIRO_FONT_SLANT_NORMAL;

		str = getStringOptionNamed (option, nOption, "weight", "");
		if (strcasecmp (str, "bold") == 0)
		    weight = CAIRO_FONT_WEIGHT_BOLD;
		else
		    weight = CAIRO_FONT_WEIGHT_NORMAL;

		x      = getFloatOptionNamed (option, nOption, "x", 0);
		y      = getFloatOptionNamed (option, nOption, "y", 0);
		text   = getStringOptionNamed (option, nOption, "text", "");
		family = getStringOptionNamed (option, nOption, "family",
					       "Sans");
		size   = getFloatOptionNamed (option, nOption, "size", 36.0);

		annoDrawText (s, x, y, text, family, size, slant, weight,
			      fillColor, strokeColor, strokeWidth);
	    }
	}
    }

    return FALSE;
}

static Bool
annoInitiate (CompDisplay     *d,
	      CompAction      *action,
	      CompActionState state,
	      CompOption      *option,
	      int	      nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    s = findScreenAtDisplay (d, xid);
    if (s)
    {
	ANNO_SCREEN (s);

	if (otherScreenGrabExist (s, 0))
	    return FALSE;

	if (!as->grabIndex)
	    as->grabIndex = pushScreenGrab (s, None, "annotate");

	if (state & CompActionStateInitButton)
	    action->state |= CompActionStateTermButton;

	if (state & CompActionStateInitKey)
	    action->state |= CompActionStateTermKey;

	annoLastPointerX = pointerX;
	annoLastPointerY = pointerY;

	as->eraseMode = FALSE;
    }

    return TRUE;
}

static Bool
annoTerminate (CompDisplay     *d,
	       CompAction      *action,
	       CompActionState state,
	       CompOption      *option,
	       int	       nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    for (s = d->screens; s; s = s->next)
    {
	ANNO_SCREEN (s);

	if (xid && s->root != xid)
	    continue;

	if (as->grabIndex)
	{
	    removeScreenGrab (s, as->grabIndex, NULL);
	    as->grabIndex = 0;
	}
    }

    action->state &= ~(CompActionStateTermKey | CompActionStateTermButton);

    return FALSE;
}

static Bool
annoEraseInitiate (CompDisplay     *d,
		   CompAction      *action,
		   CompActionState state,
		   CompOption      *option,
		   int	           nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    s = findScreenAtDisplay (d, xid);
    if (s)
    {
	ANNO_SCREEN (s);

	if (otherScreenGrabExist (s, 0))
	    return FALSE;

	if (!as->grabIndex)
	    as->grabIndex = pushScreenGrab (s, None, "annotate");

	if (state & CompActionStateInitButton)
	    action->state |= CompActionStateTermButton;

	if (state & CompActionStateInitKey)
	    action->state |= CompActionStateTermKey;

	annoLastPointerX = pointerX;
	annoLastPointerY = pointerY;

	as->eraseMode = TRUE;
    }

    return FALSE;
}

static Bool
annoClear (CompDisplay     *d,
	   CompAction      *action,
	   CompActionState state,
	   CompOption      *option,
	   int		   nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);

    s = findScreenAtDisplay (d, xid);
    if (s)
    {
	ANNO_SCREEN (s);

	if (as->content)
	{
	    cairo_t *cr;

	    cr = annoCairoContext (s);
	    if (cr)
		annoCairoClear (s, as->cairo);

	    damageScreen (s);
	}

	return TRUE;
    }

    return FALSE;
}

static Bool
annoPaintScreen (CompScreen		 *s,
		 const ScreenPaintAttrib *sAttrib,
		 const CompTransform	 *transform,
		 Region			 region,
		 int			 output,
		 unsigned int		 mask)
{
    Bool status;

    ANNO_SCREEN (s);

    UNWRAP (as, s, paintScreen);
    status = (*s->paintScreen) (s, sAttrib, transform, region, output, mask);
    WRAP (as, s, paintScreen, annoPaintScreen);

    if (status && as->content && region->numRects)
    {
	BoxPtr pBox;
	int    nBox;

	glPushMatrix ();

	prepareXCoords (s, output, -DEFAULT_Z_CAMERA);

	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
	glEnable (GL_BLEND);

	enableTexture (s, &as->texture, COMP_TEXTURE_FILTER_FAST);

	pBox = region->rects;
	nBox = region->numRects;

	glBegin (GL_QUADS);

	while (nBox--)
	{
	    glTexCoord2f (COMP_TEX_COORD_X (&as->texture.matrix, pBox->x1),
			  COMP_TEX_COORD_Y (&as->texture.matrix, pBox->y2));
	    glVertex2i (pBox->x1, pBox->y2);
	    glTexCoord2f (COMP_TEX_COORD_X (&as->texture.matrix, pBox->x2),
			  COMP_TEX_COORD_Y (&as->texture.matrix, pBox->y2));
	    glVertex2i (pBox->x2, pBox->y2);
	    glTexCoord2f (COMP_TEX_COORD_X (&as->texture.matrix, pBox->x2),
			  COMP_TEX_COORD_Y (&as->texture.matrix, pBox->y1));
	    glVertex2i (pBox->x2, pBox->y1);
	    glTexCoord2f (COMP_TEX_COORD_X (&as->texture.matrix, pBox->x1),
			  COMP_TEX_COORD_Y (&as->texture.matrix, pBox->y1));
	    glVertex2i (pBox->x1, pBox->y1);

	    pBox++;
	}

	glEnd ();

	disableTexture (s, &as->texture);

	glDisable (GL_BLEND);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);

	glPopMatrix ();
    }

    return status;
}

static void
annoHandleMotionEvent (CompScreen *s,
		       int	  xRoot,
		       int	  yRoot)
{
    ANNO_SCREEN (s);

    if (as->grabIndex)
    {
	if (as->eraseMode)
	{
	    static unsigned short color[] = { 0, 0, 0, 0 };

	    annoDrawLine (s,
			  annoLastPointerX, annoLastPointerY,
			  xRoot, yRoot,
			  20.0, color);
	}
	else
	{
	    ANNO_DISPLAY(s->display);

	    annoDrawLine (s,
			  annoLastPointerX, annoLastPointerY,
			  xRoot, yRoot,
			  ad->opt[ANNO_DISPLAY_OPTION_LINE_WIDTH].value.f,
			  ad->opt[ANNO_DISPLAY_OPTION_FILL_COLOR].value.c);
	}

	annoLastPointerX = xRoot;
	annoLastPointerY = yRoot;
    }
}

static void
annoHandleEvent (CompDisplay *d,
		 XEvent      *event)
{
    CompScreen *s;

    ANNO_DISPLAY (d);

    switch (event->type) {
    case MotionNotify:
	s = findScreenAtDisplay (d, event->xmotion.root);
	if (s)
	    annoHandleMotionEvent (s, pointerX, pointerY);
	break;
    case EnterNotify:
    case LeaveNotify:
	s = findScreenAtDisplay (d, event->xcrossing.root);
	if (s)
	    annoHandleMotionEvent (s, pointerX, pointerY);
    default:
	break;
    }

    UNWRAP (ad, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (ad, d, handleEvent, annoHandleEvent);
}

static void
annoDisplayInitOptions (AnnoDisplay *ad,
			Display	    *display)
{
    CompOption *o;

    o = &ad->opt[ANNO_DISPLAY_OPTION_INITIATE];
    o->name			     = "initiate";
    o->shortDesc		     = N_("Initiate");
    o->longDesc			     = N_("Initiate annotate drawing");
    o->type			     = CompOptionTypeAction;
    o->value.action.initiate	     = annoInitiate;
    o->value.action.terminate	     = annoTerminate;
    o->value.action.bell	     = FALSE;
    o->value.action.edgeMask	     = 0;
    o->value.action.type	     = CompBindingTypeButton;
    o->value.action.state	     = CompActionStateInitButton;
    o->value.action.state	    |= CompActionStateInitKey;
    o->value.action.button.modifiers = ANNO_INITIATE_BUTTON_MODIFIERS_DEFAULT;
    o->value.action.button.button    = ANNO_INITIATE_BUTTON_DEFAULT;

    o = &ad->opt[ANNO_DISPLAY_OPTION_DRAW];
    o->name		      = "draw";
    o->shortDesc	      = N_("Draw");
    o->longDesc		      = N_("Draw using tool");
    o->type		      = CompOptionTypeAction;
    o->value.action.initiate  = annoDraw;
    o->value.action.terminate = 0;
    o->value.action.bell      = FALSE;
    o->value.action.edgeMask  = 0;
    o->value.action.type      = 0;
    o->value.action.state     = 0;

    o = &ad->opt[ANNO_DISPLAY_OPTION_ERASE];
    o->name			     = "erase";
    o->shortDesc		     = N_("Initiate erase");
    o->longDesc			     = N_("Initiate annotate erasing");
    o->type			     = CompOptionTypeAction;
    o->value.action.initiate	     = annoEraseInitiate;
    o->value.action.terminate	     = annoTerminate;
    o->value.action.bell	     = FALSE;
    o->value.action.edgeMask	     = 0;
    o->value.action.type	     = CompBindingTypeButton;
    o->value.action.state	     = CompActionStateInitButton;
    o->value.action.state	    |= CompActionStateInitKey;
    o->value.action.button.modifiers = ANNO_ERASE_BUTTON_MODIFIERS_DEFAULT;
    o->value.action.button.button    = ANNO_ERASE_BUTTON_DEFAULT;

    o = &ad->opt[ANNO_DISPLAY_OPTION_CLEAR];
    o->name			  = "clear";
    o->shortDesc		  = N_("Clear");
    o->longDesc			  = N_("Clear");
    o->type			  = CompOptionTypeAction;
    o->value.action.initiate	  = annoClear;
    o->value.action.terminate	  = 0;
    o->value.action.bell	  = FALSE;
    o->value.action.edgeMask	  = 0;
    o->value.action.type	  = CompBindingTypeKey;
    o->value.action.state	  = CompActionStateInitEdge;
    o->value.action.state	 |= CompActionStateInitButton;
    o->value.action.state	 |= CompActionStateInitKey;
    o->value.action.key.modifiers = ANNO_CLEAR_KEY_MODIFIERS_DEFAULT;
    o->value.action.key.keycode   =
	XKeysymToKeycode (display,
			  XStringToKeysym (ANNO_CLEAR_KEY_DEFAULT));

    o = &ad->opt[ANNO_DISPLAY_OPTION_FILL_COLOR];
    o->name       = "fill_color";
    o->shortDesc  = N_("Annotate Fill Color");
    o->longDesc   = N_("Fill color for annotations");
    o->type       = CompOptionTypeColor;
    o->value.c[0] = ANNO_FILL_COLOR_RED_DEFAULT;
    o->value.c[1] = ANNO_FILL_COLOR_GREEN_DEFAULT;
    o->value.c[2] = ANNO_FILL_COLOR_BLUE_DEFAULT;
    o->value.c[3] = 0xffff;

    o = &ad->opt[ANNO_DISPLAY_OPTION_STROKE_COLOR];
    o->name       = "stroke_color";
    o->shortDesc  = N_("Annotate Stroke Color");
    o->longDesc   = N_("Stroke color for annotations");
    o->type       = CompOptionTypeColor;
    o->value.c[0] = ANNO_STROKE_COLOR_RED_DEFAULT;
    o->value.c[1] = ANNO_STROKE_COLOR_GREEN_DEFAULT;
    o->value.c[2] = ANNO_STROKE_COLOR_BLUE_DEFAULT;
    o->value.c[3] = 0xffff;

    o = &ad->opt[ANNO_DISPLAY_OPTION_LINE_WIDTH];
    o->name             = "line_width";
    o->shortDesc        = N_("Line width");
    o->longDesc         = N_("Line width for annotations");
    o->type             = CompOptionTypeFloat;
    o->value.f          = ANNO_LINE_WIDTH_DEFAULT;
    o->rest.f.min       = ANNO_LINE_WIDTH_MIN;
    o->rest.f.max       = ANNO_LINE_WIDTH_MAX;
    o->rest.f.precision = ANNO_LINE_WIDTH_PRECISION;

    o = &ad->opt[ANNO_DISPLAY_OPTION_STROKE_WIDTH];
    o->name             = "stroke_width";
    o->shortDesc        = N_("Stroke width");
    o->longDesc         = N_("Stroke width for annotations");
    o->type             = CompOptionTypeFloat;
    o->value.f          = ANNO_STROKE_WIDTH_DEFAULT;
    o->rest.f.min       = ANNO_STROKE_WIDTH_MIN;
    o->rest.f.max       = ANNO_STROKE_WIDTH_MAX;
    o->rest.f.precision = ANNO_STROKE_WIDTH_PRECISION;
}

static CompOption *
annoGetDisplayOptions (CompPlugin  *plugin,
		       CompDisplay *display,
		       int	   *count)
{
    ANNO_DISPLAY (display);

    *count = NUM_OPTIONS (ad);
    return ad->opt;
}

static Bool
annoSetDisplayOption (CompPlugin      *plugin,
		      CompDisplay     *display,
		      char	      *name,
		      CompOptionValue *value)
{
    CompOption *o;
    int	       index;

    ANNO_DISPLAY (display);

    o = compFindOption (ad->opt, NUM_OPTIONS (ad), name, &index);
    if (!o)
	return FALSE;

    switch (index) {
    case ANNO_DISPLAY_OPTION_INITIATE:
    case ANNO_DISPLAY_OPTION_ERASE:
    case ANNO_DISPLAY_OPTION_CLEAR:
	if (setDisplayAction (display, o, value))
	    return TRUE;
	break;
    default:
	if (compSetOption (o, value))
	    return TRUE;
	break;
    }

    return FALSE;
}

static Bool
annoInitDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    AnnoDisplay *ad;

    ad = malloc (sizeof (AnnoDisplay));
    if (!ad)
	return FALSE;

    ad->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (ad->screenPrivateIndex < 0)
    {
	free (ad);
	return FALSE;
    }

    WRAP (ad, d, handleEvent, annoHandleEvent);

    annoDisplayInitOptions (ad, d->display);

    d->privates[displayPrivateIndex].ptr = ad;

    return TRUE;
}

static void
annoFiniDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    ANNO_DISPLAY (d);

    freeScreenPrivateIndex (d, ad->screenPrivateIndex);

    UNWRAP (ad, d, handleEvent);

    free (ad);
}

static Bool
annoInitScreen (CompPlugin *p,
		CompScreen *s)
{
    AnnoScreen *as;

    ANNO_DISPLAY (s->display);

    as = malloc (sizeof (AnnoScreen));
    if (!as)
	return FALSE;

    as->grabIndex = 0;
    as->surface   = NULL;
    as->pixmap    = None;
    as->cairo     = NULL;
    as->content   = FALSE;

    initTexture (s, &as->texture);

    addScreenAction (s, &ad->opt[ANNO_DISPLAY_OPTION_INITIATE].value.action);
    addScreenAction (s, &ad->opt[ANNO_DISPLAY_OPTION_ERASE].value.action);
    addScreenAction (s, &ad->opt[ANNO_DISPLAY_OPTION_CLEAR].value.action);

    WRAP (as, s, paintScreen, annoPaintScreen);

    s->privates[ad->screenPrivateIndex].ptr = as;

    return TRUE;
}

static void
annoFiniScreen (CompPlugin *p,
		CompScreen *s)
{
    ANNO_SCREEN (s);
    ANNO_DISPLAY (s->display);

    if (as->cairo)
	cairo_destroy (as->cairo);

    if (as->surface)
	cairo_surface_destroy (as->surface);

    finiTexture (s, &as->texture);

    if (as->pixmap)
	XFreePixmap (s->display->display, as->pixmap);

    removeScreenAction (s,
			&ad->opt[ANNO_DISPLAY_OPTION_INITIATE].value.action);
    removeScreenAction (s, &ad->opt[ANNO_DISPLAY_OPTION_ERASE].value.action);
    removeScreenAction (s, &ad->opt[ANNO_DISPLAY_OPTION_CLEAR].value.action);

    UNWRAP (as, s, paintScreen);

    free (as);
}

static Bool
annoInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
annoFini (CompPlugin *p)
{
    if (displayPrivateIndex >= 0)
	freeDisplayPrivateIndex (displayPrivateIndex);
}

static int
annoGetVersion (CompPlugin *plugin,
		int	   version)
{
    return ABIVERSION;
}

static CompPluginVTable annoVTable = {
    "annotate",
    N_("Annotate"),
    N_("Annotate plugin"),
    annoGetVersion,
    0, /* GetMetadata */
    annoInit,
    annoFini,
    annoInitDisplay,
    annoFiniDisplay,
    annoInitScreen,
    annoFiniScreen,
    0, /* InitWindow */
    0, /* FiniWindow */
    annoGetDisplayOptions,
    annoSetDisplayOption,
    0, /* GetScreenOptions */
    0, /* SetScreenOption */
    0, /* Deps */
    0, /* nDeps */
    0, /* Features */
    0  /* nFeatures */
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &annoVTable;
}
