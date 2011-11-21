#define CompWindowGrabKeyMask         (1 << 0)
#define CompWindowGrabButtonMask      (1 << 1)
#define CompWindowGrabMoveMask        (1 << 2)
#define CompWindowGrabResizeMask      (1 << 3)
#define CompWindowGrabExternalAppMask (1 << 4)

namespace compiz
{
namespace grid
{
namespace window
{

class GrabWindowHandler
{
    public:

	GrabWindowHandler (unsigned int mask);
	~GrabWindowHandler ();

    	bool track ();
	bool resetResize ();

    private:

	unsigned int mMask;

};
}
}
}
