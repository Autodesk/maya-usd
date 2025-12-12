//
// Copyright 2020 Autodesk
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
#include "proxyAccessor.h"

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/converter.h>

#include <pxr/base/tf/getenv.h>
#include <pxr/pxr.h>
#include <pxr/usd/ar/resolverScopedCache.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdGeom/xformOp.h>

#include <maya/MArrayDataBuilder.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MDGContext.h>
#include <maya/MDagPath.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MMessage.h>
#include <maya/MNodeMessage.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MProfiler.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <memory>
#include <type_traits>
#include <unordered_map>

namespace MAYAUSD_NS_DEF {

static bool useTargetedLayerInProxyAccessor()
{
    return PXR_NS::TfGetenvBool("MAYAUSD_USE_TARGETED_LAYER_IN_PROXY_ACCESSOR", false);
}

MAYAUSD_VERIFY_CLASS_NOT_MOVE_OR_COPY(ProxyAccessor);

/*! /brief  Scoped object setting up compute context for accessor

    Proxy accessor supports nested compute that allows injecting DG dependencies to USD. More
    complex setups will create dependencies between output and input accessor plugs. In such case
    computing inputs will come back to proxy accessor and request computation of a specific output.
    Such output may then be again dependent on input from Maya, so reqursion can continue.
    ComputeContext is setup at the entry to computation and allows nested compute to reuse its
    state.

    See `validateRecursiveCompute` for an example of a setup that requires nested compute.
 */
class ComputeContext
{
public:
    //! \brief  Construct compute context for both inputs and outputs
    ComputeContext(ProxyAccessor& accessor, const MObject& ownerNode, bool useTargetLayer)
        : _restoreState(accessor._inCompute)
        , _accessor(accessor)
        , _stage(accessor.getUsdStage())
        , _editContext(_stage, getLayer(useTargetLayer))
    {
        // Start with setting this context on the accessor. This is important in case
        // anything below causes compute.
        accessor._inCompute = this;

        _args._timeCode = _accessor.getTime();
        _xformCache.SetTime(_args._timeCode);

        MDagPath proxyDagPath;
        MDagPath::getAPathTo(ownerNode, proxyDagPath);
        _proxyInclusiveMatrix = proxyDagPath.inclusiveMatrix();

        // Only increment evaluation ID for top most evaluation scope.
        // Nested scope is allowed, but shouldn't in really be needed.
        if (!_restoreState) {
            accessor._evaluationId.next();
        }
    }

    //! \brief  Construct compute context for inputs only.
    ComputeContext(ProxyAccessor& accessor, bool useTargetLayer)
        : _restoreState(accessor._inCompute)
        , _accessor(accessor)
        , _stage(accessor.getUsdStage())
        , _editContext(_stage, getLayer(useTargetLayer))
    {
        // Start with setting this context on the accessor. This is important in case
        // anything below causes compute.
        accessor._inCompute = this;

        _args._timeCode = _accessor.getTime();
        _xformCache.SetTime(_args._timeCode);

        // Only increment evaluation ID for top most evaluation scope.
        // Nested scope is allowed, but shouldn't in really be needed.
        if (!_restoreState) {
            accessor._evaluationId.next();
        }
    }

    //! \brief  Restore will handle changing context pointer in the accessor to the state before
    ~ComputeContext() { _accessor._inCompute = _restoreState; }

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(ComputeContext);

private:
    pxr::SdfLayerHandle getLayer(bool useTargetLayer)
    {
        if (useTargetLayer || useTargetedLayerInProxyAccessor()) {
            return _stage->GetEditTarget().GetLayer();
        }

        return _stage->GetSessionLayer();
    }

    //! Remember context pointer at the creation of this object
    ComputeContext* _restoreState;

public:
    //! Accessor setting up this context
    ProxyAccessor& _accessor;
    //! Reference to the stage
    const UsdStageRefPtr _stage;

    //! Scoped objects setting up resolver cache
    ArResolverScopedCache _resolverCache;
    //! Scoped object changing current edit context to the given layer
    UsdEditContext _editContext;
    //! Xform compute cache
    UsdGeomXformCache _xformCache;

