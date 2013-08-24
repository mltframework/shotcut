#include "leaplistener.h"
#include <QDebug>

using namespace Leap;

LeapListener::LeapListener(QObject *parent) :
    QObject(parent)
{
    m_controller.addListener(*this);
}

LeapListener::~LeapListener()
{
    m_controller.removeListener(*this);
}

void LeapListener::onConnect(const Leap::Controller &controller)
{
    Q_UNUSED(controller);
    m_controller.enableGesture(Gesture::TYPE_CIRCLE);
    m_controller.enableGesture(Gesture::TYPE_KEY_TAP);
    m_controller.enableGesture(Gesture::TYPE_SCREEN_TAP);
    m_controller.enableGesture(Gesture::TYPE_SWIPE);
}

void LeapListener::onFocusGained(const Leap::Controller &controller)
{
    m_lastFrame = controller.frame();
}

void LeapListener::onFrame(const Leap::Controller &controller)
{
    Leap::Frame frame = controller.frame();

    // Get gestures
    const GestureList gestures = frame.gestures();
    for (int g = 0; g < gestures.count(); ++g) {
        Gesture gesture = gestures[g];

        switch (gesture.type()) {
        case Gesture::TYPE_CIRCLE:
        {
            CircleGesture circle = gesture;
            std::string clockwiseness;

            if (circle.pointable().direction().angleTo(circle.normal()) <= PI/4) {
                clockwiseness = "clockwise";
            } else {
                clockwiseness = "counterclockwise";
            }

            // Calculate angle swept since last frame
            float sweptAngle = 0;
            if (circle.state() != Gesture::STATE_START) {
                CircleGesture previousUpdate = CircleGesture(controller.frame(1).gesture(circle.id()));
                sweptAngle = (circle.progress() - previousUpdate.progress()) * 2 * PI;
            }
            std::cerr << "Circle id: " << gesture.id()
                      << ", state: " << gesture.state()
                      << ", progress: " << circle.progress()
                      << ", radius: " << circle.radius()
                      << ", angle " << sweptAngle * RAD_TO_DEG
                      <<  ", " << clockwiseness << std::endl;
            if (gesture.state() == Gesture::STATE_STOP) {
                if (circle.pointable().direction().angleTo(circle.normal()) <= PI/4)
                    emit clockwiseCircle();
                else
                    emit counterClockwiseCircle();
            }
            break;
        }
        case Gesture::TYPE_SWIPE:
        {
            SwipeGesture swipe = gesture;
            std::cerr << "Swipe id: " << gesture.id()
                      << ", state: " << gesture.state()
                      << ", direction: " << swipe.direction()
                      << ", speed: " << swipe.speed() << std::endl;
            if (gesture.state() == Gesture::STATE_STOP) {
                if (swipe.direction().x > 0.5)
                    emit swipeRight();
                else if (swipe.direction().x < -0.5)
                    emit swipeLeft();
            }
            break;
        }
        case Gesture::TYPE_KEY_TAP:
        {
            KeyTapGesture tap = gesture;
            std::cerr << "Key Tap id: " << gesture.id()
                      << ", state: " << gesture.state()
                      << ", position: " << tap.position()
                      << ", direction: " << tap.direction()<< std::endl;
            emit keyTap();
            break;
        }
        case Gesture::TYPE_SCREEN_TAP:
        {
            ScreenTapGesture screentap = gesture;
            std::cerr << "Screen Tap id: " << gesture.id()
                      << ", state: " << gesture.state()
                      << ", position: " << screentap.position()
                      << ", direction: " << screentap.direction()<< std::endl;
            emit screenTap();
            break;
        }
        default:
            std::cout << "Unknown gesture type." << std::endl;
            break;
        }
    }
    m_lastFrame = frame;
}
