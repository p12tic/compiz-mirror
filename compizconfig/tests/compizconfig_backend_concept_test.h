#ifndef _COMPIZCONFIG_CCS_BACKEND_CONCEPT_TEST
#define _COMPIZCONFIG_CCS_BACKEND_CONCEPT_TEST

#include <list>

#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ccs-backend.h>
#include <ccs.h>

#include <compizconfig_ccs_plugin_mock.h>
#include <compizconfig_ccs_setting_mock.h>
#include <compizconfig_ccs_context_mock.h>

using ::testing::Eq;
using ::testing::SetArgPointee;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::MakeMatcher;
using ::testing::Matcher;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;

class ListEqualityMatcher :
    public MatcherInterface <CCSSettingValueList>
{
    public:

	ListEqualityMatcher (CCSSettingListInfo info,
			     CCSSettingValueList cmp) :
	    mInfo (info),
	    mCmp (cmp)
	{
	}

	virtual bool MatchAndExplain (CCSSettingValueList x, MatchResultListener *listener) const
	{
	    return ccsCompareLists (x, mCmp, mInfo);
	}

	virtual void DescribeTo (std::ostream *os) const
	{
	    *os << "lists are equal";
	}

	virtual void DescribeNegationTo (std::ostream *os) const
	{
	    *os << "lists are not equal";
	}

    private:

	CCSSettingListInfo mInfo;
	CCSSettingValueList mCmp;
};

Matcher<CCSSettingValueList> ListEqual (CCSSettingListInfo info,
					CCSSettingValueList cmp)
{
    return MakeMatcher (new ListEqualityMatcher (info, cmp));
}

bool
operator== (const CCSSettingColorValue &lhs,
	    const CCSSettingColorValue &rhs)
{
    if (ccsIsEqualColor (lhs, rhs))
	return true;
    return false;
}

bool
operator== (const CCSSettingKeyValue &lhs,
	    const CCSSettingKeyValue &rhs)
{
    if (ccsIsEqualKey (lhs, rhs))
	return true;
    return false;
}

bool
operator== (const CCSSettingButtonValue &lhs,
	    const CCSSettingButtonValue &rhs)
{
    if (ccsIsEqualButton (lhs, rhs))
	return true;
    return false;
}

class CharacterWrapper :
    boost::noncopyable
{
    public:

	explicit CharacterWrapper (char *c) :
	    mChar (c)
	{
	}

	~CharacterWrapper ()
	{
	    free (mChar);
	}

	operator char * ()
	{
	    return mChar;
	}

	operator const char * () const
	{
	    return mChar;
	}

    private:

	char *mChar;
};

class CCSListWrapper :
    boost::noncopyable
{
    public:

	typedef boost::shared_ptr <CCSListWrapper> Ptr;

	CCSListWrapper (CCSSettingValueList list, bool freeItems, CCSSettingType type) :
	    mList (list),
	    mFreeItems (freeItems),
	    mType (type)
	{
	}

	CCSSettingType type () { return mType; }

	operator CCSSettingValueList ()
	{
	    return mList;
	}

	operator CCSSettingValueList () const
	{
	    return mList;
	}

	~CCSListWrapper ()
	{
	    ccsSettingValueListFree (mList, mFreeItems ? TRUE : FALSE);
	}
    private:

	CCSSettingValueList mList;
	bool		    mFreeItems;
	CCSSettingType      mType;
};

typedef boost::variant <bool,
			int,
			float,
			const char *,
			CCSSettingColorValue,
			CCSSettingKeyValue,
			CCSSettingButtonValue,
			unsigned int,
			CCSListWrapper::Ptr> VariantTypes;

class CCSBackendConceptTestEnvironmentInterface
{
    public:

	virtual ~CCSBackendConceptTestEnvironmentInterface () {};
	virtual CCSBackend * SetUp (CCSContext *context,
				    CCSContextGMock *gmockContext) = 0;
	virtual void TearDown (CCSBackend *) = 0;

	virtual void PreWrite (CCSContextGMock *,
			       CCSPluginGMock  *,
			       CCSSettingGMock *,
			       CCSSettingType) = 0;
	virtual void PostWrite (CCSContextGMock *,
				CCSPluginGMock  *,
				CCSSettingGMock *,
				CCSSettingType) = 0;

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

