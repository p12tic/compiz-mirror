#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <boost/shared_ptr.hpp>

#include "test_gsettings_tests.h"
#include "gsettings.h"
#include "wrap_gsettings.h"
#include "ccs_gsettings_backend_mock.h"
#include "compizconfig_ccs_context_mock.h"
#include "compizconfig_ccs_setting_mock.h"
#include "gtest_shared_characterwrapper.h"
#include "compizconfig_test_value_combiners.h"
#include "compizconfig_ccs_mocked_allocator.h"

using ::testing::Values;
using ::testing::ValuesIn;
using ::testing::Return;
using ::testing::ReturnNull;
using ::testing::Invoke;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::AllOf;
using ::testing::Not;
using ::testing::Matcher;
using ::testing::Eq;
using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::IsNull;

class GVariantSubtypeMatcher :
    public ::testing::MatcherInterface<GVariant *>
{
    public:
	GVariantSubtypeMatcher (const std::string &type) :
	    mType (type)
	{
	}

	virtual ~GVariantSubtypeMatcher () {}
	virtual bool MatchAndExplain (GVariant *x, MatchResultListener *listener) const
	{
	    return g_variant_type_is_subtype_of (G_VARIANT_TYPE (mType.c_str ()), g_variant_get_type (x));
	}

	virtual void DescribeTo (std::ostream *os) const
	{
	    *os << "is subtype of " << mType;
	}
    private:

	std::string mType;
};

template <typename T>
class GVariantHasValueInArrayMatcher :
    public ::testing::MatcherInterface<GVariant *>
{
    public:
	GVariantHasValueInArrayMatcher (const std::string &type,
					const T &t,
					const boost::function <bool (T const&, T const&)> &eq) :
	    mType (type),
	    mT (t),
	    mEq (eq)
	{
	}

	virtual ~GVariantHasValueInArrayMatcher () {}
	virtual bool MatchAndExplain (GVariant *x, MatchResultListener *listener) const
	{
	    GVariantIter iter;
	    T            match;
	    bool         found = false;

	    g_variant_iter_init (&iter, x);
	    while (g_variant_iter_loop (&iter, mType.c_str (), &match))
	    {
		if (mEq (match, mT))
		    found = true;
	    }

	    return found;
	}

	virtual void DescribeTo (std::ostream *os) const
	{
	    *os << "contains " << mT;
	}
    private:

	std::string mType;
	T           mT;
	boost::function <bool (T const&, T const&)> mEq;
};

template <typename T>
inline Matcher<GVariant *> GVariantHasValueInArray (const std::string &type,
						    const T &t,
						    const boost::function <bool (T const &, T const &)> &eq)
{
    return MakeMatcher (new GVariantHasValueInArrayMatcher<T> (type, t, eq));
}

inline Matcher<GVariant *> IsVariantSubtypeOf (const std::string &type)
{
    return MakeMatcher (new GVariantSubtypeMatcher (type));
}

TEST_P(CCSGSettingsTest, TestTestFixtures)
{
}

TEST_F(CCSGSettingsTestIndependent, TestTest)
{
}

namespace
{
bool streq (const char * const &s1, const char * const &s2)
{
    return g_str_equal (s1, s2);
}

}

TEST_F(CCSGSettingsTestIndependent, TestUpdateCurrentProfileNameAppendNew)
{
    g_type_init ();

    boost::shared_ptr <CCSBackend> backend (ccsGSettingsBackendGMockNew (),
					    boost::bind (ccsGSettingsBackendGMockFree, _1));
    CCSGSettingsBackendGMock *gmock = reinterpret_cast <CCSGSettingsBackendGMock *> (ccsObjectGetPrivate (backend.get ()));

    GVariantBuilder builder;

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("as"));
    g_variant_builder_add (&builder, "s", "foo");

    GVariant *existingProfiles = g_variant_builder_end (&builder);

    std::string newProfileName ("narr");

    EXPECT_CALL (*gmock, getExistingProfiles ()).WillOnce (Return (existingProfiles));
    EXPECT_CALL (*gmock, setExistingProfiles (AllOf (IsVariantSubtypeOf ("as"),
						     GVariantHasValueInArray<const gchar *> ("s",
											     newProfileName.c_str (),
											     boost::bind (streq, _1, _2)))));
    EXPECT_CALL (*gmock, setCurrentProfile (Eq (newProfileName)));

    ccsGSettingsBackendUpdateCurrentProfileNameDefault (backend.get (), newProfileName.c_str ());
}

TEST_F(CCSGSettingsTestIndependent, TestUpdateCurrentProfileNameExisting)
{
    g_type_init ();

    boost::shared_ptr <CCSBackend> backend (ccsGSettingsBackendGMockNew (),
					    boost::bind (ccsGSettingsBackendGMockFree, _1));
    CCSGSettingsBackendGMock *gmock = reinterpret_cast <CCSGSettingsBackendGMock *> (ccsObjectGetPrivate (backend.get ()));

    GVariantBuilder builder;

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("as"));
    g_variant_builder_add (&builder, "s", "foo");

    GVariant *existingProfiles = g_variant_builder_end (&builder);

    EXPECT_CALL (*gmock, getExistingProfiles ()).WillOnce (Return (existingProfiles));
    EXPECT_CALL (*gmock, setCurrentProfile (_));

    ccsGSettingsBackendUpdateCurrentProfileNameDefault (backend.get (), "foo");
}

TEST_F(CCSGSettingsTestIndependent, TestDeleteProfileExistingProfile)
{
    g_type_init ();

    boost::shared_ptr <CCSBackend> backend (ccsGSettingsBackendGMockNew (),
					    boost::bind (ccsGSettingsBackendGMockFree, _1));
    boost::shared_ptr <CCSContext> context (ccsMockContextNew (),
					    boost::bind (ccsFreeMockContext, _1));

    CCSGSettingsBackendGMock *mockBackend = reinterpret_cast <CCSGSettingsBackendGMock *> (ccsObjectGetPrivate (backend.get ()));

    std::string currentProfile ("foo");

    GVariant *existingProfiles = NULL;
    GVariantBuilder existingProfilesBuilder;

    g_variant_builder_init (&existingProfilesBuilder, G_VARIANT_TYPE ("as"));
    g_variant_builder_add (&existingProfilesBuilder, "s", currentProfile.c_str ());

    existingProfiles = g_variant_builder_end (&existingProfilesBuilder);

    EXPECT_CALL (*mockBackend, getPluginsWithSetKeys ()).WillOnce (ReturnNull ());
    EXPECT_CALL (*mockBackend, unsetAllChangedPluginKeysInProfile (context.get (), NULL, Eq (currentProfile)));
    EXPECT_CALL (*mockBackend, clearPluginsWithSetKeys ());

    EXPECT_CALL (*mockBackend, getCurrentProfile ()).WillOnce (Return (currentProfile.c_str ()));
    EXPECT_CALL (*mockBackend, getExistingProfiles ()).WillOnce (Return (existingProfiles));
    EXPECT_CALL (*mockBackend, setExistingProfiles (AllOf (IsVariantSubtypeOf ("as"),
							   Not (GVariantHasValueInArray<const gchar *> ("s",
													currentProfile.c_str (),
													boost::bind (streq, _1, _2))))));

    EXPECT_CALL (*mockBackend, updateProfile (context.get ()));

    deleteProfile (backend.get (), context.get (), currentProfile.c_str ());
}

