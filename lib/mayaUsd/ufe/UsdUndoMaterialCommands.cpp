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

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/UsdUndoBlock.h>

#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/sdr/shaderProperty.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <ufe/pathString.h>
#include <ufe/sceneItemOps.h>
#include <ufe/selection.h>

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {

// Pixar uses "Looks" to name the materials scope. The USD asset workgroup recommendations is to
// use "mtl" instead. So we will go with the WG recommendation when creating new material scopes.
static const std::string kDefaultMaterialScopeName("mtl");

#if (UFE_PREVIEW_VERSION_NUM >= 4010)
bool connectShaderToMaterial(
    Ufe::SceneItem::Ptr shaderItem,
    UsdPrim             materialPrim,
    const std::string&  nodeId)
{
    auto shaderUsdItem = std::dynamic_pointer_cast<UsdSceneItem>(shaderItem);
    if (!shaderUsdItem) {
        return false;
    }
    auto                          shaderPrim = UsdShadeShader(shaderUsdItem->prim());
    UsdShadeOutput                materialOutput;
    PXR_NS::SdrRegistry&          registry = PXR_NS::SdrRegistry::GetInstance();
    PXR_NS::SdrShaderNodeConstPtr shaderNodeDef
        = registry.GetShaderNodeByIdentifier(TfToken(nodeId));
    if (!shaderNodeDef) {
        return false;
    }
    SdrShaderPropertyConstPtr shaderOutputDef;
    if (shaderNodeDef->GetSourceType() == "glslfx") {
        materialOutput = UsdShadeMaterial(materialPrim).CreateSurfaceOutput();
        shaderOutputDef = shaderNodeDef->GetShaderOutput(TfToken("surface"));
    } else {
        if (shaderNodeDef->GetOutputNames().size() != 1) {
            TF_RUNTIME_ERROR(
                "Cannot resolve which output of shader %s should be connected to surface",
                nodeId.c_str());
            return false;
        }
        materialOutput
            = UsdShadeMaterial(materialPrim).CreateSurfaceOutput(shaderNodeDef->GetSourceType());
        shaderOutputDef = shaderNodeDef->GetShaderOutput(shaderNodeDef->GetOutputNames()[0]);
    }
    if (!shaderOutputDef) {
        return false;
    }
    UsdShadeOutput shaderOutput = shaderPrim.CreateOutput(
        shaderOutputDef->GetName(), shaderOutputDef->GetTypeAsSdfType().first);
    if (!shaderOutput) {
        return false;
    }
    UsdShadeConnectableAPI::ConnectToSource(materialOutput, shaderOutput);
    return true;
}
#endif

bool _BindMaterialCompatiblePrim(const UsdPrim& usdPrim)
{
    if (UsdShadeNodeGraph(usdPrim) || UsdShadeShader(usdPrim)) {
        // The binding schema can be applied anywhere, but it makes no sense on a
        // material or a shader.
        return false;
    }
    if (UsdGeomScope(usdPrim) && usdPrim.GetName() == kDefaultMaterialScopeName) {
        return false;
    }
    if (auto subset = UsdGeomSubset(usdPrim)) {
        TfToken elementType;
        subset.GetElementTypeAttr().Get(&elementType);
        if (elementType != UsdGeomTokens->face) {
            return false;
        }
    }
    if (PXR_NS::UsdShadeMaterialBindingAPI::CanApply(usdPrim)) {
        return true;
    }
    return false;
}

} // namespace

bool BindMaterialUndoableCommand::CompatiblePrim(const Ufe::SceneItem::Ptr& item)
{
    auto usdItem = std::dynamic_pointer_cast<const MAYAUSD_NS::ufe::UsdSceneItem>(item);
    if (!usdItem) {
        return false;
    }
    return _BindMaterialCompatiblePrim(usdItem->prim());
}

BindMaterialUndoableCommand::BindMaterialUndoableCommand(
    Ufe::Path      primPath,
    const SdfPath& materialPath)
    : _primPath(std::move(primPath))
    , _materialPath(materialPath)
{
    auto prim = ufePathToPrim(_primPath);
    if (!prim.IsValid()) {
        std::string err = TfStringPrintf(
            "Invalid primitive path [%s]. Can not bind material.",
            Ufe::PathString::string(_primPath).c_str());
        throw std::runtime_error(err);
    }
    if (!_BindMaterialCompatiblePrim(prim)) {
        std::string err = TfStringPrintf(
            "Invalid primitive type for binding [%s]. Can not bind material.",
            Ufe::PathString::string(_primPath).c_str());
        throw std::runtime_error(err);
    }
    if (_materialPath.IsEmpty()
        || !UsdShadeMaterial(prim.GetStage()->GetPrimAtPath(_materialPath))) {
        std::string err = TfStringPrintf(
            "Invalid material path [%s]. Can not bind material.",
            _materialPath.GetAsString().c_str());
        throw std::runtime_error(err);
    }
}

