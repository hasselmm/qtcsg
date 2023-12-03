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
#include "qtcsgmath.h"

namespace QtCSG {

// verify that lerp really is constexpr
static_assert(lerp({0, 0, 0}, {0, 0, 1}, 0.0) == QVector3D{0.0, 0.0, 0.0});
static_assert(lerp({0, 0, 0}, {0, 0, 1}, 0.5) == QVector3D{0.0, 0.0, 0.5});
static_assert(lerp({0, 0, 0}, {0, 0, 1}, 1.0) == QVector3D{0.0, 0.0, 1.0});

QVector3D findTranslation(const QMatrix4x4 &matrix)
{
    return {matrix(0, 3), matrix(1, 3), matrix(2, 3)};
}

QVector3D findScale(const QMatrix4x4 &matrix)
{
    const auto x = QVector3D{matrix(0, 0), matrix(1, 0), matrix(2, 0)};
    const auto y = QVector3D{matrix(0, 1), matrix(1, 1), matrix(2, 1)};
    const auto z = QVector3D{matrix(0, 2), matrix(1, 2), matrix(2, 2)};

    return {x.length(), y.length(), z.length()};
}

QMatrix4x4 findRotation(const QMatrix4x4 &matrix)
{
    const auto s = findScale(matrix);

    return {
        matrix(0, 0) / s.x(), matrix(0, 1) / s.y(), matrix(0, 2) / s.z(), 0,
        matrix(1, 0) / s.x(), matrix(1, 1) / s.y(), matrix(1, 2) / s.z(), 0,
        matrix(2, 0) / s.x(), matrix(2, 1) / s.y(), matrix(2, 2) / s.z(), 0,
                           0,                    0,                    0, 1,
    };
}

} // namespace QtCSG
