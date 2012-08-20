#include <cstring>
#include <cstdio>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <glib-object.h>

#include "gwd-settings-interface.h"
#include "gwd-settings.h"
#include "gwd-settings-writable-interface.h"

#include "decoration.h"

#include "compiz_gwd_mock_settings.h"
#include "compiz_gwd_mock_settings_writable.h"

using ::testing::TestWithParam;
using ::testing::Eq;
using ::testing::Return;
using ::testing::MatcherInterface;
using ::testing::MakeMatcher;
using ::testing::MatchResultListener;
using ::testing::Matcher;
using ::testing::IsNull;
using ::testing::Values;
using ::testing::_;

template <class ValueCType>
class GValueCmp
{
    public:

	typedef ValueCType (*GetFunc) (const GValue *value);

	bool compare (const ValueCType &val,
		      GValue           *value,
		      GetFunc	       get)
	{
	    const ValueCType &valForValue = (*get) (value);
	    return valForValue == val;
	}
};

template <>
class GValueCmp <decor_shadow_options_t>
{
    public:

	typedef gpointer (*GetFunc) (const GValue *value);

	bool compare (const decor_shadow_options_t &val,
		      GValue			   *value,
		      GetFunc			   get)
	{
	    gpointer shadowOptionsPtr = (*get) (value);
	    const decor_shadow_options_t &shadowOptions = *(reinterpret_cast <decor_shadow_options_t *> (shadowOptionsPtr));
	    if (decor_shadow_options_cmp (&val, &shadowOptions))
		return true;
	    else
		return false;
	}
};

template <>
class GValueCmp <std::string>
{
    public:

	typedef const gchar * (*GetFunc) (const GValue *value);

	bool compare (const std::string &val,
		      GValue	        *value,
		      GetFunc		get)
	{
	    const gchar *valueForValue = (*get) (value);
	    const std::string valueForValueStr (valueForValue);\

	    return val == valueForValueStr;
	}
};

/*
template <class ValueCType>
class GValueCmpGetWrapper
{
    public:

	bool compare (const ValueCType &val,
		      GValue           *value)
	{
	    return GValueCmp<ValueCType> ().compare (val, value, g_value_get_pointer);
	}
};

template <>
class GValueCmpGetWrapper <int *>
{
    public:

	static const int * getPointerWrapper (const GValue *v)
	{
	    return reinterpret_cast <const int *> (g_value_get_pointer (v));
	}

	bool compare (const int        *val,
		      GValue           *value)
	{
	    return GValueCmp<const int *> ().compare (val, value, GValueCmpGetWrapper::getPointerWrapper);
	}
};

template <>
class GValueCmpGetWrapper <bool>
{
    public:

	bool compare (const bool       &val,
		      GValue           *value)
	{
	    return GValueCmp<gboolean> ().compare (val, value, g_value_get_boolean);
	}
};


template <>
class GValueCmpGetWrapper <gint>
{
    public:

	bool compare (const gint       &val,
		      GValue           *value)
	{
	    return GValueCmp<gint> ().compare (val, value, g_value_get_int);
	}
};


template <>
class GValueCmpGetWrapper <std::string>
{
    public:

	bool compare (const std::string &val,
		      GValue            *value)
	{
	    return GValueCmp<const gchar *> ().compare (val.c_str (), value, g_value_get_string);
	}
};
*/

namespace
{
    std::ostream &
    operator<< (std::ostream &os, const decor_shadow_options_t &options)
    {
	os << " radius: " << options.shadow_radius <<
	      " opacity: " << options.shadow_opacity <<
	      " offset: (" << options.shadow_offset_x << ", " << options.shadow_offset_y << ")" <<
	      " color: r: " << options.shadow_color[0] <<
	      " g: " << options.shadow_color[1] <<
	      " b: " << options.shadow_color[2];

	return os;
    }
}

template <class ValueCType>
class GObjectPropertyMatcher :
    public ::testing::MatcherInterface <GValue *>
{
    public:

	GObjectPropertyMatcher (const ValueCType			&value,
				typename GValueCmp<ValueCType>::GetFunc	func) :
	    mValue (value),
	    mGetFunc (func)
	{
	}

	virtual bool MatchAndExplain (GValue *value, MatchResultListener *listener) const
	{
	    return GValueCmp <ValueCType> ().compare (mValue, value, mGetFunc);
	}

	virtual void DescribeTo (std::ostream *os) const
	{
	    *os << "value contains " << mValue;
	}

	virtual void DescribeNegationTo (std::ostream *os) const
	{
	    *os << "value does not contain " << mValue;
	}

    private:

	const ValueCType &mValue;
	typename GValueCmp<ValueCType>::GetFunc mGetFunc;
};

MATCHER (GBooleanTrue, "gboolean TRUE")
{
    if (arg)
	return true;
    else
	return false;
}

MATCHER (GBooleanFalse, "gboolean FALSE")
{
    if (!arg)
	return true;
    else
	return false;
}

