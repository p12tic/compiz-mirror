/**
 * Compiz Fusion Freewins plugin
 *
 * events.c
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


/* ------ Event Handlers ------------------------------------------------*/

void FWHandleIPWResizeInitiate (CompWindow *w)
{
    FREEWINS_SCREEN (w->screen);
    FREEWINS_DISPLAY (w->screen->display);

    (*w->screen->activateWindow) (w);
    fwd->grab = grabResize;
    fws->rotateCursor = XCreateFontCursor (w->screen->display->display, XC_plus);	
	if(!otherScreenGrabExist(w->screen, "freewins", "resize", 0))
	    if(!fws->grabIndex)
        {
        unsigned int mods = 0;
        mods &= CompNoMask;
		fws->grabIndex = pushScreenGrab(w->screen, fws->rotateCursor, "resize");
	    (w->screen->windowGrabNotify) (w,  w->attrib.x + (w->width / 2),
                                           w->attrib.y + (w->height / 2), mods,
					                       CompWindowGrabMoveMask |
					                       CompWindowGrabButtonMask);
        fwd->grabWindow = w;
        }
}

void FWHandleIPWMoveInitiate (CompWindow *w)
{
    FREEWINS_SCREEN (w->screen);
    FREEWINS_DISPLAY (w->screen->display);

    (*w->screen->activateWindow) (w);
    fwd->grab = grabMove;
    fws->rotateCursor = XCreateFontCursor (w->screen->display->display, XC_fleur);	
	if(!otherScreenGrabExist(w->screen, "freewins", "move", 0))
	    if(!fws->grabIndex)
        {
        unsigned int mods = 0;
        mods &= CompNoMask;
		fws->grabIndex = pushScreenGrab(w->screen, fws->rotateCursor, "move");
	    (w->screen->windowGrabNotify) (w,  w->attrib.x + (w->width / 2),
                                           w->attrib.y + (w->height / 2), mods,
					                       CompWindowGrabMoveMask |
					                       CompWindowGrabButtonMask);
        }
    fwd->grabWindow = w;
}

void FWHandleIPWMoveMotionEvent (CompWindow *w, unsigned int x, unsigned int y)
{
    FREEWINS_SCREEN (w->screen);

    //static int oldPointerX, oldPointerY;

    int dx = x - lastPointerX;
    int dy = y - lastPointerY;

    if (!fws->grabIndex)
        return;

    moveWindow(w, dx,
                  dy, TRUE, freewinsGetImmediateMoves (w->screen));
    syncWindowPosition (w);

    FWAdjustIPW (w);
}

void FWHandleIPWResizeMotionEvent (CompWindow *w, unsigned int x, unsigned int y)
{
    FREEWINS_WINDOW (w);

    int dx = (x - lastPointerX) * 10;
    int dy = (y - lastPointerY) * 10;

    fww->winH += dx;
    fww->winW += dy;

    /* In order to prevent a window redraw on resize
     * on every motion event we have a threshold
     */

    if (fww->winH - 10 > w->height || fww->winW - 10 > w->width)
    { 

        XWindowChanges xwc;
        unsigned int   mask = CWX | CWY | CWWidth | CWHeight;

        xwc.x = w->serverX;
        xwc.y = w->serverX;
        xwc.width = fww->winW;
        xwc.height = fww->winH;

        if (xwc.width == w->serverWidth)
	    mask &= ~CWWidth;

        if (xwc.height == w->serverHeight)
	    mask &= ~CWHeight;

        if (w->mapNum && (mask & (CWWidth | CWHeight)))
	    sendSyncRequest (w);

        configureXWindow (w, mask, &xwc);
    }

    FWAdjustIPW (w);
}


