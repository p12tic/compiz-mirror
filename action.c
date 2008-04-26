/**
 * Compiz Fusion Freewins plugin
 *
 * action.c
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
 * Sam Spilsbury <smspillaz@gmail.com>
 *
 * Button binding support and Reset added by:
 * enigma_0Z <enigma.0ZA@gmail.com>
 *
 * Most of the input handling here is based on
 * the shelf plugin by
 *        : Kristian Lyngstøl <kristian@bohemians.org>
 *        : Danny Baumann <maniac@opencompositing.org>
 *
 * Description:
 *
 * This plugin allows you to freely transform the texture of a window,
 * whether that be rotation or scaling to make better use of screen space
 * or just as a toy.
 *
 * Todo: 
 *  - Modifier key to rotate on the Z Axis
 *  - Fully implement an input redirection system by
 *    finding an inverse matrix, multiplying by it,
 *    translating to the actual window co-ords and 
 *    XSendEvent() the co-ords to the actual window.
 *  - Code could be cleaner
 *  - Add timestep and speed options to animation
 *  - Add window hover-over info via paintOutput : i.e
 *    - Resize borders
 *    - 'Reset' Button
 *    - 'Scale' Button
 *    - 'Rotate' Button
 */

#include "freewins.h"


/* ------ Actions -------------------------------------------------------*/

/* Initiate Mouse Rotation */
Bool initiateFWRotate (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    CompWindow* w;
    CompWindow *useW;
    CompScreen* s;
    FWWindowInputInfo *info;
    Window xid, root;
    float dx, dy;
    
    FREEWINS_DISPLAY(d);

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    useW = findWindowAtDisplay (d, xid);

    root = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, root);

    if (s)
    {

    FREEWINS_SCREEN (s);

    for (info = fws->transformedWindows; info; info = info->next)
    {
        if (w->id == info->ipw)
        /* The window we just grabbed was actually
         * an IPW, get the real window instead
         */
        useW = FWGetRealWindow (w);
    }

	fws->rotateCursor = XCreateFontCursor (s->display->display, XC_fleur);	

	if(!otherScreenGrabExist(s, "freewins", 0))
	    if(!fws->grabIndex)
		fws->grabIndex = pushScreenGrab(s, fws->rotateCursor, "freewins");

    }
    
    
    if(useW){
	FREEWINS_WINDOW(useW);

    switch (freewinsGetRotationAxis (w->screen))
    {
        case RotationAxisAlwaysCentre:
        default:
            FWCalculateInputOrigin(w, WIN_REAL_X (w) + WIN_REAL_W (w) / 2.0f,
                                      WIN_REAL_Y (w) + WIN_REAL_H (w) / 2.0f);
            FWCalculateOutputOrigin (w, WIN_OUTPUT_W (w) / 2.0f, WIN_OUTPUT_H (w) / 2.0f);
            break;
        case RotationAxisClickPoint:            
            FWCalculateInputOrigin(w, fwd->click_root_x, fwd->click_root_y);
            FWCalculateOutputOrigin(w, fwd->click_root_x, fwd->click_root_y);
            break;
        case RotationAxisOppositeToClick:            
            FWCalculateInputOrigin(w, w->attrib.x + w->width - fwd->click_root_x,
                                      w->attrib.y + w->height - fwd->click_root_y);
            FWCalculateOutputOrigin(w, w->attrib.x + w->width - fwd->click_root_x,
                                      w->attrib.y + w->height - fwd->click_root_y);
            break;
    }
	
	fww->allowRotation = TRUE;
	fww->allowScaling = FALSE;
	
	fwd->grabWindow = useW;
	
	fwd->grab = grabRotate;
	
	fwd->oldX = fwd->click_root_x;
	fwd->oldY = fwd->click_root_y;

	dx = fwd->click_win_x - fww->iMidX;
	dy = fwd->click_win_y - fww->iMidY;

    /* Save current scales and angles */

    fww->animate.oldAngX = fww->transform.angX;
    fww->animate.oldAngY = fww->transform.angY;
    fww->animate.oldAngZ = fww->transform.angZ;
    fww->animate.oldScaleX = fww->transform.scaleX;
    fww->animate.oldScaleY = fww->transform.scaleY;

	if (fwd->click_root_y > fww->iMidY)
	{
	    if (fwd->click_root_x > fww->iMidX)
	        fww->corner = CornerBottomRight;
	    else if (fwd->click_root_x < fww->iMidX)
	        fww->corner = CornerBottomLeft;
	}
	else if (fwd->click_root_y < fww->iMidY)
	{
	    if (fwd->click_root_x > fww->iMidX)
	        fww->corner = CornerTopRight;
	    else if (fwd->click_root_x < fww->iMidX)
	        fww->corner = CornerTopLeft;
	}

	dx = ABS(dx);
	dy = ABS(dy);

    switch (freewinsGetZAxisRotation (s))
    {
        case ZAxisRotationAlways3d:
            fww->can3D = TRUE;
            fww->can2D = FALSE; break;
        case ZAxisRotationAlways2d:
            fww->can3D = FALSE;
            fww->can2D = TRUE; break;
        case ZAxisRotationDetermineOnClick:
            FWDetermineZAxisClick (useW, pointerX, pointerY); break;
        case ZAxisRotationInterchangable:
            fww->can3D = TRUE;
            fww->can2D = TRUE;  break;
        default:
            break;
    }

	}
	
    return TRUE;
}