    //! Converter arguments used when translating between Maya's and USD data model
    ConverterArgs _args;
    //! Proxy share transform matrix
    MMatrix _proxyInclusiveMatrix;
};

MAYAUSD_VERIFY_CLASS_NOT_MOVE_OR_COPY(ComputeContext);

namespace {
//! Profiler category for proxy accessor events
const int kAccessorProfilerCategory = MProfiler::addCategory("ProxyAccessor", "ProxyAccessor");

static const TfToken combinedVisibilityToken("combinedVisibility");

//! \brief  Test if given plug name is matching the convention used by accessor plugs
bool isAccessorPlugName(const char* plugName) { return (strncmp(plugName, "AP_", 3) == 0); }

//! \brief  Returns True is given plug is categorized as input plug, or False if output plug
bool isAccessorInputPlug(const MPlug& plug)
{
    if (plug.isDestination())
        return true;

    if (!plug.isCompound())
        return false;

    auto childCount = plug.numChildren();
    for (unsigned int i = 0; i < childCount; ++i) {
        MPlug child = plug.child(i);
        if (child.isDestination())
            return true;
    }

    return false;
}

//! \brief  Retrieve SdfPath from give plug
using SdfPathAndPropName = std::pair<SdfPath, TfToken>;
SdfPathAndPropName getAccessorSdfPath(const MPlug& plug)
{
    MString niceNameCmd;
    niceNameCmd.format("attributeName -nice ^1s", plug.name().asChar());

    MString niceNameCmdResult;
    if (!MGlobal::executeCommand(niceNameCmd, niceNameCmdResult))
        return {};

    // Something we shouldn't have to deal with when things will get exposed
    // to C++. For array we will receive sdfpath with element index.
    // Example: "/path/to/prim[0]"
    int arrayIndex = plug.isArray() ? niceNameCmdResult.rindexW('[') : -1;
    if (arrayIndex >= 1) {
        niceNameCmdResult = niceNameCmdResult.substringW(0, arrayIndex - 1);
    }

    TfToken propName;

    const int dotIndex = niceNameCmdResult.index('.');
    if (dotIndex >= 0) {
        MStringArray primAndProp;
        niceNameCmdResult.split('.', primAndProp);
        niceNameCmdResult = primAndProp[0];
        propName = TfToken(primAndProp[1].asChar());
    }

    if (!SdfPath::IsValidPathString(niceNameCmdResult.asChar()))
        return {};

    return std::make_pair(SdfPath(niceNameCmdResult.asChar()), propName);
}
} // namespace

MStatus ProxyAccessor::addCallbacks(MObject object)
{
    _callbackIds.append(MNodeMessage::addAttributeAddedOrRemovedCallback(
        object,
        [](MNodeMessage::AttributeMessage msg, MPlug& plug, void* clientData) {
            if (clientData) {
                if (isAccessorPlugName(plug.partialName().asChar())) {
                    static_cast<ProxyAccessor*>(clientData)->invalidateAccessorItems();
                }
            }
        },
        (void*)(this)));

    _callbackIds.append(MNodeMessage::addAttributeChangedCallback(
        object,
        [](MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData) {
            if (!clientData
                || (msg & (MNodeMessage::kConnectionMade | MNodeMessage::kConnectionBroken)) == 0)
                return;

            if (isAccessorPlugName(plug.partialName().asChar())) {
                auto* accessor = static_cast<ProxyAccessor*>(clientData);
                accessor->invalidateAccessorItems();

                if ((msg & MNodeMessage::kConnectionBroken) == 0)
                    return;

                auto stage = accessor->getUsdStage();
                if (!stage)
                    return;

                MFnAttribute attr(plug.attribute());
                MObject      parentAttr = attr.parent();

                SdfPathAndPropName pathAndProp;
                if (parentAttr.isNull())
                    pathAndProp = getAccessorSdfPath(plug);
                else {
                    MPlug parentPlug(plug.node(), parentAttr);
                    pathAndProp = getAccessorSdfPath(parentPlug);
                }

                const SdfPath& path = pathAndProp.first;
                const TfToken& propName = pathAndProp.second;

                if (path.IsEmpty())
                    return;

                // Compute dependencies is considered as temporary data
                UsdEditContext editContext(stage, stage->GetSessionLayer());

                UsdPrim prim = stage->GetPrimAtPath(path);

                if (!prim.IsPseudoRoot() && !prim.RemoveProperty(propName)) {
                    TF_DEBUG(USDMAYA_PROXYACCESSOR)
                        .Msg(
                            "Failed to clear target layer on disconnect of '%s.%s'\n",
                            path.GetText(),
                            propName.GetText());
                }
            }
        },
        (void*)(this)));

    return MS::kSuccess;
}

MStatus ProxyAccessor::removeCallbacks()
{
    MMessage::removeCallbacks(_callbackIds);
    _callbackIds.clear();
    return MS::kSuccess;
}

void ProxyAccessor::collectAccessorItems(MObject node)
{
    if (_validAccessorItems)
        return;

    MProfilingScope profilingScope(
        kAccessorProfilerCategory, MProfiler::kColorB_L1, "Generate acceleration structure");

    _accessorInputItems.clear();
    _accessorOutputItems.clear();

    _validAccessorItems = true;

    auto stage = getUsdStage();
    if (!stage)
        return;

    ArResolverScopedCache resolverCache;

    MFnDependencyNode fnDep(node);
    unsigned          attrCount = fnDep.attributeCount();
    for (unsigned i = 0; i < attrCount; ++i) {
        MFnAttribute attr(fnDep.attribute(i));

        MStatus parentStatus = MS::kSuccess;
        attr.parent(&parentStatus);

        // Filter out child attributes
        if (parentStatus == MS::kSuccess)
            continue;

        MString name = attr.name();
        if (!isAccessorPlugName(name.asChar()))
            continue;

        Item item;
        item.plug = MPlug(node, attr.object());

        SdfPathAndPropName pathAndProp = getAccessorSdfPath(item.plug);
        item.path = pathAndProp.first;
        item.property = pathAndProp.second;

        if (item.path.IsEmpty()) {
            TF_DEBUG(USDMAYA_PROXYACCESSOR)
                .Msg(
                    "Plug found '%s', but it's not pointing at a valid SdfPath; ignoring\n",
                    item.plug.name().asChar());
            continue;
        }

        const UsdPrim& prim = stage->GetPrimAtPath(item.path);

        if (item.property.IsEmpty() || item.property == combinedVisibilityToken) {
            SdfValueTypeName typeName = Converter::getUsdTypeName(item.plug, false);
            SdfValueTypeName expectedType
                = item.property.IsEmpty() ? SdfValueTypeNames->Matrix4d : SdfValueTypeNames->Int;
            if (typeName != expectedType) {
                TF_DEBUG(USDMAYA_PROXYACCESSOR)
                    .Msg(
                        "Prim path found, but value plug is not a supported data type '%s.%s' "
                        "(%s); ignoring\n",
                        item.path.GetText(),
                        item.property.GetText(),
                        item.plug.attribute().apiTypeStr());
                continue;
            }

            item.converter = Converter::find(typeName, false);
        } else {
            PXR_NS::UsdAttribute attribute = prim.GetAttribute(item.property);

            if (!attribute.IsDefined()) {
                TF_DEBUG(USDMAYA_PROXYACCESSOR)
                    .Msg("Attribute is not defined '%s'; ignoring\n", item.path.GetText());
                continue;
            }

            item.converter = Converter::find(item.plug, attribute);
        }

        if (!item.converter) {
            TF_DEBUG(USDMAYA_PROXYACCESSOR)
                .Msg("Skipped attribute, no valid converter found for '%s'\n", item.path.GetText());
            continue;
        }

        if (isAccessorInputPlug(item.plug)) {
            TF_DEBUG(USDMAYA_PROXYACCESSOR).Msg("Added INPUT '%s'\n", item.path.GetText());
            _accessorInputItems.emplace_back(item);
        } else {
            TF_DEBUG(USDMAYA_PROXYACCESSOR).Msg("Added OUTPUT '%s'\n", item.path.GetText());
            _accessorOutputItems.emplace_back(item);
        }
    }

    return;
}

const ProxyAccessor::Item* ProxyAccessor::findAccessorItem(const MPlug& plug, bool isInput) const
{
    const Container& accessorItems = isInput ? _accessorInputItems : _accessorOutputItems;
    for (const auto& item : accessorItems) {
        if ((plug.isElement() && item.plug == plug.array()) || item.plug == plug)
            return &item;
    }

    return nullptr;
}

MStatus ProxyAccessor::addDependentsDirty(const MPlug& plug, MPlugArray& plugArray)
{
    if (inCompute())
        return MS::kUnknownParameter;

    MProfilingScope profilingScope(
        kAccessorProfilerCategory, MProfiler::kColorB_L1, "Dirty accessor plugs");

    collectAccessorItems(plug.node());

    if (_accessorInputItems.size() == 0 && _accessorOutputItems.size() == 0)
        return MS::kUnknownParameter;

    const bool accessorPlug = isAccessorPlugName(plug.partialName().asChar());
    const bool isInputPlug = accessorPlug && isAccessorInputPlug(plug);

    if (isInputPlug || !plug.isDynamic() || plug == _forceCompute) {
        TF_DEBUG(USDMAYA_PROXYACCESSOR).Msg("Dirty all outputs from '%s'\n", plug.name().asChar());

        for (const auto& item : _accessorOutputItems) {
            if (!item.plug.isArray())
                plugArray.append(item.plug);
            else {
                const unsigned int numElements = item.plug.numElements();
                for (unsigned int i = 0; i < numElements; i++) {
                    plugArray.append(item.plug[i]);
                }
            }
        }
    }
    return MS::kSuccess;
}

MStatus ProxyAccessor::compute(const MPlug& plug, MDataBlock& dataBlock, bool useTargetLayer)
{
    // Special handling for nested compute
    if (inCompute()) {
        MProfilingScope profilingScope(
            kAccessorProfilerCategory, MProfiler::kColorB_L3, "Nested compute USD accessor");

        const auto* accessorItem = findAccessorItem(plug, false);
        if (accessorItem) {
            TF_DEBUG(USDMAYA_PROXYACCESSOR)
                .Msg("Nested compute triggered by '%s'\n", plug.name().asChar());

            ComputeContext& topState = *_inCompute;

            // If it's not a property path, then we will be writing out world matrix data
            if (!accessorItem->path.IsPrimPropertyPath()) {
                // Read only inputs that can affect requested xform matrix and that haven't been
                // yet read. We will perform evaluationId check to prevent causing recursive
                // computation of the same plug, when there is more than one input depending on it.
                for (auto& inputItem : _accessorInputItems) {
                    if (UsdGeomXformOp::IsXformOp(inputItem.property)
                        && accessorItem->path.HasPrefix(inputItem.path)) {
                        computeInput(inputItem, topState._stage, dataBlock, topState._args);
                    }
                }

                // write to only single output that was requested.
                computeOutput(
                    *accessorItem,
                    topState._proxyInclusiveMatrix,
                    topState._stage,
                    dataBlock,
                    topState._xformCache,
                    topState._args);
            } else {
                computeOutput(
                    *accessorItem,
                    topState._proxyInclusiveMatrix,
                    topState._stage,
                    dataBlock,
                    topState._xformCache,
                    topState._args);
            }
        } else {
            TF_DEBUG(USDMAYA_PROXYACCESSOR)
                .Msg("!!!! Nested compute on a plug ignored '%s'\n", plug.name().asChar());
        }
        return MS::kSuccess;
    }

    MProfilingScope profilingScope(
        kAccessorProfilerCategory, MProfiler::kColorB_L1, "Compute USD accessor");

    TF_DEBUG(USDMAYA_PROXYACCESSOR)
        .Msg("Compute USD accessor triggered by '%s'\n", plug.name().asChar());

    collectAccessorItems(plug.node());

    // Early exit to avoid virtual function calls when no compute will happen
    if (_accessorInputItems.size() == 0 && _accessorOutputItems.size() == 0)
        return MS::kSuccess;

    ComputeContext evalState(*this, plug.node(), useTargetLayer);

    // Read and set inputs on the stage. If recursive computation was performed,
    // some of the inputs may have been already evaluated (see evaluationId check)
    for (auto& item : _accessorInputItems) {
        computeInput(item, evalState._stage, dataBlock, evalState._args);
    }
    // Write outputs that haven't been yet computed
    for (const auto& item : _accessorOutputItems) {
        computeOutput(
            item,
            evalState._proxyInclusiveMatrix,
            evalState._stage,
            dataBlock,
            evalState._xformCache,
            evalState._args);
    }

    return MS::kSuccess;
}

MStatus ProxyAccessor::computeInput(
    Item&                item,
    const UsdStageRefPtr stage,
    MDataBlock&          dataBlock,
    const ConverterArgs& args)
{
    MStatus retValue = MS::kSuccess;

    SyncId& evaluationId = item.syncId;
    if (evaluationId.inSync(_evaluationId))
        return MS::kSuccess;

    // We should cache UsdAttribute in here too and avoid expensive
    // searches (i.e. getting the prim, getting attribute, checking if defined)

    MProfilingScope profilingScope(
        kAccessorProfilerCategory, MProfiler::kColorB_L1, "Write input", item.path.GetText());

    evaluationId.sync(_evaluationId);

    const UsdPrim& itemPrim = stage->GetPrimAtPath(item.path);

    if (item.property.IsEmpty() || !item.converter)
        return MS::kFailure;

    PXR_NS::UsdAttribute itemAttribute = itemPrim.GetAttribute(item.property);

    if (!itemAttribute.IsDefined()) {
        TF_CODING_ERROR(
            "Undefined/invalid attribute '%s.%s'", item.path.GetText(), item.property.GetText());
        return MS::kFailure;
    }

    MDataHandle itemDataHandle = dataBlock.inputValue(item.plug, &retValue);
    if (MFAIL(retValue)) {
        return retValue;
    }

    VtValue convertedValue;
    item.converter->convert(itemDataHandle, convertedValue, args);

    // Don't set the value if it didn't change. This will save us expensive invalidation +
    // compute
    VtValue currentValue;
    if (itemAttribute.Get(&currentValue, args._timeCode) && convertedValue != currentValue) {
        itemAttribute.Set(convertedValue, args._timeCode);
    }

    return MS::kSuccess;
}

MStatus ProxyAccessor::computeOutput(
    const Item&          item,
    const MMatrix&       proxyInclusiveMatrix,
    const UsdStageRefPtr stage,
    MDataBlock&          dataBlock,
    UsdGeomXformCache&   xformCache,
    const ConverterArgs& args)
{
    MStatus retValue = MS::kSuccess;

    // We should cache UsdAttribute in here too and avoid expensive
    // searches (i.e. getting the prim, getting attribute, checking if defined)

    MProfilingScope profilingScope(
        kAccessorProfilerCategory, MProfiler::kColorB_L1, "Write output", item.path.GetText());

    const UsdPrim& itemPrim = stage->GetPrimAtPath(item.path);

    MDataHandle itemDataHandle = dataBlock.outputValue(item.plug, &retValue);
    if (MFAIL(retValue)) {
        return retValue;
    }

    // If it's not a property path, then we will be writing out world matrix data
    if (item.property.IsEmpty()) {
        GfMatrix4d mat = xformCache.GetLocalToWorldTransform(itemPrim);
        MMatrix    mayaMat;
        TypedConverter<MMatrix, GfMatrix4d>::convert(mat, mayaMat);

        mayaMat *= proxyInclusiveMatrix;

        MFnMatrixData data;
        MObject       dataMatrix = data.create();
        data.set(mayaMat);

        MArrayDataHandle  dstArray(itemDataHandle);
        MArrayDataBuilder dstArrayBuilder(&dataBlock, item.plug.attribute(), 1);

        MDataHandle dstElement = dstArrayBuilder.addElement(0);
        dstElement.set(dataMatrix);

        dstArray.set(dstArrayBuilder);
        dstArray.setAllClean();
    } else if (item.property == combinedVisibilityToken) {
        // First, verify visibility of the proxy shape
        Ufe::Path proxyShapeUfePath = MayaUsd::ufe::stagePath(stage);
        MDagPath  proxyShapeDagPath = MayaUsd::ufe::ufeToDagPath(proxyShapeUfePath);
        bool      visible = proxyShapeDagPath.isVisible();

        // Next, verify visibility of the usd prim
        UsdGeomImageable imageable(itemPrim);
        if (visible && imageable) {
            visible = (imageable.ComputeVisibility() == UsdGeomTokens->inherited);
        }

        itemDataHandle.set(visible ? 1 : 0);
    } else if (item.converter) {
        PXR_NS::UsdAttribute itemAttribute = itemPrim.GetAttribute(item.property);

        // cache this! expensive call
        if (!itemAttribute.IsDefined()) {
            TF_CODING_ERROR(
                "Undefined/invalid attribute '%s.%s'",
                item.path.GetText(),
                item.property.GetText());

            dataBlock.setClean(item.plug);
            return MS::kFailure;
        }

        item.converter->convert(itemAttribute, itemDataHandle, args);
    }

    // Even if we have no data to write, we set the data in data block as clean
    // This will prevent entering compute loop again and in case of changes to USD
    // which result in particular path being invalid, we will preserve the value
    // from last time it was available
    itemDataHandle.setClean();
    dataBlock.setClean(item.plug.attribute());

    return MS::kSuccess;
}

MStatus ProxyAccessor::syncCache(const MObject& node, MDataBlock& dataBlock, bool useTargetLayer)
{
    if (inCompute())
        return MS::kSuccess;

    MProfilingScope profilingScope(
        kAccessorProfilerCategory, MProfiler::kColorB_L1, "Update USD cache");

    TF_DEBUG(USDMAYA_PROXYACCESSOR).Msg("Update USD cache\n");

    collectAccessorItems(node);

    // Early exit to avoid virtual function calls when no compute will happen
    if (_accessorInputItems.size() == 0)
        return MS::kSuccess;

    ComputeContext evalState(*this, node, useTargetLayer);
    for (auto& item : _accessorInputItems) {
        computeInput(item, evalState._stage, dataBlock, evalState._args);
    }

    return MS::kSuccess;
}

MStatus ProxyAccessor::stageChanged(const MObject& node, const UsdNotice::ObjectsChanged& notice)
{
    const auto& stage = notice.GetStage();
    if (stage != getUsdStage()) {
        TF_CODING_ERROR("We shouldn't be receiving notification for other stages than one "
                        "returned by stage provider");
        return MS::kUnknownParameter;
    }

    bool needsForceCompute = true;

    if (_accessorInputItems.size() > 0) {
        auto findInputItemFn = [this](const SdfPath& changedPath) -> Item* {
            for (Item& item : _accessorInputItems) {
                if (item.path == changedPath
                    || item.path.AppendProperty(item.property) == changedPath) {
                    return &item;
                }
            }
            return nullptr;
        };

        // UFE currently doesn't write time sampled data.
        ConverterArgs args;
        args._timeCode = UsdTimeCode::Default(); // getTime();

        // Compute dependencies is considered as temporary data
        UsdEditContext editContext(stage, stage->GetSessionLayer());

        for (const auto& changedPath : notice.GetChangedInfoOnlyPaths()) {
            if (changedPath.IsPrimPropertyPath()) {
                Item* changedInput = findInputItemFn(changedPath);
                if (!changedInput) {
                    TF_DEBUG(USDMAYA_PROXYACCESSOR)
                        .Msg(
                            "Input has changed but not found in input list '%s'\n",
                            changedPath.GetText());
                    continue;
                }

                TF_DEBUG(USDMAYA_PROXYACCESSOR)
                    .Msg("Input PrimPropertyPath has changed '%s'\n", changedPath.GetText());

                MPlug&           changedPlug = changedInput->plug;
                const Converter* converter = changedInput->converter;

                SdfPath        changedPrimPath = changedPath.GetAbsoluteRootOrPrimPath();
                const UsdPrim& changedPrim = stage->GetPrimAtPath(changedPrimPath);

                const TfToken&       changedPropertyToken = changedPath.GetNameToken();
                PXR_NS::UsdAttribute changedAttribute
                    = changedPrim.GetAttribute(changedPropertyToken);

                converter->convert(changedAttribute, changedPlug, args);

                // When input plug is set, this value may be a new constant or
                // just temporary value overriding what comes from animation curve.
                // Input value change will properly cause outputs to compute so
                // forcing compute is not necessary (and it destructive for temporary values)
                needsForceCompute = false;
            }
        }
    }

    if (needsForceCompute && _accessorOutputItems.size() > 0) {
        forceCompute(node);
    }

    return MStatus::kSuccess;
}

MStatus ProxyAccessor::forceCompute(const MObject& node)
{
    // don't force compute when already doing one
    if (!inCompute()) {
        MPlug forceCompute(node, _forceCompute);
        forceCompute.setBool(!forceCompute.asBool());
        return MStatus::kSuccess;
    }

    return MStatus::kFailure;
}

} // namespace MAYAUSD_NS_DEF
