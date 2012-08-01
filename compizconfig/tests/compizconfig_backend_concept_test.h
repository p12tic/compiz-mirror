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

#include <X11/keysym.h>

#include <ccs-backend.h>
#include <ccs.h>

#include <compizconfig_ccs_plugin_mock.h>
#include <compizconfig_ccs_setting_mock.h>
#include <compizconfig_ccs_context_mock.h>

#include "gtest_shared_characterwrapper.h"

using ::testing::Eq;
using ::testing::SetArgPointee;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnNull;
using ::testing::MakeMatcher;
using ::testing::Matcher;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::AtLeast;
using ::testing::NiceMock;

MATCHER(IsTrue, "Is True") { if (arg) return true; else return false; }
MATCHER(IsFalse, "Is False") { if (!arg) return true; else return false; }

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

::std::ostream &
operator<< (::std::ostream &os, const CCSSettingColorValue &v)
{
    return os << "Red: " << std::hex << v.color.red << "Blue: " << std::hex << v.color.blue << "Green: " << v.color.green << "Alpha: " << v.color.alpha
       << std::dec << std::endl;
}

bool
operator== (const CCSSettingKeyValue &lhs,
	    const CCSSettingKeyValue &rhs)
{
    if (ccsIsEqualKey (lhs, rhs))
	return true;
    return false;
}

::std::ostream &
operator<< (::std::ostream &os, const CCSSettingKeyValue &v)
{
    return os << "Keysym: " << v.keysym << " KeyModMask " << std::hex << v.keyModMask << std::dec << std::endl;
}

bool
operator== (const CCSSettingButtonValue &lhs,
	    const CCSSettingButtonValue &rhs)
{
    if (ccsIsEqualButton (lhs, rhs))
	return true;
    return false;
}

::std::ostream &
operator<< (::std::ostream &os, const CCSSettingButtonValue &v)
{
    return os << "Button " << v.button << "Button Key Mask: " << std::hex << v.buttonModMask << "Edge Mask: " << v.edgeMask << std::dec << std::endl;
}

class CCSListWrapper :
    boost::noncopyable
{
    public:

	typedef boost::shared_ptr <CCSListWrapper> Ptr;

	CCSListWrapper (CCSSettingValueList list,
			bool freeItems,
			CCSSettingType type,
			const boost::shared_ptr <CCSSettingInfo> &listInfo,
			const boost::shared_ptr <CCSSetting> &settingReference) :
	    mList (list),
	    mFreeItems (freeItems),
	    mType (type),
	    mListInfo (listInfo),
	    mSettingReference (settingReference)
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

	const boost::shared_ptr <CCSSetting> &
	setting ()
	{
	    return mSettingReference;
	}

    private:

	CCSSettingValueList mList;
	bool		    mFreeItems;
	CCSSettingType      mType;
	boost::shared_ptr <CCSSettingInfo> mListInfo;
	boost::shared_ptr <CCSSetting> mSettingReference;
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

	typedef boost::shared_ptr <CCSBackendConceptTestEnvironmentInterface> Ptr;

	virtual ~CCSBackendConceptTestEnvironmentInterface () {};
	virtual CCSBackend * SetUp (CCSContext *context,
				    CCSContextGMock *gmockContext) = 0;
	virtual void TearDown (CCSBackend *) = 0;

	virtual const CCSBackendInfo * GetInfo () = 0;

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
				     const std::string &key,
						    CCSSetting *setting) = 0;

	virtual void PreUpdate (CCSContextGMock *,
			      CCSPluginGMock  *,
			      CCSSettingGMock *,
			      CCSSettingType) = 0;
	virtual void PostUpdate (CCSContextGMock *,
			       CCSPluginGMock  *,
			       CCSSettingGMock *,
			       CCSSettingType) = 0;

	virtual bool UpdateSettingAtKey (const std::string &plugin,
					 const std::string &setting) = 0;
};

class CCSBackendConceptTestEnvironmentFactoryInterface
{
    public:

	typedef boost::shared_ptr <CCSBackendConceptTestEnvironmentFactoryInterface> Ptr;

