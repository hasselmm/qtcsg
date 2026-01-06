/* QtCSG provides Constructive Solid Geometry (CSG) for Qt
 * Copyright Ⓒ 2026 Mathias Hasselmann
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
#include "qtcsgtestsuite.h"

#include <qtcsg/qtcsg.h>

#if !(__cpp_concepts >= 202002L)

#define QTCSGTEST_DEFINE_COMPARE_FIELDS(T)                                                                          \
                                                                                                                    \
bool QTest::qCompare(const T &a, const T &b, const char *actual, const char *expected, const char *file, int line)  \
{                                                                                                                   \
        return QtCSG::Tests::Internal::compareFields<0>(a, b, actual, expected, file, line);                        \
}

QTCSGTEST_DEFINE_COMPARE_FIELDS(QtCSG::Polygon)
QTCSGTEST_DEFINE_COMPARE_FIELDS(QtCSG::Plane)
QTCSGTEST_DEFINE_COMPARE_FIELDS(QtCSG::Vertex)

#endif // !(__cpp_concepts >= 202002L)