TEST_F(CCSGSettingsTestIndependent, TestGetSchemaNameForPlugin)
{
    const gchar *plugin = "foo";
    gchar *schemaName = getSchemaNameForPlugin (plugin);

    std::string schemaNameStr (schemaName);

    size_t pos = schemaNameStr.find (PLUGIN_SCHEMA_ID_PREFIX, 0);

    EXPECT_EQ (pos, 0);

    g_free (schemaName);
}

TEST_F(CCSGSettingsTestIndependent, TestTruncateKeyForGSettingsOver)
{
    const unsigned int OVER_KEY_SIZE = MAX_GSETTINGS_KEY_SIZE + 1;

    std::string keyname;

    for (unsigned int i = 0; i <= OVER_KEY_SIZE - 1; i++)
	keyname.push_back ('a');

    ASSERT_EQ (keyname.size (), OVER_KEY_SIZE);

    gchar *truncated = truncateKeyForGSettings (keyname.c_str ());

    EXPECT_EQ (std::string (truncated).size (), MAX_GSETTINGS_KEY_SIZE);

    g_free (truncated);
}

TEST_F(CCSGSettingsTestIndependent, TestTruncateKeyForGSettingsUnder)
{
    const unsigned int UNDER_KEY_SIZE = MAX_GSETTINGS_KEY_SIZE - 1;

    std::string keyname;

    for (unsigned int i = 0; i <= UNDER_KEY_SIZE - 1; i++)
	keyname.push_back ('a');

    ASSERT_EQ (keyname.size (), UNDER_KEY_SIZE);

    gchar *truncated = truncateKeyForGSettings (keyname.c_str ());

    EXPECT_EQ (std::string (truncated).size (), UNDER_KEY_SIZE);

    g_free (truncated);
}

TEST_F(CCSGSettingsTestIndependent, TestTranslateUnderscoresToDashesForGSettings)
{
    std::string keyname ("plugin_option");

    gchar *translated = translateUnderscoresToDashesForGSettings (keyname.c_str ());

    std::string translatedKeyname (translated);
    EXPECT_EQ (translatedKeyname, std::string ("plugin-option"));

    g_free (translated);
}

TEST_F(CCSGSettingsTestIndependent, TestTranslateUpperToLowerForGSettings)
{
    gchar keyname[] = "PLUGIN-OPTION";

    translateToLowercaseForGSettings (keyname);

    EXPECT_EQ (std::string (keyname), "plugin-option");
}

TEST_F(CCSGSettingsTestIndependent, TestTranslateKeyForGSettingsNoTrunc)
{
    std::string keyname ("FoO_BaR");
    std::string expected ("foo-bar");

    CharacterWrapper translated (translateKeyForGSettings (keyname.c_str ()));

    EXPECT_EQ (std::string (translated), expected);
}

TEST_F(CCSGSettingsTestIndependent, TestTranslateKeyForGSettingsTrunc)
{
    const unsigned int OVER_KEY_SIZE = MAX_GSETTINGS_KEY_SIZE + 1;
    std::string keyname;

    for (unsigned int i = 0; i <= OVER_KEY_SIZE - 1; i++)
	keyname.push_back ('a');

    ASSERT_EQ (keyname.size (), OVER_KEY_SIZE);

    CharacterWrapper translated (translateKeyForGSettings (keyname.c_str ()));
    std::string      stringOfTranslated (translated);

    EXPECT_EQ (stringOfTranslated.size (), MAX_GSETTINGS_KEY_SIZE);
}

TEST_F(CCSGSettingsTestIndependent, TestTranslateKeyForCCS)
{
    std::string keyname ("plugin-option");

    gchar *translated = translateKeyForCCS (keyname.c_str ());

    EXPECT_EQ (std::string (translated), "plugin_option");

    free (translated);
}

struct CCSTypeIsVariantType
{
    CCSSettingType settingType;
    bool	   isVariantType;
};

class CCSGSettingsTestVariantTypeFixture :
    public ::testing::TestWithParam <CCSTypeIsVariantType>
{
    public:

	virtual void SetUp ()
	{
	    mType = GetParam ();
	}

    protected:

	CCSTypeIsVariantType mType;
};

TEST_P(CCSGSettingsTestVariantTypeFixture, TestVariantType)
{
    EXPECT_EQ (mType.isVariantType, compizconfigTypeHasVariantType (mType.settingType));
}

namespace
{
    CCSTypeIsVariantType type[TypeNum + 1] =
    {
	{ TypeBool, true },
	{ TypeInt, true },
	{ TypeFloat, true },
	{ TypeString, true },
	{ TypeColor, true },
	{ TypeAction, false }, /* Cannot read raw actions */
	{ TypeKey, false }, /* No actions in lists */
	{ TypeButton, false }, /* No actions in lists */
	{ TypeEdge, false }, /* No actions in lists */
	{ TypeBell, false }, /* No actions in lists */
	{ TypeMatch, true },
	{ TypeList, false }, /* No lists in lists */
	{ TypeNum, false }
    };
}

INSTANTIATE_TEST_CASE_P (CCSGSettingsTestVariantTypeInstantiation, CCSGSettingsTestVariantTypeFixture,
			 ValuesIn (type));

