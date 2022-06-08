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

#include <mayaUsd/ufe/UsdConnectionHandler.h>

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/UsdConnections.h>
#include <mayaUsd/ufe/UsdHierarchyHandler.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

#include <pxr/base/tf/diagnostic.h>

#include <ufe/pathString.h>
#include <ufe/scene.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {

// TODO: The method looks for the PXR_NS::UsdAttribute from the Ufe::Attribute. However,
// the attr instance is a data member of the MayaUsd::Ufe::UsdAttribute. The challenge is that
// all the USD typed attribute classes privately inherites from MayaUsd::Ufe::UsdAttribute, and
// there is no common publicly inherited class so, the cast technic cannot be used.
// How to easily access the data member?

PXR_NS::UsdAttribute usdAttrFromUfeAttr(const Ufe::Attribute::Ptr& attr)
{
    if (!attr) {
        TF_RUNTIME_ERROR("Invalid attribute.");
        return PXR_NS::UsdAttribute();
    }

    if (attr->sceneItem()->runTimeId() != getUsdRunTimeId()) {
        TF_RUNTIME_ERROR(
            "Invalid runtime identifier for the attribute '"
            + attr->name()
            + "' in the node '"
            + Ufe::PathString::string(attr->sceneItem()->path())
            + "'.");
        return PXR_NS::UsdAttribute();
    }

    Ufe::Hierarchy::Ptr ufeHierarchy = Ufe::Hierarchy::hierarchy(attr->sceneItem());
    if (!ufeHierarchy) {
        TF_RUNTIME_ERROR(
            "Invalid hierarchy for the attribute '"
            + attr->name()
            + "' in the node '"
            + Ufe::PathString::string(attr->sceneItem()->path())
            + "'.");
        return PXR_NS::UsdAttribute();
    }

    UsdHierarchy::Ptr hierarchy
        = std::dynamic_pointer_cast<MayaUsd::ufe::UsdHierarchy>(ufeHierarchy);
    if (!hierarchy) {
        TF_CODING_ERROR("Invalid hierarchy instance.");
        return PXR_NS::UsdAttribute();
    }

    PXR_NS::UsdPrim prim = hierarchy->prim();
    return prim.GetAttribute(PXR_NS::TfToken(attr->name()));
}

bool isConnected(const PXR_NS::UsdAttribute& srcUsdAttr, const PXR_NS::UsdAttribute& dstUsdAttr)
{
    PXR_NS::SdfPathVector connectedAttrs;
    dstUsdAttr.GetConnections(&connectedAttrs);

    for (PXR_NS::SdfPath path : connectedAttrs) {
        if (path == srcUsdAttr.GetPath()) {
            return true;
        }
    }

    return false;
}

} // namespace

UsdConnectionHandler::UsdConnectionHandler()
    : Ufe::ConnectionHandler()
{
}

UsdConnectionHandler::~UsdConnectionHandler() { }

UsdConnectionHandler::Ptr UsdConnectionHandler::create()
{
    return std::make_shared<UsdConnectionHandler>();
}

Ufe::Connections::Ptr UsdConnectionHandler::sourceConnections(const Ufe::SceneItem::Ptr& item) const
{
    return UsdConnections::create(item);
}

bool UsdConnectionHandler::createConnection(
    const Ufe::Attribute::Ptr& srcAttr,
    const Ufe::Attribute::Ptr& dstAttr) const
{
    PXR_NS::UsdAttribute srcUsdAttr = usdAttrFromUfeAttr(srcAttr);
    PXR_NS::UsdAttribute dstUsdAttr = usdAttrFromUfeAttr(dstAttr);

    if (!isConnected(srcUsdAttr, dstUsdAttr)) {
        return dstUsdAttr.AddConnection(srcUsdAttr.GetPath());
    }

    // Return a failure because there is already a connection.
    return false;
}

bool UsdConnectionHandler::deleteConnection(
    const Ufe::Attribute::Ptr& srcAttr,
    const Ufe::Attribute::Ptr& dstAttr) const
{
    PXR_NS::UsdAttribute srcUsdAttr = usdAttrFromUfeAttr(srcAttr);
    PXR_NS::UsdAttribute dstUsdAttr = usdAttrFromUfeAttr(dstAttr);

    if (isConnected(srcUsdAttr, dstUsdAttr)) {
        return dstUsdAttr.RemoveConnection(srcUsdAttr.GetPath());
    }

    // Return a failure because there is no connection.
    return false;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
