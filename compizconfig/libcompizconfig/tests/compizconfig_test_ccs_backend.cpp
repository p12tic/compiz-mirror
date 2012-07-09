#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ccs.h>

#include "compizconfig_ccs_backend_mock.h"

using ::testing::_;
using ::testing::Return;

class CCSBackendTest :
    public ::testing::Test
{
};

TEST(CCSBackendTest, TestMock)
{
    CCSBackend *backend = ccsMockBackendNew ();
    CCSBackendGMock *mock = (CCSBackendGMock *) ccsObjectGetPrivate (backend);

    CCSStringList profiles = NULL;

    EXPECT_CALL (*mock, getName ());
    EXPECT_CALL (*mock, getShortDesc ());
    EXPECT_CALL (*mock, getLongDesc ());
    EXPECT_CALL (*mock, hasProfileSupport ());
    EXPECT_CALL (*mock, hasIntegrationSupport ());
    EXPECT_CALL (*mock, executeEvents (_));
    EXPECT_CALL (*mock, init (_));
    EXPECT_CALL (*mock, fini (_));
    EXPECT_CALL (*mock, readInit (_));
    EXPECT_CALL (*mock, readSetting (_,_));
    EXPECT_CALL (*mock, readDone (_));
    EXPECT_CALL (*mock, writeInit (_));
    EXPECT_CALL (*mock, writeSetting (_, _));
    EXPECT_CALL (*mock, writeDone (_));
    EXPECT_CALL (*mock, getSettingIsIntegrated (_));
    EXPECT_CALL (*mock, getSettingIsReadOnly (_));
    EXPECT_CALL (*mock, getExistingProfiles (_)).WillRepeatedly (Return (profiles));
    EXPECT_CALL (*mock, deleteProfile (_, _));

    ccsBackendGetName (backend);
    ccsBackendGetShortDesc (backend);
    ccsBackendGetLongDesc (backend);
    ccsBackendHasProfileSupport (backend);
    ccsBackendHasIntegrationSupport (backend);
    ccsBackendExecuteEvents (backend, 0);
    ccsBackendInit (backend, NULL);
    ccsBackendFini (backend, NULL);
    ccsBackendReadInit (backend, NULL);
    ccsBackendReadSetting (backend, NULL, NULL);
    ccsBackendReadDone (backend, NULL);
    ccsBackendWriteInit (backend, NULL);
    ccsBackendWriteSetting (backend, NULL, NULL);
    ccsBackendGetSettingIsIntegrated (backend, NULL);
    ccsBackendGetExistingProfiles (backend, NULL);
    ccsBackendDeleteProfile (backend, NULL, NULL);

    ccsBackendUnref (backend);
}
