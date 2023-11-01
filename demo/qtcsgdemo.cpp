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
#include <qtcsg/qtcsg.h>
#include <qt3dcsg/qt3dcsg.h>

#include <Qt3DCore/QTransform>

#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QFirstPersonCameraController>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/Qt3DWindow>

#include <Qt3DRender/QCamera>
#include <Qt3DRender/QPointLight>

#include <QtGui/QScreen>

#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QWidget>

namespace {

QPoint toPoint(QSize size)
{
    return {size.width(), size.height()};
}

template<typename... Args>
QMatrix4x4 translation(Args... args)
{
    auto matrix = QMatrix4x4{};
    matrix.translate(args...);
    return matrix;
}

template<typename... Args>
QMatrix4x4 rotation(Args... args)
{
    auto matrix = QMatrix4x4{};
    matrix.rotate(args...);
    return matrix;
}

template<typename... Args>
QMatrix4x4 scale(Args... args)
{
    auto matrix = QMatrix4x4{};
    matrix.scale(args...);
    return matrix;
}

template<class T>
class StaticInit
{
public:
    StaticInit() { T::staticInit(); }
};

class Application
        : private StaticInit<Application>
        , public QApplication
{
    friend class StaticInit;

public:
    using QApplication::QApplication;

    int run();

private:
    static void staticInit();
};

int Application::run()
{
    using namespace Qt3DCore;
    using namespace Qt3DExtras;
    using namespace Qt3DRender;

    // 3D view
    const auto view = new Qt3DWindow;
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

    const auto camController = new QFirstPersonCameraController{rootEntity};
    camController->setCamera(cameraEntity);

    // lighting
    const auto lightEntity = new QEntity{rootEntity};
    const auto light = new QPointLight{lightEntity};
    light->setColor("white");
    light->setIntensity(2.5f);
    lightEntity->addComponent(light);

    const auto lightTransform = new Qt3DCore::QTransform{lightEntity};
    lightTransform->setTranslation(cameraEntity->position());
    lightEntity->addComponent(lightTransform);

    // create entities
    const auto createEntity = [rootEntity](QGeometryRenderer *renderer, QVector3D position, QColor color) {
        const auto transform = new Qt3DCore::QTransform;
        transform->setScale(1.5f);
        transform->setRotation(QQuaternion::fromAxisAndAngle({1.0f, 0.0f, 0.0f}, 45.0f));
        transform->setTranslation(position);

        const auto material = new Qt3DExtras::QPhongMaterial;
        material->setDiffuse(color);

        const auto entity = new QEntity{rootEntity};
        entity->addComponent(renderer);
        entity->addComponent(material);
        entity->addComponent(transform);
    };

    const auto cuboidMesh = new QCuboidMesh;
    cuboidMesh->setXExtent(2);
    cuboidMesh->setYExtent(2);
    cuboidMesh->setZExtent(2);

    const auto sphereMesh = new QSphereMesh;
    sphereMesh->setRadius(1.0f);

    const auto cylinderMesh = new QCylinderMesh;
    cylinderMesh->setRadius(1.0f);
    cylinderMesh->setLength(2.0f);

    const auto colors = std::array {
            QRgb{0x66'23'54},
            QRgb{0x66'23'23},
            QRgb{0x66'54'23},
            QRgb{0x23'66'54},
            QRgb{0x23'23'66},
    };

    // Qt3D geometries
    createEntity(cuboidMesh,   {-9.0f, -5.0f, -1.5f}, colors[0]);
    createEntity(sphereMesh,   {-9.0f,  0.0f, -1.5f}, colors[0]);
    createEntity(cylinderMesh, {-9.0f,  5.0f, -1.5f}, colors[0]);

    // native QtCSG geometries
    createEntity(new Qt3DCSG::Mesh{QtCSG::cube()},     {-4.5f, -5.0f, -1.5f}, colors[1]);
    createEntity(new Qt3DCSG::Mesh{QtCSG::sphere()},   {-4.5f,  0.0f, -1.5f}, colors[1]);
    createEntity(new Qt3DCSG::Mesh{QtCSG::cylinder()}, {-4.5f,  5.0f, -1.5f}, colors[1]);

    // Qt3D shapes converted into QtCSG geometries
    if (const auto mesh = new Qt3DCSG::Mesh{Qt3DCSG::geometry(cuboidMesh)})
        createEntity(mesh, {0.0f, -5.0f, -1.5f}, colors[2]);
    if (const auto mesh = new Qt3DCSG::Mesh{Qt3DCSG::geometry(sphereMesh)})
        createEntity(mesh, {0.0f,  0.0f, -1.5f}, colors[2]);
    if (const auto mesh = new Qt3DCSG::Mesh{Qt3DCSG::geometry(cylinderMesh)})
        createEntity(mesh, {0.0f,  5.0f, -1.5f}, colors[2]);

    // CSG operations on native QtCSG geometries
    {

        const auto delta = 0.3f;
        const auto r = rotation(45, 1, 1, 0);
        const auto a = r * QtCSG::cube({-delta, -delta, +delta});
        const auto b = QtCSG::cube({+delta, +delta, -delta});
        createEntity(new Qt3DCSG::Mesh{a | b}, {4.5f, -5.0f, -1.5f}, colors[3]);
    }

    {
        const auto a = QtCSG::cube();
        const auto b = QtCSG::sphere({}, 1.3);
        createEntity(new Qt3DCSG::Mesh{a - b}, {4.5f, 0.0f, -1.5f}, colors[3]);
    }

    {
        const auto a = QtCSG::sphere();
        const auto b = QtCSG::cylinder({}, 2, 0.8);
        createEntity(new Qt3DCSG::Mesh{a & b}, {4.5f, 5.0f, -1.5f}, colors[3]);
    }

    // CSG operations on native Qt3D geometries
    {
        const auto delta = 0.3f;
        const auto r = rotation(45, 1, 1, 0);
        const auto a = Qt3DCSG::geometry(cuboidMesh, translation(-delta, -delta, +delta) * r);
        const auto b = Qt3DCSG::geometry(cuboidMesh, translation(+delta, +delta, -delta));
        createEntity(new Qt3DCSG::Mesh{a | b}, {9.0f, -5.0f, -1.5f}, colors[4]);
    }

    {
        const auto a = Qt3DCSG::geometry(cuboidMesh);
        const auto b = Qt3DCSG::geometry(sphereMesh, scale(1.3));
        createEntity(new Qt3DCSG::Mesh{a - b}, {9.0f, 0.0f, -1.5f}, colors[4]);
    }

    {
        const auto a = Qt3DCSG::geometry(sphereMesh);
        const auto b = Qt3DCSG::geometry(cylinderMesh, scale(0.8, 1.0, 0.8));
        createEntity(new Qt3DCSG::Mesh{a & b}, {9.0f, 5.0f, -1.5f}, colors[4]);
    }

    // set root object of the scene
    view->setRootEntity(rootEntity);

    // main window
    const auto widget = new QWidget;

    const auto layout = new QHBoxLayout{widget};
    layout->addWidget(container, 1);
    widget->setWindowTitle("QtCSG Demo");
    widget->show();

    const auto widgetSize = QSize{1200, 800};
    const auto position = toPoint((screenSize - widgetSize) / 2);
    widget->setGeometry({position, widgetSize});

    return exec();
}

void Application::staticInit()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
}

} // namespace

int main(int argc, char *argv[])
{
    return Application{argc, argv}.run();
}
