#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ccs-backend.h>

class CCSBackendConceptTestEnvironmentInterface
{
    public:

	virtual ~CCSBackendConceptTestEnvironmentInterface () {};
	virtual CCSBackend * SetUp () = 0;
	virtual void TearDown (CCSBackend *) = 0;
};

class CCSBackendConformanceTest :
    public ::testing::TestWithParam <CCSBackendConceptTestEnvironmentInterface *>
{
    public:

	CCSBackend * GetBackend ()
	{
	    return mBackend;
	}

	void SetUp ()
	{
	    mBackend = GetParam ()->SetUp ();
	}

	void TearDown ()
	{
	    GetParam ()->TearDown (mBackend);
	}
    private:

	CCSBackend *mBackend;
};