	virtual ~CCSBackendConceptTestEnvironmentFactoryInterface () {}

	virtual CCSBackendConceptTestEnvironmentInterface::Ptr ConstructTestEnv () = 0;
};

template <typename I>
class CCSBackendConceptTestEnvironmentFactory :
    public CCSBackendConceptTestEnvironmentFactoryInterface
{
    public:

	CCSBackendConceptTestEnvironmentInterface::Ptr
	ConstructTestEnv ()
	{
	    return boost::shared_static_cast <I> (boost::make_shared <I> ());
	}
};

namespace
{

typedef boost::function <void ()> WriteFunc;

Bool boolToBool (bool v) { return v ? TRUE : FALSE; }

CCSSettingGMock * getSettingGMockFromSetting (const boost::shared_ptr <CCSSetting> &setting) { return (CCSSettingGMock *) ccsObjectGetPrivate (setting.get ()); }

void SetIntWriteExpectation (const std::string &plugin,
			     const std::string &key,
			     const VariantTypes &value,
			     const boost::shared_ptr <CCSSetting> &setting,
			     const WriteFunc &write,
			     const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
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
			      const boost::shared_ptr <CCSSetting> &setting,
			      const WriteFunc &write,
			      const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getBool (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boolToBool (boost::get <bool> (value))),
							 Return (TRUE)));
    write ();

    bool v (boost::get <bool> (value));

    if (v)
	EXPECT_THAT (env->ReadBoolAtKey (plugin, key), IsTrue ());
    else
	EXPECT_THAT (env->ReadBoolAtKey (plugin, key), IsFalse ());
}

void SetFloatWriteExpectation (const std::string &plugin,
			       const std::string &key,
			       const VariantTypes &value,
			       const boost::shared_ptr <CCSSetting> &setting,
			       const WriteFunc &write,
			       const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
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
				const boost::shared_ptr <CCSSetting> &setting,
				const WriteFunc &write,
				const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getString (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     const_cast <char *> (boost::get <const char *> (value))),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (std::string (env->ReadStringAtKey (plugin, key)), std::string (boost::get <const char *> (value)));
}

void SetColorWriteExpectation (const std::string &plugin,
			       const std::string &key,
			       const VariantTypes &value,
			       const boost::shared_ptr <CCSSetting> &setting,
			       const WriteFunc &write,
			       const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
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
			     const boost::shared_ptr <CCSSetting> &setting,
			     const WriteFunc &write,
			     const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
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
				const boost::shared_ptr <CCSSetting> &setting,
				const WriteFunc &write,
				const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
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
			      const boost::shared_ptr <CCSSetting> &setting,
			      const WriteFunc &write,
			      const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
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
			      const boost::shared_ptr <CCSSetting> &setting,
			      const WriteFunc &write,
			      const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getBell (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     boolToBool (boost::get <bool> (value))),
							 Return (TRUE)));
    write ();
    bool v (boost::get <bool> (value));

    if (v)
	EXPECT_THAT (env->ReadBellAtKey (plugin, key), IsTrue ());
    else
	EXPECT_THAT (env->ReadBellAtKey (plugin, key), IsFalse ());
}

void SetMatchWriteExpectation (const std::string &plugin,
			       const std::string &key,
			       const VariantTypes &value,
			       const boost::shared_ptr <CCSSetting> &setting,
			       const WriteFunc &write,
			       const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    EXPECT_CALL (*gmock, getMatch (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     const_cast <char *> (boost::get <const char *> (value))),
							 Return (TRUE)));
    write ();
    EXPECT_EQ (std::string (env->ReadMatchAtKey (plugin, key)), std::string (boost::get <const char *> (value)));
}

