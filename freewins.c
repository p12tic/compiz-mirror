/*
 * Compiz Fusion Freewins plugin
 *
 * Copyright (C) 2007  Rodolfo Granata <warlock.cc@gmail.com>
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
 * Author(s): 
 * Rodolfo Granata <warlock.cc@gmail.com>
 *
 * Button binding support and Reset added by:
 * enigma_0Z <enigma.0ZA@gmail.com>
 *
 * Scaling, Animation, Input-Shaping, Snapping
 * and Key-Based Transformation added by:
 * Sam Spilsbury <smspillaz@gmail.com>
 *
 * Description:
 *
 * This plugin allows you to freely transform the texture of a window,
 * whether that be rotation or scaling to make better use of screen space
 * or just as a toy.
 *
 * Todo: 
 *  - Shape input on rotation
 *  - Show scaling-corner axes
 *  - Modifier key to rotate on the Z Axis
 *  - Correct damage and choppyness handling
 *  - Make help colors configurable
 *  - Simplify code
 */

#include <compiz-core.h>
#include <math.h>

#include <stdio.h>

#include <X11/cursorfont.h>
#include <X11/extensions/shape.h>

#include "freewins_options.h"

#define ABS(x) ((x)>0?(x):-(x))
#define D2R(x) ((x) * M_PI / 180.0)