namespace testing_values
{
    const gdouble ACTIVE_SHADOW_OPACITY_VALUE = 1.0;
    const gdouble ACTIVE_SHADOW_RADIUS_VALUE = 2.0;
    const gdouble ACTIVE_SHADOW_OFFSET_X_VALUE = 3.0;
    const gint    ACTIVE_SHADOW_OFFSET_X_INT_VALUE = ACTIVE_SHADOW_OFFSET_X_VALUE;
    const gdouble ACTIVE_SHADOW_OFFSET_Y_VALUE = 4.0;
    const gint    ACTIVE_SHADOW_OFFSET_Y_INT_VALUE = ACTIVE_SHADOW_OFFSET_Y_VALUE;
    const std::string ACTIVE_SHADOW_COLOR_STR_VALUE ("#ffffffff");
    const gushort ACTIVE_SHADOW_COLOR_VALUE[] = { 255, 255, 255 };
    const gdouble INACTIVE_SHADOW_OPACITY_VALUE = 5.0;
    const gdouble INACTIVE_SHADOW_RADIUS_VALUE = 6.0;
    const gdouble INACTIVE_SHADOW_OFFSET_X_VALUE = 7.0;
    const gint    INACTIVE_SHADOW_OFFSET_X_INT_VALUE = INACTIVE_SHADOW_OFFSET_X_VALUE;
    const gdouble INACTIVE_SHADOW_OFFSET_Y_VALUE = 8.0;
    const gint    INACTIVE_SHADOW_OFFSET_Y_INT_VALUE = INACTIVE_SHADOW_OFFSET_Y_VALUE;
    const std::string INACTIVE_SHADOW_COLOR_STR_VALUE ("#00000000");
    const gushort INACTIVE_SHADOW_COLOR_VALUE[] = { 0, 0, 0 };
    const gboolean USE_TOOLTIPS_VALUE = !USE_TOOLTIPS_DEFAULT;
    const guint DRAGGABLE_BORDER_WIDTH_VALUE = 1;
    const gboolean ATTACH_MODAL_DIALOGS_VALUE = TRUE;
    const std::string BLUR_TYPE_TITLEBAR_VALUE ("titlebar");
    const gint BLUR_TYPE_TITLEBAR_INT_VALUE = BLUR_TYPE_TITLEBAR;
    const std::string BLUR_TYPE_ALL_VALUE ("all");
    const gint BLUR_TYPE_ALL_INT_VALUE = BLUR_TYPE_ALL;
    const std::string BLUR_TYPE_NONE_VALUE ("none");
    const gint BLUR_TYPE_NONE_INT_VALUE = BLUR_TYPE_NONE;
    const gboolean USE_METACITY_THEME_VALUE  = TRUE;
    const std::string METACITY_THEME_VALUE ("metacity_theme");
    const gboolean NO_USE_METACITY_THEME_VALUE  = FALSE;
    const std::string NO_METACITY_THEME_VALUE ("");
    const gdouble ACTIVE_OPACITY_VALUE = 9.0;
    const gdouble INACTIVE_OPACITY_VALUE = 10.0;
    const gboolean ACTIVE_SHADE_OPACITY_VALUE = !METACITY_ACTIVE_SHADE_OPACITY_DEFAULT;
    const gboolean INACTIVE_SHADE_OPACITY_VALUE = !METACITY_INACTIVE_SHADE_OPACITY_DEFAULT;
    const std::string BUTTON_LAYOUT_VALUE ("button_layout");
    const gboolean USE_SYSTEM_FONT_VALUE = TRUE;
    const gboolean NO_USE_SYSTEM_FONT_VALUE = FALSE;
    const std::string TITLEBAR_FONT_VALUE ("Sans 12");
    const std::string TITLEBAR_ACTION_SHADE ("toggle_shade");
    const std::string TITLEBAR_ACTION_MAX_VERT ("toggle_maximize_vertically");
    const std::string TITLEBAR_ACTION_MAX_HORZ ("toggle_maximize_horizontally");
    const std::string TITLEBAR_ACTION_MAX ("toggle_maximize");
    const std::string TITLEBAR_ACTION_MINIMIZE ("minimize");
    const std::string TITLEBAR_ACTION_MENU ("menu");
    const std::string TITLEBAR_ACTION_LOWER ("lower");
    const std::string TITLEBAR_ACTION_NONE ("none");
    const std::string MOUSE_WHEEL_ACTION_SHADE ("shade");
}

template <class ValueCType>
inline Matcher<GValue *>
GValueMatch (const ValueCType &value,
	     typename GValueCmp<ValueCType>::GetFunc	func)
{
    return MakeMatcher (new GObjectPropertyMatcher <ValueCType> (value, func));
}

class GWDSettingsTestCommon :
    public ::testing::Test
{
    public:
	virtual void SetUp ()
	{
	    g_setenv ("G_SLICE", "always-malloc", TRUE);
	    g_type_init ();
	}
	virtual void TearDown ()
	{
	    g_unsetenv ("G_SLICE");
	}
};

class GWDMockSettingsWritableTest :
    public GWDSettingsTestCommon
{
};

const GValue referenceGValue = G_VALUE_INIT;

namespace
{
    void gwd_settings_writable_unref (GWDSettingsWritable *writable)
    {
	g_object_unref (G_OBJECT (writable));
    }

    void gwd_settings_unref (GWDSettingsImpl *settings)
    {
	g_object_unref (G_OBJECT (settings));
    }

    class AutoUnsetGValue
    {
	public:

	    AutoUnsetGValue (GType type)
	    {
		memcpy (&mValue, &referenceGValue, sizeof (GValue));
		g_value_init (&mValue, type);
	    }

