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
#include <mayaUsd/utils/converter.h>

#include <pxr/pxr.h>
#include <pxr/usd/ar/resolverScopedCache.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/xformCache.h>

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

MAYAUSD_NS_DEF
{

    namespace {
    //! Profiler category for proxy accessor events
    const int _accessorProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
        "ProxyAccessor",
        "ProxyAccessor"
#else
        "ProxyAccessor"
#endif
    );

    //! \brief  Test if given plug name is matching the convention used by accessor plugs
    bool isAccessorPlugName(const char* plugName)
    {
        return (strncmp(plugName, "AP_", 3) == 0);
    }

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
    SdfPath getAccessorSdfPath(const MPlug& plug)
    {
        MString niceNameCmd;
        niceNameCmd.format("attributeName -nice ^1s", plug.name().asChar());

        MString niceNameCmdResult;
        if (MGlobal::executeCommand(niceNameCmd, niceNameCmdResult)) {
            // Something we shouldn't have to deal with when things will get exposed
            // to C++. For array we will receive sdfpath with element index.
            // Example: "/path/to/prim[0]"
            int arrayIndex = plug.isArray() ? niceNameCmdResult.rindexW('[') : -1;
            if (arrayIndex >= 1) {
                niceNameCmdResult = niceNameCmdResult.substringW(0, arrayIndex - 1);
            }

            if (SdfPath::IsValidPathString(niceNameCmdResult.asChar()))
                return SdfPath(niceNameCmdResult.asChar());
        }

        return SdfPath();
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
            [](MNodeMessage::AttributeMessage msg,
               MPlug&                         plug,
               MPlug&                         otherPlug,
               void*                          clientData) {
                if (!clientData
                    || (msg & (MNodeMessage::kConnectionMade | MNodeMessage::kConnectionBroken))
                        == 0)
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

                    SdfPath path;
                    if (parentAttr.isNull())
                        path = getAccessorSdfPath(plug);
                    else {
                        MPlug parentPlug(plug.node(), parentAttr);
                        path = getAccessorSdfPath(parentPlug);
                    }

                    if (path.IsEmpty() || !path.IsPrimPropertyPath())
                        return;

                    // Compute dependencies is considered as temporary data
                    UsdEditContext editContext(stage, stage->GetSessionLayer());

                    SdfPath primPath = path.GetPrimPath();
                    UsdPrim prim = stage->GetPrimAtPath(primPath);

                    if (!prim.RemoveProperty(path.GetNameToken())) {
                        TF_DEBUG(USDMAYA_PROXYACCESSOR)
                            .Msg(
                                "Failed to clear target layer on disconnect of '%s'\n",
                                path.GetText());
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
            _accessorProfilerCategory, MProfiler::kColorB_L1, "Generate acceleration structure");

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

            MPlug   valuePlug(node, attr.object());
            SdfPath path = getAccessorSdfPath(valuePlug);

            if (path.IsEmpty()) {
                TF_DEBUG(USDMAYA_PROXYACCESSOR)
                    .Msg(
                        "Plug found '%s', but it's not pointing at a valid SdfPath; ignoring\n",
                        valuePlug.name().asChar());
                continue;
            }

            SdfPath        primPath = path.GetPrimPath();
            const UsdPrim& prim = stage->GetPrimAtPath(primPath);

            const Converter* converter = nullptr;
            if (!path.IsPrimPropertyPath()) {
                SdfValueTypeName typeName = Converter::getUsdTypeName(valuePlug, false);
                if (typeName != SdfValueTypeNames->Matrix4d) {
                    TF_DEBUG(USDMAYA_PROXYACCESSOR)
                        .Msg(
                            "Prim path found, but value plug is not a supported data type '%s' "
                            "(%s); ignoring\n",
                            path.GetText(),
                            valuePlug.attribute().apiTypeStr());
                    continue;
                }

                converter = Converter::find(typeName, false);
            } else {
                const TfToken& propertyToken = path.GetNameToken();
                UsdAttribute   attribute = prim.GetAttribute(propertyToken);

                if (!attribute.IsDefined()) {
                    TF_DEBUG(USDMAYA_PROXYACCESSOR)
                        .Msg("Attribute is not defined '%s'; ignoring\n", path.GetText());
                    continue;
                }

                converter = Converter::find(valuePlug, attribute);
            }

            if (!converter) {
                TF_DEBUG(USDMAYA_PROXYACCESSOR)
                    .Msg("Skipped attribute, no valid converter found for '%s'\n", path.GetText());
                continue;
            }

            if (isAccessorInputPlug(valuePlug)) {
                TF_DEBUG(USDMAYA_PROXYACCESSOR).Msg("Added INPUT '%s'\n", path.GetText());
                _accessorInputItems.emplace_back(valuePlug, path, converter);
            } else {
                TF_DEBUG(USDMAYA_PROXYACCESSOR).Msg("Added OUTPUT '%s'\n", path.GetText());
                _accessorOutputItems.emplace_back(valuePlug, path, converter);
            }
        }

        return;
    }

    MStatus ProxyAccessor::addDependentsDirty(const MPlug& plug, MPlugArray& plugArray)
    {
        if (inCompute())
            return MS::kUnknownParameter;

        MProfilingScope profilingScope(
            _accessorProfilerCategory, MProfiler::kColorB_L1, "Dirty accessor plugs");

        collectAccessorItems(plug.node());

        if (_accessorInputItems.size() == 0 && _accessorOutputItems.size() == 0)
            return MS::kUnknownParameter;

        const bool accessorPlug = isAccessorPlugName(plug.partialName().asChar());
        const bool isInputPlug = accessorPlug && isAccessorInputPlug(plug);

        if (isInputPlug || !plug.isDynamic() || plug == _forceCompute) {
            TF_DEBUG(USDMAYA_PROXYACCESSOR)
                .Msg("Dirty all outputs from '%s'\n", plug.name().asChar());

            for (const auto& item : _accessorOutputItems) {
                const MPlug& itemPlug = std::get<0>(item);
                if (!itemPlug.isArray())
                    plugArray.append(itemPlug);
                else {
                    const unsigned int numElements = itemPlug.numElements();
                    for (unsigned int i = 0; i < numElements; i++) {
                        plugArray.append(itemPlug[i]);
                    }
                }
            }
        }
        return MS::kSuccess;
    }

    MStatus ProxyAccessor::compute(const MPlug& plug, MDataBlock& dataBlock)
    {
        if (inCompute())
            return MS::kSuccess;

        MProfilingScope profilingScope(
            _accessorProfilerCategory, MProfiler::kColorB_L1, "Compute USD accessor");

        TF_DEBUG(USDMAYA_PROXYACCESSOR)
            .Msg("Compute USD accessor triggered by '%s'\n", plug.name().asChar());

        Scoped_InCompute inComputeNow(*this);

        collectAccessorItems(plug.node());

        // Early exit to avoid virtual function calls when no compute will happen
        if (_accessorInputItems.size() == 0 && _accessorOutputItems.size() == 0)
            return MS::kSuccess;

        const UsdStageRefPtr stage = getUsdStage();

        ArResolverScopedCache resolverCache;

        ConverterArgs args;
        args._timeCode = getTime();

        // Compute dependencies is considered as temporary data
        UsdEditContext editContext(stage, stage->GetSessionLayer());

        computeInputs(stage, dataBlock, args);
        computeOutputs(plug.node(), stage, dataBlock, args);

        return MS::kSuccess;
    }

    MStatus ProxyAccessor::computeInputs(
        const UsdStageRefPtr stage, MDataBlock& dataBlock, const ConverterArgs& args)
    {
        MStatus retValue = MS::kSuccess;
        for (const auto& item : _accessorInputItems) {
            const MPlug&     itemPlug = std::get<0>(item);
            const SdfPath&   itemPath = std::get<1>(item);
            const Converter* itemConverter = std::get<2>(item);
            // We should cache UsdAttribute in here too and avoid expensive
            // searches (i.e. getting the prim, getting attribute, checking if defined)

            MProfilingScope profilingScope(
                _accessorProfilerCategory,
                MProfiler::kColorB_L1,
                "Write input",
                itemPath.GetText());

            SdfPath        itemPrimPath = itemPath.GetPrimPath();
            const UsdPrim& itemPrim = stage->GetPrimAtPath(itemPrimPath);

            if (!itemPath.IsPrimPropertyPath() || !itemConverter)
                continue;

            const TfToken& itemPropertyToken = itemPath.GetNameToken();
            UsdAttribute   itemAttribute = itemPrim.GetAttribute(itemPropertyToken);

            if (!itemAttribute.IsDefined()) {
                TF_CODING_ERROR("Undefined/invalid attribute '%s'", itemPath.GetText());
                continue;
            }

            MDataHandle itemDataHandle = dataBlock.inputValue(itemPlug, &retValue);
            if (MFAIL(retValue)) {
                continue;
            }

            VtValue convertedValue;
            itemConverter->convert(itemDataHandle, convertedValue, args);

            // Don't set the value if it didn't change. This will save us expensive invalidation +
            // compute
            VtValue currentValue;
            if (itemAttribute.Get(&currentValue, args._timeCode)
                && convertedValue != currentValue) {
                itemAttribute.Set(convertedValue, args._timeCode);
            }
        }

        return retValue;
    }

    MStatus ProxyAccessor::computeOutputs(
        const MObject&       ownerNode,
        const UsdStageRefPtr stage,
        MDataBlock&          dataBlock,
        const ConverterArgs& args)
    {
        MStatus retValue = MS::kSuccess;

        UsdGeomXformCache xformCache(args._timeCode);

        MDagPath proxyDagPath;
        MDagPath::getAPathTo(ownerNode, proxyDagPath);
        const MMatrix proxyInclusiveMatrix = proxyDagPath.inclusiveMatrix();

        for (const auto& item : _accessorOutputItems) {
            const MPlug&     itemPlug = std::get<0>(item);
            const SdfPath&   itemPath = std::get<1>(item);
            const Converter* itemConverter = std::get<2>(item);
            // We should cache UsdAttribute in here too and avoid expensive
            // searches (i.e. getting the prim, getting attribute, checking if defined)

            MProfilingScope profilingScope(
                _accessorProfilerCategory,
                MProfiler::kColorB_L1,
                "Write output",
                itemPath.GetText());

            SdfPath        itemPrimPath = itemPath.GetPrimPath();
            const UsdPrim& itemPrim = stage->GetPrimAtPath(itemPrimPath);

            MDataHandle itemDataHandle = dataBlock.outputValue(itemPlug, &retValue);
            if (MFAIL(retValue)) {
                continue;
            }

            // If it's not a property path, then we will be writing out world matrix data
            if (!itemPath.IsPrimPropertyPath()) {
                GfMatrix4d mat = xformCache.GetLocalToWorldTransform(itemPrim);
                MMatrix    mayaMat;
                TypedConverter<MMatrix, GfMatrix4d>::convert(mat, mayaMat);

                mayaMat *= proxyInclusiveMatrix;

                MFnMatrixData data;
                MObject       dataMatrix = data.create();
                data.set(mayaMat);

                MArrayDataHandle  dstArray(itemDataHandle);
                MArrayDataBuilder dstArrayBuilder(&dataBlock, itemPlug.attribute(), 1);

                MDataHandle dstElement = dstArrayBuilder.addElement(0);
                dstElement.set(dataMatrix);

                dstArray.set(dstArrayBuilder);
                dstArray.setAllClean();
            } else if (itemConverter) {
                const TfToken& itemPropertyToken = itemPath.GetNameToken();
                UsdAttribute   itemAttribute = itemPrim.GetAttribute(itemPropertyToken);

                // cache this! expensive call
                if (!itemAttribute.IsDefined()) {
                    TF_CODING_ERROR("Undefined/invalid attribute '%s'", itemPath.GetText());

                    dataBlock.setClean(itemPlug);
                    continue;
                }

                itemConverter->convert(itemAttribute, itemDataHandle, args);
            }

            // Even if we have no data to write, we set the data in data block as clean
            // This will prevent entering compute loop again and in case of changes to USD
            // which result in particular path being invalid, we will preserve the value
            // from last time it was available
            itemDataHandle.setClean();
            dataBlock.setClean(itemPlug.attribute());
        }

        return retValue;
    }

    MStatus ProxyAccessor::syncCache(const MObject& node, MDataBlock& dataBlock)
    {
        if (inCompute())
            return MS::kSuccess;

        MProfilingScope profilingScope(
            _accessorProfilerCategory, MProfiler::kColorB_L1, "Update USD cache");

        TF_DEBUG(USDMAYA_PROXYACCESSOR).Msg("Update USD cache\n");

        Scoped_InCompute inComputeNow(*this);

        collectAccessorItems(node);

        // Early exit to avoid virtual function calls when no compute will happen
        if (_accessorInputItems.size() == 0)
            return MS::kSuccess;

        const UsdStageRefPtr stage = getUsdStage();

        ArResolverScopedCache resolverCache;

        ConverterArgs args;
        args._timeCode = getTime();

        // Compute dependencies is considered as temporary data
        UsdEditContext editContext(stage, stage->GetSessionLayer());

        return computeInputs(stage, dataBlock, args);
    }

    MStatus ProxyAccessor::stageChanged(
        const MObject& node, const UsdNotice::ObjectsChanged& notice)
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
                for (auto& item : _accessorInputItems) {
                    if (std::get<1>(item) == changedPath) {
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
                    auto* changedInput = findInputItemFn(changedPath);
                    if (!changedInput) {
                        TF_DEBUG(USDMAYA_PROXYACCESSOR)
                            .Msg(
                                "Input has changed but not found in input list '%s'\n",
                                changedPath.GetText());
                        continue;
                    }

                    TF_DEBUG(USDMAYA_PROXYACCESSOR)
                        .Msg("Input PrimPropertyPath has changed '%s'\n", changedPath.GetText());

                    MPlug&           changedPlug = std::get<0>(*changedInput);
                    const Converter* converter = std::get<2>(*changedInput);

                    SdfPath        changedPrimPath = changedPath.GetPrimPath();
                    const UsdPrim& changedPrim = stage->GetPrimAtPath(changedPrimPath);

                    const TfToken& changedPropertyToken = changedPath.GetNameToken();
                    UsdAttribute changedAttribute = changedPrim.GetAttribute(changedPropertyToken);

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

} // namespace MayaUsd
