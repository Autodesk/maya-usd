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
#include "MayaUsdEditForwardHost.h"

#include "MayaUsdEditForwardCommand.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/utils/util.h>

#include <usdUfe/base/debugCodes.h>
#include <usdUfe/ufe/UsdUndoableCommand.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/undo/UsdUndoManager.h>

#include <pxr/base/tf/instantiateType.h>
#include <pxr/base/tf/weakPtr.h>
#include <pxr/usd/usd/editTarget.h>

#include <maya/MGlobal.h>
#include <ufe/undoableCommandMgr.h>

#include <AdskUsdEditForward/Record.h>

#include <regex>

namespace {

bool idleTaskQueued = false;

bool IsInUsdUndoBlock() { return UsdUfe::UsdUndoBlock::depth() > 0; }

bool forwardingOpenedUndoChunk = false;

} // namespace

TF_INSTANTIATE_TYPE(MayaUsdEFFallbackTargetChangedNotice, TfType::CONCRETE, TF_1_PARENT(TfNotice));

MayaUsdEditForwardHost::MayaUsdEditForwardHost()
{
    const MString optionVar
        = UsdMayaUtil::convert(MayaUsdOptionVars->LayerEditorEchoEditForwarding);
    if (MGlobal::optionVarExists(optionVar)) {
        _wantsEcho = MGlobal::optionVarIntValue(optionVar) != 0;
    }
}

void MayaUsdEditForwardHost::ExecuteInCmd(std::function<void()> callback, bool immediate)
{
    // If requested to be run immediately, likely an explicit request to forward edits,
    // we can just create an undoable command and run it (no need to consider the UndoBlock
    // context etc. below, that is only relevant when reacting to changes in the scene).
    if (immediate) {
        if (callback) {
            auto cmd = MayaUsd::MayaUsdEditForwardCommand::create(callback);
            Ufe::UndoableCommandMgr::instance().executeCmd(cmd);
        }
        return;
    }

    // If we are not inside a USD undoable command, do not forward. We would not know how to.
    // This could be a script editing USD data, or an interactive edit (slider drag from
    // attribute editor).
    static MayaUsd::MayaUsdEditForwardCommand::Callbacks callbacks;
    const bool forwardEditsWithoutUsdUndoBlock = UsdUfe::NoUsdUndoBlockGuard::wantsForwarding();
    if (!forwardEditsWithoutUsdUndoBlock && !IsInUsdUndoBlock()) {
        TF_DEBUG_INFO_MSG(
            USDUFE_UNDOCMD, "No forwarding: not in set command and not in undo block\n");
        return;
    }

    // if we are in a undo block then we need an undo chunk to group the
    // command with the original edit command. Make sure to only open this chunk
    // once in case multiple commands are executed before the next on-idle.
    //
    // This undo chunk ensures that the original edit command and the
    // forward command executed on next idle are treated as one undo unit.
    // The chunk is opened here, while the current command is still executing
    // before it gets added to the stack.
    if (IsInUsdUndoBlock()) {
        if (!forwardingOpenedUndoChunk) {
            TF_DEBUG_INFO_MSG(USDUFE_UNDOCMD, "In undo block, opening undo chunk for forwarding\n");
            forwardingOpenedUndoChunk = true;
            MGlobal::executeCommand("undoInfo -openChunk");
        }
    }

    if (!idleTaskQueued) {
        idleTaskQueued = true;
        MGlobal::executeTaskOnIdle([](void* data) {
            // Get a local copy before we execute, in case callbacks themselves
            // append new callbacks.
            MayaUsd::MayaUsdEditForwardCommand::Callbacks callbacksCopy;
            callbacksCopy.swap(callbacks);
            idleTaskQueued = false;

            if (forwardingOpenedUndoChunk) {
                TF_DEBUG_INFO_MSG(USDUFE_UNDOCMD, "In undo block, forwarding using command\n");
                auto cmd = MayaUsd::MayaUsdEditForwardCommand::create(std::move(callbacksCopy));
                Ufe::UndoableCommandMgr::instance().executeCmd(cmd);
                forwardingOpenedUndoChunk = false;
                MGlobal::executeCommand("undoInfo -closeChunk");
            } else {
                TF_DEBUG_INFO_MSG(USDUFE_UNDOCMD, "No undo block, forwarding directly\n");
                for (const auto& cb : callbacksCopy) {
                    if (cb) {
                        cb();
                    }
                }
            }
        });
    }

    if (callback) {
        callbacks.push_back(callback);
    }
}