	    ~AutoUnsetGValue ()
	    {
		g_value_unset (&mValue);
	    }

	    operator GValue & ()
	    {
		return mValue;
	    }

	private:

	    GValue mValue;
    };
}

TEST_F(GWDMockSettingsWritableTest, TestMock)
{
    GWDMockSettingsWritableGMock writableGMock;
    boost::shared_ptr <GWDSettingsWritable> writableMock (gwd_mock_settings_writable_new (&writableGMock),
							  boost::bind (gwd_settings_writable_unref, _1));


    EXPECT_CALL (writableGMock, shadowPropertyChanged (testing_values::ACTIVE_SHADOW_RADIUS_VALUE,
						       testing_values::ACTIVE_SHADOW_OPACITY_VALUE,
						       testing_values::ACTIVE_SHADOW_OFFSET_X_VALUE,
						       testing_values::ACTIVE_SHADOW_OFFSET_Y_VALUE,
						       Eq (testing_values::ACTIVE_SHADOW_COLOR_STR_VALUE),
						       testing_values::INACTIVE_SHADOW_RADIUS_VALUE,
						       testing_values::INACTIVE_SHADOW_OPACITY_VALUE,
						       testing_values::INACTIVE_SHADOW_OFFSET_X_VALUE,
						       testing_values::INACTIVE_SHADOW_OFFSET_Y_VALUE,
						       Eq (testing_values::INACTIVE_SHADOW_COLOR_STR_VALUE))).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, useTooltipsChanged (testing_values::USE_TOOLTIPS_VALUE)).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, draggableBorderWidthChanged (testing_values::DRAGGABLE_BORDER_WIDTH_VALUE)).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, attachModalDialogsChanged (testing_values::ATTACH_MODAL_DIALOGS_VALUE)).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, blurChanged (Eq (testing_values::BLUR_TYPE_TITLEBAR_VALUE))).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, metacityThemeChanged (TRUE, Eq (testing_values::METACITY_THEME_VALUE))).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, opacityChanged (testing_values::ACTIVE_OPACITY_VALUE,
						testing_values::INACTIVE_OPACITY_VALUE,
						testing_values::ACTIVE_SHADE_OPACITY_VALUE,
						testing_values::INACTIVE_SHADE_OPACITY_VALUE)).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, buttonLayoutChanged (Eq (testing_values::BUTTON_LAYOUT_VALUE))).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, fontChanged (testing_values::USE_SYSTEM_FONT_VALUE,
					     testing_values::TITLEBAR_FONT_VALUE.c_str ())).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, titlebarActionsChanged (Eq (testing_values::TITLEBAR_ACTION_MAX),
							Eq (testing_values::TITLEBAR_ACTION_MENU),
							Eq (testing_values::TITLEBAR_ACTION_LOWER),
							Eq (testing_values::TITLEBAR_ACTION_SHADE))).WillOnce (Return (TRUE));

    EXPECT_CALL (writableGMock, dispose ());
    EXPECT_CALL (writableGMock, finalize ());

    EXPECT_THAT (gwd_settings_writable_shadow_property_changed (writableMock.get (),
								testing_values::ACTIVE_SHADOW_RADIUS_VALUE,
								testing_values::ACTIVE_SHADOW_OPACITY_VALUE,
								testing_values::ACTIVE_SHADOW_OFFSET_X_VALUE,
								testing_values::ACTIVE_SHADOW_OFFSET_Y_VALUE,
								testing_values::ACTIVE_SHADOW_COLOR_STR_VALUE.c_str (),
								testing_values::INACTIVE_SHADOW_RADIUS_VALUE,
								testing_values::INACTIVE_SHADOW_OPACITY_VALUE,
								testing_values::INACTIVE_SHADOW_OFFSET_X_VALUE,
								testing_values::INACTIVE_SHADOW_OFFSET_Y_VALUE,
								testing_values::INACTIVE_SHADOW_COLOR_STR_VALUE.c_str ()), GBooleanTrue ());
    EXPECT_THAT (gwd_settings_writable_use_tooltips_changed (writableMock.get (), testing_values::USE_TOOLTIPS_VALUE), GBooleanTrue ());
    EXPECT_THAT (gwd_settings_writable_draggable_border_width_changed (writableMock.get (), testing_values::DRAGGABLE_BORDER_WIDTH_VALUE), GBooleanTrue ());
    EXPECT_THAT (gwd_settings_writable_attach_modal_dialogs_changed (writableMock.get (), testing_values::ATTACH_MODAL_DIALOGS_VALUE), GBooleanTrue ());
    EXPECT_THAT (gwd_settings_writable_blur_changed (writableMock.get (), testing_values::BLUR_TYPE_TITLEBAR_VALUE.c_str ()), GBooleanTrue ());
    EXPECT_THAT (gwd_settings_writable_metacity_theme_changed (writableMock.get (),
							       testing_values::USE_METACITY_THEME_VALUE,
							       testing_values::METACITY_THEME_VALUE.c_str ()), GBooleanTrue ());
    EXPECT_THAT (gwd_settings_writable_opacity_changed (writableMock.get (),
							testing_values::ACTIVE_OPACITY_VALUE,
							testing_values::INACTIVE_OPACITY_VALUE,
							testing_values::ACTIVE_SHADE_OPACITY_VALUE,
							testing_values::INACTIVE_SHADE_OPACITY_VALUE), GBooleanTrue ());
    EXPECT_THAT (gwd_settings_writable_button_layout_changed (writableMock.get (),
							      testing_values::BUTTON_LAYOUT_VALUE.c_str ()), GBooleanTrue ());
    EXPECT_THAT (gwd_settings_writable_font_changed (writableMock.get (),
						     testing_values::USE_SYSTEM_FONT_VALUE,
						     testing_values::TITLEBAR_FONT_VALUE.c_str ()), GBooleanTrue ());
    EXPECT_THAT (gwd_settings_writable_titlebar_actions_changed (writableMock.get (),
								 testing_values::TITLEBAR_ACTION_MAX.c_str (),
								 testing_values::TITLEBAR_ACTION_MENU.c_str (),
								 testing_values::TITLEBAR_ACTION_LOWER.c_str (),
								 testing_values::TITLEBAR_ACTION_SHADE.c_str ()), GBooleanTrue ());
}

