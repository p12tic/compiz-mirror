#ifndef RESIZE_PROPERTY_WRITER_IMPL_H
#define RESIZE_PROPERTY_WRITER_IMPL_H

#include "propertywriter.h"
#include "property-writer-interface.h"

namespace resize
{

class PropertyWriterImpl : public PropertyWriterInterface
{
public:
    PropertyWriterImpl (PropertyWriter *impl) : mImpl (impl) {}
    virtual ~PropertyWriterImpl() {delete impl;}

    virtual bool updateProperty (Window w, CompOption::Vector &v, int i)
    {
	impl->updateProperty (w, v, i);
    }

    virtual void deleteProperty (Window w)
    {
	impl->deleteProperty (w);
    }

    virtual const CompOption::Vector & readProperty (Window w)
    {
	return impl->readProperty (w);
    }

    virtual void setReadTemplate (const CompOption::Vector &v)
    {
	impl->setReadTemplate (v);
    }

    virtual const CompOption::Vector & getReadTemplate ()
    {
	return impl->getReadTemplate ();
    }

    PropertyWriter* mImpl;
};

} /* namespace resize */

#endif /* RESIZE_PROPERTY_WRITER_IMPL_H */