BindMaterialUndoableCommand::~BindMaterialUndoableCommand() { }

void BindMaterialUndoableCommand::undo() { _undoableItem.undo(); }

void BindMaterialUndoableCommand::redo() { _undoableItem.redo(); }

void BindMaterialUndoableCommand::execute()
{
    // All validations were done in the CTOR: proceed.

    UsdUndoBlock undoBlock(&_undoableItem);

    auto             prim = ufePathToPrim(_primPath);
    UsdShadeMaterial material(prim.GetStage()->GetPrimAtPath(_materialPath));

    if (auto subset = UsdGeomSubset(prim)) {
        subset.GetFamilyNameAttr().Set(UsdShadeTokens->materialBind);
    }

    auto bindingAPI = UsdShadeMaterialBindingAPI::Apply(prim);
    bindingAPI.Bind(material);
}

const std::string BindMaterialUndoableCommand::commandName("Assign Material");

UnbindMaterialUndoableCommand::UnbindMaterialUndoableCommand(Ufe::Path primPath)
    : _primPath(std::move(primPath))
{
    if (_primPath.empty() || !ufePathToPrim(_primPath).IsValid()) {
        std::string err = TfStringPrintf(
            "Invalid primitive path [%s]. Can not unbind material.",
            Ufe::PathString::string(_primPath).c_str());
        throw std::runtime_error(err);
    }
}

UnbindMaterialUndoableCommand::~UnbindMaterialUndoableCommand() { }

void UnbindMaterialUndoableCommand::undo() { _undoableItem.undo(); }

void UnbindMaterialUndoableCommand::redo() { _undoableItem.redo(); }

void UnbindMaterialUndoableCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    auto prim = ufePathToPrim(_primPath);
    auto bindingAPI = UsdShadeMaterialBindingAPI(prim);
    if (bindingAPI) {
        bindingAPI.UnbindDirectBinding();
    }
}
const std::string UnbindMaterialUndoableCommand::commandName("Unassign Material");

#if (UFE_PREVIEW_VERSION_NUM >= 4010)
UsdUndoAssignNewMaterialCommand::UsdUndoAssignNewMaterialCommand(
    const UsdSceneItem::Ptr& parentItem,
    const std::string&       nodeId)
    : Ufe::InsertChildCommand()
    , _nodeId(nodeId)
    , _cmds(std::make_shared<Ufe::CompositeUndoableCommand>())
{
    if (!parentItem || !parentItem->prim().IsActive())
        return;

    Ufe::Path             parentPath = parentItem->path();
    const UsdStageWeakPtr stage = getStage(parentPath);
    _stagesAndPaths[stage].push_back(parentPath);
}

