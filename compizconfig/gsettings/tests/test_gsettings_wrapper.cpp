#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_gsettings_tests.h"
#include <ccs_gsettings_interface_wrapper.h>

class TestGSettingsWrapperWithMemoryBackendEnv :
    public ::testing::Test
{
    public:

	virtual void SetUp ()
	{
	    g_setenv ("GSETTINGS_BACKEND", "memory", 1);
	}

	virtual void TearDown ()
	{
	    g_unsetenv ("GSETTINGS_BACKEND");
	}
};

TEST_F (TestGSettingsWrapperWithMemoryBackendEnv, TestWrapperConstruction)
{
    FAIL ();
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
