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
#ifndef QT3DCSG_WIREFRAMEMATERIAL_H
#define QT3DCSG_WIREFRAMEMATERIAL_H

#include <QtGui/QColor>

#include <Qt3DRender/QEffect>
#include <Qt3DRender/QMaterial>

namespace QtCSG::Demo {

class WireframeMaterial : public Qt3DRender::QMaterial
{
    Q_OBJECT
    Q_PROPERTY(QColor ambient READ ambient WRITE setAmbient NOTIFY ambientChanged FINAL)
    Q_PROPERTY(QColor diffuse READ diffuse WRITE setDiffuse NOTIFY diffuseChanged FINAL)
    Q_PROPERTY(QColor specular READ specular WRITE setSpecular NOTIFY specularChanged FINAL)
    Q_PROPERTY(qreal shininess READ shininess WRITE setShininess NOTIFY shininessChanged FINAL)
    Q_PROPERTY(qreal lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged FINAL)
    Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged FINAL)

public:
    explicit WireframeMaterial(Qt3DCore::QNode *parent = nullptr);

    void setAmbient(QColor newAmbient);
    QColor ambient() const;

    void setDiffuse(QColor newDiffuse);
    QColor diffuse() const;

    void setSpecular(QColor newSpecular);
    QColor specular() const;

    void setShininess(qreal newShininess);
    qreal shininess() const;

    void setLineWidth(qreal newLineWidth);
    qreal lineWidth() const;

    void setLineColor(QColor newLineColor);
    QColor lineColor() const;

signals:
    void ambientChanged(QColor ambient);
    void diffuseChanged(QColor diffuse);
    void specularChanged(QColor specular);
    void shininessChanged(qreal shininess);
    void lineWidthChanged(qreal lineWidth);
    void lineColorChanged(QColor lineColor);

private:
    Qt3DRender::QParameter *const m_ambient;
    Qt3DRender::QParameter *const m_diffuse;
    Qt3DRender::QParameter *const m_specular;
    Qt3DRender::QParameter *const m_shininess;
    Qt3DRender::QParameter *const m_lineWidth;
    Qt3DRender::QParameter *const m_lineColor;
};

} // namespace QtCSG::Demo

#endif // QT3DCSG_WIREFRAMEMATERIAL_H
