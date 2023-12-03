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

    void testFindMatrixComponents_data()
    {
        QTest::addColumn<QMatrix4x4>("matrix");
        QTest::addColumn<QVector3D> ("expectedTranslation");
        QTest::addColumn<QVector3D> ("expectedScale");
        QTest::addColumn<QMatrix4x4>("expectedRotation");

        const auto mixed = [](const QVector3D &v)
        {
            const auto s = scaled({v.x() ? 2.0f : 1.0f,
                                   v.y() ? 4.0f : 1.0f,
                                   v.z() ? 8.0f : 1.0f});

            const auto t = translated({v.x() * 1.0f,
                                       v.y() * 2.0f,
                                       v.z() * 3.0f});

            return t * rotated(90, v) * s;
        };

        const auto t0   = QVector3D{0, 0, 0};
        const auto tx   = QVector3D{1, 0, 0};
        const auto ty   = QVector3D{0, 2, 0};
        const auto tz   = QVector3D{0, 0, 3};
        const auto txyz = QVector3D{1, 2, 3};

        const auto s0   = QVector3D{1, 1, 1};
        const auto sx   = QVector3D{2, 1, 1};
        const auto sy   = QVector3D{1, 4, 1};
        const auto sz   = QVector3D{1, 1, 8};
        const auto sxyz = QVector3D{2, 4, 8};

        const auto r0   = identity();
        const auto rx   = rotated(90, {1, 0, 0});
        const auto ry   = rotated(90, {0, 1, 0});
        const auto rz   = rotated(90, {0, 0, 1});
        const auto rxyz = rotated(90, {1, 1, 1});

        QTest::newRow("identity")       <<    identity()          << t0   << s0   << r0;
        QTest::newRow("translated-x")   <<  translated({1, 0, 0}) << tx   << s0   << r0;
        QTest::newRow("translated-y")   <<  translated({0, 2, 0}) << ty   << s0   << r0;
        QTest::newRow("translated-z")   <<  translated({0, 0, 3}) << tz   << s0   << r0;
        QTest::newRow("translated-xyz") <<  translated({1, 2, 3}) << txyz << s0   << r0;
        QTest::newRow("scaled-x")       <<      scaled({2, 1, 1}) << t0   << sx   << r0;
        QTest::newRow("scaled-y")       <<      scaled({1, 4, 1}) << t0   << sy   << r0;
        QTest::newRow("scaled-z")       <<      scaled({1, 1, 8}) << t0   << sz   << r0;
        QTest::newRow("scaled-xyz")     <<      scaled({2, 4, 8}) << t0   << sxyz << r0;
        QTest::newRow("rotated-x")      << rotated(90, {1, 0, 0}) << t0   << s0   << rx;
        QTest::newRow("rotated-y")      << rotated(90, {0, 1, 0}) << t0   << s0   << ry;
        QTest::newRow("rotated-z")      << rotated(90, {0, 0, 1}) << t0   << s0   << rz;
        QTest::newRow("rotated-xyz")    << rotated(90, {1, 1, 1}) << t0   << s0   << rxyz;
        QTest::newRow("mixed-x")        <<       mixed({1, 0, 0}) << tx   << sx   << rx;
        QTest::newRow("mixed-y")        <<       mixed({0, 1, 0}) << ty   << sy   << ry;
        QTest::newRow("mixed-z")        <<       mixed({0, 0, 1}) << tz   << sz   << rz;
        QTest::newRow("mixed-xyz")      <<       mixed({1, 1, 1}) << txyz << sxyz << rxyz;
    }

    void testFindMatrixComponents()
    {
        const QFETCH(QMatrix4x4, matrix);
        const QFETCH(QVector3D,  expectedTranslation);
        const QFETCH(QVector3D,  expectedScale);
        const QFETCH(QMatrix4x4, expectedRotation);

        QCOMPARE(findTranslation(matrix), expectedTranslation);
        QCOMPARE(findScale(matrix),       expectedScale);
        QCOMPARE(findRotation(matrix),    expectedRotation);
    }
};

} // namespace QtCSG::Tests

QTEST_MAIN(QtCSG::Tests::MathTest)

#include "qtcsgmathtest.moc"
