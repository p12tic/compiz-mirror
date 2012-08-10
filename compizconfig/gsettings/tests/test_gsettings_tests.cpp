#include <tr1/tuple>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include "test_gsettings_tests.h"
#include "gsettings_shared.h"
#include "ccs_gsettings_interface.h"
#include "ccs_gsettings_wrapper_mock.h"
#include "gsettings_mocks.h"

using ::testing::Values;
using ::testing::ValuesIn;
using ::testing::Return;
using ::testing::IsNull;

TEST_P(CCSGSettingsTest, TestTestFixtures)
{
}

TEST_F(CCSGSettingsTestIndependent, TestTest)
{
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
    public ::testing::TestWithParam <CCSTypeIsVariantType>,
    public CCSGSettingsTestingEnv
{
    public:

	virtual void SetUp ()
	{
	    CCSGSettingsTestingEnv::SetUpEnv ();
	    mType = GetParam ();
	}

	virtual void TearDown ()
	{
	    CCSGSettingsTestingEnv::TearDownEnv ();
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
	{ &GVariantSubtypeWrappers::list, TypeList, vArray }
    };
}

class CCSGSettingsTestArrayVariantSubTypeFixture :
    public ::testing::TestWithParam <ArrayVariantInfo>,
    public CCSGSettingsTestingEnv
{
    public:

	virtual void SetUp ()
	{
	    CCSGSettingsTestingEnv::SetUpEnv ();
	    mAVInfo = GetParam ();
	}

	virtual void TearDown ()
	{
	    CCSGSettingsTestingEnv::TearDownEnv ();
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
	    CCSGSettingsTestIndependent::SetUp ();
	    GVariantBuilder builder;

	    g_variant_builder_init (&builder, G_VARIANT_TYPE ("as"));

	    g_variant_builder_add (&builder, "s", "foo");
	    g_variant_builder_add (&builder, "s", "bar");

	    writtenPlugins = g_variant_builder_end (&builder);

	    newWrittenPlugins = NULL;
	    newWrittenPluginsSize = 0;
	}

	virtual void TearDown ()
	{
	    g_variant_unref (writtenPlugins);
	    g_strfreev (newWrittenPlugins);
	    CCSGSettingsTestIndependent::TearDown ();
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

class CCSGSettingsTestGSettingsWrapperWithSchemaName :
    public CCSGSettingsTestIndependent
{
    public:

	typedef std::tr1::tuple <boost::shared_ptr <CCSGSettingsWrapper>, CCSGSettingsWrapperGMock *> WrapperMock;

	CCSGSettingsTestGSettingsWrapperWithSchemaName () :
	    objectSchemaGList (NULL)
	{
	    CCSGSettingsTestIndependent::SetUp ();
	}

	~CCSGSettingsTestGSettingsWrapperWithSchemaName ()
	{
	    g_list_free (objectSchemaGList);
	    CCSGSettingsTestIndependent::TearDown ();
	}

	WrapperMock
	AddObject ()
	{
	    boost::shared_ptr <CCSGSettingsWrapper> wrapper (ccsMockGSettingsWrapperNew (),
							     boost::bind (ccsGSettingsWrapperUnref, _1));
	    CCSGSettingsWrapperGMock			  *gmockWrapper = reinterpret_cast <CCSGSettingsWrapperGMock *> (ccsObjectGetPrivate (wrapper.get ()));

	    objectSchemaGList = g_list_append (objectSchemaGList, wrapper.get ());
	    objectSchemaList.push_back (wrapper);

	    return WrapperMock (wrapper, gmockWrapper);
	}

	static const std::string VALUE_FOO;
	static const std::string VALUE_BAR;
	static const std::string VALUE_BAZ;

    protected:

	GList						       *objectSchemaGList;
	std::vector <boost::shared_ptr <CCSGSettingsWrapper> > objectSchemaList;
};

const std::string CCSGSettingsTestGSettingsWrapperWithSchemaName::VALUE_FOO = "foo";
const std::string CCSGSettingsTestGSettingsWrapperWithSchemaName::VALUE_BAR = "bar";
const std::string CCSGSettingsTestGSettingsWrapperWithSchemaName::VALUE_BAZ = "baz";

TEST_F(CCSGSettingsTestGSettingsWrapperWithSchemaName, TestFindExistingObjectWithSchema)
{
    WrapperMock wr1 (AddObject ());
    WrapperMock wr2 (AddObject ());

    EXPECT_CALL (*(std::tr1::get <1> (wr1)), getSchemaName ()).WillRepeatedly (Return (VALUE_BAR.c_str ()));
    EXPECT_CALL (*(std::tr1::get <1> (wr2)), getSchemaName ()).WillRepeatedly (Return (VALUE_FOO.c_str ()));

    EXPECT_EQ (findCCSGSettingsWrapperBySchemaName (VALUE_FOO.c_str (), objectSchemaGList), (std::tr1::get <0> (wr2)).get ());
}

TEST_F(CCSGSettingsTestGSettingsWrapperWithSchemaName, TestNoFindNonexistingObjectWithSchema)
{
    WrapperMock wr1 (AddObject ());
    WrapperMock wr2 (AddObject ());

    EXPECT_CALL (*(std::tr1::get <1> (wr1)), getSchemaName ()).WillRepeatedly (Return (VALUE_BAR.c_str ()));
    EXPECT_CALL (*(std::tr1::get <1> (wr2)), getSchemaName ()).WillRepeatedly (Return (VALUE_BAZ.c_str ()));

    EXPECT_THAT (findCCSGSettingsWrapperBySchemaName (VALUE_FOO.c_str (), objectSchemaGList), IsNull ());
}

class CCSGSettingsTestFindSettingLossy :
    public CCSGSettingsTestIndependent
{
    public:

	virtual void SetUp ()
	{
	    CCSGSettingsTestIndependent::SetUp ();
	    settingList = NULL;
	}

	virtual void TearDown ()
	{
	    ccsSettingListFree (settingList, TRUE);
	    settingList = NULL;
	    CCSGSettingsTestIndependent::TearDown ();
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

	    GListContainerEqualityBase (const PopulateFunc &populateGList)
	    {
		setenv ("G_SLICE", "always-malloc", 1);
		mList = populateGList ();
		unsetenv ("G_SLICE");
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
    public ::testing::TestWithParam <GListContainerVariantTypeWrapper>,
    public CCSGSettingsTestingEnv
{
    public:

	CCSGSettingsTestVariantTypeToCCSTypeListFixture () :
	    mListContainer (GetParam ())
	{
	}

	virtual void SetUp ()
	{
	    CCSGSettingsTestingEnv::SetUpEnv ();
	}

	virtual void TearDown ()
	{
	    CCSGSettingsTestingEnv::TearDownEnv ();
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
