#include <cstring>

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
using ::testing::Eq;

class TestGSettingsWrapperWithMemoryBackendEnv :
    public ::testing::Test
{
    public:

	TestGSettingsWrapperWithMemoryBackendEnv () :
	    mockSchema ("org.compiz.mock"),
	    mockPath ("/org/compiz/mock/mock")
	{
	}

	virtual CCSObjectAllocationInterface * GetAllocator () = 0;

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
	boost::shared_ptr <CCSGSettingsWrapper> wrapper;
	GSettings   *settings;
};

class TestGSettingsWrapperWithMemoryBackendEnvGoodAllocator :
    public TestGSettingsWrapperWithMemoryBackendEnv
{
    protected:

	CCSObjectAllocationInterface * GetAllocator ()
	{
	    return &ccsDefaultObjectAllocator;
	}
};

class TestGSettingsWrapperWithMemoryBackendEnvGoodAllocatorAutoInit :
    public TestGSettingsWrapperWithMemoryBackendEnvGoodAllocator
{
    public:

	virtual void SetUp ()
	{
	    TestGSettingsWrapperWithMemoryBackendEnvGoodAllocator::SetUp ();

	    wrapper.reset (ccsGSettingsWrapperNewForSchemaWithPath (mockSchema.c_str (),
								    mockPath.c_str (),
								    GetAllocator ()),
			   boost::bind (ccsFreeGSettingsWrapper, _1));

	    ASSERT_THAT (wrapper.get (), NotNull ());

	    settings = ccsGSettingsWrapperGetGSettings (wrapper.get ());

	    ASSERT_THAT (settings, NotNull ());
	}
};

TEST_F (TestGSettingsWrapperWithMemoryBackendEnvGoodAllocator, TestWrapperConstruction)
{
    boost::shared_ptr <CCSGSettingsWrapper> wrapper (ccsGSettingsWrapperNewForSchemaWithPath (mockSchema.c_str (),
											      mockPath.c_str (),
											      &ccsDefaultObjectAllocator),
						     boost::bind (ccsFreeGSettingsWrapper, _1));

    EXPECT_THAT (wrapper.get (), NotNull ());
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnvGoodAllocator, TestGetGSettingsWrapper)
{
    boost::shared_ptr <CCSGSettingsWrapper> wrapper (ccsGSettingsWrapperNewForSchemaWithPath (mockSchema.c_str (),
											      mockPath.c_str (),
											      &ccsDefaultObjectAllocator),
						     boost::bind (ccsFreeGSettingsWrapper, _1));

    ASSERT_THAT (wrapper.get (), NotNull ());
    EXPECT_THAT (ccsGSettingsWrapperGetGSettings (wrapper.get ()), NotNull ());
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnvGoodAllocatorAutoInit, TestSetValueOnWrapper)
{
    const unsigned int VALUE = 2;
    const std::string KEY ("integer-setting");
    boost::shared_ptr <GVariant> variant (g_variant_new ("i", VALUE, NULL),
					  boost::bind (g_variant_unref, _1));
    ccsGSettingsWrapperSetValue (wrapper.get (), KEY.c_str (), variant.get ());

    boost::shared_ptr <GVariant> value (g_settings_get_value (settings, KEY.c_str ()),
					boost::bind (g_variant_unref, _1));

    int v = g_variant_get_int32 (value.get ());
    EXPECT_EQ (VALUE, v);
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnvGoodAllocatorAutoInit, TestGetValueOnWrapper)
{
    const double VALUE = 3.0;
    const std::string KEY ("float-setting");
    boost::shared_ptr <GVariant> variant (g_variant_new ("d", VALUE, NULL),
					  boost::bind (g_variant_unref, _1));
    g_settings_set_value (settings, KEY.c_str (), variant.get ());
    boost::shared_ptr <GVariant> value (ccsGSettingsWrapperGetValue (wrapper.get (),
								     KEY.c_str ()),
					boost::bind (g_variant_unref, _1));

    double v = (double) g_variant_get_double (value.get ());
    EXPECT_EQ (VALUE, v);
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnvGoodAllocatorAutoInit, TestResetKeyOnWrapper)
{
    const char * DEFAULT = "";
    const char * VALUE = "foo";
    const std::string KEY ("string-setting");
    boost::shared_ptr <GVariant> variant (g_variant_new ("s", VALUE, NULL),
					  boost::bind (g_variant_unref, _1));
    ccsGSettingsWrapperSetValue (wrapper.get (), KEY.c_str (), variant.get ());

    boost::shared_ptr <GVariant> value (g_settings_get_value (settings, KEY.c_str ()),
					boost::bind (g_variant_unref, _1));

    gsize      length;
    std::string v (g_variant_get_string (value.get (), &length));
    ASSERT_EQ (strlen (VALUE), length);
    ASSERT_THAT (v, Eq (VALUE));

    /* g_settings_reset appears to unref the value,
     * so we need to keep it alive */
    g_variant_ref (value.get ());

    ccsGSettingsWrapperResetKey (wrapper.get (), KEY.c_str ());

    value.reset (g_settings_get_value (settings, KEY.c_str ()),
		 boost::bind (g_variant_unref, _1));

    v = std::string (g_variant_get_string (value.get (), &length));
    ASSERT_EQ (strlen (DEFAULT), length);
    ASSERT_THAT (v, Eq (DEFAULT));
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnvGoodAllocatorAutoInit, TestListKeysOnWrapper)
{
    const char * EXPECTED_KEYS[] =
    {
	"bell-setting",
	"bool-list-setting",
	"boolean-setting",
	"button-setting",
	"color-list-setting",
	"color-setting",
	"edge-setting",
	"float-list-setting",
	"float-setting",
	"int-list-setting",
	"integer-setting",
	"key-setting",
	"match-list-setting",
	"match-setting",
	"string-list-setting",
	"string-setting"
    };

    boost::shared_ptr <gchar *> keys (ccsGSettingsWrapperListKeys (wrapper.get ()),
				      boost::bind (g_strfreev, _1));

    ASSERT_EQ (g_strv_length (keys.get ()),
	       sizeof (EXPECTED_KEYS) /
	       sizeof (EXPECTED_KEYS[0]));
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnvGoodAllocatorAutoInit, TestGetSchemaName)
{
    EXPECT_THAT (ccsGSettingsWrapperGetSchemaName (wrapper.get ()), Eq (mockSchema));
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnvGoodAllocatorAutoInit, TestGetPath)
{
    EXPECT_THAT (ccsGSettingsWrapperGetPath (wrapper.get ()), Eq (mockPath));
}

namespace signal_test
{
    class VerificationInterface
    {
	public:

	    virtual ~VerificationInterface () {}
	    virtual void Verify (GSettings *settings, gchar *keyname) = 0;
    };

    class VerificationMock :
	public VerificationInterface
    {
	public:

	    MOCK_METHOD2 (Verify, void (GSettings *settings, gchar *keyname));
    };


    void dummyChangedSignal (GSettings   *s,
			     gchar       *keyName,
			     gpointer    user_data)
    {
	VerificationInterface *verifier = reinterpret_cast <VerificationInterface *> (user_data);
	verifier->Verify (s, keyName);
    }
}

TEST_F (TestGSettingsWrapperWithMemoryBackendEnvGoodAllocatorAutoInit, TestConnectToChangedSignal)
{
    FAIL ();
}
