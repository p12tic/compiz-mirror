#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ccs-backend.h>

CCSBackend * ccsMockBackendNew ();
void ccsFreeMockBackend (CCSBackend *);

class CCSBackendGMockInterface
{
    public:

	virtual ~CCSBackendGMockInterface () {};

	virtual char * getName () = 0;
	virtual char * getShortDesc () = 0;
	virtual char * getLongDesc () = 0;
	virtual Bool hasProfileSupport () = 0;
	virtual Bool hasIntegrationSupport () = 0;
	virtual void executeEvents (unsigned int) = 0;
	virtual Bool init (CCSContext *context) = 0;
	virtual Bool fini () = 0;
	virtual Bool readInit (CCSContext *context) = 0;
	virtual void readSetting (CCSContext *context, CCSSetting *setting) = 0;
	virtual void readDone (CCSContext *context) = 0;
	virtual Bool writeInit (CCSContext *conxtext) = 0;
	virtual void writeSetting (CCSContext *context, CCSSetting *setting) = 0;
	virtual void writeDone (CCSContext *context) = 0;
	virtual void updateSetting (CCSContext *context, CCSPlugin *plugin, CCSSetting *setting) = 0;
	virtual Bool getSettingIsIntegrated (CCSSetting *setting) = 0;
	virtual Bool getSettingIsReadOnly (CCSSetting *setting) = 0;
	virtual CCSStringList getExistingProfiles (CCSContext *context) = 0;
	virtual Bool deleteProfile (CCSContext *context, char *name) = 0;
};

class CCSBackendGMock :
    public CCSBackendGMockInterface
{
    public:

	/* Mock implementations */
	MOCK_METHOD0 (getName, char * ());
	MOCK_METHOD0 (getShortDesc, char * ());
	MOCK_METHOD0 (getLongDesc, char * ());
	MOCK_METHOD0 (hasProfileSupport, Bool ());
	MOCK_METHOD0 (hasIntegrationSupport, Bool ());
	MOCK_METHOD1 (executeEvents, void (unsigned int));
	MOCK_METHOD1 (init, Bool (CCSContext *));
	MOCK_METHOD0 (fini, Bool ());
	MOCK_METHOD1 (readInit, Bool (CCSContext *));
	MOCK_METHOD2 (readSetting, void (CCSContext *, CCSSetting *));
	MOCK_METHOD1 (readDone, void (CCSContext *));
	MOCK_METHOD1 (writeInit, Bool (CCSContext *));
	MOCK_METHOD2 (writeSetting, void (CCSContext *, CCSSetting *));
	MOCK_METHOD1 (writeDone, void (CCSContext *));
	MOCK_METHOD3 (updateSetting, void (CCSContext *, CCSPlugin *, CCSSetting *));
	MOCK_METHOD1 (getSettingIsIntegrated, Bool (CCSSetting *));
	MOCK_METHOD1 (getSettingIsReadOnly, Bool (CCSSetting *));
	MOCK_METHOD1 (getExistingProfiles, CCSStringList (CCSContext *));
	MOCK_METHOD2 (deleteProfile, Bool (CCSContext *, char *name));

    public:

	/* Thunking C to C++ */
	static char * ccsBackendGetName (CCSBackend *backend)
	{
	    return ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->getName ();
	}

	static char * ccsBackendGetShortDesc (CCSBackend *backend)
	{
	    return ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->getShortDesc ();
	}

	static char * ccsBackendGetLongDesc (CCSBackend *backend)
	{
	    return ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->getLongDesc ();
	}

	static Bool ccsBackendHasIntegrationSupport (CCSBackend *backend)
	{
	    return ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->hasIntegrationSupport ();
	}

	static Bool ccsBackendHasProfileSupport (CCSBackend *backend)
	{
	    return ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->hasProfileSupport ();
	}

	static void ccsBackendExecuteEvents (CCSBackend *backend, unsigned int flags)
	{
	    return ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->executeEvents (flags);
	}

	static Bool ccsBackendInit (CCSBackend *backend, CCSContext *context)
	{
	    return ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->init (context);
	}

	static Bool ccsBackendFini (CCSBackend *backend)
	{
	    return ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->fini ();
	}

	static Bool ccsBackendReadInit (CCSBackend *backend, CCSContext *context)
	{
	    return ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->readInit (context);
	}

	static void ccsBackendReadSetting (CCSBackend *backend, CCSContext *context, CCSSetting *setting)
	{
	    ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->readSetting (context, setting);
	}

	static void ccsBackendReadDone (CCSBackend *backend, CCSContext *context)
	{
	    ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->readDone (context);
	}

	static Bool ccsBackendWriteInit (CCSBackend *backend, CCSContext *context)
	{
	    return ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->writeInit (context);
	}

	static void ccsBackendWriteSetting (CCSBackend *backend, CCSContext *context, CCSSetting *setting)
	{
	    ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->writeSetting (context, setting);
	}

	static void ccsBackendWriteDone (CCSBackend *backend, CCSContext *context)
	{
	    ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->writeDone (context);
	}

	static void ccsBackendUpdateSetting (CCSBackend *backend, CCSContext *context, CCSPlugin *plugin, CCSSetting *setting)
	{
	    ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->updateSetting (context, plugin, setting);
	}

	static Bool ccsBackendGetSettingIsIntegrated (CCSBackend *backend, CCSSetting *setting)
	{
	    return ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->getSettingIsIntegrated (setting);
	}

	static Bool ccsBackendGetSettingIsReadOnly (CCSBackend *backend, CCSSetting *setting)
	{
	    return ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->getSettingIsReadOnly (setting);
	}

	static CCSStringList ccsBackendGetExistingProfiles (CCSBackend *backend, CCSContext *context)
	{
	    return ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->getExistingProfiles (context);
	}

	static Bool ccsBackendDeleteProfile (CCSBackend *backend, CCSContext *context, char *name)
	{
	    return ((CCSBackendGMock *) ccsObjectGetPrivate (backend))->deleteProfile (context, name);
	}

};

extern CCSBackendInterface CCSBackendGMockInterface;