	virtual void PreRead (CCSContextGMock *,
			      CCSPluginGMock  *,
			      CCSSettingGMock *,
			      CCSSettingType) = 0;
	virtual void PostRead (CCSContextGMock *,
			       CCSPluginGMock  *,
			       CCSSettingGMock *,
			       CCSSettingType) = 0;

	virtual Bool ReadBoolAtKey (const std::string &plugin,
				       const std::string &key) = 0;
	virtual int ReadIntegerAtKey (const std::string &plugin,
					const std::string &key) = 0;
	virtual float ReadFloatAtKey (const std::string &plugin,
				      const std::string &key) = 0;
	virtual const char * ReadStringAtKey (const std::string &plugin,
				       const std::string &key) = 0;
	virtual CCSSettingColorValue ReadColorAtKey (const std::string &plugin,
				       const std::string &key) = 0;
	virtual CCSSettingKeyValue ReadKeyAtKey (const std::string &plugin,
				       const std::string &key) = 0;
	virtual CCSSettingButtonValue ReadButtonAtKey (const std::string &plugin,
				       const std::string &key) = 0;
	virtual unsigned int ReadEdgeAtKey (const std::string &plugin,
				       const std::string &key) = 0;
	virtual const char * ReadMatchAtKey (const std::string &plugin,
				      const std::string &key) = 0;
	virtual Bool ReadBellAtKey (const std::string &plugin,
				       const std::string &key) = 0;
	virtual CCSSettingValueList ReadListAtKey (const std::string &plugin,
				     const std::string &key) = 0;
};

