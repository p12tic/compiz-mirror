#ifndef RESIZE_PROPERTY_WRITER_IMPL_H
#define RESIZE_PROPERTY_WRITER_IMPL_H

#include "core/propertywriter.h"
#include "property-writer-interface.h"

namespace resize
{

class PropertyWriterImpl : public PropertyWriterInterface
{
public:
    PropertyWriterImpl (PropertyWriter *impl) : mImpl (impl) {}
    virtual ~PropertyWriterImpl() {delete mImpl;}

    virtual bool updateProperty (Window w, CompOption::Vector &v, int i)
    {
	return mImpl->updateProperty (w, v, i);
    }

    virtual void deleteProperty (Window w)
    {
	mImpl->deleteProperty (w);
    }

    virtual const CompOption::Vector & readProperty (Window w)
    {
	return mImpl->readProperty (w);
    }

    virtual void setReadTemplate (const CompOption::Vector &v)
    {
	mImpl->setReadTemplate (v);
    }

    virtual const CompOption::Vector & getReadTemplate ()
    {
	return mImpl->getReadTemplate ();
    }

    PropertyWriter* mImpl;
};

} /* namespace resize */

#endif /* RESIZE_PROPERTY_WRITER_IMPL_H */
