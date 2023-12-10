/* QtCSG provides Constructive Solid Geometry (CSG) for Qt
 * Copyright Ⓒ 2022 Mathias Hasselmann
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
#include "qtcsgappsupport.h"
#include "orbitalcameracontroller.h"
#include "wireframematerial.h"

#include <qtcsg/qtcsg.h>
#include <qtcsg/qtcsgmath.h>
#include <qtcsg/qtcsgutils.h>

#include <qt3dcsg/qt3dcsg.h>

#include <Qt3DCore/QTransform>

#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/Qt3DWindow>

#include <Qt3DRender/QCamera>
#include <Qt3DRender/QPointLight>

#include <QtGui/QScreen>

#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

namespace QtCSG::Demo {
namespace {

using Qt3DCore::QEntity;
using Qt3DRender::QGeometryRenderer;

const auto s_colors = std::array {
    QRgb{0x66'23'54},
    QRgb{0x66'23'23},
    QRgb{0x66'54'23},
    QRgb{0x23'66'54},
    QRgb{0x23'23'66},
};

struct RenderingStyle {
    float lineWidth;
    float diffuseAlpha;
    QColor specularColor;
};

const auto s_wireframeVisible = RenderingStyle{1.0f, 0.2f, QColor::fromRgbF(0.0, 0.0, 0.0, 0.0)};
const auto s_wireframeHidden = RenderingStyle{0.0f, 1.0f, QColor::fromRgbF(0.95, 0.95, 0.95, 1.0)};

// convenience function to create Qt3D entities from geometry renderers
// -------------------------------------------------------------------------------------------------
void createEntity(QGeometryRenderer *renderer, QVector3D position, QColor color, QEntity *parent)
{
    const auto transform = new Qt3DCore::QTransform;
    transform->setScale(1.5f);
    transform->setRotation(QQuaternion::fromAxisAndAngle({1.0f, 0.0f, 0.0f}, 45.0f));
    transform->setTranslation(position);

    const auto material = new WireframeMaterial;
    material->setFrontLineWidth(s_wireframeHidden.lineWidth);
    material->setBackLineWidth(s_wireframeHidden.lineWidth);
    material->setSpecular(s_wireframeHidden.specularColor);
    color.setAlphaF(s_wireframeHidden.diffuseAlpha);
    material->setDiffuse(std::move(color));

    const auto entity = new QEntity{parent};
    entity->addComponent(renderer);
    entity->addComponent(material);
    entity->addComponent(transform);
}

// the demo application
// -------------------------------------------------------------------------------------------------
class Application
    : private AppSupport::StaticInit<Application>
    , public QApplication
{
    friend class StaticInit;

public:
    using QApplication::QApplication;

    int run();

private:
    static void staticInit();

    QEntity *createShowCase(QEntity *parent);
    QEntity *createUnionTest(QEntity *parent);

    void collectEntities(QEntity *root);
    void onWireframeBoxToggled(bool checked);

    QList<QEntity *> m_entities;
};

// -------------------------------------------------------------------------------------------------

QEntity *Application::createShowCase(QEntity *parent)
{
    using namespace Qt3DExtras;
    using namespace Qt3DRender;

    const auto showCase = new QEntity{parent};

    const auto cuboidMesh = new QCuboidMesh;
    cuboidMesh->setXExtent(2);
    cuboidMesh->setYExtent(2);
    cuboidMesh->setZExtent(2);

    const auto sphereMesh = new QSphereMesh;
    sphereMesh->setRadius(1.0f);

    const auto cylinderMesh = new QCylinderMesh;
    cylinderMesh->setRadius(1.0f);
    cylinderMesh->setLength(2.0f);

    // Qt3D geometries
    createEntity(cuboidMesh,   {-9.0f, -5.0f, -1.5f}, s_colors[0], showCase);
    createEntity(sphereMesh,   {-9.0f,  0.0f, -1.5f}, s_colors[0], showCase);
    createEntity(cylinderMesh, {-9.0f,  5.0f, -1.5f}, s_colors[0], showCase);

    // native QtCSG geometries
    createEntity(new Qt3DCSG::Mesh{QtCSG::cube()},     {-4.5f, -5.0f, -1.5f}, s_colors[1], showCase);
    createEntity(new Qt3DCSG::Mesh{QtCSG::sphere()},   {-4.5f,  0.0f, -1.5f}, s_colors[1], showCase);
    createEntity(new Qt3DCSG::Mesh{QtCSG::cylinder()}, {-4.5f,  5.0f, -1.5f}, s_colors[1], showCase);

    // Qt3D shapes converted into QtCSG geometries
    if (const auto mesh = new Qt3DCSG::Mesh{Qt3DCSG::geometry(cuboidMesh)})
        createEntity(mesh, {0.0f, -5.0f, -1.5f}, s_colors[2], showCase);
    if (const auto mesh = new Qt3DCSG::Mesh{Qt3DCSG::geometry(sphereMesh)})
        createEntity(mesh, {0.0f,  0.0f, -1.5f}, s_colors[2], showCase);
    if (const auto mesh = new Qt3DCSG::Mesh{Qt3DCSG::geometry(cylinderMesh)})
        createEntity(mesh, {0.0f,  5.0f, -1.5f}, s_colors[2], showCase);

    // CSG operations on native QtCSG geometries
    {
        const auto delta = 0.3f;
        const auto r = rotation(45, 1, 1, 0);
        const auto a = r * QtCSG::cube({-delta, -delta, +delta});
        const auto b = QtCSG::cube({+delta, +delta, -delta});
        createEntity(new Qt3DCSG::Mesh{a | b}, {4.5f, -5.0f, -1.5f}, s_colors[3], showCase);
    }

    {
        const auto a = QtCSG::cube();
        const auto b = QtCSG::sphere({}, 1.3);
        createEntity(new Qt3DCSG::Mesh{a - b}, {4.5f, 0.0f, -1.5f}, s_colors[3], showCase);
    }

    {
        const auto a = QtCSG::sphere();
        const auto b = QtCSG::cylinder({}, 2, 0.8);
        createEntity(new Qt3DCSG::Mesh{a & b}, {4.5f, 5.0f, -1.5f}, s_colors[3], showCase);
    }

    // CSG operations on native Qt3D geometries
    {
        const auto delta = 0.3f;
        const auto r = rotation(45, 1, 1, 0);
        const auto a = Qt3DCSG::geometry(cuboidMesh, translation(-delta, -delta, +delta) * r);
        const auto b = Qt3DCSG::geometry(cuboidMesh, translation(+delta, +delta, -delta));
        createEntity(new Qt3DCSG::Mesh{a | b}, {9.0f, -5.0f, -1.5f}, s_colors[4], showCase);
    }

    {
        const auto a = Qt3DCSG::geometry(cuboidMesh);
        const auto b = Qt3DCSG::geometry(sphereMesh, scale(1.3));
        createEntity(new Qt3DCSG::Mesh{a - b}, {9.0f, 0.0f, -1.5f}, s_colors[4], showCase);
    }

    {
        const auto a = Qt3DCSG::geometry(sphereMesh);
        const auto b = Qt3DCSG::geometry(cylinderMesh, scale(0.8, 1.0, 0.8));
        createEntity(new Qt3DCSG::Mesh{a & b}, {9.0f, 5.0f, -1.5f}, s_colors[4], showCase);
    }

    return showCase;
}

QEntity *Application::createUnionTest(QEntity *parent)
{
    const auto unionTest = new QEntity{parent};

    const auto createUnion = [unionTest](float delta, float x, bool adjacent, QColor color) {
        const auto a = QtCSG::cube({-delta, adjacent ? 0 : -delta, adjacent ? 0 : +delta});
        const auto b = QtCSG::cube({+delta, adjacent ? 0 : +delta, adjacent ? 0 : -delta});
        const auto c = QtCSG::merge(a, b);

        const auto y = adjacent ? -6.0f : +3.0f;
        createEntity(new Qt3DCSG::Mesh{c}, {x, y, -1.5f}, color, unionTest);
    };

    for (const auto adjacent: {false, true}) {
        createUnion(0.0, -10.5,  adjacent, s_colors[0]);
        createUnion(0.5,  -5.75, adjacent, s_colors[1]);
        createUnion(1.0,  +0.5,  adjacent, s_colors[2]);
        createUnion(1.5,  +8.25, adjacent, s_colors[3]);
    }

    return unionTest;
}

void Application::collectEntities(QEntity *root)
{
    for (const auto scene: root->childNodes()) {
        for (const auto node: scene->childNodes()) {
            if (const auto entity = dynamic_cast<QEntity *>(node))
                m_entities += entity;
        }
    }
}

void Application::onWireframeBoxToggled(bool checked)
{
    const auto &renderStyle = checked ? s_wireframeVisible : s_wireframeHidden;

    for (const auto entity: m_entities) {
        for (const auto material: entity->componentsOfType<WireframeMaterial>()) {
            auto diffuseColor = material->diffuse();
            diffuseColor.setAlphaF(renderStyle.diffuseAlpha);

            material->setAlphaBlendingEnabled(checked);
            material->setDiffuse(std::move(diffuseColor));
            material->setSpecular(renderStyle.specularColor);
            material->setFrontLineWidth(renderStyle.lineWidth);
            material->setBackLineWidth(renderStyle.lineWidth);
        }
    }
}

int Application::run()
{
    // 3D view
    const auto view = new Qt3DExtras::Qt3DWindow;
    view->defaultFrameGraph()->setClearColor(QRgb{0x4d'4d'4f});

    const auto container = QWidget::createWindowContainer(view);
    const auto screenSize = view->screen()->size();
    container->setMinimumSize({200, 100});
    container->setMaximumSize(screenSize);

    // root entity
    const auto rootEntity = new QEntity;

    // camera
    const auto cameraEntity = view->camera();
    cameraEntity->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    cameraEntity->setPosition({0, 0, 20.0f});
    cameraEntity->setUpVector({0, 1, 0});
    cameraEntity->setViewCenter({0, 0, 0});

    const auto cameraController = new OrbitCameraController{rootEntity};
    cameraController->setCamera(cameraEntity);

    // lighting
    const auto lightEntity = new QEntity{rootEntity};
    const auto light = new Qt3DRender::QPointLight{lightEntity};
    light->setColor(Qt::white);
    light->setIntensity(2.5f);
    lightEntity->addComponent(light);

    const auto lightTransform = new Qt3DCore::QTransform{lightEntity};
    lightTransform->setTranslation(cameraEntity->position());
    lightEntity->addComponent(lightTransform);

    connect(cameraEntity, &Qt3DRender::QCamera::positionChanged,
            lightTransform, &Qt3DCore::QTransform::setTranslation);

    // create entities
    const auto showCaseEntity = createShowCase(rootEntity);
    const auto unionTestEntity = createUnionTest(rootEntity);

    // set root object of the scene
    view->setRootEntity(rootEntity);
    collectEntities(rootEntity);

    // main window
    const auto window = new QWidget;
    window->setWindowTitle(tr("QtCSG Demo"));

    const auto wireframeBox = new QCheckBox{tr("Show &Wireframes"), window};
    wireframeBox->setFocusPolicy(Qt::FocusPolicy::TabFocus);

    const auto showCaseButton = new QPushButton{tr("&1: Show Case"), window};
    showCaseButton->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    showCaseButton->setCheckable(true);
    showCaseButton->setChecked(true);

    const auto unionTestButton = new QPushButton{tr("&2: Union Test"), window};
    unionTestButton->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    unionTestButton->setCheckable(true);
    unionTestEntity->setEnabled(false);

    connect(showCaseButton, &QPushButton::toggled, showCaseEntity, &QEntity::setEnabled);
    connect(unionTestButton, &QPushButton::toggled, unionTestEntity, &QEntity::setEnabled);
    connect(wireframeBox, &QCheckBox::toggled, this, &Application::onWireframeBoxToggled);

    const auto buttonGroup = new QButtonGroup{window};
    buttonGroup->addButton(showCaseButton);
    buttonGroup->addButton(unionTestButton);

    const auto buttons = new QHBoxLayout;
    buttons->addWidget(wireframeBox, 1);
    buttons->addWidget(showCaseButton);
    buttons->addWidget(unionTestButton);
    buttons->addStretch(1);

    const auto layout = new QVBoxLayout{window};
    layout->addWidget(container, 1);
    layout->addLayout(buttons);
    container->setFocus();

    const auto windowSize = QSize{1200, 800};
    const auto position = AppSupport::toPoint((screenSize - windowSize) / 2);
    window->setGeometry({position, windowSize});
    window->show();

    return exec();
}

void Application::staticInit()
{
    Utils::enabledColorfulLogging();

    // Force Qt3D OpenGL renderer
    constexpr auto rendererVariable = "QT3D_RENDERER";
    if (!qEnvironmentVariableIsSet(rendererVariable))
        qputenv(rendererVariable, "opengl");

#if QT_VERSION_MAJOR < 6
    setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
}

} // namespace
} // namespace QtCSG::Demo

int main(int argc, char *argv[])
{
    return QtCSG::Demo::Application{argc, argv}.run();
}
