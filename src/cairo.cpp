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
 * CairoLayer::rebuild
 *
 */
SelectionLayer*
SelectionLayer::rebuild (SelectionLayer *layer,
			 CompSize   size)
{
    int              timeBuf = layer->mAnimationTime;
    PaintState       stateBuf = layer->mState;
    GroupSelection   *gBuf   = layer->mGroup;

    delete layer;
    layer = SelectionLayer::create (size, gBuf);
    if (!layer)
	return NULL;

    layer->mAnimationTime = timeBuf;
    layer->mState = stateBuf;

    return layer;
}

BackgroundLayer*
BackgroundLayer::rebuild (BackgroundLayer *layer,
			  CompSize   size)
{
    int              timeBuf = layer->mAnimationTime;
    PaintState       stateBuf = layer->mState;
    GroupSelection   *gBuf = layer->mGroup;

    delete layer;
    layer = BackgroundLayer::create (size, gBuf);
    if (!layer)
	return NULL;

    layer->mAnimationTime = timeBuf;
    layer->mState = stateBuf;

    return layer;
}

/*
 * groupClearCairoLayer
 *
 */
void
CairoLayer::clear ()
{
    cairo_t *cr = mCairo;

    cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);
    cairo_restore (cr);
}

/*
 * CairoLayer::~CairoLayer ()
 *
 */
CairoLayer::~CairoLayer ()
{
    if (mCairo)
	cairo_destroy (mCairo);

    if (mSurface)
	cairo_surface_destroy (mSurface);

    if (mBuffer)
	free (mBuffer);
}

CairoLayer::CairoLayer (const CompSize &size, GroupSelection *g) :
    TextureLayer::TextureLayer (size, g)
{
    mFailed = true;
    mSurface = NULL;
    mCairo   = NULL;
    mBuffer  = NULL;

    mAnimationTime = 0;
    mState         = PaintOff;

    mBuffer = (unsigned char *) 
		   calloc (4 * width () * height (),
			   sizeof (unsigned char));
    if (mBuffer)
    {
	mSurface = cairo_image_surface_create_for_data (mBuffer,
							  CAIRO_FORMAT_ARGB32,
							  width (),
							  height (),
							  4 * width ());

	if (cairo_surface_status (mSurface) == CAIRO_STATUS_SUCCESS)
	{
	    mCairo = cairo_create (mSurface);

	    if (cairo_status (mCairo) == CAIRO_STATUS_SUCCESS)
	    {
		clear ();
		mFailed = false;
	    }
	    else
	    {
		compLogMessage ("group", CompLogLevelError,
				"Failed to create cairo layer context.");
		cairo_surface_destroy (mSurface);
		free (mBuffer);
	    }
	}
	else
	{
	    compLogMessage ("group", CompLogLevelError,
			    "Failed to create cairo layer surface");
	    free (mBuffer);
	}
    }
    else
    {
	compLogMessage ("group", CompLogLevelError,
			"Failed to allocate cairo layer buffer.");
    }
}

BackgroundLayer::BackgroundLayer (const CompSize &size,
				  GroupSelection *g) :
    CairoLayer::CairoLayer (size, g),
    mBgAnimationTime (0),
    mBgAnimation (BackgroundLayer::AnimationNone)
{
}

/*
 * groupCreateCairoLayer
 *
 */
BackgroundLayer*
BackgroundLayer::create (CompSize size, GroupSelection *g)
{
    BackgroundLayer *layer;

    layer = new BackgroundLayer (size, g);
    if (!layer || layer->mFailed)
        return NULL;

    return layer;
}

SelectionLayer*
SelectionLayer::create (CompSize size, GroupSelection *g)
{
    SelectionLayer *layer;

    layer = new SelectionLayer (size, g);
    if (!layer || layer->mFailed)
        return NULL;



    return layer;
}

/*
 * GroupTabBar::renderTopTabHighlight
 *
 */
