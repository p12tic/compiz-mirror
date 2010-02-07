/**
 *
 * Compiz group plugin
 *
 * layers.cpp
 *
 * Copyright : (C) 2006-2009 by Patrick Niklaus, Roi Cohen, Danny Baumann,
 *				Sam Spilsbury
 * Authors: Patrick Niklaus <patrick.niklaus@googlemail.com>
 *          Roi Cohen       <roico.beryl@gmail.com>
 *          Danny Baumann   <maniac@opencompositing.org>
 *	    Sam Spilsbury   <smspillaz@gmail.com>
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

Layer::Layer (int width, int height) :
    state (PaintOff),
    animationTime (0),
    texWidth (width),
    texHeight (height)
{
}

Layer::~Layer ()
{
}

void
Layer::draw (CompRegion &box,
	     const float  &wScale,
	     const float  &hScale,
	     const GLWindowPaintAttrib &attrib,
	     const GLMatrix	       &transform,
	     const CompRegion	       &clipRegion,
	     int	  	       alpha,
	     unsigned int	       mask,
	     TabBar		       *tb)
{
    foreach (GLTexture *tex, texture)
    {
	GLTexture::Matrix matrix = tex->matrix ();
	GLTexture::MatrixList matricies;
	
	CompRect boxRect (box.boundingRect ());

	/* remove the old x1 and y1 so we have a relative value */
	
	boxRect.setGeometry ((boxRect.x1 () - tb->topTab->window->x ()) / wScale + 
					      tb->topTab->window->x (),
			     (boxRect.y1 () - tb->topTab->window->y ()) / hScale + 
			     		      tb->topTab->window->y (),
			     boxRect.x2 () - boxRect.x1 (),
			     		 boxRect.y2 () - boxRect.y1 ());

	/* now add the new x1 and y1 so we have a absolute value again,
	also we don't want to stretch the texture... */
	if (boxRect.x2 () * wScale < texWidth)
	{
	    boxRect.setX (boxRect.x1 ());
	    boxRect.setWidth (boxRect.x1 () + boxRect.x2 () - 
	    						 boxRect.x1 ());
	}
	else
	    boxRect.setWidth (texWidth);

	if (boxRect.y2 () * hScale < texHeight)
	    boxRect.setHeight (boxRect.y2 () + 
	    		     box.boundingRect ().y1 () - boxRect.x1 ());
	else
	    boxRect.setHeight (texHeight);

	matrix.x0 -= boxRect.x1 () * matrix.xx;
	matrix.y0 -= boxRect.y1 () * matrix.yy;
	
	GLWindow::get (tb->topTab->window)->geometry ().reset ();
	
	matricies.push_back (matrix);
	
	box = CompRegion (boxRect);

	GLWindow::get (tb->topTab->window)->glAddGeometry (matricies, box, 
							    clipRegion);

	if (GLWindow::get (tb->topTab->window)->geometry ().vertices)
	{
	    GLFragment::Attrib fragment (attrib);
	    GLMatrix wTransform (transform);

	    wTransform.translate (WIN_X (tb->topTab->window),
	    			  WIN_Y (tb->topTab->window), 0.0f);
	    wTransform.scale (wScale, hScale, 1.0f);
	    wTransform.translate (attrib.xTranslate / wScale
	    					       - WIN_X (tb->topTab->window),
			          attrib.yTranslate / hScale 
			          		       - WIN_Y (tb->topTab->window),
			          0.0f);

	    alpha = alpha * ((float) attrib.opacity / OPAQUE);

	    fragment.setOpacity (alpha);	
	

	    glPushMatrix ();
	    glLoadMatrixf (wTransform.getMatrix ());

	    GLWindow::get (tb->topTab->window)->glDrawTexture (tex, fragment,
				       mask |
				      PAINT_WINDOW_BLEND_MASK |
				      PAINT_WINDOW_TRANSFORMED_MASK |
				      PAINT_WINDOW_TRANSLUCENT_MASK);
				      
	    glPopMatrix ();
	}
    }
}
