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
#ifndef QTCSG_QTCSGUTILS_H
#define QTCSG_QTCSGUTILS_H

#include "qtcsg.h"

#include <QMetaEnum>

#include <source_location>

namespace QtCSG::Utils {

/// Resolve the key name for `value` from enumeration `T`.
template<typename T> requires std::is_enum_v<T>
[[nodiscard]] inline const char *keyName(T value)
{
    static const auto metaEnum = QMetaEnum::fromType<T>();
    return metaEnum.valueToKey(static_cast<int>(value));
}

/// Check if `error` indicates a problem. If there is a problem, the function returns `true`.
/// Additionally `message` is logged to `category`; together with a description of `error`.
[[nodiscard]] bool reportError(const QLoggingCategory &category, Error error, const char *message,
                               std::source_location location = std::source_location::current());

/// Enable colorful logging, so that information is easier to understand.
void enabledColorfulLogging();

} // namespace QtCSG::Utils

#endif // QTCSG_QTCSGUTILS_H
