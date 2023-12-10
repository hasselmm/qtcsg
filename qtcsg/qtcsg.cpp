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
#include "qtcsg.h"
#include "qtcsgmath.h"
#include "qtcsgutils.h"

#include <QLoggingCategory>

#include <cmath>

namespace QtCSG {

namespace {

Q_LOGGING_CATEGORY(lcNode, "qtcsg.node");
Q_LOGGING_CATEGORY(lcOperator, "qtcsg.operator");

using Utils::reportError;

template<class T>
void flip(T &o)
{
    o.flip();
}

} // namespace

void Vertex::flip()
{
    m_normal = -m_normal;
}

Vertex Vertex::transformed(const QMatrix4x4 &matrix) const
{
    auto newPosition = matrix * position();
    auto newNormal = findRotation(matrix) * normal(); // only rotate; do not translate, or scale
    return Vertex{std::move(newPosition), std::move(newNormal)};
}

Vertex Vertex::interpolated(Vertex other, float t) const
{
    return Vertex{
        lerp(position(), other.position(), t),
        lerp(normal(), other.normal(), t),
    };
}

Plane Plane::fromPoints(QVector3D a, QVector3D b, QVector3D c)
{
    const auto n = QVector3D::crossProduct(b - a, c - a).normalized();
    return Plane{n, QVector3D::dotProduct(n, a)};
}

void Plane::flip()
{
    m_normal = -m_normal;
    m_w = -m_w;
}

void Polygon::flip()
{
    std::reverse(m_vertices.begin(), m_vertices.end());
    std::for_each(m_vertices.begin(), m_vertices.end(), &QtCSG::flip<Vertex>);
    m_plane.flip();
}

Polygon Polygon::transformed(const QMatrix4x4 &matrix) const
{
    auto transformed = QList<Vertex>{};
    transformed.reserve(m_vertices.count());

    const auto applyMatrix = [matrix](const Vertex &vertex) {
        return vertex.transformed(matrix);
    };

    std::transform(m_vertices.cbegin(), m_vertices.cend(),
                   std::back_inserter(transformed), applyMatrix);

    return Polygon{std::move(transformed)};
}

void Polygon::split(const Plane &plane,
                    QList<Polygon> *coplanarFront, QList<Polygon> *coplanarBack,
                    QList<Polygon> *front, QList<Polygon> *back, float epsilon) const
{
    enum VertexType {
        Coplanar = 0,
        Front = (1 << 0),
        Back = (1 << 1),
        Spanning = Front | Back
    };

    // Classify each point as well as the entire polygon into one of the above four classes.
    auto polygonType = Coplanar;
    auto vertexTypes = std::vector<VertexType>{};
    vertexTypes.reserve(m_vertices.size());

    for (const auto &v: m_vertices) {
        const auto t = QVector3D::dotProduct(plane.normal(), v.position()) - plane.w();
        const auto type = (t < -epsilon) ? Back : (t > epsilon) ? Front : Coplanar;
        polygonType = static_cast<VertexType>(polygonType | type);
        vertexTypes.emplace_back(type);
    }

    // Put the polygon in the correct list, splitting it when necessary.
    switch (polygonType) {
    case Coplanar:
        if (QVector3D::dotProduct(plane.normal(), m_plane.normal()) > 0)
            coplanarFront->append(*this);
        else
            coplanarBack->append(*this);

        break;

    case Front:
        front->append(*this);
        break;

    case Back:
        back->append(*this);
        break;

    case Spanning:
        auto f = QList<Vertex>{};
        auto b = QList<Vertex>{};

        for (auto i = 0; i < m_vertices.count(); ++i) {
              const auto j = (i + 1) % m_vertices.count();

              const auto ti = vertexTypes[i];
              const auto tj = vertexTypes[j];
              const auto vi = m_vertices[i];
              const auto vj = m_vertices[j];

              if (ti != Back)
                  f.append(vi);
              if (ti != Front)
                  b.append(vi);

              if ((ti | tj) == Spanning) {
                  const auto t = (plane.w()
                                  - QVector3D::dotProduct(plane.normal(), vi.position()))
                                  / QVector3D::dotProduct(plane.normal(), vj.position() - vi.position());
                  const auto v = vi.interpolated(vj, t);

                  f.append(v);
                  b.append(v);
              }
        }

        if (f.count() >= 3)
            front->append(Polygon{std::move(f), m_shared});
        if (b.count() >= 3)
            back->append(Polygon{std::move(b), m_shared});

        break;
    }
}

Geometry Geometry::inversed() const
{
    auto inverse = QList<Polygon>{};
    inverse.reserve(m_polygons.size());
    std::copy(m_polygons.begin(), m_polygons.end(), std::back_inserter(inverse));
    std::for_each(inverse.begin(), inverse.end(), &flip<Polygon>);
    return Geometry{std::move(inverse)};
}

Geometry Geometry::transformed(const QMatrix4x4 &matrix) const
{
    auto transformed = QList<Polygon>{};
    transformed.reserve(m_polygons.count());

    const auto applyMatrix = [matrix](const Polygon &polygon) {
        return polygon.transformed(matrix);
    };

    std::transform(m_polygons.cbegin(), m_polygons.cend(),
                   std::back_inserter(transformed), applyMatrix);

    return Geometry{std::move(transformed)};
}

Geometry cube(QVector3D center, QVector3D size)
{
    const auto makePolygon = [center, size](std::array<int, 4> indices, QVector3D normal) {
        QList<Vertex> vertices;

        vertices.reserve(indices.size());
        std::transform(indices.begin(), indices.end(), std::back_inserter(vertices), [=](int i) {
            const auto directions = QVector3D{
                i & 1 ? +1.0f : -1.0f,
                i & 2 ? +1.0f : -1.0f,
                i & 4 ? +1.0f : -1.0f,
            };

            return Vertex{center + size * directions, normal};
        });

        return Polygon{vertices};
    };

    return Geometry{{
        makePolygon({0, 4, 6, 2}, {-1, 0, 0}),
        makePolygon({1, 3, 7, 5}, {+1, 0, 0}),
        makePolygon({0, 1, 5, 4}, {0, -1, 0}),
        makePolygon({2, 6, 7, 3}, {0, +1, 0}),
        makePolygon({0, 2, 3, 1}, {0, 0, -1}),
        makePolygon({4, 5, 7, 6}, {0, 0, +1}),
    }};
}

Geometry cube(QVector3D center, float size)
{
    return cube(std::move(center), {size, size, size});
}

Geometry sphere(QVector3D center, float radius, int slices, int stacks)
{
    auto polygons = QList<Polygon>{};

    const auto vertex = [center, radius, slices, stacks](int i, int j) {
        const auto theta = 2 * M_PI * i / slices;
        const auto phi = M_PI * j / stacks;

        const auto normal = QVector3D{cosf(theta) * sinf(phi), cosf(phi), sinf(theta) * sinf(phi)};
        return Vertex{center + normal * radius, normal};
    };

    for (auto i = 0; i < slices; ++i) {
        for (auto j = 0; j < stacks; ++j) {
            auto vertices = QList<Vertex>{};
            vertices.append(vertex(i, j));

            if (j > 0)
                vertices.append(vertex(i + 1, j));
            if (j < stacks - 1)
                vertices.append(vertex(i + 1, j + 1));

            vertices.append(vertex(i, j + 1));
            polygons.append(Polygon{std::move(vertices)});
        }
    }

    return Geometry{std::move(polygons)};
}

Geometry cylinder(QVector3D start, QVector3D end, float radius, float slices)
{
    auto polygons = QList<Polygon>{};

    const auto ray = end - start;
    const auto axisZ = ray.normalized();
    const auto isY = abs(axisZ.y()) > 0.5;
    const auto axisX = QVector3D::crossProduct({isY ? 1.0f : 0, isY ? 0 : 1.0f, 0}, axisZ).normalized();
    const auto axisY = QVector3D::crossProduct(axisX, axisZ).normalized();
    const auto vertexStart = Vertex{start, -axisZ};
    const auto vertexEnd = Vertex{end, axisZ};

    const auto point = [=](int stack, int slice, int normalBlend) {
        const auto phi = 2 * M_PI * slice / slices;
        const auto out = (axisX * cosf(phi)) + (axisY * sinf(phi));
        auto pos = start + (ray * stack) + (out * radius);
        auto normal = out * (1 - abs(normalBlend)) + (axisZ * normalBlend);
        return Vertex{std::move(pos), std::move(normal)};
    };

    for (auto i = 0; i < slices; ++i) {
        polygons += Polygon{{vertexStart, point(0, i, -1), point(0, i + 1, -1)}};
        polygons += Polygon{{point(0, i + 1, 0), point(0, i, 0), point(1, i, 0), point(1, i + 1, 0)}};
        polygons += Polygon{{vertexEnd, point(1, i + 1, 1), point(1, i, 1)}};
    }

    return Geometry{std::move(polygons)};
}

Geometry cylinder(QVector3D center, float height, float radius, float slices)
{
    return cylinder(center - QVector3D{0, height/2, 0},
                    center + QVector3D{0, height/2, 0},
                    radius, slices);
}

Geometry merge(Geometry lhs, Geometry rhs, int limit)
{
    if (reportError(lcOperator(), lhs.error(), "Invalid lhs geometry"))
        return Geometry{lhs.error()};
    if (reportError(lcOperator(), rhs.error(), "Invalid rhs geometry"))
        return Geometry{lhs.error()};

    auto a = Node{};
    auto b = Node{};

    if (const auto error = a.build(lhs.polygons(), limit);
        reportError(lcOperator(), error, "Could not build BSP tree from lhs geometry"))
        return Geometry{error};
    if (const auto error = b.build(rhs.polygons(), limit);
        reportError(lcOperator(), error, "Could not build BSP tree from rhs geometry"))
        return Geometry{error};

    a.clipTo(b);
    b.clipTo(a);
    b.invert();
    b.clipTo(a);
    b.invert();

    if (const auto error = a.build(b.allPolygons(), limit);
        reportError(lcOperator(), error, "Could not build BSP tree from transformed tree"))
        return Geometry{error};

    return Geometry{a.allPolygons()};
}

Geometry subtract(Geometry lhs, Geometry rhs, int limit)
{
    if (reportError(lcOperator(), lhs.error(), "Invalid lhs geometry"))
        return Geometry{lhs.error()};
    if (reportError(lcOperator(), rhs.error(), "Invalid rhs geometry"))
        return Geometry{lhs.error()};

    auto a = Node{};
    auto b = Node{};

    if (const auto error = a.build(lhs.polygons(), limit);
        reportError(lcOperator(), error, "Could not build BSP tree from lhs geometry"))
        return Geometry{error};
    if (const auto error = b.build(rhs.polygons(), limit);
        reportError(lcOperator(), error, "Could not build BSP tree from rhs geometry"))
        return Geometry{error};

    a.invert();
    a.clipTo(b);
    b.clipTo(a);
    b.invert();
    b.clipTo(a);
    b.invert();

    if (const auto error = a.build(b.allPolygons(), limit);
        reportError(lcOperator(), error, "Could not build BSP tree from transformed tree"))
        return Geometry{error};

    a.invert();

    return Geometry{a.allPolygons()};
}

Geometry intersect(Geometry lhs, Geometry rhs, int limit)
{
    if (reportError(lcOperator(), lhs.error(), "Invalid lhs geometry"))
        return Geometry{lhs.error()};
    if (reportError(lcOperator(), rhs.error(), "Invalid rhs geometry"))
        return Geometry{lhs.error()};

    auto a = Node{};
    auto b = Node{};

    if (const auto error = a.build(lhs.polygons(), limit);
        reportError(lcOperator(), error, "Could not build BSP tree from lhs geometry"))
        return Geometry{error};
    if (const auto error = b.build(rhs.polygons(), limit);
        reportError(lcOperator(), error, "Could not build BSP tree from rhs geometry"))
        return Geometry{error};

    a.invert();
    b.clipTo(a);
    b.invert();
    a.clipTo(b);
    b.clipTo(a);

    if (const auto error = a.build(b.allPolygons(), limit);
        reportError(lcOperator(), error, "Could not build BSP tree from transformed tree"))
        return Geometry{error};

    a.invert();

    return Geometry{a.allPolygons()};
}

std::variant<Node, Error> Node::fromPolygons(QList<Polygon> polygons, int limit)
{
    auto node = Node{};

    if (const auto error = node.build(std::move(polygons), limit);
        reportError(lcNode(), error, "Could not build BSP tree from polygons"))
        return {error};

    return {std::move(node)};
}

void Node::invert()
{
    std::for_each(m_polygons.begin(), m_polygons.end(), &flip<Polygon>);

    m_plane.flip();

    if (m_front)
        m_front->invert();
    if (m_back)
        m_back->invert();

    std::swap(m_front, m_back);
}

Node Node::inverted() const
{
    auto node = *this;
    node.invert();
    return node;
}

QList<Polygon> Node::clipPolygons(QList<Polygon> polygons) const
{
    if (m_plane.isNull())
        return polygons;

    auto front = QList<Polygon>{};
    auto back = QList<Polygon>{};

    for (const auto &p: polygons)
        p.split(m_plane, &front, &back, &front, &back);

    if (m_front)
        front = m_front->clipPolygons(front);

    if (m_back)
        back = m_back->clipPolygons(back);
    else
        back.clear();

    return front + back;
}

void Node::clipTo(const Node &bsp)
{
    m_polygons = bsp.clipPolygons(std::move(m_polygons));

    if (m_front)
        m_front->clipTo(bsp);
    if (m_back)
        m_back->clipTo(bsp);
}

QList<Polygon> Node::allPolygons() const
{
    auto polygons = m_polygons;

    if (m_front)
        polygons += m_front->allPolygons();
    if (m_back)
        polygons += m_back->allPolygons();

    return polygons;
}

Error Node::build(QList<Polygon> polygons, int level, int limit)
{
    if (level == limit) {
        qWarning(lcNode, "Maximum recursion level reached");
        return Error::RecursionError;
    }

    if (polygons.isEmpty())
        return Error::NoError;

    if (m_plane.isNull())
        m_plane = polygons.first().plane();

    auto result = Error::NoError;
    auto front = QList<Polygon>{};
    auto back = QList<Polygon>{};

    for (const auto &p: polygons)
        p.split(m_plane, &m_polygons, &m_polygons, &front, &back);

    if (!front.empty()) {
        if (!m_front)
            m_front = std::make_shared<Node>();

        if (const auto error = m_front->build(std::move(front), level + 1, limit);
            error != Error::NoError && result == Error::NoError)
            result = error;
    }

    if (!back.empty()) {
        if (!m_back)
            m_back = std::make_shared<Node>();

        if (const auto error = m_back->build(std::move(back), level + 1, limit);
            error != Error::NoError && result == Error::NoError)
            result = error;
    }

    return result;
}


QDebug operator<<(QDebug debug, Geometry geometry)
{
    const auto stateGuard = QDebugStateSaver{debug};

    return debug.nospace()
            << "Geometry(polygons="
            << geometry.polygons()
            << ")";
}

QDebug operator<<(QDebug debug, Plane plane)
{
    const auto stateGuard = QDebugStateSaver{debug};

    return debug.nospace()
            << "Plane(normal="
            << plane.normal()
            << ", w="
            << plane.w()
            << ")";
}

QDebug operator<<(QDebug debug, Polygon polygon)
{
    const auto stateGuard = QDebugStateSaver{debug};

    return debug.nospace()
            << "Polygon(vertices="
            << polygon.vertices()
            << ", plane="
            << polygon.plane()
            << ")";
}

QDebug operator<<(QDebug debug, Vertex vertex)
{
    const auto stateGuard = QDebugStateSaver{debug};

    return debug.nospace()
            << "Vertex(position="
            << vertex.position()
            << ", normal="
            << vertex.normal()
            << ")";
}

} // namespace QtCSG
