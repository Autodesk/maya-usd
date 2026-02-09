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
#include "ProxyShapeContextOpsHandler.h"

#include <mayaUsd/ufe/MayaUsdContextOpsHandler.h>
#include <mayaUsd/ufe/Utils.h>

#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/usd/usd/stage.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(Ufe::ContextOpsHandler, ProxyShapeContextOpsHandler);

ProxyShapeContextOpsHandler::ProxyShapeContextOpsHandler(
    const Ufe::ContextOpsHandler::Ptr& mayaContextOpsHandler)
    : Ufe::ContextOpsHandler()
    , _mayaContextOpsHandler(mayaContextOpsHandler)
{
}

/*static*/
ProxyShapeContextOpsHandler::Ptr
ProxyShapeContextOpsHandler::create(const Ufe::ContextOpsHandler::Ptr& mayaContextOpsHandler)
{
    return std::make_shared<ProxyShapeContextOpsHandler>(mayaContextOpsHandler);
}

//------------------------------------------------------------------------------
// Ufe::ContextOpsHandler overrides
//------------------------------------------------------------------------------

Ufe::ContextOps::Ptr ProxyShapeContextOpsHandler::contextOps(const Ufe::SceneItem::Ptr& item) const
{
    if (isAGatewayType(UsdUfe::getSceneItemNodeType(item))) {
        // UsdContextOps expects a UsdSceneItem which wraps a prim, so
        // create one using the pseudo-root and our own path.
        PXR_NS::UsdStageWeakPtr stage = getStage(item->path());
        if (stage) {
            auto usdItem = UsdUfe::UsdSceneItem::create(item->path(), stage->GetPseudoRoot());
            auto usdContextOpsHandler = MayaUsdContextOpsHandler::create();
            auto cOps = usdContextOpsHandler->contextOps(usdItem);
            auto usdCOps = std::dynamic_pointer_cast<UsdUfe::UsdContextOps>(cOps);

            // We explicitly set the context ops as a gateway type because we created a
            // new UsdSceneItem with a USD prim. Thus the scene item is no longer from
            // the derived class in Maya and we cannot properly query the node type.
            if (usdCOps)
                usdCOps->setIsAGatewayType(true);
            return cOps;
        }

        return nullptr;
    } else if (_mayaContextOpsHandler) {
        return _mayaContextOpsHandler->contextOps(item);
    }

    // Context ops handler is not mandatory.
    return nullptr;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
