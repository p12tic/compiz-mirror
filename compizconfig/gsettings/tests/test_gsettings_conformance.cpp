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
using ::testing::ReturnNull;

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

	static void registerGConfClientWrapper (CCSBackend *backend)
	{
	}

	static void unregisterGConfClientWrapper (CCSBackend *backend)
	{
	}

	CCSBackend * SetUp (CCSContext *context, CCSContextGMock *gmockContext)
	{
	    CCSGSettingsBackendInterface *overloadedInterface = NULL;

	    g_setenv ("GSETTINGS_SCHEMA_DIR", MOCK_PATH.c_str (), true);
	    g_setenv ("GSETTINGS_BACKEND", "memory", true);
	    g_setenv ("LIBCOMPIZCONFIG_BACKEND_PATH", BACKEND_BINARY_PATH, true);

	    mContext = context;

	    std::string path ("gsettings");

	    mBackend = reinterpret_cast <CCSDynamicBackend *> (ccsOpenBackend (&ccsDefaultInterfaceTable, mContext, path.c_str ()));

	    EXPECT_TRUE (mBackend);

	    mGSettingsBackend = ccsDynamicBackendGetRawBackend (mBackend);

	    CCSBackendInitFunc backendInit = (GET_INTERFACE (CCSBackendInterface, mBackend))->backendInit;

	    if (backendInit)
		(*backendInit) ((CCSBackend *) mBackend, mContext);

	    overloadedInterface = GET_INTERFACE (CCSGSettingsBackendInterface, mGSettingsBackend);
	    overloadedInterface->gsettingsBackendConnectToChangedSignal = CCSGSettingsBackendEnv::connectToSignalWrapper;
	    overloadedInterface->gsettingsBackendRegisterGConfClient = CCSGSettingsBackendEnv::registerGConfClientWrapper;
	    overloadedInterface->gsettingsBackendUnregisterGConfClient = CCSGSettingsBackendEnv::unregisterGConfClientWrapper;

	    mSettings = ccsGSettingsGetSettingsObjectForPluginWithPath (mGSettingsBackend, "mock",
									CharacterWrapper (makeCompizPluginPath (profileName.c_str (), "mock")),
									mContext);

	    ON_CALL (*gmockContext, getProfile ()).WillByDefault (Return (profileName.c_str ()));

	    return (CCSBackend *) mBackend;
	}

	void TearDown (CCSBackend *)
	{
	    g_unsetenv ("GSETTINGS_SCHEMA_DIR");
	    g_unsetenv ("GSETTINGS_BACKEND");
	    g_unsetenv ("LIBCOMPIZCONFIG_BACKEND_PATH");

	    ccsFreeDynamicBackend (mBackend);
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

	    if (type == TypeList)
		EXPECT_CALL (*gmockSetting, getDefaultValue ()).WillRepeatedly (ReturnNull ());
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
				       const VariantTypes &value)
	{
	    GVariant *variant = NULL;
	    if (writeStringToVariant (boost::get <const char *> (value), &variant))
		writeVariantToKey (mSettings, CharacterWrapper (translateKeyForGSettings (key.c_str ())), variant);
	    else
		throw std::exception ();
	}

	void WriteColorAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value)
	{
	    GVariant *variant = NULL;
	    if (writeColorToVariant (boost::get <CCSSettingColorValue> (value), &variant))
		writeVariantToKey (mSettings, CharacterWrapper (translateKeyForGSettings (key.c_str ())), variant);
	    else
		throw std::exception ();
	}

	void WriteKeyAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value)
	{
	    GVariant *variant = NULL;
	    if (writeKeyToVariant (boost::get <CCSSettingKeyValue> (value), &variant))
		writeVariantToKey (mSettings, CharacterWrapper (translateKeyForGSettings (key.c_str ())), variant);
	    else
		throw std::exception ();
	}

	void WriteButtonAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value)
	{
	    GVariant *variant = NULL;
	    if (writeButtonToVariant (boost::get <CCSSettingButtonValue> (value), &variant))
		writeVariantToKey (mSettings, CharacterWrapper (translateKeyForGSettings (key.c_str ())), variant);
	    else
		throw std::exception ();
	}

	void WriteEdgeAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value)
	{
	    GVariant *variant = NULL;
	    if (writeEdgeToVariant (boost::get <unsigned int> (value), &variant))
		writeVariantToKey (mSettings, CharacterWrapper (translateKeyForGSettings (key.c_str ())), variant);
	    else
		throw std::exception ();
	}

	void WriteMatchAtKey (const std::string &plugin,
				      const std::string &key,
				      const VariantTypes &value)
	{
	    GVariant *variant = NULL;
	    if (writeStringToVariant (boost::get <const char *> (value), &variant))
		writeVariantToKey (mSettings, CharacterWrapper (translateKeyForGSettings (key.c_str ())), variant);
	    else
		throw std::exception ();
	}

	void WriteBellAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value)
	{
	    GVariant *variant = NULL;
	    if (writeBoolToVariant (boolToBool (boost::get <bool> (value)), &variant))
		writeVariantToKey (mSettings, CharacterWrapper (translateKeyForGSettings (key.c_str ())), variant);
	    else
		throw std::exception ();
	}

	void WriteListAtKey (const std::string &plugin,
				     const std::string &key,
				     const VariantTypes &value)
	{
	    GVariant *variant = NULL;

	    const CCSListWrapper::Ptr &lw (boost::get <CCSListWrapper::Ptr> (value));

	    if (writeListValue (*lw, lw->type (), &variant))
		writeVariantToKey (mSettings, CharacterWrapper (translateKeyForGSettings (key.c_str ())), variant);
	    else
		throw std::exception ();
	}

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

	    if (type == TypeList)
	    {
		EXPECT_CALL (*gmockSetting, getInfo ()).Times (AtLeast (1));
		EXPECT_CALL (*gmockSetting, getDefaultValue ()).WillRepeatedly (ReturnNull ());
	    }
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
						 CharacterWrapper (makeCompizPluginPath (profileName.c_str (), plugin.c_str ())),
						 TypeBool);
	    return readBoolFromVariant (variant);
	}

	int ReadIntegerAtKey (const std::string &plugin,
					const std::string &key)
	{
	    GVariant *variant = getVariantAtKey (mSettings,
						 CharacterWrapper (translateKeyForGSettings (key.c_str ())),
						 CharacterWrapper (makeCompizPluginPath (profileName.c_str (), plugin.c_str ())),
						 TypeInt);
	    return readIntFromVariant (variant);
	}

	float ReadFloatAtKey (const std::string &plugin,
				      const std::string &key)
	{
	    GVariant *variant = getVariantAtKey (mSettings,
						 CharacterWrapper (translateKeyForGSettings (key.c_str ())),
						 CharacterWrapper (makeCompizPluginPath (profileName.c_str (), plugin.c_str ())),
						 TypeFloat);
	    return readFloatFromVariant (variant);
	}

	const char * ReadStringAtKey (const std::string &plugin,
				      const std::string &key)
	{
	    GVariant *variant = getVariantAtKey (mSettings,
						 CharacterWrapper (translateKeyForGSettings (key.c_str ())),
						 CharacterWrapper (makeCompizPluginPath (profileName.c_str (), plugin.c_str ())),
						 TypeString);
	    return readStringFromVariant (variant);
	}

	CCSSettingColorValue ReadColorAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    Bool success = FALSE;
	    GVariant *variant = getVariantAtKey (mSettings,
						 CharacterWrapper (translateKeyForGSettings (key.c_str ())),
						 CharacterWrapper (makeCompizPluginPath (profileName.c_str (), plugin.c_str ())),
						 TypeColor);
	    CCSSettingColorValue value = readColorFromVariant (variant, &success);
	    EXPECT_TRUE (success);
	    return value;
	}

	CCSSettingKeyValue ReadKeyAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    Bool success = FALSE;
	    GVariant *variant = getVariantAtKey (mSettings,
						 CharacterWrapper (translateKeyForGSettings (key.c_str ())),
						 CharacterWrapper (makeCompizPluginPath (profileName.c_str (), plugin.c_str ())),
						 TypeKey);
	    CCSSettingKeyValue value = readKeyFromVariant (variant, &success);
	    EXPECT_TRUE (success);
	    return value;
	}

	CCSSettingButtonValue ReadButtonAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    Bool success = FALSE;
	    GVariant *variant = getVariantAtKey (mSettings,
						 CharacterWrapper (translateKeyForGSettings (key.c_str ())),
						 CharacterWrapper (makeCompizPluginPath (profileName.c_str (), plugin.c_str ())),
						 TypeButton);
	    CCSSettingButtonValue value = readButtonFromVariant (variant, &success);
	    EXPECT_TRUE (success);
	    return value;
	}

	unsigned int ReadEdgeAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    GVariant *variant = getVariantAtKey (mSettings,
						 CharacterWrapper (translateKeyForGSettings (key.c_str ())),
						 CharacterWrapper (makeCompizPluginPath (profileName.c_str (), plugin.c_str ())),
						 TypeEdge);
	    return readEdgeFromVariant (variant);
	}

	const char * ReadMatchAtKey (const std::string &plugin,
				     const std::string &key)
	{
	    GVariant *variant = getVariantAtKey (mSettings,
						 CharacterWrapper (translateKeyForGSettings (key.c_str ())),
						 CharacterWrapper (makeCompizPluginPath (profileName.c_str (), plugin.c_str ())),
						 TypeMatch);
	    return readStringFromVariant (variant);
	}

	Bool ReadBellAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    GVariant *variant = getVariantAtKey (mSettings,
						 CharacterWrapper (translateKeyForGSettings (key.c_str ())),
						 CharacterWrapper (makeCompizPluginPath (profileName.c_str (), plugin.c_str ())),
						 TypeBell);
	    return readBoolFromVariant (variant);
	}

	CCSSettingValueList ReadListAtKey (const std::string &plugin,
					   const std::string &key,
					   CCSSetting        *setting)
	{
	    GVariant *variant = getVariantAtKey (mSettings,
						 CharacterWrapper (translateKeyForGSettings (key.c_str ())),
						 CharacterWrapper (makeCompizPluginPath (profileName.c_str (), plugin.c_str ())),
						 TypeList);
	    return readListValue (variant, setting);
	}

	void PreUpdate (CCSContextGMock *gmockContext,
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

	    if (type == TypeList)
	    {
		EXPECT_CALL (*gmockSetting, getInfo ()).Times (AtLeast (1));
		EXPECT_CALL (*gmockSetting, getDefaultValue ()).WillRepeatedly (ReturnNull ());
	    }

	    EXPECT_CALL (*gmockContext, getProfile ());
	}

	void PostUpdate (CCSContextGMock *gmockContext,
		       CCSPluginGMock  *gmockPlugin,
		       CCSSettingGMock *gmockSetting,
		       CCSSettingType  type) {}

	bool UpdateSettingAtKey (const std::string &plugin,
				 const std::string &setting)
	{
	    CharacterWrapper keyName (translateKeyForGSettings (setting.c_str ()));

	    if (updateSettingWithGSettingsKeyName (reinterpret_cast <CCSBackend *> (mGSettingsBackend), mSettings, keyName, ccsBackendUpdateSetting))
		return true;

	    return false;
	}

    private:

	GSettings  *mSettings;
	CCSContext *mContext;
	CCSDynamicBackend *mBackend;
	CCSBackend		   *mGSettingsBackend;
	std::string pluginToMatch;

	static const std::string profileName;
};

const std::string CCSGSettingsBackendEnv::profileName = "mock";

INSTANTIATE_TEST_CASE_P (CCSGSettingsBackendConcept, CCSBackendConformanceTestReadWrite,
			 compizconfig::test::GenerateTestingParametersForBackendInterface <CCSGSettingsBackendEnv> ());

