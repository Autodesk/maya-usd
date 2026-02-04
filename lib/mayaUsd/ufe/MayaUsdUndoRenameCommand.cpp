//
// Copyright 2025 Autodesk
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
#include "MayaUsdUndoRenameCommand.h"

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/Utils.h>

#include <usdUfe/ufe/Utils.h>

#include <ufe/pathSegment.h>
#include <ufe/pathString.h>
#include <ufe/sceneNotification.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

// Ensure that MayaUsdUndoRenameCommand is properly setup.
MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::UsdUndoRenameCommand, MayaUsdUndoRenameCommand);

MayaUsdUndoRenameCommand::MayaUsdUndoRenameCommand(
    const UsdUfe::UsdSceneItem::Ptr& srcItem,
    const Ufe::PathComponent&        newName)
    : UsdUfe::UsdUndoRenameCommand(srcItem, newName)
{
}

UsdUfe::UsdUndoRenameCommand::Ptr MayaUsdUndoRenameCommand::create(
    const UsdUfe::UsdSceneItem::Ptr& srcItem,
    const Ufe::PathComponent&        newName)
{
    return std::make_shared<MayaUsdUndoRenameCommand>(srcItem, newName);
}

void MayaUsdUndoRenameCommand::sendRenameNotification(
    const PXR_NS::UsdStagePtr& stage,
    const PXR_NS::UsdPrim&     prim,
    const Ufe::Path&           srcPath,
    const Ufe::Path&           dstPath)
{
    for (const std::string& proxyName : ProxyShapeHandler::getAllNames()) {
        PXR_NS::UsdStagePtr proxyStage = ProxyShapeHandler::dagPathToStage(proxyName);
        if (proxyStage != stage)
            continue;

        // For all the proxy shapes that are mapping the same stage, we need to fixup the
        // Ufe path since they have different Ufe Paths because it contains proxy shape name.
        auto proxyNamePath = Ufe::PathString::path(proxyName);
        auto proxySegment = proxyNamePath.getSegments()[0];

        const Ufe::PathSegment& srcUsdSegment = srcPath.getSegments()[1];
        const Ufe::Path adjustedSrcPath(Ufe::Path::Segments({ proxySegment, srcUsdSegment }));

        const Ufe::PathSegment& dstUsdSegment = dstPath.getSegments()[1];
        const Ufe::Path adjustedDstPath(Ufe::Path::Segments({ proxySegment, dstUsdSegment }));

        auto newItem = UsdUfe::UsdSceneItem::create(adjustedDstPath, prim);

        UsdUfe::sendNotification<Ufe::ObjectRename>(newItem, adjustedSrcPath);
    }
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
