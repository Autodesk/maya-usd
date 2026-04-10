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

#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>

#include <usdUfe/base/debugCodes.h>
#include <usdUfe/ufe/UsdUndoableCommand.h>
#include <usdUfe/ufe/trf/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/undo/UsdUndoManager.h>

#include <maya/MGlobal.h>
#include <ufe/undoableCommandMgr.h>

namespace {

bool idleTaskQueued = false;

bool IsInUsdUndoBlock() { return UsdUfe::UsdUndoBlock::depth() > 0; }

bool forwardingOpenedUndoChunk = false;

} // namespace

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
        TF_DEBUG_MSG(USDUFE_UNDOCMD, "No forwarding: not in set command and not in undo block\n");
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
            TF_DEBUG_MSG(USDUFE_UNDOCMD, "In undo block, opening undo chunk for forwarding\n");
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
                TF_DEBUG_MSG(USDUFE_UNDOCMD, "In undo block, forwarding using command\n");
                auto cmd = MayaUsd::MayaUsdEditForwardCommand::create(std::move(callbacksCopy));
                Ufe::UndoableCommandMgr::instance().executeCmd(cmd);
                forwardingOpenedUndoChunk = false;
                MGlobal::executeCommand("undoInfo -closeChunk");
            } else {
                TF_DEBUG_MSG(USDUFE_UNDOCMD, "No undo block, forwarding directly\n");
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
        TF_DEBUG_MSG(USDUFE_UNDOCMD, "Forwarding is not paused: in transform set()\n");
        return false;
    }

    // When in undo/redo that *do* use UsdUndoBlocks, we pause forwarding.
    // This is because the UsdUndoBlock will correctly restore the data
    // and thus will not need to be forwarded.
    if (MGlobal::isUndoing() || MGlobal::isRedoing()) {
        TF_DEBUG_MSG(USDUFE_UNDOCMD, "Forwarding is paused: in undo/redo, \n");
        return true;
    }

    TF_DEBUG_MSG(USDUFE_UNDOCMD, "Forwarding is not paused\n");
    return false;
}

void MayaUsdEditForwardHost::PauseEditForwarding(bool pause) { _paused = pause; }

void MayaUsdEditForwardHost::TrackLayerStates(const pxr::SdfLayerHandle& layer)
{
    UsdUfe::UsdUndoManager::instance().trackLayerStates(layer);
}

// =============================================================================
// MayaUsd::MayaForwardRuleProvider
// =============================================================================

namespace MayaUsd {

std::mutex MayaForwardRuleProvider::s_registryMutex;
std::unordered_map<const PXR_NS::UsdStage*, std::weak_ptr<MayaForwardRuleProvider>>
    MayaForwardRuleProvider::s_registry;

MayaForwardRuleProvider::MayaForwardRuleProvider(const PXR_NS::UsdStageRefPtr& stage)
    : AdskUsdEditForward::StageRuleProvider(stage)
    , _stage(stage)
{
}

std::vector<AdskUsdEditForward::RuleDef::Ptr> MayaForwardRuleProvider::GetRules() const
{
    // When forwarding is disabled, return no rules.
    if (!isEnabled())
        return {};

    // Start with any rules authored on the stage (read from root layer custom data).
    auto rules = AdskUsdEditForward::StageRuleProvider::GetRules();

    if (_fallbackTarget) {
        // Append an in-memory catch-all rule targeting the fallback layer.
        // Appended last so any user-authored rules take precedence.
        static const std::string kFallbackRuleId = "maya_ef_fallback_rule";

        auto rule       = std::make_shared<AdskUsdEditForward::RuleDef>();
        rule->id        = kFallbackRuleId;
        rule->description
            = "Maya EF fallback rule (in-memory, not persisted to root layer)";
        rule->inputObjectExpression = AdskUsdEditForward::RuleExpression(".*");
        rule->targetLayerExpression = AdskUsdEditForward::RuleExpression(
            regexEscapeLayerPath(_fallbackTarget->GetIdentifier()));
        
        rule->blockWeakLocalOpinion = true;

        rules.push_back(rule);
    }

    return rules;
}

void MayaForwardRuleProvider::setEnabled(bool enabled)
{
    if (!_stage)
        return;
    auto sessionLayer = _stage->GetSessionLayer();
    if (!sessionLayer)
        return;
    PXR_NS::VtDictionary data = sessionLayer->GetCustomLayerData();
    data["adsk_forward_active"]  = PXR_NS::VtValue(enabled);
    sessionLayer->SetCustomLayerData(data);
}

bool MayaForwardRuleProvider::isEnabled() const
{
    if (!_stage)
        return false;
    auto sessionLayer = _stage->GetSessionLayer();
    if (!sessionLayer)
        return false;
    const auto& data = sessionLayer->GetCustomLayerData();
    auto        it   = data.find("adsk_forward_active");
    return it != data.end() && it->second.IsHolding<bool>()
        && it->second.UncheckedGet<bool>();
}

void MayaForwardRuleProvider::setFallbackTarget(const PXR_NS::SdfLayerRefPtr& layer)
{
    _fallbackTarget = layer;
}

void MayaForwardRuleProvider::clearFallbackTarget() { _fallbackTarget = nullptr; }

/*static*/
MayaForwardRuleProvider::Ptr
MayaForwardRuleProvider::GetForStage(const PXR_NS::UsdStageRefPtr& stage)
{
    if (!stage)
        return nullptr;
    std::lock_guard<std::mutex> lock(s_registryMutex);
    // Prune all expired entries on each lookup. The map stays small (one entry per open
    // stage), so a full scan is negligible and avoids unbounded accumulation of dead keys.
    for (auto it = s_registry.begin(); it != s_registry.end();) {
        if (it->second.expired())
            it = s_registry.erase(it);
        else
            ++it;
    }
    auto it = s_registry.find(stage.operator->());
    if (it != s_registry.end())
        return it->second.lock();
    return nullptr;
}

/*static*/
void MayaForwardRuleProvider::RegisterForStage(
    const PXR_NS::UsdStageRefPtr& stage,
    const Ptr&                    provider)
{
    if (!stage)
        return;
    std::lock_guard<std::mutex> lock(s_registryMutex);
    s_registry[stage.operator->()] = provider;
}

/*static*/
std::string MayaForwardRuleProvider::regexEscapeLayerPath(const std::string& input)
{
    static const std::string specialChars = R"(\.^$*+?()[]{}|)";
    std::string              result;
    result.reserve(input.size() * 2);
    for (char c : input) {
        if (specialChars.find(c) != std::string::npos)
            result += '\\';
        result += c;
    }
    return result;
}

} // namespace MayaUsd
