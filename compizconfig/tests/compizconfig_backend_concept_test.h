#ifndef _COMPIZCONFIG_CCS_BACKEND_CONCEPT_TEST
#define _COMPIZCONFIG_CCS_BACKEND_CONCEPT_TEST

#include <list>

#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ccs-backend.h>
#include <ccs.h>

#include <compizconfig_ccs_plugin_mock.h>
#include <compizconfig_ccs_setting_mock.h>
#include <compizconfig_ccs_context_mock.h>

using ::testing::Eq;

typedef boost::variant <bool,
			int,
			float,
			const char *,
			CCSSettingColorValue,
			CCSSettingKeyValue,
			CCSSettingButtonValue,
			CCSSettingValueList> VariantTypes;

class CCSBackendConceptTestEnvironmentInterface
{
    public:

	virtual ~CCSBackendConceptTestEnvironmentInterface () {};
	virtual CCSBackend * SetUp () = 0;
	virtual void TearDown (CCSBackend *) = 0;
	virtual void WriteBoolAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) = 0;
	virtual void WriteIntegerAtKey (const std::string &plugin,
					const std::string &key,
					const VariantTypes &value) = 0;
	virtual void WriteFloatAtKey (const std::string &plugin,
				      const std::string &key,
				      const VariantTypes &value) = 0;
	virtual void WriteStringAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) = 0;
	virtual void WriteColorAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) = 0;
	virtual void WriteKeyAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) = 0;
	virtual void WriteButtonAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) = 0;
	virtual void WriteEdgeAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) = 0;
	virtual void WriteMatchAtKey (const std::string &plugin,
				      const std::string &key,
				      const VariantTypes &value) = 0;
	virtual void WriteBellAtKey (const std::string &plugin,
				       const std::string &key,
				       const VariantTypes &value) = 0;

	virtual void WriteListAtKey (const std::string &plugin,
				     const std::string &key,
				     const VariantTypes &value) = 0;
};

namespace
{
Bool boolToBool (bool v) { return v ? TRUE : FALSE; }

void SetIntExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setInt (boost::get <int> (value), _));
}

void SetBoolExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setBool (boolToBool (boost::get <bool> (value)), _));
}

void SetBellExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setBell (boolToBool (boost::get <bool> (value)), _));
}

void SetFloatExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setFloat (boost::get <float> (value), _));
}

void SetStringExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setString (Eq (std::string (boost::get <const char *> (value))), _));
}

void SetMatchExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setMatch (Eq (std::string (boost::get <const char *> (value))), _));
}
}

class CCSBackendConceptTestParam
{
    public:

	typedef boost::shared_ptr <CCSBackendConceptTestParam> Ptr;

	typedef boost::function <void (const std::string &,
				       const std::string &,
				       const VariantTypes &)> NativeWrite;
	typedef boost::function <void (CCSSettingGMock *,
				       const VariantTypes &)> SetExpectation;

	CCSBackendConceptTestParam (const VariantTypes &value,
				    const NativeWrite &write,
				    const CCSSettingType &type,
				    const std::string &keyname,
				    const SetExpectation &setExpectation,
				    const std::string &what) :
	    mValue (value),
	    mNativeWrite (write),
	    mType (type),
	    mKeyname (keyname),
	    mSetExpectation (setExpectation),
	    mWhat (what)
	{
	}

	VariantTypes & value () { return mValue; }
	NativeWrite & nativeWrite () { return mNativeWrite; }
	CCSSettingType & type () { return mType; }
	std::string & keyname () { return mKeyname; }
	SetExpectation & setExpectation () { return mSetExpectation; }
	std::string & what () { return mWhat; }

    private:

	VariantTypes mValue;
	NativeWrite mNativeWrite;
	CCSSettingType mType;
	std::string mKeyname;
	SetExpectation mSetExpectation;
	std::string mWhat;

};