class GWDMockSettingsTest :
    public GWDSettingsTestCommon
{
};

TEST_F(GWDMockSettingsTest, TestMock)
{
    GWDMockSettingsGMock settingsGMock;
    boost::shared_ptr <GWDSettingsImpl> settingsMock (gwd_mock_settings_new (&settingsGMock),
						  boost::bind (gwd_settings_unref, _1));

    AutoUnsetGValue pointerValue (G_TYPE_POINTER);
    AutoUnsetGValue booleanValue (G_TYPE_BOOLEAN);
    AutoUnsetGValue stringValue (G_TYPE_STRING);
    AutoUnsetGValue integerValue (G_TYPE_INT);
    AutoUnsetGValue doubleValue (G_TYPE_DOUBLE);

    GValue &pointerGValue = pointerValue;
    GValue &booleanGValue = booleanValue;
    GValue &stringGValue = stringValue;
    GValue &integerGValue = integerValue;
    GValue &doubleGValue  = doubleValue;

    int	  POINTEE_VALUE = 1;
    gpointer POINTER_VALUE = &POINTEE_VALUE;
    const std::string STRING_VALUE ("test");
    const int INTEGER_VALUE = 2;
    const gboolean BOOLEAN_VALUE = TRUE;
    const gdouble DOUBLE_VALUE = 2.0;

    g_value_set_pointer (&pointerGValue, POINTER_VALUE);
    g_value_set_boolean (&booleanGValue, BOOLEAN_VALUE);
    g_value_set_string (&stringGValue, STRING_VALUE.c_str ());
    g_value_set_int (&integerGValue, INTEGER_VALUE);
    g_value_set_double (&doubleGValue, DOUBLE_VALUE);

    EXPECT_CALL (settingsGMock, dispose ());
    EXPECT_CALL (settingsGMock, finalize ());

    /* calling g_object_get_property actually resets
     * the value so expecting 0x0 is correct */
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_ACTIVE_SHADOW,
					     GValueMatch <gpointer> (0x0, g_value_get_pointer),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_INACTIVE_SHADOW,
					     GValueMatch <gpointer> (0x0, g_value_get_pointer),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_USE_TOOLTIPS,
					     GValueMatch <gboolean> (FALSE, g_value_get_boolean),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_DRAGGABLE_BORDER_WIDTH,
					     GValueMatch <gint> (0, g_value_get_int),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_ATTACH_MODAL_DIALOGS,
					     GValueMatch <gboolean> (FALSE, g_value_get_boolean),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_BLUR_CHANGED,
					     GValueMatch <gint> (0, g_value_get_int),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_METACITY_THEME,
					     GValueMatch <const gchar *> (NULL, g_value_get_string),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_ACTIVE_OPACITY,
					     GValueMatch <gdouble> (0.0, g_value_get_double),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_INACTIVE_OPACITY,
					     GValueMatch <gdouble> (0.0, g_value_get_double),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_ACTIVE_SHADE_OPACITY,
					     GValueMatch <gboolean> (FALSE, g_value_get_boolean),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_INACTIVE_SHADE_OPACITY,
					     GValueMatch <gboolean> (FALSE, g_value_get_boolean),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_BUTTON_LAYOUT,
					     GValueMatch <const gchar *> (NULL, g_value_get_string),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_TITLEBAR_ACTION_DOUBLE_CLICK,
					     GValueMatch <gint> (0, g_value_get_int),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_TITLEBAR_ACTION_MIDDLE_CLICK,
					     GValueMatch <gint> (0, g_value_get_int),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_TITLEBAR_ACTION_RIGHT_CLICK,
					     GValueMatch <gint> (0, g_value_get_int),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_MOUSE_WHEEL_ACTION,
					     GValueMatch <gint> (0, g_value_get_int),
					     _));
    EXPECT_CALL (settingsGMock, getProperty (GWD_MOCK_SETTINGS_PROPERTY_TITLEBAR_FONT,
					     GValueMatch <const gchar *> (NULL, g_value_get_string),
					     _));

    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "active-shadow",
			   &pointerGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "inactive-shadow",
			   &pointerGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "use-tooltips",
			   &booleanGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "draggable-border-width",
			   &integerGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "attach-modal-dialogs",
			   &booleanGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "blur",
			   &integerGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "metacity-theme",
			   &stringGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "metacity-active-opacity",
			   &doubleGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "metacity-inactive-opacity",
			   &doubleGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "metacity-active-shade-opacity",
			   &booleanGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "metacity-inactive-shade-opacity",
			   &booleanGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "metacity-button-layout",
			   &stringGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "titlebar-double-click-action",
			   &integerGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "titlebar-middle-click-action",
			   &integerGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "titlebar-right-click-action",
			   &integerGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "mouse-wheel-action",
			   &integerGValue);
    g_object_get_property (G_OBJECT (settingsMock.get ()),
			   "titlebar-font",
			   &stringGValue);
}

