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
GroupCairoLayer*
GroupScreen::groupRebuildCairoLayer (GroupCairoLayer *layer,
				     int             width,
				     int             height)
{
    int        timeBuf = layer->animationTime;
    PaintState stateBuf = layer->state;

    groupDestroyCairoLayer (layer);
    layer = groupCreateCairoLayer (width, height);
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
GroupScreen::groupClearCairoLayer (GroupCairoLayer *layer)
{
    cairo_t *cr = layer->cairo;

    cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);
    cairo_restore (cr);
}

/*
 * groupDestroyCairoLayer
 *
 */
void
GroupScreen::groupDestroyCairoLayer (GroupCairoLayer *layer)
{
    if (!layer)
	return;

    if (layer->cairo)
	cairo_destroy (layer->cairo);

    if (layer->surface)
	cairo_surface_destroy (layer->surface);

    if (layer->pixmap)
	XFreePixmap (screen->dpy (), layer->pixmap);

    if (layer->buffer)
	free (layer->buffer);

    delete layer;
}

/*
 * groupCreateCairoLayer
 *
 */
GroupCairoLayer*
GroupScreen::groupCreateCairoLayer (int        width,
				    int	       height)
{
    GroupCairoLayer *layer;


    layer = new GroupCairoLayer ();
    if (!layer)
        return NULL;

    layer->surface = NULL;
    layer->cairo   = NULL;
    layer->buffer  = NULL;
    layer->pixmap  = None;

    layer->animationTime = 0;
    layer->state         = PaintOff;

    layer->texWidth  = width;
    layer->texHeight = height;

    layer->buffer = (unsigned char *) calloc (4 * width * height, sizeof (unsigned char));
    if (!layer->buffer)
    {
	compLogMessage ("group", CompLogLevelError,
			"Failed to allocate cairo layer buffer.");
	groupDestroyCairoLayer (layer);
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
	groupDestroyCairoLayer (layer);
	return NULL;
    }

    layer->cairo = cairo_create (layer->surface);
    if (cairo_status (layer->cairo) != CAIRO_STATUS_SUCCESS)
    {
	compLogMessage ("group", CompLogLevelError,
			"Failed to create cairo layer context.");
	groupDestroyCairoLayer (layer);
	return NULL;
    }

    groupClearCairoLayer (layer);

    return layer;
}

/*
 * groupRenderTopTabHighlight
 *
 */
void
GroupScreen::groupRenderTopTabHighlight (GroupSelection *group)
{
    GroupTabBar     *bar = group->tabBar;
    GroupCairoLayer *layer;
    cairo_t         *cr;
    int             width, height;

    if (!bar || !HAS_TOP_WIN (group) ||
	!bar->selectionLayer || !bar->selectionLayer->cairo)
    {
	return;
    }

    width = group->topTab->region->extents.x2 -
	    group->topTab->region->extents.x1;
    height = group->topTab->region->extents.y2 -
	     group->topTab->region->extents.y1;

    bar->selectionLayer = groupRebuildCairoLayer (bar->selectionLayer,
						  width, height);
    if (!bar->selectionLayer)
	return;

    layer = bar->selectionLayer;
    cr = bar->selectionLayer->cairo;

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

    layer->texture = GLTexture::imageBufferToTexture ((char*) layer->buffer,
			 		  CompSize (layer->texWidth, layer->texHeight));
}

/*
 * groupRenderTabBarBackground
 *
 */
