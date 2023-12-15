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

#include <QRegularExpression>
#include <cmath>

namespace QtCSG {

namespace {

Q_LOGGING_CATEGORY(lcGeometry,  "qtcsg.geometry");
Q_LOGGING_CATEGORY(lcNode,      "qtcsg.node");
Q_LOGGING_CATEGORY(lcOperator,  "qtcsg.operator");

using Utils::reportError;

template<class T>
void flip(T &o)
{
    o.flip();
}

[[nodiscard]] constexpr bool isConvexPoint(const QVector3D &a,
                                           const QVector3D &b,
                                           const QVector3D &c,
                                           const QVector3D &normal,
                                           float epsilon = 0)
{
    const auto cross = QVector3D::crossProduct(b - a, c - b);
    const auto d = QVector3D::dotProduct(cross, normal);
    return d >= epsilon;
}

/// Split `polygon` by `plane` if needed, then put the polygon or polygon
/// fragments in the appropriate lists. Coplanar polygons go into either
/// `coplanarFront` or `coplanarBack` depending on their orientation with
/// respect to this plane. Polygons in front or in back of this plane go into
/// either `front` or `back`.
void split(const Polygon &polygon, const Plane &plane,
           QList<Polygon> *coplanarFront, QList<Polygon> *coplanarBack,
           QList<Polygon> *front, QList<Polygon> *back,
           const Options &options)
{
    enum VertexType {
        Coplanar = 0,
        Front = (1 << 0),
        Back = (1 << 1),
        Spanning = Front | Back
    };

    const auto vertices = polygon.vertices();
    const auto normal = polygon.plane().normal();
    const auto epsilon = options.epsilon;

    // Classify each point as well as the entire polygon into one of the above four classes.
    auto polygonType = Coplanar;
    auto vertexTypes = std::vector<VertexType>{};
    vertexTypes.reserve(vertices.size());

    for (const auto &v: vertices) {
        const auto t = dotProduct(plane.normal(), v.position()) - plane.w();
        const auto type = (t < -epsilon) ? Back : (t > epsilon) ? Front : Coplanar;
        polygonType = static_cast<VertexType>(polygonType | type);
        vertexTypes.emplace_back(type);
    }

    // Put the polygon in the correct list, splitting it when necessary.
    switch (polygonType) {
    case Coplanar:
        if (dotProduct(plane.normal(), normal) > 0)
            coplanarFront->append(polygon);
        else
            coplanarBack->append(polygon);

        break;

    case Front:
        front->append(polygon);
        break;

    case Back:
        back->append(polygon);
        break;

    case Spanning:
        auto f = QList<Vertex>{};
        auto b = QList<Vertex>{};

        for (auto i = 0; i < vertices.count(); ++i) {
            const auto j = (i + 1) % vertices.count();

            const auto ti = vertexTypes[i];
            const auto tj = vertexTypes[j];
            const auto vi = vertices[i];
            const auto vj = vertices[j];

            if (ti != Back)
                f.append(vi);
            if (ti != Front)
                b.append(vi);

            if ((ti | tj) == Spanning) {
                const auto t = (plane.w()
                                - dotProduct(plane.normal(), vi.position()))
                               / dotProduct(plane.normal(), vj.position() - vi.position());
                const auto v = vi.interpolated(vj, t);

                f.append(v);
                b.append(v);
            }
        }

        if (f.count() >= 3)
            front->append(Polygon{std::move(f), polygon.shared()});
        if (b.count() >= 3)
            back->append(Polygon{std::move(b), polygon.shared()});

        break;
    }
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
    const auto n = normalVector(a, b, c);
    return Plane{n, dotProduct(n, a)};
}

void Plane::flip()
{
    m_normal = -m_normal;
    m_w = -m_w;
}

bool Polygon::isConvex() const
{
    if (m_vertices.size() < 3)
        return true;

    const auto planeNormal = m_plane.normal();

    for (auto i = m_vertices.size() - 2, j = m_vertices.size() - 1, k = qsizetype{0};
         k < m_vertices.size(); i = j, j = k, ++k) {
        if (!isConvexPoint(m_vertices[i].position(),
                           m_vertices[j].position(),
                           m_vertices[k].position(),
                           planeNormal))
            return false;
    }

    return true;
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

void Geometry::validate(const Options &options)
{
    if (m_error != Error::NoError)
        return;

    if (options.flags & Options::CheckConvexity) {
        for (const auto &p: std::as_const(m_polygons)) {
            if (!p.isConvex()) {
                m_error = Error::ConvexityError;
                break;
            }
        }
    }
}

namespace {

struct EnsureValue
{
    const QVariantMap &arguments;

    template<typename T>
    T operator()(const QString &key, T defaultValue = {}) const
    {
        return (*this)(arguments.constFind(key), std::move(defaultValue));
    }

    template<typename T>
    T operator()(const QVariantMap::ConstIterator &iter, T defaultValue = {}) const
    {
        if (iter != arguments.constEnd())
            return qvariant_cast<T>(*iter);

        return defaultValue;
    }
};

Geometry createGeometry(QStringView primitiveName, QVariantMap arguments)
{
    const auto value = EnsureValue{arguments};

    if (primitiveName == u"cube") {
        const auto radius = arguments.value("r", 1.0f);

        if (radius.userType() == qMetaTypeId<QVector3D>()) {
            return cube(value("center", QVector3D{}),
                        qvariant_cast<QVector3D>(radius));
        }

        return cube(value("center", QVector3D{}), radius.toFloat());
    }

    if (primitiveName == u"cylinder") {
        const auto start = arguments.constFind("start");
        const auto end = arguments.constFind("end");

        if (start != arguments.constEnd() || end != arguments.constEnd()) {
            static const auto conflicts = std::array<QString, 2>{"center", "h"};

            for (const auto &conflictingName: conflicts) {
                if (arguments.contains(conflictingName)) {
                    qCWarning(lcGeometry,
                              R"(Argument "%ls" conflicts with arguments )"
                              R"("start" and "end" of %ls primitive)",
                              qUtf16Printable(conflictingName),
                              qUtf16Printable(primitiveName.toString()));

                    return Geometry{Error::FileFormatError};
                }
            }

            return cylinder(value(start,    QVector3D{}),
                            value(end,      QVector3D{}),
                            value("r",      1.0f),
                            value("slices", 16));
        } else {
            return cylinder(value("center", QVector3D{}),
                            value("h",      2.0f),
                            value("r",      1.0f),
                            value("slices", 16));
        }
    }

    if (primitiveName == u"sphere") {
        return sphere(value("center",   QVector3D{}),
                      value("r",        1.0f),
                      value("slices",   16),
                      value("stacks",   8));
    }

    qCCritical(lcGeometry, R"(Unsupported primitive type: "%ls")",
               qUtf16Printable(primitiveName.toString()));

    return Geometry{Error::FileFormatError};
}

using ArgumentTypeMap = QMap<QString, QList<int>>;

QVariant parseArgument(const QString &primitive,
                       const QString &argName,
                       const QRegularExpressionMatch &match,
                       const ArgumentTypeMap &argTypeMap)
{
    const auto valueSpec = argTypeMap.constFind(argName);

    if (valueSpec == argTypeMap.constEnd()) {
        qCWarning(lcGeometry, R"(Unsupported argument "%ls" for %ls primitive)",
                  qUtf16Printable(argName), qUtf16Printable(primitive));
        return {};
    }

    auto argValue = QVariant{};

    if (const auto scalar = match.captured(u"scalar"); !scalar.isEmpty()) {
        argValue = scalar.toFloat();
    } else if (const auto x = match.captured(u"vecx"); !x.isEmpty()) {
        if (const auto y = match.captured(u"vecy"); !x.isEmpty())
            if (const auto z = match.captured(u"vecz"); !x.isEmpty())
                argValue = QVector3D{x.toFloat(), y.toFloat(), z.toFloat()};
    }

    if (!valueSpec->contains(argValue.userType())) {
        qCWarning(lcGeometry, R"(Unsupported value type for argument "%ls" of %ls primitive)",
                  qUtf16Printable(argName), qUtf16Printable(primitive));
        return {};
    }

    return argValue;
}

} // namespace

Geometry parseGeometry(QString expression)
{
    static const auto s_callPattern = QRegularExpression{R"(^(?<name>[a-z]+)\((?<args>[^)]*\))$)"};
    static const auto s_argPattern  = QRegularExpression{R"(\s*(?<name>[a-z]+)\s*=\s*(?:)"
                                                         R"((?<scalar>[+-]?\d+(?:\.\d*)?)|\[)"
                                                         R"(\s*(?<vecx>[+-]?\d+(?:\.\d*)?)\s*,)"
                                                         R"(\s*(?<vecy>[+-]?\d+(?:\.\d*)?)\s*,)"
                                                         R"(\s*(?<vecz>[+-]?\d+(?:\.\d*)?)\s*)"
                                                         R"(\])\s*[,)])"};

    Q_ASSERT_X(s_callPattern.isValid(), "s_callPattern", qPrintable(s_callPattern.errorString()));
    Q_ASSERT_X(s_argPattern.isValid(), "s_argPattern", qPrintable(s_argPattern.errorString()));

    static constexpr auto scalarType = qMetaTypeId<float>();
    static constexpr auto vectorType = qMetaTypeId<QVector3D>();

    static const auto s_supportedArguments = QMap<QString, ArgumentTypeMap> {
        {"cube", {{"center", {vectorType}},
                  {"r",      {scalarType, vectorType}}}},

        {"cylinder", {{"start",  {vectorType}},
                      {"center", {vectorType}},
                      {"end",    {vectorType}},
                      {"h",      {scalarType}},
                      {"r",      {scalarType}},
                      {"slices", {scalarType}}}},

        {"sphere", {{"center", {vectorType}},
                    {"r",      {scalarType}},
                    {"slices", {scalarType}},
                    {"stacks", {scalarType}}}},
    };

    const auto parsedExpression = s_callPattern.match(expression);

    if (!parsedExpression.hasMatch())
        return Geometry{Error::FileFormatError};

    auto primitive = parsedExpression.captured(u"name");
    const auto argSpec = s_supportedArguments.constFind(primitive);

    if (argSpec == s_supportedArguments.constEnd()) {
        qCWarning(lcGeometry, R"(Unsupported primitive: "%ls")",
                  qUtf16Printable(primitive));

        return Geometry{Error::NotSupportedError};
    }

    const auto argList = parsedExpression.capturedView(u"args");
    auto arguments = QVariantMap{};

    if (argList != u")") {
        if (auto it = s_argPattern.globalMatch(argList); it.hasNext()) {
            auto expectedStart = 0;

            while (it.hasNext()) {
                const auto match = it.next();

                if (match.capturedStart() != expectedStart) {
                    const auto length = match.capturedStart() - expectedStart;
                    const auto expression = argList.mid(expectedStart, length);
                    qCWarning(lcGeometry, R"(Unexpected expression: "%ls")",
                              qUtf16Printable(expression.toString()));
                    return Geometry{Error::FileFormatError};
                }

                expectedStart = match.capturedEnd();
                auto argName = match.captured(u"name");

                if (arguments.contains(argName)) {
                    qCWarning(lcGeometry, R"(Duplicate argument "%ls")", qUtf16Printable(argName));
                    return Geometry{Error::FileFormatError};
                }

                auto argValue = parseArgument(primitive, argName, match, *argSpec);

                if (argValue.isNull())
                    return Geometry{Error::FileFormatError};

                arguments.insert(std::move(argName), std::move(argValue));
            }
        } else {
            qCWarning(lcGeometry, R"(Invalid argument list: "(%ls")",
                      qUtf16Printable(argList.toString()));
            return Geometry{Error::FileFormatError};
        }
    }

    return createGeometry(std::move(primitive), std::move(arguments));
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
    const auto axisX = crossProduct({isY ? 1.0f : 0, isY ? 0 : 1.0f, 0}, axisZ).normalized();
    const auto axisY = crossProduct(axisX, axisZ).normalized();
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

Geometry merge(Geometry lhs, Geometry rhs, Options options)
{
    if (reportError(lcOperator(), lhs.error(), "Invalid lhs geometry"))
        return Geometry{lhs.error()};
    if (reportError(lcOperator(), rhs.error(), "Invalid rhs geometry"))
        return Geometry{lhs.error()};

    auto a = Node{};
    auto b = Node{};

    if (const auto error = a.build(lhs.polygons(), options);
        reportError(lcOperator(), error, "Could not build BSP tree from lhs geometry"))
        return Geometry{error};
    if (const auto error = b.build(rhs.polygons(), options);
        reportError(lcOperator(), error, "Could not build BSP tree from rhs geometry"))
        return Geometry{error};

    a.clipTo(b, options);
    b.clipTo(a, options);
    b.invert(options);
    b.clipTo(a, options);
    b.invert(options);

    if (const auto error = a.build(b.allPolygons(), options);
        reportError(lcOperator(), error, "Could not build BSP tree from transformed tree"))
        return Geometry{error};

    return Geometry{a.allPolygons()};
}

Geometry subtract(Geometry lhs, Geometry rhs, Options options)
{
    if (reportError(lcOperator(), lhs.error(), "Invalid lhs geometry"))
        return Geometry{lhs.error()};
    if (reportError(lcOperator(), rhs.error(), "Invalid rhs geometry"))
        return Geometry{lhs.error()};

    auto a = Node{};
    auto b = Node{};

    if (const auto error = a.build(lhs.polygons(), options);
        reportError(lcOperator(), error, "Could not build BSP tree from lhs geometry"))
        return Geometry{error};
    if (const auto error = b.build(rhs.polygons(), options);
        reportError(lcOperator(), error, "Could not build BSP tree from rhs geometry"))
        return Geometry{error};

    a.invert(options);
    a.clipTo(b, options);
    b.clipTo(a, options);
    b.invert(options);
    b.clipTo(a, options);
    b.invert(options);

    if (const auto error = a.build(b.allPolygons(), options);
        reportError(lcOperator(), error, "Could not build BSP tree from transformed tree"))
        return Geometry{error};

    a.invert(options);

    return Geometry{a.allPolygons()};
}

Geometry intersect(Geometry lhs, Geometry rhs, Options options)
{
    if (reportError(lcOperator(), lhs.error(), "Invalid lhs geometry"))
        return Geometry{lhs.error()};
    if (reportError(lcOperator(), rhs.error(), "Invalid rhs geometry"))
        return Geometry{lhs.error()};

    auto a = Node{};
    auto b = Node{};

    if (const auto error = a.build(lhs.polygons(), options);
        reportError(lcOperator(), error, "Could not build BSP tree from lhs geometry"))
        return Geometry{error};
    if (const auto error = b.build(rhs.polygons(), options);
        reportError(lcOperator(), error, "Could not build BSP tree from rhs geometry"))
        return Geometry{error};

    a.invert(options);
    b.clipTo(a, options);
    b.invert(options);
    a.clipTo(b, options);
    b.clipTo(a, options);

    if (const auto error = a.build(b.allPolygons(), options);
        reportError(lcOperator(), error, "Could not build BSP tree from transformed tree"))
        return Geometry{error};

    a.invert(options);

    return Geometry{a.allPolygons()};
}

std::variant<Node, Error> Node::fromPolygons(QList<Polygon> polygons, Options options)
{
    auto node = Node{};

    if (const auto error = node.build(std::move(polygons), options);
        reportError(lcNode(), error, "Could not build BSP tree from polygons"))
        return {error};

    return {std::move(node)};
}

void Node::invert(const Options &options)
{
    if (Q_UNLIKELY(options.inspection))
        if (options.inspection(Inspection::Event::Invert, {}) == Inspection::Result::Abort)
            return;

    std::for_each(m_polygons.begin(), m_polygons.end(), &flip<Polygon>);

    m_plane.flip();

    if (m_front)
        m_front->invert(options);
    if (m_back)
        m_back->invert(options);

    std::swap(m_front, m_back);
}

Node Node::inverted(const Options &options) const
{
    auto node = *this;
    node.invert(options);
    return node;
}

QList<Polygon> Node::clipPolygons(QList<Polygon> polygons, const Options &options) const
{
    if (m_plane.isNull())
        return polygons;

    auto front = QList<Polygon>{};
    auto back = QList<Polygon>{};

    for (const auto &p: polygons)
        split(p, m_plane, &front, &back, &front, &back, options);

    if (m_front)
        front = m_front->clipPolygons(front, options);

    if (m_back)
        back = m_back->clipPolygons(back, options);
    else
        back.clear();

    return front + back;
}

void Node::clipTo(const Node &bsp, const Options &options)
{
    if (Q_UNLIKELY(options.inspection))
        if (options.inspection(Inspection::Event::Clip, bsp) == Inspection::Result::Abort)
            return;

    m_polygons = bsp.clipPolygons(std::move(m_polygons), options);

    if (m_front)
        m_front->clipTo(bsp, options);
    if (m_back)
        m_back->clipTo(bsp, options);
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

Error Node::build(QList<Polygon> polygons, int level, const Options &options)
{
    if (Q_UNLIKELY(level >= options.recursionLimit)) {
        qWarning(lcNode, "Maximum recursion level reached");
        return Error::RecursionError;
    }

    if (Q_UNLIKELY(options.inspection))
        if (options.inspection(Inspection::Event::Build, {}) == Inspection::Result::Abort)
            return Error::NoError; // FIXME: AbortedError?

    if (polygons.isEmpty())
        return Error::NoError;

    if (m_plane.isNull())
        m_plane = polygons.first().plane();

    auto result = Error::NoError;
    auto front = QList<Polygon>{};
    auto back = QList<Polygon>{};

    for (const auto &p: polygons)
        split(p, m_plane, &m_polygons, &m_polygons, &front, &back, options);

    if (!front.empty()) {
        if (!m_front)
            m_front = std::make_shared<Node>();

        if (const auto error = m_front->build(std::move(front), level + 1, options);
            error != Error::NoError && result == Error::NoError)
            result = error;
    }

    if (!back.empty()) {
        if (!m_back)
            m_back = std::make_shared<Node>();

        if (const auto error = m_back->build(std::move(back), level + 1, options);
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

namespace Tests {

/// This class is an obscure attempt to export internal functions for unit tests.
/// Might be this must be replaced by more regular friend declarations.
class Helper
{
    Q_NEVER_INLINE static void split(const Polygon &polygon, const Plane &plane,
                                     QList<Polygon> *coplanarFront, QList<Polygon> *coplanarBack,
                                     QList<Polygon> *front, QList<Polygon> *back, Options options);
};

/// Defining this function out-of place should convince the compiler to export it.
void Helper::split(const Polygon &polygon, const Plane &plane,
                   QList<Polygon> *coplanarFront, QList<Polygon> *coplanarBack,
                   QList<Polygon> *front, QList<Polygon> *back, Options options)
{
    QtCSG::split(polygon, plane, coplanarFront, coplanarBack, front, back, options);
}

} // namespace Tests
} // namespace QtCSG
