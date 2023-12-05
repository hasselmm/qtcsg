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
#include "qt3dcsg.h"

#include <qtcsg/qtcsg.h>

#include <QFloat16>
#include <QLoggingCategory>

#if QT_VERSION_MAJOR < 6
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QBufferDataGenerator>
#else
#include <Qt3DCore/QAttribute>
#include <Qt3DCore/QBuffer>
#endif

namespace Qt3DCSG {

#if QT_VERSION_MAJOR < 6
using Qt3DRender::QAttribute;
using Qt3DRender::QBuffer;
#else
using Qt3DCore::QAttribute;
using Qt3DCore::QBuffer;
#endif

using QtCSG::Polygon;
using QtCSG::Vertex;

namespace {

Q_LOGGING_CATEGORY(lcGeometry, "qt3dcsg.geometry");

/// Sets buffer data from some vector
template<typename T>
void setData(QBuffer *buffer, QVector<T> vector)
{
    const auto data = reinterpret_cast<const char *>(vector.constData());
    const auto len = vector.size() * static_cast<int>(sizeof(T));
    buffer->setData({data, len});
}

/// Finds buffer data. Normally it should be sufficient to just call QBuffer::data(). Unfortunatly there also
/// is this legacy API arround QBuffer::dataGenerator(), which doesn't interact at all with QBuffer::data().
/// This function hides the issue.
QByteArray data(const QBuffer *buffer)
{
    auto data = buffer->data();

#if QT_VERSION_MAJOR < 6
    if (data.isEmpty()) {
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_DEPRECATED // generators are deprecated in Qt 5.15, still they get used by Qt3DExtras

        if (const auto generator = buffer->dataGenerator())
            data = std::invoke(*generator);

        QT_WARNING_POP
    }
#endif

    return data;
}

/// Finds the specified Qt3D attribute
const QAttribute *findAttribute(const QGeometry *geometry, QAttribute::AttributeType type, QString name = {})
{
    const auto acceptAttribute = [name, type](const QAttribute *attribute) {
        if (attribute->attributeType() != type)
            return false;
        if (name.isEmpty())
            return true;

        return attribute->name() == name;
    };

    const auto attributes = geometry->attributes();
    const auto it = std::find_if(attributes.begin(), attributes.end(), acceptAttribute);

    if (it == attributes.end())
        return nullptr;

    return *it;
}

/// Base class for natively accessing elements of a Qt3D attribute.
/// See `AttributeReader<>` for actual implementation.
class AttributeReaderBase
{
public:
    explicit AttributeReaderBase(const QGeometry *geometry, QAttribute::AttributeType type, QString name = {})
        : AttributeReaderBase{findAttribute(geometry, type, std::move(name))}
    {}

    explicit AttributeReaderBase(const QAttribute *attribute)
        : m_attribute{attribute}
        , m_data{data(attribute->buffer())}
    {}

    auto attribute() const { return m_attribute; }
    bool isValid() const { return m_attribute && !m_data.isEmpty(); }

    /// The attribute's entry at the given index. Returns `nullptr` on error.
    const void *entry(int index) const;

private:
    size_t elementSize() const;
    size_t stride() const;

    const QAttribute *const m_attribute;
    const QByteArray m_data;
};

/// This class help to access native elements of a Qt3D attribute
template<typename T>
class AttributeReader : public AttributeReaderBase
{
public:
    using AttributeReaderBase::AttributeReaderBase;