namespace
{

typedef boost::function <void ()> WriteFunc;

Bool boolToBool (bool v) { return v ? TRUE : FALSE; }

CCSSettingGMock * getSettingGMockFromSetting (CCSSetting *setting) { return (CCSSettingGMock *) ccsObjectGetPrivate (setting); }

void SetIntWriteExpectation (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value,
			     CCSSetting       *setting,
			     const WriteFunc &write,
			     CCSBackendConceptTestEnvironmentInterface *env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getInt (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boost::get <int> (value)),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (env->ReadIntegerAtKey (plugin, key), boost::get <int> (value));
}

void SetBoolWriteExpectation (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value,
			      CCSSetting       *setting,
			      const WriteFunc &write,
			      CCSBackendConceptTestEnvironmentInterface *env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getBool (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boolToBool (boost::get <bool> (value))),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (env->ReadBoolAtKey (plugin, key), boolToBool (boost::get <bool> (value)));
}

void SetFloatWriteExpectation (const std::string &plugin,
			       const std::string &key,
			       const VariantTypes &value,
			       CCSSetting       *setting,
			       const WriteFunc &write,
			       CCSBackendConceptTestEnvironmentInterface *env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getFloat (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boost::get <float> (value)),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (env->ReadFloatAtKey (plugin, key), boost::get <float> (value));
}

void SetStringWriteExpectation (const std::string &plugin,
				const std::string &key,
				const VariantTypes &value,
				CCSSetting       *setting,
				const WriteFunc &write,
				CCSBackendConceptTestEnvironmentInterface *env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getString (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     const_cast <char *> (boost::get <const char *> (value))),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (env->ReadStringAtKey (plugin, key), boost::get <const char *> (value));
}

void SetColorWriteExpectation (const std::string &plugin,
			       const std::string &key,
			       const VariantTypes &value,
			       CCSSetting       *setting,
			       const WriteFunc &write,
			       CCSBackendConceptTestEnvironmentInterface *env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getColor (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boost::get <CCSSettingColorValue> (value)),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (env->ReadColorAtKey (plugin, key), boost::get <CCSSettingColorValue> (value));
}

void SetKeyWriteExpectation (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value,
			     CCSSetting       *setting,
			     const WriteFunc &write,
			     CCSBackendConceptTestEnvironmentInterface *env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getKey (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boost::get <CCSSettingKeyValue> (value)),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (env->ReadKeyAtKey (plugin, key), boost::get <CCSSettingKeyValue> (value));
}

void SetButtonWriteExpectation (const std::string &plugin,
				const std::string &key,
				const VariantTypes &value,
				CCSSetting       *setting,
				const WriteFunc &write,
				CCSBackendConceptTestEnvironmentInterface *env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getButton (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boost::get <CCSSettingButtonValue> (value)),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (env->ReadButtonAtKey (plugin, key), boost::get <CCSSettingButtonValue> (value));
}

void SetEdgeWriteExpectation (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value,
			      CCSSetting       *setting,
			      const WriteFunc &write,
			      CCSBackendConceptTestEnvironmentInterface *env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getEdge (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boost::get <unsigned int> (value)),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (env->ReadEdgeAtKey (plugin, key), boost::get <unsigned int> (value));
}

void SetBellWriteExpectation (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value,
			      CCSSetting       *setting,
			      const WriteFunc &write,
			      CCSBackendConceptTestEnvironmentInterface *env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getBell (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boolToBool (boost::get <bool> (value))),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (env->ReadBellAtKey (plugin, key), boolToBool (boost::get <bool> (value)));
}

void SetMatchWriteExpectation (const std::string &plugin,
			       const std::string &key,
			       const VariantTypes &value,
			       CCSSetting       *setting,
			       const WriteFunc &write,
			       CCSBackendConceptTestEnvironmentInterface *env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getMatch (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     const_cast <char *> (boost::get <const char *> (value))),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (env->ReadMatchAtKey (plugin, key), boost::get <const char *> (value));
}

void SetListWriteExpectation (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value,
			      CCSSetting       *setting,
			      const WriteFunc &write,
			      CCSBackendConceptTestEnvironmentInterface *env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    CCSSettingValueList list = *(boost::get <boost::shared_ptr <CCSListWrapper> > (value));
    boost::shared_ptr <CCSSettingInfo> info (boost::make_shared <CCSSettingInfo> ());

    info->forList.listType = (boost::get <boost::shared_ptr <CCSListWrapper> > (value))->type ();

    EXPECT_CALL (*gmock, getInfo ()).WillRepeatedly (Return (info.get ()));
    EXPECT_CALL (*gmock, getList (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     list),
							 Return (TRUE)));
    write ();
    EXPECT_THAT (env->ReadListAtKey (plugin, key), ListEqual (ccsSettingGetInfo (setting)->forList, list));
}

void SetIntReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setInt (boost::get <int> (value), _));
}

void SetBoolReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setBool (boolToBool (boost::get <bool> (value)), _));
}

void SetBellReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setBell (boolToBool (boost::get <bool> (value)), _));
}

void SetFloatReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setFloat (boost::get <float> (value), _));
}

void SetStringReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setString (Eq (std::string (boost::get <const char *> (value))), _));
}

void SetMatchReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setMatch (Eq (std::string (boost::get <const char *> (value))), _));
}

void SetColorReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setColor (boost::get <CCSSettingColorValue> (value), _));
}

void SetKeyReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setKey (boost::get <CCSSettingKeyValue> (value), _));
}

void SetButtonReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setButton (boost::get <CCSSettingButtonValue> (value), _));
}

void SetEdgeReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setEdge (boost::get <unsigned int> (value), _));
}

CCSSettingInfo globalListInfo;

void SetListReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    globalListInfo.forList.listType = (boost::get <boost::shared_ptr <CCSListWrapper> > (value))->type ();
    globalListInfo.forList.listInfo = NULL;

    ON_CALL (*gmock, getInfo ()).WillByDefault (Return (&globalListInfo));
    EXPECT_CALL (*gmock, setList (ListEqual (globalListInfo.forList, *(boost::get <boost::shared_ptr <CCSListWrapper> > (value))), _));
}

}

class CCSBackendConceptTestParamInterface
{
    public:

	typedef boost::shared_ptr <CCSBackendConceptTestParamInterface> Ptr;

