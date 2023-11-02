/* QtCSG provides Constructive Solid Geometry (CSG) for Qt
 * Copyright Ⓒ 2022 Mathias Hasselmann
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
#include <qtcsg/qtcsg.h>

#include <QTest>

namespace QtCSG::Tests {

using std::make_pair;

class Test : public QObject
{
    Q_OBJECT

private slots:
    void testCube()
    {
        const auto polygons = cube().polygons();

        QCOMPARE(polygons.count(), 6);

        for (int i = 0; i < polygons.count(); ++i)
            QCOMPARE(make_pair(i, static_cast<int>(polygons[i].vertices().count())), make_pair(i, 4));

        const auto vertices = polygons[0].vertices();
        const auto expectedNormal = QVector3D{-1, 0, 0};

        QCOMPARE(vertices[0].position(), (QVector3D{-1, -1, -1}));
        QCOMPARE(vertices[1].position(), (QVector3D{-1, -1,  1}));
        QCOMPARE(vertices[2].position(), (QVector3D{-1,  1,  1}));
        QCOMPARE(vertices[3].position(), (QVector3D{-1,  1, -1}));
        QCOMPARE(vertices[0].normal(),   expectedNormal);
        QCOMPARE(vertices[1].normal(),   expectedNormal);
        QCOMPARE(vertices[2].normal(),   expectedNormal);
        QCOMPARE(vertices[3].normal(),   expectedNormal);
    }

    void testSphere()
    {
        const auto polygons = sphere().polygons();

        QCOMPARE(polygons.count(), 128);

        for (int i = 0; i < polygons.count(); ++i) {
            QCOMPARE(make_pair(i, static_cast<int>(polygons[i].vertices().count())),
                     make_pair(i, i % 8 == 0 || i % 8 == 7 ? 3 : 4));
        }
    }

    void testCylinder()
    {
        const auto polygons = cylinder().polygons();

        QCOMPARE(polygons.count(), 48);

        for (int i = 0; i < polygons.count(); ++i) {
            QCOMPARE(make_pair(i, static_cast<int>(polygons[i].vertices().count())),
                     make_pair(i, i % 3 != 1 ? 3 : 4));
        }
    }

    void testUnion_data()
    {
        QTest::addColumn<float>("deltaX");
        QTest::addColumn<float>("deltaY");
        QTest::addColumn<float>("deltaZ");

        QTest::addColumn<int>("expectedPolygonCount");

        QTest::newRow("identity")        << 0.0f << 0.0f << 0.0f << 6 * 1;

        QTest::newRow("overlapping:xyz") << 0.5f << 0.5f << 0.5f << 6 * 4;
        QTest::newRow("adjacent:xyz")    << 1.0f << 1.0f << 1.0f << 6 * 2;
        QTest::newRow("distant:xyz")     << 1.5f << 1.5f << 1.5f << 6 * 2;

        QTest::newRow("overlapping:x")   << 0.5f << 0.0f << 0.0f << 4 * 3 + 2;
        QTest::newRow("adjacent:x")      << 1.0f << 0.0f << 0.0f << 6 * 2 - 2;
        QTest::newRow("distant:x")       << 1.5f << 0.0f << 0.0f << 6 * 2;
    }

    void testUnion()
    {
        const QFETCH(float, deltaX);
        const QFETCH(float, deltaY);
        const QFETCH(float, deltaZ);

        const QFETCH(int, expectedPolygonCount);

        const auto a = cube({-deltaX, -deltaY, +deltaZ});
        const auto b = cube({+deltaX, +deltaY, -deltaZ});
        const auto c = merge(a, b);

        if (qFuzzyCompare(deltaX, 0)
            && qFuzzyCompare(deltaY, 0)
            && qFuzzyCompare(deltaZ, 0))
            QCOMPARE(a.polygons(), b.polygons());

        QCOMPARE(a.polygons().count(), 6);
        QCOMPARE(b.polygons().count(), 6);
        QCOMPARE(c.polygons().count(), expectedPolygonCount);
    }

    void testNodeConstruct()
    {
        const auto expectedNormal = QVector3D{-1, 0, 0};
        const auto node = Node{cube().polygons()};

        {
            auto depth = 0;

            for (auto subNode = &node; subNode; subNode = subNode->back().get(), ++depth) {
                QCOMPARE(make_pair(depth, static_cast<int>(subNode->polygons().count())),
                         make_pair(depth, 1));
                QCOMPARE(make_pair(depth, static_cast<int>(subNode->polygons().constFirst().vertices().count())),
                         make_pair(depth, 4));
                QCOMPARE(make_pair(depth, !!subNode->front()),
                         make_pair(depth, false));
                QCOMPARE(make_pair(depth, !!subNode->back()),
                         make_pair(depth, depth < 5));
            }
        }

        QCOMPARE(node.allPolygons().count(), 6);

        const auto plane = node.plane();

        QVERIFY(!plane.isNull());
        QCOMPARE(plane.normal(), expectedNormal);
        QCOMPARE(plane.w(), 1);
    }

    void testNodeInvert()
    {
        const auto expectedNormal = QVector3D{1, 0, 0};
        const auto node = Node{cube().polygons()}.inverted();

        {
            auto depth = 0;

            for (auto subNode = &node; subNode; subNode = subNode->back().get(), ++depth) {
                QCOMPARE(make_pair(depth, static_cast<int>(subNode->polygons().count())),
                         make_pair(depth, 1));
                QCOMPARE(make_pair(depth, static_cast<int>(subNode->polygons().constFirst().vertices().count())),
                         make_pair(depth, 4));
                QCOMPARE(make_pair(depth, !!subNode->front()),
                         make_pair(depth, depth < 5));
                QCOMPARE(make_pair(depth, !!subNode->back()),
                         make_pair(depth, false));
            }
        }

        QCOMPARE(node.allPolygons().count(), 6);

        const auto plane = node.plane();

        QVERIFY(!plane.isNull());
        QCOMPARE(plane.normal(), expectedNormal);
        QCOMPARE(plane.w(), -1);
    }

    void testSplitWithAllInFront()
    {
        // Vertical YZ plane through the origin
        const auto plane = Plane::fromPoints({0, 0, 0}, {0, 1, 0}, {0, 0, 1});

        // Polygon in the +x hemisphere
        const auto poly = Polygon{{
                                   Vertex{{1, 0, 0}, {1, 0, 0}},
                                   Vertex{{1, 1, 0}, {1, 0, 0}},
                                   Vertex{{1, 0, 1}, {1, 0, 0}},
                                   }};

        auto cpf = QList<Polygon>{};
        auto cpb = QList<Polygon>{};
        auto front = QList<Polygon>{};
        auto back = QList<Polygon>{};

        poly.split(plane, &cpf, &cpb, &front, &back);

        QCOMPARE(cpf.length(), 0);
        QCOMPARE(cpb.length(), 0);
        QCOMPARE(front.length(), 1);
        QCOMPARE(back.length(), 0);
    }

    void testSplitWithAllBehind()
    {
        // Vertical YZ plane through the origin
        const auto plane = Plane::fromPoints({0, 0, 0}, {0, 1, 0}, {0, 0, 1});

        // Polygon in the -x hemisphere
        const auto poly = Polygon{{
                                   Vertex{{-1, 0, 0}, {1, 0, 0}},
                                   Vertex{{-1, 1, 0}, {1, 0, 0}},
                                   Vertex{{-1, 0, 1}, {1, 0, 0}},
                                   }};

        auto cpf = QList<Polygon>{};
        auto cpb = QList<Polygon>{};
        auto front = QList<Polygon>{};
        auto back = QList<Polygon>{};

        poly.split(plane, &cpf, &cpb, &front, &back);

        QCOMPARE(cpf.length(), 0);
        QCOMPARE(cpb.length(), 0);
        QCOMPARE(front.length(), 0);
        QCOMPARE(back.length(), 1);
    }

    void testSplitDownTheMiddle()
    {
        // Vertical YZ plane through the origin
        const auto plane = Plane::fromPoints({0, 0, 0}, {0, 1, 0}, {0, 0, 1});

        // Polygon describing a square on the XY plane with radius 2
        const auto poly = Polygon{{
                                   Vertex{{-1, +1, 0}, {0, 0, 1}},
                                   Vertex{{-1, -1, 0}, {0, 0, 1}},
                                   Vertex{{+1, -1, 0}, {0, 0, 1}},
                                   Vertex{{+1, +1, 0}, {0, 0, 1}},
                                   }};

        auto cpf = QList<Polygon>{};
        auto cpb = QList<Polygon>{};
        auto front = QList<Polygon>{};
        auto back = QList<Polygon>{};

        poly.split(plane, &cpf, &cpb, &front, &back);

        QCOMPARE(cpf.length(), 0);
        QCOMPARE(cpb.length(), 0);
        QCOMPARE(front.length(), 1);
        QCOMPARE(back.length(), 1);

        for (const auto &v: front.constFirst().vertices())
            QVERIFY2(v.position().x() >= 0, "All front vertices must have x >= 0");
        for (const auto &v: back.constFirst().vertices())
            QVERIFY2(v.position().x() <= 0, "All back vertices must have x <= 0");
    }
};

} // namespace QtCSG::Tests

QTEST_MAIN(QtCSG::Tests::Test)

#include "qtcsgtest.moc"
