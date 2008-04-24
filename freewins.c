#include "freewins.h"

static CompMetadata freewinsMetadata;

/* Information on window resize */
void FWWindowResizeNotify(CompWindow *w, int dx, int dy, int dw, int dh)
{
    FREEWINS_WINDOW(w);
    FREEWINS_SCREEN(w->screen);

    fww->iMidX += dw;
    fww->iMidY += dh;

    fww->winH += dh;
    fww->winW += dw;

	int x = WIN_REAL_X(w) + WIN_REAL_W(w)/2.0;
	int y = WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0;

    fww->radius = sqrt(pow((x - WIN_REAL_X (w)), 2) + pow((y - WIN_REAL_Y (w)), 2));

    /*if (FWCanShape (w))
        FWShapeInput (w);*/

    UNWRAP(fws, w->screen, windowResizeNotify);
    (*w->screen->windowResizeNotify)(w, dx, dy, dw, dh);
    WRAP(fws, w->screen, windowResizeNotify, FWWindowResizeNotify);
}

void
FWWindowMoveNotify (CompWindow *w,
		       int        dx,
		       int        dy,
		       Bool       immediate)
{
    FREEWINS_SCREEN (w->screen);
    FREEWINS_WINDOW (w);

    CompWindow *useWindow;

    useWindow = FWGetRealWindow (w); /* Did we move an IPW and not the actual window? */
    if (useWindow)
        moveWindow (useWindow, dx, dy, TRUE, freewinsGetImmediateMoves (w->screen));
    else
        FWAdjustIPW (w); /* We moved a window but not the IPW, so adjust it */

	int x = WIN_REAL_X(w) + WIN_REAL_W(w)/2.0;
	int y = WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0;

    fww->radius = sqrt(pow((x - WIN_REAL_X (w)), 2) + pow((y - WIN_REAL_Y (w)), 2));

    UNWRAP (fws, w->screen, windowMoveNotify);
    (*w->screen->windowMoveNotify) (w, dx, dy, immediate);
    WRAP (fws, w->screen, windowMoveNotify, FWWindowMoveNotify);
}

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


/* ------ Plugin Initialisation ---------------------------------------*/

/* Window initialisation / cleaning */
static Bool freewinsInitWindow(CompPlugin *p, CompWindow *w){
    FWWindow *fww;
    FREEWINS_SCREEN(w->screen);
    FREEWINS_DISPLAY(w->screen->display);

    if( !(fww = (FWWindow*)malloc( sizeof(FWWindow) )) )
	return FALSE;

    fww->transform.angX = 0.0;
    fww->transform.angY = 0.0;
    fww->transform.angZ = 0.0;

    fww->iMidX = WIN_REAL_W(w)/2.0;
    fww->iMidY = WIN_REAL_H(w)/2.0;

	int x = WIN_REAL_X(w) + WIN_REAL_W(w)/2.0;
	int y = WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0;

    fww->radius = sqrt(pow((x - WIN_REAL_X (w)), 2) + pow((y - WIN_REAL_Y (w)), 2));

    fww->outputRect.x1 = WIN_OUTPUT_X (w);
    fww->outputRect.x2 = WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w);
    fww->outputRect.y1 = WIN_OUTPUT_Y (w);
    fww->outputRect.y2 = WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w);

    fwd->grab = grabNone;
    fww->can2D = FALSE;
    fww->can3D = FALSE;

    fww->rotated = FALSE;
    
    // Don't allow anything yet
    fww->allowScaling = FALSE;
    fww->allowRotation = FALSE;
    fww->doAnimate = FALSE;
    fww->resetting = FALSE;
    
    // Don't allow incorrect window drawing as soon as the plugin is started
    
    fww->transform.scaleX = 1.0;
    fww->transform.scaleY = 1.0;
    
    fww->transform.unsnapScaleX = 1.0;
    fww->transform.unsnapScaleY = 1.0;
    
    fww->animate.aTimeRemaining = freewinsGetResetTime (w->screen);
    fww->animate.cTimeRemaining = freewinsGetResetTime (w->screen);

    w->base.privates[fws->windowPrivateIndex].ptr = fww;
    fww->input = NULL;
    
    // Shape window back to normal
    /*if (FWCanShape (w))
        FWShapeInput (w); - disabled as it causes problems*/

    return TRUE;
}

