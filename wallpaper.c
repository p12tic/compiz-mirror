/**
 *
 * Compiz wallpaper plugin
 *
 * wallpaper.c
 *
 * Copyright (c) 2007 Robert Carr <racarr@opencompositing.org>
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


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <compiz.h>

#include <X11/Xatom.h>
#include <cairo-xlib-xrender.h>

#include "wallpaper_options.h"

static int displayPrivateIndex;

typedef struct _WallpaperWallpaper
{
  CompTexture texture;
  
  Bool fillOnly;
  float fillColor[3];
  
  float opacity;
  
} WallpaperWallpaper;

  

typedef struct _WallpaperDisplay
{
	int screenPrivateIndex;
	Atom compizWallpaperAtom;
} WallpaperDisplay;

typedef struct _WallpaperScreen
{
	PaintBackgroundProc paintBackground;
  
	WallpaperWallpaper * wallpapers;
	int nWallpapers;

	Bool propSet; /* Avoid having to ask server */
} WallpaperScreen;

#define GET_WALLPAPER_DISPLAY(d)					\
	((WallpaperDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define WALLPAPER_DISPLAY(d)					\
	WallpaperDisplay * wd = GET_WALLPAPER_DISPLAY(d)

#define GET_WALLPAPER_SCREEN(s, wd)					\
	((WallpaperScreen *) (s)->privates[(wd)->screenPrivateIndex].ptr)

#define WALLPAPER_SCREEN(s)						\
	WallpaperScreen * ws = GET_WALLPAPER_SCREEN(s, GET_WALLPAPER_DISPLAY(s->display))

/* fini and remove all textures on the WallpaperScreen */
static void
cleanupBackgrounds (CompScreen *s)
{
	WALLPAPER_SCREEN(s);
	int i;
  
	for ( i = 0; i < ws->nWallpapers; i++)
	{
		if (!ws->wallpapers[i].fillOnly)
			finiTexture(s, &ws->wallpapers[i].texture);
	}
	free(ws->wallpapers);
	ws->wallpapers = 0;
	ws->nWallpapers = 0;
}

/* Installed as a handler for the images setting changing through bcop */
static void wallpaperImagesChanged(CompScreen *s,
				   CompOption *o,
				   WallpaperScreenOptions num)
{
	cleanupBackgrounds(s);
}

static Bool updateWallpaperProperty(CompScreen *s)
{
	WALLPAPER_SCREEN(s);
  
	if (!ws->nWallpapers)
	{
		WALLPAPER_DISPLAY(s->display);
	  
		if (!ws->propSet)
			XDeleteProperty(s->display->display, 
					s->root, wd->compizWallpaperAtom);
		ws->propSet = FALSE;

		return 0;
	}
	else if (!ws->propSet)
	{
		WALLPAPER_DISPLAY(s->display);
		unsigned char sd = 1;
	  
		XChangeProperty(s->display->display, s->root, 
				wd->compizWallpaperAtom, XA_CARDINAL, 	
				8, PropModeReplace, &sd, 1);      
		ws->propSet = TRUE;
	}    
	return 1;
}

static void
wallpaperLoadImages(CompScreen *s)
{
	CompListValue * images;
	WALLPAPER_SCREEN(s);
	int i;
	unsigned int w,h;
	
	images = wallpaperGetImages(s);
	ws->wallpapers = malloc(sizeof(WallpaperWallpaper) * images->nValue);
	
	for (i = 0; i < images->nValue; i++)
	{
		char * component = images->value[i].s;
		char * type,* data,* opacity;
		
		ws->wallpapers[i].fillOnly = FALSE;

		type = strsep(&component,":");
		data = strsep(&component,":");
		opacity = strsep(&component,";");
		
		sscanf(opacity,"%f",&ws->wallpapers[i].opacity);
		
		if (!strcmp(type,"file"))
		{
			initTexture(s, &ws->wallpapers[i].texture);
			readImageToTexture(s, &ws->wallpapers[i].texture,data, &w, &h);
		}
		else if (!strcmp(type,"fill"))
		{
			float r,g,b;
			sscanf(data,"%f,%f,%f",&r,&g,&b);
			
			ws->wallpapers[i].fillOnly = TRUE;
			ws->wallpapers[i].fillColor[0]=r;
			ws->wallpapers[i].fillColor[1]=g;
			ws->wallpapers[i].fillColor[2]=b;
		}
		else if (!strcmp(type,"linear"))
		{
			Pixmap p;
			cairo_t * cr;
			cairo_pattern_t * gradient;
			cairo_surface_t * surface;
			XImage * image;
			XRenderPictFormat * format;
			CompTexture * texture = &ws->wallpapers[i].texture;
			Screen *screen;
			float r1,g1,b1,r2,g2,b2,x1,y1,x2,y2;
			
			sscanf(data,"%f,%f,%f,%f|%f,%f,%f,%f,%f,%f",&x1,&y1,&x2,&y2,&r1,&g1,&b1,&r2,&g2,&b2);
			
			initTexture(s, texture);
			
			screen = ScreenOfDisplay(s->display->display, s->screenNum);
			format = XRenderFindStandardFormat(s->display->display,
							   PictStandardARGB32);
			
			p = XCreatePixmap(s->display->display, s->root, s->width, s->height, 32);
			
			surface = cairo_xlib_surface_create_with_xrender_format(s->display->display,
										p, screen,
										format, s->width, s->height);
			cr = cairo_create(surface);
			
			gradient = cairo_pattern_create_linear(x1,y1,x2*s->width,y2*s->height);
			cairo_pattern_add_color_stop_rgb(gradient,0.0f,r1,g1,b1);
			cairo_pattern_add_color_stop_rgb(gradient,1.0f,r2,g2,b2);

			
	
			cairo_set_source(cr,gradient);
			cairo_paint(cr);
			cairo_fill(cr);
			
			image = XGetImage(s->display->display, p, 0, 0, s->width, s->height, AllPlanes, ZPixmap);
			imageBufferToTexture(s, texture, image->data, s->width, s->height);
			
			XFreePixmap(s->display->display, p);
			
			cairo_surface_destroy(surface);
			cairo_pattern_destroy(gradient);
			cairo_destroy(cr);
			if (image)
			  XDestroyImage(image);
			
		}
		

	}
	ws->nWallpapers = images->nValue;
}



/* Return a texture for viewport x/y if there are no textures, load them */
static WallpaperWallpaper *
getTextureForViewport(CompScreen *s, int x, int y)
{
	WALLPAPER_SCREEN(s);
  
	if (!ws->wallpapers)
	{  
		wallpaperLoadImages(s);
	}

	if (!updateWallpaperProperty(s))
		return 0;
    
	return &ws->wallpapers[(x+y*s->hsize) % ws->nWallpapers];
  
    
}

static void
wallpaperPaintBackground (CompScreen *s,
			  Region region,
			  unsigned int mask)
{

	WALLPAPER_SCREEN(s);
	BoxPtr pBox = region->rects;
	int n, nBox = region->numRects;
	GLfloat *d, *data;
	CompTexture * bg;
	WallpaperWallpaper * ww;
	
 
	ww = getTextureForViewport(s, s->x, s->y);
	bg = &ww->texture;
	
	if (!nBox)
		return;

	if (!bg)
	{
		UNWRAP (ws, s, paintBackground);
		(*s->paintBackground)(s, region, mask);
		WRAP (ws, s, paintBackground, wallpaperPaintBackground);
		return;
	}
  
	data = malloc(sizeof(GLfloat)*nBox*16);

	d = data;
	n = nBox;
	while (n--)
	{
		*d++ = COMP_TEX_COORD_X(&bg->matrix, pBox->x1);
		*d++ = COMP_TEX_COORD_Y(&bg->matrix, pBox->y2);
      
		*d++ = pBox->x1;
		*d++ = pBox->y2;
      
		*d++ = COMP_TEX_COORD_X(&bg->matrix, pBox->x2);
		*d++ = COMP_TEX_COORD_Y(&bg->matrix, pBox->y2);
      
		*d++ = pBox->x2;
		*d++ = pBox->y2;

		*d++ = COMP_TEX_COORD_X(&bg->matrix, pBox->x2);
		*d++ = COMP_TEX_COORD_Y(&bg->matrix, pBox->y1);
      
		*d++ = pBox->x2;
		*d++ = pBox->y1;

		*d++ = COMP_TEX_COORD_X(&bg->matrix, pBox->x1);
		*d++ = COMP_TEX_COORD_Y(&bg->matrix, pBox->y1);

		*d++ = pBox->x1;
		*d++ = pBox->y1;
      
		pBox++;
	}
	if (!ww->fillOnly)
	{
		glTexCoordPointer(2, GL_FLOAT, sizeof(GLfloat) * 4, data);
		glColor4f(1.0f,1.0f,1.0f,ww->opacity);
	} else {
		glColor4f(ww->fillColor[0], ww->fillColor[1],
			  ww->fillColor[2], ww->opacity);
	}
	
	
	glVertexPointer(2, GL_FLOAT, sizeof(GLfloat) * 4, data+2);
	
	if (!ww->fillOnly)
	{
		if (mask & PAINT_BACKGROUND_ON_TRANSFORMED_SCREEN_MASK)
			enableTexture(s, bg, COMP_TEXTURE_FILTER_GOOD);
		else
			enableTexture(s, bg, COMP_TEXTURE_FILTER_FAST);
	}
	
	if (ww->opacity != 1.0f)
		glEnable(GL_BLEND);
	
	glDrawArrays(GL_QUADS, 0, nBox * 4);
	
	glDisable(GL_BLEND);
  
	if (!ww->fillOnly)
		disableTexture(s, bg);
  
}


static Bool
wallpaperInitDisplay (CompPlugin * p,
		      CompDisplay *d)
{
	WallpaperDisplay * wd;
	wd = malloc(sizeof(WallpaperDisplay));
	if (!wd)
		return FALSE;
  
	wd->screenPrivateIndex = allocateScreenPrivateIndex(d);
	if (wd->screenPrivateIndex < 0)
	{
		free (wd);
		return FALSE;
	}

	wd->compizWallpaperAtom = XInternAtom(d->display, 
					      "_COMPIZ_WALLPAPER_SUPPORTED", 0);
  
	d->privates[displayPrivateIndex].ptr = wd;
  
	return TRUE;
  
}

static void wallpaperFiniDisplay (CompPlugin * p,
				  CompDisplay *d)
{
	WALLPAPER_DISPLAY(d);
  
	freeScreenPrivateIndex(d, wd->screenPrivateIndex);
	free(wd);
}

static Bool wallpaperInitScreen (CompPlugin * p,
				 CompScreen *s)
{
	WallpaperScreen * ws;
	WALLPAPER_DISPLAY(s->display);
	unsigned char sd = 1;
  
	ws = malloc(sizeof(WallpaperScreen));
	if (!ws)
		return FALSE;
  
	ws->wallpapers = 0;
	ws->nWallpapers = 0;

	wallpaperSetImagesNotify(s, wallpaperImagesChanged);
  
    
	WRAP (ws, s, paintBackground, wallpaperPaintBackground);


	XChangeProperty(s->display->display, s->root, 
			wd->compizWallpaperAtom, XA_CARDINAL,
			8, PropModeReplace, &sd, 1);
  
	ws->propSet = TRUE;
  
  
	s->privates[wd->screenPrivateIndex].ptr = ws;
 
  
	return TRUE;
  
}

static void wallpaperFiniScreen (CompPlugin * p,
				 CompScreen *s)
{
	WALLPAPER_SCREEN(s);
	WALLPAPER_DISPLAY(s->display);
    
	XDeleteProperty(s->display->display, s->root, wd->compizWallpaperAtom);
  
	cleanupBackgrounds(s);
  
	UNWRAP(ws, s, paintBackground);
  
	free(ws);
}

static Bool
wallpaperInit(CompPlugin *p)
{
	displayPrivateIndex = allocateDisplayPrivateIndex();
	if (displayPrivateIndex < 0)
		return FALSE;
	return TRUE;
}

static void
wallpaperFini (CompPlugin *p)
{
	freeDisplayPrivateIndex(displayPrivateIndex);
}

static int
wallpaperGetVersion(CompPlugin * p,
		    int version)
{
	return ABIVERSION;
}

static CompPluginVTable  wallpaperVTable=
{
	"wallpaper",
	wallpaperGetVersion,
	0,
	wallpaperInit,
	wallpaperFini,
	wallpaperInitDisplay,
	wallpaperFiniDisplay,
	wallpaperInitScreen,
	wallpaperFiniScreen,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};

CompPluginVTable * getCompPluginInfo(void)
{
	return &wallpaperVTable;
}