/* Initiate Scaling */
Bool initiateFWScale (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    CompWindow* w;
    CompWindow *useW;
    CompScreen* s;
    FWWindowInputInfo *info;
    Window xid, root;
    float dx, dy;
    
    FREEWINS_DISPLAY(d);

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    useW = findWindowAtDisplay (d, xid);

    root = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, root);

    if (s)
    {

	FREEWINS_SCREEN(s);

    for (info = fws->transformedWindows; info; info = info->next)
    {
        if (w->id == info->ipw)
        /* The window we just grabbed was actually
         * an IPW, get the real window instead
         */
        useW = FWGetRealWindow (w);
    }

	fws->rotateCursor = XCreateFontCursor (s->display->display, XC_plus);	

	if(!otherScreenGrabExist(s, "freewins", 0))
	    if(!fws->grabIndex)
		fws->grabIndex = pushScreenGrab(s, fws->rotateCursor, "freewins");

    }
    
    
    if(useW){
	FREEWINS_WINDOW(useW);
	
	fww->allowScaling = TRUE;
	fww->allowRotation = FALSE;
	
	fwd->grabWindow = useW;
	
	/* Find out the corner we clicked in */

    float MidX = fww->inputRect.x1 + ((fww->inputRect.x2 - fww->inputRect.x1) / 2.0f);
    float MidY = fww->inputRect.y1 + ((fww->inputRect.y2 - fww->inputRect.y1) / 2.0f);
	
	/* Check for Y axis clicking (Top / Bottom) */
	if (fwd->click_root_y > MidY)
	{
	    /* Check for X axis clicking (Left / Right) */
	    if (fwd->click_root_x > MidX)
	        fww->corner = CornerBottomRight;
	    else if (fwd->click_root_x < MidX)
	        fww->corner = CornerBottomLeft;
	}
	else if (fwd->click_win_y < MidY)
	{
	    /* Check for X axis clicking (Left / Right) */
	    if (fwd->click_root_x > MidX)
	        fww->corner = CornerTopRight;
	    else if (fwd->click_root_x < MidX)
	        fww->corner = CornerTopLeft;
	}

    switch (freewinsGetScaleMode (w->screen))
    {
        case ScaleModeToCentre:
            FWCalculateInputOrigin(w, WIN_REAL_X (w) + WIN_REAL_W (w) / 2.0f,
                                      WIN_REAL_Y (w) + WIN_REAL_H (w) / 2.0f);
            FWCalculateOutputOrigin(w, WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w) / 2.0f,
                                       WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w) / 2.0f);
            break;
        case ScaleModeToOppositeCorner:
            switch (fww->corner)
            {
                case CornerBottomRight:
                /* Translate origin to the top left of the window */
                FWCalculateInputOrigin (w, WIN_REAL_X (w), WIN_REAL_Y (w));
                FWCalculateOutputOrigin (w, WIN_OUTPUT_X (w), WIN_OUTPUT_Y (w));
                break;
                case CornerBottomLeft:
                /* Translate origin to the top right of the window */
                FWCalculateInputOrigin (w, WIN_REAL_X (w) + WIN_REAL_W (w), WIN_REAL_Y (w));
                FWCalculateOutputOrigin (w, WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w), WIN_OUTPUT_Y (w));
                break;
                case CornerTopRight:
                /* Translate origin to the bottom left of the window */
                FWCalculateInputOrigin (w, WIN_REAL_X (w), WIN_REAL_Y (w) + WIN_REAL_H (w));
                FWCalculateOutputOrigin (w, WIN_OUTPUT_X (w), WIN_OUTPUT_Y (w));
                break;
                case CornerTopLeft:
                /* Translate origin to the bottom right of the window */
                FWCalculateInputOrigin (w, WIN_REAL_X (w) + WIN_REAL_W (w), WIN_REAL_Y (w) + WIN_REAL_H (w));
                FWCalculateOutputOrigin (w, WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w), WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w));
                break;
            }
            break;
    }

    fwd->grab = grabScale;

	fwd->oldX = fwd->click_root_x;
	fwd->oldY = fwd->click_root_y;

	dx = fwd->click_win_x - fww->iMidX;
	dy = fwd->click_win_y - fww->iMidY;

	fww->grabLeft = (dx > 0 ? FALSE : TRUE);
	fww->grabTop = (dy > 0 ? FALSE : TRUE);

	dx = ABS(dx);
	dy = ABS(dy);
	
	}
    
    return TRUE;
}

