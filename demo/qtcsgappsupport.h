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
#ifndef QT3DCSG_APPSUPPORT_H
#define QT3DCSG_APPSUPPORT_H

#include <QPoint>
#include <QSize>

namespace QtCSG::AppSupport {

// some utility functions making it easier to deal with matrices and vectors
// -------------------------------------------------------------------------------------------------

[[nodiscard]] inline QPoint toPoint(QSize size)
{
    return {size.width(), size.height()};
}

// move static initialization of QApplication from main() into our Application class
// -------------------------------------------------------------------------------------------------

template<class T>
class StaticInit
{
public:
    StaticInit() { T::staticInit(); }
};

} // namespace QtCSG::AppSupport

#endif // QT3DCSG_APPSUPPORT_H