// Macros/*{{{*/
#define GET_FREEWINS_DISPLAY(d)                                       \
    ((FWDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define FREEWINS_DISPLAY(d)                      \
    FWDisplay *fwd = GET_FREEWINS_DISPLAY (d)

#define GET_FREEWINS_SCREEN(s, fwd)                                        \
    ((FWScreen *) (s)->privates[(fwd)->screenPrivateIndex].ptr)

#define FREEWINS_SCREEN(s)                                                      \
    FWScreen *fws = GET_FREEWINS_SCREEN (s, GET_FREEWINS_DISPLAY (s->display))

#define GET_FREEWINS_WINDOW(w, fws)                                        \
    ((FWWindow *) (w)->privates[(fws)->windowPrivateIndex].ptr)

#define FREEWINS_WINDOW(w)                                         \
    FWWindow *fww = GET_FREEWINS_WINDOW  (w,                    \
                       GET_FREEWINS_SCREEN  (w->screen,            \
                       GET_FREEWINS_DISPLAY (w->screen->display)))


/*
#define WIN_REAL_X(w) (w->attrib.x - w->output.left)
#define WIN_REAL_Y(w) (w->attrib.y - w->output.top)

#define WIN_REAL_W(w) (w->width + w->output.left + w->output.right)
#define WIN_REAL_H(w) (w->height + w->output.top + w->output.bottom)
*/
#define WIN_REAL_X(w) (w->attrib.x - w->input.left)
#define WIN_REAL_Y(w) (w->attrib.y - w->input.top)

#define WIN_REAL_W(w) (w->width + w->input.left + w->input.right)
#define WIN_REAL_H(w) (w->height + w->input.top + w->input.bottom)

/*}}}*/
/* Enums */
typedef enum _StartCorner {
    CornerTopLeft = 0,
    CornerTopRight = 1,
    CornerBottomLeft = 2,
    CornerBottomRight = 3
} StartCorner;

// Structures /*{{{*/
typedef struct _FWDisplay{
    int screenPrivateIndex;

    int click_root_x;
    int click_root_y;

    int click_win_x;
    int click_win_y;

    HandleEventProc handleEvent;

    CompWindow *grabWindow;
    CompWindow *focusWindow;
    
    Bool axisHelp;

} FWDisplay;

typedef struct _FWScreen{
    int windowPrivateIndex;

    PaintOutputProc paintOutput;
    PaintWindowProc paintWindow;

    DamageWindowRectProc damageWindowRect;

    WindowResizeNotifyProc windowResizeNotify;
    
    Cursor rotateCursor;

    int grabIndex;
    int rotatedWindows;

} FWScreen;

typedef struct _FWWindow{
    float angX;
    float angY;
    float angZ;
    
    float scaleY;
    float scaleX;

    float midX;
    float midY;
    
    // For animation
    float aTimeRemaining; // Actual time remaining (is decremented)
    float cTimeRemaining; // Constant time remaining (is referenced and not decremented)
    
    // Old values to animate from
    float oldAngX;
    float oldAngY;
    float oldAngZ;
    
    float oldScaleX;
    float oldScaleY;
    
    // New values to animate to
    float destAngX;
    float destAngY;
    float destAngZ;
    
    float destScaleX;
    float destScaleY;
    
    // Unsnapped values. These are not painted but always remain true
    
    float unsnapAngX;
    float unsnapAngY;
    float unsnapAngZ;

    float unsnapScaleX;
    float unsnapScaleY;

    // Used for determining direction
    int oldX;
    int oldY;
    
    // Used to determine starting point
    StartCorner corner;

    //Box rect;

    Bool grabbed;
    
    // Used to determine whether to animate the window
    Bool doAnimate;
    Bool resetting;
    
    // Used to determine whether rotating on X and Y axis, or just on Z
    Bool zaxis;

    Bool grabLeft;
    Bool grabTop;

    Bool rotated;
    Bool allowScaling;
    Bool allowRotation;

} FWWindow;
/*}}}*/

int displayPrivateIndex;
static CompMetadata freewinsMetadata;

static Bool FWCanShape (CompWindow *w)
{

    if (!freewinsGetShapeInput (w->screen))
    return FALSE;
    
    if (!w->screen->display->shapeExtension)
    return FALSE;
    
    if (!matchEval (freewinsGetShapeWindowTypes (w->screen), w))
    return FALSE;
    
    return TRUE;
}

/* Input Shaper */
static void FWShapeInput (CompWindow *w)
{
    FREEWINS_WINDOW(w);
    XRectangle Rectangle;
    float ScaleX;
    float ScaleY;
    
    // Check if we are scaling uniformly
    if (freewinsGetScaleUniform (w->screen))
    {
        ScaleX = ((fww->scaleX + fww->scaleY) / 2);
        ScaleY = ((fww->scaleX + fww->scaleX) / 2);
    }
    else
    {
        ScaleX = fww->scaleX;
        ScaleY = fww->scaleY;
    }
    Rectangle.x = (int)(0 + ((1 - ScaleX) / 2) * w->width);
    Rectangle.y = (int)(0 + ((1 - ScaleY) / 2) * w->height);
    Rectangle.width = (int)(w->serverWidth * ((ScaleX)));
    Rectangle.height = (int)(w->serverHeight * ((ScaleY)));
    
    XShapeSelectInput (w->screen->display->display, w->id, NoEventMask);
    XShapeCombineRectangles  (w->screen->display->display, w->id, 
			      ShapeInput, 0, 0, &Rectangle, 1,  ShapeSet, 0);
	// Shape the window frame too
	if (w->frame)
	{
	    XShapeCombineRectangles  (w->screen->display->display, w->frame, 
			      ShapeInput, 0, 0, &Rectangle, 1,  ShapeSet, 0);
	    XShapeSelectInput (w->screen->display->display, w->id, ShapeNotify);
	}
}

// Event handler/*{{{*/
static void FWHandleEvent(CompDisplay *d, XEvent *ev){

    CompScreen *s;
    float dx, dy;
    FREEWINS_DISPLAY(d);

    switch(ev->type){
	
	// Motion Notify/*{{{*/
	case MotionNotify:
	    
	    if(fwd->grabWindow){
		FREEWINS_WINDOW(fwd->grabWindow);
		FREEWINS_SCREEN(fwd->grabWindow->screen);

		dx = (float)(ev->xmotion.x_root - fww->oldX) / fwd->grabWindow->screen->width;
		dy = (float)(ev->xmotion.y_root - fww->oldY) / fwd->grabWindow->screen->height;

        if (fww->allowRotation)
        {        
        if(fww->zaxis){
		    if(ABS(dy)>ABS(dx))
		    {
			if(fww->grabLeft)
			{
			    fww->angZ -= 360 * (dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			    fww->unsnapAngZ -= 360 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			}
			else
			{
			    fww->angZ += 360 * (dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			    fww->unsnapAngZ -= 360 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			}
		    }
		    else
		    {
			if(fww->grabTop)
			{
			    fww->angZ += 360 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			    fww->unsnapAngZ -= 360 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			}
			else
		    {
			    fww->angZ -= 360 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			    fww->unsnapAngZ -= 360 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			}
		    }
		}
		else
		{
		    fww->angX -= 360.0 * (dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		    fww->unsnapAngX -= 360.0 * (dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		    fww->angY += 360.0 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		    fww->unsnapAngY += 360.0 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		}
		}
		if (fww->allowScaling)
		{
		
		    switch (fww->corner)
		    {
		    
		    case CornerTopLeft:
		    		        
		            // Check X Direction
		            if ((ev->xmotion.x - 100.0) < fww->oldX)
		            {
		                fww->scaleX -= dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleX -= (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		            }
		            else if ((ev->xmotion.x - 100.0) > fww->oldX)
		            {
		                fww->scaleX -= dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleX -= (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		            }
		            
		            // Check Y Direction
		            if ((ev->xmotion.y - 100.0) < fww->oldY)
		            {
		                fww->scaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		            }
		            else if ((ev->xmotion.y - 100.0) > fww->oldY)
		            {
		                fww->scaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		            }
		    break;            
		    		            
            case CornerTopRight:
             
		            // Check X Direction
		            if ((ev->xmotion.x - 100.0) < fww->oldX)
		            {
		                fww->scaleX += dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleX += (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		            }
		            else if ((ev->xmotion.x - 100.0) > fww->oldX)
		            {
		                fww->scaleX += dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleX += (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		            }
		            
		            
		            // Check Y Direction
		            if ((ev->xmotion.y - 100.0) < fww->oldY)
		            {
		                fww->scaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		            }
		            else if ((ev->xmotion.y - 100.0) > fww->oldY)
		            {
		                fww->scaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		            }
		                
		     break;
		            
		     case CornerBottomLeft:
		        
		            // Check X Direction
		            if ((ev->xmotion.x - 100.0) < fww->oldX)
		            {
		                fww->scaleX -= dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleX -= (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		            }
		            else if ((ev->xmotion.x - 100.0) > fww->oldX)
		            {
		                fww->scaleX -= dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleX -= (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		            }
		            
		            // Check Y Direction
		            if ((ev->xmotion.y - 100.0) < fww->oldY)
		            {
		                fww->scaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		            }
		            else if ((ev->xmotion.y - 100.0) > fww->oldY)
		            {
		                fww->scaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		            }
		                
		            break;
		            
		     case CornerBottomRight:
		        
		            // Check X Direction
		            if ((ev->xmotion.x - 100.0) < fww->oldX)
		            {
		                fww->scaleX += dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleX += (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		            }
		            else if ((ev->xmotion.x - 100.0) > fww->oldX)
		            {
		                fww->scaleX += dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleX += (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		            }
		            
		            // Check Y Direction
		            if ((ev->xmotion.y - 100.0) < fww->oldY)
		            {
		                fww->scaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		            }
		            else if ((ev->xmotion.y - 100.0) > fww->oldY)
		            {
		                fww->scaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                fww->unsnapScaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		            }
		                
		            break;
		     }
		      
		}
		
		// Handle Snapping
		if (freewinsGetSnap (fwd->grabWindow->screen))
		{
		    int snapFactor = freewinsGetSnapThreshold (fwd->grabWindow->screen);
            fww->angX = ((int) (fww->unsnapAngX) / snapFactor) * snapFactor;
            fww->angY = ((int) (fww->unsnapAngY) / snapFactor) * snapFactor;
            fww->angZ = ((int) (fww->unsnapAngZ) / snapFactor) * snapFactor;
            fww->scaleX = ((float) ( (int) (fww->unsnapScaleX * (21 - snapFactor) + 0.5))) / (21 - snapFactor); 
            fww->scaleY = ((float) ( (int) (fww->unsnapScaleY * (21 - snapFactor) + 0.5))) / (21 - snapFactor);
        }
		
		fww->oldAngX = fww->angX;
		fww->oldAngY = fww->angY;
		fww->oldAngZ = fww->angZ;
		
		fww->oldScaleX = fww->scaleX;
		fww->oldScaleY = fww->scaleY;
		    
		fww->oldX = ev->xmotion.x_root;
		fww->oldY = ev->xmotion.y_root;

		fww->grabLeft = (ev->xmotion.x - fww->midX > 0 ? FALSE : TRUE);
		fww->grabTop = (ev->xmotion.y - fww->midY > 0 ? FALSE : TRUE);

		//fww->rect.x1 = 0; 
		//fww->rect.y1 = 0; 

		//fww->rect.x2 = fwd->grabWindow->screen->width; 
		//fww->rect.y2 = fwd->grabWindow->screen->height; 

		if(dx != 0.0 || dy != 0.0)
		    damageScreen(fwd->grabWindow->screen);
		    //damageScreenRegion(fwd->grabWindow->screen, fww->rect);

		// Check if there are rotated windows
		if(fww->angX != 0.0 || fww->angY != 0.0 || fww->angZ != 0.0 || fww->scaleX != 1.0 || fww->scaleY != 1.0){
		    if( !fww->rotated ){
			fws->rotatedWindows++;
			fww->rotated = TRUE;
		    }
		}else{
		    if( fww->rotated ){
			fws->rotatedWindows--;
			fww->rotated = FALSE;
		    }
		}
		
	    }
	    break;


	// Button press / release/*{{{*/
	case ButtonPress:
	    fwd->click_root_x = ev->xbutton.x_root;
	    fwd->click_root_y = ev->xbutton.y_root;
	    fwd->click_win_x = ev->xbutton.x;
	    fwd->click_win_y = ev->xbutton.y;

	    break;

	case ButtonRelease:
	    if(fwd->grabWindow){
	    if (FWCanShape (fwd->grabWindow))
	        FWShapeInput (fwd->grabWindow);
		for(s = d->screens; s; s = s->next){
		    FREEWINS_SCREEN(s);

		    if(fws->grabIndex){
			removeScreenGrab(s, fws->grabIndex, 0);
			fws->grabIndex = 0;
		    }
		}

		FREEWINS_WINDOW(fwd->grabWindow);
		fww->grabbed = FALSE;
		fwd->grabWindow = 0;
	    }
	    break;

	case FocusOut:
	    break;

	case FocusIn:
	    if(ev->xfocus.mode != NotifyGrab)
		fwd->focusWindow = findWindowAtDisplay(d, ev->xfocus.window);
	    break;

    default:
	    if (ev->type == d->shapeEvent + ShapeNotify)
	    {
	        XShapeEvent *se = (XShapeEvent *) ev;
	        if (se->kind == ShapeInput)
	        {
		        CompWindow *w;
		        w = findWindowAtDisplay (d, se->window);
		        if (w)
		        {
		            FREEWINS_WINDOW (w);

		            if (FWCanShape (w) && (fww->scaleX != 1.0f || fww->scaleY != 1.0f))
		            {
		                // Reset the window back to normal, scale-wise anyways
		                fww->scaleX = 1.0f;
		                fww->scaleY = 1.0f;
			            FWShapeInput (w);
			        }
		        }
	        }
	    }
    }

    UNWRAP(fwd, d, handleEvent);
    (*d->handleEvent)(d, ev);
    WRAP(fwd, d, handleEvent, FWHandleEvent);
}

// Paint Window/*{{{*/
static Bool FWPaintWindow(CompWindow *w, const WindowPaintAttrib *attrib, 
	const CompTransform *transform, Region region, unsigned int mask){

    CompTransform wTransform = *transform;
    Bool wasCulled = glIsEnabled(GL_CULL_FACE);
    Bool status;
    float ScaleX;
    float ScaleY;
        
    FREEWINS_SCREEN(w->screen);
    FREEWINS_WINDOW(w);
    
    float timeRemaining = fww->cTimeRemaining;
    
    if((fww->angX != 0.0 || fww->angY != 0.0 || fww->angZ != 0.0 ||
                         fww->scaleX != 1.0 || fww->scaleY != 1.0) && !(w->type == CompWindowTypeDesktopMask))
    {


	mask |= PAINT_WINDOW_TRANSFORMED_MASK;

	// Ajustar espesor de la ventana
	matrixScale (&wTransform, 1.0f, 1.0f, 1.0f / w->screen->width);

	matrixTranslate(&wTransform, 
		WIN_REAL_X(w) + WIN_REAL_W(w)/2.0, 
		WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0, 0.0);


    matrixRotate(&wTransform, fww->angX, 1.0, 0.0, 0.0);
    matrixRotate(&wTransform, fww->angY, 0.0, 1.0, 0.0);
    matrixRotate(&wTransform, fww->angZ, 0.0, 0.0, 1.0);
    
    if (freewinsGetScaleUniform (w->screen))
    {
        ScaleX = ((fww->scaleX + fww->scaleY) / 2);
        ScaleY = ((fww->scaleX + fww->scaleY) / 2);
    }
    else
    {
        ScaleX = fww->scaleX;
        ScaleY = fww->scaleY;
    }
    if (!freewinsGetAllowNegative (w->screen) || (FWCanShape (w)))
	{
	    float minScale = freewinsGetMinScale (w->screen);
		if (ScaleX < minScale)
		{
		  matrixScale(&wTransform, minScale, 1.0, 0.0);
		  fww->scaleX = minScale; // Don't allow negative values either, which could be referenced
		}
		else
		  matrixScale(&wTransform, ScaleX, 1.0, 0.0);
		
		if (ScaleY < minScale)
	    {
		  matrixScale(&wTransform, 1.0, minScale, 0.0);
		  fww->scaleY = minScale;
		}
		else
		  matrixScale(&wTransform, 1.0, ScaleY, 0.0);
	}
	else
	{
	    matrixScale(&wTransform, ScaleX, ScaleY, 0.0);
	}

	matrixTranslate(&wTransform, 
		-(WIN_REAL_X(w) + WIN_REAL_W(w)/2.0), 
		-(WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0), 0.0);
    }

    if (fww->doAnimate)
    {
        float raX = (fww->destAngX - fww->oldAngX) / timeRemaining;
        float raY = (fww->destAngY - fww->oldAngY) / timeRemaining;
        float raZ = (fww->destAngZ - fww->oldAngZ) / timeRemaining;
        
        float saX = ((fww->destScaleX - fww->oldScaleX) / timeRemaining);
        float saY = ((fww->destScaleX - fww->oldScaleY) / timeRemaining);
        
        fww->angX += raX;
        fww->angY += raY;
        fww->angZ += raZ;
        
        fww->scaleX += saX;
        fww->scaleY += saY;
                
        fww->aTimeRemaining--;
        damageScreen (w->screen); // FIXME: Find a better way to fix jitteryness
        
        if (fww->aTimeRemaining <= 0 || (fww->angX == fww->destAngX && fww->angY == fww->destAngY
                                        && fww->angZ == fww->destAngZ && fww->scaleX == fww->destScaleX
                                        && fww->scaleY == fww->destScaleY))
        {
            fww->resetting = FALSE;
            
            fww->doAnimate = FALSE;
            fww->aTimeRemaining = freewinsGetResetTime (w->screen);
            fww->cTimeRemaining = freewinsGetResetTime (w->screen);
            if (FWCanShape (w))
                FWShapeInput (w);
        }
    }

    if(wasCulled)
	glDisable(GL_CULL_FACE);
	

    UNWRAP(fws, w->screen, paintWindow);
    status = (*w->screen->paintWindow)(w, attrib, &wTransform, region, mask);
    WRAP(fws, w->screen, paintWindow, FWPaintWindow);

    if(wasCulled)
	glEnable(GL_CULL_FACE);

    return status;
}
/*}}}*/

//Paint Output/*{{{*/
static Bool FWPaintOutput(CompScreen *s, const ScreenPaintAttrib *sAttrib, 
	const CompTransform *transform, Region region, CompOutput *output, unsigned int mask){

    Bool wasCulled, status;
    CompTransform zTransform;
    float x, y;
    int j;

    FREEWINS_SCREEN(s);
    FREEWINS_DISPLAY(s->display);

    if(fws->rotatedWindows > 0)
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    UNWRAP(fws, s, paintOutput);
    status = (*s->paintOutput)(s, sAttrib, transform, region, output, mask);
    WRAP(fws, s, paintOutput, FWPaintOutput);

    // z-axis circle/*{{{*/
    if(fwd->axisHelp && fwd->focusWindow){
    	
	x = WIN_REAL_X(fwd->focusWindow) + WIN_REAL_W(fwd->focusWindow)/2.0;
	y = WIN_REAL_Y(fwd->focusWindow) + WIN_REAL_H(fwd->focusWindow)/2.0;

	wasCulled = glIsEnabled(GL_CULL_FACE);
	zTransform = *transform;

	transformToScreenSpace(s, output, -DEFAULT_Z_CAMERA, &zTransform);

	glPushMatrix();
	glLoadMatrixf(zTransform.m);

	if(wasCulled)
	    glDisable(GL_CULL_FACE);

	glColor4usv  (freewinsGetCircleColor (s->display));
	glEnable(GL_BLEND);

	glBegin(GL_POLYGON);
	for(j=0; j<360; j += 10)
	    glVertex3f( x + 100 * cos(D2R(j)), y + 100 * sin(D2R(j)), 0.0 );
	glEnd ();

	glDisable(GL_BLEND);
	glColor4usv  (freewinsGetLineColor (s->display));
	glLineWidth(3.0);

	glBegin(GL_LINE_LOOP);
	for(j=360; j>=0; j -= 10)
	    glVertex3f( x + 100 * cos(D2R(j)), y + 100 * sin(D2R(j)), 0.0 );
	glEnd ();
	
	glColor4usv  (freewinsGetCrossLineColor (s->display));
	glBegin(GL_LINES);
	glVertex3f(x, y - (WIN_REAL_H (fwd->focusWindow) / 2), 0.0f);
	glVertex3f(x, y + (WIN_REAL_H (fwd->focusWindow) / 2), 0.0f);
	glEnd ();
	
	glBegin(GL_LINES);
	glVertex3f(x - (WIN_REAL_W (fwd->focusWindow) / 2), y, 0.0f);
	glVertex3f(x + (WIN_REAL_W (fwd->focusWindow) / 2), y, 0.0f);
	glEnd ();
	
	if(wasCulled)
	    glEnable(GL_CULL_FACE);

	glColor4usv(defaultColor);
	glPopMatrix ();
    }
    
/*}}}*/

    return status;
}

// Damage Window Rect/*{{{*/
static Bool FWDamageWindowRect(CompWindow *w, Bool initial, BoxPtr rect){

    Bool status = TRUE;
    FREEWINS_SCREEN(w->screen);
//    FREEWINS_WINDOW(w);

    damageScreen(w->screen);

    UNWRAP(fws, w->screen, damageWindowRect);
    status |= (*w->screen->damageWindowRect)(w, initial, rect);
    //(*w->screen->damageWindowRect)(w, initial, &fww->rect);
    WRAP(fws, w->screen, damageWindowRect, FWDamageWindowRect);

    // true if damaged something
    return status;
}
/*}}}*/


// Resize Notify/*{{{*/
static void FWWindowResizeNotify(CompWindow *w, int dx, int dy, int dw, int dh){

    FREEWINS_WINDOW(w);
    FREEWINS_SCREEN(w->screen);

    fww->midX += dw;
    fww->midY += dh;
    
    if (FWCanShape (w))
        FWShapeInput (w);

    UNWRAP(fws, w->screen, windowResizeNotify);
    (*w->screen->windowResizeNotify)(w, dx, dy, dw, dh);
    WRAP(fws, w->screen, windowResizeNotify, FWWindowResizeNotify);
}
/*}}}*/

// Initiate Rotate/*{{{*/
static Bool initiateFWRotate (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    CompWindow* w;
    CompScreen* s;
    Window xid;
    float dx, dy;
    
    FREEWINS_DISPLAY(d);

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    
    for(s = d->screens; s; s = s->next){
	FREEWINS_SCREEN(s);
	
	fws->rotateCursor = XCreateFontCursor (s->display->display, XC_fleur);	

	if(!otherScreenGrabExist(s, "freewins", 0))
	    if(!fws->grabIndex)
		fws->grabIndex = pushScreenGrab(s, fws->rotateCursor, "freewins");
    }
    
    
    if(w){
	FREEWINS_WINDOW(w);
	
	fww->allowRotation = TRUE;
	fww->allowScaling = FALSE;
	
	fwd->grabWindow = w;
	
	fww->grabbed = TRUE;
	
	fww->oldX = fwd->click_root_x;
	fww->oldY = fwd->click_root_y;

	dx = fwd->click_win_x - fww->midX;
	dy = fwd->click_win_y - fww->midY;

	fww->grabLeft = (dx > 0 ? FALSE : TRUE);
	fww->grabTop = (dy > 0 ? FALSE : TRUE);

	dx = ABS(dx);
	dy = ABS(dy);
	
	if (!freewinsGetAlwaysZ (w->screen)) // Check the option
	{
	    if( (dx>dy?dx:dy) > 100 ) // Check the position
	        {
	            fww->zaxis = TRUE;
	        }
	    else
	        {
	            fww->zaxis = FALSE;
	        }
	}
	else // It's always Z Axis rotation
	{
	    fww->zaxis = FALSE;
	}
	}
	
    return TRUE;
}

// Initiate Scale/*{{{*/
static Bool initiateFWScale (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    CompWindow* w;
    CompScreen* s;
    Window xid;
    float dx, dy;
    
    FREEWINS_DISPLAY(d);

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    
    for(s = d->screens; s; s = s->next){
	FREEWINS_SCREEN(s);
	
	fws->rotateCursor = XCreateFontCursor (s->display->display, XC_plus);	

	if(!otherScreenGrabExist(s, "freewins", 0))
	    if(!fws->grabIndex)
		fws->grabIndex = pushScreenGrab(s, fws->rotateCursor, "freewins");
    }
    
    
    if(w){
	FREEWINS_WINDOW(w);
	
	fww->allowScaling = TRUE;
	fww->allowRotation = FALSE;
	
	fwd->grabWindow = w;
	
	/* Find out the corner we clicked in */
	
	/* Check for Y axis clicking (Top / Bottom) */
	if (fwd->click_win_y > fww->midY)
	{
	    /* Check for X axis clicking (Left / Right) */
	    if (fwd->click_win_x > fww->midX)
	        fww->corner = CornerBottomRight;
	    else if (fwd->click_win_x < fww->midX)
	        fww->corner = CornerBottomLeft;
	}
	else if (fwd->click_win_y < fww->midY)
	{
	    /* Check for X axis clicking (Left / Right) */
	    if (fwd->click_win_x > fww->midX)
	        fww->corner = CornerTopRight;
	    else if (fwd->click_win_x < fww->midX)
	        fww->corner = CornerTopLeft;
	}
	
	fww->grabbed = TRUE;

	fww->oldX = fwd->click_root_x;
	fww->oldY = fwd->click_root_y;

	dx = fwd->click_win_x - fww->midX;
	dy = fwd->click_win_y - fww->midY;

	fww->grabLeft = (dx > 0 ? FALSE : TRUE);
	fww->grabTop = (dy > 0 ? FALSE : TRUE);

	dx = ABS(dx);
	dy = ABS(dy);
	
	fww->zaxis = TRUE;	
	
	}
    
    return TRUE;
}

static Bool FWRotateUp (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    CompWindow* w;
    Window xid;
   
    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    
    FREEWINS_WINDOW(w);
    
    fww->oldAngX = fww->angX;
    fww->oldAngY = fww->angY;
    fww->oldAngZ = fww->angZ;
    
    fww->oldScaleX = fww->scaleX;
    fww->oldScaleY = fww->scaleY;
    
    fww->destAngX = fww->angX + freewinsGetRotateIncrementAmount (w->screen);
    fww->destAngY = fww->angY;
    fww->destAngZ = fww->angZ;
    
    fww->destScaleX = fww->scaleX;
    fww->destScaleY = fww->scaleY;
    
    fww->aTimeRemaining = freewinsGetRotateIncrementTime (w->screen);
    fww->cTimeRemaining = freewinsGetRotateIncrementTime (w->screen);
    
    fww->doAnimate = TRUE; // Start animating
    
    return TRUE;
    
}

static Bool FWRotateDown (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    CompWindow* w;
    Window xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    
    FREEWINS_WINDOW(w);
    
    fww->oldAngX = fww->angX;
    fww->oldAngY = fww->angY;
    fww->oldAngZ = fww->angZ;
    
    fww->oldScaleX = fww->scaleX;
    fww->oldScaleY = fww->scaleY;
    
    fww->destAngX = fww->angX - freewinsGetRotateIncrementAmount (w->screen);
    fww->destAngY = fww->angY;
    fww->destAngZ = fww->angZ;
    
    fww->destScaleX = fww->scaleX;
    fww->destScaleY = fww->scaleY;
    
    fww->aTimeRemaining = freewinsGetRotateIncrementTime (w->screen);
    fww->cTimeRemaining = freewinsGetRotateIncrementTime (w->screen);
    
    fww->doAnimate = TRUE; // Start animating
    
    return TRUE;
    
}

static Bool FWRotateLeft (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    CompWindow* w;
    Window xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    
    FREEWINS_WINDOW(w);
    
    fww->oldAngX = fww->angX;
    fww->oldAngY = fww->angY;
    fww->oldAngZ = fww->angZ;
    
    fww->oldScaleX = fww->scaleX;
    fww->oldScaleY = fww->scaleY;
    
    fww->destAngX = fww->angX;
    fww->destAngY = fww->angY - freewinsGetRotateIncrementAmount (w->screen);
    fww->destAngZ = fww->angZ;
    
    fww->destScaleX = fww->scaleX;
    fww->destScaleY = fww->scaleY;
    
    fww->aTimeRemaining = freewinsGetRotateIncrementTime (w->screen);
    fww->cTimeRemaining = freewinsGetRotateIncrementTime (w->screen);
    
    fww->doAnimate = TRUE; // Start animating
    
    return TRUE;
    
}

static Bool FWRotateRight (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    CompWindow* w;
    Window xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    
    FREEWINS_WINDOW(w);
    
    fww->oldAngX = fww->angX;
    fww->oldAngY = fww->angY;
    fww->oldAngZ = fww->angZ;
    
    fww->oldScaleX = fww->scaleX;
    fww->oldScaleY = fww->scaleY;
    
    fww->destAngX = fww->angX;
    fww->destAngY = fww->angY + freewinsGetRotateIncrementAmount (w->screen);
    fww->destAngZ = fww->angZ;
    
    fww->destScaleX = fww->scaleX;
    fww->destScaleY = fww->scaleY;
    
    fww->aTimeRemaining = freewinsGetRotateIncrementTime (w->screen);
    fww->cTimeRemaining = freewinsGetRotateIncrementTime (w->screen);
    
    fww->doAnimate = TRUE; // Start animating
    
    return TRUE;
    
}

static Bool FWRotateClockwise (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    CompWindow* w;
    Window xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    
    FREEWINS_WINDOW(w);
    
    fww->oldAngX = fww->angX;
    fww->oldAngY = fww->angY;
    fww->oldAngZ = fww->angZ;
    
    fww->oldScaleX = fww->scaleX;
    fww->oldScaleY = fww->scaleY;
    
    fww->destAngX = fww->angX;
    fww->destAngY = fww->angY;
    fww->destAngZ = fww->angZ + freewinsGetRotateIncrementAmount (w->screen);
    
    fww->destScaleX = fww->scaleX;
    fww->destScaleY = fww->scaleY;
    
    fww->aTimeRemaining = freewinsGetRotateIncrementTime (w->screen);
    fww->cTimeRemaining = freewinsGetRotateIncrementTime (w->screen);
    
    fww->doAnimate = TRUE; // Start animating
    
    return TRUE;
    
}

static Bool FWRotateCounterclockwise (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    CompWindow* w;
    Window xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    
    FREEWINS_WINDOW(w);
    
    fww->oldAngX = fww->angX;
    fww->oldAngY = fww->angY;
    fww->oldAngZ = fww->angZ;
    
    fww->oldScaleX = fww->scaleX;
    fww->oldScaleY = fww->scaleY;
    
    fww->destAngX = fww->angX;
    fww->destAngY = fww->angY;
    fww->destAngZ = fww->angZ - freewinsGetRotateIncrementAmount (w->screen);

    fww->destScaleX = fww->scaleX;
    fww->destScaleY = fww->scaleY;
    
    fww->aTimeRemaining = freewinsGetRotateIncrementTime (w->screen);
    fww->cTimeRemaining = freewinsGetRotateIncrementTime (w->screen);
    
    fww->doAnimate = TRUE; // Start animating
    
    return TRUE;
    
}

static Bool FWScaleUp (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    CompWindow* w;
    Window xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    
    FREEWINS_WINDOW(w);
    
    fww->oldAngX = fww->angX;
    fww->oldAngY = fww->angY;
    fww->oldAngZ = fww->angZ;
    
    fww->oldScaleX = fww->scaleX;
    fww->oldScaleY = fww->scaleY;
    
    fww->destAngX = fww->angX;
    fww->destAngY = fww->angY;
    fww->destAngZ = fww->angZ;
    
    fww->destScaleX = fww->scaleX + freewinsGetScaleIncrementAmount (w->screen);
    fww->destScaleY = fww->scaleY + freewinsGetScaleIncrementAmount (w->screen);
    
    fww->aTimeRemaining = freewinsGetScaleIncrementTime (w->screen);
    fww->cTimeRemaining = freewinsGetScaleIncrementTime (w->screen);
    
    fww->doAnimate = TRUE; // Start animating
    
    damageScreen (w->screen); // Smoothen Painting
    
    if (FWCanShape (w))
        FWShapeInput (w);
    
    return TRUE;
    
}

static Bool FWScaleDown (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    CompWindow* w;
    Window xid;

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    
    FREEWINS_WINDOW(w);
    
    fww->oldAngX = fww->angX;
    fww->oldAngY = fww->angY;
    fww->oldAngZ = fww->angZ;
    
    fww->oldScaleX = fww->scaleX;
    fww->oldScaleY = fww->scaleY;
    
    fww->destAngX = fww->angX;
    fww->destAngY = fww->angY;
    fww->destAngZ = fww->angZ;
    
    fww->destScaleX = fww->scaleX - freewinsGetScaleIncrementAmount (w->screen);
    fww->destScaleY = fww->scaleY - freewinsGetScaleIncrementAmount (w->screen);
    
    fww->aTimeRemaining = freewinsGetScaleIncrementTime (w->screen);
    fww->cTimeRemaining = freewinsGetScaleIncrementTime (w->screen);
    
    fww->doAnimate = TRUE; // Start animating
    
    if (FWCanShape (w))
        FWShapeInput (w);
    
    damageScreen (w->screen); // Smoothen Painting
    
    return TRUE;
    
}
/*}}}*/

/* Callable action to rotate a window to the angle provided*/
static Bool freewinsRotateWindow (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption){
	CompWindow *w;

    //FREEWINS_DISPLAY(d);
    
    w = findWindowAtDisplay (d, getIntOptionNamed(option, nOption, "window", 0));
    
    if (w)
    {
        FREEWINS_WINDOW(w);
        
        float x, y, z;
        
        y = getFloatOptionNamed(option, nOption, "x", 0.0f);
        x = getFloatOptionNamed(option, nOption, "y", 0.0f);
        z = getFloatOptionNamed(option, nOption, "z", 0.0f);
        
        fww->angX = x;
        fww->angY = y;
        fww->angZ = z;
        
    }
    else
    {
        return FALSE;
    }
    
    damageScreen(w->screen);

    //fwd->axisHelp = !fwd->axisHelp;

    return TRUE;
}

/* Callable action to rotate a window to the angle provided*/
static Bool freewinsIncrementRotateWindow (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption){
	CompWindow *w;

    //FREEWINS_DISPLAY(d);
    
    w = findWindowAtDisplay (d, getIntOptionNamed(option, nOption, "window", 0));
    
    if (w)
    {
        FREEWINS_WINDOW(w);
        
        float x, y, z;
        
        x = getFloatOptionNamed(option, nOption, "x", 0.0f);
        y = getFloatOptionNamed(option, nOption, "y", 0.0f);
        z = getFloatOptionNamed(option, nOption, "z", 0.0f);
        
        /* Respect dx, dy, dz, first */
        fww->angX += x;
        fww->angY += y;
        fww->angZ += z;
        
    }
    else
    {
        return FALSE;
    }
    
    damageScreen(w->screen);

    //fwd->axisHelp = !fwd->axisHelp;

    return TRUE;
}

/* Callable action to scale a window to the scale provided*/
static Bool freewinsScaleWindow (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption){
	CompWindow *w;

    w = findWindowAtDisplay (d, getIntOptionNamed(option, nOption, "window", 0));
    
    if (w)
    {
        FREEWINS_WINDOW(w);
        
        fww->scaleX = getFloatOptionNamed(option, nOption, "x", 0.0f);
        fww->scaleY = getFloatOptionNamed(option, nOption, "y", 0.0f);
    }
    else
    {
        return FALSE;
    }
    
    if (FWCanShape (w))
        FWShapeInput (w);
    
    damageScreen(w->screen);

    return TRUE;
}

// Toggle Axis /*{{{*/
static Bool toggleFWAxis (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption){

    FREEWINS_DISPLAY(d);

    fwd->axisHelp = !fwd->axisHelp;

    return TRUE;
}

// Reset Rotation/*{{{*/
static Bool resetFWRotation (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption){
    
    CompWindow* w;
    
    w = findWindowAtDisplay (d, getIntOptionNamed(option, nOption, "window", 0));

    if(w){
	FREEWINS_WINDOW(w);

	damageScreen(w->screen);

	if( fww->rotated ){
	    FREEWINS_SCREEN(w->screen);
	    fws->rotatedWindows--;
	    fww->rotated = FALSE;
	}

    // Set values to animate from
	fww->oldAngX = fww->angX;
    fww->oldAngY = fww->angY;
    fww->oldAngZ = fww->angZ;
	fww->oldScaleX = fww->scaleX;
	fww->oldScaleY = fww->scaleY;
	
	// Set values to animate to
	fww->destAngX = 0.0f;
	fww->destAngY = 0.0f;
	fww->destAngZ = 0.0f;
	
	fww->destScaleX = 1.0f;
	fww->destScaleY = 1.0f;
	
	// Reset unsnapped values too
	
    fww->unsnapAngX = 0.0f;
    fww->unsnapAngY = 0.0f;
    fww->unsnapAngZ = 0.0f;
    
    fww->unsnapScaleX = 1.0f;
    fww->unsnapScaleY = 1.0f;

	
	fww->doAnimate = TRUE;
	fww->resetting = TRUE;
    }
    
    return TRUE;
}
/*}}}*/

// Window initialisation / cleaning
static Bool freewinsInitWindow(CompPlugin *p, CompWindow *w){
    FWWindow *fww;
    FREEWINS_SCREEN(w->screen);

    if( !(fww = (FWWindow*)malloc( sizeof(FWWindow) )) )
	return FALSE;

    fww->angX = 0.0;
    fww->angY = 0.0;
    fww->angZ = 0.0;

    fww->midX = WIN_REAL_W(w)/2.0;
    fww->midY = WIN_REAL_H(w)/2.0;

    fww->grabbed = 0;
    fww->zaxis = FALSE;

    // Window Bounding box
    //fww->rect.x1 = fww->rect.x2 = WIN_REAL_X(w);
    //fww->rect.y1 = fww->rect.y2 = WIN_REAL_Y(w);
    //fww->rect.x2 += WIN_REAL_W(w);
    //fww->rect.y2 += WIN_REAL_H(w);

    fww->rotated = FALSE;
    
    // Don't allow anything yet
    fww->allowScaling = FALSE;
    fww->allowRotation = FALSE;
    fww->doAnimate = FALSE;
    fww->resetting = FALSE;
    
    // Don't allow incorrect window drawing as soon as the plugin is started
    
    fww->scaleX = 1.0;
    fww->scaleY = 1.0;
    
    fww->unsnapScaleX = 1.0;
    fww->unsnapScaleY = 1.0;
    
    fww->aTimeRemaining = freewinsGetResetTime (w->screen);
    fww->cTimeRemaining = freewinsGetResetTime (w->screen);

    w->privates[fws->windowPrivateIndex].ptr = fww;
    
    // Shape window back to normal
    if (FWCanShape (w))
        FWShapeInput (w);

    return TRUE;
}

static void freewinsFiniWindow(CompPlugin *p, CompWindow *w){

    FREEWINS_WINDOW(w);
    FREEWINS_DISPLAY(w->screen->display);
    
    /* Shape window back to normal */
    fww->scaleX = 1.0f;
    fww->scaleY = 1.0f;
    
    if (FWCanShape (w))
        FWShapeInput (w);

    if(fwd->grabWindow == w){
	fwd->grabWindow = NULL;
    }

   free(fww); 
}
/*}}}*/

// Screen init / clean/*{{{*/
static Bool freewinsInitScreen(CompPlugin *p, CompScreen *s){
    FWScreen *fws;

    FREEWINS_DISPLAY(s->display);

    if( !(fws = (FWScreen*)malloc( sizeof(FWScreen) )) )
	return FALSE;

    if( (fws->windowPrivateIndex = allocateWindowPrivateIndex(s)) < 0){
	free(fws);
	return FALSE;
    }

    fws->grabIndex = 0;
    fws->rotatedWindows = 0;

    s->privates[fwd->screenPrivateIndex].ptr = fws;
    
    WRAP(fws, s, paintWindow, FWPaintWindow);
    WRAP(fws, s, paintOutput, FWPaintOutput);

    WRAP(fws, s, damageWindowRect, FWDamageWindowRect);

    WRAP(fws, s, windowResizeNotify, FWWindowResizeNotify);

    return TRUE;
}

static void freewinsFiniScreen(CompPlugin *p, CompScreen *s){

    FREEWINS_SCREEN(s);

    freeWindowPrivateIndex(s, fws->windowPrivateIndex);

    UNWRAP(fws, s, paintWindow);
    UNWRAP(fws, s, paintOutput);

    UNWRAP(fws, s, damageWindowRect);

    UNWRAP(fws, s, windowResizeNotify);
    free(fws);
}
/*}}}*/

// Display init / clean/*{{{*/
static Bool freewinsInitDisplay(CompPlugin *p, CompDisplay *d){

    FWDisplay *fwd; 

    if( !(fwd = (FWDisplay*)malloc( sizeof(FWDisplay) )) )
	return FALSE;
    
    // Set variables correctly
    fwd->grabWindow = 0;
    fwd->axisHelp = FALSE;
    fwd->focusWindow = 0;
     
    if( (fwd->screenPrivateIndex = allocateScreenPrivateIndex(d)) < 0 ){
	free(fwd);
	return FALSE;
    }
    
    // Spit out a warning if there is no shape extension
    if (!d->shapeExtension)
        compLogMessage(d, "freewins", CompLogLevelInfo, "No input shaping extension. Input shaping disabled");


    /* BCOP Action initiation */
    freewinsSetInitiateRotationInitiate(d, initiateFWRotate);
    freewinsSetInitiateScaleInitiate(d, initiateFWScale);
    freewinsSetResetInitiate(d, resetFWRotation);
    freewinsSetToggleAxisInitiate(d, toggleFWAxis);
    
    // Rotate / Scale Up Down Left Right
    freewinsSetScaleUpInitiate(d, FWScaleUp);
    freewinsSetScaleDownInitiate(d, FWScaleDown);

    freewinsSetRotateUpInitiate(d, FWRotateUp);
    freewinsSetRotateDownInitiate(d, FWRotateDown);
    freewinsSetRotateLeftInitiate(d, FWRotateLeft);
    freewinsSetRotateRightInitiate(d, FWRotateRight);
    freewinsSetRotateCInitiate(d, FWRotateClockwise);
    freewinsSetRotateCcInitiate(d, FWRotateCounterclockwise);
    
    freewinsSetRotateInitiate (d, freewinsRotateWindow);
    freewinsSetIncrementRotateInitiate (d, freewinsIncrementRotateWindow);
    freewinsSetScaleInitiate (d, freewinsScaleWindow);
    
    d->base.privates[displayPrivateIndex].ptr = fwd;
    WRAP(fwd, d, handleEvent, FWHandleEvent);
    
    return TRUE;
}

static void freewinsFiniDisplay(CompPlugin *p, CompDisplay *d){

    FREEWINS_DISPLAY(d);
    
    freeScreenPrivateIndex(d, fwd->screenPrivateIndex);

    UNWRAP(fwd, d, handleEvent);

    free(fwd);
}
/*}}}*/
// Plugin init / clean
static Bool freewinsInit(CompPlugin *p){
    
    
    if( (displayPrivateIndex = allocateDisplayPrivateIndex()) < 0 )
	return FALSE;

	compAddMetadataFromFile (&freewinsMetadata, p->vTable->name);

    return TRUE;
}


static void freewinsFini(CompPlugin *p){

    if(displayPrivateIndex >= 0)
	freeDisplayPrivateIndex( displayPrivateIndex );
}

static int
freewinsGetVersion (CompPlugin *plugin,
		int	   version)
{
    return ABIVERSION;
}

// Plugin implementation export
CompPluginVTable freewinsVTable = {
    "freewins",
    freewinsGetVersion,
    0,
    freewinsInit,
    freewinsFini,
    freewinsInitDisplay,
    freewinsFiniDisplay,
    freewinsInitScreen,
    freewinsFiniScreen,
    freewinsInitWindow,
    freewinsFiniWindow,
    0,
    0,
    0,
    0
};

CompPluginVTable *getCompPluginInfo (void){ return &freewinsVTable; }