TEST_F(CCSGSettingsTestIndependent, TestDecomposeGSettingsPath)
{
    std::string compiz_gsettings_path (PROFILE_PATH_PREFIX);
    std::string fake_option_path ("PROFILENAME/plugins/PLUGINNAME");

    compiz_gsettings_path += fake_option_path;

    char *pluginName;
    unsigned int screenNum;

    ASSERT_TRUE (decomposeGSettingsPath (compiz_gsettings_path.c_str (), &pluginName, &screenNum));
    EXPECT_EQ (std::string (pluginName), "PLUGINNAME");
    EXPECT_EQ (screenNum, 0);

    g_free (pluginName);
}

TEST_F(CCSGSettingsTestIndependent, TestDecomposeGSettingsPathBadPathname)
{
    std::string compiz_gsettings_path ("org/this/path/is/wrong/");
    std::string fake_option_path ("PROFILENAME/plugins/PLUGINNAME");

    compiz_gsettings_path += fake_option_path;

    CharacterWrapper pluginName (strdup ("aaa"));
    char             *pluginNameC = pluginName;
    unsigned int screenNum = 1;

    EXPECT_FALSE (decomposeGSettingsPath (compiz_gsettings_path.c_str (), &pluginNameC, &screenNum));
    EXPECT_EQ (std::string (pluginNameC), "aaa");
    EXPECT_EQ (screenNum, 1);
}

TEST_F(CCSGSettingsTestIndependent, TestMakeCompizProfilePath)
{
    gchar *a = makeCompizProfilePath ("alpha");
    ASSERT_TRUE (a != NULL);
    EXPECT_EQ (std::string (a), "/org/compiz/profiles/alpha/");
    g_free (a);

    gchar *b = makeCompizProfilePath ("beta/");
    ASSERT_TRUE (b != NULL);
    EXPECT_EQ (std::string (b), "/org/compiz/profiles/beta/");
    g_free (b);

    gchar *c = makeCompizProfilePath ("/gamma");
    ASSERT_TRUE (c != NULL);
    EXPECT_EQ (std::string (c), "/org/compiz/profiles/gamma/");
    g_free (c);

    gchar *d = makeCompizProfilePath ("/delta");
    ASSERT_TRUE (d != NULL);
    EXPECT_EQ (std::string (d), "/org/compiz/profiles/delta/");
    g_free (d);
}

TEST_F(CCSGSettingsTestIndependent, TestMakeCompizPluginPath)
{
    gchar *x = makeCompizPluginPath ("one", "two");
    ASSERT_TRUE (x != NULL);
    EXPECT_EQ (std::string (x), "/org/compiz/profiles/one/plugins/two/");
    g_free (x);

    gchar *y = makeCompizPluginPath ("/three", "four/");
    ASSERT_TRUE (y != NULL);
    EXPECT_EQ (std::string (y), "/org/compiz/profiles/three/plugins/four/");
    g_free (y);
}

namespace GVariantSubtypeWrappers
{
    typedef gboolean (*IsSubtype) (GVariant *v);

    gboolean boolean (GVariant *v)
    {
	return g_variant_type_is_subtype_of (G_VARIANT_TYPE_BOOLEAN, g_variant_get_type (v));
    }

    gboolean bell (GVariant *v)
    {
	return boolean (v);
    }

    gboolean string (GVariant *v)
    {
	return g_variant_type_is_subtype_of (G_VARIANT_TYPE_STRING, g_variant_get_type (v));
    }

    gboolean match (GVariant *v)
    {
	return string (v);
    }

    gboolean color (GVariant *v)
    {
	return string (v);
    }

    gboolean key (GVariant *v)
    {
	return string (v);
    }

    gboolean button (GVariant *v)
    {
	return string (v);
    }

    gboolean edge (GVariant *v)
    {
	return string (v);
    }

    gboolean integer (GVariant *v)
    {
	return g_variant_type_is_subtype_of (G_VARIANT_TYPE_INT32, g_variant_get_type (v));
    }

    gboolean doubleprecision (GVariant *v)
    {
	return g_variant_type_is_subtype_of (G_VARIANT_TYPE_DOUBLE, g_variant_get_type (v));
    }

    gboolean list (GVariant *v)
    {
	return g_variant_type_is_array (g_variant_get_type (v));
    }

    gboolean unknown (GVariant *)
    {
	return FALSE;
    }
}

struct ArrayVariantInfo
{
    GVariantSubtypeWrappers::IsSubtype func;
    CCSSettingType		       ccsType;
    const char			       *vType;
};

namespace
{
    const char *vBoolean = "b";
    const char *vString = "s";
    const char *vInt = "i";
    const char *vDouble = "d";
    const char *vArray = "as";
    const char *vUnknown = "";

    ArrayVariantInfo arrayVariantInfo[] =
    {
	{ &GVariantSubtypeWrappers::boolean, TypeBool, vBoolean },
	{ &GVariantSubtypeWrappers::bell, TypeBell, vBoolean },
	{ &GVariantSubtypeWrappers::string, TypeString, vString },
	{ &GVariantSubtypeWrappers::match, TypeMatch, vString },
	{ &GVariantSubtypeWrappers::color, TypeColor, vString },
	{ &GVariantSubtypeWrappers::key, TypeKey, vString },
	{ &GVariantSubtypeWrappers::button, TypeButton, vString },
	{ &GVariantSubtypeWrappers::edge, TypeEdge, vString },
	{ &GVariantSubtypeWrappers::integer, TypeInt, vInt },
	{ &GVariantSubtypeWrappers::doubleprecision, TypeFloat, vDouble },
	{ &GVariantSubtypeWrappers::list, TypeList, vArray },
	{ &GVariantSubtypeWrappers::unknown, TypeNum, vUnknown }
    };
}

class CCSGSettingsTestArrayVariantSubTypeFixture :
    public ::testing::TestWithParam <ArrayVariantInfo>
{
    public:

	virtual void SetUp ()
	{
	    mAVInfo = GetParam ();
	}

	virtual void TearDown ()
	{
	    g_variant_unref (v);
	}

    protected:

	ArrayVariantInfo mAVInfo;
	GVariant	 *v;
};

TEST_P(CCSGSettingsTestArrayVariantSubTypeFixture, TestArrayVariantValidForCCSTypeBool)
{
    v = g_variant_new (vBoolean, TRUE);

    EXPECT_EQ ((*mAVInfo.func) (v), variantIsValidForCCSType (v, mAVInfo.ccsType));
}

TEST_P(CCSGSettingsTestArrayVariantSubTypeFixture, TestArrayVariantValidForCCSTypeString)
{
    v = g_variant_new (vString, "foo");

    EXPECT_EQ ((*mAVInfo.func) (v), variantIsValidForCCSType (v, mAVInfo.ccsType));
}

