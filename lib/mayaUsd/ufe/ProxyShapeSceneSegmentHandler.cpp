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
#include "ProxyShapeSceneSegmentHandler.h"

#include <mayaUsd/ufe/Utils.h>

#include <ufe/runTimeMgr.h>
#include <ufe/hierarchy.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

extern Ufe::Rtid g_USDRtid;

ProxyShapeSceneSegmentHandler::ProxyShapeSceneSegmentHandler(
    const Ufe::SceneSegmentHandler::Ptr& mayaSceneSegmentHandler)
    : Ufe::SceneSegmentHandler()
    , fMayaSceneSegmentHandler(mayaSceneSegmentHandler)
{
}

ProxyShapeSceneSegmentHandler::~ProxyShapeSceneSegmentHandler() { }

/*static*/
ProxyShapeSceneSegmentHandler::Ptr
ProxyShapeSceneSegmentHandler::create(const Ufe::SceneSegmentHandler::Ptr& mayaSceneSegmentHandler)
{
    return std::make_shared<ProxyShapeSceneSegmentHandler>(mayaSceneSegmentHandler);
}

//------------------------------------------------------------------------------
// Ufe::SceneSegmentHandler overrides
//------------------------------------------------------------------------------

Ufe::Selection ProxyShapeSceneSegmentHandler::findGatewayItems(const Ufe::Path& path) const
{
    Ufe::Selection result = fMayaSceneSegmentHandler ? fMayaSceneSegmentHandler->findGatewayItems(path) : Ufe::Selection();
    for(const auto& stage : getAllStages()) {
        Ufe::Path proxyShapePath = stagePath(stage);

        // This handles gateway nodes that have a mix of children in other scene segments and children in the same
        // segment as the gateway node, as well as gateway nodes with multiple descendent scene segments.
        if (proxyShapePath == path) {
            Ufe::SceneItem::Ptr gatewayItem = Ufe::Hierarchy::createItem(path);
            Ufe::Hierarchy::Ptr gatewayHierarchy = Ufe::Hierarchy::hierarchy(gatewayItem);
            for (const auto& gatewayChild : gatewayHierarchy->children())
            {
                Ufe::SceneSegmentHandler::Ptr sceneSegmentHandler = Ufe::RunTimeMgr::instance().sceneSegmentHandler(gatewayChild->runTimeId());
                Ufe::Selection gatewayItems = sceneSegmentHandler ? sceneSegmentHandler->findGatewayItems(gatewayChild->path()) : Ufe::Selection();
                for(const auto& descendentGatewayItem : gatewayItems) {
                    result.append(descendentGatewayItem);
                }
            }
        } else if (proxyShapePath.startsWith(path)) {
            result.append(Ufe::Hierarchy::createItem(proxyShapePath));
        }
    }
    return result;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
