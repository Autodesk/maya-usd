//
// Copyright 2019 Autodesk
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
#include "UsdUndoMaterialCommands.h"

#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/sdr/shaderProperty.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/sceneItemOps.h>

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdPrim BindMaterialUndoableCommand::CompatiblePrim(const Ufe::SceneItem::Ptr& item)
{
    auto usdItem = std::dynamic_pointer_cast<const MAYAUSD_NS::ufe::UsdSceneItem>(item);
    if (!usdItem) {
        return {};
    }
    UsdPrim usdPrim = usdItem->prim();
    if (UsdShadeNodeGraph(usdPrim) || UsdShadeShader(usdPrim)) {
        // The binding schema can be applied anywhere, but it makes no sense on a
        // material or a shader.
        return {};
    }
    if (PXR_NS::UsdShadeMaterialBindingAPI::CanApply(usdPrim)) {
        return usdPrim;
    }
    return {};
}

BindMaterialUndoableCommand::BindMaterialUndoableCommand(
    const UsdPrim& prim,
    const SdfPath& materialPath)
    : _stage(prim.GetStage())
    , _primPath(prim.GetPath())
    , _materialPath(materialPath)
{
}

BindMaterialUndoableCommand::~BindMaterialUndoableCommand() { }

void BindMaterialUndoableCommand::undo()
{
    if (!_stage || _primPath.IsEmpty() || _materialPath.IsEmpty()) {
        return;
    }

    UsdPrim prim = _stage->GetPrimAtPath(_primPath);
    if (prim.IsValid()) {
        auto bindingAPI = UsdShadeMaterialBindingAPI(prim);
        if (bindingAPI) {
            if (_previousMaterialPath.IsEmpty()) {
                bindingAPI.UnbindDirectBinding();
            } else {
                UsdShadeMaterial material(_stage->GetPrimAtPath(_previousMaterialPath));
                bindingAPI.Bind(material);
            }
        }
        if (_appliedBindingAPI) {
            prim.RemoveAPI<UsdShadeMaterialBindingAPI>();
        }
    }
}

void BindMaterialUndoableCommand::redo()
{
    if (!_stage || _primPath.IsEmpty() || _materialPath.IsEmpty()) {
        return;
    }

    UsdPrim          prim = _stage->GetPrimAtPath(_primPath);
    UsdShadeMaterial material(_stage->GetPrimAtPath(_materialPath));
    if (prim.IsValid() && material) {
        UsdShadeMaterialBindingAPI bindingAPI;
        if (prim.HasAPI<UsdShadeMaterialBindingAPI>()) {
            bindingAPI = UsdShadeMaterialBindingAPI(prim);
            _previousMaterialPath = bindingAPI.GetDirectBinding().GetMaterialPath();
        } else {
            bindingAPI = UsdShadeMaterialBindingAPI::Apply(prim);
            _appliedBindingAPI = true;
        }
        bindingAPI.Bind(material);
    }
}
const std::string BindMaterialUndoableCommand::commandName("Bind Material");

UnbindMaterialUndoableCommand::UnbindMaterialUndoableCommand(const UsdPrim& prim)
    : _stage(prim.GetStage())
    , _primPath(prim.GetPath())
{
}

UnbindMaterialUndoableCommand::~UnbindMaterialUndoableCommand() { }

void UnbindMaterialUndoableCommand::undo()
{
    if (!_stage || _primPath.IsEmpty() || _materialPath.IsEmpty()) {
        return;
    }

    UsdPrim          prim = _stage->GetPrimAtPath(_primPath);
    UsdShadeMaterial material(_stage->GetPrimAtPath(_materialPath));
    if (prim.IsValid() && material) {
        // BindingAPI is still there since we did not remove it.
        auto             bindingAPI = UsdShadeMaterialBindingAPI(prim);
        UsdShadeMaterial material(_stage->GetPrimAtPath(_materialPath));
        if (bindingAPI && material) {
            bindingAPI.Bind(material);
        }
    }
}

void UnbindMaterialUndoableCommand::redo()
{
    if (!_stage || _primPath.IsEmpty()) {
        return;
    }

    UsdPrim prim = _stage->GetPrimAtPath(_primPath);
    if (prim.IsValid()) {
        auto bindingAPI = UsdShadeMaterialBindingAPI(prim);
        if (bindingAPI) {
            auto materialBinding = bindingAPI.GetDirectBinding();
            _materialPath = materialBinding.GetMaterialPath();
            if (!_materialPath.IsEmpty()) {
                bindingAPI.UnbindDirectBinding();
                // TODO: Can we remove the BindingAPI at this point?
                //       Not easy to know for sure.
            }
        }
    }
}
const std::string UnbindMaterialUndoableCommand::commandName("Unbind Material");