class GWDSettingsTest :
    public GWDSettingsTestCommon
{
    public:

	virtual void SetUp ()
	{
	    mSettings.reset (gwd_settings_impl_new (),
			     boost::bind (gwd_settings_unref, _1));
	}

    protected:

	boost::shared_ptr <GWDSettingsImpl> mSettings;
};

TEST_F(GWDSettingsTest, TestGWDSettingsInstantiation)
{
}

TEST_F(GWDSettingsTest, TestShadowPropertyChanged)
{
    EXPECT_THAT (gwd_settings_writable_shadow_property_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
								testing_values::ACTIVE_SHADOW_OPACITY_VALUE,
								testing_values::ACTIVE_SHADOW_RADIUS_VALUE,
								testing_values::ACTIVE_SHADOW_OFFSET_X_VALUE,
								testing_values::ACTIVE_SHADOW_OFFSET_Y_VALUE,
								testing_values::ACTIVE_SHADOW_COLOR_STR_VALUE.c_str (),
								testing_values::INACTIVE_SHADOW_OPACITY_VALUE,
								testing_values::INACTIVE_SHADOW_RADIUS_VALUE,
								testing_values::INACTIVE_SHADOW_OFFSET_X_VALUE,
								testing_values::INACTIVE_SHADOW_OFFSET_Y_VALUE,
								testing_values::INACTIVE_SHADOW_COLOR_STR_VALUE.c_str ()), GBooleanTrue ());

    AutoUnsetGValue activeShadowValue (G_TYPE_POINTER);
    AutoUnsetGValue inactiveShadowValue (G_TYPE_POINTER);

    GValue &activeShadowGValue = activeShadowValue;
    GValue &inactiveShadowGValue = inactiveShadowValue;

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "active-shadow",
			   &activeShadowGValue);

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "inactive-shadow",
			   &inactiveShadowGValue);

    decor_shadow_options_t activeShadow;

    activeShadow.shadow_opacity = testing_values::ACTIVE_SHADOW_OPACITY_VALUE;
    activeShadow.shadow_radius = testing_values::ACTIVE_SHADOW_RADIUS_VALUE;
    activeShadow.shadow_offset_x = testing_values::ACTIVE_SHADOW_OFFSET_X_INT_VALUE;
    activeShadow.shadow_offset_y = testing_values::ACTIVE_SHADOW_OFFSET_Y_INT_VALUE;
    activeShadow.shadow_color[0] = testing_values::ACTIVE_SHADOW_COLOR_VALUE[0];
    activeShadow.shadow_color[1] = testing_values::ACTIVE_SHADOW_COLOR_VALUE[1];
    activeShadow.shadow_color[2] = testing_values::ACTIVE_SHADOW_COLOR_VALUE[2];

    decor_shadow_options_t inactiveShadow;

    inactiveShadow.shadow_opacity = testing_values::INACTIVE_SHADOW_OPACITY_VALUE;
    inactiveShadow.shadow_radius = testing_values::INACTIVE_SHADOW_RADIUS_VALUE;
    inactiveShadow.shadow_offset_x = testing_values::INACTIVE_SHADOW_OFFSET_X_INT_VALUE;
    inactiveShadow.shadow_offset_y = testing_values::INACTIVE_SHADOW_OFFSET_Y_INT_VALUE;
    inactiveShadow.shadow_color[0] = testing_values::INACTIVE_SHADOW_COLOR_VALUE[0];
    inactiveShadow.shadow_color[1] = testing_values::INACTIVE_SHADOW_COLOR_VALUE[1];
    inactiveShadow.shadow_color[2] = testing_values::INACTIVE_SHADOW_COLOR_VALUE[2];

    EXPECT_THAT (&activeShadowGValue, GValueMatch <decor_shadow_options_t> (activeShadow,
									    g_value_get_pointer));
    EXPECT_THAT (&inactiveShadowGValue, GValueMatch <decor_shadow_options_t> (inactiveShadow,
									      g_value_get_pointer));
}

TEST_F(GWDSettingsTest, TestShadowPropertyChangedIsDefault)
{
    EXPECT_THAT (gwd_settings_writable_shadow_property_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
								ACTIVE_SHADOW_RADIUS_DEFAULT,
								ACTIVE_SHADOW_OPACITY_DEFAULT,
								ACTIVE_SHADOW_OFFSET_X_DEFAULT,
								ACTIVE_SHADOW_OFFSET_Y_DEFAULT,
								ACTIVE_SHADOW_COLOR_DEFAULT,
								INACTIVE_SHADOW_RADIUS_DEFAULT,
								INACTIVE_SHADOW_OPACITY_DEFAULT,
								INACTIVE_SHADOW_OFFSET_X_DEFAULT,
								INACTIVE_SHADOW_OFFSET_Y_DEFAULT,
								INACTIVE_SHADOW_COLOR_DEFAULT), GBooleanFalse ());
}

