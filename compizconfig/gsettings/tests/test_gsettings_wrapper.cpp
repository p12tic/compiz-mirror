#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <glib-object.h>

#include "test_gsettings_tests.h"
#include <gsettings-mock-schemas-config.h>
#include <ccs_gsettings_interface_wrapper.h>

using ::testing::NotNull;

class TestGSettingsWrapperWithMemoryBackendEnv :
    public ::testing::Test
{
    public:

	TestGSettingsWrapperWithMemoryBackendEnv () :
	    mockSchema ("org.compiz.mock"),
	    mockPath ("/org/compiz/mock/mock")
	{
	}

	virtual void SetUp ()
	{
	    g_setenv ("G_SLICE", "always-malloc", 1);
	    g_setenv ("GSETTINGS_SCHEMA_DIR", MOCK_PATH.c_str (), true);
	    g_setenv ("GSETTINGS_BACKEND", "memory", 1);

	    g_type_init ();
	}

	virtual void TearDown ()
	{
	    g_unsetenv ("GSETTINGS_BACKEND");
	    g_unsetenv ("GSETTINGS_SCHEMA_DIR");
	    g_unsetenv ("G_SLICE");
	}

    protected:

	std::string mockSchema;
	std::string mockPath;
};

TEST_F (TestGSettingsWrapperWithMemoryBackendEnv, TestWrapperConstruction)
{
    boost::shared_ptr <CCSGSettingsWrapper> wrapper (ccsGSettingsWrapperNewForSchemaWithPath (mockSchema.c_str (),
											      mockPath.c_str (),
											      &ccsDefaultObjectAllocator),
						     boost::bind (ccsFreeGSettingsWrapper, _1));

    EXPECT_THAT (wrapper.get (), NotNull ());
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnv, TestSetValueOnWrapper)
{
    FAIL ();
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnv, TestGetValueOnWrapper)
{
    FAIL ();
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnv, TestResetKeyOnWrapper)
{
    FAIL ();
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnv, TestListKeysOnWrapper)
{
    FAIL ();
}
