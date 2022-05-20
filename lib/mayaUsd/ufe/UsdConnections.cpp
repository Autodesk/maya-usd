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

#include "UsdConnections.h"

#include "UsdConnection.h"
#include "UsdSceneItem.h"

#include <pxr/base/tf/diagnostic.h>

#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/sdf/path.h>

#include <ufe/hierarchy.h>
#include <ufe/pathString.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

std::vector<Ufe::Connection::Ptr> UsdConnections::sourceConnections() const
{
    std::vector<Ufe::Connection::Ptr> result;

    UsdSceneItem::Ptr usdSceneItem = std::dynamic_pointer_cast<UsdSceneItem>(sceneItem());
    if (!usdSceneItem) {
        TF_CODING_ERROR("Invalid scene item.");
        return result;
    }

    Ufe::Path materialMayaPath = usdSceneItem->path();
    materialMayaPath = materialMayaPath.popSegment();
    PXR_NS::UsdPrim prim = usdSceneItem->prim();

    Ufe::Path primPath = Ufe::PathString::path(prim.GetPrimPath().GetAsString());
    primPath = primPath.reparent(Ufe::PathString::path(""), materialMayaPath);

    // The method looks for all the connections in which one of the attribute of this node is 
    // the destination.

    PXR_NS::UsdShadeConnectableAPI connectableAttrs(prim);

    // Look for all the connected input attributes which are a destination of a connection.

    for (PXR_NS::UsdShadeInput input : connectableAttrs.GetInputs()) {
        if (input.HasConnectedSource()) {
            for (PXR_NS::UsdShadeConnectionSourceInfo sourceInfo : input.GetConnectedSources()) {
                PXR_NS::UsdPrim connectedPrim = sourceInfo.source.GetPrim();
                Ufe::Path connectedPrimPath = Ufe::PathString::path(connectedPrim.GetPrimPath().GetAsString());
                connectedPrimPath = connectedPrimPath.reparent(Ufe::PathString::path(""), materialMayaPath);

                UsdConnection::Ptr connection =
                    UsdConnection::create(Ufe::AttributeInfo(connectedPrimPath, sourceInfo.sourceName.GetString()),
                                          Ufe::AttributeInfo(primPath, input.GetBaseName()));
                result.push_back(connection);
            }
        }
    }

    // Look for all the connected output attributes which are a destination of a connection.

    for (PXR_NS::UsdShadeOutput output : connectableAttrs.GetOutputs()) {
        if (output.HasConnectedSource()) {
            for (PXR_NS::UsdShadeConnectionSourceInfo sourceInfo : output.GetConnectedSources()) {
                PXR_NS::UsdPrim connectedPrim = sourceInfo.source.GetPrim();
                Ufe::Path connectedPrimPath = Ufe::PathString::path(connectedPrim.GetPrimPath().GetAsString());
                connectedPrimPath = connectedPrimPath.reparent(Ufe::PathString::path(""), materialMayaPath);

                UsdConnection::Ptr connection =
                    UsdConnection::create(Ufe::AttributeInfo(connectedPrimPath, sourceInfo.sourceName.GetString()),
                                          Ufe::AttributeInfo(primPath, output.GetBaseName()));
                result.push_back(connection);
            }
        }
    }

    return result;
}

UsdConnections::Ptr UsdConnections::create(const Ufe::SceneItem::Ptr& item)
{
    return std::make_shared<UsdConnections>(item);
}


UsdConnections::UsdConnections(const Ufe::SceneItem::Ptr& item)
    : Ufe::Connections(item)
{
}

UsdConnections::~UsdConnections()
{
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
