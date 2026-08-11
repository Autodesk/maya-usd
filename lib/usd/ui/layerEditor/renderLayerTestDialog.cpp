//
// Copyright 2026 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "renderLayerTestDialog.h"

#if defined(ADSK_ABI) && ADSK_ABI >= 2027

#include <mayaUsd/ufe/ProxyShapeHandler.h>

#include <pxr/base/gf/vec3d.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <pxr/usd/usdLux/distantLight.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <AdskUsdRenderSetup/RenderLayerManager.h>
#include <AdskUsdRenderSetup/RenderSetupUtils.h>

#include <QtCore/QTimer>
#include <QtGui/QFont>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStyle>
#include <QtWidgets/QVBoxLayout>

#include <set>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

RenderLayerTestDialog::RenderLayerTestDialog(QWidget* in_parent)
    : QDialog(in_parent)
{
    setWindowTitle("Render Layer Test");
    resize(620, 420);

    auto* mainLayout = new QVBoxLayout(this);

    auto* createSetupButton = new QPushButton("Create Setup", this);
    mainLayout->addWidget(createSetupButton);

    auto* stageRow = new QHBoxLayout();
    stageRow->addWidget(new QLabel("Stage", this));
    _stageLabel = new QLabel(this);
    _stageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    stageRow->addWidget(_stageLabel, 1);
    mainLayout->addLayout(stageRow);

    auto* nameRow = new QHBoxLayout();
    nameRow->addWidget(new QLabel("Layer name", this));
    _nameField = new QLineEdit(this);
    _nameField->setPlaceholderText("optional - blank allocates the next free RenderLayer_N");
    nameRow->addWidget(_nameField);
    mainLayout->addLayout(nameRow);

    auto* createLayerButton = new QPushButton("Create Render Layer", this);
    mainLayout->addWidget(createLayerButton);

    mainLayout->addWidget(new QLabel(
        "Render layers   -   check = active, disk = tracked   -   double-click to activate / "
        "deactivate",
        this));

    _layerList = new QListWidget(this);
    mainLayout->addWidget(_layerList);

    auto* refreshButton = new QPushButton("Refresh", this);
    mainLayout->addWidget(refreshButton);

    connect(createSetupButton, &QPushButton::clicked, this, [this]() { createSetup(); });
    connect(createLayerButton, &QPushButton::clicked, this, [this]() { createRenderLayer(); });
    connect(refreshButton, &QPushButton::clicked, this, [this]() { refreshLayers(); });
    connect(_layerList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        toggleActiveRenderLayer(item);
    });

    _dirtinessNoticeKey = PXR_NS::TfNotice::Register(
        PXR_NS::TfCreateWeakPtr(this), &RenderLayerTestDialog::onLayerDirtinessChanged);
}

RenderLayerTestDialog::~RenderLayerTestDialog()
{
    PXR_NS::TfNotice::Revoke(_dirtinessNoticeKey);
}

void RenderLayerTestDialog::onLayerDirtinessChanged(
    const PXR_NS::SdfNotice::LayerDirtinessChanged&)
{
    refreshLayersOnIdle();
}

void RenderLayerTestDialog::refreshLayersOnIdle()
{
    // Dirtiness changes arrive in bursts and are delivered during USD's own notification,
    // so rebuild once on the event loop rather than inside the notice.
    if (_refreshPending) {
        return;
    }
    _refreshPending = true;

    QTimer::singleShot(0, this, [this]() {
        _refreshPending = false;
        if (!isVisible()) {
            return;
        }
        // An automatic refresh should stay silent, so skip rather than let stage() warn.
        if (MayaUsd::ufe::ProxyShapeHandler::getAllStages().empty()) {
            return;
        }
        refreshLayers();
    });
}

UsdStageRefPtr RenderLayerTestDialog::stage() const
{
    // First proxy shape wins. Create Setup makes one, and a test scene is not expected to
    // hold several; the stage label shows which one the buttons are acting on.
    for (const auto& candidate : MayaUsd::ufe::ProxyShapeHandler::getAllStages()) {
        if (candidate) {
            return candidate;
        }
    }

    MGlobal::displayWarning("No USD stage in the scene. Use Create Setup first.");
    return nullptr;
}

void RenderLayerTestDialog::createSetup()
{
    MString transform;
    MGlobal::executeCommand("createNode transform -name renderLayerTest", transform);

    MString shapeCommand("createNode mayaUsdProxyShape -name renderLayerTestShape -parent ");
    shapeCommand += transform;
    MString shape;
    MGlobal::executeCommand(shapeCommand, shape);

    MString connectCommand("connectAttr time1.outTime ");
    connectCommand += shape;
    connectCommand += ".time";
    MGlobal::executeCommand(connectCommand);

    UsdStageRefPtr theStage = stage();
    if (!theStage) {
        return;
    }

    // Something for render layers to override.
    auto world = UsdGeomXform::Define(theStage, SdfPath("/World"));
    theStage->SetDefaultPrim(world.GetPrim());
    UsdGeomSphere::Define(theStage, SdfPath("/World/Sphere"));

    auto cube = UsdGeomCube::Define(theStage, SdfPath("/World/Cube"));
    UsdGeomXformCommonAPI(cube).SetTranslate(GfVec3d(3.0, 0.0, 0.0));

    UsdLuxDistantLight::Define(theStage, SdfPath("/World/Light"));

    // Seeds /Render and marks the primary settings prim active, so a render layer has a
    // render pass to bind to.
    AdskUsdRenderSetup::RenderSetupUtils::AuthorDefaultRenderSettings(theStage);

    refreshLayers();
}