TEST_P(CCSGSettingsTestArrayVariantSubTypeFixture, TestArrayVariantValidForCCSTypeInt)
{
    v = g_variant_new (vInt, 1);

    EXPECT_EQ ((*mAVInfo.func) (v), variantIsValidForCCSType (v, mAVInfo.ccsType));
}

TEST_P(CCSGSettingsTestArrayVariantSubTypeFixture, TestArrayVariantValidForCCSTypeDouble)
{
    v = g_variant_new (vDouble, 2.0);

    EXPECT_EQ ((*mAVInfo.func) (v), variantIsValidForCCSType (v, mAVInfo.ccsType));
}

TEST_P(CCSGSettingsTestArrayVariantSubTypeFixture, TestArrayVariantValidForCCSTypeArray)
{
    GVariantBuilder vb;

    g_variant_builder_init (&vb, G_VARIANT_TYPE (vArray));

    g_variant_builder_add (&vb, "s", "foo");
    g_variant_builder_add (&vb, "s", "bar");

    v = g_variant_builder_end (&vb);

    EXPECT_EQ ((*mAVInfo.func) (v), variantIsValidForCCSType (v, mAVInfo.ccsType));
}

INSTANTIATE_TEST_CASE_P (CCSGSettingsTestArrayVariantSubTypeInstantiation, CCSGSettingsTestArrayVariantSubTypeFixture,
			 ValuesIn (arrayVariantInfo));

class CCSGSettingsTestPluginsWithSetKeysGVariantSetup :
    public CCSGSettingsTestIndependent
{
    public:

	virtual void SetUp ()
	{
	    builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));

	    g_variant_builder_add (builder, "s", "foo");
	    g_variant_builder_add (builder, "s", "bar");

	    writtenPlugins = g_variant_new ("as", builder);

	    g_variant_builder_unref (builder);

	    newWrittenPlugins = NULL;
	    newWrittenPluginsSize = 0;
	}

	virtual void TearDown ()
	{
	    g_variant_unref (writtenPlugins);
	    g_strfreev (newWrittenPlugins);
	}

    protected:

	GVariantBuilder *builder;
	GVariant	*writtenPlugins;
	char     **newWrittenPlugins;
	gsize    newWrittenPluginsSize;

};

TEST_F(CCSGSettingsTestPluginsWithSetKeysGVariantSetup, TestAppendToPluginsWithSetKeysListNewItem)
{
    EXPECT_TRUE (appendToPluginsWithSetKeysList ("plugin",
						 writtenPlugins,
						 &newWrittenPlugins,
						 &newWrittenPluginsSize));

    EXPECT_EQ (newWrittenPluginsSize, 3);
    EXPECT_EQ (std::string (newWrittenPlugins[0]), std::string ("foo"));
    EXPECT_EQ (std::string (newWrittenPlugins[1]), std::string ("bar"));
    EXPECT_EQ (std::string (newWrittenPlugins[2]), std::string ("plugin"));
}

TEST_F(CCSGSettingsTestPluginsWithSetKeysGVariantSetup, TestAppendToPluginsWithSetKeysListExistingItem)
{
    EXPECT_FALSE (appendToPluginsWithSetKeysList ("foo",
						  writtenPlugins,
						  &newWrittenPlugins,
						  &newWrittenPluginsSize));

    EXPECT_EQ (newWrittenPluginsSize, 2);
    EXPECT_EQ (std::string (newWrittenPlugins[0]), std::string ("foo"));
    EXPECT_EQ (std::string (newWrittenPlugins[1]), std::string ("bar"));
}

class CCSGSettingsTestGObjectListWithProperty :
    public CCSGSettingsTestIndependent
{
    public:

	virtual void SetUp ()
	{
	    g_type_init ();

	    objectSchemaList = NULL;
	}

	virtual void TearDown ()
	{
	    GList *iter = objectSchemaList;

	    while (iter)
	    {
		g_object_unref ((GObject *) iter->data);
		iter = g_list_next (iter);
	    }

	    g_list_free (objectSchemaList);
	    objectSchemaList = NULL;
	}

	CCSGSettingsWrapGSettings * AddObjectWithSchemaName (const std::string &schemaName)
	{
	    CCSGSettingsWrapGSettings *wrapGSettingsObject =
		    compizconfig_gsettings_wrap_gsettings_new (COMPIZCONFIG_GSETTINGS_TYPE_MOCK_WRAP_GSETTINGS, schemaName.c_str ());
	    objectSchemaList = g_list_append (objectSchemaList, wrapGSettingsObject);

	    return wrapGSettingsObject;
	}

    protected:

	GList *objectSchemaList;
};

TEST_F(CCSGSettingsTestGObjectListWithProperty, TestFindExistingObjectWithSchema)
{
    GObject *obj = reinterpret_cast <GObject *> (AddObjectWithSchemaName ("foo"));

    EXPECT_EQ (findObjectInListWithPropertySchemaName ("foo", objectSchemaList), obj);
}

TEST_F(CCSGSettingsTestGObjectListWithProperty, TestNoFindNonexistingObjectWithSchema)
{
    AddObjectWithSchemaName ("bar");

    EXPECT_EQ (NULL, findObjectInListWithPropertySchemaName ("foo", objectSchemaList));
}

class CCSGSettingsTestFindSettingLossy :
    public CCSGSettingsTestIndependent
{
    public:

	virtual void SetUp ()
	{
	    settingList = NULL;
	}

	virtual void TearDown ()
	{
	    ccsSettingListFree (settingList, TRUE);
	    settingList = NULL;
	}

	CCSSetting * AddMockSettingWithNameAndType (char	      *name,
						    CCSSettingType    type)
	{
	    CCSSetting *mockSetting = ccsMockSettingNew ();

	    settingList = ccsSettingListAppend (settingList, mockSetting);

	    CCSSettingGMock *gmock = reinterpret_cast <CCSSettingGMock *> (ccsObjectGetPrivate (mockSetting));

	    ON_CALL (*gmock, getName ()).WillByDefault (Return (name));
	    ON_CALL (*gmock, getType ()).WillByDefault (Return (type));

	    return mockSetting;
	}

	void ExpectNameCallOnSetting (CCSSetting *setting)
	{
	    CCSSettingGMock *gs = reinterpret_cast <CCSSettingGMock *> (ccsObjectGetPrivate (setting));
	    EXPECT_CALL (*gs, getName ());
	}

	void ExpectTypeCallOnSetting (CCSSetting *setting)
	{
	    CCSSettingGMock *gs = reinterpret_cast <CCSSettingGMock *> (ccsObjectGetPrivate (setting));
	    EXPECT_CALL (*gs, getType ());
	}

    protected:

	CCSSettingList settingList;
};

