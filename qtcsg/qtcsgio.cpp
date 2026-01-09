#include "qtcsgio.h"

#include "qtcsgmath.h"

#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QTextStream>

#include <optional>

namespace QtCSG {

namespace {

Q_LOGGING_CATEGORY(lcInputOutput, "qtcsg.io");

template<typename T>
T parseNumber(QStringView text, bool *isValid);

template<>
float parseNumber<float>(QStringView text, bool *isValid)
{ return text.toFloat(isValid); }

template<>
int parseNumber<int>(QStringView text, bool *isValid)
{ return text.toInt(isValid); }

template<>
uint parseNumber<uint>(QStringView text, bool *isValid)
{ return text.toUInt(isValid); }

template<typename T>
std::optional<T> parse(QStringView text)
{
    auto isValid = false;

    if (const auto value = parseNumber<T>(std::move(text), &isValid); isValid)
        return value;

    return std::nullopt;
}

#if defined(__cpp_concepts) && __cpp_concepts >= 202002L

template<class T>
static constexpr int s_minimumEmplaceBackVersion = -1;

template<class T>
static constexpr int s_minimumEmplaceBackVersion<QList<T>> = QT_VERSION_CHECK(6, 0, 0);

template<class T>
static constexpr int s_minimumEmplaceBackVersion<QVarLengthArray<T>> = QT_VERSION_CHECK(6, 3, 0);

template<class T>
concept HasEmplaceBack = requires(T *object) {
    object->emplace_back(typename T::value_type{});
};

template<class Container, typename... Args>
void emplaceBack(Container &container, Args... args)
{
    using T = Container::value_type;

    static_assert(HasEmplaceBack<Container>
                  || QT_VERSION < s_minimumEmplaceBackVersion<Container>);

    if constexpr (HasEmplaceBack<Container>) {
        container.emplace_back(std::forward<Args>(args)...);
    } else {
        container.append(T{std::forward<Args>(args)...});
    }
}

#else

template<class Container, typename... Args>
void emplaceBack(Container &container, Args... args)
{
    using T = Container::value_type;
    container.append(T{std::forward<Args>(args)...});
}

#endif

// http://www.geomview.org/docs/html/OFF.html
class OffFileFormat : public FileFormat<Geometry>
{
public:
    [[nodiscard]] QString id() const override { return "OFF"; }
    [[nodiscard]] bool accepts(QString fileName) const override;
    [[nodiscard]] Geometry readGeometry(QIODevice *device) const override;
    [[nodiscard]] Error writeGeometry(Geometry geometry, QIODevice *device) const override;
};

bool OffFileFormat::accepts(QString fileName) const
{
    return fileName.endsWith(".off", Qt::CaseInsensitive);
}

Geometry OffFileFormat::readGeometry(QIODevice *device) const
{
    enum class State {
        Magic,
        Header,
        Vertices,
        Faces,
    };

    auto state = State::Magic;
    auto vertexCount = 0;
    auto faceCount = 0;

    auto stream = QTextStream{device};
    auto vertices = std::vector<QVector3D>{};
    auto polygons = QList<Polygon>{};

    for (auto lineNumber = 1; !stream.atEnd(); ++lineNumber) {
        const auto line = stream.readLine().trimmed();

        if (line.startsWith('#'))
            continue;

        switch (state) {
        case State::Magic:
            if (line != "OFF") {
                qCWarning(lcInputOutput, "Unsupported file format");
                return Geometry{Error::NotSupportedError};
            }

            state = State::Header;
            break;

        case State::Header:
            if (const auto number = parse<int>(line.section(' ', 0, 0))) {
                vertexCount = *number;
            } else {
                qCWarning(lcInputOutput, "Invalid vertex count at line %d", lineNumber);
                return Geometry{Error::FileFormatError};
            }

            if (const auto number = parse<int>(line.section(' ', 1, 1))) {
                faceCount = *number;
            } else {
                qCWarning(lcInputOutput, "Invalid face count at line %d", lineNumber);
                return Geometry{Error::FileFormatError};
            }

            vertices.reserve(vertexCount);
            polygons.reserve(faceCount);
            state = State::Vertices;
            break;

        case State::Vertices:
            if (const auto x = parse<float>(line.section(' ', 0, 0))) {
                if (const auto y = parse<float>(line.section(' ', 1, 1))) {
                    if (const auto z = parse<float>(line.section(' ', 2, 2))) {
                        vertices.emplace_back(*x, *y, *z);

                        if (--vertexCount == 0)
                            state = State::Faces;

                        break;
                    }
                }
            }

            qCWarning(lcInputOutput, "Invalid vertex at line %d", lineNumber);
            return Geometry{Error::FileFormatError};

        case State::Faces:
            if (const auto n = parse<int>(line.section(' ', 0, 0))) {
                auto indices = QVarLengthArray<uint>{};
                indices.reserve(*n);

                for (auto i = 1; i <= *n; ++i) {
                    const auto index = parse<uint>(line.section(' ', i, i));
                    static_assert(std::is_unsigned_v<std::remove_reference_t<decltype(*index)>>);

                    if (index && *index < vertices.size()) {
                        emplaceBack(indices, *index);
                    } else {
                        qCWarning(lcInputOutput, "Invalid index at line %d, field %d", lineNumber, i);
                        return Geometry{Error::FileFormatError};
                    }
                }

                auto outline = QList<Vertex>{};
                outline.reserve(indices.size());

                for (auto j = 0; j < n; ++j) {
                    const auto i = (j + *n - 1) % *n;
                    const auto k = (j + 1) % *n;

                    auto a = vertices.at(indices.at(i));
                    auto b = vertices.at(indices.at(j));
                    auto c = vertices.at(indices.at(k));
                    auto n = crossProduct(b - a, c - a);

                    n.normalize();

                    emplaceBack(outline, std::move(b), std::move(n));
                }

                emplaceBack(polygons, std::move(outline));

                if (--faceCount == 0)
                    return Geometry{std::move(polygons)};

                continue;
            }

            qCWarning(lcInputOutput, "Invalid index count at line %d", lineNumber);
            return Geometry{Error::FileFormatError};
        }
    }

    qCWarning(lcInputOutput, "Unexpected end of file");
    return Geometry{Error::FileFormatError};
}

Error OffFileFormat::writeGeometry(Geometry geometry, QIODevice *device) const
{
    auto faces = std::vector<std::vector<std::size_t>>{};
    auto vertices = std::vector<QVector3D>{};

    const auto polygons = geometry.polygons();
    faces.reserve(polygons.size());

    for (const auto &p: polygons) {
        faces.emplace_back();
        faces.back().reserve(p.vertices().count());

        for (const auto &pv = p.vertices(); const auto &v : pv) {
            const auto p = v.position();

            auto it = std::ranges::find(vertices, p);

            if (it == vertices.end())
                it = vertices.emplace(vertices.end(), p.x(), p.y(), p.z());

            faces.back().emplace_back(it - vertices.begin());
        }
    }

    auto stream = QTextStream{device};

    stream << "OFF" << Qt::endl;
    stream << vertices.size() << ' ' << faces.size() << " 0" << Qt::endl;

    for (const auto &v: vertices)
        stream << v.x() << ' '<< v.y() << ' '<< v.z() << Qt::endl;

    for (const auto &f: faces) {
        stream << f.size();

        for (const auto i: f)
            stream << ' ' << i;

        stream << Qt::endl;
    }

    return Error::NoError;
}

Geometry readGeometry(const FileFormat<Geometry> *format, QString fileName)
{
    auto file = QFile{fileName};

    if (file.open(QFile::ReadOnly))
        return format->readGeometry(&file);

    qCWarning(lcInputOutput, "%ls: %ls",
              qUtf16Printable(file.fileName()),
              qUtf16Printable(file.errorString()));

    return Geometry{Error::FileSystemError};
}

Error writeGeometry(const FileFormat<Geometry> *format, Geometry geometry, QString fileName)
{
    auto file = QFile{fileName};

    if (file.open(QFile::WriteOnly))
        return format->writeGeometry(std::move(geometry), &file);

    qCWarning(lcInputOutput, "%ls: %ls",
              qUtf16Printable(file.fileName()),
              qUtf16Printable(file.errorString()));

    return Error::FileSystemError;
}

} // namespace

template<>
FileFormat<Geometry>::List FileFormat<Geometry>::supported()
{
    static const auto list = QList {
        offFileFormat(),
    };

    return list;
}

const FileFormat<Geometry> *offFileFormat()
{
    static const auto format = OffFileFormat{};
    return &format;
}

Geometry readGeometry(QString fileName)
{
    for (const auto &supportedGeometryFormats = FileFormat<Geometry>::supported();
         const auto &fileFormat : supportedGeometryFormats) {
        if (fileFormat->accepts(fileName))
            return readGeometry(fileFormat, std::move(fileName));
    }

    qCWarning(lcInputOutput, "%ls: Unsupported file format", qUtf16Printable(fileName));
    return Geometry{Error::NotSupportedError};
}

Error writeGeometry(Geometry geometry, QString fileName)
{
    for (const auto &supportedGeometryFormats = FileFormat<Geometry>::supported();
         const auto &fileFormat : supportedGeometryFormats) {
        if (fileFormat->accepts(fileName))
            return writeGeometry(fileFormat, std::move(geometry), std::move(fileName));
    }

    qCWarning(lcInputOutput, "%ls: Unsupported file format", qUtf16Printable(fileName));
    return Error::NotSupportedError;
}

} // namespace QtCSG
