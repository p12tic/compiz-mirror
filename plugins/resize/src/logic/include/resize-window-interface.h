#ifndef RESIZE_RESIZE_WINDOW_INTERFACE_H
#define RESIZE_RESIZE_WINDOW_INTERFACE_H

namespace resize
{

class ResizeWindowInterface
{
    public:
	virtual ~ResizeWindowInterface () {}
	virtual void getStretchScale (BoxPtr pBox, float *xScale, float *yScale) = 0;
};

} /* namespace resize */

#endif /* RESIZE_RESIZE_WINDOW_INTERFACE_H */