void SetListWriteExpectation (const std::string &plugin,
			      const std::string &key,
			      const VariantTypes &value,
			      const boost::shared_ptr <CCSSetting> &setting,
			      const WriteFunc &write,
			      const CCSBackendConceptTestEnvironmentInterface::Ptr &env)
{
    CCSSettingGMock *gmock (getSettingGMockFromSetting (setting));
    CCSSettingValueList list = *(boost::get <boost::shared_ptr <CCSListWrapper> > (value));

    EXPECT_CALL (*gmock, getInfo ());

    CCSSettingInfo      *info = ccsSettingGetInfo (setting.get ());

    info->forList.listType = (boost::get <boost::shared_ptr <CCSListWrapper> > (value))->type ();

    EXPECT_CALL (*gmock, getInfo ()).Times (AtLeast (1));
    EXPECT_CALL (*gmock, getList (_)).WillRepeatedly (DoAll (
							 SetArgPointee <0> (
							     list),
							 Return (TRUE)));
    write ();

    EXPECT_THAT (CCSListWrapper (env->ReadListAtKey (plugin, key, setting.get ()),
				 true,
				 info->forList.listType,
				 boost::shared_ptr <CCSSettingInfo> (),
				 setting),
		 ListEqual (info->forList, list));
}

void SetIntReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    EXPECT_CALL (*gmock, setInt (boost::get <int> (value), _));
}

void SetBoolReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    bool v (boost::get <bool> (value));

    if (v)
	EXPECT_CALL (*gmock, setBool (IsTrue (), _));
    else
	EXPECT_CALL (*gmock, setBool (IsFalse (), _));
}

void SetBellReadExpectation (CCSSettingGMock *gmock, const VariantTypes &value)
{
    bool v (boost::get <bool> (value));

    if (v)
	EXPECT_CALL (*gmock, setBell (IsTrue (), _));
    else
	EXPECT_CALL (*gmock, setBell (IsFalse (), _));
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

	typedef void (CCSBackendConceptTestEnvironmentInterface::*NativeWriteMethod) (const std::string &plugin,
										      const std::string &keyname,
										      const VariantTypes &value);

	typedef boost::function <void (CCSSettingGMock *,
				       const VariantTypes &)> SetReadExpectation;
	typedef boost::function <void (const std::string &,
				       const std::string &,
				       const VariantTypes &,
				       const boost::shared_ptr <CCSSetting> &,
				       const WriteFunc &,
				       const CCSBackendConceptTestEnvironmentInterface::Ptr & )> SetWriteExpectation;

	virtual void TearDown (CCSBackend *) = 0;

	virtual CCSBackendConceptTestEnvironmentInterface::Ptr testEnv () = 0;
	virtual VariantTypes & value () = 0;
	virtual void nativeWrite (const CCSBackendConceptTestEnvironmentInterface::Ptr &  iface,
				  const std::string &plugin,
				  const std::string &keyname,
				  const VariantTypes &value) = 0;
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

	CCSBackendConceptTestParam (CCSBackendConceptTestEnvironmentFactoryInterface *testEnvFactory,
				    const VariantTypes &value,
				    const NativeWriteMethod &write,
				    const CCSSettingType &type,
				    const std::string &keyname,
				    const SetReadExpectation &setReadExpectation,
				    const SetWriteExpectation &setWriteExpectation,
				    const std::string &what) :
	    mTestEnvFactory (testEnvFactory),
	    mTestEnv (),
	    mValue (value),
	    mNativeWrite (write),
	    mType (type),
	    mKeyname (keyname),
	    mSetReadExpectation (setReadExpectation),
	    mSetWriteExpectation (setWriteExpectation),
	    mWhat (what)
	{
	}

	void TearDown (CCSBackend *b)
	{
	    if (mTestEnv)
		mTestEnv->TearDown (b);

	    mTestEnv.reset ();
	}

	CCSBackendConceptTestEnvironmentInterface::Ptr testEnv ()
	{
	    if (!mTestEnv)
		mTestEnv = mTestEnvFactory->ConstructTestEnv ();

	    return mTestEnv;
	}

	VariantTypes & value () { return mValue; }
	void nativeWrite (const CCSBackendConceptTestEnvironmentInterface::Ptr &  iface,
			  const std::string &plugin,
			  const std::string &keyname,
			  const VariantTypes &value)
	{
	    return ((iface.get ())->*mNativeWrite) (plugin, keyname, value);
	}
	CCSSettingType & type () { return mType; }
	std::string & keyname () { return mKeyname; }
	CCSBackendConceptTestParamInterface::SetReadExpectation & setReadExpectation () { return mSetReadExpectation; }
	CCSBackendConceptTestParamInterface::SetWriteExpectation & setWriteExpectationAndWrite () { return mSetWriteExpectation; }
	std::string & what () { return mWhat; }

    private:

	CCSBackendConceptTestEnvironmentFactoryInterface *mTestEnvFactory;
	CCSBackendConceptTestEnvironmentInterface::Ptr mTestEnv;
	VariantTypes mValue;
	NativeWriteMethod mNativeWrite;
	CCSSettingType mType;
	std::string mKeyname;
	SetReadExpectation mSetReadExpectation;
	SetWriteExpectation mSetWriteExpectation;
	std::string mWhat;

};