static void freewinsFiniWindow(CompPlugin *p, CompWindow *w){

    FREEWINS_WINDOW(w);
    FREEWINS_DISPLAY(w->screen->display);
    
    /* Shape window back to normal */
    fww->transform.scaleX = 1.0f;
    fww->transform.scaleY = 1.0f;

    fww->rotated = FALSE;
    
    if (FWCanShape (w))
        FWHandleWindowInputInfo (w);

    if(fwd->grabWindow == w){
	fwd->grabWindow = NULL;
    }

   free(fww); 
}

/* Screen initialization / cleaning */
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
    fws->transformedWindows = NULL;

    s->base.privates[fwd->screenPrivateIndex].ptr = fws;
    
    WRAP(fws, s, preparePaintScreen, FWPreparePaintScreen);
    WRAP(fws, s, paintWindow, FWPaintWindow);
    WRAP(fws, s, paintOutput, FWPaintOutput);

    WRAP(fws, s, damageWindowRect, FWDamageWindowRect);

    WRAP(fws, s, windowResizeNotify, FWWindowResizeNotify);
    WRAP(fws, s, windowMoveNotify, FWWindowMoveNotify);

    return TRUE;
}

static void freewinsFiniScreen(CompPlugin *p, CompScreen *s){

    FREEWINS_SCREEN(s);

    freeWindowPrivateIndex(s, fws->windowPrivateIndex);

    UNWRAP(fws, s, preparePaintScreen);
    UNWRAP(fws, s, paintWindow);
    UNWRAP(fws, s, paintOutput);

    UNWRAP(fws, s, damageWindowRect);

    UNWRAP(fws, s, windowResizeNotify);
    UNWRAP(fws, s, windowMoveNotify);
    free(fws);
}

/* Display initialization / cleaning */
static Bool freewinsInitDisplay(CompPlugin *p, CompDisplay *d){

    FWDisplay *fwd; 

    if( !(fwd = (FWDisplay*)malloc( sizeof(FWDisplay) )) )
	return FALSE;
    
    // Set variables correctly
    fwd->grabWindow = 0;
    fwd->lastGrabWindow = 0;
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
    freewinsSetInitiateRotationButtonInitiate(d, initiateFWRotate);
    freewinsSetInitiateScaleButtonInitiate(d, initiateFWScale);
    freewinsSetResetButtonInitiate(d, resetFWRotation);
    freewinsSetResetKeyInitiate(d, resetFWRotation);
    freewinsSetToggleAxisKeyInitiate(d, toggleFWAxis);
    
    // Rotate / Scale Up Down Left Right

    freewinsSetScaleUpButtonInitiate(d, FWScaleUp);
    freewinsSetScaleDownButtonInitiate(d, FWScaleDown);
    freewinsSetScaleUpKeyInitiate(d, FWScaleUp);
    freewinsSetScaleDownKeyInitiate(d, FWScaleDown);

    freewinsSetRotateUpKeyInitiate(d, FWRotateUp);
    freewinsSetRotateDownKeyInitiate(d, FWRotateDown);
    freewinsSetRotateLeftKeyInitiate(d, FWRotateLeft);
    freewinsSetRotateRightKeyInitiate(d, FWRotateRight);
    freewinsSetRotateCKeyInitiate(d, FWRotateClockwise);
    freewinsSetRotateCcKeyInitiate(d, FWRotateCounterclockwise);

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

/* Object Initiation and Finitialization */

static CompBool
freewinsInitObject (CompPlugin *p,
		     CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0,
	(InitPluginObjectProc) freewinsInitDisplay,
	(InitPluginObjectProc) freewinsInitScreen,
	(InitPluginObjectProc) freewinsInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
freewinsFiniObject (CompPlugin *p,
		     CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0,
	(FiniPluginObjectProc) freewinsFiniDisplay,
	(FiniPluginObjectProc) freewinsFiniScreen,
	(FiniPluginObjectProc) freewinsFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}


/* Plugin initialization / cleaning */
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

/* Plugin implementation export */
CompPluginVTable freewinsVTable = {
    "freewins",
    0,
    freewinsInit,
    freewinsFini,
    freewinsInitObject,
    freewinsFiniObject,
    0,
    0,
};

CompPluginVTable *getCompPluginInfo (void){ return &freewinsVTable; }