TEST_F(CCSGSettingsTestFindSettingLossy, TestFilterAvailableSettingsByType)
{
    char *name1 = strdup ("foo_bar_baz");
    char *name2 = strdup ("foo_bar-baz");

    CCSSetting *s1 = AddMockSettingWithNameAndType (name1, TypeInt);
    CCSSetting *s2 = AddMockSettingWithNameAndType (name2, TypeBool);

    ExpectTypeCallOnSetting (s1);
    ExpectTypeCallOnSetting (s2);

    CCSSettingList filteredList = filterAllSettingsMatchingType (TypeInt, settingList);

    /* Needs to be expressed in terms of a boolean expression */
    ASSERT_TRUE (filteredList);
    EXPECT_EQ (ccsSettingListLength (filteredList), 1);
    EXPECT_EQ (filteredList->data, s1);
    EXPECT_NE (filteredList->data, s2);
    EXPECT_EQ (NULL, filteredList->next);

    free (name2);
    free (name1);

    ccsSettingListFree (filteredList, FALSE);
}

TEST_F(CCSGSettingsTestFindSettingLossy, TestFilterAvailableSettingsMatchingPartOfStringIgnoringDashesUnderscoresAndCase)
{
    char *name1 = strdup ("foo_bar_baz_bob");
    char *name2 = strdup ("FOO_bar_baz_fred");
    char *name3 = strdup ("foo-bar");

    CCSSetting *s1 = AddMockSettingWithNameAndType (name1, TypeInt);
    CCSSetting *s2 = AddMockSettingWithNameAndType (name2, TypeInt);
    CCSSetting *s3 = AddMockSettingWithNameAndType (name3, TypeInt);

    ExpectNameCallOnSetting (s1);
    ExpectNameCallOnSetting (s2);
    ExpectNameCallOnSetting (s3);

    CCSSettingList filteredList = filterAllSettingsMatchingPartOfStringIgnoringDashesUnderscoresAndCase ("foo-bar-baz",
													 settingList);

    ASSERT_TRUE (filteredList);
    ASSERT_EQ (ccsSettingListLength (filteredList), 2);
    EXPECT_EQ (filteredList->data, s1);
    EXPECT_NE (filteredList->data, s3);
    ASSERT_TRUE (filteredList->next);
    EXPECT_EQ (filteredList->next->data, s2);
    EXPECT_NE (filteredList->data, s3);
    EXPECT_EQ (NULL, filteredList->next->next);

    free (name1);
    free (name2);
    free (name3);

    ccsSettingListFree (filteredList, FALSE);
}

TEST_F(CCSGSettingsTestFindSettingLossy, TestAttemptToFindCCSSettingFromLossyNameSuccess)
{
    char *name1 = strdup ("foo_bar_baz_bob");
    char *name2 = strdup ("FOO_bar_baz_bob-fred");
    char *name3 = strdup ("foo-bar");
    char *name4 = strdup ("FOO_bar_baz_bob-fred");

    CCSSetting *s1 = AddMockSettingWithNameAndType (name1, TypeInt);
    CCSSetting *s2 = AddMockSettingWithNameAndType (name2, TypeInt);
    CCSSetting *s3 = AddMockSettingWithNameAndType (name3, TypeInt);
    CCSSetting *s4 = AddMockSettingWithNameAndType (name4, TypeString);

    ExpectNameCallOnSetting (s1);
    ExpectNameCallOnSetting (s2);
    ExpectNameCallOnSetting (s3);

    ExpectTypeCallOnSetting (s1);
    ExpectTypeCallOnSetting (s2);
    ExpectTypeCallOnSetting (s3);
    ExpectTypeCallOnSetting (s4);

    CCSSetting *found = attemptToFindCCSSettingFromLossyName (settingList, "foo-bar-baz-bob-fred", TypeInt);

    EXPECT_EQ (found, s2);
    EXPECT_NE (found, s1);
    EXPECT_NE (found, s3);
    EXPECT_NE (found, s4);

    free (name1);
    free (name2);
    free (name3);
    free (name4);
}

TEST_F(CCSGSettingsTestFindSettingLossy, TestAttemptToFindCCSSettingFromLossyNameFailTooMany)
{
    char *name1 = strdup ("foo_bar_baz_bob");
    char *name2 = strdup ("FOO_bar_baz_bob-fred");
    char *name3 = strdup ("FOO_BAR_baz_bob-fred");
    char *name4 = strdup ("foo-bar");
    char *name5 = strdup ("FOO_bar_baz_bob-fred");

    CCSSetting *s1 = AddMockSettingWithNameAndType (name1, TypeInt);
    CCSSetting *s2 = AddMockSettingWithNameAndType (name2, TypeInt);
    CCSSetting *s3 = AddMockSettingWithNameAndType (name3, TypeInt);
    CCSSetting *s4 = AddMockSettingWithNameAndType (name4, TypeInt);
    CCSSetting *s5 = AddMockSettingWithNameAndType (name5, TypeString);

    ExpectNameCallOnSetting (s1);
    ExpectNameCallOnSetting (s2);
    ExpectNameCallOnSetting (s3);
    ExpectNameCallOnSetting (s4);

    ExpectTypeCallOnSetting (s1);
    ExpectTypeCallOnSetting (s2);
    ExpectTypeCallOnSetting (s3);
    ExpectTypeCallOnSetting (s4);
    ExpectTypeCallOnSetting (s5);

    CCSSetting *found = attemptToFindCCSSettingFromLossyName (settingList, "foo-bar-baz-bob-fred", TypeInt);

    ASSERT_FALSE (found);
    EXPECT_NE (found, s1);
    EXPECT_NE (found, s2);
    EXPECT_NE (found, s3);
    EXPECT_NE (found, s4);
    EXPECT_NE (found, s5);

    free (name1);
    free (name2);
    free (name3);
    free (name4);
    free (name5);
}