class CCSBackendConformanceSpawnObjectsTestFixtureBase
{
    protected:

	CCSBackendConformanceSpawnObjectsTestFixtureBase () :
	    profileName ("mock")
	{
	}

	virtual ~CCSBackendConformanceSpawnObjectsTestFixtureBase ()
	{
	}

	/* Having the returned context, setting and plugin
	 * as out params is awkward, but GTest doesn't let
	 * you use ASSERT_* unless the function returns void
	 */
	void
	SpawnContext (boost::shared_ptr <CCSContext> &context)
	{
	    context.reset (ccsMockContextNew (), boost::bind (ccsFreeMockContext, _1));
	}

	void
	SpawnPlugin (const std::string &name, const boost::shared_ptr <CCSContext> &context, boost::shared_ptr <CCSPlugin> &plugin)
	{
	    plugin.reset (ccsMockPluginNew (), boost::bind (ccsFreeMockPlugin, _1));

	    CCSPluginGMock *gmockPlugin = (CCSPluginGMock *) ccsObjectGetPrivate (plugin.get ());

	    ASSERT_FALSE (name.empty ());
	    ASSERT_TRUE (context.get ());

	    ON_CALL (*gmockPlugin, getName ()).WillByDefault (Return ((char *) name.c_str ()));
	    ON_CALL (*gmockPlugin, getContext ()).WillByDefault (Return (context.get ()));
	}

	void
	SpawnSetting (const std::string &name,
		      CCSSettingType	type,
		      const boost::shared_ptr <CCSPlugin> &plugin,
		      boost::shared_ptr <CCSSetting>      &setting)
	{
	    setting.reset (ccsMockSettingNew (), boost::bind (ccsFreeMockSetting, _1));
	    mSpawnedSettingInfo.push_back (CCSSettingInfo ());

	    CCSSettingGMock *gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting.get ());

	    ASSERT_FALSE (name.empty ());
	    ASSERT_NE (type, TypeNum);
	    ASSERT_TRUE (plugin);

	    ON_CALL (*gmockSetting, getName ()).WillByDefault (Return ((char *) name.c_str ()));
	    ON_CALL (*gmockSetting, getType ()).WillByDefault (Return (type));
	    ON_CALL (*gmockSetting, getParent ()).WillByDefault (Return (plugin.get ()));
	    ON_CALL (*gmockSetting, getInfo ()).WillByDefault (Return (&(mSpawnedSettingInfo.back ())));
	}

	void
	SetupContext ()
	{
	    SpawnContext (context);
	    gmockContext = (CCSContextGMock *) ccsObjectGetPrivate (context.get ());

	    ON_CALL (*gmockContext, getProfile ()).WillByDefault (Return (profileName.c_str ()));
	}

	CCSBackend * GetBackend ()
	{
	    return mBackend;
	}

	boost::shared_ptr <CCSContext> context;
	CCSContextGMock *gmockContext;
	CCSBackend *mBackend;

    private:

	std::vector <CCSSettingInfo> mSpawnedSettingInfo;
	std::string		     profileName;
};

