#include "test_gsettings_tests.h"
#include "gsettings.h"
#include "gsettings_mocks.h"

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
