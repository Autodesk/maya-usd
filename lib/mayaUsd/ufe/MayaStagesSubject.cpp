//
// Copyright 2023 Autodesk
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
#include "MayaStagesSubject.h"

#include "MayaUsdHierarchy.h"

#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/UsdStageMap.h>

#include <usdUfe/undo/UsdUndoManager.h>

#include <maya/MMessage.h>
#include <maya/MSceneMessage.h>
#include <ufe/hierarchy.h>

namespace {

// Prevent re-entrant stage set.
std::atomic_bool stageSetGuardCount { false };

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::StagesSubject, MayaStagesSubject);

//------------------------------------------------------------------------------
// Global variables & macros
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// MayaStagesSubject
//------------------------------------------------------------------------------

MayaStagesSubject::MayaStagesSubject()
{
    // Workaround to MAYA-65920: at startup, MSceneMessage.kAfterNew file
    // callback is incorrectly called by Maya before the
    // MSceneMessage.kBeforeNew file callback, which should be illegal.
    // Detect this and ignore illegal calls to after new file callbacks.
    // PPT, 19-Jan-16.
    setInNewScene(false);

    MStatus res;
    _cbIds.append(
        MSceneMessage::addCallback(MSceneMessage::kBeforeNew, beforeNewCallback, this, &res));
    CHECK_MSTATUS(res);
    _cbIds.append(
        MSceneMessage::addCallback(MSceneMessage::kBeforeOpen, beforeOpenCallback, this, &res));
    CHECK_MSTATUS(res);
    _cbIds.append(
        MSceneMessage::addCallback(MSceneMessage::kAfterOpen, afterOpenCallback, this, &res));
    CHECK_MSTATUS(res);
    _cbIds.append(
        MSceneMessage::addCallback(MSceneMessage::kAfterNew, afterNewCallback, this, &res));
    CHECK_MSTATUS(res);

    auto me = PXR_NS::TfCreateWeakPtr(this);
    TfNotice::Register(me, &MayaStagesSubject::onStageSet);
    TfNotice::Register(me, &MayaStagesSubject::onStageInvalidate);
}

MayaStagesSubject::~MayaStagesSubject()
{
    MMessage::removeCallbacks(_cbIds);
    _cbIds.clear();
}

/*static*/
MayaStagesSubject::RefPtr MayaStagesSubject::create()
{
    return TfCreateRefPtr(new MayaStagesSubject);
}

void MayaStagesSubject::stageChanged(
    UsdNotice::ObjectsChanged const& notice,
    UsdStageWeakPtr const&           sender)
{
    // Override to bracket the stageChanged notification with
    // mayaUsdHierarchyStageChangedBegin/End calls.
    mayaUsdHierarchyStageChangedBegin();
    StagesSubject::stageChanged(notice, sender);
    mayaUsdHierarchyStageChangedEnd();
}

bool MayaStagesSubject::isInNewScene() const { return _isInNewScene; }

void MayaStagesSubject::setInNewScene(bool b)
{
    _isInNewScene = b;
    _invalidStages.clear();
}

/*static*/
void MayaStagesSubject::beforeNewCallback(void* clientData)
{
    MayaStagesSubject* ss = static_cast<MayaStagesSubject*>(clientData);
    ss->setInNewScene(true);
    ss->beforeOpen();
}

/*static*/
void MayaStagesSubject::beforeOpenCallback(void* clientData)
{
    MayaStagesSubject::beforeNewCallback(clientData);
}

/*static*/
void MayaStagesSubject::afterNewCallback(void* clientData)
{
    MayaStagesSubject* ss = static_cast<MayaStagesSubject*>(clientData);

    // Workaround to MAYA-65920: detect and avoid illegal callback sequence.
    if (!ss->isInNewScene())
        return;

    ss->setInNewScene(false);
    MayaStagesSubject::afterOpenCallback(clientData);
}

/*static*/
void MayaStagesSubject::afterOpenCallback(void* clientData) { afterNewCallback(clientData); }

void MayaStagesSubject::beforeOpen() { clearListeners(); }

void MayaStagesSubject::clearListeners()
{
    // Observe stage changes, for all stages.  Return listener object can
    // optionally be used to call Revoke() to remove observation, but must
    // keep reference to it, otherwise its reference count is immediately
    // decremented, falls to zero, and no observation occurs.

    // Ideally, we would observe the data model only if there are observers,
    // to minimize cost of observation.  However, since observation is
    // frequent, we won't implement this for now.  PPT, 22-Dec-2017.
    std::for_each(
        std::begin(_stageListeners),
        std::end(_stageListeners),
        [](StageListenerMap::value_type element) {
            for (auto& noticeKey : element.second) {
                TfNotice::Revoke(noticeKey);
            }
        });
    _stageListeners.clear();

    // Set up our stage to proxy shape UFE path (and reverse)
    // mapping.  We do this with the following steps:
    // - get all proxyShape nodes in the scene.
    // - get their Dag paths.
    // - convert the Dag paths to UFE paths.
    // - get their stage.
    UsdStageMap::getInstance().setDirty();
}

void MayaStagesSubject::onStageSet(const MayaUsdProxyStageSetNotice& notice)
{
    auto noticeStage = notice.GetStage();
    // Check if stage received from notice is valid. We could have cases where a ProxyShape has an
    // invalid stage.
    if (noticeStage) {
        // Track the edit target layer's state
        UsdUfe::UsdUndoManager::instance().trackLayerStates(
            noticeStage->GetEditTarget().GetLayer());
    }

    setupListeners();
}

void MayaStagesSubject::setupListeners()
{
    // Handle re-entrant MayaUsdProxyShapeBase::compute; allow update only on first compute call.
    if (MayaUsdProxyShapeBase::in_compute > 1)
        return;

    // Handle re-entrant onStageSet
    bool expectedState = false;
    if (stageSetGuardCount.compare_exchange_strong(expectedState, true)) {
        // We should have no listeners and stage map is dirty.
        TF_VERIFY(UsdStageMap::getInstance().isDirty());
        TF_VERIFY(_stageListeners.empty());

        auto me = PXR_NS::TfCreateWeakPtr(this);
        for (auto stage : ProxyShapeHandler::getAllStages()) {
            NoticeKeys noticeKeys;
            noticeKeys[0] = TfNotice::Register(me, &MayaStagesSubject::stageChanged, stage);
            noticeKeys[1]
                = TfNotice::Register(me, &MayaStagesSubject::stageEditTargetChanged, stage);
            _stageListeners[stage] = noticeKeys;
        }

        // Now we can send the notifications about stage change.
        mayaUsdHierarchyStageChangedBegin();
        for (auto& path : _invalidStages) {
            Ufe::SceneItem::Ptr sceneItem = Ufe::Hierarchy::createItem(path);
            if (sceneItem) {
                sendSubtreeInvalidate(sceneItem);
            }
        }
        mayaUsdHierarchyStageChangedEnd();

        _invalidStages.clear();

        stageSetGuardCount = false;
    }
}

void MayaStagesSubject::onStageInvalidate(const MayaUsdProxyStageInvalidateNotice& notice)
{
    clearListeners();

    auto p = notice.GetProxyShape().ufePath();
    if (!p.empty()) {
        // We can't send notification to clients from dirty propagation.
        // Delay it till the new stage is actually set during compute.
        _invalidStages.insert(p);
    }
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
