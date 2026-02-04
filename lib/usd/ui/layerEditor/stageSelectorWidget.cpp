//
// Copyright 2020 Autodesk
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
#include "stageSelectorWidget.h"

#include "qtUtils.h"
#include "sessionState.h"
#include "stringResources.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/common.h>

#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MUuid.h>
#include <ufe/globalSelection.h>
#include <ufe/hierarchy.h>
#include <ufe/observableSelection.h>
#include <ufe/selection.h>
#include <ufe/selectionNotification.h>

#include <QtCore/QSignalBlocker>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

Q_DECLARE_METATYPE(UsdLayerEditor::SessionState::StageEntry);

namespace {

int getEntryIndexById(
    const std::string&                                           id,
    std::vector<UsdLayerEditor::SessionState::StageEntry> const& stages)
{
    auto it = std::find_if(
        stages.begin(), stages.end(), [&id](UsdLayerEditor::SessionState::StageEntry stageEntry) {
            return (id == stageEntry._id);
        });

    if (it != stages.end()) {
        return std::distance(stages.begin(), it);
    }

    return -1;
}

int getEntryIndexById(
    UsdLayerEditor::SessionState::StageEntry const&              entry,
    std::vector<UsdLayerEditor::SessionState::StageEntry> const& stages)
{
    return getEntryIndexById(entry._id, stages);
}

bool loadStagePinnedOption()
{
    const MString optionName = PXR_NS::MayaUsdOptionVars->PinLayerEditorStage.GetText();
    return MGlobal::optionVarExists(optionName) && MGlobal::optionVarIntValue(optionName) != 0;
}

void saveStagePinnedOption(bool isPinned)
{
    const MString optionName = PXR_NS::MayaUsdOptionVars->PinLayerEditorStage.GetText();
    MGlobal::setOptionVarValue(optionName, isPinned ? 1 : 0);
}

//----------------------------------------------------------------------------------------------------------------------

// Observe the global UFE selection to update the stage selector widgets.
class StageSelectorSelectionObserver
    : public Ufe::Observer
    , public std::enable_shared_from_this<StageSelectorSelectionObserver>
{
public:
    using StageSelectorWidget = UsdLayerEditor::StageSelectorWidget;

    StageSelectorSelectionObserver();
    ~StageSelectorSelectionObserver() override;

    static const std::shared_ptr<StageSelectorSelectionObserver>& instance();

    void operator()(const Ufe::Notification& notification) override;

    void addStageSelector(StageSelectorWidget& selector);
    void removeStageSelector(StageSelectorWidget& selector);

private:
    std::set<StageSelectorWidget*> _stageSelectors;
};

// Automatically register and unretsgiter a selection observer.
class AutoObserveSelection
{
public:
    AutoObserveSelection(const std::shared_ptr<StageSelectorSelectionObserver>& observer)
        : _observer(observer)
    {
        if (!_observer)
            return;

        const Ufe::GlobalSelection::Ptr& ufeSelection = Ufe::GlobalSelection::get();
        if (!ufeSelection)
            return;

        ufeSelection->addObserver(_observer);
    }

    ~AutoObserveSelection()
    {
        if (!_observer)
            return;

        const Ufe::GlobalSelection::Ptr& ufeSelection = Ufe::GlobalSelection::get();
        if (!ufeSelection)
            return;

        ufeSelection->removeObserver(_observer);
    }

private:
    std::shared_ptr<StageSelectorSelectionObserver> _observer;
};

StageSelectorSelectionObserver::StageSelectorSelectionObserver() { }

StageSelectorSelectionObserver::~StageSelectorSelectionObserver() { }

// static
const std::shared_ptr<StageSelectorSelectionObserver>& StageSelectorSelectionObserver::instance()
{
    static auto                 observer = std::make_shared<StageSelectorSelectionObserver>();
    static AutoObserveSelection autoObservation(observer);
    return observer;
}

void StageSelectorSelectionObserver::operator()(const Ufe::Notification& notification)
{
    auto selectionChanged = dynamic_cast<const Ufe::SelectionChanged*>(&notification);
    if (selectionChanged == nullptr) {
        return;
    }

    for (auto& selector : _stageSelectors)
        selector->selectionChanged();
}

void StageSelectorSelectionObserver::addStageSelector(StageSelectorWidget& selector)
{
    _stageSelectors.insert(&selector);
}

void StageSelectorSelectionObserver::removeStageSelector(StageSelectorWidget& selector)
{
    _stageSelectors.erase(&selector);
}

} // namespace