#define GET_WINDOW \
    CompWindow *tW, *w; \
    CompScreen *s; \
    Window xid; \
    FWWindowInputInfo *info; \
    xid = getIntOptionNamed (option, nOption, "window", 0); \
    tW = findWindowAtDisplay (d, xid); \
    w = tW; \
    s = findScreenAtDisplay (d, getIntOptionNamed (option, nOption, "root", 0)); \
    if (s) \
    { \
	FREEWINS_SCREEN(s); \
    for (info = fws->transformedWindows; info; info = info->next) \
    { \
        if (tW->id == info->ipw) \
        w = FWGetRealWindow (tW); \
        break; \
    } \
    } \


/* Repetitive Stuff */

void
FWSetPrepareRotation (CompWindow *w, float dx, float dy, float dz, float dsu, float dsd)
{
    FREEWINS_WINDOW (w);

    FWCalculateInputOrigin(w, WIN_REAL_X (w) + WIN_REAL_W (w) / 2.0f,
                              WIN_REAL_Y (w) + WIN_REAL_H (w) / 2.0f);
    FWCalculateOutputOrigin(w, WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w) / 2.0f,
                               WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w) / 2.0f);

    fww->transform.unsnapAngX += dy;
    fww->transform.unsnapAngY -= dx;
    fww->transform.unsnapAngZ += dz;
    
    fww->transform.unsnapScaleX += dsu;
    fww->transform.unsnapScaleY += dsd;

    fww->animate.oldAngX = fww->transform.angX; 
    fww->animate.oldAngY = fww->transform.angY; 
    fww->animate.oldAngZ = fww->transform.angZ;

    fww->animate.oldScaleX = fww->transform.scaleX; 
    fww->animate.oldScaleY = fww->transform.scaleY;

    fww->animate.destAngX = fww->transform.angX + dy; 
    fww->animate.destAngY = fww->transform.angY - dx; 
    fww->animate.destAngZ = fww->transform.angZ + dz;

    fww->animate.destScaleX = fww->transform.scaleX + dsu; 
    fww->animate.destScaleY = fww->transform.scaleY + dsd;

    fww->animate.aTimeRemaining = freewinsGetRotateIncrementTime (w->screen); 
    fww->animate.cTimeRemaining = freewinsGetRotateIncrementTime (w->screen); 
    fww->doAnimate = TRUE; // Start animating
}

#define ROTATE_INC freewinsGetRotateIncrementAmount (w->screen)
#define NEG_ROTATE_INC freewinsGetRotateIncrementAmount (w->screen) *-1

#define SCALE_INC freewinsGetScaleIncrementAmount (w->screen)
#define NEG_SCALE_INC freewinsGetScaleIncrementAmount (w->screen) *-1

Bool FWRotateUp (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {

    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, 0, ROTATE_INC, 0, 0, 0);

        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);
    }
    
    return TRUE;
    
}

Bool FWRotateDown (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {

    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, 0, NEG_ROTATE_INC, 0, 0, 0);
    
        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);
    }
    return TRUE;
    
}