TEST_F(CCSGSettingsTestFindSettingLossy, TestAttemptToFindCCSSettingFromLossyNameFailNoMatches)
{
    char *name1 = strdup ("foo_bar_baz_bob");
    char *name2 = strdup ("FOO_bar_baz_bob-richard");
    char *name3 = strdup ("foo-bar");
    char *name4 = strdup ("FOO_bar_baz_bob-richard");

    CCSSetting *s1 = AddMockSettingWithNameAndType (name1, TypeInt);
    CCSSetting *s2 = AddMockSettingWithNameAndType (name2, TypeInt);
    CCSSetting *s3 = AddMockSettingWithNameAndType (name3, TypeInt);
    CCSSetting *s4 = AddMockSettingWithNameAndType (name4, TypeString);

    ExpectNameCallOnSetting (s1);
    ExpectNameCallOnSetting (s2);
    ExpectNameCallOnSetting (s3);

    ExpectTypeCallOnSetting (s1);
    ExpectTypeCallOnSetting (s2);
    ExpectTypeCallOnSetting (s3);
    ExpectTypeCallOnSetting (s4);

    CCSSetting *found = attemptToFindCCSSettingFromLossyName (settingList, "foo-bar-baz-bob-fred", TypeInt);

    ASSERT_FALSE (found);
    EXPECT_NE (found, s1);
    EXPECT_NE (found, s2);
    EXPECT_NE (found, s3);
    EXPECT_NE (found, s4);

    free (name1);
    free (name2);
    free (name3);
    free (name4);
}

namespace
{
    class GListContainerEqualityInterface
    {
	public:

	    virtual ~GListContainerEqualityInterface () {}

	    virtual bool operator== (GList *) const = 0;
	    bool operator!= (GList *l) const
	    {
		return !(*this == l);
	    }

	    friend bool operator== (GList *lhs, const GListContainerEqualityInterface &rhs);
	    friend bool operator!= (GList *lhs, const GListContainerEqualityInterface &rhs);
    };

    bool
    operator== (GList *lhs, const GListContainerEqualityInterface &rhs)
    {
	return rhs == lhs;
    }

    bool
    operator!= (GList *lhs, const GListContainerEqualityInterface &rhs)
    {
	return !(rhs == lhs);
    }

    class GListContainerEqualityBase :
	public GListContainerEqualityInterface
    {
	public:

	    typedef boost::function <GList * (void)> PopulateFunc;

	    GListContainerEqualityBase (const PopulateFunc &populateGList) :
		mList (populateGList ())
	    {
	    }

	    GListContainerEqualityBase (const GListContainerEqualityBase &other) :
		mList (g_list_copy (other.mList))
	    {
	    }

	    GListContainerEqualityBase &
	    operator= (GListContainerEqualityBase &other)
	    {
		if (this == &other)
		    return *this;

		GListContainerEqualityBase tmp (other);

		tmp.swap (*this);
		return *this;
	    }

	    void swap (GListContainerEqualityBase &other)
	    {
		std::swap (this->mList, other.mList);
	    }

	    bool operator== (GList *other) const
	    {
		unsigned int numInternal = g_list_length (mList);
		unsigned int numOther = g_list_length (other);

		if (numInternal != numOther)
		    return false;

		GList *iterOther = other;
		GList *iterInternal = mList;

		for (unsigned int i = 0; i < numInternal; i++)
		{
		    if (static_cast <CCSSettingType> (GPOINTER_TO_INT (iterOther->data)) !=
			static_cast <CCSSettingType> (GPOINTER_TO_INT (iterInternal->data)))
			return false;

		    iterOther = g_list_next (iterOther);
		    iterInternal = g_list_next (iterInternal);
		}

		return true;
	    }

	    ~GListContainerEqualityBase ()
	    {
		g_list_free (mList);
	    }

	private:

	    GList *mList;
    };

    GList * populateBoolCCSTypes ()
    {
	GList *ret = NULL;
	ret = g_list_append (ret, GINT_TO_POINTER (static_cast <int> (TypeBool)));
	ret = g_list_append (ret, GINT_TO_POINTER (static_cast <int> (TypeBell)));
	return ret;
    }

    GList * populateStringCCSTypes ()
    {
	GList *ret = NULL;
	ret = g_list_append (ret, GINT_TO_POINTER (static_cast <int> (TypeString)));
	ret = g_list_append (ret, GINT_TO_POINTER (static_cast <int> (TypeColor)));
	ret = g_list_append (ret, GINT_TO_POINTER (static_cast <int> (TypeKey)));
	ret = g_list_append (ret, GINT_TO_POINTER (static_cast <int> (TypeButton)));
	ret = g_list_append (ret, GINT_TO_POINTER (static_cast <int> (TypeEdge)));
	ret = g_list_append (ret, GINT_TO_POINTER (static_cast <int> (TypeMatch)));
	return ret;
    }

    GList * populateIntCCSTypes ()
    {
	GList *ret = NULL;
	ret = g_list_append (ret, GINT_TO_POINTER (static_cast <int> (TypeInt)));
	return ret;
    }

    GList * populateDoubleCCSTypes ()
    {
	GList *ret = NULL;
	ret = g_list_append (ret, GINT_TO_POINTER (static_cast <int> (TypeFloat)));
	return ret;
    }

    GList * populateArrayCCSTypes ()
    {
	GList *ret = NULL;
	ret = g_list_append (ret, GINT_TO_POINTER (static_cast <int> (TypeList)));
	return ret;
    }

    struct GListContainerVariantTypeWrapper
    {
	const gchar *variantType;
	GListContainerEqualityBase listOfCCSTypes;
    };

    GListContainerVariantTypeWrapper variantTypeToListOfCCSTypes[] =
    {
	{ "b", GListContainerEqualityBase (boost::bind (populateBoolCCSTypes)) },
	{ "s", GListContainerEqualityBase (boost::bind (populateStringCCSTypes)) },
	{ "i", GListContainerEqualityBase (boost::bind (populateIntCCSTypes)) },
	{ "d", GListContainerEqualityBase (boost::bind (populateDoubleCCSTypes)) },
	{ "a", GListContainerEqualityBase (boost::bind (populateArrayCCSTypes)) }
    };
}

class CCSGSettingsTestVariantTypeToCCSTypeListFixture :
    public ::testing::TestWithParam <GListContainerVariantTypeWrapper>
{
    public:

	CCSGSettingsTestVariantTypeToCCSTypeListFixture () :
	    mListContainer (GetParam ())
	{
	}

    protected:

	GListContainerVariantTypeWrapper mListContainer;
};