class CCSBackendConformanceTestParameterizedByBackendFixture :
    public CCSBackendConformanceSpawnObjectsTestFixtureBase,
    public ::testing::TestWithParam <CCSBackendConceptTestEnvironmentFactoryInterface::Ptr>
{
    public:

	CCSBackendConformanceTestParameterizedByBackendFixture () :
	    mTestEnv (GetParam ()->ConstructTestEnv ())
	{
	}

	virtual void SetUp ()
	{
	    SetupContext ();
	    mBackend = mTestEnv->SetUp (context.get (), gmockContext);
	}

	virtual void TearDown ()
	{
	    if (mTestEnv)
		mTestEnv->TearDown (mBackend);

	    mTestEnv.reset ();
	}

    protected:

	CCSBackendConceptTestEnvironmentInterface::Ptr mTestEnv;
};

class CCSBackendConformanceTestParameterized :
    public CCSBackendConformanceSpawnObjectsTestFixtureBase,
    public ::testing::TestWithParam <CCSBackendConceptTestParamInterface::Ptr>
{
    public:

	virtual ~CCSBackendConformanceTestParameterized () {}

	virtual void SetUp ()
	{
	    SetupContext ();
	    mBackend = GetParam ()->testEnv ()->SetUp (context.get (), gmockContext);
	}

	virtual void TearDown ()
	{
	    CCSBackendConformanceTestParameterized::GetParam ()->TearDown (mBackend);
	}
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
const char * matchValues[] = { "type=foo", "class=bar", "xid=42" };

const unsigned int NUM_COLOR_VALUES = 3;

CCSSettingColorValue *
getColorValueList ()
{
    static const unsigned short max = std::numeric_limits <unsigned short>::max ();
    static const unsigned short maxD2 = max / 2;
    static const unsigned short maxD4 = max / 4;
    static const unsigned short maxD8 = max / 8;

    static CCSSettingColorValue colorValues[NUM_COLOR_VALUES] = { { .color = { maxD2, maxD4, maxD8, max } },
								  { .color = { maxD8, maxD4, maxD2, max } },
								  { .color = { max, maxD4, maxD2, maxD8 } }
								};
    static bool colorValueListInitialized = false;

    if (!colorValueListInitialized)
    {
	for (unsigned int i = 0; i < NUM_COLOR_VALUES; i++)
	{
	    CharacterWrapper s (ccsColorToString (&colorValues[i]));

	    ccsStringToColor (s, &colorValues[i]);
	}

	colorValueListInitialized = true;
    }

    return colorValues;
}

CCSSettingKeyValue keyValue = { XK_A,
				(1 << 0)};

CCSSettingButtonValue buttonValue = { 1,
				      (1 << 0),
				      (1 << 1) };

}

typedef boost::function <CCSSettingValueList (CCSSetting *)> ConstructorFunc;

CCSListWrapper::Ptr
CCSListConstructionExpectationsSetter (const ConstructorFunc &c,
				       CCSSettingType        type,
				       bool                  freeItems)
{
    boost::function <void (CCSSetting *)> f (boost::bind (ccsFreeMockSetting, _1));
    boost::shared_ptr <CCSSetting> mockSetting (ccsNiceMockSettingNew (), f);
    NiceMock <CCSSettingGMock>     *gmockSetting = reinterpret_cast <NiceMock <CCSSettingGMock> *> (ccsObjectGetPrivate (mockSetting.get ()));

    ON_CALL (*gmockSetting, getType ()).WillByDefault (Return (TypeList));

    boost::shared_ptr <CCSSettingInfo> listInfo (new CCSSettingInfo);

    listInfo->forList.listType = type;

    ON_CALL (*gmockSetting, getInfo ()).WillByDefault (Return (listInfo.get ()));
    ON_CALL (*gmockSetting, getDefaultValue ()).WillByDefault (ReturnNull ());
    return boost::make_shared <CCSListWrapper> (c (mockSetting.get ()), freeItems, type, listInfo, mockSetting);
}

