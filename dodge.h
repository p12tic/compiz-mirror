void fxDodgeProcessSubject (CompWindow *wCur, Region wRegion,
							Region dodgeRegion, Bool alwaysInclude);
void fxDodgeFindDodgeBox (CompWindow *w, XRectangle *dodgeBox);
void fxDodgePostPreparePaintScreen (CompScreen *s,CompWindow *w);
void fxDodgeUpdateWindowTransform (CompScreen *s,CompWindow *w,CompTransform *wTransform);
void fxDodgeAnimStep (CompScreen *s,CompWindow *w,float time);
