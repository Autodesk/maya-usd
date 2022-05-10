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

#include <ufe/hierarchy.h>
#include <ufe/runTimeMgr.h>

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

Ufe::Selection ProxyShapeSceneSegmentHandler::findGatewayItems_(const Ufe::Path& path) const
{
    // Handle other gateway node types that MayaUSD is not aware of
    Ufe::Selection result = fMayaSceneSegmentHandler
        ? fMayaSceneSegmentHandler->findGatewayItems_(path)
        : Ufe::Selection();

    // Find the MayaUSD proxyShapes
    for (const auto& stage : getAllStages()) {
        Ufe::Path proxyShapePath = stagePath(stage);
        // recall that findGatewayItems searches for descendents of path that are
        // gateway nodes. If path itself is a gateway node it should not be included
        // in the results.
        if (proxyShapePath.startsWith(path) && proxyShapePath != path) {
            result.append(Ufe::Hierarchy::createItem(proxyShapePath));
        }
    }

    // If there were Usd prims that are gateway items then we'd have an implementation
    // of UsdSceneSegmentHandler that could find the gateway items and extra code
    // here to handle the case where isAGatewayType() for path is true. But right now
    // there are no gateway items in Usd, so I don't have to handle that.

    return result;
}

bool ProxyShapeSceneSegmentHandler::isGateway_(const Ufe::Path& path) const
{
    PXR_NS::UsdStageWeakPtr stage = getStage(path);
    return stage ? true
                 : fMayaSceneSegmentHandler ? fMayaSceneSegmentHandler->isGateway_(path) : false;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
