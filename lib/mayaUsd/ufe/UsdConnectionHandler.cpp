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
#include <mayaUsd/ufe/UsdConnectionHandler.h>
#include <mayaUsd/ufe/UsdConnections.h>

#include <usdUfe/ufe/UsdAttribute.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/usdUtils.h>
#ifdef UFE_V4_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdUndoConnectionCommands.h>
#endif

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/sdr/registry.h>

#include <ufe/pathString.h>
#include <ufe/scene.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

#ifndef UFE_V4_FEATURES_AVAILABLE

//
// For UFE v4 the connection/disconnection code is moved to UsdUndoConnectionCommands.cpp
//

namespace {

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

#if PXR_VERSION < 2302
void _SendStrongConnectionChangeNotification(const UsdPrim& usdPrim)
{
    // See https://github.com/PixarAnimationStudios/USD/issues/2013 for details.
    //
    // The notification sent on connection change is not strong enough to force a Hydra resync of
    // the material, which forces a resync of the dependent geometries. This means the list of
    // primvars required by the material will not be updated on those geometries. Play a trick on
    // the stage that generates a stronger notification so the primvars get properly rescanned.
    const TfToken waToken("Issue_2013_Notif_Workaround");
    SdfPath       waPath = usdPrim.GetPath().AppendChild(waToken);
    usdPrim.GetStage()->DefinePrim(waPath);
    usdPrim.GetStage()->RemovePrim(waPath);
}
#endif

} // namespace

#endif

MAYAUSD_VERIFY_CLASS_SETUP(Ufe::ConnectionHandler, UsdConnectionHandler);

UsdConnectionHandler::Ptr UsdConnectionHandler::create()
{
    return std::make_shared<UsdConnectionHandler>();
}

Ufe::Connections::Ptr UsdConnectionHandler::sourceConnections(const Ufe::SceneItem::Ptr& item) const
{
    return UsdConnections::create(item);
}

#ifdef UFE_V4_FEATURES_AVAILABLE

Ufe::ConnectionResultUndoableCommand::Ptr UsdConnectionHandler::createConnectionCmd(
    const Ufe::Attribute::Ptr& srcAttr,
    const Ufe::Attribute::Ptr& dstAttr) const
{
    return UsdUndoCreateConnectionCommand::create(srcAttr, dstAttr);
}

Ufe::UndoableCommand::Ptr UsdConnectionHandler::deleteConnectionCmd(
    const Ufe::Attribute::Ptr& srcAttr,
    const Ufe::Attribute::Ptr& dstAttr) const
{
    return UsdUndoDeleteConnectionCommand::create(srcAttr, dstAttr);
}

#else

//
// For UFE 0.4.43 the connection/disconnection code is moved to UsdUndoConnectionCommands.cpp
//

bool UsdConnectionHandler::createConnection(
    const Ufe::Attribute::Ptr& srcAttr,
    const Ufe::Attribute::Ptr& dstAttr) const
{
    auto srcUsdAttr = UsdUfe::usdAttrFromUfeAttr(srcAttr);
    auto dstUsdAttr = UsdUfe::usdAttrFromUfeAttr(dstAttr);

    if (!srcUsdAttr || !dstUsdAttr
        || UsdUfe::isConnected(srcUsdAttr->usdAttribute(), dstUsdAttr->usdAttribute())) {
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

    bool retVal = false;

    if (srcAttrType == UsdShadeAttributeType::Input) {
        UsdShadeInput srcInput = srcApi.CreateInput(srcBaseName, srcUsdAttr->usdAttributeType());
        if (dstAttrType == UsdShadeAttributeType::Input) {
            UsdShadeInput dstInput
                = dstApi.CreateInput(dstBaseName, dstUsdAttr->usdAttributeType());
            retVal = UsdShadeConnectableAPI::ConnectToSource(dstInput, srcInput);
        } else {
            UsdShadeOutput dstOutput
                = dstApi.CreateOutput(dstBaseName, dstUsdAttr->usdAttributeType());
            retVal = UsdShadeConnectableAPI::ConnectToSource(dstOutput, srcInput);
        }
    } else {
        UsdShadeOutput srcOutput = srcApi.CreateOutput(srcBaseName, srcUsdAttr->usdAttributeType());
        if (dstAttrType == UsdShadeAttributeType::Input) {
            UsdShadeInput dstInput
                = dstApi.CreateInput(dstBaseName, dstUsdAttr->usdAttributeType());
            retVal = UsdShadeConnectableAPI::ConnectToSource(dstInput, srcOutput);
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
            retVal = UsdShadeConnectableAPI::ConnectToSource(dstOutput, srcOutput);
        }
    }

#if PXR_VERSION < 2302
    if (retVal) {
        _SendStrongConnectionChangeNotification(dstApi.GetPrim());
    }
#endif

    return retVal;
}

bool UsdConnectionHandler::deleteConnection(
    const Ufe::Attribute::Ptr& srcAttr,
    const Ufe::Attribute::Ptr& dstAttr) const
{
    auto srcUsdAttr = UsdUfe::usdAttrFromUfeAttr(srcAttr);
    auto dstUsdAttr = UsdUfe::usdAttrFromUfeAttr(dstAttr);

    if (!srcUsdAttr || !dstUsdAttr
        || !UsdUfe::isConnected(srcUsdAttr->usdAttribute(), dstUsdAttr->usdAttribute())) {
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

#if PXR_VERSION < 2302
    if (retVal) {
        _SendStrongConnectionChangeNotification(dstUsdAttr->usdPrim());
    }
#endif

    return retVal;
}

#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
