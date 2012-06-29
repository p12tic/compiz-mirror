#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ccs.h>

class CCSPluginGMockInterface
{
    public:

	virtual ~CCSPluginGMockInterface () {};

	virtual char * getName () = 0;
	virtual char * getShortDesc () = 0;
	virtual char * getLongDesc () = 0;
	virtual char * getHints () = 0;
	virtual char * getCategory () = 0;
	virtual CCSStringList getLoadAfter () = 0;
	virtual CCSStringList getLoadBefore () = 0;
	virtual CCSStringList getRequiresPlugins () = 0;
	virtual CCSStringList getConflictPlugins () = 0;
	virtual CCSStringList getProvidesFeatures () = 0;
	virtual CCSStringList getRequiresFeatures () = 0;
	virtual void * getPrivatePtr () = 0;
	virtual void setPrivatePtr (void *) = 0;
	virtual CCSContext * getContext () = 0;
	virtual CCSSetting * findSetting (const char *name) = 0;
	virtual CCSSettingList getPluginSettings () = 0;
	virtual CCSGroupList getPluginGroups () = 0;
	virtual void readPluginSettings () = 0;
	virtual CCSStrExtensionList getPluginStrExtensions () = 0;
};

class CCSPluginGMock :
    public CCSPluginGMockInterface
{
    public:

	/* Mock implementations */
	MOCK_METHOD0 (getName, char * ());
	MOCK_METHOD0 (getShortDesc, char * ());
	MOCK_METHOD0 (getLongDesc, char * ());
	MOCK_METHOD0 (getHints, char * ());
	MOCK_METHOD0 (getCategory, char * ());
	MOCK_METHOD0 (getLoadAfter, CCSStringList ());
	MOCK_METHOD0 (getLoadBefore, CCSStringList ());
	MOCK_METHOD0 (getRequiresPlugins, CCSStringList ());
	MOCK_METHOD0 (getConflictPlugins, CCSStringList ());
	MOCK_METHOD0 (getProvidesFeatures, CCSStringList ());
	MOCK_METHOD0 (getRequiresFeatures, CCSStringList ());
	MOCK_METHOD0 (getPrivatePtr, void * ());
	MOCK_METHOD1 (setPrivatePtr, void (void *));
	MOCK_METHOD0 (getContext, CCSContext * ());
	MOCK_METHOD1 (findSetting, CCSSetting * (const char *));
	MOCK_METHOD0 (getPluginSettings, CCSSettingList ());
	MOCK_METHOD0 (getPluginGroups, CCSGroupList ());
	MOCK_METHOD0 (readPluginSettings, void ());
	MOCK_METHOD0 (getPluginStrExtensions, CCSStrExtensionList ());

    public:

	/* Thunking C to C++ */
	static char * ccsPluginGetName (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getName ();
	}

	static char * ccsPluginGetShortDesc (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getShortDesc ();
	}

	static char * ccsPluginGetLongDesc (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getLongDesc ();
	}

	static char * ccsPluginGetHints (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getHints ();
	}

	static char * ccsPluginGetCategory (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getCategory ();
	}

	static CCSStringList ccsPluginGetLoadAfter (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getLoadAfter ();
	}

	static CCSStringList ccsPluginGetLoadBefore (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getLoadBefore ();
	}

	static CCSStringList ccsPluginGetRequiresPlugins (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getRequiresPlugins ();
	}

	static CCSStringList ccsPluginGetConflictPlugins (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getConflictPlugins ();
	}

	static CCSStringList ccsPluginGetProvidesFeatures (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getProvidesFeatures ();
	}

	static CCSStringList ccsPluginGetRequiresFeatures (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getRequiresFeatures ();
	}

	static void * ccsPluginGetPrivatePtr (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getPrivatePtr ();
	}

	static void ccsPluginSetPrivatePtr (CCSPlugin *plugin, void *ptr)
	{
	    ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->setPrivatePtr (ptr);
	}

	static CCSContext * ccsPluginGetContext (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getContext ();
	}

	static CCSSettingList ccsGetPluginSettings (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getPluginSettings ();
	}

	static CCSGroupList ccsGetPluginGroups (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getPluginGroups ();
	}

	static CCSSetting *
	ccsFindSetting (CCSPlugin *plugin, const char *name)
	{
	    if (!plugin)
		return NULL;

	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->findSetting (name);
	}

	static void
	ccsReadPluginSettings (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->readPluginSettings ();
	}

	static CCSStrExtensionList ccsGetPluginStrExtensions (CCSPlugin *plugin)
	{
	    return ((CCSPluginGMock *) ccsObjectGetPrivate (plugin))->getPluginStrExtensions ();
	}
};

extern CCSPluginInterface CCSPluginGMockInterface;

CCSPlugin * ccsMockPluginNew ();
void ccsFreeMockPlugin (CCSPlugin *);
