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
#include <QVariant>

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

// combines multiple enum classes into one
// -------------------------------------------------------------------------------------------------

template<class Alias, typename... Enums>
struct MultiEnum : public std::variant<Enums...>
{
    static constexpr bool EnumsArgumentsOnly = (std::is_enum_v<Enums> &&...);
    static_assert(EnumsArgumentsOnly);

    using std::variant<Enums...>::variant;

    constexpr bool operator==(const MultiEnum &) const = default;

    template<typename T>
    constexpr bool operator==(const T &v) const
    {
        if (!std::holds_alternative<T>(*this))
            return false;

        return std::get<T>(*this) == v;
    }

    bool operator==(const QVariant &v) const
    {
        registerConverters();

        if (QMetaType::canConvert(v.metaType(), QMetaType::fromType<MultiEnum>()))
            return qvariant_cast<MultiEnum>(v) == *this;

        return false;
    }

    operator QVariant() const
    {
        registerConverters();
        return QVariant::fromValue(*this);
    }


    operator const Alias &() const { return *static_cast<const Alias *>(this); }
    template<typename T> constexpr operator T() const { return std::get<T>(*this); }

    static bool registerConverters()
    {
        static const auto s_registered = (QMetaType::registerConverter<MultiEnum, Alias>()
                                          && QMetaType::registerConverter<Alias, MultiEnum>()
                                          && (QMetaType::registerConverter<Enums, Alias>() && ...)
                                          && (QMetaType::registerConverter<Enums, MultiEnum>() && ...)
                                          && (QMetaType::registerConverter<Alias, Enums>() && ...)
                                          && (QMetaType::registerConverter<MultiEnum, Enums>() && ...));

        return s_registered;
    }
};

template<typename... Enums>
inline QDebug operator<<(QDebug debug, const MultiEnum<Enums...> &multiEnum)
{
    return std::visit([debug](auto v) {
        return debug << v;
    }, multiEnum);
}

} // namespace QtCSG::AppSupport

#endif // QT3DCSG_APPSUPPORT_H
