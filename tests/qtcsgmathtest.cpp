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

#include <qtcsg/qtcsgmath.h>

namespace QtCSG::Tests {

class MathTest : public QObject
{
    Q_OBJECT

private slots:
    void testLerp_data()
    {
        QTest::addColumn<QVector3D>("a");
        QTest::addColumn<QVector3D>("b");
        QTest::addColumn<float>    ("t");
        QTest::addColumn<QVector3D>("expectedResult");

        QTest::newRow("start-x")    << QVector3D{0, 0, 0} << QVector3D{1, 0, 0} << 0.0f << QVector3D{0.0f, 0.0f, 0.0f};
        QTest::newRow("start-y")    << QVector3D{0, 0, 0} << QVector3D{0, 1, 0} << 0.0f << QVector3D{0.0f, 0.0f, 0.0f};
        QTest::newRow("start-z")    << QVector3D{0, 0, 0} << QVector3D{0, 0, 1} << 0.0f << QVector3D{0.0f, 0.0f, 0.0f};
        QTest::newRow("start-xyz")  << QVector3D{0, 0, 0} << QVector3D{1, 1, 1} << 0.0f << QVector3D{0.0f, 0.0f, 0.0f};

        QTest::newRow("middle-x")   << QVector3D{0, 0, 0} << QVector3D{1, 0, 0} << 0.5f << QVector3D{0.5f, 0.0f, 0.0f};
        QTest::newRow("middle-y")   << QVector3D{0, 0, 0} << QVector3D{0, 1, 0} << 0.5f << QVector3D{0.0f, 0.5f, 0.0f};
        QTest::newRow("middle-z")   << QVector3D{0, 0, 0} << QVector3D{0, 0, 1} << 0.5f << QVector3D{0.0f, 0.0f, 0.5f};
        QTest::newRow("middle-xyz") << QVector3D{0, 0, 0} << QVector3D{1, 1, 1} << 0.5f << QVector3D{0.5f, 0.5f, 0.5f};

        QTest::newRow("end-x")      << QVector3D{0, 0, 0} << QVector3D{1, 0, 0} << 1.0f << QVector3D{1.0f, 0.0f, 0.0f};
        QTest::newRow("end-y")      << QVector3D{0, 0, 0} << QVector3D{0, 1, 0} << 1.0f << QVector3D{0.0f, 1.0f, 0.0f};
        QTest::newRow("end-z")      << QVector3D{0, 0, 0} << QVector3D{0, 0, 1} << 1.0f << QVector3D{0.0f, 0.0f, 1.0f};
        QTest::newRow("end-xyz")    << QVector3D{0, 0, 0} << QVector3D{1, 1, 1} << 1.0f << QVector3D{1.0f, 1.0f, 1.0f};
    }

    void testLerp()
    {
        const QFETCH(QVector3D, a);
        const QFETCH(QVector3D, b);
        const QFETCH(float,     t);
        const QFETCH(QVector3D, expectedResult);
        QCOMPARE(lerp(a, b, t), expectedResult);
    }
};

} // namespace QtCSG::Tests

QTEST_MAIN(QtCSG::Tests::MathTest)

#include "qtcsgmathtest.moc"
