/*
 * Compiz text plugin
 * Description: Adds text to pixmap support to Compiz.
 *
 * text.c
 *
 * Copyright: (C) 2006-2007 Patrick Niklaus, Danny Baumann, Dennis Kasprzyk
 * Authors: Patrick Niklaus <marex@opencompsiting.org>
 *	    Danny Baumann   <maniac@opencompositing.org>
 *	    Dennis Kasprzyk <onestone@opencompositing.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "private.h"

#define PI 3.14159265359f

static CompMetadata textMetadata;

COMPIZ_PLUGIN_20081216 (text, TextPluginVTable);

static char *
textGetUtf8Property (CompScreen *s,
		     Window      id,
		     Atom        atom)
{
    Atom          type;
    int           result, format;
    unsigned long nItems, bytesAfter;
    char          *val, *retval = NULL;

    TEXT_SCREEN (s);

    result = XGetWindowProperty (s->dpy (), id, atom, 0L, 65536, False,
				 ts->utf8StringAtom, &type, &format, &nItems, // AT
				 &bytesAfter, (unsigned char **) &val);

    if (result != Success)
	return NULL;

    if (type == ts->utf8StringAtom && format == 8 && val && nItems > 0)
    {
	retval = (char *) malloc (sizeof (char) * (nItems + 1));
	if (retval)
	{
	    strncpy (retval, val, nItems);
	    retval[nItems] = 0;
	}
    }

    if (val)
	XFree (val);

    return retval;
}

static char *
textGetTextProperty (CompScreen  *s,
		     Window      id,
		     Atom        atom)
{
    XTextProperty text;
    char          *retval = NULL;

    text.nitems = 0;
    if (XGetTextProperty (s->dpy (), id, &text, atom))
    {
        if (text.value)
	{
	    retval = (char *) malloc (sizeof (char) * (text.nitems + 1));
	    if (retval)
	    {
		strncpy (retval, (char *) text.value, text.nitems);
		retval[text.nitems] = 0;
	    }

	    XFree (text.value);
	}
    }

    return retval;
}

static char *
textGetWindowName (CompScreen *s,
		   Window      id)
{
    char *name;

    TEXT_SCREEN (s);

    name = textGetUtf8Property (s, id, ts->visibleNameAtom); // AT

    if (!name)
	name = textGetUtf8Property (s, id, ts->wmNameAtom); // AT

    if (!name)
	name = textGetTextProperty (s, id, XA_WM_NAME);

    return name;
}

/* Actual text rendering functions */

/*
 * Draw a rounded rectangle path
 */
static void
textDrawTextBackground (cairo_t *cr,
			int     x,
			int     y,
			int     width,
			int     height,
			int     radius)
{
    int x0, y0, x1, y1;

    x0 = x;
    y0 = y;
    x1 = x + width;
    y1 = y + height;

    cairo_new_path (cr);
    cairo_arc (cr, x0 + radius, y1 - radius, radius, PI / 2, PI);
    cairo_line_to (cr, x0, y0 + radius);
    cairo_arc (cr, x0 + radius, y0 + radius, radius, PI, 3 * PI / 2);
    cairo_line_to (cr, x1 - radius, y0);
    cairo_arc (cr, x1 - radius, y0 + radius, radius, 3 * PI / 2, 2 * PI);
    cairo_line_to (cr, x1, y1 - radius);
    cairo_arc (cr, x1 - radius, y1 - radius, radius, 0, PI / 2);
    cairo_close_path (cr);
}

static Bool
textInitCairo (CompScreen      *s,
	       TextHelper      *data,
	       int             width,
	       int             height)
{
    Display *dpy = s->dpy ();

    data->pixmap = None;
    if (width > 0 && height > 0)
	data->pixmap = XCreatePixmap (dpy, s->root (), width, height, 32);

    data->width  = width;
    data->height = height;

    if (!data->pixmap)
    {
	compLogMessage ("text", CompLogLevelError,
			"Couldn't create %d x %d pixmap.", width, height);
	return FALSE;
    }

    data->surface = cairo_xlib_surface_create_with_xrender_format (dpy,
								   data->pixmap,
								   data->screen,
								   data->format,
								   width,
								   height);
    if (cairo_surface_status (data->surface) != CAIRO_STATUS_SUCCESS)
    {
	compLogMessage ("text", CompLogLevelError, "Couldn't create surface.");
	return FALSE;
    }

    data->cr = cairo_create (data->surface);
    if (cairo_status (data->cr) != CAIRO_STATUS_SUCCESS)
    {
	compLogMessage ("text", CompLogLevelError,
			"Couldn't create cairo context.");
	return FALSE;
    }

    return TRUE;
}

