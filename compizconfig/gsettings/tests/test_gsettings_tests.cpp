#include "test_gsettings_tests.h"
#include "gsettings.h"
#include "gsettings_mocks.h"

using ::testing::Values;

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
			 Values ((type[0]),
				 (type[1]),
				 (type[2]),
				 (type[3]),
				 (type[4]),
				 (type[5]),
				 (type[6]),
				 (type[7]),
				 (type[8]),
				 (type[9]),
				 (type[10]),
				 (type[11]),
				 (type[12])));