class CCSBackendConformanceTest :
    public ::testing::TestWithParam <std::tr1::tuple <CCSBackendConceptTestEnvironmentInterface *,
						      CCSBackendConceptTestParam::Ptr> >
{
    public:

	CCSBackend * GetBackend ()
	{
	    return mBackend;
	}

	void SetUp ()
	{
	    mBackend = std::tr1::get<0> (GetParam ())->SetUp ();
	}

	void TearDown ()
	{
	    std::tr1::get<0> (GetParam ())->TearDown (mBackend);

	    for (std::list <CCSContext *>::iterator it = mSpawnedContexts.begin ();
		 it != mSpawnedContexts.end ();
		 it++)
		ccsFreeMockContext (*it);

	    for (std::list <CCSPlugin *>::iterator it = mSpawnedPlugins.begin ();
		 it != mSpawnedPlugins.end ();
		 it++)
		ccsFreeMockPlugin (*it);

	    for (std::list <CCSSetting *>::iterator it = mSpawnedSettings.begin ();
		 it != mSpawnedSettings.end ();
		 it++)
		ccsFreeMockSetting (*it);
	}

    protected:

	CCSContext *
	SpawnContext ()
	{
	    CCSContext *context = ccsMockContextNew ();
	    mSpawnedContexts.push_back (context);
	    return context;
	}

	CCSPlugin *
	SpawnPlugin (const std::string &name = "")
	{
	    CCSPlugin *plugin = ccsMockPluginNew ();
	    mSpawnedPlugins.push_back (plugin);

	    CCSPluginGMock *gmockPlugin = (CCSPluginGMock *) ccsObjectGetPrivate (plugin);

	    if (!name.empty ())
		EXPECT_CALL (*gmockPlugin, getName ()).WillRepeatedly (Return ((char *) name.c_str ()));

	    return plugin;
	}

	CCSSetting *
	SpawnSetting (const std::string &name = "",
		      CCSSettingType	type = TypeNum,
		      CCSPlugin		*plugin = NULL)
	{
	    CCSSetting *setting = ccsMockSettingNew ();
	    mSpawnedSettings.push_back (setting);

	    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

	    if (!name.empty ())
		EXPECT_CALL (*gmockSetting, getName ()).WillRepeatedly (Return ((char *) name.c_str ()));

	    if (type != TypeNum)
		EXPECT_CALL (*gmockSetting, getType ()).WillRepeatedly (Return (type));

	    if (plugin)
		EXPECT_CALL (*gmockSetting, getParent ()).WillRepeatedly (Return (plugin));

	    return setting;
	}

    private:

	std::list <CCSContext *> mSpawnedContexts;
	std::list <CCSPlugin  *> mSpawnedPlugins;
	std::list <CCSSetting *> mSpawnedSettings;

	CCSBackend *mBackend;
};

TEST_P (CCSBackendConformanceTest, TestSetup)
{
}

TEST_P (CCSBackendConformanceTest, TestReadValue)
{
    SCOPED_TRACE (std::tr1::get <1> (GetParam ())->what ());

    std::string pluginName ("plugin");
    const std::string &settingName (std::tr1::get <1> (GetParam ())->keyname ());
    const VariantTypes &VALUE (std::tr1::get <1> (GetParam ())->value ());

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, std::tr1::get <1> (GetParam ())->type (), plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    std::tr1::get <1> (GetParam ())->nativeWrite () (pluginName, settingName, VALUE);
    std::tr1::get <1> (GetParam ())->setExpectation () (gmockSetting, VALUE);

    ccsBackendReadSetting (GetBackend (), context, setting);
}

#if 0

