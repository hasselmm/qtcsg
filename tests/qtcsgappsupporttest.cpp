/* QtCSG provides Constructive Solid Geometry (CSG) for Qt
 * Copyright Ⓒ 2023 Mathias Hasselmann
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "qtcsgtest.h"

#include <qtcsg/qtcsg.h>

#include <demo/qtcsgappsupport.h>

namespace QtCSG::Tests {

class AppSupportTest : public QObject
{
    Q_OBJECT

public:
    enum MetaEvent { None, Any };
    Q_ENUM(MetaEvent)

private slots:
    void testMetaEnum()
    {
        struct Mode : public AppSupport::MultiEnum<Mode, Inspection::Event, MetaEvent>
        {
            using MultiEnum::MultiEnum;
            using enum Inspection::Event;
            using enum MetaEvent;
        };

        auto mode = Mode{Mode::None};
        QCOMPARE(mode, Mode::None);

        //        auto variant = QVariant::fromValue(mode);
        //        QCOMPARE(variant.userType(), qMetaTypeId<Mode>());
        auto variant = static_cast<QVariant>(mode);
        // QCOMPARE(variant.userType(), qMetaTypeId<Mode>());
        QCOMPARE(variant, Mode{Mode::None});

        QVERIFY(QMetaType::canConvert(variant.metaType(), QMetaType::fromType<Mode>()));

        mode = qvariant_cast<Mode>(variant);
        QCOMPARE(mode, Mode::None);

        variant = QVariant::fromValue(Mode::Clip);
        QCOMPARE(variant, Mode{Mode::Clip});

        const auto t1 = QMetaType::fromType<AppSupport::MultiEnum<Mode, Inspection::Event, MetaEvent>>();
        const auto t2 = QMetaType::fromType<Mode>();
        const auto t3 = QMetaType::fromType<Inspection::Event>();
        const auto t4 = QMetaType::fromType<MetaEvent>();

        QVERIFY(QMetaType::canConvert(t1, t2));
        QVERIFY(QMetaType::canConvert(t1, t3));
        QVERIFY(QMetaType::canConvert(t1, t4));

        QVERIFY(QMetaType::canConvert(t2, t1));
        QVERIFY(QMetaType::canConvert(t2, t3));
        QVERIFY(QMetaType::canConvert(t2, t4));

        QVERIFY(QMetaType::canConvert(t3, t1));
        QVERIFY(QMetaType::canConvert(t3, t2));

        QVERIFY(QMetaType::canConvert(t4, t1));
        QVERIFY(QMetaType::canConvert(t4, t2));
    }
};

} // namespace QtCSG::Tests

QTEST_MAIN(QtCSG::Tests::AppSupportTest)

#include "qtcsgappsupporttest.moc"
