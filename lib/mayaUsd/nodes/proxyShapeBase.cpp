//
// Copyright 2016 Pixar
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
#include "proxyShapeBase.h"

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/base/tokens.h>
#include <mayaUsd/listeners/proxyShapeNotice.h>
#include <mayaUsd/nodes/stageData.h>
#include <mayaUsd/utils/query.h>
#include <mayaUsd/utils/stageCache.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <pxr/base/gf/bbox3d.h>
#include <pxr/base/gf/range3d.h>
#include <pxr/base/gf/ray.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/fileUtils.h>
#include <pxr/base/tf/hash.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/staticData.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/trace/trace.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/stageCacheContext.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/bboxCache.h>
#include <pxr/usd/usdGeom/imageable.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdUtils/stageCache.h>

#include <maya/MBoundingBox.h>
#include <maya/MDGContext.h>
#include <maya/MDGContextGuard.h>
#include <maya/MDagPath.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MEvaluationNode.h>
#include <maya/MFileIO.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnReference.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MPoint.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MSelectionMask.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTime.h>
#include <maya/MViewport2Renderer.h>

#include <boost/filesystem.hpp>

#include <map>
#include <string>
#include <utility>
#include <vector>

#if defined(WANT_UFE_BUILD)
#include <ufe/path.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#include <ufe/pathString.h>
#endif
#endif

using namespace MAYAUSD_NS_DEF;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(MayaUsdProxyShapeBaseTokens, MAYAUSD_PROXY_SHAPE_BASE_TOKENS);

MayaUsdProxyShapeBase::ClosestPointDelegate MayaUsdProxyShapeBase::_sharedClosestPointDelegate
    = nullptr;

const std::string kAnonymousLayerName { "anonymousLayer1" };

// ========================================================

// TypeID from the MayaUsd type ID range.
const MTypeId MayaUsdProxyShapeBase::typeId(0x58000094);
const MString MayaUsdProxyShapeBase::typeName(MayaUsdProxyShapeBaseTokens->MayaTypeName.GetText());

const MString MayaUsdProxyShapeBase::displayFilterName(
    TfStringPrintf("%sDisplayFilter", MayaUsdProxyShapeBaseTokens->MayaTypeName.GetText()).c_str());
const MString MayaUsdProxyShapeBase::displayFilterLabel("USD Proxies");

// Attributes
MObject MayaUsdProxyShapeBase::filePathAttr;
MObject MayaUsdProxyShapeBase::primPathAttr;
MObject MayaUsdProxyShapeBase::excludePrimPathsAttr;
MObject MayaUsdProxyShapeBase::loadPayloadsAttr;
MObject MayaUsdProxyShapeBase::timeAttr;
MObject MayaUsdProxyShapeBase::complexityAttr;
MObject MayaUsdProxyShapeBase::inStageDataAttr;
MObject MayaUsdProxyShapeBase::inStageDataCachedAttr;
MObject MayaUsdProxyShapeBase::stageCacheIdAttr;
MObject MayaUsdProxyShapeBase::drawRenderPurposeAttr;
MObject MayaUsdProxyShapeBase::drawProxyPurposeAttr;
MObject MayaUsdProxyShapeBase::drawGuidePurposeAttr;
// Output attributes
MObject MayaUsdProxyShapeBase::outTimeAttr;
MObject MayaUsdProxyShapeBase::outStageDataAttr;
MObject MayaUsdProxyShapeBase::outStageCacheIdAttr;

/* static */
void* MayaUsdProxyShapeBase::creator() { return new MayaUsdProxyShapeBase(); }

