#ifndef RESIZEDEFS_H
#define RESIZEDEFS_H

#define RESIZE_SCREEN(s) ResizeScreen *rs = ResizeScreen::get(s)
#define RESIZE_WINDOW(w) ResizeWindow *rw = ResizeWindow::get(w)

#define ResizeUpMask    (1L << 0)
#define ResizeDownMask  (1L << 1)
#define ResizeLeftMask  (1L << 2)
#define ResizeRightMask (1L << 3)

#define NUM_KEYS 4

#define MIN_KEY_WIDTH_INC  24
#define MIN_KEY_HEIGHT_INC 24

#endif /* RESIZEDEFS_H */
