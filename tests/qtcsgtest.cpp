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
#include "qtcsgtest.h"

#include <qtcsg/qtcsg.h>
#include <qtcsg/qtcsgmath.h>

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
        const auto maybeNode = Node::fromPolygons(cube().polygons());

        if (std::holds_alternative<Error>(maybeNode))
            QCOMPARE(std::get<Error>(maybeNode), Error::NoError);

        QVERIFY(std::holds_alternative<Node>(maybeNode));
        const auto node = std::get<Node>(maybeNode);

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
        const auto maybeNode = Node::fromPolygons(cube().polygons());

        if (std::holds_alternative<Error>(maybeNode))
            QCOMPARE(std::get<Error>(maybeNode), Error::NoError);

        QVERIFY(std::holds_alternative<Node>(maybeNode));
        const auto node = std::get<Node>(maybeNode);

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

    void testVertexTransform_data()
    {
        QTest::addColumn<Vertex>    ("vertex");
        QTest::addColumn<QMatrix4x4>("matrix");
        QTest::addColumn<Vertex>    ("expectedResult");
        QTest::addColumn<float>     ("expectedLength");

        const auto ra = +2.577350f;
        const auto rb = +0.845299f;
        const auto na = +0.333333f;
        const auto nb = +0.910684f;
        const auto nc = -0.244017f;

        const auto v0   = Vertex{{+1, +2, +3}, {+1, +0, +0}};
        const auto sx   = Vertex{{+2, +2, +3}, {+1, +0, +0}};
        const auto sy   = Vertex{{+1, +4, +3}, {+1, +0, +0}};
        const auto sz   = Vertex{{+1, +2, +6}, {+1, +0, +0}};
        const auto sxyz = Vertex{{+2, +4, +6}, {+1, +0, +0}};
        const auto tx   = Vertex{{+2, +2, +3}, {+1, +0, +0}};
        const auto ty   = Vertex{{+1, +3, +3}, {+1, +0, +0}};
        const auto tz   = Vertex{{+1, +2, +4}, {+1, +0, +0}};
        const auto txyz = Vertex{{+2, +3, +4}, {+1, +0, +0}};
        const auto rx   = Vertex{{+1, -3, +2}, {+1, +0, +0}};
        const auto ry   = Vertex{{+3, +2, -1}, {+0, +0, -1}};
        const auto rz   = Vertex{{-2, +1, +3}, {+0, +1, +0}};
        const auto rxyz = Vertex{{ra, rb, ra}, {na, nb, nc}};

        QTest::newRow("identity")       << v0 <<     identity()          << v0   << 14.0f;
        QTest::newRow("scaled-x")       << v0 <<        scale({2, 1, 1}) << sx   << 17.0f;
        QTest::newRow("scaled-y")       << v0 <<        scale({1, 2, 1}) << sy   << 26.0f;
        QTest::newRow("scaled-z")       << v0 <<        scale({1, 1, 2}) << sz   << 41.0f;
        QTest::newRow("scaled-xyz")     << v0 <<        scale({2, 2, 2}) << sxyz << 56.0f;
        QTest::newRow("translated-x")   << v0 <<  translation({1, 0, 0}) << tx   << 17.0f;
        QTest::newRow("translated-y")   << v0 <<  translation({0, 1, 0}) << ty   << 19.0f;
        QTest::newRow("translated-z")   << v0 <<  translation({0, 0, 1}) << tz   << 21.0f;
        QTest::newRow("translated-xyz") << v0 <<  translation({1, 1, 1}) << txyz << 29.0f;
        QTest::newRow("rotated-x")      << v0 << rotation(90, {1, 0, 0}) << rx   << 14.0f;
        QTest::newRow("rotated-y")      << v0 << rotation(90, {0, 1, 0}) << ry   << 14.0f;
        QTest::newRow("rotated-z")      << v0 << rotation(90, {0, 0, 1}) << rz   << 14.0f;
        QTest::newRow("rotated-xyz")    << v0 << rotation(90, {1, 1, 1}) << rxyz << 14.0f;
    }

    void testVertexTransform()
    {
        const QFETCH(Vertex,     vertex);
        const QFETCH(QMatrix4x4, matrix);
        const QFETCH(Vertex,     expectedResult);
        const QFETCH(float,      expectedLength);

        const auto transformed = vertex.transformed(matrix);

        QCOMPARE(transformed.position().lengthSquared(), expectedLength);
        QCOMPARE(transformed.normal().lengthSquared(),   1.0f);

        QCOMPARE(transformed.position(), expectedResult.position());
        QCOMPARE(transformed.normal(),   expectedResult.normal());
        QCOMPARE(transformed,            expectedResult);
    }

    void testParseGeometry_data()
    {
        QTest::addColumn<QString>("expression");
        QTest::addColumn<Geometry>("expectedGeometry");
        QTest::addColumn<QString>("expectedWarning");

        QTest::newRow("cube:default")
            << "cube()"
            << cube()
            << "";
        QTest::newRow("cube:center")
            << "cube(center=[0.5,1,2.])"
            << cube({0.5, 1, 2.0})
            << "";
        QTest::newRow("cube:radius:scalar")
            << "cube(r=3.1)"
            << cube({}, 3.1)
            << "";
        QTest::newRow("cube:radius:vector")
            << "cube(r=[1,2.2,3.5])"
            << cube({}, {1, 2.2, 3.5})
            << "";
        QTest::newRow("cube:center+radius")
            << "cube(r=5, center=[-1,+2,-3.0])"
            << cube({-1, 2, -3}, 5)
            << "";

        QTest::newRow("cylinder:default")
            << "cylinder()"
            << cylinder()
            << "";
        QTest::newRow("cylinder:start")
            << "cylinder(start=[0,0,1])"
            << cylinder({0, 0, 1}, {0, 0, 0})
            << "";
        QTest::newRow("cylinder:end+r")
            << "cylinder(end=[0,0,-1], r=2)"
            << cylinder({0, 0, 0}, {0, 0, -1}, 2)
            << "";
        QTest::newRow("cylinder:start+end+r+slices")
            << "cylinder(start=[1,1,1], end=[-1,-1,-1], r=1.5, slices=5)"
            << cylinder({1, 1, 1}, {-1, -1, -1}, 1.5f, 5)
            << "";
        QTest::newRow("cylinder:center")
            << "cylinder(center=[1,2,3])"
            << cylinder({1, 2, 3})
            << "";
        QTest::newRow("cylinder:center+r")
            << "cylinder(center=[2,3,4], r=2)"
            << cylinder({2, 3, 4}, 2, 2)
            << "";
        QTest::newRow("cylinder:center+h+r+slices")
            << "cylinder( center=[ 3, 4, 5 ], h = 6 , r = 7, slices=8 )"
            << cylinder({3, 4, 5}, 6, 7, 8)
            << "";

        QTest::newRow("sphere:default")
            << "sphere()"
            << sphere()
            << "";
        QTest::newRow("sphere:center+radius+slices+stacks")
            << "sphere(center=[1,2,3], r=4, slices=5, stacks=6)"
            << sphere({1, 2, 3}, 4, 5, 6)
            << "";

        QTest::newRow("error:filename")
            << "/home/you/are/pretty.off"
            << Geometry{Error::NotSupportedError}
            << "";
        QTest::newRow("error:unknown-primitive")
            << "unknown()"
            << Geometry{Error::NotSupportedError}
            << R"*(Unsupported primitive: "unknown")*";
        QTest::newRow("error:malformed-argument-list")
            << "cube(bad)"
            << Geometry{Error::FileFormatError}
            << R"*(Invalid argument list: "(bad)")*";
        QTest::newRow("error:unknown-argument")
            << "cube(unknown=23)"
            << Geometry{Error::FileFormatError}
            << R"*(Unsupported argument "unknown" for cube primitive)*";
        QTest::newRow("error:invalid-type")
            << "cube(center=42)"
            << Geometry{Error::FileFormatError}
            << R"*(Unsupported value type for argument "center" of cube primitive)*";
        QTest::newRow("error:conflicting-arguments")
            << "cylinder(start=[1,1,1], center=[0,0,0])"
            << Geometry{Error::FileFormatError}
            << R"*(Argument "center" conflicts with arguments )*"
               R"*("start" and "end" of cylinder primitive)*";
    }

    void testParseGeometry()
    {
        const QFETCH(QString, expression);
        const QFETCH(Geometry, expectedGeometry);
        const QFETCH(QString, expectedWarning);

        if (!expectedWarning.isEmpty())
            QTest::ignoreMessage(QtWarningMsg, qUtf8Printable(expectedWarning));

        const auto parsedGeometry = parseGeometry(expression);

        QCOMPARE(expectedGeometry.error(), expectedGeometry.error());
        QCOMPARE(parsedGeometry.polygons(), expectedGeometry.polygons());
    }
};

} // namespace QtCSG::Tests

QTEST_MAIN(QtCSG::Tests::Test)

#include "qtcsgtest.moc"