TEST_P(CCSGSettingsTestVariantTypeToCCSTypeListFixture, TestVariantTypesInListTemplate)
{
    GList *potentialTypeList = variantTypeToPossibleSettingType (mListContainer.variantType);
    EXPECT_EQ (mListContainer.listOfCCSTypes, potentialTypeList);

    g_list_free (potentialTypeList);
}

INSTANTIATE_TEST_CASE_P(CCSGSettingsTestVariantTypeToCCSTypeListInstantiation, CCSGSettingsTestVariantTypeToCCSTypeListFixture,
			ValuesIn (variantTypeToListOfCCSTypes));

TEST_F(CCSGSettingsTestIndependent, TestGetNameForCCSSetting)
{
    CCSSetting *setting = ccsMockSettingNew ();
    CCSSettingGMock *gmock = (CCSSettingGMock *) ccsObjectGetPrivate (setting);
    char *rawSettingName = strdup ("FoO_BaR");
    char *properSettingName = translateKeyForGSettings (rawSettingName);

    EXPECT_CALL (*gmock, getName ()).WillOnce (Return (rawSettingName));

    char *translatedSettingName = getNameForCCSSetting (setting);

    EXPECT_EQ (std::string (translatedSettingName), std::string (properSettingName));
    EXPECT_NE (std::string (translatedSettingName), std::string (rawSettingName));

    free (translatedSettingName);
    free (properSettingName);
    free (rawSettingName);

    ccsSettingUnref (setting);
}

TEST_F(CCSGSettingsTestIndependent, TestReadVariantIsValidNULL)
{
    EXPECT_FALSE (checkReadVariantIsValid (NULL, TypeNum, "foo/bar"));
}

TEST_F(CCSGSettingsTestIndependent, TestReadVariantIsValidTypeBad)
{
    GVariant *v = g_variant_new ("i", 1);

    EXPECT_FALSE (checkReadVariantIsValid (v, TypeString, "foo/bar"));

    g_variant_unref (v);
}

TEST_F(CCSGSettingsTestIndependent, TestReadVariantIsValidTypeGood)
{
    GVariant *v = g_variant_new ("i", 1);

    EXPECT_TRUE (checkReadVariantIsValid (v, TypeInt, "foo/bar"));

    g_variant_unref (v);
}

typedef CCSSettingValueList (*ReadValueListOfDataTypeFunc) (GVariantIter *, guint nItems, CCSSetting *setting);

class ReadListValueTypeTestParam
{
    public:

	typedef boost::function <CCSSettingValueList (GVariantIter *,
						      guint nItems,
						      CCSSetting *setting,
						      CCSObjectAllocationInterface *allocator)> ReadValueListFunc;
	typedef boost::function <GVariant * ()> GVariantPopulator;
	typedef boost::function <CCSSettingValueList (CCSSetting *)> CCSSettingValueListPopulator;

	ReadListValueTypeTestParam (const ReadValueListFunc &readFunc,
				    const GVariantPopulator &variantPopulator,
				    const CCSSettingValueListPopulator &listPopulator,
				    const CCSSettingType    &type) :
	    mReadFunc (readFunc),
	    mVariantPopulator (variantPopulator),
	    mListPopulator (listPopulator),
	    mType (type)
	{
	}

	CCSSettingValueList read (GVariantIter *iter,
				  guint        nItems,
				  CCSSetting   *setting,
				  CCSObjectAllocationInterface *allocator) const
	{
	    return mReadFunc (iter, nItems, setting, allocator);
	}

	boost::shared_ptr <GVariant> populateVariant () const
	{
	    return boost::shared_ptr <GVariant> (mVariantPopulator (), boost::bind (g_variant_unref, _1));
	}

	boost::shared_ptr <_CCSSettingValueList> populateList (CCSSetting *setting) const
	{
	    return boost::shared_ptr <_CCSSettingValueList> (mListPopulator (setting), boost::bind (ccsSettingValueListFree, _1, TRUE));
	}

	CCSSettingType type () const { return mType; }

    private:

	ReadValueListFunc mReadFunc;
	GVariantPopulator mVariantPopulator;
	CCSSettingValueListPopulator mListPopulator;
	CCSSettingType    mType;

};

namespace compizconfig
{
    namespace test
    {
	namespace impl
	{
	    namespace populators
	    {
		namespace variant
		{
		    GVariant * boolean ()
		    {
			GVariantBuilder vb;
			g_variant_builder_init (&vb, G_VARIANT_TYPE ("ab"));
			g_variant_builder_add (&vb, "b", boolValues[0]);
			g_variant_builder_add (&vb, "b", boolValues[1]);
			g_variant_builder_add (&vb, "b", boolValues[2]);
			return g_variant_builder_end (&vb);
		    }

		    GVariant * integer ()
		    {
			GVariantBuilder vb;
			g_variant_builder_init (&vb, G_VARIANT_TYPE ("ai"));
			g_variant_builder_add (&vb, "i", intValues[0]);
			g_variant_builder_add (&vb, "i", intValues[1]);
			g_variant_builder_add (&vb, "i", intValues[2]);
			return g_variant_builder_end (&vb);
		    }

		    GVariant * doubleprecision ()
		    {
			GVariantBuilder vb;
			g_variant_builder_init (&vb, G_VARIANT_TYPE ("ad"));
			g_variant_builder_add (&vb, "d", floatValues[0]);
			g_variant_builder_add (&vb, "d", floatValues[1]);
			g_variant_builder_add (&vb, "d", floatValues[2]);
			return g_variant_builder_end (&vb);
		    }

		    GVariant * string ()
		    {
			GVariantBuilder vb;
			g_variant_builder_init (&vb, G_VARIANT_TYPE ("as"));
			g_variant_builder_add (&vb, "s", stringValues[0]);
			g_variant_builder_add (&vb, "s", stringValues[1]);
			g_variant_builder_add (&vb, "s", stringValues[2]);
			return g_variant_builder_end (&vb);
		    }

		    GVariant * color ()
		    {
			GVariantBuilder vb;

			CharacterWrapper s1 (ccsColorToString (&(getColorValueList ()[0])));
			CharacterWrapper s2 (ccsColorToString (&(getColorValueList ()[1])));
			CharacterWrapper s3 (ccsColorToString (&(getColorValueList ()[2])));

			char * c1 = s1;
			char * c2 = s2;
			char * c3 = s3;

			g_variant_builder_init (&vb, G_VARIANT_TYPE ("as"));
			g_variant_builder_add (&vb, "s", c1);
			g_variant_builder_add (&vb, "s", c2);
			g_variant_builder_add (&vb, "s", c3);
			return g_variant_builder_end (&vb);
		    }
		}
	    }
	}
    }
}

