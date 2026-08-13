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

#include "models/mixtransitionorder.h"

#include <Mlt.h>
#include <QList>
#include <QScopedPointer>
#include <QString>
#include <QtTest>

class TestMixTransitionOrder : public QObject
{
    Q_OBJECT

private:
    static QList<int> reversed(const QList<int> &input)
    {
        QList<int> result;
        result.reserve(input.size());
        for (int i = input.size() - 1; i >= 0; --i) {
            result.append(input.at(i));
        }
        return result;
    }

    static QList<int> collectMixBTrackOrder(Mlt::Tractor &tractor)
    {
        QList<int> order;
        QScopedPointer<Mlt::Service> service(tractor.producer());
        while (service && service->is_valid()) {
            if (service->type() == mlt_service_transition_type) {
                Mlt::Transition transition((mlt_transition) service->get_service());
                if (QString::fromLatin1(transition.get("mlt_service")) == QStringLiteral("mix"))
                    order.append(transition.get_b_track());
            }
            service.reset(service->producer());
        }
        return order;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(Mlt::Factory::init(nullptr));
    }

    void cleanupTestCase()
    {
        Mlt::Factory::close();
    }

    void reordersMixTransitionsToTrackListOrder()
    {
        Mlt::Profile profile("dv_pal");
        QVERIFY(profile.is_valid());

        Mlt::Tractor tractor(profile);
        QVERIFY(tractor.is_valid());

        // Build track indexes 0..3 where track 0 is the base mix source.
        for (int i = 0; i <= 3; ++i) {
            Mlt::Playlist playlist(profile);
            QVERIFY(playlist.is_valid());
            playlist.blank(0);
            QCOMPARE(tractor.set_track(playlist, i), 0);
        }

        // Plant intentionally out-of-order mix transitions.
        Mlt::Transition mix2(profile, "mix");
        Mlt::Transition mix3(profile, "mix");
        Mlt::Transition mix1(profile, "mix");
        QVERIFY(mix2.is_valid());
        QVERIFY(mix3.is_valid());
        QVERIFY(mix1.is_valid());
        tractor.plant_transition(mix2, 0, 2);
        tractor.plant_transition(mix3, 0, 3);
        tractor.plant_transition(mix1, 0, 1);

        QList<int> initialOrder = collectMixBTrackOrder(tractor);
        QCOMPARE(initialOrder, QList<int>({1, 3, 2}));

        const QList<int> uiOrder = {3, 1, 2};
        Timeline::reorderMixTransitions(tractor, uiOrder, profile);

        QList<int> reordered = collectMixBTrackOrder(tractor);
        QCOMPARE(reordered, reversed(uiOrder));
    }

    void reordersMixTransitionsAfterMoveLikeOrderChange()
    {
        Mlt::Profile profile("dv_pal");
        QVERIFY(profile.is_valid());

        Mlt::Tractor tractor(profile);
        QVERIFY(tractor.is_valid());

        // Build track indexes 0..4 where track 0 is the base mix source.
        for (int i = 0; i <= 4; ++i) {
            Mlt::Playlist playlist(profile);
            QVERIFY(playlist.is_valid());
            playlist.blank(0);
            QCOMPARE(tractor.set_track(playlist, i), 0);
        }

        // Plant transitions in a non-UI order first.
        for (int bTrack : QList<int>({2, 4, 1, 3})) {
            Mlt::Transition mix(profile, "mix");
            QVERIFY(mix.is_valid());
            tractor.plant_transition(mix, 0, bTrack);
        }

        // Initial UI order (before move): V3, V2, V1, A1 -> bTrack 4,3,2,1.
        const QList<int> beforeMoveOrder = {4, 3, 2, 1};
        Timeline::reorderMixTransitions(tractor, beforeMoveOrder, profile);
        QCOMPARE(collectMixBTrackOrder(tractor), reversed(beforeMoveOrder));

        // Simulate moving the old top track below two others.
        // New UI order example: V2, V1, V3, A1 -> bTrack 3,2,4,1.
        const QList<int> afterMoveOrder = {3, 2, 4, 1};
        Timeline::reorderMixTransitions(tractor, afterMoveOrder, profile);
        QCOMPARE(collectMixBTrackOrder(tractor), reversed(afterMoveOrder));
    }
};

QTEST_APPLESS_MAIN(TestMixTransitionOrder)

#include "test_mix_transition_order.moc"
