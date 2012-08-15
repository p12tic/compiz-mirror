#ifndef RESIZE_PROPERTY_WRITER_INTERFACE_H
#define RESIZE_PROPERTY_WRITER_INTERFACE_H

namespace resize
{

class PropertyWriterInterface
{
public:
    virtual ~PropertyWriterInterface() {}

    virtual bool updateProperty (Window, CompOption::Vector &, int) = 0;
    virtual void deleteProperty (Window) = 0;
    virtual const CompOption::Vector & readProperty (Window) = 0;
    virtual void setReadTemplate (const CompOption::Vector &) = 0;
    virtual const CompOption::Vector & getReadTemplate () = 0;
};

} /* namespace resize */

#endif /* RESIZE_PROPERTY_WRITER_INTERFACE_H */
