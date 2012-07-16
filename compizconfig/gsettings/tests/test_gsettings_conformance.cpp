#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <gsettings_util.h>

#include <gio/gio.h>

#include "backend-conformance-config.h"

#include <ccs.h>
#include <compizconfig_backend_concept_test.h>

#include <iostream>

using ::testing::AtLeast;
using ::testing::Pointee;

namespace
{
const std::string MOCK_SCHEMA ("org.compiz.mock");
const std::string MOCK_PATH (MOCK_SCHEMA_PATH);
}

class CCSGSettingsBackendEnv :
    public CCSBackendConceptTestEnvironmentInterface
{
    public:

	CCSGSettingsBackendEnv () :
	    pluginToMatch ("mock")
	{
	    g_type_init ();
	}

	/* A wrapper to prevent signals from being added */
	static void connectToSignalWrapper (CCSBackend *backend, GObject *object)
	{
	};

	CCSBackend * SetUp (CCSContext *context, CCSContextGMock *gmockContext)
	{
	    CCSBackendInterface *interface = NULL;
	    Bool		fallback   = FALSE;

	    g_setenv ("GSETTINGS_SCHEMA_DIR", MOCK_PATH.c_str (), true);

	    /* For some reason a number of tests fail when using
	     * this GSettings backend */
	    //g_setenv ("GSETTINGS_BACKEND", "memory", true);
	    g_setenv ("LIBCOMPIZCONFIG_BACKEND_PATH", BACKEND_BINARY_PATH, true);

	    mSettings = g_settings_new_with_path (MOCK_SCHEMA.c_str (), makeCompizPluginPath ("mock", "mock"));
	    mContext = context;

	    std::string path ("gsettings");

	    void *dlhand = ccsOpenBackend (path.c_str (), &interface, &fallback);

	    EXPECT_FALSE (fallback);
	    EXPECT_TRUE (dlhand);

	    CCSBackend *backend = ccsBackendNewWithInterface (mContext, interface, dlhand);
	    mBackend = ccsBackendWithCapabilitiesWrapBackend (&ccsDefaultInterfaceTable, backend);

	    CCSBackendInitFunc backendInit = (GET_INTERFACE (CCSBackendInterface, mBackend))->backendInit;

	    if (backendInit)
		(*backendInit) ((CCSBackend *) mBackend, mContext);

	    overloadedInterface = *(GET_INTERFACE (CCSGSettingsBackendInterface, backend));
	    overloadedInterface.gsettingsBackendConnectToChangedSignal = CCSGSettingsBackendEnv::connectToSignalWrapper;

	    ccsObjectRemoveInterface (backend, GET_INTERFACE_TYPE (CCSGSettingsBackendInterface));
	    ccsObjectAddInterface (backend, (CCSInterface *) &overloadedInterface, GET_INTERFACE_TYPE (CCSGSettingsBackendInterface));

	    return (CCSBackend *) mBackend;
	}

	void TearDown (CCSBackend *)
	{
	    g_unsetenv ("GSETTINGS_SCHEMA_DIR");
	    //g_unsetenv ("GSETTINGS_BACKEND");

	    ccsFreeBackendWithCapabilities (mBackend);

	    g_object_unref (mSettings);
	}

	void PreWrite (CCSContextGMock *gmockContext,
		       CCSPluginGMock  *gmockPlugin,
		       CCSSettingGMock *gmockSetting,
		       CCSSettingType  type)
	{
	    EXPECT_CALL (*gmockContext, getIntegrationEnabled ()).WillRepeatedly (Return (FALSE));
	    EXPECT_CALL (*gmockPlugin, getContext ()).Times (AtLeast (1));
	    EXPECT_CALL (*gmockPlugin, getName ()).Times (AtLeast (1));
	    EXPECT_CALL (*gmockSetting, getType ()).Times (AtLeast (1));
	    EXPECT_CALL (*gmockSetting, getName ()).Times (AtLeast (1));
	    EXPECT_CALL (*gmockSetting, getParent ()).Times (AtLeast (1));
	    EXPECT_CALL (*gmockSetting, getIsDefault ()).WillRepeatedly (Return (FALSE));
	}

	void PostWrite (CCSContextGMock *gmockContext,
			CCSPluginGMock  *gmockPlugin,
			CCSSettingGMock *gmockSetting,
			CCSSettingType  type) {}

	void WriteBoolAtKey (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value)
	{
	    GVariant *variant = NULL;
	    if (writeBoolToVariant (boolToBool (boost::get <bool> (value)), &variant))
		writeVariantToKey (mSettings, CharacterWrapper (translateKeyForGSettings (key.c_str ())), variant);
	    else
		throw std::exception ();
	}

	void WriteIntegerAtKey (const std::string &plugin,
				const std::string &key,
				const VariantTypes &value)
	{
	    GVariant *variant = NULL;
	    if (writeIntToVariant (boost::get <int> (value), &variant))
		writeVariantToKey (mSettings, CharacterWrapper (translateKeyForGSettings (key.c_str ())), variant);
	    else
		throw std::exception ();
	}

	void WriteFloatAtKey (const std::string &plugin,
				      const std::string &key,
				      const VariantTypes &value)
	{
	    GVariant *variant = NULL;
	    if (writeFloatToVariant (boost::get <float> (value), &variant))
		writeVariantToKey (mSettings, CharacterWrapper (translateKeyForGSettings (key.c_str ())), variant);
	    else
		throw std::exception ();
	}

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

	void PreRead (CCSContextGMock *gmockContext,
		      CCSPluginGMock  *gmockPlugin,
		      CCSSettingGMock *gmockSetting,
		      CCSSettingType  type)
	{
	    EXPECT_CALL (*gmockContext, getIntegrationEnabled ()).WillOnce (Return (FALSE));
	    EXPECT_CALL (*gmockPlugin, getContext ()).Times (AtLeast (1));
	    EXPECT_CALL (*gmockPlugin, getName ()).Times (AtLeast (1));
	    EXPECT_CALL (*gmockSetting, getType ()).Times (AtLeast (1));
	    EXPECT_CALL (*gmockSetting, getName ()).Times (AtLeast (1));
	    EXPECT_CALL (*gmockSetting, getParent ()).Times (AtLeast (1));
	    EXPECT_CALL (*gmockSetting, isReadOnly ()).WillRepeatedly (Return (FALSE));
	}

	void PostRead (CCSContextGMock *gmockContext,
		       CCSPluginGMock  *gmockPlugin,
		       CCSSettingGMock *gmockSetting,
		       CCSSettingType  type) {}

	Bool ReadBoolAtKey (const std::string &plugin,
			    const std::string &key)
	{
	    GVariant *variant = getVariantAtKey (mSettings,
						 CharacterWrapper (translateKeyForGSettings (key.c_str ())),
						 CharacterWrapper (makeCompizPluginPath ("mock", plugin.c_str ())),
						 TypeBool);
	    return readBoolFromVariant (variant);
	}

	int ReadIntegerAtKey (const std::string &plugin,
					const std::string &key)
	{
	    GVariant *variant = getVariantAtKey (mSettings,
						 CharacterWrapper (translateKeyForGSettings (key.c_str ())),
						 CharacterWrapper (makeCompizPluginPath ("mock", plugin.c_str ())),
						 TypeInt);
	    return readIntFromVariant (variant);
	}

	float ReadFloatAtKey (const std::string &plugin,
				      const std::string &key)
	{
	    GVariant *variant = getVariantAtKey (mSettings,
						 CharacterWrapper (translateKeyForGSettings (key.c_str ())),
						 CharacterWrapper (makeCompizPluginPath ("mock", plugin.c_str ())),
						 TypeInt);
	    return readFloatFromVariant (variant);
	}

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
	std::string pluginToMatch;
	CCSGSettingsBackendInterface overloadedInterface;
};

INSTANTIATE_TEST_CASE_P (CCSGSettingsBackendConcept, CCSBackendConformanceTestReadWrite,
			 compizconfig::test::GenerateTestingParametersForBackendInterface <CCSGSettingsBackendEnv> ());

