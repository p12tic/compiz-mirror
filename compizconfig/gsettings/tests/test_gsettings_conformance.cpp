#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <gio/gio.h>

#include "backend-conformance-config.h"
#include "gsettings-mock-schemas-config.h"

#include <ccs.h>
#include <compizconfig_backend_concept_test.h>

#include <gsettings_util.h>
#include <ccs_gsettings_backend_interface.h>

#include <iostream>

using ::testing::AtLeast;
using ::testing::Pointee;
using ::testing::ReturnNull;

CCSIntegrationBackend *
ccsMockIntegrationBackendNew (CCSObjectAllocationInterface *ai);

void
ccsMockIntegrationBackendFree (CCSIntegrationBackend *integration);

class CCSIntegrationBackendGMockInterface
{
    public:

	virtual ~CCSIntegrationBackendGMockInterface () {}

	virtual int getIntegratedOptionIndex (const char *pluginName, const char *settingName) = 0;
	virtual Bool readOptionIntoSetting (CCSBackend *backend, CCSContext *context, CCSSetting *setting, int index) = 0;
	virtual void writeOptionFromSetting (CCSBackend *backend, CCSContext *context, CCSSetting *setting, int index) = 0;
};

class CCSIntegrationBackendGMock :
    public CCSIntegrationBackendGMockInterface
{
    public:

	MOCK_METHOD2 (getIntegratedOptionIndex, int (const char *, const char *));
	MOCK_METHOD4 (readOptionIntoSetting, Bool (CCSBackend *, CCSContext *, CCSSetting *, int));
	MOCK_METHOD4 (writeOptionFromSetting, void (CCSBackend *, CCSContext *, CCSSetting *, int));


	CCSIntegrationBackendGMock (CCSIntegrationBackend *integration) :
	    mIntegration (integration)
	{
	}

	CCSIntegrationBackend *
	getIntegrationBackend ()
	{
	    return mIntegration;
	}

    public:

	static
	int ccsIntegrationBackendGetIntegratedOptionIndex (CCSIntegrationBackend *integration,
							   const char *pluginName,
							   const char *settingName)
	{
	    return reinterpret_cast <CCSIntegrationBackendGMockInterface *> (ccsObjectGetPrivate (integration))->getIntegratedOptionIndex (pluginName, settingName);
	}

	static
	Bool ccsIntegrationBackendReadOptionIntoSetting (CCSIntegrationBackend *integration,
							 CCSBackend		  *backend,
							 CCSContext		  *context,
							 CCSSetting		  *setting,
							 int			  index)
	{
	    return reinterpret_cast <CCSIntegrationBackendGMockInterface *> (ccsObjectGetPrivate (integration))->readOptionIntoSetting (backend, context, setting, index);
	}

	static
	void ccsIntegrationBackendWriteSettingIntoOption (CCSIntegrationBackend *integration,
							  CCSBackend		   *backend,
							  CCSContext		   *context,
							  CCSSetting		   *setting,
							  int			    index)
	{
	    return reinterpret_cast <CCSIntegrationBackendGMockInterface *> (ccsObjectGetPrivate (integration))->writeOptionFromSetting (backend, context, setting, index);
	}

	static
	void ccsFreeIntegrationBackend (CCSIntegrationBackend *integration)
	{
	    ccsMockIntegrationBackendFree (integration);
	}

    private:

	CCSIntegrationBackend *mIntegration;
};

const CCSIntegrationBackendInterface mockIntegrationBackendInterface =
{
    CCSIntegrationBackendGMock::ccsIntegrationBackendGetIntegratedOptionIndex,
    CCSIntegrationBackendGMock::ccsIntegrationBackendReadOptionIntoSetting,
    CCSIntegrationBackendGMock::ccsIntegrationBackendWriteSettingIntoOption,
    CCSIntegrationBackendGMock::ccsFreeIntegrationBackend
};

CCSIntegrationBackend *
ccsMockIntegrationBackendNew (CCSObjectAllocationInterface *ai)
{
    CCSIntegrationBackend *integration = reinterpret_cast <CCSIntegrationBackend *> ((*ai->calloc_) (ai->allocator, 1, sizeof (CCSIntegrationBackend)));

    if (!integration)
	return NULL;

    CCSIntegrationBackendGMock *gmockBackend = new CCSIntegrationBackendGMock (integration);

    ccsObjectInit (integration, ai);
    ccsObjectSetPrivate (integration, (CCSPrivate *) gmockBackend);
    ccsObjectAddInterface (integration, (const CCSInterface *) &mockIntegrationBackendInterface, GET_INTERFACE_TYPE (CCSIntegrationBackendInterface));

    ccsObjectRef (integration);

    return integration;
}

void
ccsMockIntegrationBackendFree (CCSIntegrationBackend *integration)
{
    CCSIntegrationBackendGMock *gmockBackend = reinterpret_cast <CCSIntegrationBackendGMock *> (ccsObjectGetPrivate (integration));

    delete gmockBackend;

    ccsObjectSetPrivate (integration, NULL);
    ccsObjectFinalize (integration);
    (*integration->object.object_allocation->free_) (integration->object.object_allocation->allocator, integration);
}

namespace
{
    template <typename T>
    class AutoDestroyHelper
    {
	public:

	    typedef void (*SimpleDestructor) (T *);
	    typedef boost::shared_ptr <T> TPtr;

	    AutoDestroyHelper (T *t,
			       SimpleDestructor d) :
		mShared (t, boost::bind (d, _1))
	    {
	    }

	    operator const TPtr & ()
	    {
		return mShared;
	    }

	private:

	    TPtr mShared;
    };
}

class CCSGSettingsBackendEnv :
    public CCSBackendConceptTestEnvironmentInterface
{
    public:

	typedef boost::shared_ptr <GVariant> GVariantShared;

	CCSGSettingsBackendEnv () :
	    pluginToMatch ("mock")
	{
	    g_type_init ();
	}

	/* A wrapper to prevent signals from being added */
	static void connectToSignalWrapper (CCSBackend *backend, CCSGSettingsWrapper *wrapper)
	{
	};

	static void registerGConfClientWrapper (CCSBackend *backend)
	{
	}

	static void unregisterGConfClientWrapper (CCSBackend *backend)
	{
	}

	const CCSBackendInfo * GetInfo ()
	{
	    return &gsettingsBackendInfo;
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

	    /* Set the new integration, drop our reference on it */
	    CCSIntegrationBackend *integration = ccsMockIntegrationBackendNew (&ccsDefaultObjectAllocator);
	    CCSIntegrationBackendGMock *gmockIntegration = reinterpret_cast <CCSIntegrationBackendGMock *> (ccsObjectGetPrivate (integration));

	    EXPECT_CALL (*gmockIntegration, getIntegratedOptionIndex (_, _)).WillRepeatedly (Return (-1));

	    ccsBackendSetIntegration ((CCSBackend *) mBackend, integration);
	    ccsIntegrationBackendUnref (integration);

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

	    g_variant_unref (pluginKeys);
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

	    GVariantShared shared (AutoDestroyHelper <GVariant> (rawVariant,
								 g_variant_unref));



	    return shared;
	}

	CCSGSettingsWrapper  *mSettings;
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