TEST_F(GWDSettingsTest, TestUseTooltipsChanged)
{
    EXPECT_THAT (gwd_settings_writable_use_tooltips_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
							     testing_values::USE_TOOLTIPS_VALUE), GBooleanTrue ());

    AutoUnsetGValue useTooltipsValue (G_TYPE_BOOLEAN);
    GValue &useTooltipsGValue = useTooltipsValue;

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "use-tooltips",
			   &useTooltipsGValue);

    EXPECT_THAT (&useTooltipsGValue, GValueMatch <gboolean> (testing_values::USE_TOOLTIPS_VALUE,
							     g_value_get_boolean));
}

TEST_F(GWDSettingsTest, TestUseTooltipsChangedIsDefault)
{
    EXPECT_THAT (gwd_settings_writable_use_tooltips_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
							     USE_TOOLTIPS_DEFAULT), GBooleanFalse ());
}

TEST_F(GWDSettingsTest, TestDraggableBorderWidthChanged)
{
    EXPECT_THAT (gwd_settings_writable_draggable_border_width_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
								       testing_values::DRAGGABLE_BORDER_WIDTH_VALUE), GBooleanTrue ());

    AutoUnsetGValue draggableBorderWidthValue (G_TYPE_INT);
    GValue &draggableBorderWidthGValue = draggableBorderWidthValue;

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "draggable-border-width",
			   &draggableBorderWidthGValue);

    EXPECT_THAT (&draggableBorderWidthGValue, GValueMatch <gint> (testing_values::DRAGGABLE_BORDER_WIDTH_VALUE,
								  g_value_get_int));
}

TEST_F(GWDSettingsTest, TestDraggableBorderWidthChangedIsDefault)
{
    EXPECT_THAT (gwd_settings_writable_draggable_border_width_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
								       DRAGGABLE_BORDER_WIDTH_DEFAULT), GBooleanFalse ());
}

TEST_F(GWDSettingsTest, TestAttachModalDialogsChanged)
{
    EXPECT_THAT (gwd_settings_writable_attach_modal_dialogs_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
								     testing_values::ATTACH_MODAL_DIALOGS_VALUE), GBooleanTrue ());

    AutoUnsetGValue attachModalDialogsValue (G_TYPE_BOOLEAN);
    GValue &attachModalDialogsGValue = attachModalDialogsValue;

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "attach-modal-dialogs",
			   &attachModalDialogsGValue);

    EXPECT_THAT (&attachModalDialogsGValue, GValueMatch <gboolean> (testing_values::ATTACH_MODAL_DIALOGS_VALUE,
								    g_value_get_boolean));
}

TEST_F(GWDSettingsTest, TestAttachModalDialogsChangedIsDefault)
{
    EXPECT_THAT (gwd_settings_writable_attach_modal_dialogs_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
								     ATTACH_MODAL_DIALOGS_DEFAULT), GBooleanFalse ());
}

TEST_F(GWDSettingsTest, TestBlurChangedTitlebar)
{
    EXPECT_THAT (gwd_settings_writable_blur_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
						     testing_values::BLUR_TYPE_TITLEBAR_VALUE.c_str ()), GBooleanTrue ());

    AutoUnsetGValue blurValue (G_TYPE_INT);
    GValue &blurGValue = blurValue;

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "blur",
			   &blurGValue);

    EXPECT_THAT (&blurGValue, GValueMatch <gint> (testing_values::BLUR_TYPE_TITLEBAR_INT_VALUE,
						  g_value_get_int));
}

TEST_F(GWDSettingsTest, TestBlurChangedAll)
{
    EXPECT_THAT (gwd_settings_writable_blur_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
						     testing_values::BLUR_TYPE_ALL_VALUE.c_str ()), GBooleanTrue ());

    AutoUnsetGValue blurValue (G_TYPE_INT);
    GValue &blurGValue = blurValue;

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "blur",
			   &blurGValue);

    EXPECT_THAT (&blurGValue, GValueMatch <gint> (testing_values::BLUR_TYPE_ALL_INT_VALUE,
						  g_value_get_int));
}

TEST_F(GWDSettingsTest, TestBlurChangedNone)
{
    EXPECT_THAT (gwd_settings_writable_blur_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
						     testing_values::BLUR_TYPE_NONE_VALUE.c_str ()), GBooleanFalse ());

    AutoUnsetGValue blurValue (G_TYPE_INT);
    GValue &blurGValue = blurValue;

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "blur",
			   &blurGValue);

    EXPECT_THAT (&blurGValue, GValueMatch <gint> (testing_values::BLUR_TYPE_NONE_INT_VALUE,
						  g_value_get_int));
}

TEST_F(GWDSettingsTest, TestMetacityThemeChanged)
{
    EXPECT_THAT (gwd_settings_writable_metacity_theme_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
							       testing_values::USE_METACITY_THEME_VALUE,
							       testing_values::METACITY_THEME_VALUE.c_str ()), GBooleanTrue ());

    AutoUnsetGValue metacityThemeValue (G_TYPE_STRING);
    GValue &metacityThemeGValue = metacityThemeValue;

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "metacity-theme",
			   &metacityThemeGValue);

    EXPECT_THAT (&metacityThemeGValue, GValueMatch <std::string> (testing_values::METACITY_THEME_VALUE,
								  g_value_get_string));
}

