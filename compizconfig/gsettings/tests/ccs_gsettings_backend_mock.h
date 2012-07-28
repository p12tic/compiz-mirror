#ifndef _COMPIZCONFIG_CCS_GSETTINGS_BACKEND_MOCK
#define _COMPIZCONFIG_CCS_GSETTINGS_BACKEND_MOCK

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <gsettings_shared.h>
#include <ccs-backend.h>

using ::testing::_;
using ::testing::Return;

CCSBackend * ccsGSettingsBackendGMockNew ();
void ccsGSettingsBackendGMockFree (CCSBackend *backend);

class CCSGSettingsBackendGMockInterface
{
    public:

	virtual ~CCSGSettingsBackendGMockInterface () {}

	virtual CCSContext * getContext () = 0;
	virtual void connectToChangedSignal (GObject *) = 0;
	virtual GSettings * getSettingsObjectForPluginWithPath (const char * plugin,
								const char * path,
								CCSContext * context) = 0;
	virtual void registerGConfClient () = 0;
	virtual void unregisterGConfClient () = 0;
	virtual GVariant * getExistingProfiles () = 0;
	virtual void setExistingProfiles (GVariant *) = 0;
	virtual void setCurrentProfile (const gchar *) = 0;
};

class CCSGSettingsBackendGMock :
    public CCSGSettingsBackendGMockInterface
{
    public:

	CCSGSettingsBackendGMock (CCSBackend *backend) :
	    mBackend (backend)
	{
	}

	MOCK_METHOD0 (getContext, CCSContext * ());
	MOCK_METHOD1 (connectToChangedSignal, void (GObject *));
	MOCK_METHOD3 (getSettingsObjectForPluginWithPath, GSettings * (const char * plugin,
								       const char * path,
								       CCSContext * context));
	MOCK_METHOD0 (registerGConfClient, void ());
	MOCK_METHOD0 (unregisterGConfClient, void ());
	MOCK_METHOD0 (getExistingProfiles, GVariant * ());
	MOCK_METHOD1 (setExistingProfiles, void (GVariant *));
	MOCK_METHOD1 (setCurrentProfile, void (const gchar *));

	CCSBackend * getBackend () { return mBackend; }

    private:

	CCSBackend *mBackend;

    public:

	static CCSContext *
	ccsGSettingsBackendGetContext (CCSBackend *backend)
	{
	    return (reinterpret_cast <CCSGSettingsBackendGMock *> (ccsObjectGetPrivate (backend)))->getContext ();
	}

	static void
	ccsGSettingsBackendConnectToValueChangedSignal (CCSBackend *backend, GObject *object)
	{
	    (reinterpret_cast <CCSGSettingsBackendGMock *> (ccsObjectGetPrivate (backend)))->connectToChangedSignal(object);
	}

	static GSettings *
	ccsGSettingsBackendGetSettingsObjectForPluginWithPath (CCSBackend *backend,
							       const char *plugin,
							       const char *path,
							       CCSContext *context)
	{
	    return (reinterpret_cast <CCSGSettingsBackendGMock *> (ccsObjectGetPrivate (backend)))->getSettingsObjectForPluginWithPath (plugin,
																	path,
																	context);
	}

	static void
	ccsGSettingsBackendRegisterGConfClient (CCSBackend *backend)
	{
	    (reinterpret_cast <CCSGSettingsBackendGMock *> (ccsObjectGetPrivate (backend)))->registerGConfClient ();
	}

	static void
	ccsGSettingsBackendUnregisterGConfClient (CCSBackend *backend)
	{
	    (reinterpret_cast <CCSGSettingsBackendGMock *> (ccsObjectGetPrivate (backend)))->unregisterGConfClient ();
	}

	static GVariant *
	ccsGSettingsBackendGetExistingProfiles (CCSBackend *backend)
	{
	    return (reinterpret_cast <CCSGSettingsBackendGMock *> (ccsObjectGetPrivate (backend)))->getExistingProfiles ();
	}

	static void
	ccsGSettingsBackendSetExistingProfiles (CCSBackend *backend, GVariant *value)
	{
	    (reinterpret_cast <CCSGSettingsBackendGMock *> (ccsObjectGetPrivate (backend)))->setExistingProfiles (value);
	}

	static void
	ccsGSettingsBackendSetCurrentProfile (CCSBackend *backend, const gchar *value)
	{
	    (reinterpret_cast <CCSGSettingsBackendGMock *> (ccsObjectGetPrivate (backend)))->setCurrentProfile (value);
	}
};

#endif
