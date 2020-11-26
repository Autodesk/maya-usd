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

#include <mayaUsd/ufe/UsdContextOpsHandler.h>
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/usd/stage.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

ProxyShapeContextOpsHandler::ProxyShapeContextOpsHandler(
    const Ufe::ContextOpsHandler::Ptr& mayaContextOpsHandler)
    : Ufe::ContextOpsHandler()
    , _mayaContextOpsHandler(mayaContextOpsHandler)
{
}

ProxyShapeContextOpsHandler::~ProxyShapeContextOpsHandler() { }

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
    if (isAGatewayType(item->nodeType())) {
        // UsdContextOps expects a UsdSceneItem which wraps a prim, so
        // create one using the pseudo-root and our own path.
        UsdStageWeakPtr stage = getStage(item->path());
        if (stage) {
            auto               usdItem = UsdSceneItem::create(item->path(), stage->GetPseudoRoot());
            auto               usdContextOpsHandler = UsdContextOpsHandler::create();
            auto               cOps = usdContextOpsHandler->contextOps(usdItem);
            UsdContextOps::Ptr usdCOps = std::dynamic_pointer_cast<UsdContextOps>(cOps);

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
