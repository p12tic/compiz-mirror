#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <gio/gio.h>

#include "backend-conformance-config.h"
#include "gsettings-mock-schemas-config.h"

#include <ccs.h>
#include <compizconfig_backend_concept_test.h>

#include <gsettings_util.h>
#include <ccs_gsettings_backend_interface.h>

#include "gtest_shared_autodestroy.h"
#include "test_gsettings_tests.h"

using ::testing::AtLeast;
using ::testing::Pointee;
using ::testing::ReturnNull;

CCSIntegration *
ccsMockIntegrationBackendNew (CCSObjectAllocationInterface *ai);

void
ccsMockIntegrationBackendFree (CCSIntegration *integration);

class CCSIntegrationGMockInterface
{
    public:

	virtual ~CCSIntegrationGMockInterface () {}

	virtual CCSIntegratedSetting * getIntegratedOptionIndex (const char *pluginName, const char *settingName) = 0;
	virtual Bool readOptionIntoSetting (CCSContext *context, CCSSetting *setting, CCSIntegratedSetting *) = 0;
	virtual void writeOptionFromSetting (CCSContext *context, CCSSetting *setting, CCSIntegratedSetting *) = 0;
	virtual void updateIntegratedSettings (CCSContext *context, CCSIntegratedSettingList settingList) = 0;
	virtual void disallowIntegratedWrites () = 0;
	virtual void allowIntegratedWrites () = 0;
};

class CCSIntegrationGMock :
    public CCSIntegrationGMockInterface
{
    public:

	MOCK_METHOD2 (getIntegratedOptionIndex, CCSIntegratedSetting * (const char *, const char *));
	MOCK_METHOD3 (readOptionIntoSetting, Bool (CCSContext *, CCSSetting *, CCSIntegratedSetting *));
	MOCK_METHOD3 (writeOptionFromSetting, void (CCSContext *, CCSSetting *, CCSIntegratedSetting *));
	MOCK_METHOD2 (updateIntegratedSettings, void (CCSContext *, CCSIntegratedSettingList));
	MOCK_METHOD0 (disallowIntegratedWrites, void ());
	MOCK_METHOD0 (allowIntegratedWrites, void ());


	CCSIntegrationGMock (CCSIntegration *integration) :
	    mIntegration (integration)
	{
	}

	CCSIntegration *
	getIntegrationBackend ()
	{
	    return mIntegration;
	}

    public:

	static
	CCSIntegratedSetting * CCSIntegrationGetIntegratedOptionIndex (CCSIntegration *integration,
								       const char *pluginName,
								       const char *settingName)
	{
	    return reinterpret_cast <CCSIntegrationGMockInterface *> (ccsObjectGetPrivate (integration))->getIntegratedOptionIndex (pluginName, settingName);
	}

	static
	Bool CCSIntegrationReadOptionIntoSetting (CCSIntegration *integration,
						  CCSContext		  *context,
						  CCSSetting		  *setting,
						  CCSIntegratedSetting *integrated)
	{
	    return reinterpret_cast <CCSIntegrationGMockInterface *> (ccsObjectGetPrivate (integration))->readOptionIntoSetting (context, setting, integrated);
	}

	static
	void CCSIntegrationWriteSettingIntoOption  (CCSIntegration *integration,
						    CCSContext		   *context,
						    CCSSetting		   *setting,
						    CCSIntegratedSetting *integrated)
	{
	    return reinterpret_cast <CCSIntegrationGMockInterface *> (ccsObjectGetPrivate (integration))->writeOptionFromSetting (context, setting, integrated);
	}

	static void CCSIntegrationUpdateIntegratedSettings (CCSIntegration *integration,
							    CCSContext	   *context,
							    CCSIntegratedSettingList settingList)
	{
	    return reinterpret_cast <CCSIntegrationGMockInterface *> (ccsObjectGetPrivate (integration))->updateIntegratedSettings (context, settingList);
	}

	static
	void ccsFreeIntegration (CCSIntegration *integration)
	{
	    ccsMockIntegrationBackendFree (integration);
	}

	static void ccsIntegrationDisallowIntegratedWrites (CCSIntegration *integration)
	{
	    reinterpret_cast <CCSIntegrationGMockInterface *> (ccsObjectGetPrivate (integration))->disallowIntegratedWrites ();
	}

	static void ccsIntegrationAllowIntegratedWrites (CCSIntegration *integration)
	{
	    reinterpret_cast <CCSIntegrationGMockInterface *> (ccsObjectGetPrivate (integration))->allowIntegratedWrites ();
	}

    private:

	CCSIntegration *mIntegration;
};

