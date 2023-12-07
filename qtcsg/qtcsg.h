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
#ifndef QTCSG_H
#define QTCSG_H

#include <QVariant>
#include <QVector3D>

#include <memory>

namespace Qt3DCSG {
class Geometry;
}

namespace QtCSG {

Q_NAMESPACE

constexpr auto defaultRecursionLimit() { return 1024; }

enum class Error
{
    NoError,
    RecursionError,
    NotSupportedError,
    FileSystemError,
    FileFormatError,
};

Q_ENUM_NS(Error)

/// Represents a vertex of a polygon. Use your own vertex class instead of this
/// one to provide additional features like texture coordinates and vertex
/// colors. Custom vertex classes need to provide a `pos` property and `clone()`,
/// `flip()`, and `interpolate()` methods that behave analogous to the ones
/// defined by `CSG.Vertex`. This class provides `normal` so convenience
/// functions like `CSG.sphere()` can return a smooth vertex normal, but `normal`
/// is not used anywhere else.
class Vertex
{
public:
    Vertex() = default;

    explicit Vertex(QVector3D position, QVector3D normal)
        : m_position{std::move(position)}
        , m_normal{std::move(normal)}
    {}

    [[nodiscard]] auto position() const { return m_position; }
    [[nodiscard]] auto normal() const { return m_normal; }

    /// Invert all orientation-specific data (e.g. vertex normal).
    /// Called when the orientation of a polygon is flipped.
    void flip();

    /// Create a new vertex between this vertex and `other` by linearly
    /// interpolating all properties using a parameter of `t`. Subclasses should
    /// override this to interpolate additional properties.
    [[nodiscard]] Vertex interpolated(Vertex other, float t) const;

    /// Returns a new vertex which has the transformations described
    /// by `matrix` applied to the position and normal this vertex.
    [[nodiscard]] Vertex transformed(const QMatrix4x4 &matrix) const;

    [[nodiscard]] auto fields() const { return std::tie(m_position, m_normal); }
    [[nodiscard]] bool operator==(const Vertex &rhs) const { return fields() == rhs.fields(); }

private:
    friend class Qt3DCSG::Geometry; // FIXME: Build a vertex type that's simple but also directly wraps Qt3D attributes

    QVector3D m_position;
    QVector3D m_normal;
};

///  Represents a plane in 3D space.
class Plane
{
public:
    Plane() = default;

    explicit Plane(QVector3D normal, float w)
        : m_normal{std::move(normal)}
        , m_w{w}
    {}

    [[nodiscard]] static Plane fromPoints(QVector3D a, QVector3D b, QVector3D c);

    [[nodiscard]] auto isNull() const { return m_normal.isNull(); }
    [[nodiscard]] auto normal() const { return m_normal; }
    [[nodiscard]] auto w() const { return m_w; }

    void flip();

    [[nodiscard]] auto fields() const { return std::tie(m_normal, m_w); }
    [[nodiscard]] bool operator==(const Plane &rhs) const { return fields() == rhs.fields(); }

private:
    QVector3D m_normal;
    float m_w;
};

/// Represents a convex polygon.
/// The vertices used to initialize a polygon must
/// be coplanar and form a convex loop.
///
/// Each convex polygon has a `shared` property, which is shared between all
/// polygons that are clones of each other or were split from the same polygon.
/// This can be used to define per-polygon properties (such as surface color).
class Polygon
{
public:
    Polygon() = default;

    explicit Polygon(QList<Vertex> vertices, QVariant shared = {})
        : m_vertices{std::move(vertices)}
        , m_shared{std::move(shared)}
        , m_plane{Plane::fromPoints(m_vertices[0].position(),
                                    m_vertices[1].position(),
                                    m_vertices[2].position())}
    {}

    [[nodiscard]] auto vertices() const { return m_vertices; }
    [[nodiscard]] auto shared() const { return m_shared; }
    [[nodiscard]] auto plane() const { return m_plane; }

    void flip();