void
SelectionLayer::render ()
{
    cairo_t         *cr;
    int             width, height;

    if (!HAS_TOP_WIN (mGroup) || !mCairo)
    {
	return;
    }

    width = mGroup->mTabBar->mTopTab->mRegion.boundingRect ().x2 () -
	    mGroup->mTabBar->mTopTab->mRegion.boundingRect ().x1 ();
    height = mGroup->mTabBar->mTopTab->mRegion.boundingRect ().y2 () -
	     mGroup->mTabBar->mTopTab->mRegion.boundingRect ().y1 ();

    cr = mCairo;

    /* fill */
    cairo_set_line_width (cr, 2);
    cairo_set_source_rgba (cr,
			   (mGroup->mColor[0] / 65535.0f),
			   (mGroup->mColor[1] / 65535.0f),
			   (mGroup->mColor[2] / 65535.0f),
			   (mGroup->mColor[3] / (65535.0f * 2)));

    cairo_move_to (cr, 0, 0);
    cairo_rectangle (cr, 0, 0, width, height);

    cairo_fill_preserve (cr);

    /* outline */
    cairo_set_source_rgba (cr,
			   (mGroup->mColor[0] / 65535.0f),
			   (mGroup->mColor[1] / 65535.0f),
			   (mGroup->mColor[2] / 65535.0f),
			   (mGroup->mColor[3] / 65535.0f));
    cairo_stroke (cr);

    mTexture = GLTexture::imageBufferToTexture ((char*) mBuffer,
			 		  (CompSize &) *this);
    
    
}

/*
 * GroupTabBar::renderTabBarBackground
 *
 */
