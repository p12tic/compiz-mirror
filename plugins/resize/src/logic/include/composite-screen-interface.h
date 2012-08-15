#ifndef RESIZE_COMPOSITE_SCREEN_INTERFACE
#define RESIZE_COMPOSITE_SCREEN_INTERFACE

namespace resize
{

class CompositeScreenInterface
{
    public:
	virtual ~CompositeScreenInterface () {}
	virtual bool compositingActive () = 0;
	virtual void damageRegion (const CompRegion &r) = 0;
};

} /* namespace resize */

#endif /* RESIZE_COMPOSITE_SCREEN_INTERFACE */
