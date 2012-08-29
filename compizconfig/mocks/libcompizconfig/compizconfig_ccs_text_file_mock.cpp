#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ccs-object.h>
#include <ccs_text_file_interface.h>
#include "compizconfig_ccs_text_file_mock.h"

const CCSTextFileInterface ccsMockTextFileInterface =
{
    CCSTextFileGMock::ccsTextFileReadFromStart,
    CCSTextFileGMock::ccsTextFileAppendString,
    CCSTextFileGMock::ccsFreeTextFile
};

static void
finalizeAndFreeTextFile (CCSTextFile *textFile)
{
    CCSObjectAllocationInterface *ai = textFile->object.object_allocation;

    ccsObjectFinalize (textFile);
    (*ai->free_) (ai->allocator, textFile);
}

void
ccsFreeMockTextFile (CCSTextFile *textFile)
{
    CCSTextFileGMockInterface *gmockInterface = reinterpret_cast <CCSTextFileGMockInterface *> (ccsObjectGetPrivate (textFile));
    delete gmockInterface;

    finalizeAndFreeTextFile (textFile);

}

static CCSTextFileGMockInterface *
newCCSTextFileGMockInterface (CCSTextFile *textFile)
{
    CCSTextFileGMockInterface *gmock = new CCSTextFileGMock ();

    if (!gmock)
    {
	finalizeAndFreeTextFile (textFile);
	return NULL;
    }

    return gmock;
}

static CCSTextFile *
allocateCCSTextFile (CCSObjectAllocationInterface *ai)
{
    CCSTextFile *textFile = reinterpret_cast <CCSTextFile *> ((*ai->calloc_) (ai->allocator, 1, sizeof (CCSTextFile)));

    if (!textFile)
	return NULL;

    ccsObjectInit (textFile, ai);

    return textFile;
}

CCSTextFile *
ccsMockTextFileNew (CCSObjectAllocationInterface *ai)
{
    CCSTextFile *textFile = allocateCCSTextFile (ai);

    if (!textFile)
	return NULL;

    CCSTextFileGMockInterface *gmockInterface = newCCSTextFileGMockInterface (textFile);

    if (!gmockInterface)
	return NULL;

    ccsObjectSetPrivate (textFile, (CCSPrivate *) gmockInterface);
    ccsObjectAddInterface (textFile,
			   (const CCSInterface *) &ccsMockTextFileInterface,
			   GET_INTERFACE_TYPE (CCSTextFileInterface));
    ccsObjectRef (textFile);

    return textFile;
}
