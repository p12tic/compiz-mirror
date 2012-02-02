#include "test-local-menu.h"
#include <cstring>

namespace
{
    void initializeMetaButtonLayout (MetaButtonLayout *layout)
    {
	memset (layout, 0, sizeof (MetaButtonLayout));

	unsigned int i;

	for (i = 0; i < MAX_BUTTONS_PER_CORNER; i++)
	{
	    layout->right_buttons[i] = META_BUTTON_FUNCTION_LAST;
	    layout->left_buttons[i] = META_BUTTON_FUNCTION_LAST;
	}
    }
}

class GtkWindowDecoratorTestLocalMenuLayout :
    public GtkWindowDecoratorTestLocalMenu
{
    public:

	MetaButtonLayout * getLayout () { return &mLayout; }

	virtual void SetUp ()
	{
	    GtkWindowDecoratorTestLocalMenu::SetUp ();
	    ::initializeMetaButtonLayout (&mLayout);
	    g_settings_set_boolean (getSettings (), "force-local-menus", TRUE);
	}

    private:

	MetaButtonLayout mLayout;
};

TEST_F (GtkWindowDecoratorTestLocalMenuLayout, TestForceNoButtonsSet)
{
    force_local_menus_on (getWindow (), getLayout ());

    EXPECT_EQ (getLayout ()->right_buttons[0], META_BUTTON_FUNCTION_LAST);
    EXPECT_EQ (getLayout ()->left_buttons[0], META_BUTTON_FUNCTION_LAST);
}

TEST_F (GtkWindowDecoratorTestLocalMenuLayout, TestForceNoCloseButtonSet)
{
    getLayout ()->right_buttons[0] = META_BUTTON_FUNCTION_CLOSE;

    force_local_menus_on (getWindow (), getLayout ());

    EXPECT_EQ (getLayout ()->right_buttons[1], META_BUTTON_FUNCTION_WINDOW_MENU);
    EXPECT_EQ (getLayout ()->left_buttons[0], META_BUTTON_FUNCTION_LAST);
}

TEST_F (GtkWindowDecoratorTestLocalMenuLayout, TestForceNoCloseMinimizeMaximizeButtonSet)
{
    getLayout ()->left_buttons[0] = META_BUTTON_FUNCTION_CLOSE;
    getLayout ()->left_buttons[1] = META_BUTTON_FUNCTION_CLOSE;
    getLayout ()->left_buttons[2] = META_BUTTON_FUNCTION_CLOSE;

    force_local_menus_on (getWindow (), getLayout ());

    EXPECT_EQ (getLayout ()->right_buttons[0], META_BUTTON_FUNCTION_LAST);
    EXPECT_EQ (getLayout ()->left_buttons[3], META_BUTTON_FUNCTION_WINDOW_MENU);
}