namespace UsdLayerEditor {

StageSelectorWidget::StageSelectorWidget(SessionState* in_sessionState, QWidget* in_parent)
    : QWidget(in_parent)
{
    createUI();
    setSessionState(in_sessionState);
    StageSelectorSelectionObserver::instance()->addStageSelector(*this);

    _pinStageSelection = loadStagePinnedOption();
    updatePinnedStage();
    updateContentButton();
}

StageSelectorWidget::~StageSelectorWidget()
{
    StageSelectorSelectionObserver::instance()->removeStageSelector(*this);
}

void StageSelectorWidget::createUI()
{
    auto mainHLayout = new QHBoxLayout();
    auto spacing = DPIScale(4);
    auto margin = DPIScale(0);
    QtUtils::initLayoutMargins(mainHLayout);
    mainHLayout->setSpacing(spacing);
    mainHLayout->setContentsMargins(margin, 0, 0, 0);
    mainHLayout->addWidget(QtUtils::fixedWidget(
        new QLabel(StringResources::getAsQString(StringResources::kUsdStage))));

    _dropDown = new QComboBox();
    mainHLayout->addWidget(_dropDown, 1);
    connect(
        _dropDown,
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this,
        &StageSelectorWidget::selectedIndexChanged);

    auto higButtonYOffset = DPIScale(4);
    auto buttonSize = DPIScale(24);
    _pinStage = new QPushButton();
    _pinStage->move(0, higButtonYOffset);
    QtUtils::setupButtonWithHIGBitmaps(_pinStage, ":/UsdLayerEditor/pin_on");
    _pinStage->setFixedSize(buttonSize, buttonSize);
    _pinStage->setToolTip(StringResources::getAsQString(StringResources::kPinUsdStageTooltip));
    mainHLayout->addWidget(_pinStage, 0, Qt::AlignLeft | Qt::AlignRight);
    connect(_pinStage, &QAbstractButton::clicked, this, &StageSelectorWidget::stagePinClicked);

    _collapseContent = new QPushButton();
    _collapseContent->move(0, higButtonYOffset);
    QtUtils::setupButtonWithHIGBitmaps(_collapseContent, ":/UsdLayerEditor/contents_on");
    _collapseContent->setFixedSize(buttonSize, buttonSize);
    _collapseContent->setToolTip(
        StringResources::getAsQString(StringResources::kDisplayLayerContents));
    mainHLayout->addWidget(_collapseContent, 0, Qt::AlignLeft | Qt::AlignRight);
    connect(
        _collapseContent,
        &QAbstractButton::clicked,
        this,
        &StageSelectorWidget::collapseContentClicked);

    setLayout(mainHLayout);
}

void StageSelectorWidget::setSessionState(SessionState* in_sessionState)
{
    _sessionState = in_sessionState;
    connect(
        _sessionState,
        &SessionState::stageListChangedSignal,
        this,
        &StageSelectorWidget::updateFromSessionState);
    connect(
        _sessionState,
        &SessionState::currentStageChangedSignal,
        this,
        &StageSelectorWidget::sessionStageChanged);
    connect(
        _sessionState, &SessionState::stageRenamedSignal, this, &StageSelectorWidget::stageRenamed);
    connect(_sessionState, &SessionState::stageResetSignal, this, &StageSelectorWidget::stageReset);

    connect(
        in_sessionState,
        &SessionState::showDisplayLayerContents,
        this,
        &StageSelectorWidget::updateContentButton);

    updateFromSessionState(_sessionState->stageEntry());
}

SessionState::StageEntry const StageSelectorWidget::selectedStage()
{
    if (_dropDown->currentIndex() != -1) {
        auto const& data = _dropDown->currentData();
        return data.value<SessionState::StageEntry>();
    }

    return SessionState::StageEntry();
}
// repopulates the combo based on the session stage list
void StageSelectorWidget::updateFromSessionState(SessionState::StageEntry const& entryToSelect)
{
    // Keep track of the current stage before we recreate the dropdown items
    auto     currentEntry = selectedStage();
    QVariant currentEntryQVariant;
    currentEntryQVariant.setValue(currentEntry);

    QSignalBlocker blocker(_dropDown);
    _dropDown->clear();
    auto allStages = _sessionState->allStages();
    for (auto const& stageEntry : allStages) {
        QVariant stageEntryQVariant;
        stageEntryQVariant.setValue(stageEntry);

        // Reuse same QVariant as in the currentEntry if we are processing
        // the same stages
        if (stageEntry._id == currentEntry._id) {
            stageEntryQVariant = currentEntryQVariant;
        }

        _dropDown->addItem(QString(stageEntry._displayName.c_str()), stageEntryQVariant);
    }

    // Either no entry was selected or we have a stage pinned
    if (!entryToSelect._stage || _pinStageSelection) {
        // If the current/previous entry is still part of the list, keep it set.
        int idx = _dropDown->findData(currentEntryQVariant);
        if (idx != -1) {
            _sessionState->setStageEntry(currentEntry);
            _dropDown->setCurrentIndex(idx);
        } else {
            _sessionState->setStageEntry(selectedStage());
        }
    } else {
        _sessionState->setStageEntry(entryToSelect);
    }
}

// called when combo value is changed by user
void StageSelectorWidget::selectedIndexChanged(int index)
{
    _internalChange = true;
    _sessionState->setStageEntry(selectedStage());
    _internalChange = false;
}

static MayaUsdProxyShapeBase* getChildProxyShape(const Ufe::SceneItem::Ptr& item)
{
    Ufe::Hierarchy::Ptr hierarchy = Ufe::Hierarchy::hierarchy(item);
    if (!hierarchy)
        return nullptr;

    const bool rebuildCacheIfNeeded = false;
    for (const auto& subItem : hierarchy->children()) {
        auto proxyShapePtr = MayaUsd::ufe::getProxyShape(subItem->path(), rebuildCacheIfNeeded);
        if (!proxyShapePtr)
            continue;

        return proxyShapePtr;
    }

    return nullptr;
}

void StageSelectorWidget::selectionChanged()
{
    // When the stage selection is pinned, don't follow the selection.
    if (_pinStageSelection)
        return;

    const Ufe::GlobalSelection::Ptr& ufeGlobalSelection = Ufe::GlobalSelection::get();
    if (!ufeGlobalSelection)
        return;

    // We will set the currently selected stage to be the stage of the first item
    // that is a USD item. So if multiple stages are selected, the first one wins.
    const Ufe::Selection& ufeSelection = *ufeGlobalSelection;
    const bool            rebuildCacheIfNeeded = false;
    for (const auto& item : ufeSelection) {
        auto proxyShapePtr = MayaUsd::ufe::getProxyShape(item->path(), rebuildCacheIfNeeded);
        if (!proxyShapePtr) {
            proxyShapePtr = getChildProxyShape(item);
            if (!proxyShapePtr) {
                continue;
            }
        }

        MFnDagNode        dagNode(proxyShapePtr->thisMObject());
        const std::string id = dagNode.uuid().asString().asChar();
        const int         index = getEntryIndexById(id, _sessionState->allStages());
        if (index == -1)
            continue;

        _dropDown->setCurrentIndex(index);
        break;
    }
}

void StageSelectorWidget::stagePinClicked()
{
    _pinStageSelection = !_pinStageSelection;
    saveStagePinnedOption(_pinStageSelection);
    updatePinnedStage();
}

void StageSelectorWidget::updatePinnedStage()
{
    QtUtils::setupButtonWithHIGBitmaps(
        _pinStage, _pinStageSelection ? ":/UsdLayerEditor/pin_on" : ":/UsdLayerEditor/pin_off");

    if (_pinStageSelection) {
        selectedIndexChanged(_dropDown->currentIndex());
    } else {
        selectionChanged();
    }
}

void StageSelectorWidget::collapseContentClicked()
{
    if (_sessionState) {
        _sessionState->setDisplayLayerContents(!_sessionState->displayLayerContents());
    }
}

void StageSelectorWidget::updateContentButton()
{
    const bool showIt = _sessionState && _sessionState->displayLayerContents();
    QtUtils::setupButtonWithHIGBitmaps(
        _collapseContent,
        showIt ? ":/UsdLayerEditor/contents_on" : ":/UsdLayerEditor/contents_off");
    // Since we use a different set of bitmaps for on/off we don't call setDown() on button
}

// handles when someone else changes the current stage
// but also called when when do it ourselves
void StageSelectorWidget::sessionStageChanged()
{
    if (!_internalChange) {
        auto index = getEntryIndexById(_sessionState->stageEntry(), _sessionState->allStages());
        if (index != -1) {
            QSignalBlocker blocker(_dropDown);
            _dropDown->setCurrentIndex(index);
        }
    }
}

void StageSelectorWidget::stageRenamed(SessionState::StageEntry const& renamedEntry)
{
    auto index = getEntryIndexById(renamedEntry, _sessionState->allStages());
    if (index != -1) {
        _dropDown->setItemText(index, renamedEntry._displayName.c_str());
        _dropDown->setItemData(index, QVariant::fromValue(renamedEntry));
    }
}

void StageSelectorWidget::stageReset(SessionState::StageEntry const& entry)
{
    // Individual combo box entries have a short display name and a reference to a stage,
    // which is not a unique combination.  By construction the combo box indices do line
    // up with the SessionState StageEntry vector though, so in the case of resetting
    // a proxy we will find the matching full proxy path in that vector and use its index
    // to update the combo box.
    auto count = _dropDown->count();
    if (count <= 0) {
        return;
    }

    auto index = getEntryIndexById(entry, _sessionState->allStages());
    if (index >= 0 && index < count) {
        _dropDown->setItemData(index, QVariant::fromValue(entry));
    }
}

} // namespace UsdLayerEditor