void
GroupScreen::groupRenderTabBarBackground (GroupSelection *group)
{
    GroupCairoLayer *layer;
    cairo_t         *cr;
    int             width, height, radius;
    int             borderWidth;
    float           r, g, b, a;
    double          x0, y0, x1, y1;
    GroupTabBar     *bar = group->tabBar;

    if (!bar || !HAS_TOP_WIN (group) || !bar->bgLayer || !bar->bgLayer->cairo)
	return;

    width = bar->region->extents.x2 - bar->region->extents.x1;
    height = bar->region->extents.y2 - bar->region->extents.y1;
    radius = optionGetBorderRadius ();

    if (width > bar->bgLayer->texWidth)
	width = bar->bgLayer->texWidth;

    if (radius > width / 2)
	radius = width / 2;

    layer = bar->bgLayer;
    cr = layer->cairo;

    groupClearCairoLayer (layer);

    borderWidth = optionGetBorderWidth ();
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

    switch (optionGetTabStyle ()) {
    case TabStyleSimple:
	{
	    /* base color */
	    r = optionGetTabBaseColorRed () / 65535.0f;
	    g = optionGetTabBaseColorGreen () / 65535.0f;
	    b = optionGetTabBaseColorBlue () / 65535.0f;
	    a = optionGetTabBaseColorAlpha () / 65535.0f;
	    cairo_set_source_rgba (cr, r, g, b, a);

    	    cairo_fill_preserve (cr);
	    break;
	}

    case TabStyleGradient:
	{
	    /* fill */
	    cairo_pattern_t *pattern;
	    pattern = cairo_pattern_create_linear (0, 0, width, height);

	    /* highlight color */
	    r = optionGetTabHighlightColorRed () / 65535.0f;
	    g = optionGetTabHighlightColorGreen () / 65535.0f;
	    b = optionGetTabHighlightColorBlue () / 65535.0f;
	    a = optionGetTabHighlightColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 0.0f, r, g, b, a);

	    /* base color */
	    r = optionGetTabBaseColorRed () / 65535.0f;
	    g = optionGetTabBaseColorGreen () / 65535.0f;
	    b = optionGetTabBaseColorBlue () / 65535.0f;
	    a = optionGetTabBaseColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 1.0f, r, g, b, a);

	    cairo_set_source (cr, pattern);
	    cairo_fill_preserve (cr);
	    cairo_pattern_destroy (pattern);
	    break;
	}

    case TabStyleGlass:
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
	    r = optionGetTabHighlightColorRed () / 65535.0f;
	    g = optionGetTabHighlightColorGreen () / 65535.0f;
	    b = optionGetTabHighlightColorBlue () / 65535.0f;
	    a = optionGetTabHighlightColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 0.0f, r, g, b, a);

	    /* base color */
	    r = optionGetTabBaseColorRed () / 65535.0f;
	    g = optionGetTabBaseColorGreen () / 65535.0f;
	    b = optionGetTabBaseColorBlue () / 65535.0f;
	    a = optionGetTabBaseColorAlpha () / 65535.0f;
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
	    r = (optionGetTabHighlightColorRed () +
		 optionGetTabBaseColorRed ()) / (2 * 65535.0f);
	    g = (optionGetTabHighlightColorGreen () +
		 optionGetTabBaseColorGreen ()) / (2 * 65535.0f);
	    b = (optionGetTabHighlightColorBlue () +
		 optionGetTabBaseColorBlue ()) / (2 * 65535.0f);
	    a = (optionGetTabHighlightColorAlpha () +
		 optionGetTabBaseColorAlpha ()) / (2 * 65535.0f);
	    cairo_pattern_add_color_stop_rgba (pattern, 1.0f, r, g, b, a);

	    /* base color */
	    r = optionGetTabBaseColorRed () / 65535.0f;
	    g = optionGetTabBaseColorGreen () / 65535.0f;
	    b = optionGetTabBaseColorBlue () / 65535.0f;
	    a = optionGetTabBaseColorAlpha () / 65535.0f;
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

    case TabStyleMetal:
	{
	    /* fill */
	    cairo_pattern_t *pattern;
	    pattern = cairo_pattern_create_linear (0, 0, 0, height);

	    /* base color #1 */
	    r = optionGetTabBaseColorRed () / 65535.0f;
	    g = optionGetTabBaseColorGreen () / 65535.0f;
	    b = optionGetTabBaseColorBlue () / 65535.0f;
	    a = optionGetTabBaseColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 0.0f, r, g, b, a);

	    /* highlight color */
	    r = optionGetTabHighlightColorRed () / 65535.0f;
	    g = optionGetTabHighlightColorGreen () / 65535.0f;
	    b = optionGetTabHighlightColorBlue () / 65535.0f;
	    a = optionGetTabHighlightColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 0.55f, r, g, b, a);

	    /* base color #2 */
	    r = optionGetTabBaseColorRed () / 65535.0f;
	    g = optionGetTabBaseColorGreen () / 65535.0f;
	    b = optionGetTabBaseColorBlue () / 65535.0f;
	    a = optionGetTabBaseColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 1.0f, r, g, b, a);

	    cairo_set_source (cr, pattern);
	    cairo_fill_preserve (cr);
	    cairo_pattern_destroy (pattern);
	    break;
	}

    case TabStyleMurrina:
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
	    r = (optionGetTabHighlightColorRed () +
		 optionGetTabBaseColorRed ()) / (2 * 65535.0f);
	    g = (optionGetTabHighlightColorGreen () +
		 optionGetTabBaseColorGreen ()) / (2 * 65535.0f);
	    b = (optionGetTabHighlightColorBlue () +
		 optionGetTabBaseColorBlue ()) / (2 * 65535.0f);
	    a = (optionGetTabHighlightColorAlpha () +
		 optionGetTabBaseColorAlpha ()) / (2 * 65535.0f);
	    cairo_pattern_add_color_stop_rgba (pattern, 0.0f, r, g, b, a);

	    /* highlight color */
	    r = optionGetTabHighlightColorRed () / 65535.0f;
	    g = optionGetTabHighlightColorGreen () / 65535.0f;
	    b = optionGetTabHighlightColorBlue () / 65535.0f;
	    a = optionGetTabHighlightColorAlpha () / 65535.0f;
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
	    r = optionGetTabBaseColorRed () / 65535.0f;
	    g = optionGetTabBaseColorGreen () / 65535.0f;
	    b = optionGetTabBaseColorBlue () / 65535.0f;
	    a = optionGetTabBaseColorAlpha () / 65535.0f;
	    cairo_pattern_add_color_stop_rgba (pattern, 0.0f, r, g, b, a);

	    /* we don't want to use a full highlight here
	       so we mix the colors */
	    r = (optionGetTabHighlightColorRed () +
		 optionGetTabBaseColorRed ()) / (2 * 65535.0f);
	    g = (optionGetTabHighlightColorGreen () +
		 optionGetTabBaseColorGreen ()) / (2 * 65535.0f);
	    b = (optionGetTabHighlightColorBlue () +
		 optionGetTabBaseColorBlue ()) / (2 * 65535.0f);
	    a = (optionGetTabHighlightColorAlpha () +
		 optionGetTabBaseColorAlpha ()) / (2 * 65535.0f);
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
	    radius = optionGetBorderRadius ();

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
    r = optionGetTabBorderColorRed () / 65535.0f;
    g = optionGetTabBorderColorGreen () / 65535.0f;
    b = optionGetTabBorderColorBlue () / 65535.0f;
    a = optionGetTabBorderColorAlpha () / 65535.0f;
    cairo_set_source_rgba (cr, r, g, b, a);

    if (bar->bgAnimation != AnimationNone)
	cairo_stroke_preserve (cr);
    else
	cairo_stroke (cr);

    switch (bar->bgAnimation) {
    case AnimationPulse:
	{
	    double animationProgress;
	    double alpha;

	    animationProgress = bar->bgAnimationTime /
		                (optionGetPulseTime () * 1000.0);
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

	    animationProgress = bar->bgAnimationTime /
		                (optionGetReflexTime () * 1000.0);
	    reflexWidth = (bar->nSlots / 2.0) * 30;
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
			  		  CompSize (layer->texWidth, layer->texHeight));
}

