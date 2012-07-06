#include "test_gsettings_tests.h"
#include "gsettings.h"
#include "gsettings_mocks.h"

using ::testing::Values;
using ::testing::ValuesIn;
using ::testing::Return;

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

    size_t pos = schemaNameStr.find (std::string (COMPIZ_SCHEMA_ID) + std::string ("."), 0);

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
    std::string compiz_gsettings_path (COMPIZ);
    std::string fake_option_path ("profile/plugins/fake/screen1");

    compiz_gsettings_path += fake_option_path;

    char *pluginName;
    unsigned int screenNum;

    EXPECT_TRUE (decomposeGSettingsPath (compiz_gsettings_path.c_str (), &pluginName, &screenNum));
    EXPECT_EQ (std::string (pluginName), "fake");
    EXPECT_EQ (screenNum, 1);

    g_free (pluginName);
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

    protected:

	CCSSettingList settingList;
};

TEST_F(CCSGSettingsTestFindSettingLossy, TestFilterAvailableSettingsByType)
{
    char *name1 = strdup ("foo_bar_baz");
    char *name2 = strdup ("foo_bar-baz");

    CCSSetting *s1 = AddMockSettingWithNameAndType (name1, TypeInt);
    CCSSettingGMock *gs1 = reinterpret_cast <CCSSettingGMock *> (ccsObjectGetPrivate (s1));
    CCSSetting *s2 = AddMockSettingWithNameAndType (name2, TypeBool);
    CCSSettingGMock *gs2 = reinterpret_cast <CCSSettingGMock *> (ccsObjectGetPrivate (s2));

    EXPECT_CALL (*gs1, getType ());
    EXPECT_CALL (*gs2, getType ());

    CCSSettingList filteredList = filterAllSettingsMatchingType (TypeInt, settingList);

    /* Needs to be expressed in terms of a boolean expression */
    ASSERT_TRUE (filteredList);
    EXPECT_EQ (ccsSettingListLength (filteredList), 1);
    EXPECT_EQ (filteredList->data, s1);
    EXPECT_NE (filteredList->data, s2);
    EXPECT_EQ (NULL, filteredList->next);

    free (name2);
    free (name1);
}

TEST_F(CCSGSettingsTestFindSettingLossy, TestFilterAvailableSettingsMatchingPartOfStringIgnoringDashesUnderscoresAndCase)
{
    char *name1 = strdup ("foo_bar_baz_bob");
    char *name2 = strdup ("FOO_bar_baz_fred");
    char *name3 = strdup ("foo-bar");

    CCSSetting *s1 = AddMockSettingWithNameAndType (name1, TypeInt);
    CCSSetting *s2 = AddMockSettingWithNameAndType (name2, TypeInt);
    CCSSetting *s3 = AddMockSettingWithNameAndType (name3, TypeInt);

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