void RenderLayerTestDialog::createRenderLayer()
{
    UsdStageRefPtr theStage = stage();
    if (!theStage) {
        return;
    }

    // Empty name lets the manager allocate the next free RenderLayer_N.
    const std::string name = _nameField->text().trimmed().toStdString();

    auto& manager = AdskUsdRenderSetup::RenderLayerManager::instance();
    if (manager.createRenderLayer(theStage, name).empty()) {
        MString message("Could not create render layer");
        if (!name.empty()) {
            message += " '";
            message += name.c_str();
            message += "'";
        }
        message += ".";
        MGlobal::displayWarning(message);
        return;
    }

    _nameField->clear();
    refreshLayers();
}

void RenderLayerTestDialog::refreshLayers()
{
    _layerList->clear();

    UsdStageRefPtr theStage = stage();
    if (!theStage) {
        _stageLabel->setText(QStringLiteral("<none>"));
        return;
    }

    _stageLabel->setText(
        QString::fromStdString(theStage->GetRootLayer()->GetIdentifier()));

    auto& manager = AdskUsdRenderSetup::RenderLayerManager::instance();

    const auto        active = manager.getActiveRenderLayer(theStage);
    const std::string activeName = active ? active->name : std::string();

    std::set<std::string> trackedNames;
    for (const auto& info : manager.trackedRenderLayers(theStage)) {
        trackedNames.insert(info.name);
    }

    for (const auto& info : manager.getRenderLayers(theStage)) {
        // "tracked" means the manager retains the handle because the layer is anonymous
        // or dirty, which is exactly the set the save flow has to ask about.
        // "tracked" means the manager retains the handle because the layer is anonymous
        // or dirty, which is exactly the set the save flow has to ask about.
        const bool isActive = (info.name == activeName);
        const bool isTracked = (trackedNames.count(info.name) > 0);

        const QString renderPass = info.renderPassPath.IsEmpty()
            ? QStringLiteral("<none>")
            : QString::fromStdString(info.renderPassPath.GetString());

        auto* item = new QListWidgetItem();
        // The row is a widget, so carry the registry key for the double-click.
        item->setData(Qt::UserRole, QString::fromStdString(info.name));
        item->setToolTip(QStringLiteral("%1\n%2")
                             .arg(isActive ? "Active" : "Not active")
                             .arg(isTracked ? "Tracked (anonymous or dirty)" : "Not tracked"));

        auto* row = createLayerRow(
            isActive,
            isTracked,
            QStringLiteral("%1  |  %2  |  pass: %3")
                .arg(QString::fromStdString(info.name))
                .arg(QString::fromStdString(info.identifier))
                .arg(renderPass));

        item->setSizeHint(row->sizeHint());
        _layerList->addItem(item);
        _layerList->setItemWidget(item, row);
    }
}

QWidget* RenderLayerTestDialog::createLayerRow(
    bool           isActive,
    bool           isTracked,
    const QString& description)
{
    const int iconSize = 16;

    auto* row = new QWidget();
    // Without this the row widget swallows the clicks and the view never emits
    // itemDoubleClicked.
    row->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(4, 2, 4, 2);
    rowLayout->setSpacing(6);

    // An off state still gets a fixed-size label so the description stays aligned.
    auto addIcon = [&](bool on, QStyle::StandardPixmap pixmap) {
        auto* iconLabel = new QLabel(row);
        iconLabel->setFixedSize(iconSize, iconSize);
        if (on) {
            iconLabel->setPixmap(style()->standardIcon(pixmap).pixmap(iconSize, iconSize));
        }
        rowLayout->addWidget(iconLabel);
    };

    addIcon(isActive, QStyle::SP_DialogApplyButton);
    addIcon(isTracked, QStyle::SP_DialogSaveButton);

    auto* descriptionLabel = new QLabel(description, row);
    // Maya specifies its fonts in pixels, and pointSize() reports -1 for those. Adding to
    // it would ask for a 1 point font, which renders as an unreadable row of marks.
    QFont font = descriptionLabel->font();
    if (font.pointSize() > 0) {
        font.setPointSize(font.pointSize() + 2);
    } else if (font.pixelSize() > 0) {
        font.setPixelSize(font.pixelSize() + 3);
    }
    descriptionLabel->setFont(font);
    rowLayout->addWidget(descriptionLabel, 1);

    return row;
}

void RenderLayerTestDialog::toggleActiveRenderLayer(QListWidgetItem* in_item)
{
    if (!in_item) {
        return;
    }

    UsdStageRefPtr theStage = stage();
    if (!theStage) {
        return;
    }

    const std::string name = in_item->data(Qt::UserRole).toString().toStdString();

    auto&      manager = AdskUsdRenderSetup::RenderLayerManager::instance();
    const auto active = manager.getActiveRenderLayer(theStage);

    // Empty name deactivates to base composition, so re-activating the current layer
    // toggles it off.
    const bool alreadyActive = active && active->name == name;
    manager.setActiveRenderLayer(theStage, alreadyActive ? std::string() : name);

    // Target whatever is now active so edits land in the render layer. Only the active
    // layer is composed into the stack, which is what makes it a legal edit target;
    // deactivating puts the target back on the root layer.
    if (const auto nowActive = manager.getActiveRenderLayer(theStage)) {
        if (auto activeLayer = PXR_NS::SdfLayer::Find(nowActive->identifier)) {
            theStage->SetEditTarget(activeLayer);
        }
    } else {
        theStage->SetEditTarget(theStage->GetRootLayer());
    }

    refreshLayers();
}

} // namespace UsdLayerEditor

#endif // ADSK_ABI >= 2027