template <typename I>
::testing::internal::ParamGenerator<typename CCSBackendConceptTestParamInterface::Ptr>
GenerateTestingParametersForBackendInterface ()
{
    static CCSBackendConceptTestEnvironmentFactory <I> interfaceFactory;
    static CCSBackendConceptTestEnvironmentFactoryInterface *backendEnvFactory = &interfaceFactory;

    typedef CCSBackendConceptTestParam<I> ConceptParam;

    static typename CCSBackendConceptTestParamInterface::Ptr testParam[] =
    {
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (1),
					   &CCSBackendConceptTestEnvironmentInterface::WriteIntegerAtKey,
					   TypeInt,
					   "integer_setting",
					   boost::bind (SetIntReadExpectation, _1, _2),
					   boost::bind (SetIntWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestInt"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (true),
					   &CCSBackendConceptTestEnvironmentInterface::WriteBoolAtKey,
					   TypeBool,
					   "boolean_setting",
					   boost::bind (SetBoolReadExpectation, _1, _2),
					   boost::bind (SetBoolWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestBool"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (static_cast <float> (3.0)),
					   &CCSBackendConceptTestEnvironmentInterface::WriteFloatAtKey,
					   TypeFloat,
					   "float_setting",
					   boost::bind (SetFloatReadExpectation, _1, _2),
					   boost::bind (SetFloatWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestFloat"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (static_cast <const char *> ("foo")),
					   &CCSBackendConceptTestEnvironmentInterface::WriteStringAtKey,
					   TypeString,
					   "string_setting",
					   boost::bind (SetStringReadExpectation, _1, _2),
					   boost::bind (SetStringWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestString"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (static_cast <const char *> ("foo=bar")),
					   &CCSBackendConceptTestEnvironmentInterface::WriteMatchAtKey,
					   TypeMatch,
					   "match_setting",
					   boost::bind (SetMatchReadExpectation, _1, _2),
					   boost::bind (SetMatchWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestMatch"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (true),
					   &CCSBackendConceptTestEnvironmentInterface::WriteBellAtKey,
					   TypeBell,
					   "bell_setting",
					   boost::bind (SetBellReadExpectation, _1, _2),
					   boost::bind (SetBellWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestBell"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (impl::getColorValueList ()[0]),
					   &CCSBackendConceptTestEnvironmentInterface::WriteColorAtKey,
					   TypeColor,
					   "color_setting",
					   boost::bind (SetColorReadExpectation, _1, _2),
					   boost::bind (SetColorWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestColor"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (impl::keyValue),
					   &CCSBackendConceptTestEnvironmentInterface::WriteKeyAtKey,
					   TypeKey,
					   "key_setting",
					   boost::bind (SetKeyReadExpectation, _1, _2),
					   boost::bind (SetKeyWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestKey"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (impl::buttonValue),
					   &CCSBackendConceptTestEnvironmentInterface::WriteButtonAtKey,
					   TypeButton,
					   "button_setting",
					   boost::bind (SetButtonReadExpectation, _1, _2),
					   boost::bind (SetButtonWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestButton"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (static_cast <unsigned int> (1)),
					   &CCSBackendConceptTestEnvironmentInterface::WriteEdgeAtKey,
					   TypeEdge,
					   "edge_setting",
					   boost::bind (SetEdgeReadExpectation, _1, _2),
					   boost::bind (SetEdgeWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestEdge"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (CCSListConstructionExpectationsSetter (boost::bind (ccsGetValueListFromIntArray,
													     impl::intValues,
													     sizeof (impl::intValues) / sizeof (impl::intValues[0]), _1),
												TypeInt, true)),
					   &CCSBackendConceptTestEnvironmentInterface::WriteListAtKey,
					   TypeList,
					   "int_list_setting",
					   boost::bind (SetListReadExpectation, _1, _2),
					   boost::bind (SetListWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestListInt"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (CCSListConstructionExpectationsSetter (boost::bind (ccsGetValueListFromFloatArray,
													     impl::floatValues,
													     sizeof (impl::floatValues) / sizeof (impl::floatValues[0]), _1),
												TypeFloat, true)),
					   &CCSBackendConceptTestEnvironmentInterface::WriteListAtKey,
					   TypeList,
					   "float_list_setting",
					   boost::bind (SetListReadExpectation, _1, _2),
					   boost::bind (SetListWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestListFloat"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (CCSListConstructionExpectationsSetter (boost::bind (ccsGetValueListFromBoolArray,
													     impl::boolValues,
													     sizeof (impl::boolValues) / sizeof (impl::boolValues[0]), _1),
												TypeBool, true)),
					   &CCSBackendConceptTestEnvironmentInterface::WriteListAtKey,
					   TypeList,
					   "bool_list_setting",
					   boost::bind (SetListReadExpectation, _1, _2),
					   boost::bind (SetListWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestListBool"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (CCSListConstructionExpectationsSetter (boost::bind (ccsGetValueListFromStringArray,
													     impl::stringValues,
													     sizeof (impl::stringValues) / sizeof (impl::stringValues[0]), _1),
												TypeString, true)),
					   &CCSBackendConceptTestEnvironmentInterface::WriteListAtKey,
					   TypeList,
					   "string_list_setting",
					   boost::bind (SetListReadExpectation, _1, _2),
					   boost::bind (SetListWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestListString"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (CCSListConstructionExpectationsSetter (boost::bind (ccsGetValueListFromMatchArray,
													     impl::matchValues,
													     sizeof (impl::matchValues) / sizeof (impl::matchValues[0]), _1),
												TypeMatch, true)),
					   &CCSBackendConceptTestEnvironmentInterface::WriteListAtKey,
					   TypeList,
					   "match_list_setting",
					   boost::bind (SetListReadExpectation, _1, _2),
					   boost::bind (SetListWriteExpectation, _1, _2, _3, _4, _5, _6),
					   "TestListMatch"),
	boost::make_shared <ConceptParam> (backendEnvFactory,
					   VariantTypes (CCSListConstructionExpectationsSetter (boost::bind (ccsGetValueListFromColorArray,
													     impl::getColorValueList (),
													     impl::NUM_COLOR_VALUES, _1),
											       TypeColor, true)),
					   &CCSBackendConceptTestEnvironmentInterface::WriteListAtKey,
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
    public CCSBackendConformanceTestParameterized
{
    public:

	virtual ~CCSBackendConformanceTestReadWrite () {}

	virtual void SetUp ()
	{
	    CCSBackendConformanceTestParameterized::SetUp ();

	    pluginName = "mock";
	    settingName = GetParam ()->keyname ();
	    VALUE = GetParam ()->value ();

	    CCSBackendConformanceTestParameterized::SpawnPlugin (pluginName, context, plugin);
	    CCSBackendConformanceTestParameterized::SpawnSetting (settingName, GetParam ()->type (), plugin, setting);

	    gmockPlugin = (CCSPluginGMock *) ccsObjectGetPrivate (plugin.get ());
	    gmockSetting = (CCSSettingGMock *) ccsObjectGetPrivate (setting.get ());
	}

	virtual void TearDown ()
	{
	    CCSBackendConformanceTestParameterized::TearDown ();
	}

    protected:

	std::string pluginName;
	std::string settingName;
	VariantTypes VALUE;
	boost::shared_ptr <CCSPlugin> plugin;
	boost::shared_ptr <CCSSetting> setting;
	CCSPluginGMock  *gmockPlugin;
	CCSSettingGMock *gmockSetting;

};

TEST_P (CCSBackendConformanceTestReadWrite, TestReadValue)
{
    SCOPED_TRACE (CCSBackendConformanceTestParameterized::GetParam ()->what () + "Read");

    CCSBackendConformanceTestParameterized::GetParam ()->testEnv ()->PreRead (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());
    CCSBackendConformanceTestParameterized::GetParam ()->nativeWrite (CCSBackendConformanceTestParameterized::GetParam ()->testEnv (),
							 pluginName, settingName, VALUE);
    CCSBackendConformanceTestParameterized::GetParam ()->testEnv ()->PostRead (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());
    CCSBackendConformanceTestParameterized::GetParam ()->setReadExpectation () (gmockSetting, VALUE);

    ccsBackendReadSetting (CCSBackendConformanceTestParameterized::GetBackend (), context.get (), setting.get ());
}

TEST_P (CCSBackendConformanceTestReadWrite, TestUpdateMockedValue)
{
    SCOPED_TRACE (CCSBackendConformanceTestParameterized::GetParam ()->what () + "UpdateMocked");

    CCSBackendConformanceTestParameterized::GetParam ()->testEnv ()->PreUpdate (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());
    CCSBackendConformanceTestParameterized::GetParam ()->nativeWrite (CCSBackendConformanceTestParameterized::GetParam ()->testEnv (),
							 pluginName, settingName, VALUE);
    CCSBackendConformanceTestParameterized::GetParam ()->testEnv ()->PostUpdate (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());
    CCSBackendConformanceTestParameterized::GetParam ()->setReadExpectation () (gmockSetting, VALUE);

    ccsBackendUpdateSetting (CCSBackendConformanceTestParameterized::GetBackend (), context.get (), plugin.get (), setting.get ());
}

TEST_P (CCSBackendConformanceTestReadWrite, TestUpdateKeyedValue)
{
    SCOPED_TRACE (CCSBackendConformanceTestParameterized::GetParam ()->what () + "UpdateMocked");

    CCSBackendConformanceTestParameterized::GetParam ()->testEnv ()->PreUpdate (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());
    CCSBackendConformanceTestParameterized::GetParam ()->nativeWrite (CCSBackendConformanceTestParameterized::GetParam ()->testEnv (),
							 pluginName, settingName, VALUE);
    CCSBackendConformanceTestParameterized::GetParam ()->testEnv ()->PostUpdate (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());
    CCSBackendConformanceTestParameterized::GetParam ()->setReadExpectation () (gmockSetting, VALUE);

    EXPECT_CALL (*gmockContext, findPlugin (_)).WillOnce (Return (plugin.get ()));
    EXPECT_CALL (*gmockPlugin, findSetting (_)).WillOnce (Return (setting.get ()));

    EXPECT_TRUE (CCSBackendConformanceTestParameterized::GetParam ()->testEnv ()->UpdateSettingAtKey (pluginName, settingName));
}

TEST_P (CCSBackendConformanceTestReadWrite, TestWriteValue)
{
    SCOPED_TRACE (CCSBackendConformanceTestParameterized::GetParam ()->what () + "Write");

    CCSBackendConformanceTestParameterized::GetParam ()->testEnv ()->PreWrite (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());
    CCSBackendConformanceTestParameterized::GetParam ()->setWriteExpectationAndWrite () (pluginName,
									   settingName,
									   VALUE,
									   setting,
									   boost::bind (ccsBackendWriteSetting,
											CCSBackendConformanceTestParameterized::GetBackend (),
											context.get (),
											setting.get ()),
									   GetParam ()->testEnv ());
    CCSBackendConformanceTestParameterized::GetParam ()->testEnv ()->PostWrite (gmockContext, gmockPlugin, gmockSetting, GetParam ()->type ());

}

class CCSBackendConformanceTestInfo :
    public CCSBackendConformanceTestParameterizedByBackendFixture
{
};

TEST_P (CCSBackendConformanceTestInfo, TestGetInfo)
{
    const CCSBackendInfo *knownBackendInfo = mTestEnv->GetInfo ();
    const CCSBackendInfo *retreivedBackendInfo = ccsBackendGetInfo (GetBackend ());

    EXPECT_THAT (retreivedBackendInfo->name, Eq (knownBackendInfo->name));
    EXPECT_THAT (retreivedBackendInfo->shortDesc, Eq (knownBackendInfo->shortDesc));
    EXPECT_THAT (retreivedBackendInfo->longDesc, Eq (knownBackendInfo->longDesc));

    if (knownBackendInfo->profileSupport)
	EXPECT_TRUE (retreivedBackendInfo->profileSupport);
    else
	EXPECT_FALSE (retreivedBackendInfo->profileSupport);

    if (knownBackendInfo->integrationSupport)
	EXPECT_TRUE (retreivedBackendInfo->integrationSupport);
    else
	EXPECT_FALSE (retreivedBackendInfo->integrationSupport);
}

#endif

