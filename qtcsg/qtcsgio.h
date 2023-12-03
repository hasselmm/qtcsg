#ifndef QTCSGIO_H
#define QTCSGIO_H

#include "qtcsg.h"

class QIODevice;

namespace QtCSG {

template<class T>
struct FileFormat
{
    virtual QString id() const = 0;
    virtual bool accepts(QString fileName) const = 0;
    virtual T readGeometry(QIODevice *device) const = 0;
    virtual Error writeGeometry(T geometry, QIODevice *device) const = 0;

    using List = QList<const FileFormat *>;
    static List supported();
};

Geometry readGeometry(QString fileName);
Error writeGeometry(Geometry geometry, QString fileName);

const FileFormat<Geometry> *offFileFormat();

} // namespace QtCSG

#endif // QTCSGIO_H
