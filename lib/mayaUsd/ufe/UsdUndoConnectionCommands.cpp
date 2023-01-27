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
#include "UsdUndoConnectionCommands.h"

#include "private/Utils.h"

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/UsdAttribute.h>
#include <mayaUsd/ufe/UsdAttributes.h>
#include <mayaUsd/ufe/UsdConnectionHandler.h>
#include <mayaUsd/ufe/UsdConnections.h>
#include <mayaUsd/ufe/UsdHierarchyHandler.h>
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/UsdUndoBlock.h>
#include <mayaUsdUtils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/sdr/registry.h>

#include <ufe/attributeInfo.h>
#include <ufe/connection.h>
#include <ufe/pathString.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {

Ufe::Attribute::Ptr attrFromUfeAttrInfo(const Ufe::AttributeInfo& attrInfo)
{
    auto item
        = std::dynamic_pointer_cast<UsdSceneItem>(Ufe::Hierarchy::createItem(attrInfo.path()));
    if (!item) {
        TF_RUNTIME_ERROR("Invalid scene item.");
        return nullptr;
    }
    return UsdAttributes(item).attribute(attrInfo.name());
}

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

bool isAttrConnected(const PXR_NS::UsdPrim& prim, const PXR_NS::UsdAttribute& srcUsdAttr)
{
    for (const auto& childPrim : prim.GetChildren()) {
        for (const auto& attribute : childPrim.GetAttributes()) {
            PXR_NS::UsdAttribute dstUsdAttr = attribute.As<PXR_NS::UsdAttribute>();
            if (isConnected(srcUsdAttr, dstUsdAttr)) {
                return true;
            }
        }
    }

    PXR_NS::SdfPathVector connectedAttrs;
    srcUsdAttr.GetConnections(&connectedAttrs);

    return !connectedAttrs.empty();
}

