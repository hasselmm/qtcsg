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

#define QTCSG_QVECTOR3D_IS_CONSTEXPR (QT_VERSION_MAJOR >= 6)

#if QTCSG_QVECTOR3D_IS_CONSTEXPR
#define QTCSG_CONSTEXPR_QVECTOR3D constexpr
#else
#define QTCSG_CONSTEXPR_QVECTOR3D inline
#endif

[[nodiscard]] QTCSG_CONSTEXPR_QVECTOR3D QVector3D crossProduct(QVector3D a, QVector3D b) noexcept
{
    return QVector3D::crossProduct(std::move(a), std::move(b));
}

[[nodiscard]] QTCSG_CONSTEXPR_QVECTOR3D float dotProduct(QVector3D a, QVector3D b) noexcept
{
    return QVector3D::dotProduct(std::move(a), std::move(b));
}

[[nodiscard]] constexpr QVector3D lerp(QVector3D a, QVector3D b, float t) noexcept
{
    return a + (b - a) * t;
}

[[nodiscard]] inline QVector3D normalVector(QVector3D a, QVector3D b, QVector3D c) noexcept
{
    return crossProduct(b - a, c - a).normalized();
}

[[nodiscard]] inline auto identity()
{
    return QMatrix4x4{};
}

template<typename... Args>
[[nodiscard]] inline auto translation(Args... translation)
{
    auto matrix = QMatrix4x4{};
    matrix.translate(std::forward<Args>(translation)...);
    return matrix;
}

template<typename... Args>
[[nodiscard]] inline auto rotation(Args... rotation)
{
    auto matrix = QMatrix4x4{};
    matrix.rotate(std::forward<Args>(rotation)...);
    return matrix;
}

template<typename... Args>
[[nodiscard]] inline auto scale(Args... scale)
{
    auto matrix = QMatrix4x4{};
    matrix.scale(std::forward<Args>(scale)...);
    return matrix;
}

[[nodiscard]] inline auto translation(QVector3D vector)
{ return translation<QVector3D>(std::move(vector)); }

[[nodiscard]] inline auto rotation(float angle, QVector3D axis)
{ return rotation<float, QVector3D>(angle, std::move(axis)); }

[[nodiscard]] inline auto scale(QVector3D vector)
{ return scale<QVector3D>(std::move(vector)); }

[[nodiscard]] QVector3D  findTranslation(const QMatrix4x4 &matrix);
[[nodiscard]] QVector3D  findScale      (const QMatrix4x4 &matrix);
[[nodiscard]] QMatrix4x4 findRotation   (const QMatrix4x4 &matrix);

} // namespace QtCSG

#endif // QTCSGMATH_H
