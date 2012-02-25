#ifndef ABSTRACTPRODUCERWIDGET_H
#define ABSTRACTPRODUCERWIDGET_H

#include <MltProducer.h>

class AbstractProducerWidget
{
public:
    AbstractProducerWidget();
    virtual ~AbstractProducerWidget();
    virtual Mlt::Producer* producer(Mlt::Profile&) = 0;
    virtual void setProducer(Mlt::Producer*);
    virtual Mlt::Properties* getPreset() const
        { return new Mlt::Properties; }
    virtual void loadPreset(Mlt::Properties&) {}

protected:
    Mlt::Producer* m_producer;
};

#endif // ABSTRACTPRODUCERWIDGET_H
