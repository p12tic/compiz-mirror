/**
 *
 * Compiz group plugin
 *
 * cairo.c
 *
 * Copyright : (C) 2006-2007 by Patrick Niklaus, Roi Cohen, Danny Baumann
 * Authors: Patrick Niklaus <patrick.niklaus@googlemail.com>
 *          Roi Cohen       <roico.beryl@gmail.com>
 *          Danny Baumann   <maniac@opencompositing.org>
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

#include "group.h"

/*
 * groupRebuildCairoLayer
 *
 */
CairoLayer *
CairoLayer::rebuildCairoLayer (CairoLayer *layer,
			       int        width,
			       int        height)
{
    int        timeBuf = layer->animationTime;
    PaintState stateBuf = layer->state;

    delete layer;
    layer = CairoLayer::createCairoLayer (width, height);
    if (!layer)
	return NULL;

    layer->animationTime = timeBuf;
    layer->state = stateBuf;

    return layer;
}

/*
 * groupClearCairoLayer
 *
 */
void
CairoHelper::clear ()
{
    cairo_t *cr = cairo;

    cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);
    cairo_restore (cr);
}

/*
 * groupDestroyCairoLayer
 *
 */
CairoHelper::~CairoHelper ()
{

    if (cairo)
	cairo_destroy (cairo);

    if (surface)
	cairo_surface_destroy (surface);

    if (buffer)
	free (buffer);

    if (pixmap)
	XFreePixmap (screen->dpy (), pixmap);
}

CairoHelper::CairoHelper (int width, int height) :
    pixmap (None),
    buffer (NULL),
    surface (NULL),
    cairo (NULL),
    texWidth (width),
    texHeight (height)
{
}

CairoLayer::CairoLayer (int width, int height) :
    CairoHelper (width, height)
{
}

/*
 * groupCreateCairoLayer
 *
 */
CairoLayer *
CairoLayer::createCairoLayer (int width, int height)
{
    CairoLayer *layer = new CairoLayer (width, height);

    if (!layer)
	return NULL;

    layer->buffer = (unsigned char *)
    			    calloc (4 * width * height, sizeof (unsigned char));
    if (!layer->buffer)
    {
	compLogMessage ("group", CompLogLevelError,
			"Failed to allocate cairo layer buffer.");
	delete layer;
	return NULL;
    }

    layer->surface = cairo_image_surface_create_for_data (layer->buffer,
							  CAIRO_FORMAT_ARGB32,
							  width, height,
							  4 * width);
    if (cairo_surface_status (layer->surface) != CAIRO_STATUS_SUCCESS)
    {
	compLogMessage ("group", CompLogLevelError,
			"Failed to create cairo layer surface.");
	delete layer;
	return NULL;
    }

    layer->cairo = cairo_create (layer->surface);
    if (cairo_status (layer->cairo) != CAIRO_STATUS_SUCCESS)
    {
	compLogMessage ("group", CompLogLevelError,
			"Failed to create cairo layer context.");
	delete layer;
	return NULL;
    }

    layer->clear ();

    return layer;
}

/*
 * groupRenderTopTabHighlight
 *
 */
void
TabBar::renderTopTabHighlight ()
{
    cairo_t         *cr;
    int             width, height;
    CairoLayer      *layer;

    GROUP_SCREEN (screen);

    if (!HAS_TOP_WIN (group) ||
	!selectionLayer || !selectionLayer->cairo)
    {
	return;
    }

    width = group->topTab->region.boundingRect ().x2 () -
	    group->topTab->region.boundingRect ().x1 ();
    height = group->topTab->region.boundingRect ().y2 () -
	     group->topTab->region.boundingRect ().y1 ();

    selectionLayer = CairoLayer::rebuildCairoLayer (selectionLayer,
						         width, height);
    if (!selectionLayer)
	return;

    layer = selectionLayer;
    cr = selectionLayer->cairo;

    /* fill */
    cairo_set_line_width (cr, 2);
    cairo_set_source_rgba (cr,
			   (group->color[0] / 65535.0f),
			   (group->color[1] / 65535.0f),
			   (group->color[2] / 65535.0f),
			   (group->color[3] / (65535.0f * 2)));

    cairo_move_to (cr, 0, 0);
    cairo_rectangle (cr, 0, 0, width, height);

    cairo_fill_preserve (cr);

    /* outline */
    cairo_set_source_rgba (cr,
			   (group->color[0] / 65535.0f),
			   (group->color[1] / 65535.0f),
			   (group->color[2] / 65535.0f),
			   (group->color[3] / 65535.0f));
    cairo_stroke (cr);

    layer->texture = GLTexture::imageBufferToTexture ((char *) layer->buffer,
						      CompSize (layer->texWidth,
								layer->texHeight));
}