void
BackgroundLayer::render ()
{
    cairo_t         *cr;
    int             twidth, theight, radius;
    int             borderWidth;
    float           r, g, b, a;
    double          x0, y0, x1, y1;
    
    GROUP_SCREEN (screen);

    if (!HAS_TOP_WIN (mGroup) || !mCairo)
	return;

    twidth = mGroup->mTabBar->mRegion.boundingRect ().width ();
    theight = mGroup->mTabBar->mRegion.boundingRect ().height ();
    radius = gs->optionGetBorderRadius ();

    if (twidth > width ())
	twidth = width ();

    if (radius > twidth / 2)
	radius = twidth / 2;

    cr = mCairo;

    clear ();

    borderWidth = gs->optionGetBorderWidth ();
    cairo_set_line_width (cr, borderWidth);

    cairo_save (cr);

    x0 = borderWidth / 2.0f;
    y0 = borderWidth / 2.0f;
    x1 = twidth  - borderWidth / 2.0f;
    y1 = theight - borderWidth / 2.0f;
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
	    pattern = cairo_pattern_create_linear (0, 0, twidth, theight);

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
	    cairo_rectangle (cr, 0, 0, twidth, theight / 2);
	    pattern = cairo_pattern_create_linear (0, 0, 0, theight);

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
	    cairo_rectangle (cr, 0, theight / 2, twidth, theight);
	    pattern = cairo_pattern_create_linear (0, 0, 0, theight);

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
	    pattern = cairo_pattern_create_linear (0, 0, 0, theight);

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
	    x1 = twidth  - borderWidth / 2.0;
	    y1 = theight - borderWidth / 2.0;
	    radius = (y1 - y0) / 2;

	    /* setup pattern */
	    pattern = cairo_pattern_create_linear (0, 0, 0, theight);

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
	    x1 = twidth  - borderWidth / 2.0;
	    y1 = theight - borderWidth / 2.0;
	    radius = (y1 - y0) / 2;

	    ratio = (double)twidth / (double)theight;
	    transX = twidth - (twidth * ratio);

	    cairo_move_to (cr, x1, y1);
	    cairo_line_to (cr, x1, y0);
	    if (twidth < theight)
	    {
		cairo_translate (cr, transX, 0);
		cairo_scale (cr, ratio, 1.0);
	    }
	    cairo_arc (cr, x1 - radius, y0, radius, 0.0, M_PI * 0.5);
	    if (twidth < theight)
	    {
		cairo_scale (cr, 1.0 / ratio, 1.0);
		cairo_translate (cr, -transX, 0);
		cairo_scale (cr, ratio, 1.0);
	    }
	    cairo_arc_negative (cr, x0 + radius, y1,
				radius, M_PI * 1.5, M_PI);
	    cairo_close_path (cr);

	    /* setup pattern */
	    pattern = cairo_pattern_create_linear (0, 0, 0, theight);

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
	    x1 = twidth  - borderWidth / 2.0;
	    y1 = theight - borderWidth / 2.0;
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

    if (mBgAnimation != AnimationNone)
	cairo_stroke_preserve (cr);
    else
	cairo_stroke (cr);

    switch (mBgAnimation) {
    case AnimationPulse:
	{
	    double animationProgress;
	    double alpha;

	    animationProgress = mBgAnimationTime /
		                (gs->optionGetPulseTime () * 1000.0);
	    alpha = sin ((2 * PI * animationProgress) - 1.55)*0.5 + 0.5;
	    if (alpha <= 0)
		break;

	    cairo_save (cr);
	    cairo_clip (cr);
	    cairo_set_operator (cr, CAIRO_OPERATOR_XOR);
	    cairo_rectangle (cr, 0.0, 0.0, twidth, theight);
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

	    animationProgress = mBgAnimationTime /
		                (gs->optionGetReflexTime () * 1000.0);
	    reflexWidth = (mGroup->mTabBar->mSlots.size () / 2.0) * 30;
	    posX = (twidth + reflexWidth * 2.0) * animationProgress;
	    alpha = sin (PI * animationProgress) * 0.55;
	    if (alpha <= 0)
		break;

	    cairo_save (cr);
	    cairo_clip (cr);
	    pattern = cairo_pattern_create_linear (posX - reflexWidth,
						   0.0, posX, theight);
	    cairo_pattern_add_color_stop_rgba (pattern,
					       0.0f, 1.0, 1.0, 1.0, 0.0);
	    cairo_pattern_add_color_stop_rgba (pattern,
					       0.5f, 1.0, 1.0, 1.0, alpha);
	    cairo_pattern_add_color_stop_rgba (pattern,
					       1.0f, 1.0, 1.0, 1.0, 0.0);
	    cairo_rectangle (cr, 0.0, 0.0, twidth, theight);
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

    mTexture = GLTexture::imageBufferToTexture ((char*) mBuffer,
			  		  (CompSize &) *this);
}

TextLayer *
TextLayer::rebuild (TextLayer *layer)
{
    /* general cleanup func ... for now */
    if (layer)
    {
	if (layer->mPixmap)
	    XFreePixmap (screen->dpy (), layer->mPixmap);

	PaintState     pStateBuf = layer->mState;
	int 	       aTimeBuf = layer->mAnimationTime;
	CompSize       sBuf = (CompSize ) *layer; 
	GroupSelection *gBuf = layer->mGroup;
	
	delete layer;
	layer = new TextLayer (sBuf, gBuf);
	if (!layer)
	    return NULL;
	
	layer->mState = pStateBuf;
	layer->mAnimationTime = aTimeBuf;
    }
    
    return layer;
}

/*
 * TextLayer::render
 *
 */
void
TextLayer::render ()
{
    int             twidth, theight;
    Pixmap          pixmap = None;
    
    GROUP_SCREEN (screen);

    if (!HAS_TOP_WIN (mGroup))
	return;

    twidth = mGroup->mTabBar->mRegion.boundingRect ().width ();
    theight = mGroup->mTabBar->mRegion.boundingRect ().height ();

    if (mGroup->mTabBar->mTextSlot &&
        mGroup->mTabBar->mTextSlot->mWindow && textAvailable)
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

	textAttrib.maxWidth = twidth;
	textAttrib.maxHeight = theight;

	if (gs->mText.renderWindowTitle (
		             mGroup->mTabBar->mTextSlot->mWindow->id (),
						     false, textAttrib))
	{
	    pixmap = gs->mText.getPixmap ();
	    twidth = gs->mText.getWidth ();
	    theight = gs->mText.getHeight ();
	}
    }

    if (!pixmap)
    {
	/* getting the pixmap failed, so create an empty one */
	pixmap = XCreatePixmap (screen->dpy (), screen->root (), twidth, theight, 32);

	if (pixmap)
	{
	    XGCValues gcv;
	    GC        gc;

	    gcv.foreground = 0x00000000;
	    gcv.plane_mask = 0xffffffff;

	    gc = XCreateGC (screen->dpy (), pixmap, GCForeground, &gcv);
	    XFillRectangle (screen->dpy (), pixmap, gc, 0, 0, twidth, theight);
	    XFreeGC (screen->dpy (), gc);
	}
    }

    setWidth  (twidth);
    setHeight (theight);

    if (pixmap)
    {
	mTexture.clear ();
	mPixmap = pixmap;
	mTexture = GLTexture::bindPixmapToTexture (mPixmap,
						   width (), height (), 32);
    }
}