TEST_F(GWDSettingsTest, TestMetacityThemeChangedNoUseMetacityTheme)
{
    EXPECT_THAT (gwd_settings_writable_metacity_theme_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
							       testing_values::NO_USE_METACITY_THEME_VALUE,
							       testing_values::METACITY_THEME_VALUE.c_str ()), GBooleanTrue ());

    AutoUnsetGValue metacityThemeValue (G_TYPE_STRING);
    GValue &metacityThemeGValue = metacityThemeValue;

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "metacity-theme",
			   &metacityThemeGValue);

    EXPECT_THAT (&metacityThemeGValue, GValueMatch <std::string> (testing_values::NO_METACITY_THEME_VALUE,
								  g_value_get_string));
}

TEST_F(GWDSettingsTest, TestMetacityThemeChangedIsDefault)
{
    EXPECT_THAT (gwd_settings_writable_metacity_theme_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
							       testing_values::USE_METACITY_THEME_VALUE,
							       METACITY_THEME_DEFAULT), GBooleanFalse ());
}

TEST_F(GWDSettingsTest, TestMetacityOpacityChanged)
{
    EXPECT_THAT (gwd_settings_writable_opacity_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
							testing_values::INACTIVE_OPACITY_VALUE,
							testing_values::ACTIVE_OPACITY_VALUE,
							testing_values::INACTIVE_SHADE_OPACITY_VALUE,
							testing_values::ACTIVE_SHADE_OPACITY_VALUE), GBooleanTrue ());

    AutoUnsetGValue metacityInactiveOpacityValue (G_TYPE_DOUBLE);
    AutoUnsetGValue metacityActiveOpacityValue (G_TYPE_DOUBLE);
    AutoUnsetGValue metacityInactiveShadeOpacityValue (G_TYPE_BOOLEAN);
    AutoUnsetGValue metacityActiveShadeOpacityValue (G_TYPE_BOOLEAN);

    GValue &metacityInactiveOpacityGValue = metacityInactiveOpacityValue;
    GValue &metacityActiveOpacityGValue = metacityActiveOpacityValue;
    GValue &metacityInactiveShadeOpacityGValue = metacityInactiveShadeOpacityValue;
    GValue &metacityActiveShadeOpacityGValue = metacityActiveShadeOpacityValue;

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "metacity-inactive-opacity",
			   &metacityInactiveOpacityGValue);
    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "metacity-active-opacity",
			   &metacityActiveOpacityGValue);
    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "metacity-inactive-shade-opacity",
			   &metacityInactiveShadeOpacityGValue);
    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "metacity-active-shade-opacity",
			   &metacityActiveShadeOpacityGValue);

    EXPECT_THAT (&metacityInactiveOpacityGValue, GValueMatch <gdouble> (testing_values::INACTIVE_OPACITY_VALUE,
									g_value_get_double));
    EXPECT_THAT (&metacityActiveOpacityGValue, GValueMatch <gdouble> (testing_values::ACTIVE_OPACITY_VALUE,
									g_value_get_double));
    EXPECT_THAT (&metacityInactiveShadeOpacityGValue, GValueMatch <gboolean> (testing_values::INACTIVE_SHADE_OPACITY_VALUE,
									g_value_get_boolean));
    EXPECT_THAT (&metacityActiveShadeOpacityGValue, GValueMatch <gboolean> (testing_values::ACTIVE_SHADE_OPACITY_VALUE,
									g_value_get_boolean));
}

TEST_F(GWDSettingsTest, TestButtonLayoutChanged)
{
    EXPECT_THAT (gwd_settings_writable_button_layout_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
							      testing_values::BUTTON_LAYOUT_VALUE.c_str ()), GBooleanTrue ());

    AutoUnsetGValue buttonLayoutValue (G_TYPE_STRING);
    GValue &buttonLayoutGValue = buttonLayoutValue;

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "metacity-button-layout",
			   &buttonLayoutGValue);

    EXPECT_THAT (&buttonLayoutGValue, GValueMatch <std::string> (testing_values::BUTTON_LAYOUT_VALUE,
								 g_value_get_string));
}

TEST_F(GWDSettingsTest, TestTitlebarFontChanged)
{
    EXPECT_THAT (gwd_settings_writable_font_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
						     testing_values::NO_USE_SYSTEM_FONT_VALUE,
						     testing_values::TITLEBAR_FONT_VALUE.c_str ()), GBooleanTrue ());

    AutoUnsetGValue fontValue (G_TYPE_STRING);
    GValue	    &fontGValue = fontValue;

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "titlebar-font",
			   &fontGValue);

    EXPECT_THAT (&fontGValue, GValueMatch <std::string> (testing_values::TITLEBAR_FONT_VALUE.c_str (),
							 g_value_get_string));
}

TEST_F(GWDSettingsTest, TestTitlebarFontChangedUseSystemFont)
{
    EXPECT_THAT (gwd_settings_writable_font_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
						     testing_values::USE_SYSTEM_FONT_VALUE,
						     testing_values::TITLEBAR_FONT_VALUE.c_str ()), GBooleanTrue ());

    AutoUnsetGValue fontValue (G_TYPE_STRING);
    GValue	    &fontGValue = fontValue;

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "titlebar-font",
			   &fontGValue);

    EXPECT_THAT (&fontGValue, GValueMatch <const gchar *> (NULL,
							   g_value_get_string));
}