/*
 * groupRenderTabBarBackground
 *
 */
void
TabBar::renderTabBarBackground ()
{
    CairoLayer      *layer;
    cairo_t         *cr;
    int             width, height, radius;
    int             borderWidth;
    float           r, g, b, a;
    double          x0, y0, x1, y1;

    GROUP_SCREEN (screen);

    if (!HAS_TOP_WIN (group) || !bgLayer || !bgLayer->cairo)
	return;

    width = region.boundingRect ().x2 () - region.boundingRect ().x1 ();
    height = region.boundingRect ().y2 () - region.boundingRect ().y1 ();
    radius = gs->optionGetBorderRadius ();

    if (width > bgLayer->texWidth)
	width = bgLayer->texWidth;

    if (radius > width / 2)
	radius = width / 2;

    layer = bgLayer;
    cr = layer->cairo;

    layer->clear ();

    borderWidth = gs->optionGetBorderWidth ();
    cairo_set_line_width (cr, borderWidth);

    cairo_save (cr);

    x0 = borderWidth / 2.0f;
    y0 = borderWidth / 2.0f;
    x1 = width  - borderWidth / 2.0f;
    y1 = height - borderWidth / 2.0f;
    cairo_move_to (cr, x0 + radius, y0);
    cairo_arc (cr, x1 - radius, y0 + radius, radius, M_PI * 1.5, M_PI * 2.0);
    cairo_arc (cr, x1 - radius, y1 - radius, radius, 0.0, M_PI * 0.5);
    cairo_arc (cr, x0 + radius, y1 - radius, radius, M_PI * 0.5, M_PI);
    cairo_arc (cr, x0 + radius, y0 + radius, radius, M_PI, M_PI * 1.5);

    cairo_close_path  (cr);

    switch (gs->optionGetTabStyle ()) {
    case GroupOptions::TabStyleSimple:
	{
	    /* base color */
	    r = gs->optionGetTabBaseColorRed () / 65535.0f;
	    g = gs->optionGetTabBaseColorGreen () / 65535.0f;
	    b = gs->optionGetTabBaseColorBlue () / 65535.0f;
	    a = gs->optionGetTabBaseColorAlpha () / 65535.0f;
	    cairo_set_source_rgba (cr, r, g, b, a);

    	    cairo_fill_preserve (cr);
	    break;
	}

    case GroupOptions::TabStyleGradient:
	{
	    /* fill */
	    cairo_pattern_t *pattern;
	    pattern = cairo_pattern_create_linear (0, 0, width, height);

	    /* highlight color */
	    r = gs->optionGetTabHighlightColorRed () / 65535.0f;
	    g = gs->optionGetTabHighlightColorGreen () / 65535.0f;
	    b = gs->optionGetTabHighlightColorBlue () / 65535.0f;
	    a = gs->optionGetTabHighlightColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 0.0f, r, g, b, a);

	    /* base color */
	    r = gs->optionGetTabBaseColorRed () / 65535.0f;
	    g = gs->optionGetTabBaseColorGreen () / 65535.0f;
	    b = gs->optionGetTabBaseColorBlue () / 65535.0f;
	    a = gs->optionGetTabBaseColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 1.0f, r, g, b, a);

	    cairo_set_source (cr, pattern);
	    cairo_fill_preserve (cr);
	    cairo_pattern_destroy (pattern);
	    break;
	}

    case GroupOptions::TabStyleGlass:
	{
	    cairo_pattern_t *pattern;

	    cairo_save (cr);

	    /* clip width rounded rectangle */
	    cairo_clip (cr);

	    /* ===== HIGHLIGHT ===== */

	    /* make draw the shape for the highlight and
	       create a pattern for it */
	    cairo_rectangle (cr, 0, 0, width, height / 2);
	    pattern = cairo_pattern_create_linear (0, 0, 0, height);

	    /* highlight color */
	    r = gs->optionGetTabHighlightColorRed () / 65535.0f;
	    g = gs->optionGetTabHighlightColorGreen () / 65535.0f;
	    b = gs->optionGetTabHighlightColorBlue () / 65535.0f;
	    a = gs->optionGetTabHighlightColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 0.0f, r, g, b, a);

	    /* base color */
	    r = gs->optionGetTabBaseColorRed () / 65535.0f;
	    g = gs->optionGetTabBaseColorGreen () / 65535.0f;
	    b = gs->optionGetTabBaseColorBlue () / 65535.0f;
	    a = gs->optionGetTabBaseColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 0.6f, r, g, b, a);

	    cairo_set_source (cr, pattern);
	    cairo_fill (cr);
	    cairo_pattern_destroy (pattern);

	    /* ==== SHADOW ===== */

	    /* make draw the shape for the show and create a pattern for it */
	    cairo_rectangle (cr, 0, height / 2, width, height);
	    pattern = cairo_pattern_create_linear (0, 0, 0, height);

	    /* we don't want to use a full highlight here
	       so we mix the colors */
	    r = (gs->optionGetTabHighlightColorRed () +
		 gs->optionGetTabBaseColorRed ()) / (2 * 65535.0f);
	    g = (gs->optionGetTabHighlightColorGreen () +
		 gs->optionGetTabBaseColorGreen ()) / (2 * 65535.0f);
	    b = (gs->optionGetTabHighlightColorBlue () +
		 gs->optionGetTabBaseColorBlue ()) / (2 * 65535.0f);
	    a = (gs->optionGetTabHighlightColorAlpha () +
		 gs->optionGetTabBaseColorAlpha ()) / (2 * 65535.0f);
	    cairo_pattern_add_color_stop_rgba (pattern, 1.0f, r, g, b, a);

	    /* base color */
	    r = gs->optionGetTabBaseColorRed () / 65535.0f;
	    g = gs->optionGetTabBaseColorGreen () / 65535.0f;
	    b = gs->optionGetTabBaseColorBlue () / 65535.0f;
	    a = gs->optionGetTabBaseColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 0.5f, r, g, b, a);

	    cairo_set_source (cr, pattern);
	    cairo_fill (cr);
	    cairo_pattern_destroy (pattern);

	    cairo_restore (cr);

	    /* draw shape again for the outline */
	    cairo_move_to (cr, x0 + radius, y0);
	    cairo_arc (cr, x1 - radius, y0 + radius,
		       radius, M_PI * 1.5, M_PI * 2.0);
	    cairo_arc (cr, x1 - radius, y1 - radius,
		       radius, 0.0, M_PI * 0.5);
	    cairo_arc (cr, x0 + radius, y1 - radius,
		       radius, M_PI * 0.5, M_PI);
	    cairo_arc (cr, x0 + radius, y0 + radius,
		       radius, M_PI, M_PI * 1.5);

	    break;
	}

    case GroupOptions::TabStyleMetal:
	{
	    /* fill */
	    cairo_pattern_t *pattern;
	    pattern = cairo_pattern_create_linear (0, 0, 0, height);

	    /* base color #1 */
	    r = gs->optionGetTabBaseColorRed () / 65535.0f;
	    g = gs->optionGetTabBaseColorGreen () / 65535.0f;
	    b = gs->optionGetTabBaseColorBlue () / 65535.0f;
	    a = gs->optionGetTabBaseColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 0.0f, r, g, b, a);

	    /* highlight color */
	    r = gs->optionGetTabHighlightColorRed () / 65535.0f;
	    g = gs->optionGetTabHighlightColorGreen () / 65535.0f;
	    b = gs->optionGetTabHighlightColorBlue () / 65535.0f;
	    a = gs->optionGetTabHighlightColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 0.55f, r, g, b, a);

	    /* base color #2 */
	    r = gs->optionGetTabBaseColorRed () / 65535.0f;
	    g = gs->optionGetTabBaseColorGreen () / 65535.0f;
	    b = gs->optionGetTabBaseColorBlue () / 65535.0f;
	    a = gs->optionGetTabBaseColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 1.0f, r, g, b, a);

	    cairo_set_source (cr, pattern);
	    cairo_fill_preserve (cr);
	    cairo_pattern_destroy (pattern);
	    break;
	}

    case GroupOptions::TabStyleMurrina:
	{
	    double          ratio, transX;
	    cairo_pattern_t *pattern;

	    cairo_save (cr);

	    /* clip width rounded rectangle */
	    cairo_clip_preserve (cr);

	    /* ==== TOP ==== */

	    x0 = borderWidth / 2.0;
	    y0 = borderWidth / 2.0;
	    x1 = width  - borderWidth / 2.0;
	    y1 = height - borderWidth / 2.0;
	    radius = (y1 - y0) / 2;

	    /* setup pattern */
	    pattern = cairo_pattern_create_linear (0, 0, 0, height);

	    /* we don't want to use a full highlight here
	       so we mix the colors */
	    r = (gs->optionGetTabHighlightColorRed () +
		 gs->optionGetTabBaseColorRed ()) / (2 * 65535.0f);
	    g = (gs->optionGetTabHighlightColorGreen () +
		 gs->optionGetTabBaseColorGreen ()) / (2 * 65535.0f);
	    b = (gs->optionGetTabHighlightColorBlue () +
		 gs->optionGetTabBaseColorBlue ()) / (2 * 65535.0f);
	    a = (gs->optionGetTabHighlightColorAlpha () +
		 gs->optionGetTabBaseColorAlpha ()) / (2 * 65535.0f);
	    cairo_pattern_add_color_stop_rgba (pattern, 0.0f, r, g, b, a);

	    /* highlight color */
	    r = gs->optionGetTabHighlightColorRed () / 65535.0f;
	    g = gs->optionGetTabHighlightColorGreen () / 65535.0f;
	    b = gs->optionGetTabHighlightColorBlue () / 65535.0f;
	    a = gs->optionGetTabHighlightColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 1.0f, r, g, b, a);

	    cairo_set_source (cr, pattern);

	    cairo_fill (cr);
	    cairo_pattern_destroy (pattern);

	    /* ==== BOTTOM ===== */

	    x0 = borderWidth / 2.0;
	    y0 = borderWidth / 2.0;
	    x1 = width  - borderWidth / 2.0;
	    y1 = height - borderWidth / 2.0;
	    radius = (y1 - y0) / 2;

	    ratio = (double)width / (double)height;
	    transX = width - (width * ratio);

	    cairo_move_to (cr, x1, y1);
	    cairo_line_to (cr, x1, y0);
	    if (width < height)
	    {
		cairo_translate (cr, transX, 0);
		cairo_scale (cr, ratio, 1.0);
	    }
	    cairo_arc (cr, x1 - radius, y0, radius, 0.0, M_PI * 0.5);
	    if (width < height)
	    {
		cairo_scale (cr, 1.0 / ratio, 1.0);
		cairo_translate (cr, -transX, 0);
		cairo_scale (cr, ratio, 1.0);
	    }
	    cairo_arc_negative (cr, x0 + radius, y1,
				radius, M_PI * 1.5, M_PI);
	    cairo_close_path (cr);

	    /* setup pattern */
	    pattern = cairo_pattern_create_linear (0, 0, 0, height);

	    /* base color */
	    r = gs->optionGetTabBaseColorRed () / 65535.0f;
	    g = gs->optionGetTabBaseColorGreen () / 65535.0f;
	    b = gs->optionGetTabBaseColorBlue () / 65535.0f;
	    a = gs->optionGetTabBaseColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 0.0f, r, g, b, a);

	    /* we don't want to use a full highlight here
	       so we mix the colors */
	    r = (gs->optionGetTabHighlightColorRed () +
		 gs->optionGetTabBaseColorRed ()) / (2 * 65535.0f);
	    g = (gs->optionGetTabHighlightColorGreen () +
		 gs->optionGetTabBaseColorGreen ()) / (2 * 65535.0f);
	    b = (gs->optionGetTabHighlightColorBlue () +
		 gs->optionGetTabBaseColorBlue ()) / (2 * 65535.0f);
	    a = (gs->optionGetTabHighlightColorAlpha () +
		 gs->optionGetTabBaseColorAlpha ()) / (2 * 65535.0f);
	    cairo_pattern_add_color_stop_rgba (pattern, 1.0f, r, g, b, a);

	    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	    cairo_set_source (cr, pattern);
	    cairo_fill (cr);
	    cairo_pattern_destroy (pattern);
	    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	    cairo_restore (cr);

	    /* draw shape again for the outline */
	    x0 = borderWidth / 2.0;
	    y0 = borderWidth / 2.0;
	    x1 = width  - borderWidth / 2.0;
	    y1 = height - borderWidth / 2.0;
	    radius = gs->optionGetBorderRadius ();

	    cairo_move_to (cr, x0 + radius, y0);
	    cairo_arc (cr, x1 - radius, y0 + radius,
		       radius, M_PI * 1.5, M_PI * 2.0);
	    cairo_arc (cr, x1 - radius, y1 - radius,
		       radius, 0.0, M_PI * 0.5);
	    cairo_arc (cr, x0 + radius, y1 - radius,
		       radius, M_PI * 0.5, M_PI);
	    cairo_arc (cr, x0 + radius, y0 + radius,
		       radius, M_PI, M_PI * 1.5);

    	    break;
	}

    default:
	break;
    }

    /* outline */
    r = gs->optionGetTabBorderColorRed () / 65535.0f;
    g = gs->optionGetTabBorderColorGreen () / 65535.0f;
    b = gs->optionGetTabBorderColorBlue () / 65535.0f;
    a = gs->optionGetTabBorderColorAlpha () / 65535.0f;
    cairo_set_source_rgba (cr, r, g, b, a);

    if (bgAnimation != AnimationNone)
	cairo_stroke_preserve (cr);
    else
	cairo_stroke (cr);

    switch (bgAnimation) {
    case AnimationPulse:
	{
	    double animationProgress;
	    double alpha;

	    animationProgress = bgAnimationTime /
		                (gs->optionGetPulseTime () * 1000.0);
	    alpha = sin ((2 * PI * animationProgress) - 1.55)*0.5 + 0.5;
	    if (alpha <= 0)
		break;

	    cairo_save (cr);
	    cairo_clip (cr);
	    cairo_set_operator (cr, CAIRO_OPERATOR_XOR);
	    cairo_rectangle (cr, 0.0, 0.0, width, height);
	    cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, alpha);
	    cairo_fill (cr);
	    cairo_restore (cr);
	    break;
	}

    case AnimationReflex:
	{
	    double          animationProgress;
	    double          reflexWidth;
	    double          posX, alpha;
	    cairo_pattern_t *pattern;

	    animationProgress = bgAnimationTime /
		                (gs->optionGetReflexTime () * 1000.0);
	    reflexWidth = (tabs.size () / 2.0) * 30;
	    posX = (width + reflexWidth * 2.0) * animationProgress;
	    alpha = sin (PI * animationProgress) * 0.55;
	    if (alpha <= 0)
		break;

	    cairo_save (cr);
	    cairo_clip (cr);
	    pattern = cairo_pattern_create_linear (posX - reflexWidth,
						   0.0, posX, height);
	    cairo_pattern_add_color_stop_rgba (pattern,
					       0.0f, 1.0, 1.0, 1.0, 0.0);
	    cairo_pattern_add_color_stop_rgba (pattern,
					       0.5f, 1.0, 1.0, 1.0, alpha);
	    cairo_pattern_add_color_stop_rgba (pattern,
					       1.0f, 1.0, 1.0, 1.0, 0.0);
	    cairo_rectangle (cr, 0.0, 0.0, width, height);
	    cairo_set_source (cr, pattern);
	    cairo_fill (cr);
	    cairo_restore (cr);
	    cairo_pattern_destroy (pattern);
	    break;
	}

    case AnimationNone:
    default:
	break;
    }

    /* draw inner outline */
    cairo_move_to (cr, x0 + radius + 1.0, y0 + 1.0);
    cairo_arc (cr, x1 - radius - 1.0, y0 + radius + 1.0,
		radius, M_PI * 1.5, M_PI * 2.0);
    cairo_arc (cr, x1 - radius - 1.0, y1 - radius - 1.0,
		radius, 0.0, M_PI * 0.5);
    cairo_arc (cr, x0 + radius + 1.0, y1 - radius - 1.0,
		radius, M_PI * 0.5, M_PI);
    cairo_arc (cr, x0 + radius + 1.0, y0 + radius + 1.0,
		radius, M_PI, M_PI * 1.5);

    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.3);
    cairo_stroke(cr);

    cairo_restore (cr);
    layer->texture = GLTexture::imageBufferToTexture ((char*) layer->buffer,
			  			      CompSize (layer->texWidth,
							        layer->texHeight));
							        
    bgLayer = layer;
}

