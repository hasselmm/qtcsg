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
#include "qtcsgappsupport.h"

#include "orbitcameracontroller.h"
#include "wireframematerial.h"

#include <qtcsg/qtcsgio.h>
#include <qtcsg/qtcsgmath.h>
#include <qtcsg/qtcsgutils.h>

#include <qt3dcsg/qt3dcsg.h>

#include <Qt3DCore/QTransform>

#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QForwardRenderer>

#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QLayer>
#include <Qt3DRender/QLayerFilter>
#include <Qt3DRender/QNoDraw>
#include <Qt3DRender/QPointLight>
#include <Qt3DRender/QRenderPassFilter>
#include <Qt3DRender/QRenderSettings>
#include <Qt3DRender/QRenderSurfaceSelector>
#include <Qt3DRender/QViewport>

#include <QtCore/QConcatenateTablesProxyModel>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSettings>
#include <QtCore/QStringListModel>

#include <QtGui/QScreen>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCompleter>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

#include <QLabel>

namespace QtCSG::Inspector {

Q_NAMESPACE

namespace {

const auto s_settings_recentFileList = "RecentFiles";

Q_LOGGING_CATEGORY(lcInspector, "qtcsg.inspector");

using namespace Qt3DRender;
using Qt3DCore::QEntity;
using Qt3DCore::QNode;
using Qt3DExtras::Qt3DWindow;

using GeometryOperation = Geometry (*)(Geometry, Geometry, Options);

auto recentFileNames()
{
    return QSettings{}.value(s_settings_recentFileList).toStringList();
}

auto exampleExpressions()
{
    return QStringList {
        "cube()",
        "cube(center=[1,1,1], r=1)",
        "cylinder()",
        "sphere()",
        "sphere(r=1.3)",
    };
}

void applyGeometry(QEntity *entity, Geometry geometry)
{
    for (const auto meshList = entity->componentsOfType<Qt3DCSG::Mesh>();
         const auto mesh: meshList)
        entity->removeComponent(mesh);

    entity->addComponent(new Qt3DCSG::Mesh{std::move(geometry), entity});
}

QRenderSurfaceSelector *makeSurfaceSelector(QFrameGraphNode *parentNode)
{
    const auto surfaceSelector = new QRenderSurfaceSelector{parentNode};

    const auto clearBuffers = new QClearBuffers{surfaceSelector};
    clearBuffers->setBuffers(QClearBuffers::ColorDepthBuffer);
    clearBuffers->setClearColor(QRgb{0x4d'4d'4f});

    // ensure entities only are drawn by the smaller viewports
    const auto noDraw = new QNoDraw{clearBuffers};
    noDraw->setEnabled(true);

    return surfaceSelector;
}

void setupCamera(QCamera *camera, QSizeF ratio = {1, 1})
{
    const auto w = ratio.width() * 16.0f;
    const auto h = ratio.height() * 9.0f;

    camera->lens()->setPerspectiveProjection(45.0f, w/h, 0.1f, 1000.0f);
    camera->setPosition({0.0f, 3.5f, 3.5f}); // FIXME: distance from geometry
    camera->setUpVector({0, 1, 0});
    camera->setViewCenter({0, 0, 0});

    const auto cameraController = new Demo::OrbitCameraController{camera};
    cameraController->setCamera(camera);
}

QCamera *makeCamera(QSizeF ratio, QEntity *parentEntity)
{
    const auto camera = new QCamera{parentEntity};
    setupCamera(camera, std::move(ratio));
    return camera;
}

void inspectNode(int currentStep, Node node)
{
    const auto planeNormal = node.plane().normal();

    if (planeNormal.lengthSquared() != 1) {
        qCWarning(lcInspector).nospace()
            << "clipping step " << currentStep
            << ": node has bad plane normal " << planeNormal;
    }

    const auto polygons = node.polygons();
    for (auto i = 0; i < polygons.count(); ++i) {
        const auto vertices = polygons[i].vertices();

        for (auto j = 0, k = 1, l = 2; l < vertices.count(); j = k, k = l, ++l) {
            const auto a = vertices[j].position();
            const auto b = vertices[k].position();
            const auto c = vertices[j].position();
            const auto n = normalVector(a, b, c);

            if (n != planeNormal) {
                qCWarning(lcInspector).nospace()
                    << "clipping step " << currentStep
                    << ": bad normal for polygon " << i
                    << ", vertex [" << j << '/' << k << '/' << l
                    << "]; actual: " << n << ", expected: " << planeNormal;
            }
        }
    }

    // TODO
    // - show node's plane
    // - highlight node's polygons
    // - validate polygons
    // - allow loading of crashing geometries
    // - drop degenerated polygons during build
    // - identify (and maybe) fix reason for degeneration
}

struct GeometryView
{
    GeometryView(QString name, QFrameGraphNode *frameGraph, QEntity *parentEntity);

