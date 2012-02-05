#ifndef ABSTRACTPRODUCERWIDGET_H
#define ABSTRACTPRODUCERWIDGET_H

#include <MltProducer.h>

class AbstractProducerWidget
{
public:
    virtual Mlt::Producer* producer(Mlt::Profile&) = 0;
    virtual void setProducer(Mlt::Producer*) {};
    virtual Mlt::Properties* getPreset() const
        { return new Mlt::Properties; }
    virtual void loadPreset(Mlt::Properties&) {}
};

#endif // ABSTRACTPRODUCERWIDGET_H
