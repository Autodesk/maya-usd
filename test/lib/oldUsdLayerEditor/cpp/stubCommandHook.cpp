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

#include "stubCommandHook.h"

#include <pxr/usd/sdf/layer.h>

namespace UsdLayerEditor {

OldEditorStubCommandHook::OldEditorStubCommandHook(SessionState* sessionState)
    : AbstractCommandHook(sessionState)
{
}

void OldEditorStubCommandHook::setEditTarget(UsdLayer layer)
{
    _calls.push_back({ "setEditTarget", { layer->GetIdentifier() } });
}

void OldEditorStubCommandHook::insertSubLayerPath(UsdLayer layer, Path path, int index)
{
    _calls.push_back(
        { "insertSubLayerPath", { layer->GetIdentifier(), path, std::to_string(index) } });
    layer->InsertSubLayerPath(path, index);
}

void OldEditorStubCommandHook::removeSubLayerPath(UsdLayer layer, Path path)
{
    _calls.push_back({ "removeSubLayerPath", { layer->GetIdentifier(), path } });
    auto paths = layer->GetSubLayerPaths();
    for (size_t i = 0; i < paths.size(); ++i) {
        if (paths[i] == path) {
            layer->RemoveSubLayerPath(i);
            break;
        }
    }
}

void OldEditorStubCommandHook::replaceSubLayerPath(UsdLayer layer, Path oldPath, Path newPath)
{
    _calls.push_back({ "replaceSubLayerPath", { layer->GetIdentifier(), oldPath, newPath } });
}

void OldEditorStubCommandHook::moveSubLayerPath(
    Path path, UsdLayer oldParent, UsdLayer newParent, int index)
{
    _calls.push_back({ "moveSubLayerPath", { path, std::to_string(index) } });
    auto   oldPaths = oldParent->GetSubLayerPaths();
    size_t fromIdx  = static_cast<size_t>(-1);
    for (size_t i = 0; i < oldPaths.size(); ++i) {
        if (oldPaths[i] == path) {
            fromIdx = i;
            break;
        }
    }
    if (fromIdx != static_cast<size_t>(-1)) {
        oldParent->RemoveSubLayerPath(fromIdx);
    }
    newParent->InsertSubLayerPath(path, index);
}

void OldEditorStubCommandHook::discardEdits(UsdLayer layer)
{
    _calls.push_back({ "discardEdits", { layer->GetIdentifier() } });
    layer->Clear();
}

void OldEditorStubCommandHook::clearLayer(UsdLayer layer)
{
    _calls.push_back({ "clearLayer", { layer->GetIdentifier() } });
    layer->Clear();
}

void OldEditorStubCommandHook::flattenLayer(UsdLayer layer)
{
    _calls.push_back({ "flattenLayer", { layer->GetIdentifier() } });
}

UsdLayer OldEditorStubCommandHook::addAnonymousSubLayer(UsdLayer layer, std::string newName)
{
    _calls.push_back({ "addAnonymousSubLayer", { layer->GetIdentifier(), newName } });
    auto newLayer = PXR_NS::SdfLayer::CreateAnonymous(newName);
    layer->InsertSubLayerPath(newLayer->GetIdentifier(), 0);
    return newLayer;
}

void OldEditorStubCommandHook::muteSubLayer(UsdLayer layer, bool muteIt)
{
    _calls.push_back({ "muteSubLayer", { layer->GetIdentifier(), muteIt ? "true" : "false" } });
}

void OldEditorStubCommandHook::lockLayer(
    UsdLayer layer, MayaUsd::LayerLockType lockState, bool /*includeSubLayers*/)
{
    _calls.push_back({ "lockLayer", { layer->GetIdentifier() } });
    layer->SetPermissionToEdit(lockState == MayaUsd::LayerLock_Unlocked);
}

void OldEditorStubCommandHook::refreshLayerSystemLock(UsdLayer layer, bool /*refreshSubLayers*/)
{
    _calls.push_back({ "refreshLayerSystemLock", { layer->GetIdentifier() } });
}

void OldEditorStubCommandHook::stitchLayers(const std::vector<PXR_NS::SdfLayerRefPtr>& /*layers*/)
{
    _calls.push_back({ "stitchLayers", {} });
}

void OldEditorStubCommandHook::openUndoBracket(const QString& name)
{
    _calls.push_back({ "openUndoBracket", { name.toStdString() } });
}

void OldEditorStubCommandHook::closeUndoBracket()
{
    _calls.push_back({ "closeUndoBracket", {} });
}

void OldEditorStubCommandHook::showLayerEditorHelp()
{
    _calls.push_back({ "showLayerEditorHelp", {} });
}

void OldEditorStubCommandHook::selectPrimsWithSpec(UsdLayer layer)
{
    _calls.push_back({ "selectPrimsWithSpec", { layer->GetIdentifier() } });
}

void OldEditorStubCommandHook::clearCalls()
{
    _calls.clear();
}

bool OldEditorStubCommandHook::hasCall(const std::string& method) const
{
    return callCount(method) > 0;
}

int OldEditorStubCommandHook::callCount(const std::string& method) const
{
    int count = 0;
    for (const auto& call : _calls) {
        if (call.name == method)
            ++count;
    }
    return count;
}

const CommandCall& OldEditorStubCommandHook::lastCall() const
{
    return _calls.back();
}

const CommandCall* OldEditorStubCommandHook::lastCallOf(const std::string& method) const
{
    for (auto it = _calls.rbegin(); it != _calls.rend(); ++it)
        if (it->name == method)
            return &*it;
    return nullptr;
}

} // namespace UsdLayerEditor
