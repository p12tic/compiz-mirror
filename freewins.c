#include <compiz-core.h>

#define ABS(x) ((x)>0?(x):-(x))

int displayPrivateIndex;

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

#define FREEWINS_OPTION_INITIATEZ_BUTTON 0
#define FREEWINS_OPTION_INITIATEX_BUTTON 1
#define FREEWINS_OPTION_RESET_BUTTON 2
#define FREEWINS_OPTION_NUM 3

/*}}}*/

static CompMetadata freewinsMetadata;

// Structures /*{{{*/
typedef struct _FWDisplay{
    int screenPrivateIndex;
	int boundAction;
	int ClickX;
	int ClickY;

    HandleEventProc handleEvent;

    CompWindow *grabWindow;
	Window clickWindow;

	CompOption opt[FREEWINS_OPTION_NUM];

} FWDisplay;

typedef struct _FWScreen{
    int windowPrivateIndex;

    PreparePaintScreenProc preparePaintScreen;
    DonePaintScreenProc donePaintScreen;
    PaintOutputProc paintOutput;
    PaintWindowProc paintWindow;

    PaintTransformedOutputProc paintTransformedOutput;

    WindowGrabNotifyProc windowGrabNotify;
    WindowUngrabNotifyProc windowUngrabNotify;

	CompOption opt[FREEWINS_OPTION_NUM];

    int grabIndex;

} FWScreen;

typedef struct _FWWindow{
    float angX;
    float angY;
    float angZ;

    // Agregar resize notify
    float midX;
    float midY;

    int oldX;
    int oldY;

    Bool grabbed;
    Bool zaxis;


} FWWindow;
/*}}}*/

static void FWHandleEvent(CompDisplay *d, XEvent *ev){

    FREEWINS_DISPLAY(d);
    CompWindow *w;
    CompScreen *s;
    float dx, dy;

    switch(ev->type){
	
	case MotionNotify:
	    
		//fprintf(stderr, "MotionNotify\n");
	    if(fwd->grabWindow){
		//fprintf(stderr, "Moved\n");
		FREEWINS_WINDOW(fwd->grabWindow);

		dx = (float)(ev->xmotion.x_root - fww->oldX) / fwd->grabWindow->screen->width;
		dy = (float)(ev->xmotion.y_root - fww->oldY) / fwd->grabWindow->screen->height;
		
		if(fww->zaxis){
		    if(ABS(dy)>ABS(dx)){
			    fww->angZ -= 360 * dy;
			    fww->angZ += 360 * dx;
		    }
		}else{
		    fww->angX -= 360.0 * dy;
		    fww->angY += 360.0 * dx;
		}

		fww->oldX = ev->xmotion.x_root;
		fww->oldY = ev->xmotion.y_root;
	    }
	    break;
	    
	case ButtonPress:
	    
		//fprintf(stderr, "ButtonPress\n");
		//fprintf(stderr, "%d %d\n", ev->xbutton.x_root, ev->xbutton.y_root);

		w = findWindowAtDisplay(d, ev->xbutton.window);

		if(w){
		    fwd->ClickX = ev->xbutton.x_root;
		    fwd->ClickY = ev->xbutton.y_root;
			//fprintf(stderr, "%d %d\n", fwd->ClickX, fwd->ClickY);
		}

	    break;
	case ButtonRelease:

		//fprintf(stderr, "ButtonRelese\n");
	    for(s = d->screens; s; s = s->next){
		FREEWINS_SCREEN(s);

		if(fws->grabIndex){
		    removeScreenGrab(s, fws->grabIndex, 0);
		    fws->grabIndex = 0;
		}
	    }

	    if(fwd->grabWindow){
		//fprintf(stderr, "Released\n");
		FREEWINS_WINDOW(fwd->grabWindow);
		fww->grabbed = FALSE;
		fwd->grabWindow = 0;
	    }

		fwd->boundAction = -1;

	    break;

	default:
	    break;
    }

    UNWRAP(fwd, d, handleEvent);
    (*d->handleEvent)(d, ev);
    WRAP(fwd, d, handleEvent, FWHandleEvent);
}

