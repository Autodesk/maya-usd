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

#include "mayaEditCommitter.h"

#include <mayaUsd/undo/MayaUsdUndoBlock.h>
#include <mayaUsdUI/ui/undoChunkUtils.h>

#include <usdUfe/undo/UsdUndoManager.h>
#include <usdUfe/undo/UsdUndoUtils.h>

#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/refPtr.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/usd/sdf/changeBlock.h>
#include <pxr/usd/usd/editTarget.h>
#include <pxr/usd/usd/notice.h>

#include <memory>
#include <utility>

namespace MayaUsdRenderSetup {

namespace {

using LayerRefs = std::shared_ptr<std::vector<PXR_NS::SdfLayerRefPtr>>;

// Records an invert function holding strong references to the layers. The invert
// functions of an undoable item are replaced by the ones recorded while it runs,
// so the function re-records itself to keep the layers alive through every
// undo and redo, for as long as the item stays on the Maya undo stack.
void keepLayersAlive(const LayerRefs& layers)
{
    UsdUfe::UsdUndoManagerAccessor::addInverse([layers]() { keepLayersAlive(layers); });
}

// Must be called while a MayaUsdUndoBlock is open.
void keepLayersAliveWithUndoItem(const std::vector<PXR_NS::SdfLayerHandle>& pinnedLayers)
{
    auto layers = std::make_shared<std::vector<PXR_NS::SdfLayerRefPtr>>();
    for (const PXR_NS::SdfLayerHandle& handle : pinnedLayers) {
        if (handle) {
            layers->push_back(PXR_NS::TfCreateRefPtrFromProtectedWeakPtr(handle));
        }
    }
    if (!layers->empty()) {
        keepLayersAlive(layers);
    }
}

// Sets the edit target once the stage has recomposed. An undo or redo runs inside an
// SdfChangeBlock, so a layer it adds to the stage is not in the stage yet while the
// invert functions run, and an edit target on that layer would be refused.
class DeferredEditTarget : public PXR_NS::TfWeakBase
{
public:
    static void set(const PXR_NS::UsdStageWeakPtr& stage, const PXR_NS::UsdEditTarget& target)
    {
        if (!stage) {
            return;
        }
        if (stage->HasLocalLayer(target.GetLayer())) {
            stage->SetEditTarget(target);
            return;
        }
        auto deferred = std::shared_ptr<DeferredEditTarget>(new DeferredEditTarget(stage, target));
        deferred->_self = deferred;
        deferred->_key = PXR_NS::TfNotice::Register(
            PXR_NS::TfCreateWeakPtr(deferred.get()), &DeferredEditTarget::onStageChanged, stage);
    }

private:
    DeferredEditTarget(const PXR_NS::UsdStageWeakPtr& stage, const PXR_NS::UsdEditTarget& target)
        : _stage(stage)
        , _target(target)
    {
    }

    void onStageChanged(const PXR_NS::UsdNotice::StageContentsChanged&)
    {
        // Keeps this object alive until the end of the call.
        const std::shared_ptr<DeferredEditTarget> self = std::move(_self);
        PXR_NS::TfNotice::Revoke(_key);
        if (_stage && _stage->HasLocalLayer(_target.GetLayer())) {
            _stage->SetEditTarget(_target);
        }
    }

    PXR_NS::UsdStageWeakPtr             _stage;
    PXR_NS::UsdEditTarget               _target;
    PXR_NS::TfNotice::Key               _key;
    std::shared_ptr<DeferredEditTarget> _self;
};

// Stage edit targets are stage state, not layer state, so the layer tracking does not
// undo them. Like keepLayersAlive, the function swaps its targets on every undo and redo.
void restoreEditTarget(
    const PXR_NS::UsdStageWeakPtr& stage,
    const PXR_NS::UsdEditTarget&   target,
    const PXR_NS::UsdEditTarget&   other)
{
    UsdUfe::UsdUndoManagerAccessor::addInverse([stage, target, other]() {
        DeferredEditTarget::set(stage, target);
        restoreEditTarget(stage, other, target);
    });
}

} // namespace

MayaEditCommitter::MayaEditCommitter(QObject* parent)
    : QObject(parent)
{
}

void MayaEditCommitter::setStages(const std::vector<AdskUsdRenderSetup::HostStage>& stages)
{
    _stages.clear();
    _stages.reserve(stages.size());
    for (const auto& hostStage : stages) {
        if (hostStage.stage) {
            _stages.push_back(hostStage.stage);
        }
    }
}

void MayaEditCommitter::commit(const std::string& undoLabel, std::function<void()> doEdit)
{
    doCommit(undoLabel, std::move(doEdit), {});
}

void MayaEditCommitter::commit(
    const std::string&                         undoLabel,
    std::function<void()>                      doEdit,
    const std::vector<PXR_NS::SdfLayerHandle>& pinnedLayers)
{
    doCommit(undoLabel, std::move(doEdit), pinnedLayers);
}

void MayaEditCommitter::doCommit(
    const std::string&                         undoLabel,
    std::function<void()>                      doEdit,
    const std::vector<PXR_NS::SdfLayerHandle>& pinnedLayers)
{
    if (!doEdit || _stages.empty()) {
        return;
    }

    UsdUfe::trackStagesEditTargets(_stages);
    // The session layer holds some of the render setup state, such as the active layer.
    for (const PXR_NS::UsdStageRefPtr& stage : _stages) {
        UsdUfe::UsdUndoManager::instance().trackLayerStates(stage->GetSessionLayer());
    }

    const MayaUsdUI::UndoChunkGuard undoChunkGuard(undoLabel);
    MayaUsd::MayaUsdUndoBlock       block;
    {
        keepLayersAliveWithUndoItem(pinnedLayers);

        std::vector<PXR_NS::UsdEditTarget> editTargetsBefore;
        editTargetsBefore.reserve(_stages.size());
        for (const PXR_NS::UsdStageRefPtr& stage : _stages) {
            editTargetsBefore.push_back(stage->GetEditTarget());
        }

        doEdit();

        for (std::size_t i = 0; i < _stages.size(); ++i) {
            const PXR_NS::UsdEditTarget editTargetAfter = _stages[i]->GetEditTarget();
            if (editTargetAfter != editTargetsBefore[i]) {
                restoreEditTarget(_stages[i], editTargetsBefore[i], editTargetAfter);
            }
        }
    }
}

} // namespace MayaUsdRenderSetup
