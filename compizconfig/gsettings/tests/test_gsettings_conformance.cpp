#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <gsettings_util.h>

#include <gio/gio.h>

#include "backend-conformance-config.h"

#include <ccs.h>
#include <compizconfig_backend_concept_test.h>

#include <iostream>

namespace
{
const std::string MOCK_SCHEMA ("org.compiz.mock");
const std::string MOCK_PATH (MOCK_SCHEMA_PATH);
}

class CCSGSettingsBackendEnv :
    public CCSBackendConceptTestEnvironmentInterface
{
    public:

	CCSGSettingsBackendEnv ()
	{
	    g_type_init ();
	}

	CCSBackend * SetUp ()
	{
	    CCSBackendInterface *interface = NULL;
	    Bool		fallback   = FALSE;

	    std::cout << MOCK_PATH << std::endl;

	    g_setenv ("GSETTINGS_SCHEMA_DIR", MOCK_PATH.c_str (), true);
	    g_setenv ("GSETTINGS_BACKEND", "memory", true);

	    mSettings = g_settings_new_with_path (MOCK_SCHEMA.c_str (), makeCompizPluginPath ("mock", "mock"));
	    mContext = ccsMockContextNew ();

	    std::string path (BACKEND_BINARY_PATH "/libgsettings.so");

	    void *dlhand = ccsOpenBackend (path.c_str (), &interface, &fallback);

	    EXPECT_FALSE (fallback);
	    EXPECT_TRUE (dlhand);

	    CCSBackend *backend = ccsBackendNewWithInterface (mContext, interface, dlhand);
	    mBackend = ccsBackendWithCapabilitiesWrapBackend (&ccsDefaultInterfaceTable, backend);

	    return (CCSBackend *) mBackend;
	}

	void TearDown (CCSBackend *)
	{
	    g_unsetenv ("XDG_DATA_DIRS");
	    g_unsetenv ("GSETTINGS_BACKEND");

	    ccsFreeBackendWithCapabilities (mBackend);

	    g_object_unref (mSettings);
	    ccsFreeMockContext (mContext);
	}

	void PreWrite () {}
	void PostWrite () {}

	void WriteBoolAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) {}
	void WriteIntegerAtKey (const std::string &plugin,
					const std::string &key,
					const VariantTypes &value) {}
	void WriteFloatAtKey (const std::string &plugin,
				      const std::string &key,
				      const VariantTypes &value) {}
	void WriteStringAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) {}
	void WriteColorAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) {}
	void WriteKeyAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) {}
	void WriteButtonAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) {}
	void WriteEdgeAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) {}
	void WriteMatchAtKey (const std::string &plugin,
				      const std::string &key,
				      const VariantTypes &value) {}
	void WriteBellAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) {}
	void WriteListAtKey (const std::string &plugin,
				     const std::string &key,
				     const VariantTypes &value) {}

	void PreRead () {}
	void PostRead () {}

	Bool ReadBoolAtKey (const std::string &plugin,
				       const std::string &key) { return FALSE; }
	int ReadIntegerAtKey (const std::string &plugin,
					const std::string &key) { return 0; }
	float ReadFloatAtKey (const std::string &plugin,
				      const std::string &key) { return 0.0; }
	const char * ReadStringAtKey (const std::string &plugin,
				      const std::string &key) { return ""; }
	CCSSettingColorValue ReadColorAtKey (const std::string &plugin,
				       const std::string &key) { CCSSettingColorValue v; return v;}
	CCSSettingKeyValue ReadKeyAtKey (const std::string &plugin,
				       const std::string &key) { CCSSettingKeyValue v; return v; }
	CCSSettingButtonValue ReadButtonAtKey (const std::string &plugin,
				       const std::string &key) { CCSSettingButtonValue v; return v; }
	unsigned int ReadEdgeAtKey (const std::string &plugin,
				       const std::string &key) { return 0; }
	const char * ReadMatchAtKey (const std::string &plugin,
				     const std::string &key) { return ""; }
	Bool ReadBellAtKey (const std::string &plugin,
				       const std::string &key) { return FALSE; }
	CCSSettingValueList ReadListAtKey (const std::string &plugin,
				     const std::string &key) { return NULL; }
    private:

	GSettings  *mSettings;
	CCSContext *mContext;
	CCSBackendWithCapabilities *mBackend;
};

INSTANTIATE_TEST_CASE_P (CCSGSettingsBackendConcept, CCSBackendConformanceTest,
			 compizconfig::test::GenerateTestingParametersForBackendInterface <CCSGSettingsBackendEnv> ());