static void FWDonePaintScreen(CompScreen *s){

    FREEWINS_SCREEN(s);

    damageScreen(s);

    UNWRAP(fws, s, donePaintScreen);
    (*s->donePaintScreen)(s);
    WRAP(fws, s, donePaintScreen, FWDonePaintScreen);
}

static Bool FWPaintWindow(CompWindow *w, const WindowPaintAttrib *attrib, 
	const CompTransform *transform, Region region, unsigned int mask){

    CompTransform wTransform = *transform;
    Bool wasCulled = glIsEnabled(GL_CULL_FACE);
    Bool status;

    FREEWINS_SCREEN(w->screen);
    FREEWINS_WINDOW(w);

    if(fww->angX != 0.0 || fww->angY != 0.0 || fww->angZ != 0.0){

	mask |= PAINT_WINDOW_TRANSFORMED_MASK;

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

static Bool FWPaintOutput(CompScreen *s, const ScreenPaintAttrib *sAttrib, 
	const CompTransform *transform, Region region, CompOutput *output, unsigned int mask){

    Bool status;

    FREEWINS_SCREEN(s);

    //if(fws->rotatedWindows)
    	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    UNWRAP(fws, s, paintOutput);
    status = (*s->paintOutput)(s, sAttrib, transform, region, output, mask);
    WRAP(fws, s, paintOutput, FWPaintOutput);

    return status;
}

/*
static void FWPaintTransformedOutput(CompScreen *s, const ScreenPaintAttrib *sAttrib, 
	const CompTransform *transform, Region region, CompOutput *output, unsigned int mask){

    FREEWINS_SCREEN(s);


    UNWRAP(fws, s, paintTransformedOutput);
    (*s->paintTransformedOutput)(s, sAttrib, transform, region, output, mask);
    WRAP(fws, s, paintTransformedOutput, FWPaintTransformedOutput);

    if(wasCulled)
	glEnabled(GL_CULL_FACE);
}
*/

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

static Bool
initiateFWRotateZ (CompDisplay     *d,
         CompAction      *action,
         CompActionState state,
         CompOption      *option,
         int         nOption)
{
	//fprintf(stderr, "Initiate Freewins Rotation Keybinding\n");
	
	CompWindow* w;
	CompScreen* s;
	Window xid;

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
		//fprintf(stderr, "Grabbed\n");
		FREEWINS_WINDOW(w);

		fwd->grabWindow = w;

		fww->grabbed = TRUE;

		fww->oldX = fwd->ClickX;
		fww->oldY = fwd->ClickY;

		fww->zaxis = TRUE;
	}

	return TRUE;
}

static Bool
initiateFWRotateX (CompDisplay     *d,
         CompAction      *action,
         CompActionState state,
         CompOption      *option,
         int         nOption)
{
	//fprintf(stderr, "Initiate Freewins Rotation Keybinding\n");
	
	CompWindow* w;
	CompScreen* s;
	Window xid;

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
		//fprintf(stderr, "Grabbed\n");
		FREEWINS_WINDOW(w);

		fwd->grabWindow = w;

		fww->grabbed = TRUE;

		fww->oldX = fwd->ClickX;
		fww->oldY = fwd->ClickY;

		fww->zaxis = FALSE;
	}

	return TRUE;
}

static Bool
terminateFWRotate (CompDisplay     *d,
         CompAction      *action,
         CompActionState state,
         CompOption      *option,
         int         nOption)
{
	//fprintf(stderr, "Terminate Freewins Rotation Keybinding\n");

	return TRUE;
}