Bool FWRotateLeft (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {

    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, ROTATE_INC, 0, 0, 0, 0);
        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);
    }
    
    return TRUE;
    
}

Bool FWRotateRight (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, NEG_ROTATE_INC, 0, 0, 0, 0);
        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);
    }
    
    return TRUE;
    
}

Bool FWRotateClockwise (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, 0, 0, ROTATE_INC, 0, 0);
        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);
    }
    
    return TRUE;
    
}

Bool FWRotateCounterclockwise (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, 0, 0, NEG_ROTATE_INC, 0, 0);
        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);
    }
    
    return TRUE;
    
}

Bool FWScaleUp (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, 0, 0, 0, SCALE_INC, SCALE_INC);
        addWindowDamage (w); // Smoothen Painting
        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);
    }
    
    return TRUE;
    
}

Bool FWScaleDown (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, 0, 0, 0, NEG_SCALE_INC, NEG_SCALE_INC);
        addWindowDamage (w); // Smoothen Painting
        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);
    }
    
    return TRUE;
    
}

/* Reset the Rotation and Scale to 0 and 1 */
/* TODO: Rename to resetFWTransform */
Bool resetFWRotation (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption){
    
    GET_WINDOW;
    if (w)
    {
        FREEWINS_WINDOW (w);
        FWSetPrepareRotation (w, fww->transform.angY,
                                 -fww->transform.angX,
                                 -fww->transform.angZ,
                                 (1 - fww->transform.scaleX),
                                 (1 - fww->transform.scaleY));
        addWindowDamage (w);

	    if( fww->rotated ){
	        FREEWINS_SCREEN(w->screen);
	        fws->rotatedWindows--;
	        fww->rotated = FALSE;
	    }

        if (FWCanShape (w))
            if (FWHandleWindowInputInfo (w))
                FWAdjustIPW (w);

        fww->resetting = TRUE;
    }

    return TRUE;
}

/* Callable action to rotate a window to the angle provided
 * x: Set angle to x degrees
 * y: Set angle to y degrees
 * z: Set angle to z degrees
 * window: The window to apply the transformation to
 */
Bool freewinsRotateWindow (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption){
	CompWindow *w;
    
    w = findWindowAtDisplay (d, getIntOptionNamed(option, nOption, "window", 0));
    
    if (w)
    {
        FREEWINS_WINDOW(w);
        
        float x, y, z;
        
        y = getFloatOptionNamed(option, nOption, "x", 0.0f);
        x = getFloatOptionNamed(option, nOption, "y", 0.0f);
        z = getFloatOptionNamed(option, nOption, "z", 0.0f);
        
        fww->transform.angX = x;
        fww->transform.angY = y;
        fww->transform.angZ = z;

        addWindowDamage (w);
        
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

/* Callable action to increment window rotation by the angles provided
 * x: Increment angle by x degrees
 * y: Increment angle by y degrees
 * z: Increment angle by z degrees
 * window: The window to apply the transformation to
 */
Bool freewinsIncrementRotateWindow (CompDisplay *d, CompAction *action, 
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
        fww->transform.angX += x;
        fww->transform.angY += y;
        fww->transform.angZ += z;

        addWindowDamage (w);
        
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

/* Callable action to scale a window to the scale provided
 * x: Set scale to x factor
 * y: Set scale to y factor
 * window: The window to apply the transformation to
 */
Bool freewinsScaleWindow (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption){
	CompWindow *w;

    w = findWindowAtDisplay (d, getIntOptionNamed(option, nOption, "window", 0));
    
    if (w)
    {
        FREEWINS_WINDOW(w);
        
        fww->transform.scaleX = getFloatOptionNamed(option, nOption, "x", 0.0f);
        fww->transform.scaleY = getFloatOptionNamed(option, nOption, "y", 0.0f);

        addWindowDamage (w);
    }
    else
    {
        return FALSE;
    }
    
    if (FWCanShape (w))
        FWHandleWindowInputInfo (w);
    
    return TRUE;
}

/* Toggle Axis-Help Display */
Bool toggleFWAxis (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption){

    CompScreen *s;

    FREEWINS_DISPLAY(d);

    s = findScreenAtDisplay(d, getIntOptionNamed(option, nOption, "root", 0));

    fwd->axisHelp = !fwd->axisHelp;
    if (s)
        damageScreen (s);

    return TRUE;
}