PXR_NS::SdrShaderNodeConstPtr
_GetShaderNodeDef(const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& attrName)
{
    UsdPrim               targetPrim = prim;
    TfToken               targetName = attrName;
    UsdShadeAttributeType targetType = UsdShadeAttributeType::Output;
    UsdShadeNodeGraph     ngTarget(targetPrim);
    while (ngTarget) {
        // Dig inside, following the connection on targetName until we find a shader.
        PXR_NS::UsdAttribute targetAttr
            = targetPrim.GetAttribute(UsdShadeUtils::GetFullName(targetName, targetType));
        if (!targetAttr) {
            // Not a NodeGraph we recognize.
            return {};
        }
        UsdShadeConnectableAPI source;
        if (UsdShadeConnectableAPI::GetConnectedSource(
                targetAttr, &source, &targetName, &targetType)) {
            targetPrim = source.GetPrim();
            ngTarget = UsdShadeNodeGraph(targetPrim);
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

} // namespace

UsdUndoCreateConnectionCommand::UsdUndoCreateConnectionCommand(
    const Ufe::Attribute::Ptr& srcAttr,
    const Ufe::Attribute::Ptr& dstAttr)
    : Ufe::ConnectionResultUndoableCommand()
    , _srcInfo(std::make_unique<Ufe::AttributeInfo>(srcAttr))
    , _dstInfo(std::make_unique<Ufe::AttributeInfo>(dstAttr))
{
    // Validation goes here when we find out the right set of business rules. Failure should result
    // in a exception being thrown.
}

UsdUndoCreateConnectionCommand::~UsdUndoCreateConnectionCommand() { }

UsdUndoCreateConnectionCommand::Ptr UsdUndoCreateConnectionCommand::create(
    const Ufe::Attribute::Ptr& srcAttr,
    const Ufe::Attribute::Ptr& dstAttr)
{
    return std::make_shared<UsdUndoCreateConnectionCommand>(srcAttr, dstAttr);
}

void UsdUndoCreateConnectionCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    auto          srcAttr = attrFromUfeAttrInfo(*_srcInfo);
    UsdAttribute* srcUsdAttr = usdAttrFromUfeAttr(srcAttr);
    auto          dstAttr = attrFromUfeAttrInfo(*_dstInfo);
    UsdAttribute* dstUsdAttr = usdAttrFromUfeAttr(dstAttr);

    if (!srcUsdAttr || !dstUsdAttr) {
        _srcInfo = nullptr;
        _dstInfo = nullptr;
        return;
    }

    if (isConnected(srcUsdAttr->usdAttribute(), dstUsdAttr->usdAttribute())) {
        return;
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
        = UsdShadeUtils::GetBaseNameAndType(TfToken(srcUsdAttr->usdAttribute().GetName()));

    UsdShadeConnectableAPI dstApi(dstUsdAttr->usdPrim());
    TfToken                dstBaseName;
    UsdShadeAttributeType  dstAttrType;
    std::tie(dstBaseName, dstAttrType)
        = UsdShadeUtils::GetBaseNameAndType(TfToken(dstUsdAttr->usdAttribute().GetName()));

    bool isConnected = false;

    if (srcAttrType == UsdShadeAttributeType::Input) {
        UsdShadeInput srcInput = srcApi.CreateInput(srcBaseName, srcUsdAttr->usdAttributeType());
        if (dstAttrType == UsdShadeAttributeType::Input) {
            UsdShadeInput dstInput
                = dstApi.CreateInput(dstBaseName, dstUsdAttr->usdAttributeType());
            isConnected = UsdShadeConnectableAPI::ConnectToSource(dstInput, srcInput);
        } else {
            UsdShadeOutput dstOutput
                = dstApi.CreateOutput(dstBaseName, dstUsdAttr->usdAttributeType());
            isConnected = UsdShadeConnectableAPI::ConnectToSource(dstOutput, srcInput);
        }
    } else {
        UsdShadeOutput srcOutput = srcApi.CreateOutput(srcBaseName, srcUsdAttr->usdAttributeType());
        if (dstAttrType == UsdShadeAttributeType::Input) {
            UsdShadeInput dstInput
                = dstApi.CreateInput(dstBaseName, dstUsdAttr->usdAttributeType());
            isConnected = UsdShadeConnectableAPI::ConnectToSource(dstInput, srcOutput);
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
            isConnected = UsdShadeConnectableAPI::ConnectToSource(dstOutput, srcOutput);
            _srcInfo = std::make_unique<Ufe::AttributeInfo>(
                _srcInfo->path(), srcOutput.GetAttr().GetName());
            _dstInfo = std::make_unique<Ufe::AttributeInfo>(
                _dstInfo->path(), dstOutput.GetAttr().GetName());
        }
    }

    if (isConnected) {
        _SendStrongConnectionChangeNotification(dstApi.GetPrim());
    } else {
        _srcInfo = nullptr;
        _dstInfo = nullptr;
    }
}

Ufe::Connection::Ptr UsdUndoCreateConnectionCommand::connection() const
{
    if (_srcInfo && _srcInfo->attribute() && _dstInfo && _dstInfo->attribute()) {
        return std::make_shared<Ufe::Connection>(*_srcInfo, *_dstInfo);
    } else {
        return {};
    }
}

void UsdUndoCreateConnectionCommand::undo() { _undoableItem.undo(); }

void UsdUndoCreateConnectionCommand::redo() { _undoableItem.redo(); }

UsdUndoDeleteConnectionCommand::UsdUndoDeleteConnectionCommand(
    const Ufe::Attribute::Ptr& srcAttr,
    const Ufe::Attribute::Ptr& dstAttr)
    : Ufe::UndoableCommand()
    , _srcInfo(std::make_unique<Ufe::AttributeInfo>(srcAttr))
    , _dstInfo(std::make_unique<Ufe::AttributeInfo>(dstAttr))
{
    // Validation goes here when we find out the right set of business rules. Failure should result
    // in a exception being thrown.
}

UsdUndoDeleteConnectionCommand::~UsdUndoDeleteConnectionCommand() { }

UsdUndoDeleteConnectionCommand::Ptr UsdUndoDeleteConnectionCommand::create(
    const Ufe::Attribute::Ptr& srcAttr,
    const Ufe::Attribute::Ptr& dstAttr)
{
    return std::make_shared<UsdUndoDeleteConnectionCommand>(srcAttr, dstAttr);
}

void UsdUndoDeleteConnectionCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    auto          srcAttr = attrFromUfeAttrInfo(*_srcInfo);
    UsdAttribute* srcUsdAttr = usdAttrFromUfeAttr(srcAttr);
    auto          dstAttr = attrFromUfeAttrInfo(*_dstInfo);
    UsdAttribute* dstUsdAttr = usdAttrFromUfeAttr(dstAttr);

    if (!srcUsdAttr || !dstUsdAttr) {
        return;
    }

    deleteConnection(srcUsdAttr->usdAttribute(), dstUsdAttr->usdAttribute());
}

void UsdUndoDeleteConnectionCommand::undo() { _undoableItem.undo(); }

void UsdUndoDeleteConnectionCommand::redo() { _undoableItem.redo(); }

void UsdUndoDeleteConnectionCommand::deleteConnection(
    const PXR_NS::UsdAttribute& srcUsdAttr,
    const PXR_NS::UsdAttribute& dstUsdAttr)
{
    if (!srcUsdAttr || !dstUsdAttr || !isConnected(srcUsdAttr, dstUsdAttr)) {
        return;
    }

    bool isDisconnected = UsdShadeConnectableAPI::DisconnectSource(dstUsdAttr, srcUsdAttr);

    // We need to make sure we cleanup on disconnection. Since having an empty connection array
    // counts as having connections, we need to get the array and see if it is empty.
    PXR_NS::SdfPathVector connectedAttrs;
    dstUsdAttr.GetConnections(&connectedAttrs);
    if (connectedAttrs.empty()) {
        // Remove empty connection array
        UsdShadeConnectableAPI::ClearSources(dstUsdAttr);

        // Remove attribute if it does not have a value, default value, or time samples.
        // Note: the dstUsdAttr could be a connection source in the parent prim level.
        if (!dstUsdAttr.HasValue()
            && !isAttrConnected(dstUsdAttr.GetPrim().GetParent(), dstUsdAttr)) {
            dstUsdAttr.GetPrim().RemoveProperty(dstUsdAttr.GetName());
        }
        // Remove the src property only if it is not connected anymore and has no value.
        if (!srcUsdAttr.HasValue()
            && !(
                isAttrConnected(srcUsdAttr.GetPrim(), srcUsdAttr)
                || isAttrConnected(srcUsdAttr.GetPrim().GetParent(), srcUsdAttr))) {
            srcUsdAttr.GetPrim().RemoveProperty(srcUsdAttr.GetName());
        }
    }

    if (isDisconnected) {
        _SendStrongConnectionChangeNotification(dstUsdAttr.GetPrim());
    }
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
