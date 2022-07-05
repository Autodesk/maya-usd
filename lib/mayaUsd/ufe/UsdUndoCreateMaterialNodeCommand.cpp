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
#include "UsdUndoCreateMaterialNodeCommand.h"

#include <mayaUsd/ufe/UsdUndoAddNewPrimCommand.h>
#include <mayaUsd/ufe/UsdUndoCreateFromNodeDefCommand.h>

#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/attributes.h>
#include <ufe/connectionUndoableCommands.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoCreateMaterialNodeCommand::UsdUndoCreateMaterialNodeCommand(
    const UsdSceneItem::Ptr& parentItem,
    const std::string& materialBaseName,
    const std::string& materialNodeType,
    const Ufe::PathComponent& materialNodeName)
    : Ufe::InsertChildCommand()
    , _parentItem(parentItem)
    , _materialBaseName(materialBaseName)
    , _materialNodeType(materialNodeType)
    , _materialNodeName(materialNodeName)
    , _materialNodeCompositeCmd(std::make_shared<Ufe::CompositeUndoableCommand>())
{
}

UsdUndoCreateMaterialNodeCommand::~UsdUndoCreateMaterialNodeCommand() { }

UsdUndoCreateMaterialNodeCommand::Ptr UsdUndoCreateMaterialNodeCommand::create(
    const UsdSceneItem::Ptr& parentItem,
    const std::string& materialBaseName,
    const std::string& materialNodeType,
    const Ufe::PathComponent& materialNodeName)
{
    // Changing the hierarchy of invalid items is not allowed.
    if (!parentItem || !parentItem->prim().IsActive())
        return nullptr;

    return std::make_shared<UsdUndoCreateMaterialNodeCommand>(parentItem, materialBaseName, materialNodeType, materialNodeName);
}

Ufe::SceneItem::Ptr UsdUndoCreateMaterialNodeCommand::insertedChild() const { return _materialNodeItem; }

void UsdUndoCreateMaterialNodeCommand::execute()
{
    const auto materialCmd = UsdUndoAddNewPrimCommand::create(_parentItem, _materialBaseName, "Material");
    _materialNodeCompositeCmd->append(materialCmd);
    materialCmd->execute();
    _materialItem = UsdSceneItem::create(materialCmd->newUfePath(), materialCmd->newPrim());
    PXR_NS::SdrRegistry &registry = PXR_NS::SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr shaderNode = registry.GetShaderNodeByIdentifier(PXR_NS::TfToken(_materialNodeType.c_str()));
    if (shaderNode) {
        const auto materialNodeCmd = UsdUndoCreateFromNodeDefCommand::create(
            shaderNode, _materialItem, _materialNodeName);
        _materialNodeCompositeCmd->append(materialNodeCmd);
        materialNodeCmd->execute();
        _materialNodeItem = materialNodeCmd->insertedChild();
        Ufe::Attributes::Ptr materialAttributes = Ufe::Attributes::attributes(_materialItem);
        Ufe::Attributes::Ptr materialNodeAttributes = Ufe::Attributes::attributes(_materialNodeItem);
        Ufe::ConnectCommand::Ptr connectCommand = std::make_shared<Ufe::ConnectCommand>(
            materialNodeAttributes->attribute("out"), materialAttributes->attribute("surface"));
        _materialNodeCompositeCmd->append(connectCommand);
        connectCommand->execute();
    }
}

void UsdUndoCreateMaterialNodeCommand::undo() { _materialNodeCompositeCmd->undo(); }

void UsdUndoCreateMaterialNodeCommand::redo() { _materialNodeCompositeCmd->redo(); }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
