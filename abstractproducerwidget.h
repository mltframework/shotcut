#ifndef ABSTRACTPRODUCERWIDGET_H
#define ABSTRACTPRODUCERWIDGET_H

#include <MltProperties.h>

class AbstractProducerWidget
{
public:
    virtual QString producerName() const = 0;
    virtual QString URL() const
        { return producerName() + ":"; }
    virtual Mlt::Properties* mltProperties()
        { return new Mlt::Properties; }
    virtual void load(Mlt::Properties&)
        {}
};

#endif // ABSTRACTPRODUCERWIDGET_H
