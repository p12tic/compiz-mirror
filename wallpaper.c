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

/* Represents one wallpaper which may be a composition of various wallpaper
 * elements fillOnly is used to indicate that texture should be ignored and
 * quad with color fillColor should be drawn in it's place */

typedef struct _WallpaperWallpaper
{
    CompTexture texture;

    Bool fillOnly;
    float fillColor[3];
} WallpaperWallpaper;



typedef struct _WallpaperDisplay
{
    int screenPrivateIndex;
    /* _COMPIZ_WALLPAPER_SUPPORTED atom is used to indicate that
     * the wallpaper plugin or a plugin providing similar functionality is
     * active so that desktop managers can respond appropriately */
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
	/* In the case of fillOnly we never called initTexture */
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

/* We need to load the file in a pixmap so that it can be composited on to
 * other pixmaps in the later stages of the pipeline. To do this we create an
 * XImage from the pixel data returned from fileToImage and put it on to a
 * pixmap with XPutImage */
static Pixmap
wallpaperFileToPixmap(CompScreen *s, char * data, int * width, int *height)
{
    Bool status;
    int w, h;
    void * imageData;
    XImage * image;
    GC gc;
    Pixmap pixmap;
    XVisualInfo * vi;
    int stride;

    status = (*s->display->fileToImage)(s->display, 0, data, &w, &h, &stride, &imageData);
    if (!status)
	return 0;

    vi = glXGetVisualFromFBConfig(s->display->display,
				  s->glxPixmapFBConfigs[32].fbConfig);


    /* Here we return a pixmap the full size of the image and use XRender
       to scale it down to s->width by s->heigh later in the pipeline. This is
       to allow for tiling, etc, in the future */
    pixmap = XCreatePixmap(s->display->display, 
			   s->root, w, h, 32);


    gc = XCreateGC(s->display->display, pixmap, 0, 0);



    image = XCreateImage(s->display->display, vi->visual, 
			 32, ZPixmap, 0, imageData, w, h, 32,
			 0);

    XPutImage(s->display->display, pixmap, gc,
	      image, 0, 0, 0, 0, w, h);


    XFreeGC(s->display->display, gc);
    XDestroyImage(image);
    XFree(vi);

    *width = w;
    *height = h;

    return pixmap;
}

static Pixmap
wallpaperRadialGradientToPixmap(CompScreen *s, char * data, int * width, int * height)
{
    Pixmap p;
    cairo_t * cr;
    cairo_pattern_t * gradient;
    cairo_surface_t * surface;
    XRenderPictFormat * format;
    Screen * screen;

    float r1,g1,b1,r2,g2,b2,x1,y1,x2,y2,ra1,ra2;

    sscanf(data,"%f,%f,%f,%f,%f,%f|%f,%f,%f,%f,%f,%f",
	   &x1,&y1,&r1,&x2,&y2,&r2,
	   &r1,&g1,&b1,&r2,&g2,&b2);
    screen = ScreenOfDisplay(s->display->display, s->screenNum);
    format = XRenderFindStandardFormat(s->display->display,
				       PictStandardARGB32);

    p = XCreatePixmap(s->display->display, s->root, s->width, s->height, 32);
    surface = cairo_xlib_surface_create_with_xrender_format(s->display->display, p, screen,
							    format,s->width,s->height);
    cr = cairo_create(surface);

    gradient = cairo_pattern_create_radial(x1*s->width,y1*s->height,
					   ra1*((s->width+s->height)/2)
					   ,x2*s->width,y2*s->height,
					   ra2*((s->width+s->height)/2));

    cairo_pattern_add_color_stop_rgb(gradient, 0.0f,r1,g1,b1);
    cairo_pattern_add_color_stop_rgb(gradient, 1.0f,r2,g2,b2);

    cairo_set_source(cr, gradient);
    cairo_paint(cr);

    cairo_surface_destroy(surface);
    cairo_pattern_destroy(gradient);
    cairo_destroy(cr);

    *width = s->width;
    *height = s->height;
    return p;


}

static Pixmap
wallpaperLinearGradientToPixmap(CompScreen *s, char * data, int * width, int * height)
{
    Pixmap p;
    cairo_t * cr;
    cairo_pattern_t * gradient;
    cairo_surface_t * surface;
    XRenderPictFormat * format;
    Screen *screen;
    float r1,g1,b1,r2,g2,b2,x1,y1,x2,y2;

    sscanf(data,"%f,%f,%f,%f|%f,%f,%f,%f,%f,%f",&x1,&y1,&x2,&y2,&r1,&g1,&b1,&r2,&g2,&b2);

    screen = ScreenOfDisplay(s->display->display, s->screenNum);
    format = XRenderFindStandardFormat(s->display->display,
				       PictStandardARGB32);

    p = XCreatePixmap(s->display->display, s->root, s->width, s->height, 32);

    surface = cairo_xlib_surface_create_with_xrender_format(s->display->display,
							    p, screen,
							    format, s->width, s->height);
    cr = cairo_create(surface);

    gradient = cairo_pattern_create_linear(x1*s->width,y1*s->height,x2*s->width,y2*s->height);
    cairo_pattern_add_color_stop_rgb(gradient,0.0f,r1,g1,b1);
    cairo_pattern_add_color_stop_rgb(gradient,1.0f,r2,g2,b2);



    cairo_set_source(cr,gradient);
    cairo_paint(cr);

    cairo_surface_destroy(surface);
    cairo_pattern_destroy(gradient);
    cairo_destroy(cr);

    *width = s->width;
    *height = s->height;
    return p;
}

/* Returns a picture representing a fill of 'a' opacity, used to blend various
 * components together in the compositing stage */
static Picture
wallpaperAlphaMask(CompScreen *s, double a)
{
    Pixmap pixmap;
    Picture picture;
    XRenderPictureAttributes pa;
    XRenderColor c;

    /* 1x1 is fine here because we just need a pixmap to create the picture
       from, not actually one to draw to because we use CPRepeat */
    pixmap = XCreatePixmap(s->display->display, s->root,
			   1, 1, 32);

    pa.repeat = True;
    picture = XRenderCreatePicture(s->display->display, pixmap,
				   XRenderFindStandardFormat(s->display->display, PictStandardARGB32),
				   CPRepeat,
				   &pa);

    c.alpha = a* 0xffff;
    c.red = 0;
    c.green = 0;
    c.blue = 0;

    XRenderFillRectangle(s->display->display, PictOpSrc, picture, &c, 0, 0, 1, 1);
    XFreePixmap(s->display->display, pixmap);

    return picture;
}

static Pixmap
wallpaperFillToPixmap(CompScreen *s, char * data, int * width, int * height)
{
    Pixmap pixmap;
    Picture fill;
    Picture dest;
    XRenderPictFormat * format;
    XRenderColor c;
    float r,g,b;

    sscanf(data,"%f,%f,%f",&r,&g,&b);	
    c.red = r*65535;
    c.green = g*65535;
    c.blue = b*65535;
    c.alpha = 65535;

    format = XRenderFindStandardFormat(s->display->display, PictStandardARGB32);
    pixmap = XCreatePixmap(s->display->display, s->root, s->width, s->height, 32);
    dest = XRenderCreatePicture(s->display->display, pixmap, format, 0, 0);
    fill = XRenderCreateSolidFill(s->display->display, &c);

    XRenderComposite(s->display->display, PictOpSrc,
		     fill, 0, dest,
		     0, 0, 0, 0, 0, 0,
		     s->width, s->height);

    XRenderFreePicture(s->display->display,dest);
    XRenderFreePicture(s->display->display,fill);

    *width = s->width;
    *height = s->height;

    return pixmap;
}


static void
wallpaperFillFillOnly(CompScreen *s, char * data, int i)
{
    WALLPAPER_SCREEN(s);
    float r,g,b;
    sscanf(data,"%f,%f,%f",&r,&g,&b);

    ws->wallpapers[i].fillOnly = TRUE;
    ws->wallpapers[i].fillColor[0]=r;
    ws->wallpapers[i].fillColor[1]=g;
    ws->wallpapers[i].fillColor[2]=b;
}

static void
wallpaperCompositeElement(CompScreen *s, Picture destPicture, Pixmap p, float opacity, int w, int h)
{
    Picture sourcePicture, alpha;
    XRenderPictFormat * format;
    XRenderPictureAttributes attrib;

    attrib.repeat = wallpaperGetTile(s);
    format = XRenderFindStandardFormat(s->display->display, PictStandardARGB32);

    /* Projective transformation to scale source to
       s->width by s->height from w/h as set in the
       function which returned the pixmap */
    XTransform xform = {{
	{XDoubleToFixed(1/(s->width/(float)w)), XDoubleToFixed(0), XDoubleToFixed(0)},
	    {XDoubleToFixed(0), XDoubleToFixed(1/(s->height/(float)h)), XDoubleToFixed(0)},
	    {XDoubleToFixed(0), XDoubleToFixed(0), XDoubleToFixed(1)}}};



    sourcePicture = XRenderCreatePicture(s->display->display, p, format, CPRepeat, &attrib);
    alpha = wallpaperAlphaMask(s, opacity);

    if (((w > s->width) && (h > s->height)) || (attrib.repeat == 0))
    {
	XRenderSetPictureTransform(s->display->display, sourcePicture, &xform);
	XRenderSetPictureFilter(s->display->display, sourcePicture, "bilinear", 0, 0);
    }

    XRenderComposite(s->display->display, PictOpOver,
		     sourcePicture, alpha, destPicture,
		     0, 0, 0, 0, 0, 0,
		     s->width ,s->height);

    XFreePixmap(s->display->display, p);
    XRenderFreePicture(s->display->display, sourcePicture);
    XRenderFreePicture(s->display->display, alpha);	
}


static void
wallpaperLoadImages(CompScreen *s)
{
    CompListValue * images;
    WALLPAPER_SCREEN(s);
    int i;
    XRenderPictFormat * format = XRenderFindStandardFormat(s->display->display, 
							   PictStandardARGB32);


    images = wallpaperGetImages(s);
    ws->wallpapers = malloc(sizeof(WallpaperWallpaper) * images->nValue);

    for (i = 0; i < images->nValue; i++)
    {
	char * wallpaper = strdup(images->value[i].s);
	char * type,* data,* opacityc, * temp, * component;
	float opacity;
	Pixmap p = 0, finalPixmap;
	Picture destPicture;
	XImage * image;
	int w,h;

	initTexture(s, &ws->wallpapers[i].texture);

	temp = wallpaper;

	if (strstr(wallpaper,"fill_only"))
	{
	    ws->wallpapers[i].fillOnly = TRUE;

	    strsep(&wallpaper, ":");
	    data = strsep(&wallpaper, ":");
	    wallpaperFillFillOnly(s, data, i);

	    free(temp);
	    continue;
	}

	ws->wallpapers[i].fillOnly = FALSE;

	finalPixmap = XCreatePixmap(s->display->display, s->root, s->width, s->height, 32);
	destPicture = XRenderCreatePicture(s->display->display, finalPixmap, format, 0, 0);


	while ((component = strsep(&wallpaper,";"))
	       && *component != '\0')
	{

	    type = strsep(&component,":");
	    data = strsep(&component,":");
	    opacityc = strsep(&component,":");
	    opacity = atof(opacityc);

	    if (!strcmp(type,"file"))
	    {
		p = wallpaperFileToPixmap(s, data, &w, &h);
	    }
	    else if (!strcmp(type,"fill"))
	    {
		p = wallpaperFillToPixmap(s, data, &w, &h);
	    }
	    else if (!strcmp(type,"linear"))
	    {
		p = wallpaperLinearGradientToPixmap(s, data, &w, &h);
	    }
	    else if (!strcmp(type,"radial"))
	    {
		p = wallpaperRadialGradientToPixmap(s, data, &w, &h);
	    }

	    if (p)
		wallpaperCompositeElement(s, destPicture, p, opacity, w, h);

	}


	image = XGetImage(s->display->display, finalPixmap, 
			  0, 0, s->width, s->height, AllPlanes, ZPixmap);
	imageBufferToTexture(s, &ws->wallpapers[i].texture, image->data, s->width, s->height);

	XDestroyImage(image);
	XFreePixmap(s->display->display, finalPixmap);
	XRenderFreePicture(s->display->display, destPicture);

	free(temp);
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
    } else {
	glColor4f(ww->fillColor[0], ww->fillColor[1],
		  ww->fillColor[2], 1.0f);
    }


    glVertexPointer(2, GL_FLOAT, sizeof(GLfloat) * 4, data+2);

    if (!ww->fillOnly)
    {
	if (mask & PAINT_BACKGROUND_ON_TRANSFORMED_SCREEN_MASK)
	    enableTexture(s, bg, COMP_TEXTURE_FILTER_GOOD);
	else
	    enableTexture(s, bg, COMP_TEXTURE_FILTER_FAST);
    }


    glDrawArrays(GL_QUADS, 0, nBox * 4);
    glColor4usv(defaultColor);

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
    wallpaperSetTileNotify(s, wallpaperImagesChanged);


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
    0
};

CompPluginVTable * getCompPluginInfo(void)
{
    return &wallpaperVTable;
}

