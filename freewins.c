#include <compiz-core.h>
#include <math.h>

#define ABS(x) ((x)>0?(x):-(x))
#define D2R(x) ((x) * M_PI / 180.0)

// Macros/*{{{*/
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

#define FREEWINS_OPTION_INITIATE_BUTTON 0
#define FREEWINS_OPTION_RESET_BUTTON 1
#define FREEWINS_OPTION_AXIS_TOGGLE_KEY 2
#define FREEWINS_OPTION_NUM 3

/*}}}*/

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
    
    CompOption opt[FREEWINS_OPTION_NUM];

    Bool axisHelp;

} FWDisplay;

typedef struct _FWScreen{
    int windowPrivateIndex;

    PaintOutputProc paintOutput;
    PaintWindowProc paintWindow;

    DamageWindowRectProc damageWindowRect;

    WindowResizeNotifyProc windowResizeNotify;

    CompOption opt[FREEWINS_OPTION_NUM];

    int grabIndex;
    int rotatedWindows;

} FWScreen;

typedef struct _FWWindow{
    float angX;
    float angY;
    float angZ;

    float midX;
    float midY;

    int oldX;
    int oldY;

    //Box rect;

    Bool grabbed;
    Bool zaxis;

    Bool grabLeft;
    Bool grabTop;

    Bool rotated;

} FWWindow;
/*}}}*/

int displayPrivateIndex;
static CompMetadata freewinsMetadata;

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

		if(fww->zaxis){
		    if(ABS(dy)>ABS(dx)){
			if(fww->grabLeft)
			    fww->angZ -= 360 * dy;
			else
			    fww->angZ += 360 * dy;
		    }else{
			if(fww->grabTop)
			    fww->angZ += 360 * dx;
			else
			    fww->angZ -= 360 * dx;
		    }
		}else{
		    fww->angX -= 360.0 * dy;
		    fww->angY += 360.0 * dx;
		}

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
		if(fww->angX != 0.0 || fww->angY != 0.0 || fww->angZ != 0.0){
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
	    /*}}}*/

	// Button press / release/*{{{*/
	case ButtonPress:
	    fwd->click_root_x = ev->xbutton.x_root;
	    fwd->click_root_y = ev->xbutton.y_root;
	    fwd->click_win_x = ev->xbutton.x;
	    fwd->click_win_y = ev->xbutton.y;
	    break;

	case ButtonRelease:
	    if(fwd->grabWindow){
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
	/*}}}*/

	case FocusOut:
	    break;

	case FocusIn:
	    if(ev->xfocus.mode != NotifyGrab)
		fwd->focusWindow = findWindowAtDisplay(d, ev->xfocus.window);
	    break;

	default:
	    break;
    }

    UNWRAP(fwd, d, handleEvent);
    (*d->handleEvent)(d, ev);
    WRAP(fwd, d, handleEvent, FWHandleEvent);
}
/*}}}*/

// Paint Window/*{{{*/
static Bool FWPaintWindow(CompWindow *w, const WindowPaintAttrib *attrib, 
	const CompTransform *transform, Region region, unsigned int mask){

    CompTransform wTransform = *transform;
    Bool wasCulled = glIsEnabled(GL_CULL_FACE);
    Bool status;

    FREEWINS_SCREEN(w->screen);
    FREEWINS_WINDOW(w);

    if(fww->angX != 0.0 || fww->angY != 0.0 || fww->angZ != 0.0){

	mask |= PAINT_WINDOW_TRANSFORMED_MASK;

	// Ajustar espesor de la ventana
	matrixScale (&wTransform, 1.0f, 1.0f, 1.0f / w->screen->width);

	matrixTranslate(&wTransform, 
		WIN_REAL_X(w) + WIN_REAL_W(w)/2.0, 
		WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0, 0.0);

	matrixRotate(&wTransform, fww->angX, 1.0, 0.0, 0.0);
	matrixRotate(&wTransform, fww->angY, 0.0, 1.0, 0.0);
	matrixRotate(&wTransform, fww->angZ, 0.0, 0.0, 1.0);

	matrixTranslate(&wTransform, 
		-(WIN_REAL_X(w) + WIN_REAL_W(w)/2.0), 
		-(WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0), 0.0);
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

	glColor4f (0.6, 0.6, 1.0, 0.8f);
	glEnable(GL_BLEND);

	glBegin(GL_POLYGON);
	for(j=0; j<360; j += 10)
	    glVertex3f( x + 100 * cos(D2R(j)), y + 100 * sin(D2R(j)), 0.0 );
	glEnd ();

	glDisable(GL_BLEND);
	glColor4f (0.6, 0.6, 1.0, 1.0f);
	glLineWidth(3.0);

	glBegin(GL_LINE_LOOP);
	for(j=360; j>=0; j -= 10)
	    glVertex3f( x + 100 * cos(D2R(j)), y + 100 * sin(D2R(j)), 0.0 );
	glEnd ();

	if(wasCulled)
	    glEnable(GL_CULL_FACE);

	glColor4usv(defaultColor);
	glPopMatrix ();
    }

/*}}}*/

    return status;
}
/*}}}*/

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
	
	if(!otherScreenGrabExist(s, "freewins", 0))
	    if(!fws->grabIndex)
		fws->grabIndex = pushScreenGrab(s, None, "freewins");
    }
    
    if(w){
	FREEWINS_WINDOW(w);
	
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

	if( (dx>dy?dx:dy) > 100 )
	    fww->zaxis = TRUE;
	else
	    fww->zaxis = FALSE;
    }
    
    return TRUE;
}
/*}}}*/

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

	fww->angX = 0.0;
	fww->angY = 0.0;
	fww->angZ = 0.0;
    }
    
    return TRUE;
}
/*}}}*/