/* static */
MStatus MayaUsdProxyShapeBase::initialize()
{
    MStatus retValue = MS::kSuccess;

    //
    // create attr factories
    //
    MFnNumericAttribute numericAttrFn;
    MFnTypedAttribute   typedAttrFn;
    MFnUnitAttribute    unitAttrFn;

    filePathAttr
        = typedAttrFn.create("filePath", "fp", MFnData::kString, MObject::kNullObj, &retValue);
    typedAttrFn.setInternal(true);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(filePathAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    primPathAttr
        = typedAttrFn.create("primPath", "pp", MFnData::kString, MObject::kNullObj, &retValue);
    typedAttrFn.setInternal(true);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(primPathAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    excludePrimPathsAttr = typedAttrFn.create(
        "excludePrimPaths", "epp", MFnData::kString, MObject::kNullObj, &retValue);
    typedAttrFn.setInternal(true);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(excludePrimPathsAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    loadPayloadsAttr
        = numericAttrFn.create("loadPayloads", "lpl", MFnNumericData::kBoolean, 1.0, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    numericAttrFn.setKeyable(true);
    numericAttrFn.setReadable(false);
    numericAttrFn.setAffectsAppearance(true);
    retValue = addAttribute(loadPayloadsAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    timeAttr = unitAttrFn.create("time", "tm", MFnUnitAttribute::kTime, 0.0, &retValue);
    unitAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(timeAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    complexityAttr = numericAttrFn.create("complexity", "cplx", MFnNumericData::kInt, 0, &retValue);
    numericAttrFn.setMin(0);
    numericAttrFn.setSoftMax(4);
    numericAttrFn.setMax(8);
    numericAttrFn.setChannelBox(true);
    numericAttrFn.setStorable(false);
    numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(complexityAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    inStageDataAttr = typedAttrFn.create(
        "inStageData", "id", MayaUsdStageData::mayaTypeId, MObject::kNullObj, &retValue);
    typedAttrFn.setReadable(false);
    typedAttrFn.setStorable(false);
    typedAttrFn.setDisconnectBehavior(MFnNumericAttribute::kReset); // on disconnect, reset to Null
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(inStageDataAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // inStageData or filepath-> inStageDataCached -> outStageData
    inStageDataCachedAttr = typedAttrFn.create(
        "inStageDataCached", "idc", MayaUsdStageData::mayaTypeId, MObject::kNullObj, &retValue);
    typedAttrFn.setStorable(false);
    typedAttrFn.setWritable(false);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(inStageDataCachedAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    stageCacheIdAttr
        = numericAttrFn.create("stageCacheId", "stcid", MFnNumericData::kInt, -1, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    numericAttrFn.setDisconnectBehavior(
        MFnNumericAttribute::kReset); // on disconnect, reset to default
    numericAttrFn.setStorable(false);
    numericAttrFn.setCached(true);
    numericAttrFn.setConnectable(true);
    numericAttrFn.setReadable(true);
    numericAttrFn.setInternal(true);
    numericAttrFn.setAffectsAppearance(true);
    retValue = addAttribute(stageCacheIdAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    drawRenderPurposeAttr = numericAttrFn.create(
        "drawRenderPurpose", "drp", MFnNumericData::kBoolean, 0.0, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    numericAttrFn.setKeyable(true);
    numericAttrFn.setReadable(false);
    numericAttrFn.setAffectsAppearance(true);
    retValue = addAttribute(drawRenderPurposeAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    drawProxyPurposeAttr
        = numericAttrFn.create("drawProxyPurpose", "dpp", MFnNumericData::kBoolean, 1.0, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    numericAttrFn.setKeyable(true);
    numericAttrFn.setReadable(false);
    numericAttrFn.setAffectsAppearance(true);
    retValue = addAttribute(drawProxyPurposeAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    drawGuidePurposeAttr
        = numericAttrFn.create("drawGuidePurpose", "dgp", MFnNumericData::kBoolean, 0.0, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    numericAttrFn.setKeyable(true);
    numericAttrFn.setReadable(false);
    numericAttrFn.setAffectsAppearance(true);
    retValue = addAttribute(drawGuidePurposeAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // outputs
    outTimeAttr = unitAttrFn.create("outTime", "otm", MFnUnitAttribute::kTime, 0.0, &retValue);
    unitAttrFn.setCached(false);
    unitAttrFn.setConnectable(true);
    unitAttrFn.setReadable(true);
    unitAttrFn.setStorable(false);
    unitAttrFn.setWritable(false);
    unitAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(outTimeAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    outStageDataAttr = typedAttrFn.create(
        "outStageData", "od", MayaUsdStageData::mayaTypeId, MObject::kNullObj, &retValue);
    typedAttrFn.setStorable(false);
    typedAttrFn.setWritable(false);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(outStageDataAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    outStageCacheIdAttr
        = numericAttrFn.create("outStageCacheId", "ostcid", MFnNumericData::kInt, -1, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    numericAttrFn.setStorable(false);
    numericAttrFn.setWritable(false);
    retValue = addAttribute(outStageCacheIdAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    //
    // add attribute dependencies
    //
    retValue = attributeAffects(timeAttr, outTimeAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    retValue = attributeAffects(filePathAttr, inStageDataCachedAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = attributeAffects(filePathAttr, outStageDataAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = attributeAffects(filePathAttr, outStageCacheIdAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    retValue = attributeAffects(primPathAttr, inStageDataCachedAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = attributeAffects(primPathAttr, outStageDataAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = attributeAffects(primPathAttr, outStageCacheIdAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    retValue = attributeAffects(loadPayloadsAttr, inStageDataCachedAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = attributeAffects(loadPayloadsAttr, outStageDataAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = attributeAffects(loadPayloadsAttr, outStageCacheIdAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    retValue = attributeAffects(inStageDataAttr, inStageDataCachedAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = attributeAffects(inStageDataAttr, outStageDataAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = attributeAffects(inStageDataAttr, outStageCacheIdAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    retValue = attributeAffects(stageCacheIdAttr, outStageDataAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = attributeAffects(stageCacheIdAttr, inStageDataCachedAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = attributeAffects(stageCacheIdAttr, outStageCacheIdAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    retValue = attributeAffects(inStageDataCachedAttr, outStageDataAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = attributeAffects(inStageDataCachedAttr, outStageCacheIdAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    return retValue;
}

/* static */
MayaUsdProxyShapeBase* MayaUsdProxyShapeBase::GetShapeAtDagPath(const MDagPath& dagPath)
{
    MObject mObj = dagPath.node();
    if (mObj.apiType() != MFn::kPluginShape) {
        TF_CODING_ERROR(
            "Could not get MayaUsdProxyShapeBase for non-plugin shape node "
            "at DAG path: %s (apiTypeStr = %s)",
            dagPath.fullPathName().asChar(),
            mObj.apiTypeStr());
        return nullptr;
    }

    const MFnDependencyNode depNodeFn(mObj);
    MayaUsdProxyShapeBase*  pShape = static_cast<MayaUsdProxyShapeBase*>(depNodeFn.userNode());
    if (!pShape) {
        TF_CODING_ERROR(
            "Could not get MayaUsdProxyShapeBase for node at DAG path: %s",
            dagPath.fullPathName().asChar());
        return nullptr;
    }

    return pShape;
}

/* static */
void MayaUsdProxyShapeBase::SetClosestPointDelegate(ClosestPointDelegate delegate)
{
    _sharedClosestPointDelegate = delegate;
}

/* virtual */
bool MayaUsdProxyShapeBase::GetObjectSoftSelectEnabled() const { return false; }

void MayaUsdProxyShapeBase::enableProxyAccessor()
{
    _usdAccessor = ProxyAccessor::createAndRegister(*this);
}

/* virtual */
void MayaUsdProxyShapeBase::postConstructor()
{
    setRenderable(true);

    MayaUsdProxyStageInvalidateNotice(*this).Send();
}

/* virtual */
MStatus MayaUsdProxyShapeBase::compute(const MPlug& plug, MDataBlock& dataBlock)
{
    if (plug == outTimeAttr || plug.isDynamic())
        ProxyAccessor::compute(_usdAccessor, plug, dataBlock);

    if (plug == excludePrimPathsAttr || plug == timeAttr || plug == complexityAttr
        || plug == drawRenderPurposeAttr || plug == drawProxyPurposeAttr
        || plug == drawGuidePurposeAttr) {
        // If the attribute that needs to be computed is one of these, then it
        // does not affect the ouput stage data, but it *does* affect imaging
        // the shape. In that case, we notify Maya that the shape needs to be
        // redrawn and let it take care of computing the attribute. This covers
        // the case where an attribute on the proxy shape may have an incoming
        // connection from another node (e.g. "time1.outTime" being connected
        // to the proxy shape's "time" attribute). In that case,
        // setDependentsDirty() might not get called and only compute() might.
        MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());
        return MS::kUnknownParameter;
    } else if (plug == inStageDataCachedAttr) {
        return computeInStageDataCached(dataBlock);
    } else if (plug == outTimeAttr) {
        auto retStatus = computeOutputTime(dataBlock);
        ProxyAccessor::compute(_usdAccessor, plug, dataBlock);
        return retStatus;
    } else if (plug == outStageDataAttr) {
        return computeOutStageData(dataBlock);
    } else if (plug == outStageCacheIdAttr) {
        return computeOutStageCacheId(dataBlock);
    } else if (plug.isDynamic()) {
        return ProxyAccessor::compute(_usdAccessor, plug, dataBlock);
    }

    return MS::kUnknownParameter;
}

/* virtual */
SdfLayerRefPtr MayaUsdProxyShapeBase::computeSessionLayer(MDataBlock&) { return nullptr; }

MStatus MayaUsdProxyShapeBase::computeInStageDataCached(MDataBlock& dataBlock)
{
    MStatus retValue = MS::kSuccess;

    MDataHandle inDataHandle = dataBlock.inputValue(inStageDataAttr, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // If inData has an incoming connection, then use it. Otherwise generate stage from the filepath
    if (!inDataHandle.data().isNull()) {
        //
        // Propagate inData -> inDataCached
        //
        MDataHandle inDataCachedHandle = dataBlock.outputValue(inStageDataCachedAttr, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        inDataCachedHandle.copy(inDataHandle);

        inDataCachedHandle.setClean();
        return MS::kSuccess;
    } else if (!dataBlock.context().isNormal()) {
        // Create the output outData ========
        MFnPluginData pluginDataFn;
        MObject       stageDataObj = pluginDataFn.create(MayaUsdStageData::mayaTypeId, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        MayaUsdStageData* outData
            = reinterpret_cast<MayaUsdStageData*>(pluginDataFn.data(&retValue));
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        // When evaluating in background we should point to the same stage as in normal context
        // This way we will share the stage between all evaluation context and avoid losing data
        // in case of dirty stage, i.e. stage with new or modified layers.
        MDGContext normalContext;
        {
            MDGContextGuard contextGuard(normalContext);
            MDataBlock      dataBlockForNormalContext = forceCache();

            MDataHandle inDataCachedHandleForNormalContext
                = dataBlockForNormalContext.inputValue(inStageDataCachedAttr, &retValue);
            CHECK_MSTATUS_AND_RETURN_IT(retValue);

            MayaUsdStageData* inData = dynamic_cast<MayaUsdStageData*>(
                inDataCachedHandleForNormalContext.asPluginData());

            // Set the outUsdStageData
            outData->stage = inData->stage;
            outData->primPath = inData->primPath;
        }

        // Retrieve data handle for stage data cache
        MDataHandle outDataCachedHandle = dataBlock.outputValue(inStageDataCachedAttr, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        outDataCachedHandle.set(outData);
        outDataCachedHandle.setClean();
        return MS::kSuccess;
    } else {
        // Check if we have a Stage from the Cache Id
        const auto cacheIdNum = dataBlock.inputValue(stageCacheIdAttr, &retValue).asInt();
        CHECK_MSTATUS_AND_RETURN_IT(retValue);
        UsdStageRefPtr usdStage;
        const auto     cacheId = UsdStageCache::Id::FromLongInt(cacheIdNum);
        const auto stageCached = cacheId.IsValid() && UsdUtilsStageCache::Get().Contains(cacheId);
        if (stageCached) {
            usdStage = UsdUtilsStageCache::Get().Find(cacheId);
        } else {
            //
            // Calculate from USD filepath and primPath and variantKey
            //

            // Get input attr values
            const MString file = dataBlock.inputValue(filePathAttr, &retValue).asString();
            CHECK_MSTATUS_AND_RETURN_IT(retValue);

            //
            // let the usd stage cache deal with caching the usd stage data
            //
            std::string fileString = TfStringTrimRight(file.asChar());

            TF_DEBUG(USDMAYA_PROXYSHAPEBASE)
                .Msg(
                    "ProxyShapeBase::reloadStage original USD file path is %s\n",
                    fileString.c_str());

            boost::filesystem::path filestringPath(fileString);
            if (filestringPath.is_absolute()) {
                fileString = UsdMayaUtilFileSystem::resolvePath(fileString);
                TF_DEBUG(USDMAYA_PROXYSHAPEBASE)
                    .Msg(
                        "ProxyShapeBase::reloadStage resolved the USD file path to %s\n",
                        fileString.c_str());
            } else {
                fileString = UsdMayaUtilFileSystem::resolveRelativePathWithinMayaContext(
                    thisMObject(), fileString);
                TF_DEBUG(USDMAYA_PROXYSHAPEBASE)
                    .Msg(
                        "ProxyShapeBase::reloadStage resolved the relative USD file path to %s\n",
                        fileString.c_str());
            }

            // Fall back on providing the path "as is" to USD
            if (fileString.empty()) {
                fileString.assign(file.asChar(), file.length());
            }

            TF_DEBUG(USDMAYA_PROXYSHAPEBASE)
                .Msg("ProxyShapeBase::loadStage called for the usd file: %s\n", fileString.c_str());

            // == Load the Stage
            auto loadSet = UsdStage::InitialLoadSet::LoadAll;

            MDataHandle loadPayloadsHandle = dataBlock.inputValue(loadPayloadsAttr, &retValue);
            CHECK_MSTATUS_AND_RETURN_IT(retValue);
            if (!loadPayloadsHandle.asBool()) {
                loadSet = UsdStage::InitialLoadSet::LoadNone;
            }

            {
                // When opening or creating stages we must have an active UsdStageCache.
                // The stage cache is the only one who holds a strong reference to the
                // UsdStage. See https://github.com/Autodesk/maya-usd/issues/528 for
                // more information.
                UsdStageCacheContext ctx(
                    UsdMayaStageCache::Get(loadSet == UsdStage::InitialLoadSet::LoadAll));

                if (SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(fileString)) {
                    SdfLayerRefPtr sessionLayer = computeSessionLayer(dataBlock);

                    bool targetSession
                        = MGlobal::optionVarIntValue(UsdMayaUtil::convert(
                              MayaUsdOptionVars->mayaUsd_ProxyTargetsSessionLayerOnOpen))
                        == 1;
                    targetSession = targetSession || !rootLayer->PermissionToEdit();

                    if (sessionLayer || targetSession) {
                        if (!sessionLayer)
                            sessionLayer = SdfLayer::CreateAnonymous();
                        usdStage = UsdStage::Open(
                            rootLayer, sessionLayer, ArGetResolver().GetCurrentContext(), loadSet);
                    } else {
                        usdStage = UsdStage::Open(
                            rootLayer, ArGetResolver().GetCurrentContext(), loadSet);
                    }
                    if (sessionLayer && targetSession) {
                        usdStage->SetEditTarget(sessionLayer);
                    } else {
                        usdStage->SetEditTarget(usdStage->GetRootLayer());
                    }
                } else {
                    // Create a new stage in memory with an anonymous root layer.
                    usdStage = UsdStage::CreateInMemory(kAnonymousLayerName, loadSet);
                }
            }
        }
        SdfPath primPath;
        if (usdStage) {
            primPath = usdStage->GetPseudoRoot().GetPath();
        }

        // Create the output outData ========
        MFnPluginData pluginDataFn;
        MObject       stageDataObj = pluginDataFn.create(MayaUsdStageData::mayaTypeId, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        MayaUsdStageData* stageData
            = reinterpret_cast<MayaUsdStageData*>(pluginDataFn.data(&retValue));
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        // Set the outUsdStageData
        stageData->stage = usdStage;
        stageData->primPath = primPath;

        //
        // set the data on the output plug
        //
        MDataHandle inDataCachedHandle = dataBlock.outputValue(inStageDataCachedAttr, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        inDataCachedHandle.set(stageData);
        inDataCachedHandle.setClean();
        return MS::kSuccess;
    }
}

MStatus MayaUsdProxyShapeBase::computeOutStageData(MDataBlock& dataBlock)
{
    MStatus retValue = MS::kSuccess;

    const bool isNormalContext = dataBlock.context().isNormal();
    if (isNormalContext) {
        TfReset(_boundingBoxCache);

        // Reset the stage listener until we determine that everything is valid.
        _stageNoticeListener.SetStage(UsdStageWeakPtr());
        _stageNoticeListener.SetStageContentsChangedCallback(nullptr);
        _stageNoticeListener.SetStageObjectsChangedCallback(nullptr);
    }

    MDataHandle inDataCachedHandle = dataBlock.inputValue(inStageDataCachedAttr, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    UsdStageRefPtr usdStage;

    MayaUsdStageData* inData = dynamic_cast<MayaUsdStageData*>(inDataCachedHandle.asPluginData());
    if (inData) {
        usdStage = inData->stage;
    }

    // If failed to get a valid stage, then
    // Propagate inDataCached -> outData
    // and return
    if (!usdStage) {
        MDataHandle outDataHandle = dataBlock.outputValue(outStageDataAttr, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);
        outDataHandle.copy(inDataCachedHandle);
        return MS::kSuccess;
    }

    // Get the primPath
    const MString primPath = dataBlock.inputValue(primPathAttr, &retValue).asString();
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // Get the prim
    // If no primPath string specified, then use the pseudo-root.
    UsdPrim     usdPrim;
    std::string primPathStr(primPath.asChar(), primPath.length());
    if (!primPathStr.empty()) {
        SdfPath primPath(primPathStr);

        // Validate assumption: primPath is descendent of passed-in stage primPath
        //   Make sure that the primPath is a child of the passed in stage's primpath
        if (primPath.HasPrefix(inData->primPath)) {
            usdPrim = usdStage->GetPrimAtPath(primPath);
        } else {
            TF_WARN(
                "%s: Shape primPath <%s> is not a descendant of input "
                "stage primPath <%s>",
                MPxSurfaceShape::name().asChar(),
                primPath.GetText(),
                inData->primPath.GetText());
        }
    } else {
        usdPrim = usdStage->GetPseudoRoot();
    }

    // Create the output outData
    MFnPluginData pluginDataFn;
    MObject       stageDataObj = pluginDataFn.create(MayaUsdStageData::mayaTypeId, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    MayaUsdStageData* stageData = reinterpret_cast<MayaUsdStageData*>(pluginDataFn.data(&retValue));
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // Set the outUsdStageData
    stageData->stage = usdStage;
    stageData->primPath = usdPrim ? usdPrim.GetPath() : usdStage->GetPseudoRoot().GetPath();

    //
    // set the data on the output plug
    //
    MDataHandle outDataHandle = dataBlock.outputValue(outStageDataAttr, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    outDataHandle.set(stageData);
    outDataHandle.setClean();

    if (isNormalContext) {
        // Start listening for notices for the USD stage.
        _stageNoticeListener.SetStage(usdStage);
        _stageNoticeListener.SetStageContentsChangedCallback(
            [this](const UsdNotice::StageContentsChanged& notice) {
                return _OnStageContentsChanged(notice);
            });

        _stageNoticeListener.SetStageObjectsChangedCallback(
            [this](const UsdNotice::ObjectsChanged& notice) {
                return _OnStageObjectsChanged(notice);
            });

        MayaUsdProxyStageSetNotice(*this).Send();
    }

    return MS::kSuccess;
}

MStatus MayaUsdProxyShapeBase::computeOutputTime(MDataBlock& dataBlock)
{
    MStatus     retValue = MS::kSuccess;
    MDataHandle inDataHandle = dataBlock.inputValue(timeAttr, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    MTime inTime = inDataHandle.asTime();

    MDataHandle outDataHandle = dataBlock.outputValue(outTimeAttr, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    outDataHandle.set(inTime);
    outDataHandle.setClean();

    return retValue;
}

MStatus MayaUsdProxyShapeBase::computeOutStageCacheId(MDataBlock& dataBlock)
{
    MStatus retValue = MS::kSuccess;

    MDataHandle inDataCachedHandle = dataBlock.inputValue(inStageDataCachedAttr, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    UsdStageRefPtr usdStage;

    MayaUsdStageData* inData = dynamic_cast<MayaUsdStageData*>(inDataCachedHandle.asPluginData());
    if (inData) {
        usdStage = inData->stage;
    }

    if (!usdStage) {
        return MS::kFailure;
    }

    int  cacheId = -1;
    auto id = UsdUtilsStageCache::Get().Insert(usdStage);
    if (id)
        cacheId = id.ToLongInt();

    MDataHandle outCacheIdHandle = dataBlock.outputValue(outStageCacheIdAttr, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    outCacheIdHandle.set(cacheId);
    outCacheIdHandle.setClean();

    return MS::kSuccess;
}

/* virtual */
bool MayaUsdProxyShapeBase::isBounded() const { return isStageValid(); }

/* virtual */
void MayaUsdProxyShapeBase::CacheEmptyBoundingBox(MBoundingBox&) { }

/* virtual */
UsdTimeCode MayaUsdProxyShapeBase::GetOutputTime(MDataBlock dataBlock) const
{
    return _GetTime(dataBlock);
}

/* virtual */
MBoundingBox MayaUsdProxyShapeBase::boundingBox() const
{
    TRACE_FUNCTION();

    MStatus status;

    // Make sure outStage is up to date
    MayaUsdProxyShapeBase* nonConstThis = const_cast<ThisClass*>(this);
    MDataBlock             dataBlock = nonConstThis->forceCache();
    dataBlock.inputValue(outStageDataAttr, &status);
    CHECK_MSTATUS_AND_RETURN(status, MBoundingBox());

    // XXX:
    // If we could cheaply determine whether a stage only has static geometry,
    // we could make this value a constant one for that case, avoiding the
    // memory overhead of a cache entry per frame
    UsdTimeCode currTime = GetOutputTime(dataBlock);

    std::map<UsdTimeCode, MBoundingBox>::const_iterator cacheLookup
        = _boundingBoxCache.find(currTime);

    if (cacheLookup != _boundingBoxCache.end()) {
        return cacheLookup->second;
    }

    UsdPrim prim = _GetUsdPrim(dataBlock);
    if (!prim) {
        return MBoundingBox();
    }

    const UsdGeomImageable imageablePrim(prim);

    bool drawRenderPurpose = false;
    bool drawProxyPurpose = true;
    bool drawGuidePurpose = false;
    _GetDrawPurposeToggles(dataBlock, &drawRenderPurpose, &drawProxyPurpose, &drawGuidePurpose);

    const TfToken purpose1 = UsdGeomTokens->default_;
    const TfToken purpose2 = drawRenderPurpose ? UsdGeomTokens->render : TfToken();
    const TfToken purpose3 = drawProxyPurpose ? UsdGeomTokens->proxy : TfToken();
    const TfToken purpose4 = drawGuidePurpose ? UsdGeomTokens->guide : TfToken();

    const GfBBox3d allBox
        = imageablePrim.ComputeUntransformedBound(currTime, purpose1, purpose2, purpose3, purpose4);

    MBoundingBox& retval = nonConstThis->_boundingBoxCache[currTime];

    const GfRange3d boxRange = allBox.ComputeAlignedBox();

    // Convert to GfRange3d to MBoundingBox
    if (!boxRange.IsEmpty()) {
        const GfVec3d boxMin = boxRange.GetMin();
        const GfVec3d boxMax = boxRange.GetMax();
        retval = MBoundingBox(
            MPoint(boxMin[0], boxMin[1], boxMin[2]), MPoint(boxMax[0], boxMax[1], boxMax[2]));
    } else {
        nonConstThis->CacheEmptyBoundingBox(retval);
    }

    return retval;
}

void MayaUsdProxyShapeBase::clearBoundingBoxCache() { _boundingBoxCache.clear(); }

bool MayaUsdProxyShapeBase::isStageValid() const
{
    MStatus                localStatus;
    MayaUsdProxyShapeBase* nonConstThis = const_cast<MayaUsdProxyShapeBase*>(this);
    MDataBlock             dataBlock = nonConstThis->forceCache();

    MDataHandle outDataHandle = dataBlock.inputValue(outStageDataAttr, &localStatus);
    CHECK_MSTATUS_AND_RETURN(localStatus, false);

    MayaUsdStageData* outData = dynamic_cast<MayaUsdStageData*>(outDataHandle.asPluginData());
    if (!outData || !outData->stage) {
        return false;
    }

    return true;
}

MStatus MayaUsdProxyShapeBase::preEvaluation(
    const MDGContext&      context,
    const MEvaluationNode& evaluationNode)
{
    // Any logic here should have an equivalent implementation in
    // MayaUsdProxyShapeBase::setDependentsDirty().
    if (context.isNormal()) {
        if (evaluationNode.dirtyPlugExists(excludePrimPathsAttr)) {
            _IncreaseExcludePrimPathsVersion();
        } else if (
            evaluationNode.dirtyPlugExists(outStageDataAttr) ||
            // All the plugs that affect outStageDataAttr
            evaluationNode.dirtyPlugExists(filePathAttr)
            || evaluationNode.dirtyPlugExists(primPathAttr)
            || evaluationNode.dirtyPlugExists(loadPayloadsAttr)
            || evaluationNode.dirtyPlugExists(inStageDataAttr)
            || evaluationNode.dirtyPlugExists(stageCacheIdAttr)) {
            _IncreaseUsdStageVersion();
            MayaUsdProxyStageInvalidateNotice(*this).Send();
        }
    }

    return MPxSurfaceShape::preEvaluation(context, evaluationNode);
}

MStatus MayaUsdProxyShapeBase::postEvaluation(
    const MDGContext&      context,
    const MEvaluationNode& evaluationNode,
    PostEvaluationType     evalType)
{
    // When a node is evaluated by evaluation manager setDependentsDirty is not called. The
    // functionality in setDependentsDirty needs to be duplicated in preEvaluation or
    // postEvaluation. I don't think we need the call to setGeometryDrawDirty() in
    // setDependentsDirty, but there may be a workflow I'm not seeing that does require it. I'm
    // leaving this here commented out as a reminder that we should either have both calls to
    // setGeometryDrawDirty, or no calls to setGeometryDrawDirty.
    // MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());

    if (context.isNormal() && evalType == PostEvaluationEnum::kEvaluatedDirectly) {
        MDataBlock dataBlock = forceCache();
        ProxyAccessor::syncCache(_usdAccessor, thisMObject(), dataBlock);
    }

    return MPxSurfaceShape::postEvaluation(context, evaluationNode, evalType);
}

/* virtual */
MStatus MayaUsdProxyShapeBase::setDependentsDirty(const MPlug& plug, MPlugArray& plugArray)
{
    // Any logic here should have an equivalent implementation in
    // MayaUsdProxyShapeBase::preEvaluation() or postEvaluation().

    MStatus retValue;

    // If/when the MPxDrawOverride for the proxy shape specifies
    // isAlwaysDirty=false to improve performance, we must be sure to notify
    // the Maya renderer that the geometry is dirty and needs to be redrawn
    // when any plug on the proxy shape is dirtied.
    MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());

    if (plug == excludePrimPathsAttr) {
        _IncreaseExcludePrimPathsVersion();
    } else if (
        plug == outStageDataAttr ||
        // All the plugs that affect outStageDataAttr
        plug == filePathAttr || plug == primPathAttr || plug == loadPayloadsAttr
        || plug == inStageDataAttr || plug == stageCacheIdAttr) {
        _IncreaseUsdStageVersion();
        MayaUsdProxyStageInvalidateNotice(*this).Send();
    }

    retValue = MPxSurfaceShape::setDependentsDirty(plug, plugArray);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // If accessor returns success when adding dirty plugs we have to get renderer to
    // trigger compute. We achieve it by adding timeAttr to dirty plugArray. This will guarantee
    // we don't render something that requires inputs evaluted by DG.
    if (plug == timeAttr || plug.isDynamic()) {
        if (ProxyAccessor::addDependentsDirty(_usdAccessor, plug, plugArray) == MS::kSuccess) {
            MPlug outTimePlug(thisMObject(), outTimeAttr);
            plugArray.append(outTimePlug);
        }
    }

    return retValue;
}

#if MAYA_API_VERSION >= 20210000
/* virtual */
void MayaUsdProxyShapeBase::getCacheSetup(
    const MEvaluationNode&   evalNode,
    MNodeCacheDisablingInfo& disablingInfo,
    MNodeCacheSetupInfo&     cacheSetupInfo,
    MObjectArray&            monitoredAttributes) const
{
    MPxSurfaceShape::getCacheSetup(evalNode, disablingInfo, cacheSetupInfo, monitoredAttributes);
    // We want this node to be cached by default (unless cache rules have been configured
    // to exclude it.
    cacheSetupInfo.setPreference(MNodeCacheSetupInfo::kWantToCacheByDefault, true);
}

/* virtual */
void MayaUsdProxyShapeBase::configCache(const MEvaluationNode& evalNode, MCacheSchema& schema) const
{
    MPxSurfaceShape::configCache(evalNode, schema);
    // Out time is not always a dirty plug, but time can be animated. This is why we will
    // store input time and enable quick compute within proxy shape for out time
    schema.add(timeAttr);
}
#endif

UsdPrim MayaUsdProxyShapeBase::_GetUsdPrim(MDataBlock dataBlock) const
{
    MStatus localStatus;
    UsdPrim usdPrim;

    MDataHandle outDataHandle = dataBlock.inputValue(outStageDataAttr, &localStatus);
    CHECK_MSTATUS_AND_RETURN(localStatus, usdPrim);

    MayaUsdStageData* outData = dynamic_cast<MayaUsdStageData*>(outDataHandle.asPluginData());
    if (!outData) {
        return usdPrim; // empty UsdPrim
    }

    if (!outData->stage) {
        return usdPrim; // empty UsdPrim
    }

    usdPrim = (outData->primPath.IsEmpty()) ? outData->stage->GetPseudoRoot()
                                            : outData->stage->GetPrimAtPath(outData->primPath);

    return usdPrim;
}

int MayaUsdProxyShapeBase::getComplexity() const
{
    return _GetComplexity(const_cast<MayaUsdProxyShapeBase*>(this)->forceCache());
}

int MayaUsdProxyShapeBase::_GetComplexity(MDataBlock dataBlock) const
{
    int     complexity = 0;
    MStatus status;

    complexity = dataBlock.inputValue(complexityAttr, &status).asInt();

    return complexity;
}

UsdTimeCode MayaUsdProxyShapeBase::getTime() const
{
    return _GetTime(const_cast<MayaUsdProxyShapeBase*>(this)->forceCache());
}

UsdTimeCode MayaUsdProxyShapeBase::_GetTime(MDataBlock dataBlock) const
{
    MStatus status;

    return UsdTimeCode(dataBlock.inputValue(outTimeAttr, &status).asTime().value());
}

UsdStageRefPtr MayaUsdProxyShapeBase::getUsdStage() const
{
    MStatus                localStatus;
    MayaUsdProxyShapeBase* nonConstThis = const_cast<MayaUsdProxyShapeBase*>(this);
    MDataBlock             dataBlock = nonConstThis->forceCache();

    MDataHandle outDataHandle = dataBlock.inputValue(outStageDataAttr, &localStatus);
    CHECK_MSTATUS_AND_RETURN(localStatus, UsdStageRefPtr());

    MayaUsdStageData* outData = dynamic_cast<MayaUsdStageData*>(outDataHandle.asPluginData());

    if (outData && outData->stage)
        return outData->stage;
    else
        return {};
}

size_t MayaUsdProxyShapeBase::getUsdStageVersion() const { return _UsdStageVersion; }

void MayaUsdProxyShapeBase::getDrawPurposeToggles(
    bool* drawRenderPurpose,
    bool* drawProxyPurpose,
    bool* drawGuidePurpose) const
{
    MDataBlock dataBlock = const_cast<MayaUsdProxyShapeBase*>(this)->forceCache();
    _GetDrawPurposeToggles(dataBlock, drawRenderPurpose, drawProxyPurpose, drawGuidePurpose);
}

SdfPathVector MayaUsdProxyShapeBase::getExcludePrimPaths() const
{
    return _GetExcludePrimPaths(const_cast<MayaUsdProxyShapeBase*>(this)->forceCache());
}

size_t MayaUsdProxyShapeBase::getExcludePrimPathsVersion() const
{
    return _excludePrimPathsVersion;
}

SdfPathVector MayaUsdProxyShapeBase::_GetExcludePrimPaths(MDataBlock dataBlock) const
{
    SdfPathVector ret;

    const MString excludePrimPathsStr = dataBlock.inputValue(excludePrimPathsAttr).asString();
    std::vector<std::string> excludePrimPaths = TfStringTokenize(excludePrimPathsStr.asChar(), ",");
    ret.resize(excludePrimPaths.size());
    for (size_t i = 0; i < excludePrimPaths.size(); ++i) {
        ret[i] = SdfPath(TfStringTrim(excludePrimPaths[i]));
    }

    return ret;
}

bool MayaUsdProxyShapeBase::_GetDrawPurposeToggles(
    MDataBlock dataBlock,
    bool*      drawRenderPurpose,
    bool*      drawProxyPurpose,
    bool*      drawGuidePurpose) const
{
    MStatus status;

    MDataHandle drawRenderPurposeHandle = dataBlock.inputValue(drawRenderPurposeAttr, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MDataHandle drawProxyPurposeHandle = dataBlock.inputValue(drawProxyPurposeAttr, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MDataHandle drawGuidePurposeHandle = dataBlock.inputValue(drawGuidePurposeAttr, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    if (drawRenderPurpose) {
        *drawRenderPurpose = drawRenderPurposeHandle.asBool();
    }
    if (drawProxyPurpose) {
        *drawProxyPurpose = drawProxyPurposeHandle.asBool();
    }
    if (drawGuidePurpose) {
        *drawGuidePurpose = drawGuidePurposeHandle.asBool();
    }

    return true;
}

bool MayaUsdProxyShapeBase::GetAllRenderAttributes(
    UsdPrim*       usdPrimOut,
    SdfPathVector* excludePrimPathsOut,
    int*           complexityOut,
    UsdTimeCode*   timeOut,
    bool*          drawRenderPurpose,
    bool*          drawProxyPurpose,
    bool*          drawGuidePurpose)
{
    MDataBlock dataBlock = forceCache();

    *usdPrimOut = _GetUsdPrim(dataBlock);
    if (!usdPrimOut->IsValid()) {
        return false;
    }

    *excludePrimPathsOut = _GetExcludePrimPaths(dataBlock);
    *complexityOut = _GetComplexity(dataBlock);
    *timeOut = _GetTime(dataBlock);

    _GetDrawPurposeToggles(dataBlock, drawRenderPurpose, drawProxyPurpose, drawGuidePurpose);

    return true;
}

/* virtual */
UsdPrim MayaUsdProxyShapeBase::usdPrim() const
{
    return _GetUsdPrim(const_cast<MayaUsdProxyShapeBase*>(this)->forceCache());
}

MDagPath MayaUsdProxyShapeBase::parentTransform()
{
    MFnDagNode fn(thisMObject());
    MDagPath   proxyTransformPath;
    fn.getPath(proxyTransformPath);
    proxyTransformPath.pop();
    return proxyTransformPath;
}

MayaUsdProxyShapeBase::MayaUsdProxyShapeBase(const bool enableUfeSelection)
    : MPxSurfaceShape()
    , _isUfeSelectionEnabled(enableUfeSelection)
{
    TfRegistryManager::GetInstance().SubscribeTo<MayaUsdProxyShapeBase>();
}

/* virtual */
MayaUsdProxyShapeBase::~MayaUsdProxyShapeBase()
{
    //
    // empty
    //
}

MSelectionMask MayaUsdProxyShapeBase::getShapeSelectionMask() const
{
    // The intent of this function is to control whether this object is
    // selectable at all in VP2

    // However, due to a bug / quirk, it could be used to specifically control
    // whether the object was SOFT-selectable if you were using
    // MAYA_VP2_USE_VP1_SELECTON; in this mode, this setting is NOT querierd
    // when doing "normal" selection, but IS queried when doing soft
    // selection.

    // Unfortunately, it is queried for both "normal" selection AND soft
    // selection if you are using "true" VP2 selection.  So in order to
    // control soft selection, in both modes, we keep track of whether
    // we currently have object soft-select enabled, and then return an empty
    // selection mask if it is, but this object is set to be non-soft-selectable

    static const MSelectionMask emptyMask;
    static const MSelectionMask normalMask(MSelectionMask::kSelectMeshes);

    if (GetObjectSoftSelectEnabled() && !canBeSoftSelected()) {
        // Disable selection, to disable soft-selection
        return emptyMask;
    }
    return normalMask;
}

bool MayaUsdProxyShapeBase::canBeSoftSelected() const { return false; }

void MayaUsdProxyShapeBase::_OnStageContentsChanged(const UsdNotice::StageContentsChanged& notice)
{
    // If the USD stage this proxy represents changes without Maya's knowledge,
    // we need to inform Maya that the shape is dirty and needs to be redrawn.
    MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());
}

void MayaUsdProxyShapeBase::_OnStageObjectsChanged(const UsdNotice::ObjectsChanged& notice)
{
    ProxyAccessor::stageChanged(_usdAccessor, thisMObject(), notice);
}

bool MayaUsdProxyShapeBase::closestPoint(
    const MPoint&  raySource,
    const MVector& rayDirection,
    MPoint&        theClosestPoint,
    MVector&       theClosestNormal,
    bool /*findClosestOnMiss*/,
    double /*tolerance*/)
{
    if (_sharedClosestPointDelegate) {
        GfRay ray(
            GfVec3d(raySource.x, raySource.y, raySource.z),
            GfVec3d(rayDirection.x, rayDirection.y, rayDirection.z));
        GfVec3d hitPoint;
        GfVec3d hitNorm;
        if (_sharedClosestPointDelegate(*this, ray, &hitPoint, &hitNorm)) {
            theClosestPoint = MPoint(hitPoint[0], hitPoint[1], hitPoint[2]);
            theClosestNormal = MVector(hitNorm[0], hitNorm[1], hitNorm[2]);
            return true;
        }
    }

    return false;
}

bool MayaUsdProxyShapeBase::canMakeLive() const { return (bool)_sharedClosestPointDelegate; }

#if defined(WANT_UFE_BUILD)
Ufe::Path MayaUsdProxyShapeBase::ufePath() const
{
    // Build a path segment to proxyShape
    MDagPath thisPath;
    MDagPath::getAPathTo(thisMObject(), thisPath);

#ifdef UFE_V2_FEATURES_AVAILABLE
    return Ufe::PathString::path(thisPath.fullPathName().asChar());
#else
    // MDagPath does not include |world to its full path name
    MString fullpath = "|world" + thisPath.fullPathName();

    return Ufe::Path(Ufe::PathSegment(fullpath.asChar(), MAYA_UFE_RUNTIME_ID, MAYA_UFE_SEPARATOR));
#endif
}
#endif

PXR_NAMESPACE_CLOSE_SCOPE
