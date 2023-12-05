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
#ifndef QT3DCSG_H
#define QT3DCSG_H

#include <QGeometryRenderer>
#include <QMatrix4x4>

namespace QtCSG {
class Geometry;
}

namespace Qt3DCSG {

#if QT_VERSION_MAJOR < 6
using Qt3DRender::QGeometry;
using Qt3DRender::QGeometryRenderer;
#else
using Qt3DCore::QGeometry;
using Qt3DCore::QGeometryView;
using Qt3DRender::QGeometryRenderer;
#endif

/// This class generates a Qt3D mesh from a QtCSG geometry.
class Geometry : public QGeometry
{
    Q_OBJECT

public:
    explicit Geometry(QtCSG::Geometry csg, Qt3DCore::QNode *parent = nullptr);
};

/// This class generates a Qt3D mesh from a QtCSG geometry.
class Mesh : public QGeometryRenderer
{
    Q_OBJECT

public:
    explicit Mesh(QtCSG::Geometry csg, Qt3DCore::QNode *parent = nullptr);
};

/// Creates a CSG geometry from the given Qt3D `geometry`.
/// Its vertices are transformed according to `transformation`.
QtCSG::Geometry geometry(QGeometry *geometry, QMatrix4x4 transformation = {});
QtCSG::Geometry geometry(QGeometryRenderer *renderer, QMatrix4x4 transformation = {});

#if QT_VERSION_MAJOR >= 6
QtCSG::Geometry geometry(QGeometryView *view, QMatrix4x4 transformation = {});
#endif

} // namespace Qt3DCSG

#endif // QT3DCSG_H