    bool isValid() const;
    T at(int index) const;
};

const void *AttributeReaderBase::entry(int index) const
{
    const auto offset = stride() * index + m_attribute->byteOffset();

    if (Q_UNLIKELY(offset < 0))
        return nullptr;
    if (Q_UNLIKELY(offset >= m_data.size()))
        return nullptr;

    return m_data.constData() + offset;
}

size_t AttributeReaderBase::elementSize() const
{
    switch (attribute()->vertexBaseType()) {
    case QAttribute::Byte:
    case QAttribute::UnsignedByte:
        return sizeof(qint8);

    case QAttribute::Short:
    case QAttribute::UnsignedShort:
        return sizeof(short);

    case QAttribute::Int:
    case QAttribute::UnsignedInt:
        return sizeof(int);
        break;

    case QAttribute::HalfFloat:
        return sizeof(qfloat16);
    case QAttribute::Float:
        return sizeof(float);
    case QAttribute::Double:
        return sizeof(double);
    }

    return 0;
}

size_t AttributeReaderBase::stride() const
{
    if (const auto stride = m_attribute->byteStride())
        return stride;

    return elementSize() * std::max(1U, m_attribute->vertexSize());
}

template<>
bool AttributeReader<int>::isValid() const
{
    if (!AttributeReaderBase::isValid())
        return false;
    if (attribute()->attributeType() != QAttribute::IndexAttribute)
        return false;

    switch (attribute()->vertexBaseType()) {
    case QAttribute::Double:
    case QAttribute::Float:
    case QAttribute::HalfFloat:
        return false;

    case QAttribute::Byte:
    case QAttribute::Int:
    case QAttribute::Short:
    case QAttribute::UnsignedByte:
    case QAttribute::UnsignedInt:
    case QAttribute::UnsignedShort:
        break;
    }

    return true;
}

template<>
int AttributeReader<int>::at(int index) const
{
    switch (attribute()->vertexBaseType()) {
    case QAttribute::Byte:
        return *reinterpret_cast<const qint8 *>(entry(index));
    case QAttribute::UnsignedByte:
        return *reinterpret_cast<const quint8 *>(entry(index));
    case QAttribute::Short:
        return *reinterpret_cast<const short *>(entry(index));
    case QAttribute::UnsignedShort:
        return *reinterpret_cast<const ushort *>(entry(index));
    case QAttribute::Int:
        return *reinterpret_cast<const int *>(entry(index));
    case QAttribute::UnsignedInt:
        return *reinterpret_cast<const uint *>(entry(index));

    case QAttribute::Double:
    case QAttribute::Float:
    case QAttribute::HalfFloat:
        break;
    }

    return -1;
}

template<>
bool AttributeReader<QVector3D>::isValid() const
{
    if (!AttributeReaderBase::isValid())
        return false;
    if (attribute()->attributeType() != QAttribute::VertexAttribute)
        return false;
    if (attribute()->vertexSize() != 3)
        return false;

    return true;
}

template<>
QVector3D AttributeReader<QVector3D>::at(int index) const
{
    const auto makeVector = [](auto p) {
        const auto x = static_cast<float>(p[0]);
        const auto y = static_cast<float>(p[1]);
        const auto z = static_cast<float>(p[2]);
        return QVector3D{x, y, z};
    };

    switch (attribute()->vertexBaseType()) {
    case QAttribute::Byte:
        return makeVector(reinterpret_cast<const qint8 *>(entry(index)));
    case QAttribute::UnsignedByte:
        return makeVector(reinterpret_cast<const quint8 *>(entry(index)));
    case QAttribute::Short:
        return makeVector(reinterpret_cast<const short *>(entry(index)));
    case QAttribute::UnsignedShort:
        return makeVector(reinterpret_cast<const ushort *>(entry(index)));
    case QAttribute::Int:
        return makeVector(reinterpret_cast<const int *>(entry(index)));
    case QAttribute::UnsignedInt:
        return makeVector(reinterpret_cast<const uint *>(entry(index)));
    case QAttribute::HalfFloat:
        return makeVector(reinterpret_cast<const qfloat16 *>(entry(index)));
    case QAttribute::Float:
        return makeVector(reinterpret_cast<const float *>(entry(index)));
    case QAttribute::Double:
        return makeVector(reinterpret_cast<const double *>(entry(index)));
    }

    return {};
}

} // namespace

Geometry::Geometry(QtCSG::Geometry csg, Qt3DCore::QNode *parent)
{
    auto vertices = QVector<Vertex>{};
    auto indices = QVector<ushort>{};

    const auto polygons = csg.polygons();
    for (const auto &p: polygons) {
        const auto pv = p.vertices();
        const auto i0 = vertices.count();

        std::copy(pv.begin(), pv.end(), std::back_inserter(vertices));

        for (int i = 2; i < pv.count(); ++i) {
            indices.append(i0);
            indices.append(i0 + i - 1);
            indices.append(i0 + i);
        }
    }

    const auto vertexBuffer = new QBuffer{this};
    setData(vertexBuffer, vertices);

    const auto indexBuffer = new QBuffer{this};
    setData(indexBuffer, indices);

    const auto positionAttribute = new QAttribute{this};
    const auto normalAttribute = new QAttribute{this};
    const auto indexAttribute = new QAttribute{this};

    positionAttribute->setBuffer(vertexBuffer);
    positionAttribute->setByteStride(sizeof(Vertex));
    positionAttribute->setByteOffset(offsetof(Vertex, m_position));
    positionAttribute->setCount(vertices.count());
    positionAttribute->setName(QAttribute::defaultPositionAttributeName());
    positionAttribute->setVertexBaseType(QAttribute::Float);
    positionAttribute->setVertexSize(3);
    addAttribute(positionAttribute);

    normalAttribute->setBuffer(vertexBuffer);
    normalAttribute->setByteStride(sizeof(Vertex));
    normalAttribute->setByteOffset(offsetof(Vertex, m_normal));
    normalAttribute->setCount(vertices.count());
    normalAttribute->setName(QAttribute::defaultNormalAttributeName());
    normalAttribute->setVertexBaseType(QAttribute::Float);
    normalAttribute->setVertexSize(3);
    addAttribute(normalAttribute);

    indexAttribute->setBuffer(indexBuffer);
    indexAttribute->setAttributeType(QAttribute::IndexAttribute);
    indexAttribute->setCount(indices.count());
    indexAttribute->setVertexBaseType(QAttribute::UnsignedShort);
    addAttribute(indexAttribute);
}

Mesh::Mesh(QtCSG::Geometry csg, Qt3DCore::QNode *parent)
    : QGeometryRenderer{parent}
{
    setGeometry(new Geometry{std::move(csg), this});
}

QtCSG::Geometry geometry(QGeometry *geometry, QMatrix4x4 transformation)
{
    auto polygons = QList<Polygon>{};

    const auto position = AttributeReader<QVector3D>(geometry, QAttribute::VertexAttribute,
                                                     QAttribute::defaultPositionAttributeName());
    const auto normal = AttributeReader<QVector3D>(geometry, QAttribute::VertexAttribute,
                                                   QAttribute::defaultNormalAttributeName());
    const auto index = AttributeReader<int>(geometry, QAttribute::IndexAttribute);

    if (position.isValid() && normal.isValid() && index.isValid()) {
        const auto count = index.attribute()->count();
        polygons.reserve(count / 3);

        for (auto i = 0; i < count; i += 3) {
            const auto ia = index.at(i);
            const auto ib = index.at(i + 1);
            const auto ic = index.at(i + 2);

            auto va = Vertex{transformation * position.at(ia), normal.at(ia)};
            auto vb = Vertex{transformation * position.at(ib), normal.at(ib)};
            auto vc = Vertex{transformation * position.at(ic), normal.at(ic)};

            polygons += Polygon{{std::move(va), std::move(vb), std::move(vc)}};
        }
    }

    return QtCSG::Geometry{std::move(polygons)};
}

#if QT_VERSION_MAJOR < 6

QtCSG::Geometry geometry(QGeometryRenderer *renderer, QMatrix4x4 transformation)
{
    if (renderer->primitiveType() != QGeometryRenderer::Triangles) {
        qCWarning(lcGeometry, "Unsupported primitive type: %d", renderer->primitiveType());
        return QtCSG::Geometry{{}};
    }

    return geometry(renderer->geometry(), std::move(transformation));
}

#else

QtCSG::Geometry geometry(QGeometryRenderer *renderer, QMatrix4x4 transformation)
{
    if (renderer->primitiveType() != QGeometryRenderer::Triangles) {
        qCWarning(lcGeometry, "Unsupported primitive type: %d", renderer->primitiveType());
        return QtCSG::Geometry{{}};
    }

    if (const auto geometry = renderer->geometry())
        return Qt3DCSG::geometry(geometry, std::move(transformation));
    if (const auto view = renderer->view())
        return Qt3DCSG::geometry(view, std::move(transformation));

    qCWarning(lcGeometry, "Unsupported renderer without geometry or view");
    return QtCSG::Geometry{{}};
}

QtCSG::Geometry geometry(Qt3DCore::QGeometryView *view, QMatrix4x4 transformation)
{
    if (view->primitiveType() != QGeometryView::Triangles) {
        qCWarning(lcGeometry, "Unsupported primitive type: %d", view->primitiveType());
        return QtCSG::Geometry{{}};
    }

    if (const auto geometry = view->geometry())
        return Qt3DCSG::geometry(geometry, std::move(transformation));

    qCWarning(lcGeometry, "Unsupported view without geometry");
    return QtCSG::Geometry{{}};
}

#endif

} // namespace Qt3DCSG
