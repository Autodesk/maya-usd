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

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/UsdAttribute.h>
#include <mayaUsd/ufe/UsdConnectionHandler.h>
#include <mayaUsd/ufe/UsdConnections.h>
#include <mayaUsd/ufe/UsdHierarchyHandler.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/sdr/registry.h>

#include <ufe/pathString.h>
#include <ufe/scene.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {

UsdAttribute* usdAttrFromUfeAttr(const Ufe::Attribute::Ptr& attr)
{
    if (!attr) {
        TF_RUNTIME_ERROR("Invalid attribute.");
        return nullptr;
    }

    if (attr->sceneItem()->runTimeId() != getUsdRunTimeId()) {
        TF_RUNTIME_ERROR(
            "Invalid runtime identifier for the attribute '" + attr->name() + "' in the node '"
            + Ufe::PathString::string(attr->sceneItem()->path()) + "'.");
        return nullptr;
    }

    return dynamic_cast<UsdAttribute*>(attr.get());
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

PXR_NS::SdrShaderNodeConstPtr
_GetShaderNodeDef(const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& attrName)
{
    UsdPrim           targetPrim = prim;
    TfToken           targetName = attrName;
    UsdShadeNodeGraph ngTarget(targetPrim);
    while (ngTarget) {
        // Dig inside, following the connection on targetName until we find a shader.
        UsdShadeOutput graphOutput = ngTarget.GetOutput(targetName);
        if (!graphOutput) {
            // Not a NodeGraph we recognize.
            return {};
        }
        UsdShadeConnectableAPI source;
        TfToken                sourceOutputName;
        UsdShadeAttributeType  sourceType;
        if (UsdShadeConnectableAPI::GetConnectedSource(
                graphOutput, &source, &sourceOutputName, &sourceType)) {
            targetPrim = source.GetPrim();
            ngTarget = UsdShadeNodeGraph(targetPrim);
            targetName = sourceOutputName;
        } else {
            // Could not find a shader source connected to this nodegraph.
            return {};
        }
    }

    UsdShadeShader srcShader(targetPrim);
    if (!srcShader) {
        return {};
    }
    PXR_NS::SdrRegistry& registry = PXR_NS::SdrRegistry::GetInstance();
    PXR_NS::TfToken      srcInfoId;
    srcShader.GetIdAttr().Get(&srcInfoId);
    return registry.GetShaderNodeByIdentifier(srcInfoId);
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
    UsdAttribute* srcUsdAttr = usdAttrFromUfeAttr(srcAttr);
    UsdAttribute* dstUsdAttr = usdAttrFromUfeAttr(dstAttr);

    if (!srcUsdAttr || !dstUsdAttr
        || isConnected(srcUsdAttr->usdAttribute(), dstUsdAttr->usdAttribute())) {
        return false;
    }

    // Use the UsdShadeConnectableAPI to create the connections and attributes to make sure the USD
    // data model ends up in the right state.
    //
    // Using lower level APIs, like UsdPrim::CreateAttribute() tend leave the attributes marked as
    // being custom instead of native.

    UsdShadeConnectableAPI srcApi(srcUsdAttr->usdPrim());
    TfToken                srcBaseName;
    UsdShadeAttributeType  srcAttrType;
    std::tie(srcBaseName, srcAttrType)
        = UsdShadeUtils::GetBaseNameAndType(TfToken(srcAttr->name()));

    UsdShadeConnectableAPI dstApi(dstUsdAttr->usdPrim());
    TfToken                dstBaseName;
    UsdShadeAttributeType  dstAttrType;
    std::tie(dstBaseName, dstAttrType)
        = UsdShadeUtils::GetBaseNameAndType(TfToken(dstAttr->name()));

    if (srcAttrType == UsdShadeAttributeType::Input) {
        UsdShadeInput srcInput = srcApi.CreateInput(srcBaseName, srcUsdAttr->usdAttributeType());
        if (dstAttrType == UsdShadeAttributeType::Input) {
            UsdShadeInput dstInput
                = dstApi.CreateInput(dstBaseName, dstUsdAttr->usdAttributeType());
            return UsdShadeConnectableAPI::ConnectToSource(dstInput, srcInput);
        } else {
            UsdShadeOutput dstOutput
                = dstApi.CreateOutput(dstBaseName, dstUsdAttr->usdAttributeType());
            return UsdShadeConnectableAPI::ConnectToSource(dstOutput, srcInput);
        }
    } else {
        UsdShadeOutput srcOutput = srcApi.CreateOutput(srcBaseName, srcUsdAttr->usdAttributeType());
        if (dstAttrType == UsdShadeAttributeType::Input) {
            UsdShadeInput dstInput
                = dstApi.CreateInput(dstBaseName, dstUsdAttr->usdAttributeType());
            return UsdShadeConnectableAPI::ConnectToSource(dstInput, srcOutput);
        } else {
            UsdShadeOutput   dstOutput;
            UsdShadeMaterial dstMaterial(dstUsdAttr->usdPrim());
            // Special case when connecting to Material outputs:
            if (dstMaterial
                && (dstBaseName == UsdShadeTokens->surface || dstBaseName == UsdShadeTokens->volume
                    || dstBaseName == UsdShadeTokens->displacement)) {
                // Create the required output based on the type of the shader node we are trying to
                // connect:
                PXR_NS::SdrShaderNodeConstPtr srcShaderNodeDef
                    = _GetShaderNodeDef(srcUsdAttr->usdPrim(), srcBaseName);
                TfToken renderContext
                    = (!srcShaderNodeDef || srcShaderNodeDef->GetSourceType() == "glslfx")
                    ? UsdShadeTokens->universalRenderContext
                    : srcShaderNodeDef->GetSourceType();
                if (dstBaseName == UsdShadeTokens->surface) {
                    dstOutput = dstMaterial.CreateSurfaceOutput(renderContext);
                } else if (dstBaseName == UsdShadeTokens->volume) {
                    dstOutput = dstMaterial.CreateVolumeOutput(renderContext);
                } else {
                    dstOutput = dstMaterial.CreateDisplacementOutput(renderContext);
                }
            } else {
                dstOutput = dstApi.CreateOutput(dstBaseName, dstUsdAttr->usdAttributeType());
            }
            return UsdShadeConnectableAPI::ConnectToSource(dstOutput, srcOutput);
        }
    }

    // Return a failure.
    return false;
}

bool UsdConnectionHandler::deleteConnection(
    const Ufe::Attribute::Ptr& srcAttr,
    const Ufe::Attribute::Ptr& dstAttr) const
{
    UsdAttribute* srcUsdAttr = usdAttrFromUfeAttr(srcAttr);
    UsdAttribute* dstUsdAttr = usdAttrFromUfeAttr(dstAttr);

    if (!srcUsdAttr || !dstUsdAttr
        || !isConnected(srcUsdAttr->usdAttribute(), dstUsdAttr->usdAttribute())) {
        return false;
    }

    bool retVal = UsdShadeConnectableAPI::DisconnectSource(
        dstUsdAttr->usdAttribute(), srcUsdAttr->usdAttribute());

    // We need to make sure we cleanup on disconnection. Since having an empty connection array
    // counts as having connections, we need to get the array and see if it is empty.
    PXR_NS::SdfPathVector connectedAttrs;
    dstUsdAttr->usdAttribute().GetConnections(&connectedAttrs);
    if (connectedAttrs.empty()) {
        // Remove empty connection array
        UsdShadeConnectableAPI::ClearSources(dstUsdAttr->usdAttribute());

        // Remove attribute if it does not have a value, default value, or time samples. We do this
        // on Shader nodes and on the Material outputs since they are re-created automatically.
        // Other NodeGraph inputs and outputs require explicit removal.
        if (!dstUsdAttr->usdAttribute().HasValue()) {
            UsdShadeShader asShader(dstUsdAttr->usdPrim());
            if (asShader) {
                dstUsdAttr->usdPrim().RemoveProperty(dstUsdAttr->usdAttribute().GetName());
            }
            UsdShadeMaterial asMaterial(dstUsdAttr->usdPrim());
            if (asMaterial) {
                const TfToken baseName = dstUsdAttr->usdAttribute().GetBaseName();
                if (baseName == UsdShadeTokens->surface || baseName == UsdShadeTokens->volume
                    || baseName == UsdShadeTokens->displacement) {
                    dstUsdAttr->usdPrim().RemoveProperty(dstUsdAttr->usdAttribute().GetName());
                }
            }
        }
    }

    return retVal;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