static Bool
textUpdateSurface (CompScreen      *s,
		   TextHelper      *data,
		   int             width,
		   int             height)
{
    Display *dpy = s->dpy ();

    cairo_surface_destroy (data->surface);
    cairo_destroy (data->cr);
    XFreePixmap (dpy, data->pixmap);

    return textInitCairo (s, data, width, height);
}

bool
TextHelper::render (const CompText::Attrib &attrib,
		    CompString     &text)
{
    int width, height, layoutWidth;

    pango_font_description_set_family (font, attrib.family);
    pango_font_description_set_absolute_size (font,
					      attrib.size * PANGO_SCALE);
    pango_font_description_set_style (font, PANGO_STYLE_NORMAL);

    if (attrib.flags & CompTextFlagStyleBold)
	pango_font_description_set_weight (font, PANGO_WEIGHT_BOLD);

    if (attrib.flags & CompTextFlagStyleItalic)
	pango_font_description_set_style (font, PANGO_STYLE_ITALIC);

    pango_layout_set_font_description (layout, font);

    if (attrib.flags & CompTextFlagEllipsized)
	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

    pango_layout_set_auto_dir (layout, FALSE);
    pango_layout_set_text (layout, text.c_str (), -1);

    pango_layout_get_pixel_size (layout, &width, &height);

    if (attrib.flags & CompTextFlagWithBackground)
    {
	width  += 2 * attrib.bgHMargin;
	height += 2 * attrib.bgVMargin;
    }

    width  = MIN (attrib.maxWidth, width);
    height = MIN (attrib.maxHeight, height);

    /* update the size of the pango layout */
    layoutWidth = attrib.maxWidth;
    if (attrib.flags & CompTextFlagWithBackground)
	layoutWidth -= 2 * attrib.bgHMargin;

    pango_layout_set_width (layout, layoutWidth * PANGO_SCALE);

    if (!textUpdateSurface (s, this, width, height))
	return FALSE;

    pango_cairo_update_layout (cr, layout);

    cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);
    cairo_restore (cr);

    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

    if (attrib.flags & CompTextFlagWithBackground)
    {
	textDrawTextBackground (cr, 0, 0, width, height,
				MIN (attrib.bgHMargin, attrib.bgVMargin));
	cairo_set_source_rgba (cr,
			       attrib.bgColor[0] / 65535.0,
			       attrib.bgColor[1] / 65535.0,
			       attrib.bgColor[2] / 65535.0,
			       attrib.bgColor[3] / 65535.0);
	cairo_fill (cr);
	cairo_move_to (cr, attrib.bgHMargin, attrib.bgVMargin);
    }

    cairo_set_source_rgba (cr,
			   attrib.color[0] / 65535.0,
			   attrib.color[1] / 65535.0,
			   attrib.color[2] / 65535.0,
			   attrib.color[3] / 65535.0);

    pango_cairo_show_layout (cr, layout);

    return TRUE;
}

TextHelper::TextHelper (CompScreen *s) :
    width  (0),
    height (0),
    cr (NULL),
    surface (NULL),
    layout (NULL),
    pixmap (None),
    format (NULL),
    font (NULL),
    screen (NULL),
    s (s),
    failed (false)
{
    Display *dpy = s->dpy ();

    screen = ScreenOfDisplay (dpy, s->screenNum ());

    if (!screen)
    {
	compLogMessage ("text", CompLogLevelError,
			"Couldn't get screen for %d.", s->screenNum ());
	failed = true;
    }

    format = XRenderFindStandardFormat (dpy, PictStandardARGB32);
    if (!format)
    {
	compLogMessage ("text", CompLogLevelError, "Couldn't get format.");
	failed = true;
    }

    if (textInitCairo (s, this, 1, 1))
    {

	/* init pango */
	layout = pango_cairo_create_layout (cr);
	if (!layout)
	{
	    compLogMessage ("text", CompLogLevelError,
			    "Couldn't create pango layout.");
	    failed = true;
	}

    }

    font = pango_font_description_new ();
    if (!font)
    {
	compLogMessage ("text", CompLogLevelError,
			"Couldn't create font description.");
	failed = true;
    }
}

