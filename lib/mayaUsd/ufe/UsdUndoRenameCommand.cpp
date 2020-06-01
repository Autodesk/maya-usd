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

/*
    HS, May 15, 2020

    See usd-interest: Question around SdfPrimSepc's SetName routine

    SdfPrimSpec::SetName() will rename any prim in the layer, but it does not allow you to reparent the prim, 
    nor will it update any relationship or connection targets in the layer that targeted the prim or any of its 
    decendants (they will all break unless you fix them up yourself.Renaming and reparenting prims destructively 
    in composed scenes is pretty tricky stuff that cannot really practically be done with 100% guarantees.
*/

UsdUndoRenameCommand::UsdUndoRenameCommand(const UsdSceneItem::Ptr& srcItem, const Ufe::PathComponent& newName)
    : Ufe::UndoableCommand()
    , _ufeSrcItem(srcItem)
    , _ufeDstItem(nullptr)
    , _stage(_ufeSrcItem->prim().GetStage())
    , _newName(newName.string())
{
    const UsdPrim& prim = _stage->GetPrimAtPath(_ufeSrcItem->prim().GetPath());

    // if the current layer doesn't have any contributions
    if (!MayaUsdUtils::doesEditTargetLayerContribute(prim)) {
        auto strongestContributingLayer = MayaUsdUtils::strongestContributingLayer(prim);
        std::string err = TfStringPrintf("Cannot rename [%s] defined on another layer. " 
                                         "Please set [%s] as the target layer to proceed", 
                                         prim.GetName().GetString().c_str(),
                                         strongestContributingLayer->GetDisplayName().c_str());
        throw std::runtime_error(err.c_str());
    }
    else
    {
        // account for internal vs external references
        // internal references (references without a file path specified) from the same file
        // should be renamable.
        if (prim.HasAuthoredReferences()) {
            auto primSpec = MayaUsdUtils::getPrimSpecAtEditTarget(prim);

            if(!MayaUsdUtils::isInternalReference(primSpec)) {
                std::string err = TfStringPrintf("Unable to rename referenced object [%s]", 
                                                  prim.GetName().GetString().c_str());
                throw std::runtime_error(err.c_str());
            }
        }

        auto layers = MayaUsdUtils::layersWithContribution(prim);
        // if we have more than 2 layers that contributes to the final composed prim
        if (layers.size() > 1) {
            std::string layerDisplayNames;
            for (auto layer : layers) {
                layerDisplayNames.append("[" + layer->GetDisplayName() + "]" + ",");
            }
            layerDisplayNames.pop_back();
            std::string err = TfStringPrintf("Cannot rename [%s] with definitions or opinions on other layers. "
                                             "Opinions exist in %s", prim.GetName().GetString().c_str(), layerDisplayNames.c_str());
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
    const UsdPrim& prim = _stage->GetPrimAtPath(_ufeSrcItem->prim().GetPath());

    auto primSpec = MayaUsdUtils::getPrimSpecAtEditTarget(prim);
    if(!primSpec) {
        return false;
    }

    // these two lines MUST be called before the set name
    // _stage->GetDefaultPrim() and prim after the rename can be invalid.
    auto primPath = prim.GetPath();
    auto defaultPrimPath = _stage->GetDefaultPrim().GetPath();

    // set prim's name
    // XXX: SetName successfuly returns true but when you examine the _prim.GetName()
    // after the rename, the prim name shows the original name HS, 6-May-2020.
    bool status = primSpec->SetName(_newName);
    if (!status) {
        return false;
    }

    // the renamed scene item is a "sibling" of its original name.
    _ufeDstItem = createSiblingSceneItem(_ufeSrcItem->path(), _newName);
    sendRenameNotification(_ufeDstItem, _ufeSrcItem->path());

    // SdfLayer is a "simple" container, and all it knows about defaultPrim is that it is a piece of token-valued layer metadata.  
    // It is only the higher-level Usd and Pcp modules that know that it is identifying a prim on the stage.  
    // One must use the SdfLayer API for setting the defaultPrim when you rename the prim it identifies.
    if(primPath == defaultPrimPath){
        _stage->SetDefaultPrim(_ufeDstItem->prim());
    }

    return true;
}

bool UsdUndoRenameCommand::renameUndo()
{
    const UsdPrim& prim = _stage->GetPrimAtPath(_ufeDstItem->prim().GetPath());

    auto primSpec = MayaUsdUtils::getPrimSpecAtEditTarget(prim);
    if(!primSpec) {
        return false;
    }

    // these two lines MUST be called before the set name
    // _stage->GetDefaultPrim() and prim after the rename can be invalid.
    auto primPath = prim.GetPath();
    auto defaultPrimPath = _stage->GetDefaultPrim().GetPath();

    // set prim's name
    bool status = primSpec->SetName(_ufeSrcItem->prim().GetName());
    if (!status) {
        return false;
    }

    // shouldn't have to again create a sibling sceneItem here since we already have a valid _ufeSrcItem
    // however, I get random crashes if I don't which needs furthur investigation.  HS, 6-May-2020.
    _ufeSrcItem = createSiblingSceneItem(_ufeDstItem->path(), _ufeSrcItem->prim().GetName());
    sendRenameNotification(_ufeSrcItem, _ufeDstItem->path());

    // SdfLayer is a "simple" container, and all it knows about defaultPrim is that it is a piece of token-valued layer metadata.  
    // It is only the higher-level Usd and Pcp modules that know that it is identifying a prim on the stage.  
    // One must use the SdfLayer API for setting the defaultPrim when you rename the prim it identifies.
    if (primPath == defaultPrimPath) {
        _stage->SetDefaultPrim(_ufeSrcItem->prim());
    }

    _ufeDstItem = nullptr;

    return true;
}

void UsdUndoRenameCommand::undo()
{
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