/*
 * groupRenderWindowTitle
 *
 */
void
GroupScreen::groupRenderWindowTitle (GroupSelection *group)
{
    GroupCairoLayer *layer;
    int             width, height;
    Pixmap          pixmap = None;
    GroupTabBar     *bar = group->tabBar;

    if (!bar || !HAS_TOP_WIN (group) || !bar->textLayer)
	return;

    width = bar->region->extents.x2 - bar->region->extents.x1;
    height = bar->region->extents.y2 - bar->region->extents.y1;

    bar->textLayer = groupRebuildCairoLayer (bar->textLayer, width, height);
    layer = bar->textLayer;
    if (!layer)
	return;

    if (bar->textSlot && bar->textSlot->window && textAvailable)
    {
	CompText::Attrib  textAttrib;

	textAttrib.family = "Sans";
	textAttrib.size   = optionGetTabbarFontSize ();

	textAttrib.flags = CompText::StyleBold | CompText::Ellipsized |
	                   CompText::NoAutoBinding;

	textAttrib.color[0] = optionGetTabbarFontColorRed ();
	textAttrib.color[1] = optionGetTabbarFontColorGreen ();
	textAttrib.color[2] = optionGetTabbarFontColorBlue ();
	textAttrib.color[3] = optionGetTabbarFontColorAlpha ();

	textAttrib.maxWidth = width;
	textAttrib.maxHeight = height;

	if (mText.renderWindowTitle (bar->textSlot->window->id (),
				     false, textAttrib))
	{
	    pixmap = mText.getPixmap ();
	    width = mText.getWidth ();
	    height = mText.getHeight ();
	}
    }

    if (!pixmap)
    {
	/* getting the pixmap failed, so create an empty one */
	pixmap = XCreatePixmap (screen->dpy (), screen->root (), width, height, 32);

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
	layer->texture.clear ();
	layer->pixmap = pixmap;
	layer->texture = GLTexture::bindPixmapToTexture (layer->pixmap,
							 layer->texWidth, layer->texHeight, 32);
    }
}