class CCSGSettingsTestReadListValueTypes :
    public ::testing::TestWithParam <ReadListValueTypeTestParam>
{
};

TEST_P(CCSGSettingsTestReadListValueTypes, TestListValueGoodAllocation)
{
    boost::shared_ptr <GVariant>   variant = GetParam ().populateVariant ();
    boost::shared_ptr <CCSSetting> mockSetting (ccsNiceMockSettingNew (), boost::bind (ccsFreeMockSetting, _1));
    NiceMock <CCSSettingGMock>     *gmockSetting = reinterpret_cast <NiceMock <CCSSettingGMock> *> (ccsObjectGetPrivate (mockSetting.get ()));

    ON_CALL (*gmockSetting, getType ()).WillByDefault (Return (TypeList));

    CCSSettingInfo			    info =
    {
	.forList =
	{
	    GetParam ().type (),
	    NULL
	}
    };

    boost::shared_ptr <_CCSSettingValueList> valueList (GetParam ().populateList (mockSetting.get ()));
    GVariantIter			    iter;

    g_variant_iter_init (&iter, variant.get ());

    ON_CALL (*gmockSetting, getInfo ()).WillByDefault (Return (&info));
    ON_CALL (*gmockSetting, getDefaultValue ()).WillByDefault (ReturnNull ());

    boost::shared_ptr <_CCSSettingValueList> readValueList (GetParam ().read (&iter,
									      3,
									      mockSetting.get (),
									      &ccsDefaultObjectAllocator),
							    boost::bind (ccsSettingValueListFree, _1, TRUE));

    EXPECT_TRUE (ccsCompareLists (valueList.get (), readValueList.get (), info.forList));
}

TEST_P(CCSGSettingsTestReadListValueTypes, TestListValueThroughListValueDispatch)
{
    boost::shared_ptr <GVariant>   variant = GetParam ().populateVariant ();
    boost::shared_ptr <CCSSetting> mockSetting (ccsNiceMockSettingNew (), boost::bind (ccsFreeMockSetting, _1));
    NiceMock <CCSSettingGMock>     *gmockSetting = reinterpret_cast <NiceMock <CCSSettingGMock> *> (ccsObjectGetPrivate (mockSetting.get ()));

    ON_CALL (*gmockSetting, getType ()).WillByDefault (Return (TypeList));

    CCSSettingInfo			    info =
    {
	.forList =
	{
	    GetParam ().type (),
	    NULL
	}
    };

    boost::shared_ptr <_CCSSettingValueList> valueList (GetParam ().populateList (mockSetting.get ()));
    GVariantIter			    iter;

    g_variant_iter_init (&iter, variant.get ());

    ON_CALL (*gmockSetting, getInfo ()).WillByDefault (Return (&info));
    ON_CALL (*gmockSetting, getDefaultValue ()).WillByDefault (ReturnNull ());

    boost::shared_ptr <_CCSSettingValueList> readValueList (readListValue (variant.get (),
									   mockSetting.get (),
									   &ccsDefaultObjectAllocator),
							    boost::bind (ccsSettingValueListFree, _1, TRUE));

    EXPECT_TRUE (ccsCompareLists (valueList.get (), readValueList.get (), info.forList));
}

TEST_P(CCSGSettingsTestReadListValueTypes, TestListValueBadAllocation)
{
    boost::shared_ptr <GVariant>   variant = GetParam ().populateVariant ();
    boost::shared_ptr <CCSSetting> mockSetting (ccsNiceMockSettingNew (), boost::bind (ccsFreeMockSetting, _1));
    NiceMock <CCSSettingGMock>     *gmockSetting = reinterpret_cast <NiceMock <CCSSettingGMock> *> (ccsObjectGetPrivate (mockSetting.get ()));
    StrictMock <ObjectAllocationGMock> objectAllocationGMock;
    FailingObjectAllocation fakeFailingAllocator;

    CCSObjectAllocationInterface failingAllocatorGMock = failingAllocator;
    failingAllocatorGMock.allocator = reinterpret_cast <void *> (&objectAllocationGMock);

    ON_CALL (*gmockSetting, getType ()).WillByDefault (Return (TypeList));

    GVariantIter			    iter;
    g_variant_iter_init (&iter, variant.get ());

    EXPECT_CALL (objectAllocationGMock, calloc_ (_, _)).WillOnce (Invoke (&fakeFailingAllocator,
								       &FailingObjectAllocation::calloc_));

    boost::shared_ptr <_CCSSettingValueList> readValueList (GetParam ().read (&iter,
									      3,
									      mockSetting.get (),
									      &failingAllocatorGMock));

    EXPECT_THAT (readValueList.get (), IsNull ());
}

namespace variant_populators = compizconfig::test::impl::populators::variant;
namespace list_populators = compizconfig::test::impl::populators::list;

ReadListValueTypeTestParam readListValueTypeTestParam[] =
{
    ReadListValueTypeTestParam (boost::bind (readBoolListValue, _1, _2, _3, _4),
				boost::bind (variant_populators::boolean),
				boost::bind (list_populators::boolean, _1),
				TypeBool),
    ReadListValueTypeTestParam (boost::bind (readIntListValue, _1, _2, _3, _4),
				boost::bind (variant_populators::integer),
				boost::bind (list_populators::integer, _1),
				TypeInt),
    ReadListValueTypeTestParam (boost::bind (readFloatListValue, _1, _2, _3, _4),
				boost::bind (variant_populators::doubleprecision),
				boost::bind (list_populators::doubleprecision, _1),
				TypeFloat),
    ReadListValueTypeTestParam (boost::bind (readStringListValue, _1, _2, _3, _4),
				boost::bind (variant_populators::string),
				boost::bind (list_populators::string, _1),
				TypeString),
    ReadListValueTypeTestParam (boost::bind (readColorListValue, _1, _2, _3, _4),
				boost::bind (variant_populators::color),
				boost::bind (list_populators::color, _1),
				TypeColor)
};

INSTANTIATE_TEST_CASE_P (TestGSettingsReadListValueParameterized, CCSGSettingsTestReadListValueTypes,
			 ::testing::ValuesIn (readListValueTypeTestParam));