    QViewport               *const viewport;
    QCameraSelector         *const cameraSelector;

    QEntity                 *const entity;
    Demo::WireframeMaterial *const material;
    QLayer                  *const layer;

    Geometry geometry;
};

enum MetaEvent { None, Any };
Q_ENUM_NS(MetaEvent)

class Application
    : private AppSupport::StaticInit<Application>
    , public QApplication
{
    friend class StaticInit;

public:
    using QApplication::QApplication;

    int run();

private:
    struct InspectionMode : public AppSupport::MultiEnum<InspectionMode,
                                                         Inspection::Event, MetaEvent>
    {
        using MultiEnum::MultiEnum;
        using enum Inspection::Event;
        using enum MetaEvent;
    };

    static void staticInit();

    auto makeSelectFile(QLineEdit *lineEdit);
    auto makeLoadGeometry(GeometryView *view, QLineEdit *lineEdit);

    void addRecentExpression(QString fileName);

    void updateGeometry();
    void updateDebugGeometry();
    void updateDebugStepsLabel();

    Qt3DWindow *setupStage();
    void setupWidgets(Qt3DWindow *stage);

    auto geometryViews() const
    {
        return std::array{&m_left, &m_right, &m_result, &m_debug};
    }

    QStringListModel *const m_exampleExpressions = new QStringListModel{exampleExpressions(), this};
    QStringListModel *const m_recentExpressions  = new QStringListModel{recentFileNames(), this};

    QEntity                *const m_rootEntity      = new QEntity;
    QEntity                *const m_lightEntity     = new QEntity{m_rootEntity};
    QTechniqueFilter       *const m_frameGraph      = new QTechniqueFilter{m_rootEntity};
    QRenderSurfaceSelector *const m_surfaceSelector = makeSurfaceSelector(m_frameGraph);

    GeometryView m_left   = {"left",   m_surfaceSelector, m_rootEntity};
    GeometryView m_right  = {"right",  m_surfaceSelector, m_rootEntity};
    GeometryView m_result = {"result", m_surfaceSelector, m_rootEntity};
    GeometryView m_debug  = {"debug",  m_surfaceSelector, m_rootEntity};

    QWidget   *const m_window           = new QWidget;
    QComboBox *const m_operationBox     = new QComboBox{m_window};
    QLabel    *const m_debugStepsLabel  = new QLabel{m_window};
    QSlider   *const m_debugStepsSlider = new QSlider{m_window};
    QComboBox *const m_debugModeBox     = new QComboBox{m_window};

    std::map<InspectionMode, int> m_operationCounters;
};

auto makeNegateEnabledState(QNode *node)
{
    return [node](bool enabled) {
        node->setEnabled(!enabled);
    };
}

auto makeToggleWireframe(Demo::WireframeMaterial *material)
{
    return [material](bool enabled) {
        material->setFrontLineWidth(enabled ? 0.5 : 0.0);
        material->setBackLineWidth(enabled ? 0.5 : 0.0);
    };
}

auto Application::makeSelectFile(QLineEdit *lineEdit)
{
    return [lineEdit] {
        const auto initialFocusWidget = lineEdit->topLevelWidget()->focusWidget();
        auto fileName = QFileDialog::getOpenFileName(lineEdit->topLevelWidget(), {}, {},
                                                     tr("OFF Files (*.off)"));

        if (fileName.isEmpty())
            return;

        lineEdit->setText(std::move(fileName));
        emit lineEdit->editingFinished();

        if (dynamic_cast<QLineEdit *>(initialFocusWidget)) {
            lineEdit->selectAll();
            lineEdit->setFocus();
        }
    };
}

auto Application::makeLoadGeometry(GeometryView *view, QLineEdit *lineEdit)
{
    return [this, view, lineEdit] {
        auto expression = lineEdit->text();
        auto geometry = QtCSG::parseGeometry(expression);

        if (geometry.error() != Error::NoError)
            geometry = QtCSG::readGeometry(expression);

        if (Utils::reportError(lcInspector(), geometry.error(), "Could not load geometry"))
            return;
        if (!m_exampleExpressions->stringList().contains(expression))
            addRecentExpression(std::move(expression));

        applyGeometry(view->entity, geometry);
        view->geometry = std::move(geometry);
        updateGeometry();
    };
}

auto makeGeometryOperation(GeometryOperation operation)
{
    return QVariant::fromValue(operation);
}

void Application::addRecentExpression(QString fileName)
{
    const auto row = m_recentExpressions->stringList().indexOf(fileName);

    if (row > 0)
        m_recentExpressions->removeRow(row);

    if (row != 0) {
        m_recentExpressions->insertRow(0);
        m_recentExpressions->setData(m_recentExpressions->index(0), fileName);
    }

    QSettings{}.setValue(s_settings_recentFileList, m_recentExpressions->stringList());
}

void Application::updateGeometry()
{
    const auto operation = qvariant_cast<GeometryOperation>(m_operationBox->currentData());
    const auto debugMode = qvariant_cast<InspectionMode>(m_debugModeBox->currentData());

    if (operation == nullptr) {
        qCWarning(lcInspector, "No valid operation selected");
        return;
    }

    m_operationCounters.clear();

    auto inspectionHandler = [this](Inspection::Event operation, std::any) {
        ++m_operationCounters[InspectionMode::Any];
        ++m_operationCounters[operation];
        return Inspection::Result::Proceed;
    };

    auto geometry = operation(m_left.geometry, m_right.geometry,
                              Options{std::move(inspectionHandler)});

    m_debugStepsSlider->setEnabled(debugMode != InspectionMode::None);
    m_debugStepsSlider->setRange(0, m_operationCounters[debugMode]);
    m_debugStepsSlider->setValue(m_debugStepsSlider->maximum());

    applyGeometry(m_result.entity, std::move(geometry));
    updateDebugGeometry();
}

void Application::updateDebugGeometry()
{
    const auto operation = qvariant_cast<GeometryOperation>(m_operationBox->currentData());
    const auto mode = qvariant_cast<InspectionMode>(m_debugModeBox->currentData());

    if (operation == nullptr) {
        qCWarning(lcInspector, "No valid operation selected");
        return;
    }

    const auto lastStep = m_debugStepsSlider->value();

    m_operationCounters.clear();

    auto inspectionHandler = [this, mode, lastStep](Inspection::Event operation, std::any detail) {
        ++m_operationCounters[InspectionMode::Any];
        ++m_operationCounters[operation];

        if (mode == InspectionMode::Any || mode == operation) {
            const auto currentStep = m_operationCounters[mode];

            if (currentStep > lastStep)
                return Inspection::Result::Abort;

            if (currentStep == lastStep && mode == InspectionMode::Clip)
                inspectNode(currentStep, std::any_cast<Node>(detail));
        }

        return Inspection::Result::Proceed;
    };

    auto geometry = operation(m_left.geometry, m_right.geometry,
                              Options{std::move(inspectionHandler)});

    applyGeometry(m_debug.entity, std::move(geometry));
}

void Application::updateDebugStepsLabel()
{
    const auto current = QString::number(m_debugStepsSlider->value());
    const auto maximum = QString::number(m_debugStepsSlider->maximum());
    m_debugStepsLabel->setText(current + '/' + maximum);
}

GeometryView::GeometryView(QString name, QFrameGraphNode *frameGraph, QEntity *parentEntity)
    //: start{new QNoDraw{graph}}
    : viewport{new QViewport{frameGraph}}
    , cameraSelector{new QCameraSelector{viewport}}
    , entity{new QEntity{parentEntity}}
    , material{new Demo::WireframeMaterial{entity}}
    , layer{new QLayer{entity}}
{
    viewport->setObjectName(name);

    const auto filter = new QLayerFilter{cameraSelector};
    filter->setFilterMode(QLayerFilter::AcceptAnyMatchingLayers);
    filter->addLayer(layer);

    const auto noDraw = new QNoDraw{filter};
    noDraw->setEnabled(false);

    // ensure disabling the camera selector really stops rendering
    QObject::connect(cameraSelector, &QCameraSelector::enabledChanged,
                     noDraw, makeNegateEnabledState(noDraw));

    material->setAmbient(Qt::transparent);
    material->setSpecular(Qt::transparent);
    material->setDiffuse(QRgb{0xff'dd'ff'ee});
    material->setFrontLineWidth(0.5f);

    entity->addComponent(material);
    entity->addComponent(layer);
}

Qt3DWindow *Application::setupStage()
{
    const auto stage = new Qt3DWindow;
    stage->setActiveFrameGraph(m_frameGraph);
    stage->setRootEntity(m_rootEntity);

    m_surfaceSelector->setSurface(stage);

    // camera
    setupCamera(stage->camera());

    // lighting
    const auto light = new Qt3DRender::QPointLight{m_lightEntity};
    light->setColor(Qt::white);
    light->setIntensity(1.0f);

    const auto lightTransform = new Qt3DCore::QTransform{m_lightEntity};
    lightTransform->setTranslation(stage->camera()->position());

    for (const auto view: geometryViews())
        m_lightEntity->addComponent(view->layer);

    m_lightEntity->addComponent(light);
    m_lightEntity->addComponent(lightTransform);

    connect(stage->camera(), &Qt3DRender::QCamera::positionChanged,
            lightTransform, &Qt3DCore::QTransform::setTranslation);

    for (auto i = 0U; i < geometryViews().size(); ++i) {
        const auto view = geometryViews()[i];

        const auto x = i < 3 ? 0.0f : 1.0f/3.0f;
        const auto y = i < 3 ? i/3.0f : 0.0f;
        const auto w = i < 3 ? 1.0/3.0f : 1.0 - 1.0/3.0f;
        const auto h = i < 3 ? 1.0/3.0f : 1.0;

        view->viewport->setNormalizedRect({x, y, w, h}); // FIXME: move into GeometryView

        if (view == &m_debug) {
            const auto camera = makeCamera({2, 3}, m_rootEntity);
            view->cameraSelector->setCamera(camera);
        } else {
            view->cameraSelector->setCamera(stage->camera());
        }
    }

    return stage;
}

void Application::setupWidgets(Qt3DWindow *stage)
{
    // main window
    m_window->setWindowTitle(tr("QtCSG Inspector"));

    const auto completionModel = new QConcatenateTablesProxyModel{this};
    completionModel->addSourceModel(m_exampleExpressions);
    completionModel->addSourceModel(m_recentExpressions);

    const auto completer = new QCompleter{completionModel, this};
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);

    m_operationBox->addItem(tr("Union"),        makeGeometryOperation(QtCSG::merge));
    m_operationBox->addItem(tr("Difference"),   makeGeometryOperation(QtCSG::subtract));
    m_operationBox->addItem(tr("Intersection"), makeGeometryOperation(QtCSG::intersect));
    m_operationBox->setCurrentIndex(1);

    connect(m_operationBox, &QComboBox::currentIndexChanged,
            this, &Application::updateGeometry);

    const auto leftExpressionEdit = new QLineEdit{m_window};
    leftExpressionEdit->setClearButtonEnabled(true);
    leftExpressionEdit->setCompleter(completer);
    leftExpressionEdit->setText("cube()");

    const auto rightExpressionEdit = new QLineEdit{m_window};
    rightExpressionEdit->setCompleter(completer);
    rightExpressionEdit->setText("sphere(r=1.3)");

    const auto loadLeftGeometry = makeLoadGeometry(&m_left, leftExpressionEdit);
    const auto loadRightGeometry = makeLoadGeometry(&m_right, rightExpressionEdit);

    connect(leftExpressionEdit, &QLineEdit::editingFinished, this, loadLeftGeometry);
    connect(rightExpressionEdit, &QLineEdit::editingFinished, this, loadRightGeometry);

    const auto leftFileNameButton = new QPushButton{tr("Browse"), m_window};
    leftFileNameButton->setFocusPolicy(Qt::FocusPolicy::TabFocus);

    const auto rightFileNameButton = new QPushButton{tr("Browse"), m_window};
    rightFileNameButton->setFocusPolicy(Qt::FocusPolicy::TabFocus);

    connect(leftFileNameButton, &QPushButton::clicked,
            this, makeSelectFile(leftExpressionEdit));
    connect(rightFileNameButton, &QPushButton::clicked,
            this, makeSelectFile(rightExpressionEdit));

    const auto wireframeBox = new QCheckBox{tr("Show &Wireframes"), m_window};
    wireframeBox->setChecked(true);

    for (const auto view: geometryViews()) {
        auto toggleWireframe = makeToggleWireframe(view->material);
        connect(wireframeBox, &QCheckBox::toggled, view->material, toggleWireframe);
        toggleWireframe(wireframeBox->isEnabled());
    }

    m_debugStepsLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_debugStepsSlider->setOrientation(Qt::Horizontal);

    connect(m_debugStepsSlider, &QSlider::rangeChanged,
            this, &Application::updateDebugStepsLabel);
    connect(m_debugStepsSlider, &QSlider::valueChanged,
            this, &Application::updateDebugStepsLabel);
    connect(m_debugStepsSlider, &QSlider::valueChanged,
            this, &Application::updateDebugGeometry);

    m_debugModeBox->addItem(tr("None"),     InspectionMode{InspectionMode::None});
    m_debugModeBox->addItem(tr("Build"),    InspectionMode{InspectionMode::Build});
    m_debugModeBox->addItem(tr("Invert"),   InspectionMode{InspectionMode::Invert});
    m_debugModeBox->addItem(tr("Clip"),     InspectionMode{InspectionMode::Clip});
    m_debugModeBox->addItem(tr("Any"),      InspectionMode{InspectionMode::Any});

    connect(m_debugModeBox, &QComboBox::currentIndexChanged,
            this, &Application::updateGeometry);

    const auto bottomToolbar = new QHBoxLayout;

    bottomToolbar->addWidget(wireframeBox);
    bottomToolbar->addSpacing(20);

    for (const auto view: geometryViews()) {
        const auto checkBox = new QCheckBox{view->viewport->objectName(), m_window};
        checkBox->setChecked(view->cameraSelector->isEnabled());
        bottomToolbar->addWidget(checkBox);

        connect(checkBox, &QCheckBox::toggled, view->cameraSelector, &QCameraSelector::setEnabled);
    }

    bottomToolbar->addStretch(1);
    bottomToolbar->addSpacing(20);

    bottomToolbar->addWidget(m_debugStepsLabel);
    bottomToolbar->addWidget(m_debugStepsSlider, 5);
    bottomToolbar->addWidget(m_debugModeBox);

    const auto container = QWidget::createWindowContainer(stage);
    const auto screenSize = stage->screen()->size();
    container->setMinimumSize({200, 100});
    container->setMaximumSize(screenSize);

    const auto layout = new QGridLayout{m_window};
    layout->addWidget(m_operationBox, 0, 0);
    layout->addWidget(leftExpressionEdit, 0, 1);
    layout->addWidget(leftFileNameButton, 0, 2);
    layout->addWidget(rightExpressionEdit, 0, 3);
    layout->addWidget(rightFileNameButton, 0, 4);
    layout->addWidget(container, 1, 0, 1, 5);
    layout->addLayout(bottomToolbar, 2, 0, 1, 5);

    const auto windowSize = QSize{1200, 800};
    const auto position = AppSupport::toPoint((screenSize - windowSize) / 2);
    m_window->setGeometry({position, windowSize});
    m_window->show();

    container->setFocus();

    loadLeftGeometry();
    loadRightGeometry();
}

void Application::staticInit()
{
    static_assert(InspectionMode{InspectionMode::Clip}.index() == 0);
    static_assert(InspectionMode{InspectionMode::None}.index() == 1);
    static_assert(InspectionMode{InspectionMode::Build} == Application::InspectionMode::Build);

    setOrganizationDomain("taschenorakel.de");
    Utils::enabledColorfulLogging();

    // Force Qt3D OpenGL renderer
    constexpr auto rendererVariable = "QT3D_RENDERER";
    if (!qEnvironmentVariableIsSet(rendererVariable))
        qputenv(rendererVariable, "opengl");

#if QT_VERSION_MAJOR < 6
    setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
}

int Application::run()
{
    const auto stage = setupStage();
    setupWidgets(stage);
    return exec();
}

} // namespace
} // namespace QtCSG::Inspector

int main(int argc, char *argv[])
{
    return QtCSG::Inspector::Application{argc, argv}.run();
}

#include "qtcsginspector.moc"