#if (UFE_PREVIEW_VERSION_NUM >= 4010)
UsdUndoAssignNewMaterialCommand::UsdUndoAssignNewMaterialCommand(
    const UsdSceneItem::Ptr& parentItem,
    const std::string&       nodeId)
    : Ufe::InsertChildCommand()
    , _parentPath(parentItem->path())
    , _nodeId(nodeId)
{
}

UsdUndoAssignNewMaterialCommand::~UsdUndoAssignNewMaterialCommand() { }

UsdUndoAssignNewMaterialCommand::Ptr UsdUndoAssignNewMaterialCommand::create(
    const UsdSceneItem::Ptr& parentItem,
    const std::string&       nodeId)
{
    // Changing the hierarchy of invalid items is not allowed.
    if (!parentItem || !parentItem->prim().IsActive())
        return nullptr;

    return std::make_shared<UsdUndoAssignNewMaterialCommand>(parentItem, nodeId);
}

Ufe::SceneItem::Ptr UsdUndoAssignNewMaterialCommand::insertedChild() const
{
    return _createShaderCmd ? _createShaderCmd->insertedChild() : nullptr;
}

namespace {
// This class insures that either everything has succeeded, or makes sure to bring back to the
// initial state:
class UndoGuard
{
public:
    UndoGuard(Ufe::UndoableCommand* cmd)
        : _this(cmd)
    {
    }
    ~UndoGuard()
    {
        // Incomplete. Undo all stored tasks until we get back to original state.
        if (_this) {
            _this->undo();
        }
    }
    void markAsCompleted() { _this = nullptr; }

private:
    Ufe::UndoableCommand* _this = nullptr;
};

static const std::string kDefaultMaterialScopeName("mtl");
} // namespace

void UsdUndoAssignNewMaterialCommand::execute()
{
    UndoGuard guard(this);
    //
    // 1. Create the Scope "materials" if it does not exist:
    //
    auto parentItem
        = std::dynamic_pointer_cast<UsdSceneItem>(Ufe::Hierarchy::createItem(_parentPath));
    Ufe::Path               scopePath;
    PXR_NS::UsdStageWeakPtr stage = getStage(parentItem->path());
    if (stage) {
        auto stageHierarchy
            = Ufe::Hierarchy::hierarchy(Ufe::Hierarchy::createItem(_parentPath.popSegment()));
        if (stageHierarchy) {
            for (auto&& child : stageHierarchy->children()) {
                // Could be "mtl1" if there is already something named mtl which is not a scope.
                if (child->nodeName().rfind(kDefaultMaterialScopeName, 0) == 0
                    && child->nodeType() == "Scope") {
                    scopePath = child->path();
                    break;
                }
            }
        }
        if (scopePath.empty()) {
            _createScopeCmd = UsdUndoAddNewPrimCommand::create(
                UsdSceneItem::create(MayaUsd::ufe::stagePath(stage), stage->GetPseudoRoot()),
                kDefaultMaterialScopeName,
                "Scope");
            _createScopeCmd->execute();
            scopePath = _createScopeCmd->newUfePath();
            // The code automatically appends a "1". We need to rename:
            auto itemOps = Ufe::SceneItemOps::sceneItemOps(Ufe::Hierarchy::createItem(scopePath));
            auto rename = itemOps->renameItemCmd(Ufe::PathComponent(kDefaultMaterialScopeName));
            _renameScopeCmd = rename.undoableCommand;
            scopePath = rename.item->path();
        }
    }
    if (scopePath.empty()) {
        TF_RUNTIME_ERROR("Failed to create materials scope at stage root");
        return;
    }
    //
    // 2. Create the Material if it does not exist:
    //
    PXR_NS::SdrRegistry&          registry = PXR_NS::SdrRegistry::GetInstance();
    PXR_NS::SdrShaderNodeConstPtr shaderNodeDef
        = registry.GetShaderNodeByIdentifier(TfToken(_nodeId));
    if (!shaderNodeDef) {
        TF_RUNTIME_ERROR("Unknown shader identifier: %s", _nodeId.c_str());
        return;
    }
    if (shaderNodeDef->GetOutputNames().empty()) {
        TF_RUNTIME_ERROR("Surface shader %s does not have any outputs", _nodeId.c_str());
        return;
    }
    auto scopeItem = std::dynamic_pointer_cast<UsdSceneItem>(Ufe::Hierarchy::createItem(scopePath));
    _createMaterialCmd = UsdUndoAddNewPrimCommand::create(
        scopeItem, shaderNodeDef->GetFamily().GetString(), "Material");
    _createMaterialCmd->execute();
    if (!_createMaterialCmd->newPrim()) {
        TF_RUNTIME_ERROR("Could not create material under %s", scopePath.string().c_str());
        return;
    }
    //
    // 3. Create the Shader if it does not exist:
    //
    auto materialItem = std::dynamic_pointer_cast<UsdSceneItem>(
        Ufe::Hierarchy::createItem(_createMaterialCmd->newUfePath()));
    _createShaderCmd = UsdUndoCreateFromNodeDefCommand::create(
        shaderNodeDef, materialItem, shaderNodeDef->GetFamily().GetString());
    _createShaderCmd->execute();
    if (!_createShaderCmd->insertedChild()) {
        TF_RUNTIME_ERROR(
            "Could not create %s shader under %s",
            _nodeId.c_str(),
            _createMaterialCmd->newUfePath().string().c_str());
        return;
    }
    //
    // 4. Connect the Shader to the material:
    //
    if (!connectShaderToMaterial(
            _createShaderCmd->insertedChild(), _createMaterialCmd->newPrim())) {
        return;
    }
    //
    // 5. Bind the material to the parent primitive:
    //
    _bindCmd = std::make_shared<BindMaterialUndoableCommand>(
        parentItem->prim(), materialItem->prim().GetPath());
    _bindCmd->execute();

    // Successfully completed the task:
    guard.markAsCompleted();
}