bool MayaUsdEditForwardHost::IsEditForwardingPaused() const
{
    // We always respect the pause flag.
    if (_paused)
        return true;

    // We never pause forwarding when in a transform set operation, as that
    // does not use a UsdUndoBlock in its execute, set, undo and redo functions.
    // IOW, even undo and redo need to forwarded.
    if (UsdUfe::NoUsdUndoBlockGuard::wantsForwarding()) {
        TF_DEBUG_INFO_MSG(USDUFE_UNDOCMD, "Forwarding is not paused: in transform set()\n");
        return false;
    }

    // When in undo/redo that *do* use UsdUndoBlocks, we pause forwarding.
    // This is because the UsdUndoBlock will correctly restore the data
    // and thus will not need to be forwarded.
    if (MGlobal::isUndoing() || MGlobal::isRedoing()) {
        TF_DEBUG_INFO_MSG(USDUFE_UNDOCMD, "Forwarding is paused: in undo/redo, \n");
        return true;
    }

    TF_DEBUG_INFO_MSG(USDUFE_UNDOCMD, "Forwarding is not paused\n");
    return false;
}

void MayaUsdEditForwardHost::PauseEditForwarding(bool pause) { _paused = pause; }

void MayaUsdEditForwardHost::TrackLayerStates(const pxr::SdfLayerHandle& layer)
{
    UsdUfe::UsdUndoManager::instance().trackLayerStates(layer);
}

namespace {
// Per-stage registry of controllers, used by GetForStage()/RegisterForStage().
std::unordered_map<const PXR_NS::UsdStage*, std::weak_ptr<MayaUsdEditForwardController>> s_registry;

std::string EscapeForRegex(const std::string& s)
{
    static const std::regex specialChars(R"([\.\^\$\+\(\)\[\]\{\}\|\?\*])");
    // \$& in replacement: $& expands to the matched char, \ makes it literal — inserts \<char>.
    auto res = std::regex_replace(s, specialChars, R"(\$&)");
    return res;
}
} // namespace

MayaUsdEditForwardController::MayaUsdEditForwardController(const PXR_NS::UsdStageRefPtr& stage)
    : AdskUsdEditForward::StageRuleProvider(stage)
    , _stage(stage)
{
    if (stage) {
        TfWeakPtr<MayaUsdEditForwardController> me(this);

        // Listen to layer changes. If the layer metadata changes on the root layer, rules may
        // have changed, and we may have to toggle the edit forward mode active state.
        _noticeKeys.push_back(TfNotice::Register(
            me, &MayaUsdEditForwardController::_onLayerChanged, TfWeakPtr<SdfLayer>(nullptr)));

        // Listen to edit target changes. If the edit target is changed externally while in edit
        // forward mode, we have to turn it off, as it requires the session layer to be targeted.
        _noticeKeys.push_back(TfNotice::Register(
            me,
            &MayaUsdEditForwardController::_onEditTargetChanged,
            TfWeakPtr<PXR_NS::UsdStage>(_stage)));

        // Sync immediately so that a stage loaded with EF rules already authored
        // (e.g. a component) starts with the correct EF active state.
        syncEditForwardMode();
    }
}

MayaUsdEditForwardController::~MayaUsdEditForwardController() { TfNotice::Revoke(&_noticeKeys); }

void MayaUsdEditForwardController::syncEditForwardMode()
{
    if (!_stage)
        return;

    // TODO : we may want to instead have two distinct states exposed to users, continuous or not,
    // EF enabled or not. UX will iterate on this.
    const bool efShouldBeActive = !GetRules().empty() && IsContinuous();

    if (efShouldBeActive == _efActive)
        return;

    if (efShouldBeActive) {
        // Seed the fallback from the current edit target (where edits were going) only if we
        // don't already have one — a suspended-then-resumed EF keeps its fallback. If the seed
        // layer is locked, fall back to the session layer.
        if (!_fallbackTarget) {
            SdfLayerRefPtr currentTarget = _stage->GetEditTarget().GetLayer();
            _setFallbackTarget(
                (currentTarget && currentTarget->PermissionToEdit())
                    ? currentTarget
                    : SdfLayerRefPtr(_stage->GetSessionLayer()));
        }
        _efActive = true;
        // Edit forwarding needs the USD edit target on the session layer.
        _stage->SetEditTarget(_stage->GetSessionLayer());
    } else {
        // Restore the last fallback target (the user-facing target during EF).
        _efActive = false;
        if (_fallbackTarget) {
            _stage->SetEditTarget(_fallbackTarget);
        }
        _setFallbackTarget(nullptr);
    }
}

