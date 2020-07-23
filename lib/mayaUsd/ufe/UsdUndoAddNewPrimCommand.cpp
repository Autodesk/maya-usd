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
#include "UsdUndoAddNewPrimCommand.h"

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>

namespace {

Ufe::Path appendToPath(const Ufe::Path& path, const std::string& name)
{
    Ufe::Path newUfePath;
    if (1 == path.getSegments().size()) {
        newUfePath = path + Ufe::PathSegment(
        Ufe::PathComponent(name), MayaUsd::ufe::getUsdRunTimeId(), '/');
    } else {
        newUfePath = path + name;
    }
    return newUfePath;
}

}

MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoAddNewPrimCommand::UsdUndoAddNewPrimCommand(const UsdSceneItem::Ptr& usdSceneItem,
                            const std::string& name, const std::string& type)
: Ufe::UndoableCommand()
{
    // First get the stage from the proxy shape.
    auto ufePath = usdSceneItem->path();
    auto segments = ufePath.getSegments();
    auto dagSegment = segments[0];
    _stage = usdSceneItem->prim().GetStage();
    if (_stage) {
        // Append the parent path and the requested name into a full ufe path.
        _newUfePath = appendToPath(ufePath, name + '1');

        // Ensure the requested name is unique.
        auto newPrimName = uniqueChildName(usdSceneItem, _newUfePath);

        // If the name had to change then we need to update the full ufe path.
        if (name != newPrimName) {
            _newUfePath = appendToPath(ufePath, newPrimName);
        }

        // Build (and store) the usd path for the new prim with the unique name.
        PXR_NS::SdfPath usdItemPath = usdSceneItem->prim().GetPath();
        _primPath = usdItemPath.AppendChild(PXR_NS::TfToken(newPrimName));

        // The type of prim we were asked to create.
        // Note: "Def" means create typeless prim.
        _primToken = (type.empty() || type == "Def") ? TfToken() : TfToken(name);
    }
}

void UsdUndoAddNewPrimCommand::undo()
{
    if (_stage) {
        _stage->RemovePrim(_primPath);
    }
}

void UsdUndoAddNewPrimCommand::redo()
{
    if (_stage) {
        auto prim = _stage->DefinePrim(_primPath, _primToken);
        if (!prim.IsValid())
            TF_RUNTIME_ERROR("Failed to create new prim type: %s", _primToken.GetText());
    }
}

const Ufe::Path& UsdUndoAddNewPrimCommand::newUfePath() const
{
    return _newUfePath;
}

PXR_NS::UsdPrim UsdUndoAddNewPrimCommand::newPrim() const
{
    if (!_stage) {
        return UsdPrim();
    }
    
    return _stage->GetPrimAtPath(_primPath);
}

/*static*/
UsdUndoAddNewPrimCommand::Ptr UsdUndoAddNewPrimCommand::create(const UsdSceneItem::Ptr& usdSceneItem,
                            const std::string& name, const std::string& type)
{
	return std::make_shared<UsdUndoAddNewPrimCommand>(usdSceneItem, name, type);
}

} // namespace ufe
} // namespace MayaUsd
