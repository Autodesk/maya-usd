
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
#include "MayaUsdContextOpsHandler.h"

#include <mayaUsd/ufe/MayaUsdContextOps.h>
#include <mayaUsd/ufe/UsdStageMap.h>
#include <mayaUsd/ufe/Utils.h>

#ifdef MAYA_HAS_SCENE_RENDER_SETTINGS
#include <mayaUsd/nodes/sceneRenderSettings.h>
#endif

#include <maya/MFnDependencyNode.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::UsdContextOpsHandler, MayaUsdContextOpsHandler);

/*static*/
MayaUsdContextOpsHandler::Ptr MayaUsdContextOpsHandler::create()
{
    return std::make_shared<MayaUsdContextOpsHandler>();
}

//------------------------------------------------------------------------------
// Ufe::ContextOpsHandler overrides
//------------------------------------------------------------------------------

Ufe::ContextOps::Ptr MayaUsdContextOpsHandler::contextOps(const Ufe::SceneItem::Ptr& item) const
{
    // Suppress context ops for USD prims under the SceneRenderSettings node.
#ifdef MAYA_HAS_SCENE_RENDER_SETTINGS
    if (item) {
        const auto& path = item->path();
        if (!path.empty()) {
            const Ufe::Path gatewayPath
                = path.nbSegments() > 1 ? Ufe::Path(path.getSegments()[0]) : path;
            MObject gatewayObj
                = UsdStageMap::getInstance().proxyShape(gatewayPath, /*rebuildCacheIfNeeded=*/true);
            if (!gatewayObj.isNull()) {
                MFnDependencyNode depFn(gatewayObj);
                if (depFn.typeId() == UsdSceneRenderSettings::typeId) {
                    return nullptr;
                }
            }
        }
    }
#endif

    auto usdItem = downcast(item);
#if !defined(NDEBUG)
    assert(usdItem);
#endif
    return MayaUsdContextOps::create(usdItem);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