/* Handle Rotation */
void FWHandleRotateMotionEvent (CompWindow *w, float dx, float dy, int x, int y)
{
    FREEWINS_WINDOW (w);
    FREEWINS_DISPLAY (w->screen->display);

    x -= 100;
    y -= 100;

    int oldX = lastPointerX - 100;
    int oldY = lastPointerY - 100;

	float midX = WIN_REAL_X(fwd->grabWindow) + WIN_REAL_W(fwd->grabWindow)/2.0;
	float midY = WIN_REAL_Y(fwd->grabWindow) + WIN_REAL_H(fwd->grabWindow)/2.0;

  /* Check for Y axis clicking (Top / Bottom) */
	if (pointerY > midY)
	{
	    /* Check for X axis clicking (Left / Right) */
	    if (pointerX > midX)
	        fww->corner = CornerBottomRight;
	    else if (pointerX < midX)
	        fww->corner = CornerBottomLeft;
	}
	else if (pointerY < midY)
	{
	    /* Check for X axis clicking (Left / Right) */
	    if (pointerX > midX)
	        fww->corner = CornerTopRight;
	    else if (pointerX < midX)
	        fww->corner = CornerTopLeft;
	}

    float percentFromXAxis, percentFromYAxis;

    percentFromXAxis = 0.0f;
    percentFromYAxis = 0.0f;

    if (freewinsGetZAxisRotation (w->screen) == ZAxisRotationInterchangable)
    {

        /* Trackball rotation was too hard to implement. If anyone can implement it,
         * please come forward so I can replace this hacky solution to the problem.
         * Anyways, what happens here, is that we determine how far away we are from
         * each axis (y and x). The further we are away from the y axis, the more
         * up / down movements become Z axis movements and the further we are away from
         * the x-axis, the more left / right movements become z rotations. */

        /* We determine this by taking a percentage of how far away the cursor is from
         * each axis. We divide the 3D rotation by this percentage ( and divide by the
         * percentage squared in order to ensure that rotation is not too violent when we
         * are quite close to the origin. We multiply the 2D rotation by this percentage also
         * so we are essentially rotating in 3D and 2D all the time, but this is only really
         * noticeable when you move the cursor over to the extremes of a window. In every case
         * percentage can be defined as decimal-percentage (i.e 0.036 == 3.6%). Like I mentioned
         * earlier, if you can replace this with trackball rotation, please come forward! */

        float halfWidth = WIN_REAL_W (w) / 2.0f;
        float halfHeight = WIN_REAL_H (w) / 2.0f;

        float distFromXAxis = fabs (fww->iMidX - pointerX);
        float distFromYAxis = fabs (fww->iMidY - pointerY);

        percentFromXAxis = distFromXAxis / halfWidth;
        percentFromYAxis = distFromYAxis / halfHeight;

    }

    dx *= 360;
    dy *= 360;

    if(fww->can2D)
    {

       float zX = percentFromXAxis;
       float zY = percentFromYAxis;

       zX = zX > 1.0f ? 1.0f : zX;
       zY = zY > 1.0f ? 1.0f : zY;

       switch (fww->corner)
       {
            case CornerTopRight:

                if ((x) < oldX)
                fww->transform.unsnapAngZ -= dx * zX;
                else if ((x) > oldX)
                fww->transform.unsnapAngZ += dx * zX;


                if ((y) < oldY)
                fww->transform.unsnapAngZ -= dy * zY;
                else if ((y) > oldY)
                fww->transform.unsnapAngZ += dy * zY;

                break;

            case CornerTopLeft:

                if ((x) < oldX)
                fww->transform.unsnapAngZ -= dx * zX;
                else if ((x) > oldX)
                fww->transform.unsnapAngZ += dx * zX;


                if ((y) < oldY)
                fww->transform.unsnapAngZ += dy * zY;
                else if ((y) > oldY)
                fww->transform.unsnapAngZ -= dy * zY;

                break;

            case CornerBottomLeft:

                if ((x) < oldX)
                fww->transform.unsnapAngZ += dx * zX;
                else if ((x) > oldX)
                fww->transform.unsnapAngZ -= dx * zX;


                if ((y) < oldY)
                fww->transform.unsnapAngZ += dy * zY;
                else if ((y) > oldY)
                fww->transform.unsnapAngZ -= dy * zY;

                break;

            case CornerBottomRight:

                if ((x) < oldX)
                fww->transform.unsnapAngZ += dx * zX;
                else if ((x) > oldX)
                fww->transform.unsnapAngZ -= dx * zX;


                if ((y) < oldY)
                fww->transform.unsnapAngZ -= dy * zY;
                else if ((y) > oldY)
                fww->transform.unsnapAngZ += dy * zY;
                break;
        }
    }

    if (fww->can3D || freewinsGetZAxisRotation (w->screen) == ZAxisRotationAlways3d)
    {
    fww->transform.unsnapAngX -= dy * (1 - percentFromXAxis);
    fww->transform.unsnapAngY += dx * (1 - percentFromYAxis);
    }

    FWAdjustIPW (w);

}