void UsdUndoAssignNewMaterialCommand::undo()
{
    // Reverse order of creation/execution:
    if (_bindCmd) {
        _bindCmd->undo();
    }
    // We do not need to undo the connection of the shader to the surface output
    // We do not need to undo the creation of the surface shader output
    if (_createShaderCmd) {
        _createShaderCmd->undo();
    }
    if (_createMaterialCmd) {
        _createMaterialCmd->undo();
    }
    if (_createScopeCmd) {
        _renameScopeCmd->undo();
        _createScopeCmd->undo();
    }
}

void UsdUndoAssignNewMaterialCommand::redo()
{
    if (_createScopeCmd) {
        _createScopeCmd->redo();
        _renameScopeCmd->redo();
    }
    _createMaterialCmd->redo();
    _createShaderCmd->redo();
    connectShaderToMaterial(_createShaderCmd->insertedChild(), _createMaterialCmd->newPrim());
    _bindCmd->redo();
}

bool UsdUndoAssignNewMaterialCommand::connectShaderToMaterial(
    Ufe::SceneItem::Ptr shaderItem,
    UsdPrim             materialPrim)
{
    auto                 shaderUsdItem = std::dynamic_pointer_cast<UsdSceneItem>(shaderItem);
    auto                 shaderPrim = UsdShadeShader(shaderUsdItem->prim());
    UsdShadeOutput       materialOutput;
    PXR_NS::SdrRegistry& registry = PXR_NS::SdrRegistry::GetInstance();
    PXR_NS::SdrShaderNodeConstPtr shaderNodeDef
        = registry.GetShaderNodeByIdentifier(TfToken(_nodeId));
    SdrShaderPropertyConstPtr shaderOutputDef;
    if (shaderNodeDef->GetSourceType() == "glslfx") {
        materialOutput = UsdShadeMaterial(materialPrim).CreateSurfaceOutput();
        shaderOutputDef = shaderNodeDef->GetShaderOutput(TfToken("surface"));
    } else {
        if (shaderNodeDef->GetOutputNames().size() != 1) {
            TF_RUNTIME_ERROR(
                "Cannot resolve which output of shader %s should be connected to surface",
                _nodeId.c_str());
            return false;
        }
        materialOutput
            = UsdShadeMaterial(materialPrim).CreateSurfaceOutput(shaderNodeDef->GetSourceType());
        shaderOutputDef = shaderNodeDef->GetShaderOutput(shaderNodeDef->GetOutputNames()[0]);
    }
    UsdShadeOutput shaderOutput = shaderPrim.CreateOutput(
        shaderOutputDef->GetName(), shaderOutputDef->GetTypeAsSdfType().first);
    UsdShadeConnectableAPI::ConnectToSource(materialOutput, shaderOutput);
    return true;
}
#endif
} // namespace ufe
} // namespace MAYAUSD_NS_DEF
