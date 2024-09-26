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

#include <Qt3DRender/QBlendEquation>
#include <Qt3DRender/QBlendEquationArguments>
#include <Qt3DRender/QCullFace>
#include <Qt3DRender/QEffect>
#include <Qt3DRender/QGraphicsApiFilter>
#include <Qt3DRender/QNoDepthMask>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QTechnique>

#include <QUrl>
#include <QVector3D>
#include <QVector4D>

static void initResource()
{
    static bool initialized = false;

    if (!std::exchange(initialized, true))
        Q_INIT_RESOURCE(wireframematerial);
}

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
    , m_frontLineWidth{new QParameter{"frontLine.width", 0.8f, this}}
    , m_frontLineColor{new QParameter{"frontLine.color", QColor::fromRgbF(0.0f, 0.0f, 0.0f, 1.0f), this}}
    , m_backLineWidth{new QParameter{"backLine.width", 0.0f, this}}
    , m_backLineColor{new QParameter{"backLine.color", QColor::fromRgbF(0.0f, 0.0f, 0.0f, 1.0f), this}}
    , m_cullFace{new QCullFace{this}}
    , m_noDepthMask{new QNoDepthMask{this}}
    , m_blendEquation{new QBlendEquation{this}}
    , m_blendEquationArguments{new QBlendEquationArguments{this}}
{
    initResource();

    const auto fragmentShaderUrl = QUrl{"qrc:/shaders/gl3/robustwireframe.frag"};
    const auto geometryShaderUrl = QUrl{"qrc:/shaders/gl3/robustwireframe.geom"};
    const auto vertexShaderUrl = QUrl{"qrc:/shaders/gl3/robustwireframe.vert"};

    m_cullFace->setEnabled(false);
    m_cullFace->setMode(QCullFace::NoCulling);
    m_noDepthMask->setEnabled(false);
    m_blendEquation->setEnabled(false);
    m_blendEquation->setBlendFunction(QBlendEquation::Add);
    m_blendEquationArguments->setEnabled(false);
    m_blendEquationArguments->setSourceRgb(QBlendEquationArguments::SourceAlpha);
    m_blendEquationArguments->setDestinationRgb(QBlendEquationArguments::OneMinusSourceAlpha);

    const auto shaderProgram = new QShaderProgram{this};
    shaderProgram->setVertexShaderCode(QShaderProgram::loadSource(vertexShaderUrl));
    shaderProgram->setGeometryShaderCode(QShaderProgram::loadSource(geometryShaderUrl));
    shaderProgram->setFragmentShaderCode(QShaderProgram::loadSource(fragmentShaderUrl));

    const auto renderPass = new QRenderPass{this};
    renderPass->setShaderProgram(shaderProgram);
    renderPass->addRenderState(m_noDepthMask);
    renderPass->addRenderState(m_blendEquationArguments);
    renderPass->addRenderState(m_blendEquation);
    renderPass->addRenderState(m_cullFace);

    const auto technique = new QTechnique;
    technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);
    technique->graphicsApiFilter()->setMajorVersion(3);
    technique->graphicsApiFilter()->setMinorVersion(1);
    technique->addFilterKey(makeFilterKey("renderingStyle", "forward", technique));
    technique->addParameter(m_frontLineWidth);
    technique->addParameter(m_frontLineColor);
    technique->addParameter(m_backLineWidth);
    technique->addParameter(m_backLineColor);

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
    connect(m_frontLineWidth, &QParameter::valueChanged,
            this, makeSlot(this, &WireframeMaterial::frontLineWidthChanged));
    connect(m_frontLineColor, &QParameter::valueChanged,
            this, makeSlot(this, &WireframeMaterial::frontLineColorChanged));
    connect(m_backLineWidth, &QParameter::valueChanged,
            this, makeSlot(this, &WireframeMaterial::backLineWidthChanged));
    connect(m_backLineColor, &QParameter::valueChanged,
            this, makeSlot(this, &WireframeMaterial::backLineColorChanged));
    connect(m_noDepthMask, &QNoDepthMask::enabledChanged,
            this, &WireframeMaterial::alphaBlendingEnabledChanged);
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

void WireframeMaterial::setFrontLineWidth(qreal newLineWidth)
{
    m_frontLineWidth->setValue(newLineWidth);
}

qreal WireframeMaterial::frontLineWidth() const
{
    return m_frontLineWidth->value().toFloat();
}

void WireframeMaterial::setFrontLineColor(QColor newLineColor)
{
    m_frontLineColor->setValue(std::move(newLineColor));
}

QColor WireframeMaterial::frontLineColor() const
{
    return qvariant_cast<QColor>(m_frontLineColor->value());
}

void WireframeMaterial::setBackLineWidth(qreal newLineWidth)
{
    m_backLineWidth->setValue(newLineWidth);
}

qreal WireframeMaterial::backLineWidth() const
{
    return m_backLineWidth->value().toFloat();
}

void WireframeMaterial::setBackLineColor(QColor newLineColor)
{
    m_backLineColor->setValue(std::move(newLineColor));
}

QColor WireframeMaterial::backLineColor() const
{
    return qvariant_cast<QColor>(m_backLineColor->value());
}

void WireframeMaterial::setAlphaBlendingEnabled(bool enabled)
{
    m_cullFace->setEnabled(enabled);
    m_noDepthMask->setEnabled(enabled);
    m_blendEquation->setEnabled(enabled);
    m_blendEquationArguments->setEnabled(enabled);
}

bool WireframeMaterial::isAlphaBlendingEnabled() const
{
    return m_noDepthMask->isEnabled();
}

} // namespace QtCSG::Demo
