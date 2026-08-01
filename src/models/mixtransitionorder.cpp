/*
 * Copyright (c) 2026 Meltytech, LLC
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

#include "mixtransitionorder.h"

#include <MltField.h>
#include <MltService.h>
#include <MltTransition.h>
#include <QScopedPointer>
#include <QString>

#include <utility>

namespace {
Mlt::Transition *getMixTransition(Mlt::Tractor &tractor, int trackIndex)
{
    QScopedPointer<Mlt::Service> service(tractor.producer());
    while (service && service->is_valid()) {
        if (service->type() == mlt_service_transition_type) {
            Mlt::Transition transition((mlt_transition) service->get_service());
            if (QString::fromLatin1(transition.get("mlt_service")) == QStringLiteral("mix")
                && transition.get_b_track() == trackIndex) {
                return new Mlt::Transition(transition);
            }
        }
        service.reset(service->producer());
    }
    return nullptr;
}
} // namespace

namespace Timeline {
void reorderMixTransitions(Mlt::Tractor &tractor,
                           const QList<int> &orderedTrackMltIndexes,
                           Mlt::Profile &profile)
{
    struct StoredMix
    {
        int bTrack;
        Mlt::Transition transition;
    };

    QList<StoredMix> storedTransitions;
    for (int bTrack : orderedTrackMltIndexes) {
        QScopedPointer<Mlt::Transition> transition(getMixTransition(tractor, bTrack));
        if (transition && transition->is_valid()) {
            StoredMix stored = {bTrack, *transition};
            storedTransitions.append(stored);
        }
    }

    if (storedTransitions.isEmpty())
        return;

    QScopedPointer<Mlt::Field> field(tractor.field());
    for (const StoredMix &stored : std::as_const(storedTransitions)) {
        QScopedPointer<Mlt::Transition> transition(getMixTransition(tractor, stored.bTrack));
        if (transition && transition->is_valid())
            field->disconnect_service(*transition);
    }

    // Plant in top-to-bottom UI order. The MLT service producer-chain traversal
    // enumerates transitions in reverse insertion order, so chain iteration may
    // look bottom-to-top even when effective mix application follows UI order.
    for (const StoredMix &stored : std::as_const(storedTransitions)) {
        Mlt::Transition previous(stored.transition);
        Mlt::Transition transition(profile, previous.get("mlt_service"));
        transition.inherit(previous);
        tractor.plant_transition(transition, 0, stored.bTrack);
    }
}
} // namespace Timeline
