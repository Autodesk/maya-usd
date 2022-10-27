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

#include <mayaUsd/ufe/UsdConnections.h>
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdShade/types.h>

#include <ufe/hierarchy.h>
#include <ufe/pathString.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdConnections::UsdConnections(const Ufe::SceneItem::Ptr& item)
    : Ufe::Connections()
#if !defined(NDEBUG)
    , _sceneItem(std::static_pointer_cast<UsdSceneItem>(item))
#else
    , _sceneItem(std::dynamic_pointer_cast<UsdSceneItem>(item))
#endif
{
    if (!TF_VERIFY(_sceneItem)) {
        TF_FATAL_ERROR("Invalid scene item.");
    }
}

UsdConnections::~UsdConnections() { }

UsdConnections::Ptr UsdConnections::create(const Ufe::SceneItem::Ptr& item)
{
    return std::make_shared<UsdConnections>(item);
}

std::vector<Ufe::Connection::Ptr> UsdConnections::allConnections() const
{
    TF_AXIOM(_sceneItem);

    std::vector<Ufe::Connection::Ptr> result;

    // Get some information of the prim.

    const PXR_NS::UsdPrim prim = _sceneItem->prim();
    const Ufe::Path       primPath = _sceneItem->path();
    const Ufe::Path       stagePath = primPath.getSegments()[0];

    // The method looks for all the connections in which one of the attribute of this scene item is
    // the destination.

    PXR_NS::UsdShadeConnectableAPI connectableAttrs(prim);

    // Helper method to create the in-memory connection if any.
    auto createConnections = [&](const auto& attr) -> void {
        if (attr.HasConnectedSource()) {
            for (PXR_NS::UsdShadeConnectionSourceInfo sourceInfo : attr.GetConnectedSources()) {

                // Find the Maya Ufe::Path of the connected shader node.
                const PXR_NS::UsdPrim connectedPrim = sourceInfo.source.GetPrim();
                const Ufe::Path       connectedPrimPath
                    = stagePath + usdPathToUfePathSegment(connectedPrim.GetPrimPath());

                // Find the name of the connected source attribute name.
                PXR_NS::TfToken tkSourceName = PXR_NS::UsdShadeUtils::GetFullName(
                    sourceInfo.sourceName, sourceInfo.sourceType);

                // Create the in-memory representation of the connection.
                Ufe::Connection::Ptr connection = std::make_shared<Ufe::Connection>(
                    Ufe::AttributeInfo(connectedPrimPath, tkSourceName.GetString()),
                    Ufe::AttributeInfo(primPath, attr.GetFullName()));

                result.push_back(connection);
            }
        }
    };

    // Look for all the connected input attributes which are a destination of a connection.

    for (PXR_NS::UsdShadeInput input : connectableAttrs.GetInputs(false)) {
        createConnections(input);
    }

    // Look for all the connected output attributes which are a destination of a connection.

    for (PXR_NS::UsdShadeOutput output : connectableAttrs.GetOutputs(false)) {
        createConnections(output);
    }

    return result;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