TextHelper::~TextHelper ()
{
    if (layout)
	g_object_unref (layout);
    if (surface)
	cairo_surface_destroy (surface);
    if (cr)
	cairo_destroy (cr);
    if (font)
	pango_font_description_free (font);
}

CompText
CompText::renderText (CompScreen           *screen,
		      CompString           text,
		      const CompText::Attrib &attrib)
{
    TextHelper      helper (screen);
    CompText        retval (screen);
    Bool	    success = false;

    if (helper.failed)
	return FALSE;

    if (!helper.failed &&
	helper.render (attrib, text))
    {
	if (!(attrib.flags & CompTextFlagNoAutoBinding))
	{
	    retval.texture = GLTexture::bindPixmapToTexture (helper.pixmap,
						      	      helper.width,
						      	      helper.height,
						      	      32);
	}

	retval.pixmap = helper.pixmap;
	retval.width  = helper.width;
	retval.height = helper.height;

	success = true;
    }

    if (!success && helper.pixmap)
	XFreePixmap (screen->dpy (), helper.pixmap);

    return retval;
}

CompText
CompText::renderWindowTitle (CompScreen		  *screen,
			     Window               window,
		             bool                 withViewportNumber,
		             const CompText::Attrib &attrib)
{
    CompString   text;
    CompPoint    winViewport;
    CompSize	 viewportSize;

    if (withViewportNumber)
    {
	CompString title;
	
	title = textGetWindowName (screen, window);
	if (title.c_str ())
	{
	    CompWindow *w;

	    w = screen->findWindow (window);
	    if (w)
	    {
		int viewport;

		winViewport = w->defaultViewport ();
		viewportSize = screen->vpSize ();
		viewport = winViewport.y () * viewportSize.width () + winViewport.y () + 1;
		text = compPrintf ("%s -[%d]-", title.c_str (), viewport);
	    }
	    else
	    {
		text = title;
	    }
	}
    }
    else
    {
	text = textGetWindowName (screen, window);
    }

    if (!text.c_str ())
	return NULL;

    return CompText::renderText (screen, text, attrib);;
}

void
CompText::draw (float              x,
	        float              y,
	        float              alpha)
{
    GLboolean  wasBlend;
    GLint      oldBlendSrc, oldBlendDst;
    GLTexture::Matrix m;
    float      width, height;

    glGetIntegerv (GL_BLEND_SRC, &oldBlendSrc);
    glGetIntegerv (GL_BLEND_DST, &oldBlendDst);

    wasBlend = glIsEnabled (GL_BLEND);
    if (!wasBlend)
	glEnable (GL_BLEND);

    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f (alpha, alpha, alpha, alpha);

    for (unsigned int i = 0; i < texture.size (); i++)
    {
	GLTexture *tex = texture[i];
	m      = tex->matrix ();

	tex->enable (GLTexture::Good);

	glBegin (GL_QUADS);

	glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m, 0));
	glVertex2f (x, y - height);
	glTexCoord2f (COMP_TEX_COORD_X (m, 0), COMP_TEX_COORD_Y (m, height));
	glVertex2f (x, y);
	glTexCoord2f (COMP_TEX_COORD_X (m, width), COMP_TEX_COORD_Y (m, height));
	glVertex2f (x + width, y);
	glTexCoord2f (COMP_TEX_COORD_X (m, width), COMP_TEX_COORD_Y (m, 0));
	glVertex2f (x + width, y - height);

	glEnd ();

	tex->disable ();
    }

    glColor4usv (defaultColor);

    if (!wasBlend)
	glDisable (GL_BLEND);
    glBlendFunc (oldBlendSrc, oldBlendDst);
}

CompText::CompText (CompScreen *screen) :
    width (0),
    height (0),
    pixmap (None),
    texture (NULL),
    screen (screen)
{
};

CompText::~CompText ()
{
    XFreePixmap (screen->dpy (), pixmap);
};

PrivateTextScreen::PrivateTextScreen (CompScreen *screen) :
    PrivateHandler <PrivateTextScreen, CompScreen, COMPIZ_TEXT_ABI> (screen),
    screen (screen)
{
    visibleNameAtom = XInternAtom (screen->dpy (), "_NET_WM_VISIBLE_NAME", 0);
    utf8StringAtom = XInternAtom (screen->dpy (), "UTF8_STRING", 0);
    wmNameAtom = XInternAtom (screen->dpy (), "_NET_WM_NAME", 0);
}
;
PrivateTextScreen::~PrivateTextScreen ()
{
};

bool
TextPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	 return false;

    if (!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	 return false;

    return true;
}