/* Handle Scaling */
void FWHandleScaleMotionEvent (CompWindow *w, float dx, float dy, int x, int y)
{
    FREEWINS_WINDOW (w);
    FREEWINS_DISPLAY (w->screen->display);

    x -= 100.0;
    y -= 100.0;

    FWCalculateInputRect (w);

    switch (fww->corner)
    {

        case CornerTopLeft:
        
        if ((x) < fwd->oldX)
        fww->transform.unsnapScaleX -= dx;
        else if ((x) > fwd->oldX)
        fww->transform.unsnapScaleX -= dx;

        if ((y) < fwd->oldY)
        fww->transform.unsnapScaleY -= dy;
        else if ((y) > fwd->oldY)
        fww->transform.unsnapScaleY -= dy;
        break;            
            
        case CornerTopRight:

        if ((x) < fwd->oldX)
        fww->transform.unsnapScaleX += dx;
        else if ((y) > fwd->oldX)
        fww->transform.unsnapScaleX += dx;


        // Check Y Direction
        if ((y) < fwd->oldY)
        fww->transform.unsnapScaleY -= dy;
        else if ((y) > fwd->oldY)
        fww->transform.unsnapScaleY -= dy;

        break;

        case CornerBottomLeft:

        // Check X Direction
        if ((x) < fwd->oldX)
        fww->transform.unsnapScaleX -= dx;
        else if ((y) > fwd->oldX)
        fww->transform.unsnapScaleX -= dx;

        // Check Y Direction
        if ((y) < fwd->oldY)
        fww->transform.unsnapScaleY += dy;
        else if ((y) > fwd->oldY)
        fww->transform.unsnapScaleY += dy;

        break;

        case CornerBottomRight:

        // Check X Direction
        if ((x) < fwd->oldX)
        fww->transform.unsnapScaleX += dx;
        else if ((x) > fwd->oldX)
        fww->transform.unsnapScaleX += dx;

        // Check Y Direction
        if ((y) < fwd->oldY)
        fww->transform.unsnapScaleY += dy;
        else if ((y) > fwd->oldY)
        fww->transform.unsnapScaleY += dy;
        break;
    }

    FWAdjustIPW (w);

}

void FWHandleButtonReleaseEvent (CompWindow *w)
{
    FREEWINS_SCREEN (w->screen);
    FREEWINS_DISPLAY (w->screen->display);

    if (fwd->grab == grabMove || fwd->grab == grabResize)
    {
        removeScreenGrab (w->screen, fws->grabIndex, NULL);
        (w->screen->windowUngrabNotify) (w);
        moveInputFocusToWindow (w);
        fws->grabIndex = 0;
        fwd->grabWindow = NULL;
        fwd->grab = grabNone;
    }
}

void
FWHandleEnterNotify (CompWindow *w,
                     XEvent *xev)
{
    XEvent EnterNotifyEvent;

    memcpy (&EnterNotifyEvent.xcrossing, &xev->xcrossing,
	    sizeof (XCrossingEvent));
    EnterNotifyEvent.xcrossing.window = w->id;

    if (w)
    XSendEvent (w->screen->display->display, w->id,
		FALSE, EnterWindowMask, &EnterNotifyEvent);
}

void
FWHandleLeaveNotify (CompWindow *w,
                     XEvent *xev)
{
    XEvent LeaveNotifyEvent;

    memcpy (&LeaveNotifyEvent.xcrossing, &xev->xcrossing,
            sizeof (XCrossingEvent));
    LeaveNotifyEvent.xcrossing.window = w->id;

    XSendEvent (w->screen->display->display, w->id, FALSE,
                LeaveWindowMask, &LeaveNotifyEvent);
}


