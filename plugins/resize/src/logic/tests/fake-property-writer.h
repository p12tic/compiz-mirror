#ifndef RESIZE_FAKE_PROPERTY_WRITER_H
#define RESIZE_FAKE_PROPERTY_WRITER_H

#include "property-writer-interface.h"

namespace resize
{

class FakePropertyWriter : public PropertyWriterInterface
{
public:
    virtual bool updateProperty (Window, CompOption::Vector &, int)
    {
	return true;
    }

    virtual void deleteProperty (Window)
    {
    }

    virtual const CompOption::Vector & readProperty (Window)
    {
	return mPropertyValues;
    }

    virtual void setReadTemplate (const CompOption::Vector &v)
    {
	mPropertyValues = v;
    }

    virtual const CompOption::Vector & getReadTemplate ()
    {
	return mPropertyValues;
    }

    CompOption::Vector mPropertyValues;
};

} /* namespace resize */

#endif /* RESIZE_FAKE_PROPERTY_WRITER_H */
