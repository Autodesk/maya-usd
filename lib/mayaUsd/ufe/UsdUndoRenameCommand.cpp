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
#include "UsdUndoRenameCommand.h"

#include <ufe/log.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <mayaUsd/ufe/Utils.h>

#include <mayaUsdUtils/util.h>

#include "private/InPathChange.h"

#ifdef UFE_V2_FEATURES_AVAILABLE
#define UFE_ENABLE_ASSERTS
#include <ufe/ufeAssert.h>
#else
#include <cassert>
#endif

MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoRenameCommand::UsdUndoRenameCommand(const UsdSceneItem::Ptr& srcItem, const Ufe::PathComponent& newName)
    : Ufe::UndoableCommand()
    , _ufeSrcItem(srcItem)
    , _ufeDstItem(nullptr)
    , _prim(srcItem->prim())
    , _stage(_prim.GetStage())
    , _newName(newName.string())
{
    // if the current layer doesn't have any opinions that affects selected prim
    if (!MayaUsdUtils::doesEditTargetLayerHavePrimSpec(_prim)) {
        auto possibleTargetLayer = MayaUsdUtils::strongestLayerWithPrimSpec(_prim);
        std::string err = TfStringPrintf("Cannot rename [%s] defined on another layer. " 
                                         "Please set [%s] as the target layer to proceed", 
                                         _prim.GetName().GetString().c_str(),
                                         possibleTargetLayer->GetDisplayName().c_str());
        throw std::runtime_error(err.c_str());
    }
    else
    {
        auto layers = MayaUsdUtils::layersWithPrimSpec(_prim);

        if (layers.size() > 1) {
            std::string layerDisplayNames;
            for (auto layer : layers) {
                layerDisplayNames.append("[" + layer->GetDisplayName() + "]" + ",");
            }
            layerDisplayNames.pop_back();
            std::string err = TfStringPrintf("Cannot rename [%s] with definitions or opinions on other layers. "
                                             "Opinions exist in %s", _prim.GetName().GetString().c_str(), layerDisplayNames.c_str());
            throw std::runtime_error(err.c_str());
        }
    }
}

UsdUndoRenameCommand::~UsdUndoRenameCommand()
{
}

UsdUndoRenameCommand::Ptr UsdUndoRenameCommand::create(const UsdSceneItem::Ptr& srcItem, const Ufe::PathComponent& newName)
{
    return std::make_shared<UsdUndoRenameCommand>(srcItem, newName);
}

UsdSceneItem::Ptr UsdUndoRenameCommand::renamedItem() const
{
    return _ufeDstItem;
}

bool UsdUndoRenameCommand::renameRedo()
{
    auto primSpec = MayaUsdUtils::getPrimSpecAtEditTarget(_stage, _prim);

    if(primSpec) {

        // set prim's name
        primSpec->SetName(_newName);

         // the renamed scene item is a "sibling" of its original name.
        _ufeDstItem = createSiblingSceneItem(_ufeSrcItem->path(), _newName);

        sendRenameNotification(_ufeDstItem, _ufeSrcItem->path());
    }
    else {
        return false;
    }

    return true;
}

bool UsdUndoRenameCommand::renameUndo()
{
    auto primSpec = MayaUsdUtils::getPrimSpecAtEditTarget(_stage, _ufeDstItem->prim());
 
    if(primSpec) {

        // set prim's name
        primSpec->SetName(_prim.GetPath().GetElementString());

        // shouldn't have to again create a sibling sceneItem here since we already have a valid _ufeSrcItem
        // however, I get random crashes if I don't which needs furthur investigation.  HS, 6-May-2020.
        _ufeSrcItem = createSiblingSceneItem(_ufeDstItem->path(), _prim.GetPath().GetElementString());

        sendRenameNotification(_ufeSrcItem, _ufeDstItem->path());

        _ufeDstItem = nullptr;
    }
    else {
        return false;
    }

    return true;
}

void UsdUndoRenameCommand::undo()
{
    // MAYA-92264: Pixar bug prevents undo from working.  Try again with USD
    // version 0.8.5 or later.  PPT, 7-Jul-2018.
    try {
        InPathChange pc;
        if (!renameUndo()) {
            UFE_LOG("rename undo failed");
        }
    }
    catch (const std::exception& e) {
        UFE_LOG(e.what());
        throw;  // re-throw the same exception
    }
}

void UsdUndoRenameCommand::redo()
{
    InPathChange pc;
    if (!renameRedo()) {
        UFE_LOG("rename redo failed");
    }
}

} // namespace ufe
} // namespace MayaUsd
