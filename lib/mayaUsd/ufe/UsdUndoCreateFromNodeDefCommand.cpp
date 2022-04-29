//
// Copyright 2022 Autodesk
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
#include "UsdUndoCreateFromNodeDefCommand.h"

#include <mayaUsd/ufe/UsdUndoAddNewPrimCommand.h>

#include <pxr/usd/kind/registry.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdShade/shader.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoCreateFromNodeDefCommand::UsdUndoCreateFromNodeDefCommand(
    const PXR_NS::SdrShaderNodeConstPtr shaderNodeDef,
    const UsdSceneItem::Ptr&            parentItem,
    const Ufe::PathComponent&           name)
    : Ufe::InsertChildCommand()
    , _shaderNodeDef(shaderNodeDef)
    , _addPrimCmd(UsdUndoAddNewPrimCommand::create(parentItem, name.string(), "Shader"))
{
}

UsdUndoCreateFromNodeDefCommand::~UsdUndoCreateFromNodeDefCommand() { }

UsdUndoCreateFromNodeDefCommand::Ptr UsdUndoCreateFromNodeDefCommand::create(
    const PXR_NS::SdrShaderNodeConstPtr shaderNodeDef,
    const UsdSceneItem::Ptr&            parentItem,
    const Ufe::PathComponent&           name)
{
    return std::make_shared<UsdUndoCreateFromNodeDefCommand>(shaderNodeDef, parentItem, name);
}

Ufe::SceneItem::Ptr UsdUndoCreateFromNodeDefCommand::insertedChild() const
{
    return UsdSceneItem::create(_addPrimCmd->newUfePath(), _addPrimCmd->newPrim());
}

void UsdUndoCreateFromNodeDefCommand::execute()
{
    _addPrimCmd->execute();

    UsdShadeShader shader(_addPrimCmd->newPrim());
    shader.CreateIdAttr(VtValue(_shaderNodeDef->GetIdentifier()));
}

void UsdUndoCreateFromNodeDefCommand::undo() { _addPrimCmd->undo(); }

void UsdUndoCreateFromNodeDefCommand::redo()
{
    _addPrimCmd->redo();

    UsdShadeShader shader(_addPrimCmd->newPrim());
    shader.CreateIdAttr(VtValue(_shaderNodeDef->GetIdentifier()));
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
