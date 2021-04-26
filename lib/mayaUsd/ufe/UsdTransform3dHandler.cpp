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
#include "UsdTransform3dHandler.h"

#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/Utils.h>

#include <maya/MGlobal.h>

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdTransform3dPointInstance.h>
#endif

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdTransform3dHandler::UsdTransform3dHandler()
    : Ufe::Transform3dHandler()
{
}

UsdTransform3dHandler::~UsdTransform3dHandler() { }

/*static*/
UsdTransform3dHandler::Ptr UsdTransform3dHandler::create()
{
    return std::make_shared<UsdTransform3dHandler>();
}

//------------------------------------------------------------------------------
// Ufe::Transform3dHandler overrides
//------------------------------------------------------------------------------

Ufe::Transform3d::Ptr UsdTransform3dHandler::transform3d(const Ufe::SceneItem::Ptr& item) const
{
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
    assert(usdItem);
#endif

    if (usdItem->isPointInstance()) {
        // Point instance manipulation using this handler is only supported
        // with UFE v2. Otherwise, we disallow any manipulation for point
        // instance scene items.
#ifdef UFE_V2_FEATURES_AVAILABLE
        return UsdTransform3dPointInstance::create(usdItem);
#else
        return nullptr;
#endif
    }

    // According to USD docs, editing scene description via instance proxies and their properties is
    // not allowed.
    // https://graphics.pixar.com/usd/docs/api/_usd__page__scenegraph_instancing.html#Usd_ScenegraphInstancing_InstanceProxies
    if (usdItem->prim().IsInstanceProxy()) {
        MGlobal::displayError(
            MString("Authoring to the descendant of an instance [")
            + MString(usdItem->prim().GetName().GetString().c_str()) + MString("] is not allowed.")
            + MString("Please mark 'instanceable=false' to author edits to instance proxies."));
        return nullptr;
    }

    std::string errMsg;
    if (!MayaUsd::ufe::isEditTargetLayerModifiable(usdItem->prim().GetStage(), &errMsg)) {
        MGlobal::displayError(errMsg.c_str());
        return nullptr;
    }

    return UsdTransform3d::create(usdItem);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