    /// Split this polygon by `plane` if needed, then put the polygon or polygon
    /// fragments in the appropriate lists. Coplanar polygons go into either
    /// `coplanarFront` or `coplanarBack` depending on their orientation with
    /// respect to this plane. Polygons in front or in back of this plane go into
    /// either `front` or `back`.
    void split(const Plane &plane,
               QList<Polygon> *coplanarFront, QList<Polygon> *coplanarBack,
               QList<Polygon> *front, QList<Polygon> *back,
               float epsilon = 1e-5) const;

    /// Returns a new polygon which has the transformations described
    /// by `matrix` applied to all vertices of this polygon.
    [[nodiscard]] Polygon transformed(const QMatrix4x4 &matrix) const;

    [[nodiscard]] auto fields() const { return std::tie(m_vertices, m_shared, m_plane); }
    [[nodiscard]] bool operator==(const Polygon &rhs) const { return fields() == rhs.fields(); }

private:
    QList<Vertex> m_vertices;
    QVariant m_shared;
    Plane m_plane;
};

/// Holds a binary space partition tree representing a 3D solid. Two solids can
/// be combined using the `unite()`, `subtract()`, and `intersect()` methods.
class Geometry
{
public:
    explicit Geometry(Error error = Error::NoError)
        : m_error{error} {}
    explicit Geometry(QList<Polygon> polygons, Error error = Error::NoError)
        : m_polygons{std::move(polygons)}
        , m_error{error} {}

    [[nodiscard]] auto isEmpty() const { return m_polygons.isEmpty(); }
    [[nodiscard]] auto polygons() const { return m_polygons; }
    [[nodiscard]] Error error() const { return m_error; }

    /// Return a new CSG solid with solid and empty space switched.
    [[nodiscard]] Geometry inversed() const;

    /// Returns a new geometry which has the transformations described
    /// by `matrix` applied to all the polygons of this geometry.
    [[nodiscard]] Geometry transformed(const QMatrix4x4 &matrix) const;

private:
    QList<Polygon> m_polygons;
    Error m_error;
};

/// Holds a node in a BSP tree. A BSP tree is built from a collection of polygons
/// by picking a polygon to split along. That polygon (and all other coplanar
/// polygons) are added directly to that node and the other polygons are added to
/// the front and/or back subtrees. This is not a leafy BSP tree since there is
/// no distinction between internal and leaf nodes.
class Node
{
public:
    Node() = default;

    static std::variant<Node, Error> fromPolygons(QList<Polygon> polygons,
                                                  int limit = defaultRecursionLimit());

    [[nodiscard]] auto plane() const { return m_plane; }
    [[nodiscard]] auto polygons() const { return m_polygons; }
    [[nodiscard]] auto front() const { return m_front; }
    [[nodiscard]] auto back() const { return m_back; }

    /// Convert solid space to empty space and empty space to solid space.
    void invert();
    [[nodiscard]] Node inverted() const;

    /// Recursively remove all polygons in `polygons` that are inside this BSP tree.
    [[nodiscard]] QList<Polygon> clipPolygons(QList<Polygon> polygons) const;

    /// Remove all polygons in this BSP tree that are inside the other BSP tree `bsp`.
    void clipTo(const Node &bsp);

    /// Return a list of all polygons in this BSP tree.
    [[nodiscard]] QList<Polygon> allPolygons() const;

    /// Build a BSP tree out of `polygons`. When called on an existing tree, the
    /// new polygons are filtered down to the bottom of the tree and become new
    /// nodes there. Each set of polygons is partitioned using the first polygon
    /// (no heuristic is used to pick a good split).
    Error build(QList<Polygon> polygons, int limit = defaultRecursionLimit())
    {
        return build(std::move(polygons), 0, limit);
    }

private:
    [[nodiscard]] Error build(QList<Polygon> polygons, int level, int limit);

    Plane m_plane;

    QList<Polygon> m_polygons;

