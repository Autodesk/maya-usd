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

#include "UsdConnectionsHandler.h"

#include "UsdHierarchyHandler.h"
#include "UsdSceneItem.h"
#include "UsdConnections.h"

#include <ufe/scene.h>

#include <pxr/base/tf/diagnostic.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {

PXR_NS::UsdAttribute usdAttrFromUfeAttr(const UsdHierarchyHandler::Ptr& hierarchyHandler, const Ufe::Attribute::Ptr& attr)
{
    if (!hierarchyHandler)
    {
        TF_RUNTIME_ERROR("Invalid hierarchy handler.");
        return PXR_NS::UsdAttribute();
    }

    if (!attr)
    {
        TF_RUNTIME_ERROR("Invalid attribute.");
        return PXR_NS::UsdAttribute();
    }

    MayaUsd::ufe::UsdHierarchy::Ptr hierarchy
        = std::dynamic_pointer_cast<MayaUsd::ufe::UsdHierarchy>(hierarchyHandler->hierarchy(attr->sceneItem()));
    if (!hierarchy)
    {
        TF_CODING_ERROR("Invalid hierarchy handler.");
        return PXR_NS::UsdAttribute();
    }

    PXR_NS::UsdPrim prim = hierarchy->prim();
    return prim.GetAttribute(PXR_NS::TfToken(attr->name()));
}

bool isConnected(const PXR_NS::UsdAttribute& srcUsdAttr, const PXR_NS::UsdAttribute& dstUsdAttr)
{
    PXR_NS::SdfPathVector connectedAttrs;
    dstUsdAttr.GetConnections(&connectedAttrs);

    bool connected = false;
    for (PXR_NS::SdfPath path : connectedAttrs)
    {
        if (path == srcUsdAttr.GetPath())
        {
            connected = true;
        }
    }

    return connected;
}

}

UsdConnectionsHandler::UsdConnectionsHandler()
    : Ufe::ConnectionsHandler()
{}

UsdConnectionsHandler::~UsdConnectionsHandler()
{}

UsdConnectionsHandler::Ptr UsdConnectionsHandler::create()
{
    return std::make_shared<UsdConnectionsHandler>();
}

Ufe::Connections::Ptr UsdConnectionsHandler::sourceConnections(const Ufe::SceneItem::Ptr& item) const
{
    return UsdConnections::create(item);
}

bool UsdConnectionsHandler::connect(const Ufe::Attribute::Ptr& srcAttr, const Ufe::Attribute::Ptr& dstAttr) const
{
    MayaUsd::ufe::UsdHierarchyHandler::Ptr hierarchyHandler = MayaUsd::ufe::UsdHierarchyHandler::create();
    PXR_NS::UsdAttribute srcUsdAttr = usdAttrFromUfeAttr(hierarchyHandler, srcAttr);
    PXR_NS::UsdAttribute dstUsdAttr = usdAttrFromUfeAttr(hierarchyHandler, dstAttr);

    if (!isConnected(srcUsdAttr, dstUsdAttr))
    {
        return dstUsdAttr.AddConnection(srcUsdAttr.GetPath());
    }

    return true;
}

bool UsdConnectionsHandler::disconnect(const Ufe::Attribute::Ptr& srcAttr, const Ufe::Attribute::Ptr& dstAttr) const
{
    MayaUsd::ufe::UsdHierarchyHandler::Ptr hierarchyHandler = MayaUsd::ufe::UsdHierarchyHandler::create();
    PXR_NS::UsdAttribute srcUsdAttr = usdAttrFromUfeAttr(hierarchyHandler, srcAttr);
    PXR_NS::UsdAttribute dstUsdAttr = usdAttrFromUfeAttr(hierarchyHandler, dstAttr);

    if (isConnected(srcUsdAttr, dstUsdAttr))
    {
        return dstUsdAttr.RemoveConnection(srcUsdAttr.GetPath());
    }

    return true;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