/*
 * groupRenderWindowTitle
 *
 */
void
TabBar::renderWindowTitle ()
{
    TextLayer       *layer;
    int             width, height;
    Pixmap          pixmap = None;
    
    GROUP_SCREEN (screen);

    if (!HAS_TOP_WIN (group) || !textLayer)
	return;

    width = region.boundingRect ().x2 () - region.boundingRect ().x1 ();
    height = region.boundingRect ().y2 () - region.boundingRect ().y1 ();

    delete textLayer;

    textLayer = new TextLayer ();

    layer = textLayer;
    if (!layer)
	return;

    if (textSlot && textSlot->window)
    {
	CompText::Attrib  textAttrib;

	textAttrib.family = "Sans";
	textAttrib.size   = gs->optionGetTabbarFontSize ();

	textAttrib.flags = CompText::StyleBold | CompText::Ellipsized |
	                   CompText::NoAutoBinding;

	textAttrib.color[0] = gs->optionGetTabbarFontColorRed ();
	textAttrib.color[1] = gs->optionGetTabbarFontColorGreen ();
	textAttrib.color[2] = gs->optionGetTabbarFontColorBlue ();
	textAttrib.color[3] = gs->optionGetTabbarFontColorAlpha ();

	textAttrib.maxWidth = width;
	textAttrib.maxHeight = height;

	
	if (layer->text.renderWindowTitle (textSlot->window->id (), false,
					   textAttrib))
	{
	    pixmap = layer->text.getPixmap ();
	    width = layer->text.getWidth ();
	    height = layer->text.getHeight ();
	}
    }

    if (!pixmap)
    {
	/* getting the pixmap failed, so create an empty one */
	pixmap = XCreatePixmap (screen->dpy (), screen->root (), width, height,
									    32);

	if (pixmap)
	{
	    XGCValues gcv;
	    GC        gc;

	    gcv.foreground = 0x00000000;
	    gcv.plane_mask = 0xffffffff;

	    gc = XCreateGC (screen->dpy (), pixmap, GCForeground, &gcv);
	    XFillRectangle (screen->dpy (), pixmap, gc, 0, 0, width, height);
	    XFreeGC (screen->dpy (), gc);
	}
    }

    layer->texWidth = width;
    layer->texHeight = height;

    if (pixmap)
    {
	layer->pixmap = pixmap;
	layer->texture = GLTexture::bindPixmapToTexture (layer->pixmap,
			     				 layer->texWidth,
							 layer->texHeight, 32);
    }
}