namespace
{
    class GWDTitlebarActionInfo
    {
	public:

	    GWDTitlebarActionInfo (const std::string &titlebarAction,
				   const std::string &mouseWheelAction,
				   const gint	     titlebarActionId,
				   const gint	     mouseWheelActionId) :
		mTitlebarAction (titlebarAction),
		mMouseWheelAction (mouseWheelAction),
		mTitlebarActionId (titlebarActionId),
		mMouseWheelActionId (mouseWheelActionId)
	    {
	    }

	    const std::string & titlebarAction () const { return mTitlebarAction; }
	    const std::string & mouseWheelAction () const { return mMouseWheelAction; }
	    const gint	      & titlebarActionId () const { return mTitlebarActionId; }
	    const gint	      & mouseWheelActionId () const { return mMouseWheelActionId; }

	private:

	    std::string mTitlebarAction;
	    std::string mMouseWheelAction;
	    gint	mTitlebarActionId;
	    gint	mMouseWheelActionId;
    };
}

class GWDSettingsTestClickActions :
    public ::testing::TestWithParam <GWDTitlebarActionInfo>
{
    public:

	virtual void SetUp ()
	{
	    mSettings.reset (gwd_settings_impl_new (),
			     boost::bind (gwd_settings_unref, _1));
	}

    protected:

	boost::shared_ptr <GWDSettingsImpl> mSettings;
};

TEST_P(GWDSettingsTestClickActions, TestClickActionsAndMouseActions)
{
    EXPECT_THAT (gwd_settings_writable_titlebar_actions_changed (GWD_SETTINGS_WRITABLE_INTERFACE (mSettings.get ()),
								 GetParam ().titlebarAction ().c_str (),
								 GetParam ().titlebarAction ().c_str (),
								 GetParam ().titlebarAction ().c_str (),
								 GetParam ().mouseWheelAction ().c_str ()), GBooleanTrue ());

    AutoUnsetGValue doubleClickActionValue (G_TYPE_INT);
    AutoUnsetGValue middleClickActionValue (G_TYPE_INT);
    AutoUnsetGValue rightClickActionValue (G_TYPE_INT);
    AutoUnsetGValue mouseWheelActionValue (G_TYPE_INT);

    GValue &doubleClickActionGValue = doubleClickActionValue;
    GValue &middleClickActionGValue = middleClickActionValue;
    GValue &rightClickActionGValue = rightClickActionValue;
    GValue &mouseWheelActionGValue = mouseWheelActionValue;

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "titlebar-double-click-action",
			   &doubleClickActionGValue);

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "titlebar-middle-click-action",
			   &middleClickActionGValue);

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "titlebar-right-click-action",
			   &rightClickActionGValue);

    g_object_get_property (G_OBJECT (mSettings.get ()),
			   "mouse-wheel-action",
			   &mouseWheelActionGValue);

    EXPECT_THAT (&doubleClickActionGValue, GValueMatch <gint> (GetParam ().titlebarActionId (),
							      g_value_get_int));
    EXPECT_THAT (&middleClickActionGValue, GValueMatch <gint> (GetParam ().titlebarActionId (),
							      g_value_get_int));
    EXPECT_THAT (&rightClickActionGValue, GValueMatch <gint> (GetParam ().titlebarActionId (),
							     g_value_get_int));
    EXPECT_THAT (&mouseWheelActionGValue, GValueMatch <gint> (GetParam ().mouseWheelActionId (),
							     g_value_get_int));
}

INSTANTIATE_TEST_CASE_P (MouseActions, GWDSettingsTestClickActions,
			 ::testing::Values (GWDTitlebarActionInfo (testing_values::TITLEBAR_ACTION_NONE,
								   testing_values::TITLEBAR_ACTION_NONE,
								   CLICK_ACTION_NONE,
								   WHEEL_ACTION_NONE),
					    GWDTitlebarActionInfo (testing_values::TITLEBAR_ACTION_SHADE,
								   testing_values::MOUSE_WHEEL_ACTION_SHADE,
								   CLICK_ACTION_SHADE,
								   WHEEL_ACTION_SHADE),
					    GWDTitlebarActionInfo (testing_values::TITLEBAR_ACTION_MAX,
								   testing_values::MOUSE_WHEEL_ACTION_SHADE,
								   CLICK_ACTION_MAXIMIZE,
								   WHEEL_ACTION_SHADE),
					    GWDTitlebarActionInfo (testing_values::TITLEBAR_ACTION_MINIMIZE,
								   testing_values::MOUSE_WHEEL_ACTION_SHADE,
								   CLICK_ACTION_MINIMIZE,
								   WHEEL_ACTION_SHADE),
					    GWDTitlebarActionInfo (testing_values::TITLEBAR_ACTION_LOWER,
								   testing_values::MOUSE_WHEEL_ACTION_SHADE,
								   CLICK_ACTION_LOWER,
								   WHEEL_ACTION_SHADE),
					    GWDTitlebarActionInfo (testing_values::TITLEBAR_ACTION_MENU,
								   testing_values::MOUSE_WHEEL_ACTION_SHADE,
								   CLICK_ACTION_MENU,
								   WHEEL_ACTION_SHADE)));
