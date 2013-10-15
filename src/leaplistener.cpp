/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "leaplistener.h"
#include <QDebug>

using namespace Leap;

static const float BIG_CIRCLE_THRESHOLD = 70.0f;

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
//    m_controller.enableGesture(Gesture::TYPE_KEY_TAP);
//    m_controller.enableGesture(Gesture::TYPE_SCREEN_TAP);
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
    if (gestures.count())
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
                      <<  ", " << clockwiseness
                      << std::endl;
            if (circle.pointable().direction().angleTo(circle.normal()) <= PI/4) {
                if (circle.radius() < BIG_CIRCLE_THRESHOLD)
                    emit jogRightFrame();
                else
                    emit jogRightSecond();
            } else {
                if (circle.radius() < BIG_CIRCLE_THRESHOLD)
                    emit jogLeftFrame();
                else
                    emit jogLeftSecond();
            }
            break;
        }
        case Gesture::TYPE_KEY_TAP:
        {
            KeyTapGesture tap = gesture;
            std::cerr << "Key Tap id: " << gesture.id()
                      << ", state: " << gesture.state()
                      << ", position: " << tap.position()
                      << ", direction: " << tap.direction()
                      << std::endl;
            emit keyTap();
            break;
        }
        case Gesture::TYPE_SCREEN_TAP:
        {
            ScreenTapGesture screentap = gesture;
            std::cerr << "Screen Tap id: " << gesture.id()
                      << ", state: " << gesture.state()
                      << ", position: " << screentap.position()
                      << ", direction: " << screentap.direction()
                      << std::endl;
            emit screenTap();
            break;
        }
        default:
            std::cout << "Unknown gesture type." << std::endl;
            break;
        }
    }
    else if (!frame.hands().isEmpty()) {
        // Get the first hand
        const Hand hand = frame.hands()[0];

        if (hand.fingers().count() == 0) {
            const Vector palmPosition = frame.interactionBox().normalizePoint(hand.palmPosition());
            std::cerr << "X: " << (1.0 - palmPosition.x)
                      << " rotat prob: " << hand.rotationProbability(m_lastFrame)
                      << " trans prob: " << hand.translationProbability(m_lastFrame)
                      << std::endl;
            emit shuttle(2.0f * (palmPosition.x - 0.5f));
        } else {
            emit shuttle(0);
        }
    }
    m_lastFrame = frame;
}
