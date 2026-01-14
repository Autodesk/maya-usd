//
// Copyright 2024 Autodesk
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

#include "utils.h"

#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Global.h>
#ifdef UFE_V4_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdUndoCreateStageWithNewLayerCommand.h>
#include <mayaUsd/ufe/UsdUndoMaterialCommands.h>
#endif
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <usdUfe/ufe/Global.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/mergePrims.h>
#include <usdUfe/utils/usdUtils.h>

#include <pxr/usd/usd/prim.h>

namespace MAYAUSDAPI_NS_DEF {

Ufe::Rtid getMayaRunTimeId() { return MayaUsd::ufe::getMayaRunTimeId(); }

Ufe::Rtid getUsdRunTimeId() { return UsdUfe::getUsdRunTimeId(); }

std::string getUsdRunTimeName() { return UsdUfe::getUsdRunTimeName(); }

bool isUsdSceneItem(const Ufe::SceneItem::Ptr& item) { return (UsdUfe::downcast(item) != nullptr); }

Ufe::SceneItem::Ptr createUsdSceneItem(const Ufe::Path& path, const PXR_NS::UsdPrim& prim)
{
    return UsdUfe::UsdSceneItem::create(path, prim);
}

PXR_NS::UsdPrim getPrimForUsdSceneItem(const Ufe::SceneItem::Ptr& item)
{
    if (auto usdItem = UsdUfe::downcast(item)) {
        return usdItem->prim();
    }
    return PXR_NS::UsdPrim();
}

PXR_NS::UsdPrim ufePathToPrim(const Ufe::Path& path) { return UsdUfe::ufePathToPrim(path); }

Ufe::PathSegment usdPathToUfePathSegment(const SdfPath& usdPath, int instanceIndex)
{
    return UsdUfe::usdPathToUfePathSegment(usdPath, instanceIndex);
}

PXR_NS::UsdTimeCode getTime(const Ufe::Path& path) { return UsdUfe::getTime(path); }

PXR_NS::UsdStageWeakPtr getStage(const Ufe::Path& path) { return UsdUfe::getStage(path); }

Ufe::Path stagePath(PXR_NS::UsdStageWeakPtr stage) { return UsdUfe::stagePath(stage); }

bool isConnected(const PXR_NS::UsdAttribute& srcUsdAttr, const PXR_NS::UsdAttribute& dstUsdAttr)
{
    return UsdUfe::isConnected(srcUsdAttr, dstUsdAttr);
}

PXR_NS::UsdStageWeakPtr usdStage(const Ufe::Attribute::Ptr& attribute)
{
    if (auto usdAttribute = std::dynamic_pointer_cast<UsdUfe::UsdAttribute>(attribute)) {
        return usdAttribute->usdPrim().GetStage();
    }
    return nullptr;
}

PXR_NS::SdfValueTypeName usdAttributeType(const Ufe::Attribute::Ptr& attribute)
{
    if (auto usdAttribute = std::dynamic_pointer_cast<UsdUfe::UsdAttribute>(attribute)) {
        return usdAttribute->usdAttributeType();
    }
    return PXR_NS::SdfValueTypeName();
}

bool getUsdValue(
    const Ufe::Attribute::Ptr& attribute,
    PXR_NS::VtValue&           value,
    PXR_NS::UsdTimeCode        time)
{
    if (auto usdAttribute = std::dynamic_pointer_cast<UsdUfe::UsdAttribute>(attribute)) {
        return usdAttribute->get(value, time);
    }
    return false;
}

#ifdef UFE_V4_FEATURES_AVAILABLE

Ufe::UndoableCommand::Ptr
addNewMaterialCommand(const Ufe::SceneItem::Ptr& parentItem, const std::string& sdrShaderIdentifier)
{
    if (auto usdSceneItem = UsdUfe::downcast(parentItem)) {
        return MayaUsd::ufe::UsdUndoAddNewMaterialCommand::create(
            usdSceneItem, sdrShaderIdentifier);
    }
    return nullptr;
}

Ufe::UndoableCommand::Ptr createMaterialsScopeCommand(const Ufe::SceneItem::Ptr& parentItem)
{
    if (auto usdSceneItem = UsdUfe::downcast(parentItem)) {
        return MayaUsd::ufe::UsdUndoCreateMaterialsScopeCommand::create(usdSceneItem);
    }
    return nullptr;
}

Ufe::UndoableCommand::Ptr createStageWithNewLayerCommand(const Ufe::SceneItem::Ptr& parentItem)
{
    return MayaUsd::ufe::UsdUndoCreateStageWithNewLayerCommand::create(parentItem);
}

MAYAUSD_API_PUBLIC
Ufe::SceneItemResultUndoableCommand::Ptr createAddNewPrimCommand(
    const Ufe::SceneItem::Ptr& parentItem,
    const std::string&         name,
    const std::string&         type)
{
    if (auto usdSceneItem = UsdUfe::downcast(parentItem)) {
        return UsdUfe::UsdUndoAddNewPrimCommand::create(usdSceneItem, name, type);
    }
    return nullptr;
}

#endif

bool isMaterialsScope(const Ufe::SceneItem::Ptr& item) { return UsdUfe::isMaterialsScope(item); }

bool isAGatewayType(const std::string& mayaNodeType)
{
    return MayaUsd::ufe::isAGatewayType(mayaNodeType);
}

bool mergePrims(
    const PXR_NS::UsdStageRefPtr& srcStage,
    const PXR_NS::SdfLayerRefPtr& srcLayer,
    const PXR_NS::SdfPath&        srcPath,
    const PXR_NS::UsdStageRefPtr& dstStage,
    const PXR_NS::SdfLayerRefPtr& dstLayer,
    const PXR_NS::SdfPath&        dstPath)
{
    return UsdUfe::mergePrims(srcStage, srcLayer, srcPath, dstStage, dstLayer, dstPath);
}

std::string getDir(const std::string& fullFilePath)
{
    return PXR_NS::UsdMayaUtilFileSystem::getDir(fullFilePath);
}

std::pair<std::string, bool>
makePathRelativeTo(const std::string& fileName, const std::string& relativeToDir)
{
    return PXR_NS::UsdMayaUtilFileSystem::makePathRelativeTo(fileName, relativeToDir);
}

bool requireUsdPathsRelativeToEditTargetLayer()
{
    return PXR_NS::UsdMayaUtilFileSystem::requireUsdPathsRelativeToEditTargetLayer();
}

std::string handleAssetPathThatMaybeRelativeToLayer(
    std::string                   fileName,
    const std::string&            attrName,
    const PXR_NS::SdfLayerHandle& layer,
    const std::string&            optionVarName)
{
    return PXR_NS::UsdMayaUtilFileSystem::handleAssetPathThatMaybeRelativeToLayer(
        fileName, attrName, layer, optionVarName);
}

MString getProxyShapeDisplayFilter() { return MayaUsdProxyShapeBase::displayFilterName; }

} // End of namespace MAYAUSDAPI_NS_DEF