	typedef boost::function <void (const std::string &plugin,
				       const std::string &keyname,
				       const VariantTypes &value)> NativeWriteMethod;
	typedef boost::function <void (CCSSettingGMock *,
				       const VariantTypes &)> SetReadExpectation;
	typedef boost::function <void (const std::string &,
				       const std::string &,
				       const VariantTypes &,
				       CCSSetting        *,
				       const WriteFunc &,
				       CCSBackendConceptTestEnvironmentInterface *)> SetWriteExpectation;

	virtual CCSBackendConceptTestEnvironmentInterface * testEnv () = 0;
	virtual VariantTypes & value () = 0;
	virtual NativeWriteMethod & nativeWrite () = 0;
	virtual CCSSettingType & type () = 0;
	virtual std::string & keyname () = 0;
	virtual SetWriteExpectation & setWriteExpectationAndWrite () = 0;
	virtual SetReadExpectation & setReadExpectation () = 0;
	virtual std::string & what () = 0;
};

template <typename I>
class CCSBackendConceptTestParam :
    public CCSBackendConceptTestParamInterface
{
    public:

	typedef boost::shared_ptr <CCSBackendConceptTestParam <I> > Ptr;

	CCSBackendConceptTestParam (CCSBackendConceptTestEnvironmentInterface *testEnv,
				    const VariantTypes &value,
				    const NativeWriteMethod &write,
				    const CCSSettingType &type,
				    const std::string &keyname,
				    const SetReadExpectation &setReadExpectation,
				    const SetWriteExpectation &setWriteExpectation,
				    const std::string &what) :
	    mTestEnv (testEnv),
	    mValue (value),
	    mNativeWrite (write),
	    mType (type),
	    mKeyname (keyname),
	    mSetReadExpectation (setReadExpectation),
	    mSetWriteExpectation (setWriteExpectation),
	    mWhat (what)
	{
	}

	CCSBackendConceptTestEnvironmentInterface * testEnv () { return mTestEnv; }
	VariantTypes & value () { return mValue; }
	CCSBackendConceptTestParamInterface::NativeWriteMethod & nativeWrite () { return mNativeWrite; }
	CCSSettingType & type () { return mType; }
	std::string & keyname () { return mKeyname; }
	CCSBackendConceptTestParamInterface::SetReadExpectation & setReadExpectation () { return mSetReadExpectation; }
	CCSBackendConceptTestParamInterface::SetWriteExpectation & setWriteExpectationAndWrite () { return mSetWriteExpectation; }
	std::string & what () { return mWhat; }

    private:

	CCSBackendConceptTestEnvironmentInterface *mTestEnv;
	VariantTypes mValue;
	NativeWriteMethod mNativeWrite;
	CCSSettingType mType;
	std::string mKeyname;
	SetReadExpectation mSetReadExpectation;
	SetWriteExpectation mSetWriteExpectation;
	std::string mWhat;

};

