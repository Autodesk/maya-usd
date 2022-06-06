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

namespace {

/**
 *  \brief Build the Ufe::Path (i.e. Maya USD path) from a Prim::Path (i.e. USD path)
 *
 *   The method builds a Maya Ufe path from a USD prim path. For example, the method converts
 *   from '/DisplayColorCube/Looks/usdPreviewSurface1SG/usdPreviewSurface1' to
 *   '|world|stage|stageShape/DisplayColorCube/Looks/usdPreviewSurface1SG/usdPreviewSurface1'
 *   where:
 *   1. '|world|stage|stageShape' represents the Maya path in Ufe
 *   2. '/DisplayColorCube/Looks/usdPreviewSurface1SG/usdPreviewSurface1' represents the USD prim
 * path.
 *
 *  \param prim is the prim of the selected scene item in the USD data model.
 *  \param baseMayaPath is the Ufe base path in the Maya data model.
 */
Ufe::Path
buildMayaUfePathFromPrimPath(const PXR_NS::UsdPrim& prim, const Ufe::Path& baseMayaPath)
{
    // Get the prim path i.e. the path in the USD world.
    Ufe::Path primPath = Ufe::PathString::path(prim.GetPrimPath().GetAsString());
    // Create the Maya Ufe::Path which is composed of the Maya path prefix and the USD prim path.
    return primPath.reparent(Ufe::PathString::path(""), baseMayaPath);
}

} // namespace

UsdConnections::UsdConnections(const Ufe::SceneItem::Ptr& item)
    : Ufe::Connections()
    , fSceneItem(std::dynamic_pointer_cast<UsdSceneItem>(item))
{
    if (!TF_VERIFY(fSceneItem)) {
        TF_RUNTIME_ERROR("Invalid scene item.");
    }
}

UsdConnections::~UsdConnections() { }

UsdConnections::Ptr UsdConnections::create(const Ufe::SceneItem::Ptr& item)
{
    return std::make_shared<UsdConnections>(item);
}

std::vector<Ufe::Connection::Ptr> UsdConnections::allConnections() const
{
    TF_AXIOM(fSceneItem);

    std::vector<Ufe::Connection::Ptr> result;

    // The scene item is a shader node.
    const Ufe::Path sceneItemPath = fSceneItem->path();
    // Find the base maya path i.e. the base path to use for all the prim paths.
    const Ufe::Path baseMayaPath = sceneItemPath.popSegment();

    // Find the Maya Ufe::Path of the selected shader node.
    const PXR_NS::UsdPrim prim = fSceneItem->prim();
    const Ufe::Path       primPath = buildMayaUfePathFromPrimPath(prim, baseMayaPath);

    // The method looks for all the connections in which one of the attribute of this scene item is
    // the destination.

    PXR_NS::UsdShadeConnectableAPI connectableAttrs(prim);

    // Look for all the connected input attributes which are a destination of a connection.

    for (PXR_NS::UsdShadeInput input : connectableAttrs.GetInputs()) {
        if (input.HasConnectedSource()) {
            for (PXR_NS::UsdShadeConnectionSourceInfo sourceInfo : input.GetConnectedSources()) {

                // Find the Maya Ufe::Path of the connected shader node.
                const PXR_NS::UsdPrim connectedPrim = sourceInfo.source.GetPrim();
                const Ufe::Path       connectedPrimPath
                    = buildMayaUfePathFromPrimPath(connectedPrim, baseMayaPath);

                // Find the name of the connected source attribute name.
                PXR_NS::TfToken tkSourceName = PXR_NS::UsdShadeUtils::GetFullName(
                    sourceInfo.sourceName, sourceInfo.sourceType);

                // Create the in-memory representation of the connection.
                Ufe::Connection::Ptr connection = std::make_shared<Ufe::Connection>(
                    Ufe::AttributeInfo(connectedPrimPath, tkSourceName.GetString()),
                    Ufe::AttributeInfo(primPath, input.GetFullName()));

                result.push_back(connection);
            }
        }
    }

    // Look for all the connected output attributes which are a destination of a connection.

    for (PXR_NS::UsdShadeOutput output : connectableAttrs.GetOutputs()) {
        if (output.HasConnectedSource()) {
            for (PXR_NS::UsdShadeConnectionSourceInfo sourceInfo : output.GetConnectedSources()) {

                // Find the Maya Ufe::Path of the connected shader node.
                const PXR_NS::UsdPrim connectedPrim = sourceInfo.source.GetPrim();
                const Ufe::Path       connectedPrimPath
                    = buildMayaUfePathFromPrimPath(connectedPrim, baseMayaPath);

                // Find the name of the connected source attribute name.
                PXR_NS::TfToken tkSourceName = PXR_NS::UsdShadeUtils::GetFullName(
                    sourceInfo.sourceName, sourceInfo.sourceType);

                // Create the in-memory representation of the connection.
                Ufe::Connection::Ptr connection = std::make_shared<Ufe::Connection>(
                    Ufe::AttributeInfo(connectedPrimPath, tkSourceName.GetString()),
                    Ufe::AttributeInfo(primPath, output.GetFullName()));

                result.push_back(connection);
            }
        }
    }

    return result;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