UsdUndoAssignNewMaterialCommand::UsdUndoAssignNewMaterialCommand(
    const Ufe::Selection& parentItems,
    const std::string&    nodeId)
    : Ufe::InsertChildCommand()
    , _nodeId(nodeId)
    , _cmds(std::make_shared<Ufe::CompositeUndoableCommand>())
{
    for (const auto& parentItem : parentItems) {
        const UsdSceneItem::Ptr usdSceneItem = std::dynamic_pointer_cast<UsdSceneItem>(parentItem);
        if (!usdSceneItem || !usdSceneItem->prim().IsActive())
            continue;

        Ufe::Path             parentPath = usdSceneItem->path();
        const UsdStageWeakPtr stage = getStage(parentPath);
        _stagesAndPaths[stage].push_back(parentPath);
    }
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

UsdUndoAssignNewMaterialCommand::Ptr UsdUndoAssignNewMaterialCommand::create(
    const Ufe::Selection& parentItems,
    const std::string&    nodeId)
{
    return std::make_shared<UsdUndoAssignNewMaterialCommand>(parentItems, nodeId);
}

Ufe::SceneItem::Ptr UsdUndoAssignNewMaterialCommand::insertedChild() const
{
    // This is broken. Since PR 2641 we now loop over the selection to handle multiple stages.
    // This command returns a single inserted child, while this new implementation can now create
    // multiple shaders. This will have to be fixed at a higher level.
    // There is still a shader creation command directly after the command at _createMaterialCmdIdx,
    // but it will be the last created shader. Still better than nothing, and works correctly in
    // the most common workflow where selection covers a single stage.
    if (_cmds) {
        auto cmdsIt = _cmds->cmdsList().begin();
        std::advance(cmdsIt, _createMaterialCmdIdx + 1);
        auto addShaderCmd = std::dynamic_pointer_cast<UsdUndoCreateFromNodeDefCommand>(*cmdsIt);
        return addShaderCmd->insertedChild();
    }
    return {};
}

std::string UsdUndoAssignNewMaterialCommand::resolvedMaterialScopeName()
{
    return UsdMayaJobExportArgs::GetDefaultMaterialsScopeName();
}

void UsdUndoAssignNewMaterialCommand::execute()
{
    std::string materialsScopeNamePrefix = resolvedMaterialScopeName();

    // Materials cannot be shared between stages. So we create a unique material per stage,
    // which can then be shared between any number of objects within that stage.
    for (const auto& selectedInStage : _stagesAndPaths) {
        const auto& stage = selectedInStage.first;
        const auto& selectedPaths = selectedInStage.second;
        if (!stage) {
            markAsFailed();
            return;
        }
        if (selectedPaths.empty()) {
            markAsFailed();
            return;
        }

        //
        // 1. Create the Scope "materials" if it does not exist:
        //
        auto stagePath = selectedPaths[0].popSegment();
        auto stageHierarchy = Ufe::Hierarchy::hierarchy(Ufe::Hierarchy::createItem(stagePath));
        if (!stageHierarchy) {
            markAsFailed();
            return;
        }

        // Find an available materials scope name.
        // Usually the materials scope will simply have the default name (e.g. "mtl"). However, if
        // that name is used by a non-scope object, a number should be appended (e.g. "mtl1"). If
        // this name is not available either, increment the number until an available name is found.
        Ufe::SceneItem::Ptr materialsScope = nullptr;
        std::string         materialsScopeName = materialsScopeNamePrefix;
        Ufe::SceneItemList  children = stageHierarchy->children();
        for (size_t i = 1;; ++i) {
            auto hasName = [&materialsScopeName](const Ufe::SceneItem::Ptr& item) {
                return item->nodeName() == materialsScopeName;
            };
            auto childrenIterator = std::find_if(children.begin(), children.end(), hasName);
            if (childrenIterator == children.end()) {
                break;
            }
            if ((*childrenIterator)->nodeType() == "Scope") {
                materialsScope = *childrenIterator;
                break;
            }

            // Name is already used by something that is not a scope. Try the next name.
            materialsScopeName = materialsScopeNamePrefix + std::to_string(i);
        }

        if (!materialsScope) {
            auto createScopeCmd = UsdUndoAddNewPrimCommand::create(
                UsdSceneItem::create(MayaUsd::ufe::stagePath(stage), stage->GetPseudoRoot()),
                materialsScopeName,
                "Scope");
            if (!createScopeCmd) {
                markAsFailed();
                return;
            }
            createScopeCmd->execute();
            _cmds->append(createScopeCmd);
            auto scopePath = createScopeCmd->newUfePath();
            // The code automatically appends a "1". We need to rename:
            auto itemOps = Ufe::SceneItemOps::sceneItemOps(Ufe::Hierarchy::createItem(scopePath));
            if (!itemOps) {
                markAsFailed();
                return;
            }
            auto rename = itemOps->renameItemCmd(Ufe::PathComponent(materialsScopeName));
            if (!rename.undoableCommand) {
                markAsFailed();
                return;
            }
            _cmds->append(rename.undoableCommand);
            materialsScope = rename.item;
        }
        if (!materialsScope || materialsScope->path().empty()) {
            // The _createScopeCmd and/or _renameScopeCmd will have emitted errors.
            markAsFailed();
            return;
        }

        //
        // 2. Create the Material:
        //
        PXR_NS::SdrRegistry&          registry = PXR_NS::SdrRegistry::GetInstance();
        PXR_NS::SdrShaderNodeConstPtr shaderNodeDef
            = registry.GetShaderNodeByIdentifier(TfToken(_nodeId));
        if (!shaderNodeDef) {
            TF_RUNTIME_ERROR("Unknown shader identifier: %s", _nodeId.c_str());
            markAsFailed();
            return;
        }
        if (shaderNodeDef->GetOutputNames().empty()) {
            TF_RUNTIME_ERROR("Surface shader %s does not have any outputs", _nodeId.c_str());
            markAsFailed();
            return;
        }
        auto scopeItem = std::dynamic_pointer_cast<UsdSceneItem>(materialsScope);
        auto createMaterialCmd = UsdUndoAddNewPrimCommand::create(
            scopeItem, shaderNodeDef->GetFamily().GetString(), "Material");
        if (!createMaterialCmd) {
            markAsFailed();
            return;
        }
        createMaterialCmd->execute();
        _createMaterialCmdIdx = _cmds->cmdsList().size();
        _cmds->append(createMaterialCmd);
        if (!createMaterialCmd->newPrim()) {
            // The _createMaterialCmd will have emitted errors.
            markAsFailed();
            return;
        }

        //
        // 3. Create the Shader:
        //
        UsdSceneItem::Ptr materialItem = std::dynamic_pointer_cast<UsdSceneItem>(
            Ufe::Hierarchy::createItem(createMaterialCmd->newUfePath()));
        auto createShaderCmd = UsdUndoCreateFromNodeDefCommand::create(
            shaderNodeDef, materialItem, shaderNodeDef->GetFamily().GetString());
        if (!createShaderCmd) {
            markAsFailed();
            return;
        }
        createShaderCmd->execute();
        _cmds->append(createShaderCmd);
        if (!createShaderCmd->insertedChild()) {
            // The _createShaderCmd will have emitted errors.
            markAsFailed();
            return;
        }

        //
        // 4. Connect the Shader to the material:
        //
        if (!connectShaderToMaterial(
                createShaderCmd->insertedChild(), createMaterialCmd->newPrim(), _nodeId)) {
            markAsFailed();
            return;
        }

        //
        // 5. Bind the material to all selected primitives in the stage:
        //
        for (const auto& parentPath : selectedPaths) {
            auto parentItem
                = std::dynamic_pointer_cast<UsdSceneItem>(Ufe::Hierarchy::createItem(parentPath));
            if (!parentItem) {
                markAsFailed();
                return;
            }
            // There might be some unassignable items in the selection list. Skip and warn.
            // We know there is at least one assignable item found in the ContextOps resolver.
            if (!BindMaterialUndoableCommand::CompatiblePrim(parentItem)) {
                const std::string error = TfStringPrintf(
                    "Assign new material: Skipping incompatible prim [%s] found in selection.",
                    Ufe::PathString::string(parentItem->path()).c_str());
                TF_WARN("%s", error.c_str());
                continue;
            }
            auto bindCmd = std::make_shared<BindMaterialUndoableCommand>(
                parentItem->path(), materialItem->prim().GetPath());
            if (!bindCmd) {
                markAsFailed();
                return;
            }
            bindCmd->execute();
            _cmds->append(bindCmd);
        }
    }
}

void UsdUndoAssignNewMaterialCommand::undo()
{
    if (_cmds) {
        _cmds->undo();
    }
}

void UsdUndoAssignNewMaterialCommand::redo()
{
    if (_cmds) {
        _cmds->redo();

        auto cmdsIt = _cmds->cmdsList().begin();
        while (cmdsIt != _cmds->cmdsList().end()) {
            // Find out all Material creation followed by a shader creation and reconnect the
            // shader to the material. Don't assume any ordering.
            auto addMaterialCmd
                = std::dynamic_pointer_cast<MAYAUSD_NS::ufe::UsdUndoAddNewPrimCommand>(*cmdsIt++);
            if (addMaterialCmd && addMaterialCmd->newPrim()
                && UsdShadeMaterial(addMaterialCmd->newPrim())
                && cmdsIt != _cmds->cmdsList().end()) {
                auto addShaderCmd
                    = std::dynamic_pointer_cast<UsdUndoCreateFromNodeDefCommand>(*cmdsIt++);
                if (addShaderCmd) {
                    connectShaderToMaterial(
                        addShaderCmd->insertedChild(), addMaterialCmd->newPrim(), _nodeId);
                }
            }
        }
    }
}

void UsdUndoAssignNewMaterialCommand::markAsFailed()
{
    _cmds->undo();
    _cmds.reset();
}

UsdUndoAddNewMaterialCommand::UsdUndoAddNewMaterialCommand(
    const UsdSceneItem::Ptr& parentItem,
    const std::string&       nodeId)
    : Ufe::InsertChildCommand()
    , _parentPath((parentItem && parentItem->prim().IsActive()) ? parentItem->path() : Ufe::Path())
    , _nodeId(nodeId)
{
}

UsdUndoAddNewMaterialCommand::~UsdUndoAddNewMaterialCommand() { }

UsdUndoAddNewMaterialCommand::Ptr
UsdUndoAddNewMaterialCommand::create(const UsdSceneItem::Ptr& parentItem, const std::string& nodeId)
{
    // Changing the hierarchy of invalid items is not allowed.
    if (!parentItem || !parentItem->prim().IsActive())
        return nullptr;

    return std::make_shared<UsdUndoAddNewMaterialCommand>(parentItem, nodeId);
}

Ufe::SceneItem::Ptr UsdUndoAddNewMaterialCommand::insertedChild() const
{
    if (_createShaderCmd) {
        return _createShaderCmd->insertedChild();
    }
    return {};
}

bool UsdUndoAddNewMaterialCommand::CompatiblePrim(const Ufe::SceneItem::Ptr& target)
{
    if (!target) {
        return false;
    }

    // Must be a scope.
    if (target->nodeType() != "Scope") {
        return false;
    }

    // With the magic name.
    if (target->nodeName() == UsdUndoAssignNewMaterialCommand::resolvedMaterialScopeName()) {
        return true;
    }

    // Or with only materials inside
    auto scopeHierarchy = Ufe::Hierarchy::hierarchy(target);
    if (scopeHierarchy) {
        for (auto&& child : scopeHierarchy->children()) {
            if (child->nodeType() != "Material") {
                // At least one non material
                return false;
            }
        }
    }

    return true;
}

void UsdUndoAddNewMaterialCommand::execute()
{
    if (_parentPath.empty()) {
        return;
    }

    //
    // Create the Material:
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

    auto scopeItem
        = std::dynamic_pointer_cast<UsdSceneItem>(Ufe::Hierarchy::createItem(_parentPath));
    _createMaterialCmd = UsdUndoAddNewPrimCommand::create(
        scopeItem, shaderNodeDef->GetFamily().GetString(), "Material");
    if (!_createMaterialCmd) {
        return;
    }
    _createMaterialCmd->execute();
    if (!_createMaterialCmd->newPrim()) {
        // The _createMaterialCmd will have emitted errors.
        markAsFailed();
        return;
    }

    //
    // Create the Shader:
    //
    auto materialItem = std::dynamic_pointer_cast<UsdSceneItem>(
        Ufe::Hierarchy::createItem(_createMaterialCmd->newUfePath()));
    _createShaderCmd = UsdUndoCreateFromNodeDefCommand::create(
        shaderNodeDef, materialItem, shaderNodeDef->GetFamily().GetString());
    if (!_createShaderCmd) {
        markAsFailed();
        return;
    }
    _createShaderCmd->execute();
    if (!_createShaderCmd->insertedChild()) {
        // The _createShaderCmd will have emitted errors.
        markAsFailed();
        return;
    }

    //
    // Connect the Shader to the material:
    //
    if (!connectShaderToMaterial(
            _createShaderCmd->insertedChild(), _createMaterialCmd->newPrim(), _nodeId)) {
        markAsFailed();
        return;
    }
}

void UsdUndoAddNewMaterialCommand::undo()
{
    if (_createMaterialCmd) {
        _createShaderCmd->undo();
        _createMaterialCmd->undo();
    }
}

void UsdUndoAddNewMaterialCommand::redo()
{
    if (_createMaterialCmd) {
        _createMaterialCmd->redo();
        _createShaderCmd->redo();

        connectShaderToMaterial(
            _createShaderCmd->insertedChild(), _createMaterialCmd->newPrim(), _nodeId);
    }
}

void UsdUndoAddNewMaterialCommand::markAsFailed()
{
    if (_createShaderCmd) {
        _createShaderCmd->undo();
        _createShaderCmd.reset();
    }
    if (_createMaterialCmd) {
        _createMaterialCmd->undo();
        _createMaterialCmd.reset();
    }
}

#endif
} // namespace ufe
} // namespace MAYAUSD_NS_DEF
