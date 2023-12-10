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
#include "wireframematerial.h"

#include <Qt3DRender/QEffect>
#include <Qt3DRender/QGraphicsApiFilter>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QTechnique>

#include <QUrl>
#include <QVector3D>
#include <QVector4D>

namespace QtCSG::Demo {
namespace {

using namespace Qt3DRender;

auto makeFilterKey(QString name, QString value, Qt3DCore::QNode *parent = nullptr)
{
    const auto filterKey = new Qt3DRender::QFilterKey{parent};
    filterKey->setName(std::move(name));
    filterKey->setValue(std::move(value));
    return filterKey;
}

template<typename ValueType, class ObjectType>
auto makeSlot(ObjectType *target, void (ObjectType::*slot)(ValueType))
{
    return [target, slot](QVariant value) {
        return (target->*slot)(qvariant_cast<ValueType>(value));
    };
}

} // namespace

WireframeMaterial::WireframeMaterial(Qt3DCore::QNode *parent)
    : QMaterial{parent}
    , m_ambient{new QParameter{"ka", QColor::fromRgbF(0.05f, 0.05f, 0.05f, 1.0f), this}}
    , m_diffuse{new QParameter{"kd", QColor::fromRgbF(0.7f, 0.7f, 0.7f, 1.0f), this}}
    , m_specular{new QParameter{"ks", QColor::fromRgbF(0.95f, 0.95f, 0.95f, 1.0f), this}}
    , m_shininess{new QParameter{"shininess", 150.0f, this}}
    , m_lineWidth{new QParameter{"line.width", 0.8f, this}}
    , m_lineColor{new QParameter{"line.color", QColor::fromRgbF(0.0f, 0.0f, 0.0f, 1.0f), this}}
{
    const auto fragmentShaderUrl = QUrl{"qrc:/shaders/gl3/robustwireframe.frag"};
    const auto geometryShaderUrl = QUrl{"qrc:/shaders/gl3/robustwireframe.geom"};
    const auto vertexShaderUrl = QUrl{"qrc:/shaders/gl3/robustwireframe.vert"};

    const auto shaderProgram = new QShaderProgram{this};
    shaderProgram->setVertexShaderCode(QShaderProgram::loadSource(vertexShaderUrl));
    shaderProgram->setGeometryShaderCode(QShaderProgram::loadSource(geometryShaderUrl));
    shaderProgram->setFragmentShaderCode(QShaderProgram::loadSource(fragmentShaderUrl));

    const auto renderPass = new QRenderPass{this};
    renderPass->setShaderProgram(shaderProgram);

    const auto technique = new QTechnique;
    technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);
    technique->graphicsApiFilter()->setMajorVersion(3);
    technique->graphicsApiFilter()->setMinorVersion(1);
    technique->addFilterKey(makeFilterKey("renderingStyle", "forward", technique));
    technique->addParameter(m_lineWidth);
    technique->addParameter(m_lineColor);

    technique->addRenderPass(renderPass);

    const auto effect = new QEffect{this};
    effect->addParameter(m_ambient);
    effect->addParameter(m_diffuse);
    effect->addParameter(m_specular);
    effect->addParameter(m_shininess);
    effect->addTechnique(technique);
    setEffect(effect);

    connect(m_ambient, &QParameter::valueChanged,
            this, makeSlot(this, &WireframeMaterial::ambientChanged));
    connect(m_diffuse, &QParameter::valueChanged,
            this, makeSlot(this, &WireframeMaterial::diffuseChanged));
    connect(m_specular, &QParameter::valueChanged,
            this, makeSlot(this, &WireframeMaterial::specularChanged));
    connect(m_shininess, &QParameter::valueChanged,
            this, makeSlot(this, &WireframeMaterial::shininessChanged));
    connect(m_lineWidth, &QParameter::valueChanged,
            this, makeSlot(this, &WireframeMaterial::lineWidthChanged));
    connect(m_lineColor, &QParameter::valueChanged,
            this, makeSlot(this, &WireframeMaterial::lineColorChanged));
}

void WireframeMaterial::setAmbient(QColor newAmbient)
{
    m_ambient->setValue(std::move(newAmbient));
}

QColor WireframeMaterial::ambient() const
{
    return qvariant_cast<QColor>(m_ambient->value());
}

void WireframeMaterial::setDiffuse(QColor newDiffuse)
{
    m_diffuse->setValue(std::move(newDiffuse));
}

QColor WireframeMaterial::diffuse() const
{
    return qvariant_cast<QColor>(m_diffuse->value());
}

void WireframeMaterial::setSpecular(QColor newSpecular)
{
    m_specular->setValue(std::move(newSpecular));
}

QColor WireframeMaterial::specular() const
{
    return qvariant_cast<QColor>(m_specular->value());
}

void WireframeMaterial::setShininess(qreal newShininess)
{
    m_shininess->setValue(newShininess);
}

qreal WireframeMaterial::shininess() const
{
    return m_shininess->value().toFloat();
}

void WireframeMaterial::setLineWidth(qreal newLineWidth)
{
    m_lineWidth->setValue(newLineWidth);
}

qreal WireframeMaterial::lineWidth() const
{
    return m_lineWidth->value().toFloat();
}

void WireframeMaterial::setLineColor(QColor newLineColor)
{
    m_lineColor->setValue(std::move(newLineColor));
}

QColor WireframeMaterial::lineColor() const
{
    return qvariant_cast<QColor>(m_lineColor->value());
}

} // namespace QtCSG::Demo
