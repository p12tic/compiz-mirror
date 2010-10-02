/**
 *
 * Compiz group plugin
 *
 * group.h
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
 
#ifndef _GROUP_LAYERS_H
#define _GROUP_LAYERS_H

#include "group.h"

typedef enum {
    PaintOff = 0,
    PaintFadeIn,
    PaintFadeOut,
    PaintOn,
    PaintPermanentOn
} PaintState;

class Layer :
    public CompSize
{
    public:
	Layer (CompSize &size, GroupSelection *g) :
	    CompSize::CompSize (size),
	    mGroup (g) {}
    public:
	virtual void damage () {};

	GroupSelection  *mGroup;
	PaintState      mState;
	int             mAnimationTime;
};

class GLLayer :
    public Layer
{
    public:
	GLLayer (CompSize &size, GroupSelection *g) :
	    Layer::Layer (size, g) {}

    public:

	virtual void paint (const GLWindowPaintAttrib &attrib,
			    const GLMatrix	      &transform,
			    const CompRegion	      &paintRegion,
			    const CompRegion	      &clipRegion,
			    int			      mask) = 0;
	
};

class TextureLayer :
    public GLLayer
{
    public:
	TextureLayer (CompSize &size, GroupSelection *g) :
	    GLLayer::GLLayer (size, g) {}

    public:

	void setPaintWindow (CompWindow *);
	virtual void paint (const GLWindowPaintAttrib &attrib,
			    const GLMatrix	      &transform,
			    const CompRegion	      &paintRegion,
			    const CompRegion	      &clipRegion,
			    int			      mask);
    public:

	GLTexture::List mTexture;
	CompWindow	*mPaintWindow; /* the window we are going to
					* paint with geometry */
	
};

class CairoLayer :
    public TextureLayer
{
    public:

	~CairoLayer ();

    public:
	
	void clear ();
	virtual void render () = 0;
	virtual void paint (const GLWindowPaintAttrib &attrib,
			    const GLMatrix	      &transform,
			    const CompRegion	      &paintRegion,
			    const CompRegion	      &clipRegion,
			    int			      mask) = 0;

    public:

	/* used if layer is used for cairo drawing */
	unsigned char   *mBuffer;
	cairo_surface_t *mSurface;
	cairo_t	        *mCairo;
	bool	        mFailed;

    protected:
	CairoLayer (CompSize &size, GroupSelection *group);
};

class BackgroundLayer :
    public CairoLayer
{
    public:

	static BackgroundLayer * create (CompSize, GroupSelection *);
	static BackgroundLayer * rebuild (BackgroundLayer *,
				     CompSize);

	void render ();
	void paint (const GLWindowPaintAttrib &attrib,
		    const GLMatrix	      &transform,
		    const CompRegion	      &paintRegion,
		    const CompRegion	      &clipRegion,
		    int			      mask);

    private:
	BackgroundLayer (CompSize &size, GroupSelection *group) :
	    CairoLayer::CairoLayer (size, group) {}
};

class SelectionLayer :
    public CairoLayer
{
    public:

	static SelectionLayer * create (CompSize, GroupSelection *);
	static SelectionLayer * rebuild (SelectionLayer *,
					 CompSize);

	void render ();
	void paint (const GLWindowPaintAttrib &attrib,
		    const GLMatrix	      &transform,
		    const CompRegion	      &paintRegion,
		    const CompRegion	      &clipRegion,
		    int			      mask);

    private:
	SelectionLayer (CompSize &size, GroupSelection *group) :
	    CairoLayer::CairoLayer (size, group) {}
};

class TextLayer :
    public TextureLayer
{
    public:
    
	static TextLayer *
	rebuild (TextLayer *);

	void paint (const GLWindowPaintAttrib &attrib,
		    const GLMatrix	      &transform,
		    const CompRegion	      &paintRegion,
		    const CompRegion	      &clipRegion,
		    int			      mask);
	
	void render ();

    public:

	TextLayer (CompSize size, GroupSelection *g) :
	    TextureLayer::TextureLayer (size, g),
	    mPixmap (None) {}
    public:

	/* used if layer is used for text drawing */
	Pixmap mPixmap;
};

#endif