const CCSIntegrationInterface mockIntegrationBackendInterface =
{
    CCSIntegrationGMock::CCSIntegrationGetIntegratedOptionIndex,
    CCSIntegrationGMock::CCSIntegrationReadOptionIntoSetting,
    CCSIntegrationGMock::CCSIntegrationWriteSettingIntoOption,
    CCSIntegrationGMock::CCSIntegrationUpdateIntegratedSettings,
    CCSIntegrationGMock::ccsIntegrationDisallowIntegratedWrites,
    CCSIntegrationGMock::ccsIntegrationAllowIntegratedWrites,
    CCSIntegrationGMock::ccsFreeIntegration
};

CCSIntegration *
ccsMockIntegrationBackendNew (CCSObjectAllocationInterface *ai)
{
    CCSIntegration *integration = reinterpret_cast <CCSIntegration *> ((*ai->calloc_) (ai->allocator, 1, sizeof (CCSIntegration)));

    if (!integration)
	return NULL;

    CCSIntegrationGMock *gmockBackend = new CCSIntegrationGMock (integration);

    ccsObjectInit (integration, ai);
    ccsObjectSetPrivate (integration, (CCSPrivate *) gmockBackend);
    ccsObjectAddInterface (integration, (const CCSInterface *) &mockIntegrationBackendInterface, GET_INTERFACE_TYPE (CCSIntegrationInterface));

    ccsObjectRef (integration);

    return integration;
}

void
ccsMockIntegrationBackendFree (CCSIntegration *integration)
{
    CCSIntegrationGMock *gmockBackend = reinterpret_cast <CCSIntegrationGMock *> (ccsObjectGetPrivate (integration));

    delete gmockBackend;

    ccsObjectSetPrivate (integration, NULL);
    ccsObjectFinalize (integration);
    (*integration->object.object_allocation->free_) (integration->object.object_allocation->allocator, integration);
}

