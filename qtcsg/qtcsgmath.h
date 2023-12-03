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
#ifndef QTCSGMATH_H
#define QTCSGMATH_H

#include <QMatrix4x4>
#include <QVector3D>

namespace QtCSG {

[[nodiscard]] constexpr QVector3D lerp(QVector3D a, QVector3D b, float t)
{
    return a + (b - a) * t;
}

[[nodiscard]] inline QMatrix4x4 identity()
{
    return QMatrix4x4{};
}

[[nodiscard]] inline QMatrix4x4 scaled(const QVector3D &scale)
{
    auto matrix = QMatrix4x4{};
    matrix.scale(scale);
    return matrix;
}

[[nodiscard]] inline QMatrix4x4 translated(const QVector3D &translation)
{
    auto matrix = QMatrix4x4{};
    matrix.translate(translation);
    return matrix;
}

[[nodiscard]] inline QMatrix4x4 rotated(float angle, const QVector3D &axis)
{
    auto matrix = QMatrix4x4{};
    matrix.rotate(angle, axis);
    return matrix;
}

[[nodiscard]] QVector3D  findTranslation(const QMatrix4x4 &matrix);
[[nodiscard]] QVector3D  findScale      (const QMatrix4x4 &matrix);
[[nodiscard]] QMatrix4x4 findRotation   (const QMatrix4x4 &matrix);

} // namespace QtCSG

#endif // QTCSGMATH_H