static Bool
resetFWRotation (CompDisplay     *d,
         CompAction      *action,
         CompActionState state,
         CompOption      *option,
         int         nOption)
{
	//fprintf(stderr, "Reset Freewins Rotation Keybinding\n");
	CompWindow* w;
	CompScreen* s;
	Window xid;

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
	    fww->grabbed = FALSE;
/*
		fww->oldX = fww->oldX;
		fww->oldY = fww->oldY;
*/		
		fww->angX = 0;
		fww->angY = 0;
		fww->angZ = 0;
	}
	return TRUE;
}

static const CompMetadataOptionInfo freewinsOptionInfo[] = {
	{ "initiatez_button", "button", 0, initiateFWRotateZ, terminateFWRotate },
	{ "initiatex_button", "button", 0, initiateFWRotateX, terminateFWRotate },
	{ "reset_button", "button", 0, resetFWRotation, 0}
};

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

    s->base.privates[fwd->screenPrivateIndex].ptr = fws;
    
    WRAP(fws, s, donePaintScreen, FWDonePaintScreen);

    WRAP(fws, s, paintOutput, FWPaintOutput);
    WRAP(fws, s, paintWindow, FWPaintWindow);

//    WRAP(fws, s, paintTransformedOutput, FWPaintTransformedOutput);

    return TRUE;
}

static void freewinsFiniScreen(CompPlugin *p, CompScreen *s){

    FREEWINS_SCREEN(s);

    freeWindowPrivateIndex(s, fws->windowPrivateIndex);

 //   UNWRAP(fws, s, paintTransformedOutput);
    UNWRAP(fws, s, donePaintScreen);
    UNWRAP(fws, s, paintWindow);
    UNWRAP(fws, s, paintOutput);

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
    fwd->boundAction = -1;
     
    if( (fwd->screenPrivateIndex = allocateScreenPrivateIndex(d)) < 0 ){
	free(fwd);
	return FALSE;
    }

    if (!compInitDisplayOptionsFromMetadata (d,
                         &freewinsMetadata,
                         freewinsOptionInfo,
                         fwd->opt,
                         FREEWINS_OPTION_NUM))
    {
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
	if (!compInitPluginMetadataFromInfo (&freewinsMetadata,
                     p->vTable->name,
                     freewinsOptionInfo,
                     FREEWINS_OPTION_NUM,
					 0,
					 0))
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

static CompOption *
freewinsGetDisplayOptions (CompPlugin  *plugin,
               CompDisplay *display,
               int     *count)
{
    FREEWINS_DISPLAY (display);

    *count = FREEWINS_OPTION_NUM;
    return fwd->opt;
}

static Bool
freewinsSetDisplayOption (CompPlugin      *plugin,
              CompDisplay     *display,
              const char      *name,
              CompOptionValue *value)
{
    CompOption *o;
    int        index;

    FREEWINS_DISPLAY (display);

    o = compFindOption (fwd->opt, FREEWINS_OPTION_NUM, name, &index);
    if (!o)
    return FALSE;

    return compSetDisplayOption (display, o, value);

    return FALSE;
}

static CompOption *
freewinsGetObjectOptions (CompPlugin *plugin,
              CompObject *object,
              int    *count)
{
    static GetPluginObjectOptionsProc dispTab[] = {
    (GetPluginObjectOptionsProc) 0, /* GetCoreOptions */
    (GetPluginObjectOptionsProc) freewinsGetDisplayOptions
    };

    RETURN_DISPATCH (object, dispTab, ARRAY_SIZE (dispTab),
             (void *) (*count = 0), (plugin, object, count));
}

static CompBool
freewinsSetObjectOption (CompPlugin      *plugin,
             CompObject      *object,
             const char      *name,
             CompOptionValue *value)
{
    static SetPluginObjectOptionProc dispTab[] = {
    (SetPluginObjectOptionProc) 0, /* SetCoreOption */
    (SetPluginObjectOptionProc) freewinsSetDisplayOption
    };

    RETURN_DISPATCH (object, dispTab, ARRAY_SIZE (dispTab), FALSE,
             (plugin, object, name, value));
}

static CompMetadata*
freewinsGetMetadata(CompPlugin* plugin)
{
	return &freewinsMetadata;
}

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
