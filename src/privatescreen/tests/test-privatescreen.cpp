#include "privatescreen.h"


// Get rid of stupid macro from X.h
// Why, oh why, are we including X.h?
#undef None

#include <gtest/gtest.h>
#include <gmock/gmock.h>


namespace {

class MockCompScreen : public AbstractCompScreen
{
public:
    // Interface hoisted from CompScreen
    MOCK_METHOD0(updateDefaultIcon, bool ());
    MOCK_METHOD0(dpy, Display * ());
    MOCK_METHOD0(root, Window ());
    MOCK_CONST_METHOD0(vpSize, const CompSize  & () );
    MOCK_METHOD1(forEachWindow, void (CompWindow::ForEach));
    MOCK_METHOD0(windows, CompWindowList & ());
    MOCK_METHOD3(moveViewport, void (int tx, int ty, bool sync));
    MOCK_CONST_METHOD0(vp,const CompPoint & ());
    MOCK_METHOD0(updateWorkarea, void ());
    MOCK_METHOD1(addAction, bool (CompAction *action));
    MOCK_METHOD1(findWindow, CompWindow * (Window id));

    MOCK_METHOD2(findTopLevelWindow, CompWindow * (
	    Window id, bool   override_redirect));
    MOCK_METHOD6(toolkitAction, void (
	    Atom   toolkitAction,
	    Time   eventTime,
	    Window window,
	    long   data0,
	    long   data1,
	    long   data2));
    MOCK_CONST_METHOD0(showingDesktopMask, unsigned int ());
    MOCK_CONST_METHOD0(grabsEmpty, bool ());
    MOCK_METHOD1(sizePluginClasses, void (unsigned int size));

    MOCK_METHOD1(_initPluginForScreen, bool (CompPlugin *));
    MOCK_METHOD1(_finiPluginForScreen, void (CompPlugin *));
    MOCK_METHOD3(_setOptionForPlugin, bool (const char *, const char *, CompOption::Value &));
    MOCK_METHOD1(_handleEvent, void (XEvent *event));
    MOCK_METHOD3(_logMessage, void (const char *, CompLogLevel, const char*));
    MOCK_METHOD0(_enterShowDesktopMode, void ());
    MOCK_METHOD1(_leaveShowDesktopMode, void (CompWindow *));
    MOCK_METHOD1(_addSupportedAtoms, void (std::vector<Atom>& atoms));

    MOCK_METHOD1(_fileWatchAdded, void (CompFileWatch *));
    MOCK_METHOD1(_fileWatchRemoved, void (CompFileWatch *));
    MOCK_METHOD2(_sessionEvent, void (CompSession::Event, CompOption::Vector &));
    MOCK_METHOD3(_handleCompizEvent, void (const char *, const char *, CompOption::Vector &));
    MOCK_METHOD4(_fileToImage, bool (CompString &, CompSize &, int &, void *&));
    MOCK_METHOD5(_imageToFile, bool (CompString &, CompString &, CompSize &, int, void *));
    MOCK_METHOD1(_matchInitExp, CompMatch::Expression * (const CompString&));
    MOCK_METHOD0(_matchExpHandlerChanged, void ());
    MOCK_METHOD1(_matchPropertyChanged, void (CompWindow *));
    MOCK_METHOD0(_outputChangeNotify, void ());
};


} // (abstract) namespace



TEST(PrivateScreenTest, dummy)
{
    using namespace testing;

    MockCompScreen comp_screen;

    PrivateScreen ps(&comp_screen);
}

