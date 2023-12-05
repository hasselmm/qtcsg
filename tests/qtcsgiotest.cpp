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
#include "qtcsgtest.h"

#include <qtcsg/qtcsgio.h>

#include <QBuffer>

Q_DECLARE_METATYPE(const QtCSG::FileFormat<QtCSG::Geometry> *)

namespace QtCSG::Tests {

class IOTest : public QObject
{
    Q_OBJECT

private slots:
    void testRoundTrip_data()
    {
        QTest::addColumn<const FileFormat<Geometry> *>("format");

        for (const auto format: FileFormat<Geometry>::supported())
            QTest::addRow("%ls", qUtf16Printable(format->id())) << format;
    }

    void testRoundTrip()
    {
        QFETCH(const FileFormat<Geometry> *const, format);

        auto buffer = QBuffer{};

        const auto geometry = QtCSG::cube();
        QVERIFY2(buffer.open(QFile::WriteOnly), qUtf8Printable(buffer.errorString()));
        format->writeGeometry(geometry, &buffer);
        buffer.close();

        QVERIFY2(buffer.open(QFile::ReadOnly), qUtf8Printable(buffer.errorString()));
        const auto readBack = format->readGeometry(&buffer);
        buffer.close();

        QCOMPARE(readBack.error(), Error::NoError);
        QCOMPARE(readBack.polygons(), geometry.polygons());
    }
};

} // namespace QtCSG::Tests

QTEST_MAIN(QtCSG::Tests::IOTest)

#include "qtcsgiotest.moc"
