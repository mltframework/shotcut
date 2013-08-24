#ifndef LEAPLISTENER_H
#define LEAPLISTENER_H

#include <QObject>
#include <Leap.h>

class LeapListener : public QObject, Leap::Listener
{
    Q_OBJECT
public:
    explicit LeapListener(QObject *parent = 0);
    virtual ~LeapListener();

signals:
    void keyTap();
    void screenTap();
    void swipeLeft();
    void swipeRight();
    void clockwiseCircle();
    void counterClockwiseCircle();

private:
    Leap::Controller m_controller;
    Leap::Frame m_lastFrame;

    void onConnect(const Leap::Controller& controller);
    void onFocusGained(const Leap::Controller& controller);
    void onFrame(const Leap::Controller& controller);
};

#endif // LEAPLISTENER_H