/* X Event Handler */
void FWHandleEvent(CompDisplay *d, XEvent *ev){

    float dx, dy;
    CompWindow *oldPrev, *oldNext, *w;
    FREEWINS_DISPLAY(d);

    switch(ev->type){
	
	/* Motion Notify Event */
    case EnterNotify:
    {
        CompWindow *btnW;
        btnW = findWindowAtDisplay (d, ev->xbutton.subwindow);

        /* It wasn't the subwindow, try the window */
        if (!btnW)
            btnW = findWindowAtDisplay (d, ev->xbutton.window);

        /* We have established the CompWindow we clicked
         * on. Get the real window */
        if (btnW)
        {
            if (FWCanShape (btnW) && !fwd->grabWindow && !otherScreenGrabExist(btnW->screen, 0))
                fwd->hoverWindow = btnW;
            btnW = FWGetRealWindow (btnW);
        }

        if (btnW)
        {
            if (FWCanShape (btnW) && !fwd->grabWindow && !otherScreenGrabExist(btnW->screen, 0))
                fwd->hoverWindow = btnW;
            FWHandleEnterNotify (btnW, ev);
        }
    }
    break;
    case LeaveNotify:
    {

        CompWindow *btnW;
        btnW = findWindowAtDisplay (d, ev->xbutton.subwindow);

        /* It wasn't the subwindow, try the window */
        if (!btnW)
            btnW = findWindowAtDisplay (d, ev->xbutton.window);

        /* We have established the CompWindow we clicked
         * on. Get the real window */
        if (btnW)
            btnW = FWGetRealWindow (btnW);

        if (btnW)
            FWHandleLeaveNotify (btnW, ev);
    }
    break;
	case MotionNotify:
	    
	    if(fwd->grab != grabNone)
        {
		    FREEWINS_WINDOW(fwd->grabWindow);

		    dx = ((float)(ev->xmotion.x_root - fwd->oldX) / fwd->grabWindow->screen->width) * \
            freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		    dy = ((float)(ev->xmotion.y_root - fwd->oldY) / fwd->grabWindow->screen->height) * \
            freewinsGetMouseSensitivity (fwd->grabWindow->screen);


            if (fwd->grab == grabMove || fwd->grab == grabResize)
            {
                FREEWINS_SCREEN (fwd->grabWindow->screen);
                FWWindowInputInfo *info;
                CompWindow *w = fwd->grabWindow;
                for (info = fws->transformedWindows; info; info = info->next)
                {
                    if (fwd->grabWindow->id == info->ipw)
                    /* The window we just grabbed was actually
                     * an IPW, get the real window instead
                     */
                    w = FWGetRealWindow (fwd->grabWindow);
                }
                switch (fwd->grab)
                {
                    case grabMove:
                        FWHandleIPWMoveMotionEvent (w, pointerX, pointerY); break;
                    case grabResize:
                        FWHandleIPWResizeMotionEvent (w, pointerX, pointerY); break;
                    default:
                        break;
                }
                fww->allowRotation = FALSE;
                fww->allowScaling = FALSE;
            }

            if (fwd->grab == grabRotate)
            {        
                FWHandleRotateMotionEvent(fwd->grabWindow, dx, dy, ev->xmotion.x, ev->xmotion.y);
		    }
		    if (fwd->grab == grabScale)
		    {
		        FWHandleScaleMotionEvent(fwd->grabWindow, dx, dy, ev->xmotion.x, ev->xmotion.y);		      
		    }

            fww->transform.angX = fww->transform.unsnapAngX;
            fww->transform.angY = fww->transform.unsnapAngY;
            fww->transform.angZ = fww->transform.unsnapAngZ;
            fww->transform.scaleX = fww->transform.unsnapScaleX;
            fww->transform.scaleY = fww->transform.unsnapScaleY;

            fwd->oldX += (ev->xmotion.x_root - fwd->oldX);
		    fwd->oldY += (ev->xmotion.y_root - fwd->oldY);

		    fww->grabLeft = (ev->xmotion.x - fww->iMidX > 0 ? FALSE : TRUE);
		    fww->grabTop = (ev->xmotion.y - fww->iMidY > 0 ? FALSE : TRUE);

            /* Stop scale at threshold specified */
            if (!freewinsGetAllowNegative (fwd->grabWindow->screen))
            {
                float minScale = freewinsGetMinScale (fwd->grabWindow->screen);
	            if (fww->transform.scaleX < minScale)
	              fww->transform.scaleX = minScale; // Don't allow negative values either, which could be referenced
	
	            if (fww->transform.scaleY < minScale)
	              fww->transform.scaleY = minScale;
            }

            /* Change scales for maintaining aspect ratio */
            if (freewinsGetScaleUniform (fwd->grabWindow->screen))
            {
                float tempscaleX = fww->transform.scaleX;
                float tempscaleY = fww->transform.scaleY;
                fww->transform.scaleX = (tempscaleX + tempscaleY) / 2;
                fww->transform.scaleY = (tempscaleX + tempscaleY) / 2;
                fww->transform.unsnapScaleX = (tempscaleX + tempscaleY) / 2;
                fww->transform.unsnapScaleY = (tempscaleX + tempscaleY) / 2;
            }

	        /* Handle Snapping */
	        if (freewinsGetSnap (fwd->grabWindow->screen))
	        {
	            int snapFactor = freewinsGetSnapThreshold (fwd->grabWindow->screen);
                fww->transform.angX = ((int) (fww->transform.unsnapAngX) / snapFactor) * snapFactor;
                fww->transform.angY = ((int) (fww->transform.unsnapAngY) / snapFactor) * snapFactor;
                fww->transform.angZ = ((int) (fww->transform.unsnapAngZ) / snapFactor) * snapFactor;
                fww->transform.scaleX = ((float) ( (int) (fww->transform.unsnapScaleX * (21 - snapFactor) + 0.5))) / (21 - snapFactor); 
                fww->transform.scaleY = ((float) ( (int) (fww->transform.unsnapScaleY * (21 - snapFactor) + 0.5))) / (21 - snapFactor);
            }
	
	        /*fww->animate.oldAngX = fww->transform.angX;
	        fww->animate.oldAngY = fww->transform.angY;
	        fww->animate.oldAngZ = fww->transform.angZ;
	
	        fww->animate.oldScaleX = fww->transform.scaleX;
	        fww->animate.oldScaleY = fww->transform.scaleY;*/

	        if(dx != 0.0 || dy != 0.0)
                addWindowDamage (fwd->grabWindow);

        }

    break;


	/* Button Press and Release */
	case ButtonPress:
    {
        CompWindow *btnW;
        btnW = findWindowAtDisplay (d, ev->xbutton.subwindow);

        /* It wasn't the subwindow, try the window */
        if (!btnW)
            btnW = findWindowAtDisplay (d, ev->xbutton.window);

        /* We have established the CompWindow we clicked
         * on. Get the real window
         * FIXME: Free btnW and use another CompWindow * such as realW
         */
        if (btnW)
        btnW = FWGetRealWindow (btnW);

        if (btnW)
        {
            if (fwd->grab == grabNone)
                switch (ev->xbutton.button)
                {
                    case Button1:
                        FWHandleIPWMoveInitiate (btnW); break;
                    case Button3:
                        FWHandleIPWResizeInitiate (btnW); break;
                }
        }

        fwd->oldX =  ev->xbutton.x_root;
	    fwd->oldY =  ev->xbutton.y_root;
	    fwd->click_root_x = ev->xbutton.x_root;
	    fwd->click_root_y = ev->xbutton.y_root;
	    fwd->click_win_x = ev->xbutton.x;
	    fwd->click_win_y = ev->xbutton.y;

        /*if (btnW)
        {

        float x = ev->xbutton.x_root;
        float y = ev->xbutton.y_root;

        fprintf(stderr, "X %f Y %f\n", x, y);

        CompVector vec = { .v = { x, y, 1.0f, 1.0f } };

        CompWindow *realW;

        realW = FWGetRealWindow (btnW);

        if (!realW)
            realW = btnW;

        FREEWINS_WINDOW (realW);

        CompTransform matrix =  FWCreateMatrix (realW,
                                                0.0f - fww->transform.angX,
                                                0.0f - fww->transform.angY,
                                                0.0f - fww->transform.angZ,
                                                fww->iMidX, fww->iMidY, 0.0f,
                                                1.0f - fww->transform.scaleX,
                                                1.0f - fww->transform.scaleY,
                                                1.0f);

        GLdouble xs, ys, zs;

        FWRotateProjectVector (btnW, vec, matrix, &xs, &ys, &zs, TRUE);*/

        /*fprintf(stderr, "CLick :%i %i\n", ev->xbutton.x_root, ev->xbutton.y_root);

        fprintf(stderr, "Transform: %f %f %f\n", fww->transform.angX, fww->transform.angY, fww->transform.angZ);

        fprintf(stderr, "XSYSZS: %f %f %f\n", xs, ys, zs);*/
    }
    break;
	case ButtonRelease:
    {
        if (fwd->grabWindow)
        {
        FREEWINS_WINDOW (fwd->grabWindow);
        if (fwd->grab == grabMove || fwd->grab == grabResize)
            FWHandleButtonReleaseEvent (fwd->grabWindow);

	    if((fwd->grab == grabScale) || (fwd->grab == grabRotate)){
        CompScreen *s;
	    if (FWCanShape (fwd->grabWindow))
	        if (FWHandleWindowInputInfo (fwd->grabWindow))
	            FWAdjustIPW (fwd->grabWindow);
		for(s = d->screens; s; s = s->next){
		    FREEWINS_SCREEN(s);

		    if(fws->grabIndex){
			removeScreenGrab(s, fws->grabIndex, 0);
			fws->grabIndex = 0;
		    }
		}

        fwd->grab = grabNone;
        fwd->lastGrabWindow = fwd->grabWindow;
		fwd->grabWindow = 0;
        fww->rotated = TRUE;
	    }
        }
	    break;
    }

	case ConfigureNotify:
	    w = findWindowAtDisplay (d, ev->xconfigure.window);
	    if (w)
	    {
		oldPrev = w->prev;
		oldNext = w->next;

        FREEWINS_WINDOW (w);

        fww->winH = WIN_REAL_H (w);
        fww->winW = WIN_REAL_W (w);
	    }
	    break;

    case ClientMessage:
    if (ev->xclient.message_type == d->desktopViewportAtom)
    {
        /* Viewport change occurred, or something like that - adjust the IPW's */
        CompScreen *s;
        CompWindow *adjW, *actualW;

        for (s = d->screens; s; s = s->next)
            for (adjW = s->windows; adjW; adjW = adjW->next)
            {
                int vX, vY, dX, dY;

                actualW = FWGetRealWindow (adjW);

                if (!actualW)
                    actualW = adjW;

                if (actualW)
                {
                    CompWindow *ipw;

                    FREEWINS_WINDOW (actualW);

                    if (!fww->input || fww->input->ipw)
                        break;

                    ipw = findWindowAtDisplay (d, fww->input->ipw);

                    dX = s->x - vX;
                    dY = s->y - vY;

                    defaultViewportForWindow (actualW, &vX, &vY);
            		moveWindowToViewportPosition (ipw,
			              ipw->attrib.x - dX * s->width,
			              ipw->attrib.y - dY * s->height,
			              FALSE);
                }
            }
    }
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

		            if (FWCanShape (w) && (fww->transform.scaleX != 1.0f || fww->transform.scaleY != 1.0f))
		            {
		                // Reset the window back to normal
		                fww->transform.scaleX = 1.0f;
		                fww->transform.scaleY = 1.0f;
                        fww->transform.angX = 0.0f;
                        fww->transform.angY = 0.0f;
                        fww->transform.angZ = 0.0f;
			            /*FWShapeInput (w); - Disabled due to problems it causes*/ 
			        }
		        }
	        }
	    }
    }

    UNWRAP(fwd, d, handleEvent);
    (*d->handleEvent)(d, ev);
    WRAP(fwd, d, handleEvent, FWHandleEvent);

    /* Now we can find out if a restacking occurred while we were handing events */

    switch (ev->type)
    {
	case ConfigureNotify:
	    if (w) /* already assigned above */
	    {
		if (w->prev != oldPrev || w->next != oldNext)
		{
		    /* restacking occured, ensure ipw stacking */
		    FWAdjustIPWStacking (w->screen);
            FWAdjustIPW (w);
		}
	    }
	    break;
    }
}