class CCSGSettingsStorageEnv :
    public CCSSettingsConceptTestEnvironmentInterface,
    public CCSGSettingsMemoryBackendTestingEnv
{
    public:

	typedef boost::shared_ptr <GVariant> GVariantShared;

	virtual void SetUp ()
	{
	    SetUpEnv ();
	}

	virtual void TearDown ()
	{
	    TearDownEnv ();
	}

	CCSGSettingsStorageEnv (CCSGSettingsWrapper *settings,
				const std::string   &profileName) :
	    mSettings (settings),
	    profileName (profileName)
	{
	}

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

	Bool ReadBoolAtKey (const std::string &plugin,
			    const std::string &key)
	{
	    GVariantShared variant (ReadVariantAtKeyToShared (plugin,
							      key,
							      TypeBool));
	    return readBoolFromVariant (variant.get ());
	}

	int ReadIntegerAtKey (const std::string &plugin,
					const std::string &key)
	{
	    GVariantShared variant (ReadVariantAtKeyToShared (plugin,
							      key,
							      TypeInt));
	    return readIntFromVariant (variant.get ());
	}

	float ReadFloatAtKey (const std::string &plugin,
				      const std::string &key)
	{
	    GVariantShared variant (ReadVariantAtKeyToShared (plugin,
							      key,
							      TypeFloat));
	    return readFloatFromVariant (variant.get ());
	}

	const char * ReadStringAtKey (const std::string &plugin,
				      const std::string &key)
	{
	    GVariantShared variant (ReadVariantAtKeyToShared (plugin,
							      key,
							      TypeString));
	    return readStringFromVariant (variant.get ());
	}

	CCSSettingColorValue ReadColorAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    Bool success = FALSE;
	    GVariantShared variant (ReadVariantAtKeyToShared (plugin,
							      key,
							      TypeColor));
	    CCSSettingColorValue value = readColorFromVariant (variant.get (), &success);
	    EXPECT_TRUE (success);
	    return value;
	}

	CCSSettingKeyValue ReadKeyAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    Bool success = FALSE;
	    GVariantShared variant (ReadVariantAtKeyToShared (plugin,
							      key,
							      TypeKey));
	    CCSSettingKeyValue value = readKeyFromVariant (variant.get (), &success);
	    EXPECT_TRUE (success);
	    return value;
	}

	CCSSettingButtonValue ReadButtonAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    Bool success = FALSE;
	    GVariantShared variant (ReadVariantAtKeyToShared (plugin,
							      key,
							      TypeButton));
	    CCSSettingButtonValue value = readButtonFromVariant (variant.get (), &success);
	    EXPECT_TRUE (success);
	    return value;
	}

	unsigned int ReadEdgeAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    GVariantShared variant (ReadVariantAtKeyToShared (plugin,
							      key,
							      TypeEdge));
	    return readEdgeFromVariant (variant.get ());
	}

	const char * ReadMatchAtKey (const std::string &plugin,
				     const std::string &key)
	{
	    GVariantShared variant (ReadVariantAtKeyToShared (plugin,
							      key,
							      TypeMatch));
	    return readStringFromVariant (variant.get ());
	}

	Bool ReadBellAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    GVariantShared variant (ReadVariantAtKeyToShared (plugin,
							      key,
							      TypeBell));
	    return readBoolFromVariant (variant.get ());
	}

	CCSSettingValueList ReadListAtKey (const std::string &plugin,
					   const std::string &key,
					   CCSSetting        *setting)
	{
	    GVariantShared variant (ReadVariantAtKeyToShared (plugin,
							      key,
							      TypeList));
	    return readListValue (variant.get (), setting, &ccsDefaultObjectAllocator);
	}

    private:

	GVariantShared
	ReadVariantAtKeyToShared (const std::string   &plugin,
				  const std::string   &key,
				  CCSSettingType	  type)
	{
	    CharacterWrapper translatedKey (translateKeyForGSettings (key.c_str ()));
	    CharacterWrapper pluginPath (makeCompizPluginPath (profileName.c_str (),
							       plugin.c_str ()));

	    GVariant *rawVariant = getVariantAtKey (mSettings,
						    translatedKey,
						    pluginPath,
						    type);

	    GVariantShared shared (AutoDestroy (rawVariant, g_variant_unref));



	    return shared;
	}

	CCSGSettingsWrapper  *mSettings;
	std::string	     profileName;


};

