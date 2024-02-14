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
#ifndef USD_UNDO_SET_SCENE_ITEM_METADATA_COMMAND_H
#define USD_UNDO_SET_SCENE_ITEM_METADATA_COMMAND_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/UsdUndoableCommand.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/sceneItem.h>
#include <ufe/undoableCommand.h>
#include <ufe/value.h>

namespace USDUFE_NS_DEF {

//! \brief Undoable command to set meta data on a scene item as custom data
//

class USDUFE_PUBLIC SetSceneItemMetadataCommand
    : public UsdUfe::UsdUndoableCommand<Ufe::UndoableCommand>
{
public:
    SetSceneItemMetadataCommand(
        const PXR_NS::UsdPrim& prim,
        const std::string&     group,
        const std::string&     key,
        const Ufe::Value&      value);

    SetSceneItemMetadataCommand(
        const PXR_NS::UsdPrim& prim,
        const std::string&     key,
        const Ufe::Value&      value);

    void executeImplementation() override;

private:
    const PXR_NS::UsdStageWeakPtr _stage;
    const PXR_NS::SdfPath         _primPath;
    const PXR_NS::TfToken         _group;
    const std::string             _key;
    const Ufe::Value              _value;
};

} // namespace USDUFE_NS_DEF
#endif
