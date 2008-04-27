/**
 * Compiz Fusion Freewins plugin
 *
 * freewins.h
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

#include <compiz-core.h>
#include <math.h>

#include <stdio.h>
#include <string.h>

#include <X11/cursorfont.h>
#include <X11/extensions/shape.h>

#include <GL/glu.h>
#include <GL/gl.h>

#include "freewins_options.h"

#define ABS(x) ((x)>0?(x):-(x))
#define D2R(x) ((x) * (M_PI / 180.0))
#define R2D(x) ((x) * (180 / M_PI))

/* ------ Macros ---------------------------------------------------------*/
#define GET_FREEWINS_DISPLAY(d)                                       \
    ((FWDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define FREEWINS_DISPLAY(d)                      \
    FWDisplay *fwd = GET_FREEWINS_DISPLAY (d)

#define GET_FREEWINS_SCREEN(s, fwd)                                        \
    ((FWScreen *) (s)->base.privates[(fwd)->screenPrivateIndex].ptr)

#define FREEWINS_SCREEN(s)                                                      \
    FWScreen *fws = GET_FREEWINS_SCREEN (s, GET_FREEWINS_DISPLAY (s->display))

#define GET_FREEWINS_WINDOW(w, fws)                                        \
    ((FWWindow *) (w)->base.privates[(fws)->windowPrivateIndex].ptr)

#define FREEWINS_WINDOW(w)                                         \
    FWWindow *fww = GET_FREEWINS_WINDOW  (w,                    \
                       GET_FREEWINS_SCREEN  (w->screen,            \
                       GET_FREEWINS_DISPLAY (w->screen->display)))



#define WIN_OUTPUT_X(w) (w->attrib.x - w->output.left)
#define WIN_OUTPUT_Y(w) (w->attrib.y - w->output.top)

#define WIN_OUTPUT_W(w) (w->width + w->output.left + w->output.right)
#define WIN_OUTPUT_H(w) (w->height + w->output.top + w->output.bottom)

#define WIN_REAL_X(w) (w->attrib.x - w->input.left)
#define WIN_REAL_Y(w) (w->attrib.y - w->input.top)

#define WIN_REAL_W(w) (w->width + w->input.left + w->input.right)
#define WIN_REAL_H(w) (w->height + w->input.top + w->input.bottom)

/* ------ Structures and Enums ------------------------------------------*/

/* Enums */
typedef enum _StartCorner {
    CornerTopLeft = 0,
    CornerTopRight = 1,
    CornerBottomLeft = 2,
    CornerBottomRight = 3
} StartCorner;

typedef enum _FWGrabType {
    grabNone = 0,
    grabRotate,
    grabScale,
    grabMove,
    grabResize
} FWGrabType;

typedef enum _Direction {
    UpDown = 0,
    LeftRight = 1
} Direction;

/* Shape info / restoration */
typedef struct _FWWindowInputInfo {
    CompWindow                *w;
    struct _FWWindowInputInfo *next;

    Window     ipw;

    XRectangle *inputRects;
    int        nInputRects;
    int        inputRectOrdering;

    XRectangle *frameInputRects;
    int        frameNInputRects;
    int        frameInputRectOrdering;
} FWWindowInputInfo;

/* Transformation info */
typedef struct _FWTransformedWindowInfo
{
    float angX;
    float angY;
    float angZ;
    
    float scaleY;
    float scaleX;

    /* Used for snapping */

    float unsnapAngX;
    float unsnapAngY;
    float unsnapAngZ;

    float unsnapScaleX;
    float unsnapScaleY;

} FWTransformedWindowInfo;

typedef struct _FWAnimationInfo
{
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

    // For animation
    float aTimeRemaining; // Actual time remaining (is decremented)
    float cTimeRemaining; // Constant time remaining (is referenced and not decremented)
    float steps;
} FWAnimationInfo;

/* Freewins Display Structure */
typedef struct _FWDisplay{
    int screenPrivateIndex;

    int click_root_x;
    int click_root_y;

    int click_win_x;
    int click_win_y;

    // ZOMG Input Redirection
    /*
    float transformed_px;
    float transformed_py;
    */

    // Used for determining cursor direction
    int oldX;
    int oldY;

    HandleEventProc handleEvent;

    CompWindow *grabWindow;
    CompWindow *lastGrabWindow;
    CompWindow *hoverWindow;

    FWGrabType grab;
    
    Bool axisHelp;

} FWDisplay;

/* Freewins Screen Structure */
typedef struct _FWScreen{
    int windowPrivateIndex;

    PreparePaintScreenProc preparePaintScreen;
    PaintOutputProc paintOutput;
    PaintWindowProc paintWindow;

    DamageWindowRectProc damageWindowRect;

    WindowResizeNotifyProc windowResizeNotify;
    WindowMoveNotifyProc   windowMoveNotify;

    FWWindowInputInfo *transformedWindows;
    
    Cursor rotateCursor;

    int grabIndex;
    int rotatedWindows;

} FWScreen;

/* Freewins Window Structure */
typedef struct _FWWindow{

    float iMidX;
    float iMidY;

    float oMidX;
    float oMidY;

    float radius;
    
    // Used for determining window movement

    int oldWinX;
    int oldWinY;

    // Used for resize

    int winH;
    int winW;

    // Used to determine axis

    float adjustX;
    float adjustY;

    Direction direction;
    
    // Used to determine starting point
    StartCorner corner;

    // Transformation info
    FWTransformedWindowInfo transform;

    // Animation Info
    FWAnimationInfo animate;

    // Input Info
    FWWindowInputInfo *input;

    Box outputRect;
    Box inputRect;

    // Used to determine whether to animate the window
    Bool doAnimate;
    Bool resetting;
    
    // Used to determine whether rotating on X and Y axis, or just on Z
    Bool can2D;
    Bool can3D;

    Bool grabLeft;
    Bool grabTop;

    Bool rotated;
    Bool allowScaling;
    Bool allowRotation;

} FWWindow;

int displayPrivateIndex;

/* ------ Forward Declarations --------------------------------------------*/

/* freewins.c */

void FWWindowResizeNotify(CompWindow *w,
                         int dx,
                         int dy,
                         int dw,
                         int dh);

void FWWindowMoveNotify (CompWindow *w,
	                       int        dx,
	                       int        dy,
	                       Bool       immediate);

/* paint.c */

void FWPreparePaintScreen (CompScreen *s,
	                      int        ms);

Bool FWPaintWindow(CompWindow *w,
                  const WindowPaintAttrib *attrib,
                  const CompTransform *transform,
                  Region region,
                  unsigned int mask);

Bool FWPaintOutput(CompScreen *s,
                  const ScreenPaintAttrib *sAttrib,
                  const CompTransform *transform,
                  Region region,
                  CompOutput *output,
                  unsigned int mask);

Bool FWDamageWindowRect(CompWindow *w,
                       Bool initial,
                       BoxPtr rect);

/* action.c */

Bool initiateFWRotate (CompDisplay *d,
                      CompAction *action,
                      CompActionState state,
                      CompOption *option,
                      int nOption);

Bool initiateFWScale (CompDisplay *d,
                      CompAction *action,
                      CompActionState state,
                      CompOption *option,
                      int nOption);

void FWSetPrepareRotation (CompWindow *w,
                              float dx,
                              float dy,
                              float dz,
                              float dsu,
                              float dsd);

Bool FWRotateDown (CompDisplay *d,
                  CompAction *action, 
                  CompActionState state,
                  CompOption *option,
                  int nOption);

Bool FWRotateUp (CompDisplay *d,
                  CompAction *action, 
                  CompActionState state,
                  CompOption *option,
                  int nOption);

Bool FWRotateLeft (CompDisplay *d,
                  CompAction *action, 
                  CompActionState state,
                  CompOption *option,
                  int nOption);

Bool FWRotateRight (CompDisplay *d,
                      CompAction *action, 
                      CompActionState state,
                      CompOption *option,
                      int nOption);

Bool FWRotateClockwise (CompDisplay *d,
                          CompAction *action, 
                          CompActionState state,
                          CompOption *option,
                          int nOption);

Bool FWRotateCounterclockwise (CompDisplay *d,
                              CompAction *action, 
                              CompActionState state,
                              CompOption *option,
                              int nOption);

Bool FWScaleUp (CompDisplay *d,
                  CompAction *action, 
                  CompActionState state,
                  CompOption *option,
                  int nOption);

Bool FWScaleDown (CompDisplay *d,
                  CompAction *action, 
                  CompActionState state,
                  CompOption *option,
                  int nOption);

Bool resetFWRotation (CompDisplay *d,
                     CompAction *action,
                     CompActionState state, 
                     CompOption *option, 
                     int nOption);

Bool freewinsRotateWindow (CompDisplay *d,
                          CompAction *action, 
                          CompActionState state,
                          CompOption *option,
                          int nOption);

Bool freewinsIncrementRotateWindow (CompDisplay *d,
                                  CompAction *action, 
                                  CompActionState state,
                                  CompOption *option,
                                  int nOption);

Bool freewinsScaleWindow (CompDisplay *d,
                          CompAction *action, 
                          CompActionState state,
                          CompOption *option,
                          int nOption);

Bool toggleFWAxis (CompDisplay *d,
                  CompAction *action,
                  CompActionState state,
                  CompOption *option,
                  int nOption);

/* event.c */

void FWHandleEvent (CompDisplay *d,
                   XEvent *ev);

void FWHandleEnterNotify (CompWindow *w,
                         XEvent *xev);

void FWHandleLeaveNotify (CompWindow *w,
                         XEvent *xev);

void FWHandleScaleMotionEvent (CompWindow *w,
                              float dx,
                              float dy,
                              int x,
                              int y);

void FWHandleRotateMotionEvent (CompWindow *w,
                               float dx,
                               float dy,
                               int x,
                               int y);

void FWHandleIPWResizeMotionEvent (CompWindow *w,
                                  unsigned int x,
                                  unsigned int y);

void FWHandleIPWMoveMotionEvent (CompWindow *w,
                                unsigned int x,
                                unsigned int y);

void FWHandleIPWMoveInitiate (CompWindow *w);

void FWHandleIPWResizeInitiate (CompWindow *w);

void FWHandleButtonReleaseEvent (CompWindow *w);

/* input.c */

/** Use FWHandleWindowInputInfo() instead
 *  of the functions following it to adjust
 *  input handling parameters correctly
 */

Bool FWHandleWindowInputInfo (CompWindow *w);

void FWAdjustIPWStacking (CompScreen *s);

void FWCreateIPW (CompWindow *w);

void FWAdjustIPW (CompWindow *w);

void FWRemoveWindowFromList (FWWindowInputInfo *info);

void FWAddWindowToList (FWWindowInputInfo *info);

void FWShapeInput (CompWindow *w);

void FWUnshapeInput (CompWindow *w);

void FWSaveInputShape (CompWindow *w,
                      XRectangle **retRects,
                      int        *retCount,
                      int        *retOrdering);
/* util.c */

CompWindow * FWGetRealWindow (CompWindow *w);

Bool FWCanShape (CompWindow *w);

void FWDetermineZAxisClick (CompWindow *w,
                           int px,
                           int py);

void FWCalculateOutputOrigin (CompWindow *w,
                             float x,
                             float y);

void FWCalculateInputOrigin (CompWindow *w,
                             float x,
                             float y);

/** Use FWCalculateInputRect() to store
 *  input rect data in the _FWWindow
 *  structure for the given CompWindow *
 */

void FWCalculateInputRect (CompWindow *w);

/** Use FWCalculateOutputRect() to store
 *  input rect data in the _FWWindow
 *  structure for the given CompWindow *
 */

void FWCalculateOutputRect (CompWindow *w);

Box FWCalculateWindowRect (CompWindow *w,
                           CompVector c1,
                           CompVector c2,
                           CompVector c3,
                           CompVector c4);

Box FWCreateSizedRect (float xScreen1,
                       float xScreen2,
                       float xScreen3,
                       float xScreen4,
                       float yScreen1,
                       float yScreen2,
                       float yScreen3,
                       float yScreen4);

void FWCreateMatrix  (CompWindow *w, CompTransform *mTransform,
                     float angX, float angY, float angZ,
                     float tX, float tY, float tZ,
                     float scX, float scY, float scZ);

void FWRotateProjectVector (CompWindow *w,
                           CompVector vector,
                           CompTransform transform,
                           GLdouble *resultX,
                           GLdouble *resultY,
                           GLdouble *resultZ,
                           Bool report);