class CCSGSettingsBackendEnv :
    public CCSBackendConceptTestEnvironmentInterface,
    public CCSGSettingsMemoryBackendTestingEnv
{
    private:

	virtual void SetUp ()
	{
	}

	virtual void TearDown ()
	{
	}

    public:

	CCSGSettingsBackendEnv () :
	    pluginToMatch ("mock")
	{
	    g_type_init ();
	}

	/* A wrapper to prevent signals from being added */
	static void connectToSignalWrapper (CCSBackend *backend, CCSGSettingsWrapper *wrapper)
	{
	};

	const CCSBackendInfo * GetInfo ()
	{
	    return &gsettingsBackendInfo;
	}

	CCSBackend * SetUp (CCSContext *context, CCSContextGMock *gmockContext)
	{
	    CCSGSettingsBackendInterface *overloadedInterface = NULL;

	    SetUpEnv ();
	    g_setenv ("LIBCOMPIZCONFIG_BACKEND_PATH", BACKEND_BINARY_PATH, true);

	    mContext = context;

	    std::string path ("gsettings");

	    mBackend = reinterpret_cast <CCSDynamicBackend *> (ccsOpenBackend (&ccsDefaultInterfaceTable, mContext, path.c_str ()));

	    EXPECT_TRUE (mBackend);

	    mGSettingsBackend = ccsDynamicBackendGetRawBackend (mBackend);


	    CCSBackendInitFunc backendInit = (GET_INTERFACE (CCSBackendInterface, mBackend))->backendInit;

	    if (backendInit)
		(*backendInit) ((CCSBackend *) mBackend, mContext);

	    /* Set the new integration, drop our reference on it */
	    CCSIntegration *integration = ccsMockIntegrationBackendNew (&ccsDefaultObjectAllocator);
	    CCSIntegrationGMock *gmockIntegration = reinterpret_cast <CCSIntegrationGMock *> (ccsObjectGetPrivate (integration));

	    EXPECT_CALL (*gmockIntegration, getIntegratedOptionIndex (_, _)).WillRepeatedly (ReturnNull ());

	    ccsBackendSetIntegration ((CCSBackend *) mBackend, integration);
	    ccsIntegrationUnref (integration);

	    overloadedInterface = GET_INTERFACE (CCSGSettingsBackendInterface, mGSettingsBackend);
	    overloadedInterface->gsettingsBackendConnectToChangedSignal = CCSGSettingsBackendEnv::connectToSignalWrapper;

	    mSettings = ccsGSettingsGetSettingsObjectForPluginWithPath (mGSettingsBackend, "mock",
									CharacterWrapper (makeCompizPluginPath (profileName.c_str (), "mock")),
									mContext);

	    mStorage.reset (new CCSGSettingsStorageEnv (mSettings, profileName));

	    ON_CALL (*gmockContext, getProfile ()).WillByDefault (Return (profileName.c_str ()));

	    return (CCSBackend *) mBackend;
	}

	void TearDown (CCSBackend *)
	{
	    GVariantBuilder noProfilesBuilder;
	    g_variant_builder_init (&noProfilesBuilder, G_VARIANT_TYPE ("as"));
	    g_variant_builder_add (&noProfilesBuilder, "s", "Default");
	    GVariant *noProfiles = g_variant_builder_end (&noProfilesBuilder);

	    GVariantBuilder pluginKeysBuilder;
	    g_variant_builder_init (&pluginKeysBuilder, G_VARIANT_TYPE ("as"));
	    g_variant_builder_add (&pluginKeysBuilder, "s", "mock");
	    GVariant *pluginKeys = g_variant_builder_end (&pluginKeysBuilder);

	    ccsGSettingsBackendUnsetAllChangedPluginKeysInProfile (mGSettingsBackend,
								   mContext,
								   pluginKeys,
								   ccsGSettingsBackendGetCurrentProfile (
								       mGSettingsBackend));
	    ccsGSettingsBackendClearPluginsWithSetKeys (mGSettingsBackend);
	    ccsGSettingsBackendSetExistingProfiles (mGSettingsBackend, noProfiles);
	    ccsGSettingsBackendSetCurrentProfile (mGSettingsBackend, "Default");

	    ccsFreeDynamicBackend (mBackend);

	    ccsGSettingsWrapperUnref (mSettings);
	    mStorage.reset ();

	    g_variant_unref (pluginKeys);

	    TearDownEnv ();
	    g_unsetenv ("LIBCOMPIZCONFIG_BACKEND_PATH");
	}

	void AddProfile (const std::string &profile)
	{
	    ccsGSettingsBackendAddProfile (mGSettingsBackend, profile.c_str ());
	}

	void SetGetExistingProfilesExpectation (CCSContext *context,
						CCSContextGMock *gmockContext)
	{
	    EXPECT_CALL (*gmockContext, getProfile ()).Times (AtLeast (1));
	}

	void SetDeleteProfileExpectation (const std::string &profileToDelete,
					  CCSContext *context,
					  CCSContextGMock *gmockContext)
	{
	    EXPECT_CALL (*gmockContext, getProfile ()).Times (AtLeast (1));
	}

	void SetReadInitExpectation (CCSContext *context,
				     CCSContextGMock *gmockContext)
	{
	    EXPECT_CALL (*gmockContext, getProfile ()).WillOnce (Return (profileName.c_str ()));
	}

	void SetReadDoneExpectation (CCSContext *context,
				     CCSContextGMock *gmockContext)
	{
	}

	void SetWriteInitExpectation (CCSContext *context,
				      CCSContextGMock *gmockContext)
	{
	    EXPECT_CALL (*gmockContext, getProfile ()).WillOnce (Return (profileName.c_str ()));
	}

	void SetWriteDoneExpectation (CCSContext *context,
				      CCSContextGMock *gmockContext)
	{
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
	    EXPECT_CALL (*gmockSetting, isReadableByBackend ()).WillRepeatedly (Return (TRUE));

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
	    EXPECT_CALL (*gmockSetting, isReadableByBackend ()).WillRepeatedly (Return (TRUE));

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

	void WriteBoolAtKey (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value)
	{
	    mStorage->WriteBoolAtKey (plugin, key, value);
	}

	void WriteIntegerAtKey (const std::string &plugin,
				const std::string &key,
				const VariantTypes &value)
	{
	    mStorage->WriteIntegerAtKey (plugin, key, value);
	}

	void WriteFloatAtKey (const std::string &plugin,
				      const std::string &key,
				      const VariantTypes &value)
	{
	    mStorage->WriteFloatAtKey (plugin, key, value);
	}

	void WriteStringAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value)
	{
	    mStorage->WriteStringAtKey (plugin, key, value);
	}

	void WriteColorAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value)
	{
	    mStorage->WriteColorAtKey (plugin, key, value);
	}

	void WriteKeyAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value)
	{
	    mStorage->WriteKeyAtKey (plugin, key, value);
	}

	void WriteButtonAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value)
	{
	    mStorage->WriteButtonAtKey (plugin, key, value);
	}

	void WriteEdgeAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value)
	{
	    mStorage->WriteEdgeAtKey (plugin, key, value);
	}

	void WriteMatchAtKey (const std::string &plugin,
				      const std::string &key,
				      const VariantTypes &value)
	{
	    mStorage->WriteMatchAtKey (plugin, key, value);
	}

	void WriteBellAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value)
	{
	    mStorage->WriteBellAtKey (plugin, key, value);
	}

	void WriteListAtKey (const std::string &plugin,
				     const std::string &key,
				     const VariantTypes &value)
	{
	    mStorage->WriteListAtKey (plugin, key, value);
	}

	Bool ReadBoolAtKey (const std::string &plugin,
			    const std::string &key)
	{
	    return mStorage->ReadBoolAtKey (plugin, key);
	}

	int ReadIntegerAtKey (const std::string &plugin,
					const std::string &key)
	{
	    return mStorage->ReadIntegerAtKey (plugin, key);
	}

	float ReadFloatAtKey (const std::string &plugin,
				      const std::string &key)
	{
	    return mStorage->ReadFloatAtKey (plugin, key);
	}

	const char * ReadStringAtKey (const std::string &plugin,
				      const std::string &key)
	{
	    return mStorage->ReadStringAtKey (plugin, key);
	}

	CCSSettingColorValue ReadColorAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    return mStorage->ReadColorAtKey (plugin, key);
	}

	CCSSettingKeyValue ReadKeyAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    return mStorage->ReadKeyAtKey (plugin, key);
	}

	CCSSettingButtonValue ReadButtonAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    return mStorage->ReadButtonAtKey (plugin, key);
	}

	unsigned int ReadEdgeAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    return mStorage->ReadEdgeAtKey (plugin, key);
	}

	const char * ReadMatchAtKey (const std::string &plugin,
				     const std::string &key)
	{
	    return mStorage->ReadMatchAtKey (plugin, key);
	}

	Bool ReadBellAtKey (const std::string &plugin,
				       const std::string &key)
	{
	    return mStorage->ReadBellAtKey (plugin, key);
	}

	CCSSettingValueList ReadListAtKey (const std::string &plugin,
					   const std::string &key,
					   CCSSetting        *setting)
	{
	    return mStorage->ReadListAtKey (plugin, key, setting);
	}

    private:

	CCSGSettingsWrapper  *mSettings;
	boost::shared_ptr <CCSGSettingsStorageEnv> mStorage;
	CCSContext *mContext;
	CCSDynamicBackend *mBackend;
	CCSBackend		   *mGSettingsBackend;
	std::string pluginToMatch;

	static const std::string profileName;
};

const std::string CCSGSettingsBackendEnv::profileName = "mock";

INSTANTIATE_TEST_CASE_P (CCSGSettingsBackendConcept, CCSBackendConformanceTestReadWrite,
			 compizconfig::test::GenerateTestingParametersForBackendInterface <CCSGSettingsBackendEnv> ());

INSTANTIATE_TEST_CASE_P (CCSGSettingsBackendConcept, CCSBackendConformanceTestInfo,
			 compizconfig::test::GenerateTestingEnvFactoryBackendInterface <CCSGSettingsBackendEnv> ());

INSTANTIATE_TEST_CASE_P (CCSGSettingsBackendConcept, CCSBackendConformanceTestInitFiniFuncs,
			 compizconfig::test::GenerateTestingEnvFactoryBackendInterface <CCSGSettingsBackendEnv> ());

INSTANTIATE_TEST_CASE_P (CCSGSettingsBackendConcept, CCSBackendConformanceTestProfileHandling,
			 compizconfig::test::GenerateTestingEnvFactoryBackendInterface <CCSGSettingsBackendEnv> ());
