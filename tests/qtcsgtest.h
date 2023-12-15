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
#ifndef QTCSGTEST_H
#define QTCSGTEST_H

#include <QMatrix4x4>
#include <QTest>

namespace QtCSG::Tests::Internal {

#if QT_VERSION_MAJOR < 6
using QByteArrayView = QByteArray;
#endif

/// This class provides a temporary buffer to display which detail made a custom qCompare() fail.
class ConcatBuffer
{
public:
    /// Provides a temporary buffer for tests that access methods and fields
    explicit ConcatBuffer(QByteArrayView prefix, QByteArrayView suffix)
        : m_prefix{std::move(prefix)}
        , m_suffix{std::move(suffix)}
    {}

    /// Provides a temporary buffer for tests that access via index
    explicit ConcatBuffer(QByteArray prefix, QByteArray infix, std::size_t index)
        : m_buffer{prefix + infix + '[' + QByteArray::number(static_cast<uint>(index)) + ']'}
        , m_instantiated{true}
    {}

    /// Converts this buffer into a plain old C string
    [[nodiscard]] operator const char *() const noexcept
    {
        if (!std::exchange(m_instantiated, true)) {
            m_buffer.append(m_prefix);
            m_buffer.append(m_suffix);
        }

        return m_buffer.constData();
    }

private:
    mutable QByteArray m_buffer;
    mutable bool m_instantiated = false;

    QByteArrayView m_prefix;
    QByteArrayView m_suffix;
};

/// Creates a temporary buffer to display which detail made a custom qCompare() fail.
template<typename... Args>
[[nodiscard]] inline auto concat(Args... args)
{
    return ConcatBuffer{args...};
}

} // namespace QtCSG::Tests::Internal

namespace QTest {

/// Compares two vectors via fuzzy compare.
/// This is neccessary because QTest only provides a strict compare.
/// Strict comparison doesn't work well for floating point numbers.
inline bool qCompare(const QVector3D &a,const QVector3D &b,
                     const char *actual, const char *expected,
                     const char *file, int line)
{
    using QtCSG::Tests::Internal::concat;

    return qCompare(a.x(), b.x(), concat(actual, ".x()"), concat(expected, ".x()"), file, line)
        && qCompare(a.y(), b.y(), concat(actual, ".y()"), concat(expected, ".y()"), file, line)
        && qCompare(a.z(), b.z(), concat(actual, ".z()"), concat(expected, ".z()"), file, line);
}

/// Compares two matrices via fuzzy compare.
/// This is neccessary because QTest only provides a strict compare.
/// Strict comparison doesn't work well for floating point numbers.
inline bool qCompare(const QMatrix4x4 &a,const QMatrix4x4 &b,
                     const char *actual, const char *expected,
                     const char *file, int line)
{
    if (!qFuzzyCompare(a, b)) {
        return compare_helper(false, "Compared values are not the same",
                              toString(a), toString(b), actual, expected,
                              file, line);
    }

    return true;
}

} // namespace QTest

#if defined(__cpp_concepts) && __cpp_concepts >= 202002L

namespace QtCSG::Tests::Internal {

/// This concept provides that the given type has a fields() method.
template<class T>
concept HasFieldsMethod = requires(T *instance) {
    instance->fields();
};

/// Compares two objects with a fields() method that returns a tuple.
/// This is useful to enable fully compare of the fields without
/// implementing a custom version of qFuzzyCompare.
///
/// Note that this function has to occur after all other qCompare()
/// implemenations to ensure all other implemenations get picked up.
template<std::size_t N, HasFieldsMethod T>
inline bool compareFields(const T &a, const T &b,
                          const char *actual, const char *expected,
                          const char *file, int line)
{
    using TypeType = decltype(a.fields());

    if constexpr(N < std::tuple_size_v<TypeType>) {
        if (!QTest::qCompare(std::get<N>(a.fields()),
                             std::get<N>(b.fields()),
                             concat(actual, ".fields", N),
                             concat(expected, ".fields", N),
                             file, line))
            return false;

        return compareFields<N + 1>(a, b, actual, expected, file, line);
    }

    return true;
}

} // namespace QtCSG::Tests::Internal

namespace QTest {

/// Compares two objects with a fields() method that returns a tuple.
/// This is useful to enable fully compare of the fields without
/// implementing a custom version of qFuzzyCompare.
///
/// Note that this function has to occur after all other qCompare()
/// implemenations to ensure all other implemenations get picked up.
template<QtCSG::Tests::Internal::HasFieldsMethod T>
inline bool qCompare(const T &a, const T &b,
                     const char *actual, const char *expected,
                     const char *file, int line)
{
    return QtCSG::Tests::Internal::compareFields<0>(a, b, actual, expected, file, line);
}

} // QTest

#endif // defined(__cpp_concepts) && __cpp_concepts >= 202002L

#endif // QTCSGTEST_H
