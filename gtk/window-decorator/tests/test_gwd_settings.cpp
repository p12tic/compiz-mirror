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

#include "compiz_gwd_mock_settings.h"
#include "compiz_gwd_mock_settings_writable.h"

using ::testing::Eq;
using ::testing::Return;
using ::testing::MatcherInterface;
using ::testing::MakeMatcher;
using ::testing::MatchResultListener;
using ::testing::Matcher;
using ::testing::IsNull;
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
    const gdouble ACTIVE_SHADOW_OFFSET_Y_VALUE = 4.0;
    const std::string ACTIVE_SHADOW_COLOR_VALUE ("active_shadow_color");
    const gdouble INACTIVE_SHADOW_OPACITY_VALUE = 5.0;
    const gdouble INACTIVE_SHADOW_RADIUS_VALUE = 6.0;
    const gdouble INACTIVE_SHADOW_OFFSET_X_VALUE = 7.0;
    const gdouble INACTIVE_SHADOW_OFFSET_Y_VALUE = 8.0;
    const std::string INACTIVE_SHADOW_COLOR_VALUE ("inactive_shadow_color");
    const gboolean USE_TOOLTIPS_VALUE = TRUE;
    const guint DRAGGABLE_BORDER_WIDTH_VALUE = 1;
    const gboolean ATTACH_MODAL_DIALOGS_VALUE = TRUE;
    const std::string BLUR_TYPE_VALUE ("blur_type");
    const gboolean USE_METACITY_THEME_VALUE  = TRUE;
    const std::string METACITY_THEME_VALUE ("metacity_theme");
    const gdouble ACTIVE_OPACITY_VALUE = 9.0;
    const gdouble INACTIVE_OPACITY_VALUE = 10.0;
    const gboolean ACTIVE_SHADE_OPACITY_VALUE = TRUE;
    const gboolean INACTIVE_SHADE_OPACITY_VALUE = TRUE;
    const std::string BUTTON_LAYOUT_VALUE ("button_layout");
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
						       Eq (testing_values::ACTIVE_SHADOW_COLOR_VALUE),
						       testing_values::INACTIVE_SHADOW_RADIUS_VALUE,
						       testing_values::INACTIVE_SHADOW_OPACITY_VALUE,
						       testing_values::INACTIVE_SHADOW_OFFSET_X_VALUE,
						       testing_values::INACTIVE_SHADOW_OFFSET_Y_VALUE,
						       Eq (testing_values::INACTIVE_SHADOW_COLOR_VALUE))).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, useTooltipsChanged (testing_values::USE_TOOLTIPS_VALUE)).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, draggableBorderWidthChanged (testing_values::DRAGGABLE_BORDER_WIDTH_VALUE)).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, attachModalDialogsChanged (testing_values::ATTACH_MODAL_DIALOGS_VALUE)).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, blurChanged (Eq (testing_values::BLUR_TYPE_VALUE))).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, metacityThemeChanged (TRUE, Eq (testing_values::METACITY_THEME_VALUE))).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, opacityChanged (testing_values::ACTIVE_OPACITY_VALUE,
						testing_values::INACTIVE_OPACITY_VALUE,
						testing_values::ACTIVE_SHADE_OPACITY_VALUE,
						testing_values::INACTIVE_SHADE_OPACITY_VALUE)).WillOnce (Return (TRUE));
    EXPECT_CALL (writableGMock, buttonLayoutChanged (Eq (testing_values::BUTTON_LAYOUT_VALUE))).WillOnce (Return (TRUE));

    EXPECT_CALL (writableGMock, dispose ());
    EXPECT_CALL (writableGMock, finalize ());

    EXPECT_THAT (gwd_settings_writable_shadow_property_changed (writableMock.get (),
								testing_values::ACTIVE_SHADOW_RADIUS_VALUE,
								testing_values::ACTIVE_SHADOW_OPACITY_VALUE,
								testing_values::ACTIVE_SHADOW_OFFSET_X_VALUE,
								testing_values::ACTIVE_SHADOW_OFFSET_Y_VALUE,
								testing_values::ACTIVE_SHADOW_COLOR_VALUE.c_str (),
								testing_values::INACTIVE_SHADOW_RADIUS_VALUE,
								testing_values::INACTIVE_SHADOW_OPACITY_VALUE,
								testing_values::INACTIVE_SHADOW_OFFSET_X_VALUE,
								testing_values::INACTIVE_SHADOW_OFFSET_Y_VALUE,
								testing_values::INACTIVE_SHADOW_COLOR_VALUE.c_str ()), GBooleanTrue ());
    EXPECT_THAT (gwd_settings_writable_use_tooltips_changed (writableMock.get (), testing_values::USE_TOOLTIPS_VALUE), GBooleanTrue ());
    EXPECT_THAT (gwd_settings_writable_draggable_border_width_changed (writableMock.get (), testing_values::DRAGGABLE_BORDER_WIDTH_VALUE), GBooleanTrue ());
    EXPECT_THAT (gwd_settings_writable_attach_modal_dialogs_changed (writableMock.get (), testing_values::ATTACH_MODAL_DIALOGS_VALUE), GBooleanTrue ());
    EXPECT_THAT (gwd_settings_writable_blur_changed (writableMock.get (), testing_values::BLUR_TYPE_VALUE.c_str ()), GBooleanTrue ());
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
}

class GWDSettingsTest :
    public GWDSettingsTestCommon
{
};

TEST_F(GWDSettingsTest, TestGWDSettingsInstantiation)
{
    boost::shared_ptr <GWDSettingsImpl> settingsMock (gwd_settings_impl_new (),
						  boost::bind (gwd_settings_unref, _1));


}
