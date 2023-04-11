//
// Copyright 2023 Autodesk
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
#include "proxyShapeListenerBase.h"

#include <pxr/usd/usdUtils/stageCache.h>

#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnNumericAttribute.h>

#include <mutex>

using namespace MAYAUSD_NS_DEF;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(
    MayaUsdProxyShapeListenerBaseTokens,
    MAYAUSD_PROXY_SHAPE_LISTENER_BASE_TOKENS);

const MTypeId MayaUsdProxyShapeListenerBase::typeId(0x5800009A);
const MString MayaUsdProxyShapeListenerBase::typeName(
    MayaUsdProxyShapeListenerBaseTokens->MayaTypeName.GetText());

MObject MayaUsdProxyShapeListenerBase::updateCounterAttr;
MObject MayaUsdProxyShapeListenerBase::resyncCounterAttr;
MObject MayaUsdProxyShapeListenerBase::stageCacheIdAttr;
MObject MayaUsdProxyShapeListenerBase::outStageCacheIdAttr;

void MayaUsdProxyShapeListenerBase::_ReInit()
{
    _lastKnownStageCacheId = -1;
    _stageNoticeListener.SetStage(nullptr);
    _IncrementCounter(resyncCounterAttr);
    _IncrementCounter(updateCounterAttr);
}

void MayaUsdProxyShapeListenerBase::_OnStageContentsChanged(const UsdNotice::StageContentsChanged&)
{
    _ReInit();
}

void MayaUsdProxyShapeListenerBase::_OnStageObjectsChanged(const UsdNotice::ObjectsChanged& notice)
{
    switch (UsdMayaStageNoticeListener::ClassifyObjectsChanged(notice)) {
    case UsdMayaStageNoticeListener::ChangeType::kIgnored: return;
    case UsdMayaStageNoticeListener::ChangeType::kResync: _IncrementCounter(resyncCounterAttr);
    // [[fallthrough]]; // We want that fallthrough to have the update always triggered.
    case UsdMayaStageNoticeListener::ChangeType::kUpdate:
        _IncrementCounter(updateCounterAttr);
        break;
    }
}

void MayaUsdProxyShapeListenerBase::_IncrementCounter(MObject attribute)
{
    auto plug = MPlug(thisMObject(), attribute);
    plug.setInt64(plug.asInt64() + 1);
}

/* static */
void* MayaUsdProxyShapeListenerBase::creator() { return new MayaUsdProxyShapeListenerBase(); }

/* static */
MStatus MayaUsdProxyShapeListenerBase::initialize()
{
    MStatus retValue = MS::kSuccess;

    //
    // create attr factories
    //
    MFnNumericAttribute numericAttrFn;

    stageCacheIdAttr
        = numericAttrFn.create("stageCacheId", "stcid", MFnNumericData::kInt, -1, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    numericAttrFn.setStorable(false);
    numericAttrFn.setConnectable(false);
    numericAttrFn.setReadable(false);
    retValue = addAttribute(stageCacheIdAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    outStageCacheIdAttr
        = numericAttrFn.create("outStageCacheId", "ostcid", MFnNumericData::kInt, -1, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    numericAttrFn.setStorable(false);
    numericAttrFn.setConnectable(true);
    numericAttrFn.setWritable(false);
    retValue = addAttribute(outStageCacheIdAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    //
    // Smart signaling attributes:
    //
    updateCounterAttr
        = numericAttrFn.create("updateId", "upid", MFnNumericData::kInt64, 0, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    numericAttrFn.setStorable(false);
    numericAttrFn.setHidden(true);
    numericAttrFn.setIndeterminant(true);
    numericAttrFn.setWritable(false);
    numericAttrFn.setCached(false);
    numericAttrFn.setAffectsAppearance(false);
    retValue = addAttribute(updateCounterAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    resyncCounterAttr
        = numericAttrFn.create("resyncId", "rsid", MFnNumericData::kInt64, 0, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    numericAttrFn.setStorable(false);
    numericAttrFn.setHidden(true);
    numericAttrFn.setIndeterminant(true);
    numericAttrFn.setWritable(false);
    numericAttrFn.setCached(false);
    numericAttrFn.setAffectsAppearance(false);
    retValue = addAttribute(resyncCounterAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    //
    // add attribute dependencies
    //
    retValue = attributeAffects(stageCacheIdAttr, outStageCacheIdAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    return retValue;
}

void MayaUsdProxyShapeListenerBase::postConstructor()
{
    _stageNoticeListener.SetStageObjectsChangedCallback(
        [this](const UsdNotice::ObjectsChanged& notice) { return _OnStageObjectsChanged(notice); });
    setExistWithoutInConnections(false);
}

MStatus MayaUsdProxyShapeListenerBase::compute(const MPlug& plug, MDataBlock& dataBlock)
{
    if (plug == outStageCacheIdAttr) {
        MStatus retValue = MS::kSuccess;

        MDataHandle inHandle = dataBlock.inputValue(stageCacheIdAttr, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        int cacheIdNum = inHandle.asInt();

        if (cacheIdNum != _lastKnownStageCacheId) {
            const auto cacheId = UsdStageCache::Id::FromLongInt(cacheIdNum);
            const auto stageCached
                = cacheId.IsValid() && UsdUtilsStageCache::Get().Contains(cacheId);
            if (stageCached) {
                auto usdStage = UsdUtilsStageCache::Get().Find(cacheId);
                _stageNoticeListener.SetStage(usdStage);
            } else {
                _stageNoticeListener.SetStage(nullptr);
            }
            auto increment = [](MDataBlock& dataBlock, MObject attr) -> MStatus {
                MStatus     retStatus = MS::kSuccess;
                MDataHandle inHandle = dataBlock.inputValue(attr, &retStatus);
                CHECK_MSTATUS_AND_RETURN_IT(retStatus);
                MDataHandle outHandle = dataBlock.outputValue(attr, &retStatus);
                CHECK_MSTATUS_AND_RETURN_IT(retStatus);
                outHandle.setInt64(inHandle.asInt64() + 1);
                outHandle.setClean();
                return retStatus;
            };

            retValue = increment(dataBlock, updateCounterAttr);
            CHECK_MSTATUS_AND_RETURN_IT(retValue);
            retValue = increment(dataBlock, resyncCounterAttr);
            CHECK_MSTATUS_AND_RETURN_IT(retValue);

            _lastKnownStageCacheId = cacheIdNum;
        }

        MDataHandle outCacheIdHandle = dataBlock.outputValue(outStageCacheIdAttr, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        outCacheIdHandle.set(cacheIdNum);
        outCacheIdHandle.setClean();

        return retValue;
    }

    return MS::kUnknownParameter;
}

MStatus
MayaUsdProxyShapeListenerBase::connectionMade(const MPlug& plug1, const MPlug& plug2, bool asSrc)
{
    if (plug1 == stageCacheIdAttr || plug2 == stageCacheIdAttr) {
        _ReInit();
    }
    return MPxNode::connectionMade(plug1, plug2, asSrc);
}

MStatus
MayaUsdProxyShapeListenerBase::connectionBroken(const MPlug& plug1, const MPlug& plug2, bool asSrc)
{
    if (plug1 == stageCacheIdAttr || plug2 == stageCacheIdAttr) {
        _ReInit();
    }
    return MPxNode::connectionMade(plug1, plug2, asSrc);
}

// Ancillary: updateId, resyncId
//        react to USD events and update counters as required.

PXR_NAMESPACE_CLOSE_SCOPE