void MayaUsdEditForwardController::_onLayerChanged(
    const SdfNotice::LayersDidChangeSentPerLayer& notice,
    const TfWeakPtr<SdfLayer>&                    sender)
{
    if (!_stage || !sender)
        return;
    auto rootLayer = _stage->GetRootLayer();
    if (!rootLayer || sender != rootLayer)
        return;

    // Only react when layer-level custom data changes, that is where EF rules live.
    for (const auto& [layer, changeList] : notice.GetChangeListVec()) {
        if (layer != rootLayer)
            continue;
        for (const auto& [path, entry] : changeList.GetEntryList()) {
            if (path != SdfPath::AbsoluteRootPath())
                continue;
            for (const auto& [field, change] : entry.infoChanged) {
                if (field == SdfFieldKeys->CustomLayerData) {
                    syncEditForwardMode();
                    return;
                }
            }
        }
    }
}

void MayaUsdEditForwardController::_onEditTargetChanged(
    const UsdNotice::StageEditTargetChanged& /*notice*/,
    const TfWeakPtr<PXR_NS::UsdStage>& /*sender*/)
{
    if (!_stage)
        return;

    if (_stage->GetEditTarget().GetLayer() == _stage->GetSessionLayer()) {
        // Target is back on the session layer (e.g. a transient edit-routing scope ended).
        // Re-evaluate EF: this reactivates it if continuous rules are present.
        syncEditForwardMode();
    } else if (_efActive) {
        // Target moved off the session layer. EF requires the session layer as the stage
        // target, so suspend it. Keep _fallbackTarget so EF can resume unchanged when the
        // target returns to the session layer (the common transient-routing case).
        _efActive = false;
        MGlobal::displayInfo(
            "Edit target moved off the session layer; edit forwarding is paused until the "
            "session layer is targeted again.");
    }
}

std::vector<AdskUsdEditForward::RuleDef::Ptr> MayaUsdEditForwardController::GetRules() const
{
    // Start with any rules authored on the stage (read from root layer custom data).
    auto rules = AdskUsdEditForward::StageRuleProvider::GetRules();

    // No rule for a session-layer fallback: session->session moves nothing.
    const bool hasNonSessionFallback
        = _fallbackTarget && _stage && _fallbackTarget != _stage->GetSessionLayer();
    if (hasNonSessionFallback) {
        // Append an in-memory catch-all rule targeting the fallback layer.
        // Appended last so any user-authored rules take precedence.
        static const std::string kFallbackRuleId = "maya_ef_fallback_rule";

        auto rule = std::make_shared<AdskUsdEditForward::RuleDef>();
        rule->id = kFallbackRuleId;
        rule->description = "MayaUsd Edit Forwarding Fallback Rule";
        rule->inputObjectExpression = AdskUsdEditForward::RuleExpression(".*");
        rule->targetLayerExpression
            = AdskUsdEditForward::RuleExpression(EscapeForRegex(_fallbackTarget->GetIdentifier()));

        rule->blockWeakOpinion = true;

        rules.push_back(rule);
    }

    return rules;
}

// These redirect where unmatched edits go; they do not toggle EF on or off.
void MayaUsdEditForwardController::setFallbackTarget(const PXR_NS::SdfLayerRefPtr& layer)
{
    _setFallbackTarget(layer);
}

void MayaUsdEditForwardController::clearFallbackTarget() { _setFallbackTarget(nullptr); }

void MayaUsdEditForwardController::_setFallbackTarget(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (_fallbackTarget == layer)
        return;
    _fallbackTarget = layer;
    MayaUsdEFFallbackTargetChangedNotice(_stage).Send();
}

/*static*/
MayaUsdEditForwardController::Ptr
MayaUsdEditForwardController::GetForStage(const PXR_NS::UsdStageRefPtr& stage)
{
    if (!stage) {
        return nullptr;
    }
    // Prune all expired entries on each lookup.
    for (auto it = s_registry.begin(); it != s_registry.end();) {
        if (it->second.expired()) {
            it = s_registry.erase(it);
        } else {
            ++it;
        }
    }
    auto it = s_registry.find(stage.operator->());
    if (it != s_registry.end()) {
        return it->second.lock();
    }
    return nullptr;
}

/*static*/
void MayaUsdEditForwardController::RegisterForStage(
    const PXR_NS::UsdStageRefPtr& stage,
    const Ptr&                    controller)
{
    if (!stage) {
        return;
    }
    s_registry[stage.operator->()] = controller;
}

bool MayaUsdEditForwardHost::WantsEcho() const { return _wantsEcho; }

void MayaUsdEditForwardHost::SetWantsEcho(bool echo) { _wantsEcho = echo; }

void MayaUsdEditForwardHost::Echo(const AdskUsdEditForward::Record& record)
{
    MGlobal::displayInfo(("[Edit Forwarding] " + record.ToString()).c_str());
}
