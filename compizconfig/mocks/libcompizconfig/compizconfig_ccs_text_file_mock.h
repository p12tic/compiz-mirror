#ifndef COMPIZCONFIG_CCS_TEST_FILE_MOCK_H
#define COMPIZCONFIG_CCS_TEST_FILE_MOCK_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ccs-object.h>
#include <ccs-defs.h>

COMPIZCONFIG_BEGIN_DECLS

typedef struct _CCSTextFile CCSTextFile;
typedef struct _CCSObjectAllocationInterface CCSObjectAllocationInterface;

CCSTextFile * ccsMockTextFileNew (CCSObjectAllocationInterface *ai);
void          ccsFreeMockTextFile (CCSTextFile *);

COMPIZCONFIG_END_DECLS

class CCSTextFileGMockInterface
{
    public:

	virtual ~CCSTextFileGMockInterface () {}
	virtual char * readFromStart () = 0;
	virtual Bool   appendString (const char *) = 0;
	virtual void   free () = 0;
};

class CCSTextFileGMock :
    public CCSTextFileGMockInterface
{
    public:

	MOCK_METHOD0 (readFromStart, char * ());
	MOCK_METHOD1 (appendString, Bool (const char *));
	MOCK_METHOD0 (free, void ());

    public:

	static char *
	ccsTextFileReadFromStart (CCSTextFile *file)
	{
	    return reinterpret_cast <CCSTextFileGMock *> (ccsObjectGetPrivate (file))->readFromStart ();
	}

	static Bool
	ccsTextFileAppendString (CCSTextFile *file, const char *str)
	{
	    return reinterpret_cast <CCSTextFileGMock *> (ccsObjectGetPrivate (file))->appendString (str);
	}

	static void
	ccsFreeTextFile (CCSTextFile *file)
	{
	    reinterpret_cast <CCSTextFileGMock *> (ccsObjectGetPrivate (file))->free ();
	    ccsFreeMockTextFile (file);
	}

};

#endif