// Toggle Axis /*{{{*/
static Bool toggleFWAxis (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption){

    FREEWINS_DISPLAY(d);

    fwd->axisHelp = !fwd->axisHelp;

    return TRUE;
}
/*}}}*/

// Display Option/*{{{*/
static CompOption* freewinsGetDisplayOptions (CompPlugin *plugin, 
	CompDisplay *display, int *count) {

    FREEWINS_DISPLAY (display);

    *count = FREEWINS_OPTION_NUM;
    return fwd->opt;
}

static Bool freewinsSetDisplayOption (CompPlugin *plugin, CompDisplay *display, 
	const char *name, CompOptionValue *value) {

    CompOption *o;
    int index;

    FREEWINS_DISPLAY (display);

    if( !(o = compFindOption (fwd->opt, FREEWINS_OPTION_NUM, name, &index)) )
	return FALSE;

    return compSetDisplayOption (display, o, value);
}
/*}}}*/

// Object Options/*{{{*/
static CompOption* freewinsGetObjectOptions (CompPlugin *plugin, 
	CompObject *object, int *count) {
    
    static GetPluginObjectOptionsProc dispTab[] = {
	(GetPluginObjectOptionsProc) 0, // GetCoreOptions
	(GetPluginObjectOptionsProc) freewinsGetDisplayOptions
    };

    RETURN_DISPATCH (object, dispTab, ARRAY_SIZE (dispTab),
             (void *) (*count = 0), (plugin, object, count));
}

static CompBool freewinsSetObjectOption (CompPlugin *plugin, CompObject *object, 
	const char *name, CompOptionValue *value) {
    
    static SetPluginObjectOptionProc dispTab[] = {
	(SetPluginObjectOptionProc) 0, /* SetCoreOption */
	(SetPluginObjectOptionProc) freewinsSetDisplayOption
    };
    
    RETURN_DISPATCH (object, dispTab, ARRAY_SIZE (dispTab), 
	FALSE, (plugin, object, name, value));
}
/*}}}*/

static const CompMetadataOptionInfo freewinsOptionInfo[] = {
	{ "initiate_button", "button", 0, initiateFWRotate, 0 /* terminateFWRotate */ },
	{ "reset_button", "button", 0, resetFWRotation, 0},
	{ "toggle_axis", "key", 0, toggleFWAxis, 0}
};

static CompMetadata* freewinsGetMetadata(CompPlugin* plugin){
    return &freewinsMetadata;
}

// Window init / clean/*{{{*/
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

    w->base.privates[fws->windowPrivateIndex].ptr = fww;

    return TRUE;
}

static void freewinsFiniWindow(CompPlugin *p, CompWindow *w){

    FREEWINS_WINDOW(w);
    FREEWINS_DISPLAY(w->screen->display);

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

    s->base.privates[fwd->screenPrivateIndex].ptr = fws;
    
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
    
    // inicializar cosas particulares del plugin.
    fwd->grabWindow = 0;
    fwd->axisHelp = FALSE;
    fwd->focusWindow = 0;
     
    if( (fwd->screenPrivateIndex = allocateScreenPrivateIndex(d)) < 0 ){
	free(fwd);
	return FALSE;
    }

    if( !compInitDisplayOptionsFromMetadata (d, &freewinsMetadata, 
	    freewinsOptionInfo, fwd->opt, FREEWINS_OPTION_NUM) ){
	
	free (fwd);
	return FALSE;
    }

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

// Object init / clean/*{{{*/
static CompBool freewinsInitObject(CompPlugin *p, CompObject *o){

    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, // InitCore
	(InitPluginObjectProc) freewinsInitDisplay,
	(InitPluginObjectProc) freewinsInitScreen,
	(InitPluginObjectProc) freewinsInitWindow
    };

    RETURN_DISPATCH(o, dispTab, ARRAY_SIZE(dispTab), TRUE, (p, o));
}

static void freewinsFiniObject(CompPlugin *p, CompObject *o){

    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, // FiniCore
	(FiniPluginObjectProc) freewinsFiniDisplay,
	(FiniPluginObjectProc) freewinsFiniScreen,
	(FiniPluginObjectProc) freewinsFiniWindow
    };

    DISPATCH(o, dispTab, ARRAY_SIZE(dispTab), (p, o));
}
/*}}}*/

// Plugin init / clean/*{{{*/
static Bool freewinsInit(CompPlugin *p){
    
    if( !compInitPluginMetadataFromInfo (&freewinsMetadata, p->vTable->name, 
		freewinsOptionInfo, FREEWINS_OPTION_NUM, 0, 0) )
	return FALSE;
    
    if( (displayPrivateIndex = allocateDisplayPrivateIndex()) < 0 )
	return FALSE;

	compAddMetadataFromFile (&freewinsMetadata, p->vTable->name);

    return TRUE;
}


static void freewinsFini(CompPlugin *p){

    if(displayPrivateIndex >= 0)
	freeDisplayPrivateIndex( displayPrivateIndex );
}
/*}}}*/

// Plugin implementation export/*{{{*/
CompPluginVTable freewinsVTable = {
    "freewins",
    freewinsGetMetadata,
    freewinsInit,
    freewinsFini,
    freewinsInitObject,
    freewinsFiniObject,
    freewinsGetObjectOptions,
    freewinsSetObjectOption
};

CompPluginVTable *getCompPluginInfo20070830 (void){ return &freewinsVTable; }
/*}}}*/
