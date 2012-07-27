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
};