class CCSBackendConformanceTest :
    public ::testing::TestWithParam <CCSBackendConceptTestParamInterface::Ptr>
{
    public:

	virtual ~CCSBackendConformanceTest () {}

	CCSBackend * GetBackend ()
	{
	    return mBackend;
	}

	virtual void SetUp ()
	{
	    CCSBackendConformanceTest::SpawnContext (&context);
	    gmockContext = (CCSContextGMock *) ccsObjectGetPrivate (context);

	    mBackend = GetParam ()->testEnv ()->SetUp (context, gmockContext);
	}

	virtual void TearDown ()
	{
	    CCSBackendConformanceTest::GetParam ()->testEnv ()->TearDown (mBackend);

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

	/* Having the returned context, setting and plugin
	 * as out params is awkward, but GTest doesn't let
	 * you use ASSERT_* unless the function returns void
	 */
	void
	SpawnContext (CCSContext **context)
	{
	    *context = ccsMockContextNew ();
	    mSpawnedContexts.push_back (*context);
	}

	void
	SpawnPlugin (const std::string &name, CCSContext *context, CCSPlugin **plugin)
	{
	    *plugin = ccsMockPluginNew ();
	    mSpawnedPlugins.push_back (*plugin);

	    CCSPluginGMock *gmockPlugin = (CCSPluginGMock *) ccsObjectGetPrivate (*plugin);

	    ASSERT_FALSE (name.empty ());
	    ASSERT_TRUE (context);

	    ON_CALL (*gmockPlugin, getName ()).WillByDefault (Return ((char *) name.c_str ()));
	    ON_CALL (*gmockPlugin, getContext ()).WillByDefault (Return (context));
	}

	void
	SpawnSetting (const std::string &name,
		      CCSSettingType	type,
		      CCSPlugin		*plugin,
		      CCSSetting	**setting)
	{
	    *setting = ccsMockSettingNew ();
	    mSpawnedSettings.push_back (*setting);

	    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (*setting);

	    ASSERT_FALSE (name.empty ());
	    ASSERT_NE (type, TypeNum);
	    ASSERT_TRUE (plugin);

	    ON_CALL (*gmockSetting, getName ()).WillByDefault (Return ((char *) name.c_str ()));
	    ON_CALL (*gmockSetting, getType ()).WillByDefault (Return (type));
	    ON_CALL (*gmockSetting, getParent ()).WillByDefault (Return (plugin));
	}

    protected:

	CCSContext *context;
	CCSContextGMock *gmockContext;

    private:

	std::list <CCSContext *> mSpawnedContexts;
	std::list <CCSPlugin  *> mSpawnedPlugins;
	std::list <CCSSetting *> mSpawnedSettings;

	CCSBackend *mBackend;
};

namespace compizconfig
{
namespace test
{
namespace impl
{

Bool boolValues[] = { TRUE, FALSE, TRUE };
int intValues[] = { 1, 2, 3 };
float floatValues[] = { 1.0, 2.0, 3.0 };
const char * stringValues[] = { "foo", "grill", "bar" };

CCSSettingColorValue colorValues[3] = { { .color = { 100, 200, 300, 100 } },
					{ .color = { 50, 100, 200, 300 } },
					{ .color = { 10, 20, 30, 40 } }
				      };

CCSSettingKeyValue keyValue = { (1 << 0) | (1 << 1),
				1 };

CCSSettingButtonValue buttonValue = { (1 << 0) | (1 << 1),
				      1,
				      (1 << 1) };
}

template <typename I>
::testing::internal::ParamGenerator<typename CCSBackendConceptTestParamInterface::Ptr>
GenerateTestingParametersForBackendInterface ()
{
    static I interface;
    static CCSBackendConceptTestEnvironmentInterface *backendEnv = &interface;

    typedef CCSBackendConceptTestParam<I> ConceptParam;

    static typename CCSBackendConceptTestParamInterface::Ptr testParam[] =
    {
	boost::make_shared <ConceptParam> (backendEnv,
					   VariantTypes (1),
					   boost::bind (&CCSBackendConceptTestEnvironmentInterface::WriteIntegerAtKey, backendEnv, _1, _2, _3),
					   TypeInt,
					   "integer_setting",
					   boost::bind (SetIntReadExpectation, _1, _2),
					   boost::bind (SetIntWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestInt"),
	boost::make_shared <ConceptParam> (backendEnv,
					   VariantTypes (true),
					   boost::bind (&CCSBackendConceptTestEnvironmentInterface::WriteBoolAtKey, backendEnv, _1, _2, _3),
					   TypeBool,
					   "boolean_setting",
					   boost::bind (SetBoolReadExpectation, _1, _2),
					   boost::bind (SetBoolWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestBool"),
	boost::make_shared <ConceptParam> (backendEnv,
					   VariantTypes (static_cast <float> (3.0)),
					   boost::bind (&CCSBackendConceptTestEnvironmentInterface::WriteFloatAtKey, backendEnv, _1, _2, _3),
					   TypeFloat,
					   "float_setting",
					   boost::bind (SetFloatReadExpectation, _1, _2),
					   boost::bind (SetFloatWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestFloat"),
	boost::make_shared <ConceptParam> (backendEnv,
					   VariantTypes (static_cast <const char *> ("foo")),
					   boost::bind (&CCSBackendConceptTestEnvironmentInterface::WriteStringAtKey, backendEnv, _1, _2, _3),
					   TypeString,
					   "string_setting",
					   boost::bind (SetStringReadExpectation, _1, _2),
					   boost::bind (SetStringWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestString"),
	boost::make_shared <ConceptParam> (backendEnv,
					   VariantTypes (static_cast <const char *> ("foo=bar")),
					   boost::bind (&CCSBackendConceptTestEnvironmentInterface::WriteMatchAtKey, backendEnv, _1, _2, _3),
					   TypeMatch,
					   "match_setting",
					   boost::bind (SetMatchReadExpectation, _1, _2),
					   boost::bind (SetMatchWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestMatch"),
	boost::make_shared <ConceptParam> (backendEnv,
					   VariantTypes (true),
					   boost::bind (&CCSBackendConceptTestEnvironmentInterface::WriteBellAtKey, backendEnv, _1, _2, _3),
					   TypeBell,
					   "bell_setting",
					   boost::bind (SetBellReadExpectation, _1, _2),
					   boost::bind (SetBellWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestBell"),
	boost::make_shared <ConceptParam> (backendEnv,
					   VariantTypes (impl::colorValues[0]),
					   boost::bind (&CCSBackendConceptTestEnvironmentInterface::WriteColorAtKey, backendEnv, _1, _2, _3),
					   TypeColor,
					   "color_setting",
					   boost::bind (SetColorReadExpectation, _1, _2),
					   boost::bind (SetColorWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestColor"),
	boost::make_shared <ConceptParam> (backendEnv,
					   VariantTypes (impl::keyValue),
					   boost::bind (&CCSBackendConceptTestEnvironmentInterface::WriteKeyAtKey, backendEnv, _1, _2, _3),
					   TypeKey,
					   "key_setting",
					   boost::bind (SetKeyReadExpectation, _1, _2),
					   boost::bind (SetKeyWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestKey"),
	boost::make_shared <ConceptParam> (backendEnv,
					   VariantTypes (impl::buttonValue),
					   boost::bind (&CCSBackendConceptTestEnvironmentInterface::WriteButtonAtKey, backendEnv, _1, _2, _3),
					   TypeButton,
					   "button_setting",
					   boost::bind (SetButtonReadExpectation, _1, _2),
					   boost::bind (SetButtonWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestButton"),
	boost::make_shared <ConceptParam> (backendEnv,
					   VariantTypes (static_cast <unsigned int> (1)),
					   boost::bind (&CCSBackendConceptTestEnvironmentInterface::WriteEdgeAtKey, backendEnv, _1, _2, _3),
					   TypeEdge,
					   "edge_setting",
					   boost::bind (SetEdgeReadExpectation, _1, _2),
					   boost::bind (SetEdgeWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestEdge"),
	boost::make_shared <ConceptParam> (backendEnv,
					   VariantTypes (boost::make_shared <CCSListWrapper> (ccsGetValueListFromIntArray (impl::intValues,
															   sizeof (impl::intValues) / sizeof (impl::intValues[0]),
															   NULL), false, TypeInt)),
					   boost::bind (&CCSBackendConceptTestEnvironmentInterface::WriteListAtKey, backendEnv, _1, _2, _3),
					   TypeList,
					   "int_list_setting",
					   boost::bind (SetListReadExpectation, _1, _2),
					   boost::bind (SetListWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestListInt"),
	boost::make_shared <ConceptParam> (backendEnv,
					   VariantTypes (boost::make_shared <CCSListWrapper> (ccsGetValueListFromFloatArray (impl::floatValues,
															     sizeof (impl::floatValues) / sizeof (impl::floatValues[0]),
															     NULL), false, TypeFloat)),
					   boost::bind (&CCSBackendConceptTestEnvironmentInterface::WriteListAtKey, backendEnv, _1, _2, _3),
					   TypeList,
					   "float_list_setting",
					   boost::bind (SetListReadExpectation, _1, _2),
					   boost::bind (SetListWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestListInt"),
	boost::make_shared <ConceptParam> (backendEnv,
					   VariantTypes (boost::make_shared <CCSListWrapper> (ccsGetValueListFromBoolArray (impl::boolValues,
															   sizeof (impl::boolValues) / sizeof (impl::boolValues[0]),
															   NULL), false, TypeBool)),
					   boost::bind (&CCSBackendConceptTestEnvironmentInterface::WriteListAtKey, backendEnv, _1, _2, _3),
					   TypeList,
					   "bool_list_setting",
					   boost::bind (SetListReadExpectation, _1, _2),
					   boost::bind (SetListWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestListBool"),
	boost::make_shared <ConceptParam> (backendEnv,
					   VariantTypes (boost::make_shared <CCSListWrapper> (ccsGetValueListFromStringArray (impl::stringValues,
															      sizeof (impl::stringValues) / sizeof (impl::stringValues[0]),
															      NULL), false, TypeString)),
					   boost::bind (&CCSBackendConceptTestEnvironmentInterface::WriteListAtKey, backendEnv, _1, _2, _3),
					   TypeList,
					   "string_list_setting",
					   boost::bind (SetListReadExpectation, _1, _2),
					   boost::bind (SetListWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestListString"),
	boost::make_shared <ConceptParam> (backendEnv,
					   VariantTypes (boost::make_shared <CCSListWrapper> (ccsGetValueListFromColorArray (impl::colorValues,
															     sizeof (impl::colorValues) / sizeof (impl::colorValues[0]),
															     NULL), false, TypeColor)),
					   boost::bind (&CCSBackendConceptTestEnvironmentInterface::WriteListAtKey, backendEnv, _1, _2, _3),
					   TypeList,
					   "color_list_setting",
					   boost::bind (SetListReadExpectation, _1, _2),
					   boost::bind (SetListWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestListColor")
    };

    return ::testing::ValuesIn (testParam);
}
}
}

class CCSBackendConformanceTestReadWrite :
    public CCSBackendConformanceTest
{
    public:

	virtual ~CCSBackendConformanceTestReadWrite () {}

	virtual void SetUp ()
	{
	    CCSBackendConformanceTest::SetUp ();

	    pluginName = "mock";
	    settingName = GetParam ()->keyname ();
	    VALUE = GetParam ()->value ();

	    CCSBackendConformanceTest::SpawnPlugin (pluginName, context, &plugin);
	    CCSBackendConformanceTest::SpawnSetting (settingName, GetParam ()->type (), plugin, &setting);

	    gmockPlugin = (CCSPluginGMock *) ccsObjectGetPrivate (plugin);
	    gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting);
	}

	virtual void TearDown ()
	{
	    CCSBackendConformanceTest::TearDown ();
	}

    protected:

	std::string pluginName;
	std::string settingName;
	VariantTypes VALUE;
	CCSPlugin *plugin;
	CCSSetting *setting;
	CCSPluginGMock  *gmockPlugin;
	CCSSettingGMock *gmockSetting;

};

TEST_P (CCSBackendConformanceTestReadWrite, TestReadValue)
{
    SCOPED_TRACE (CCSBackendConformanceTest::GetParam ()->what () + "Read");

    CCSBackendConformanceTest::GetParam ()->testEnv ()->PreRead (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());
    CCSBackendConformanceTest::GetParam ()->nativeWrite () (pluginName, settingName, VALUE);
    CCSBackendConformanceTest::GetParam ()->testEnv ()->PostRead (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());
    CCSBackendConformanceTest::GetParam ()->setReadExpectation () (gmockSetting, VALUE);

    ccsBackendReadSetting (CCSBackendConformanceTest::GetBackend (), context, setting);
}

TEST_P (CCSBackendConformanceTestReadWrite, TestWriteValue)
{
    SCOPED_TRACE (CCSBackendConformanceTest::GetParam ()->what () + "Write");

    CCSBackendConformanceTest::GetParam ()->testEnv ()->PreWrite (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());
    CCSBackendConformanceTest::GetParam ()->setWriteExpectationAndWrite () (pluginName,
									    settingName,
									    VALUE,
									    setting,
									    boost::bind (ccsBackendWriteSetting,
											 CCSBackendConformanceTest::GetBackend (),
											 context,
											 setting),
									    GetParam ()->testEnv ());
    CCSBackendConformanceTest::GetParam ()->testEnv ()->PostWrite (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());

}


#endif

