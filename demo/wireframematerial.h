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

namespace Qt3DRender {
class QBlendEquation;
class QBlendEquationArguments;
class QCullFace;
class QNoDepthMask;
} // namespace Qt3DRender

namespace QtCSG::Demo {

class WireframeMaterial : public Qt3DRender::QMaterial
{
    Q_OBJECT
    Q_PROPERTY(QColor ambient READ ambient WRITE setAmbient NOTIFY ambientChanged FINAL)
    Q_PROPERTY(QColor diffuse READ diffuse WRITE setDiffuse NOTIFY diffuseChanged FINAL)
    Q_PROPERTY(QColor specular READ specular WRITE setSpecular NOTIFY specularChanged FINAL)
    Q_PROPERTY(qreal shininess READ shininess WRITE setShininess NOTIFY shininessChanged FINAL)
    Q_PROPERTY(qreal frontLineWidth READ frontLineWidth WRITE setFrontLineWidth NOTIFY frontLineWidthChanged FINAL)
    Q_PROPERTY(QColor frontLineColor READ frontLineColor WRITE setFrontLineColor NOTIFY frontLineColorChanged FINAL)
    Q_PROPERTY(qreal backLineWidth READ backLineWidth WRITE setBackLineWidth NOTIFY backLineWidthChanged FINAL)
    Q_PROPERTY(QColor backLineColor READ backLineColor WRITE setBackLineColor NOTIFY backLineColorChanged FINAL)
    Q_PROPERTY(bool alphaBlendingEnabled READ isAlphaBlendingEnabled WRITE setAlphaBlendingEnabled NOTIFY alphaBlendingEnabledChanged FINAL)

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

    void setFrontLineWidth(qreal newLineWidth);
    qreal frontLineWidth() const;

    void setFrontLineColor(QColor newLineColor);
    QColor frontLineColor() const;

    void setBackLineWidth(qreal newLineWidth);
    qreal backLineWidth() const;

    void setBackLineColor(QColor newLineColor);
    QColor backLineColor() const;

    void setAlphaBlendingEnabled(bool enabled);
    bool isAlphaBlendingEnabled() const;

signals:
    void ambientChanged(QColor ambient);
    void diffuseChanged(QColor diffuse);
    void specularChanged(QColor specular);
    void shininessChanged(qreal shininess);
    void frontLineWidthChanged(qreal frontLineWidth);
    void frontLineColorChanged(QColor frontLineColor);
    void backLineWidthChanged(qreal backLineWidth);
    void backLineColorChanged(QColor backLineColor);

    void alphaBlendingEnabledChanged(bool alphaBlendingEnabled);

private:
    Qt3DRender::QParameter *const m_ambient;
    Qt3DRender::QParameter *const m_diffuse;
    Qt3DRender::QParameter *const m_specular;
    Qt3DRender::QParameter *const m_shininess;
    Qt3DRender::QParameter *const m_frontLineWidth;
    Qt3DRender::QParameter *const m_frontLineColor;
    Qt3DRender::QParameter *const m_backLineWidth;
    Qt3DRender::QParameter *const m_backLineColor;

    Qt3DRender::QCullFace *const m_cullFace;
    Qt3DRender::QNoDepthMask *const m_noDepthMask;
    Qt3DRender::QBlendEquation *const m_blendEquation;
    Qt3DRender::QBlendEquationArguments *const m_blendEquationArguments;
};

} // namespace QtCSG::Demo

#endif // QT3DCSG_WIREFRAMEMATERIAL_H