    std::shared_ptr<Node> m_front;
    std::shared_ptr<Node> m_back;
};

/// Construct an axis-aligned solid cuboid.
[[nodiscard]] Geometry cube(QVector3D center, QVector3D size);
[[nodiscard]] Geometry cube(QVector3D center = {}, float size = 1);

/// Construct a solid sphere.
/// The `slices` and `stacks` parameters control the tessellation along the
/// longitude and latitude directions.
[[nodiscard]] Geometry sphere(QVector3D center = {}, float radius = 1, int slices = 16, int stacks = 8);

/// Construct a solid cylinder.
/// The `slices` parameter controls the tessellation.
[[nodiscard]] Geometry cylinder(QVector3D start, QVector3D end, float radius = 1, float slices = 16);
[[nodiscard]] Geometry cylinder(QVector3D center = {}, float height = 2, float radius = 1, float slices = 16);

/// Return a new CSG solid representing space in either this solid or in the
/// solid `csg`. Neither this solid nor the solid `csg` are modified.
///
///     A.unite(B)
///
///     +-------+            +-------+
///     |       |            |       |
///     |   A   |            |       |
///     |    +--+----+   =   |       +----+
///     +----+--+    |       +----+       |
///          |   B   |            |       |
///          |       |            |       |
///          +-------+            +-------+
///
[[nodiscard]] Geometry merge(Geometry a, Geometry b, int limit = defaultRecursionLimit());

[[nodiscard]] inline auto unite(Geometry a, Geometry b) { return merge(std::move(a), std::move(b)); }
[[nodiscard]] inline auto operator|(Geometry a, Geometry b) { return merge(std::move(a), std::move(b)); }

/// Return a new CSG solid representing space in this solid but not in the
/// solid `csg`. Neither this solid nor the solid `csg` are modified.
///
///     A.subtract(B)
///
///     +-------+            +-------+
///     |       |            |       |
///     |   A   |            |       |
///     |    +--+----+   =   |    +--+
///     +----+--+    |       +----+
///          |   B   |
///          |       |
///          +-------+
///
[[nodiscard]] Geometry subtract(Geometry a, Geometry b, int limit = defaultRecursionLimit());

[[nodiscard]] inline auto difference(Geometry a, Geometry b) { return subtract(std::move(a), std::move(b)); }
[[nodiscard]] inline auto operator-(Geometry a, Geometry b) { return subtract(std::move(a), std::move(b)); }

/// Return a new CSG solid representing space both this solid and in the
/// solid `csg`. Neither this solid nor the solid `csg` are modified.
///
///     A.intersect(B)
///
///     +-------+
///     |       |
///     |   A   |
///     |    +--+----+   =   +--+
///     +----+--+    |       +--+
///          |   B   |
///          |       |
///          +-------+
///
[[nodiscard]] Geometry intersect(Geometry a, Geometry b, int limit = defaultRecursionLimit());

[[nodiscard]] inline auto intersection(Geometry a, Geometry b) { return intersect(std::move(a), std::move(b)); }
[[nodiscard]] inline auto operator&(Geometry a, Geometry b) { return intersect(std::move(a), std::move(b)); }

[[nodiscard]] inline Vertex operator*(const QMatrix4x4 &m, const Vertex &v) { return v.transformed(m); }
[[nodiscard]] inline Polygon operator*(const QMatrix4x4 &m, const Polygon &p) { return p.transformed(m); }
[[nodiscard]] inline Geometry operator*(const QMatrix4x4 &m, const Geometry &g) { return g.transformed(m); }

QDebug operator<<(QDebug debug, Geometry geometry);
QDebug operator<<(QDebug debug, Plane plane);
QDebug operator<<(QDebug debug, Polygon polygon);
QDebug operator<<(QDebug debug, Vertex vertex);

} // namespace QtCSG

Q_DECLARE_METATYPE(QtCSG::Geometry)
Q_DECLARE_METATYPE(QtCSG::Plane)
Q_DECLARE_METATYPE(QtCSG::Polygon)
Q_DECLARE_METATYPE(QtCSG::Vertex)

#endif // QTCSG_H
