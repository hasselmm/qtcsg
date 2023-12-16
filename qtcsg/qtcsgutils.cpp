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
#include "qtcsgutils.h"

#include <QLoggingCategory>

namespace QtCSG::Utils {

bool reportError(const QLoggingCategory &category, Error error,
                 const char *message, SourceLocation location)
{
    if (Q_LIKELY(error == Error::NoError))
        return false;

    if (category.isWarningEnabled()) {
        auto logger = QMessageLogger{location.file_name(), static_cast<int>(location.line()),
                                     location.function_name(), category.categoryName()};
        logger.warning("%s, the reported error is %s", message, keyName(error));
    }

#ifdef QTCSG_IGNORE_ERRORS
    return false;
#else
    return true;
#endif
}

void enabledColorfulLogging()
{
#ifdef QT_MESSAGELOGCONTEXT
#define QTCSG_MESSAGELOGCONTEXT_PATTERN "\033[0;37m (%{function} in %{file}, line %{line})\033[0m"
#else
#define QTCSG_MESSAGELOGCONTEXT_PATTERN ""
#endif

    qSetMessagePattern("%{time process} "
                       "%{if-critical}\033[1;31m%{endif}"
                       "%{if-warning}\033[1;33m%{endif}"
                       "%{type}%{if-category} %{category}%{endif} %{message}"
                       "%{if-warning}\033[0m%{endif}"
                       "%{if-critical}\033[0m%{endif}"
                       QTCSG_MESSAGELOGCONTEXT_PATTERN);
}

} // namespace QtCSG::Utils