TEST_P (CCSBackendConformanceTest, TestReadInteger)
{
    std::string pluginName ("plugin");
    std::string settingName ("integer_setting");
    const int VALUE = 1;

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeInt, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteIntegerAtKey (pluginName, settingName, VALUE);

    EXPECT_CALL (*gmockSetting, setInt (VALUE, _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTest, TestReadFloat)
{
    std::string pluginName ("plugin");
    std::string settingName ("float_setting");
    const float VALUE = 1.0;

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeFloat, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteFloatAtKey (pluginName, settingName, VALUE);

    EXPECT_CALL (*gmockSetting, setFloat (VALUE, _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTest, TestReadString)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    const std::string VALUE ("foo");

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeString, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteStringAtKey (pluginName, settingName, VALUE);

    EXPECT_CALL (*gmockSetting, setString (VALUE.c_str (), _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

namespace
{
    bool
    operator== (const CCSSettingColorValue &lhs,
		const CCSSettingColorValue &rhs)
    {
	return (lhs.color.red == rhs.color.red &&
		lhs.color.green == rhs.color.green &&
		lhs.color.blue == rhs.color.blue &&
		lhs.color.alpha == lhs.color.alpha);
    }
}

TEST_P (CCSBackendConformanceTest, TestReadColor)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingColorValue VALUE;

    VALUE.color.red = 100;
    VALUE.color.blue = 100;
    VALUE.color.green = 100;
    VALUE.color.alpha = 100;

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeColor, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteColorAtKey (pluginName, settingName, VALUE);

    /* FIXME: We can't verify right now that the color returned is correct */
    EXPECT_CALL (*gmockSetting, setColor (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTest, TestReadKey)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingKeyValue VALUE;

    VALUE.keyModMask = (1 << 0) | (1 << 1);
    VALUE.keysym = 1;

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeKey, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteKeyAtKey (pluginName, settingName, VALUE);

    /* FIXME: We can't verify right now that the key returned is correct */
    EXPECT_CALL (*gmockSetting, setKey (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTest, TestReadButton)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingButtonValue VALUE;

    VALUE.buttonModMask = (1 << 0) | (1 << 1);
    VALUE.button = 1;
    VALUE.edgeMask = (1 << 0);

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeButton, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteButtonAtKey (pluginName, settingName, VALUE);

    /* FIXME: We can't verify right now that the button returned is correct */
    EXPECT_CALL (*gmockSetting, setButton (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTest, TestReadEdge)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    unsigned int VALUE;

    VALUE = 1;

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeEdge, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteEdgeAtKey (pluginName, settingName, VALUE);

    EXPECT_CALL (*gmockSetting, setEdge (VALUE, _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTest, TestReadBell)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    Bool VALUE = TRUE;

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeBell, plugin);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteBellAtKey (pluginName, settingName, VALUE);

    EXPECT_CALL (*gmockSetting, setBell (VALUE, _));

    ccsBackendReadSetting (GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTest, TestReadListBool)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingValueList VALUE;
    Bool		VBool[3] = { FALSE, TRUE, FALSE };

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeList, plugin);

    VALUE = ccsGetValueListFromBoolArray (VBool, 3, setting);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteListAtKey (pluginName, settingName, VALUE);

    /* No matcher for lists yet */
    EXPECT_CALL (*gmockSetting, setList (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);

    ccsSettingValueListFree (VALUE, FALSE);
}

TEST_P (CCSBackendConformanceTest, TestReadListInt)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingValueList VALUE;
    int		VInt[3] = { 1, 2, 3 };

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeList, plugin);

    VALUE = ccsGetValueListFromIntArray (VInt, 3, setting);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteListAtKey (pluginName, settingName, VALUE);

    /* No matcher for lists yet */
    EXPECT_CALL (*gmockSetting, setList (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);

    ccsSettingValueListFree (VALUE, FALSE);
}

TEST_P (CCSBackendConformanceTest, TestReadListFloat)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingValueList VALUE;
    float		VFloat[3] = { 1.0, 2.0, 3.0 };

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeList, plugin);

    VALUE = ccsGetValueListFromFloatArray (VFloat, 3, setting);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteListAtKey (pluginName, settingName, VALUE);

    /* No matcher for lists yet */
    EXPECT_CALL (*gmockSetting, setList (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);

    ccsSettingValueListFree (VALUE, FALSE);
}

TEST_P (CCSBackendConformanceTest, TestReadListString)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingValueList VALUE;
    const char *	VString[3] = { "foo",
				       "bar",
				       "baz" };

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeList, plugin);

    VALUE = ccsGetValueListFromStringArray (VString, 3, setting);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteListAtKey (pluginName, settingName, VALUE);

    /* No matcher for lists yet */
    EXPECT_CALL (*gmockSetting, setList (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);

    ccsSettingValueListFree (VALUE, FALSE);
}

TEST_P (CCSBackendConformanceTest, TestReadListMatch)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingValueList VALUE;
    const char *	VMatch[3] = { "foo",
				      "bar",
				      "baz" };

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeList, plugin);

    VALUE = ccsGetValueListFromMatchArray (VMatch, 3, setting);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteListAtKey (pluginName, settingName, VALUE);

    /* No matcher for lists yet */
    EXPECT_CALL (*gmockSetting, setList (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);

    ccsSettingValueListFree (VALUE, FALSE);
}

TEST_P (CCSBackendConformanceTest, TestReadListColor)
{
    std::string pluginName ("plugin");
    std::string settingName ("string_setting");
    CCSSettingValueList VALUE;
    CCSSettingColorValue VColor[3] = { { .color = { 100, 200, 300, 100 } },
					{ .color = { 50, 100, 200, 300 } },
					{ .color = { 10, 20, 30, 40 } }
				     };

    CCSContext *context = SpawnContext ();
    CCSPlugin *plugin = SpawnPlugin (pluginName);
    CCSSetting *setting = SpawnSetting (settingName, TypeList, plugin);

    VALUE = ccsGetValueListFromColorArray (VColor, 3, setting);

    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);

    GetParam ()->WriteListAtKey (pluginName, settingName, VALUE);

    /* No matcher for lists yet */
    EXPECT_CALL (*gmockSetting, setList (_, _));

    ccsBackendReadSetting (GetBackend (), context, setting);

    ccsSettingValueListFree (VALUE, FALSE);
}

#endif


#endif

