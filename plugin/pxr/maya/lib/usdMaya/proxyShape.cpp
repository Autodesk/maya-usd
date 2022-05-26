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
#include "usdMaya/proxyShape.h"

#include <mayaUsd/nodes/hdImagingShape.h>
#include <mayaUsd/nodes/proxyShapePlugin.h>
#include <mayaUsd/nodes/stageData.h>
#include <mayaUsd/render/pxrUsdMayaGL/batchRenderer.h>
#include <mayaUsd/utils/query.h>
#include <mayaUsd/utils/stageCache.h>
#include <mayaUsd/utils/util.h>

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
#include <maya/MDagPath.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MGlobal.h>
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

#include <map>
#include <string>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdMayaProxyShapeTokens, PXRUSDMAYA_PROXY_SHAPE_TOKENS);

// Hydra performs its own high-performance frustum culling, so
// we don't want to rely on Maya to do it on the CPU. AS such, the best
// performance comes from telling Maya to pretend that every object has no
// bounds.
TF_DEFINE_ENV_SETTING(
    PIXMAYA_ENABLE_BOUNDING_BOX_MODE,
    false,
    "Enable bounding box rendering (slows refresh rate)");

UsdMayaProxyShape::ObjectSoftSelectEnabledDelegate
    UsdMayaProxyShape::_sharedObjectSoftSelectEnabledDelegate
    = nullptr;

// ========================================================

const MTypeId UsdMayaProxyShape::typeId(0x0010A259);
const MString UsdMayaProxyShape::typeName(UsdMayaProxyShapeTokens->MayaTypeName.GetText());

// Attributes
MObject UsdMayaProxyShape::variantKeyAttr;
MObject UsdMayaProxyShape::fastPlaybackAttr;
MObject UsdMayaProxyShape::softSelectableAttr;

/* static */
void* UsdMayaProxyShape::creator() { return new UsdMayaProxyShape(); }

/* static */
MStatus UsdMayaProxyShape::initialize()
{
    MStatus retValue = inheritAttributesFrom(MayaUsdProxyShapeBase::typeName);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    //
    // create attr factories
    //
    MFnNumericAttribute numericAttrFn;
    MFnTypedAttribute   typedAttrFn;

    variantKeyAttr = typedAttrFn.create(
        "variantKey", "variantKey", MFnData::kString, MObject::kNullObj, &retValue);
    typedAttrFn.setReadable(false);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(variantKeyAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    fastPlaybackAttr
        = numericAttrFn.create("fastPlayback", "fs", MFnNumericData::kBoolean, 0, &retValue);
    numericAttrFn.setInternal(true);
    numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(fastPlaybackAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    softSelectableAttr = numericAttrFn.create(
        "softSelectable", "softSelectable", MFnNumericData::kBoolean, 0.0, &retValue);
    numericAttrFn.setStorable(false);
    numericAttrFn.setAffectsAppearance(true);
    retValue = addAttribute(softSelectableAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    //
    // add attribute dependencies
    //
    retValue = attributeAffects(variantKeyAttr, inStageDataCachedAttr);
    retValue = attributeAffects(variantKeyAttr, outStageDataAttr);
    retValue = attributeAffects(primPathAttr, inStageDataCachedAttr);
    return retValue;
}

/* static */
void UsdMayaProxyShape::SetObjectSoftSelectEnabledDelegate(ObjectSoftSelectEnabledDelegate delegate)
{
    _sharedObjectSoftSelectEnabledDelegate = delegate;
}

/* virtual */
bool UsdMayaProxyShape::GetObjectSoftSelectEnabled() const
{
    // If the delegate isn't set, we just assume soft select isn't currently
    // enabled - this will mean that the object is selectable in VP2, by default
    if (!_sharedObjectSoftSelectEnabledDelegate) {
        return false;
    }
    return _sharedObjectSoftSelectEnabledDelegate();
}

SdfLayerRefPtr UsdMayaProxyShape::computeSessionLayer(MDataBlock& dataBlock)
{
    MStatus retValue = MS::kSuccess;

    // get the variantKey
    MDataHandle variantKeyHandle = dataBlock.inputValue(variantKeyAttr, &retValue);
    if (retValue != MS::kSuccess) {
        return nullptr;
    }
    const MString variantKey = variantKeyHandle.asString();

    SdfLayerRefPtr                                   sessionLayer;
    std::vector<std::pair<std::string, std::string>> variantSelections;
    std::string                                      variantKeyString = variantKey.asChar();
    if (!variantKeyString.empty()) {
        variantSelections.push_back(std::make_pair("modelingVariant", variantKeyString));

        // Get the primPath
        const MString primPathMString = dataBlock.inputValue(primPathAttr, &retValue).asString();
        if (retValue != MS::kSuccess) {
            return nullptr;
        }

        std::vector<std::string> primPathEltStrs = TfStringTokenize(primPathMString.asChar(), "/");
        if (!primPathEltStrs.empty()) {
            sessionLayer = UsdUtilsStageCache::GetSessionLayerForVariantSelections(
                TfToken(primPathEltStrs[0]), variantSelections);
        }
    }

    return sessionLayer;
}

/* virtual */
bool UsdMayaProxyShape::isBounded() const
{
    return !_useFastPlayback && TfGetEnvSetting(PIXMAYA_ENABLE_BOUNDING_BOX_MODE)
        && ParentClass::isBounded();
}

/* virtual */
MBoundingBox UsdMayaProxyShape::boundingBox() const
{
    if (_useFastPlayback) {
        return UsdMayaUtil::GetInfiniteBoundingBox();
    }

    return ParentClass::boundingBox();
}

/* virtual */
bool UsdMayaProxyShape::setInternalValueInContext(
    const MPlug&       plug,
    const MDataHandle& dataHandle,
    MDGContext&        ctx)
{
    if (plug == fastPlaybackAttr) {
        _useFastPlayback = dataHandle.asBool();
        return true;
    }

    return MPxSurfaceShape::setInternalValueInContext(plug, dataHandle, ctx);
}

/* virtual */
bool UsdMayaProxyShape::getInternalValueInContext(
    const MPlug& plug,
    MDataHandle& dataHandle,
    MDGContext&  ctx)
{
    if (plug == fastPlaybackAttr) {
        dataHandle.set(_useFastPlayback);
        return true;
    }

    return MPxSurfaceShape::getInternalValueInContext(plug, dataHandle, ctx);
}

UsdMayaProxyShape::UsdMayaProxyShape()
    : MayaUsdProxyShapeBase(
        /* enableUfeSelection = */ false,
        /* useLoadRulesHandling = */ false)
    , _useFastPlayback(false)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaProxyShape>();
}

/* virtual */
UsdMayaProxyShape::~UsdMayaProxyShape()
{
    //
    // empty
    //
}

bool UsdMayaProxyShape::canBeSoftSelected() const
{
    UsdMayaProxyShape* nonConstThis = const_cast<UsdMayaProxyShape*>(this);
    MDataBlock         dataBlock = nonConstThis->forceCache();
    MStatus            status;
    MDataHandle        softSelHandle = dataBlock.inputValue(softSelectableAttr, &status);
    if (!status) {
        return false;
    }

    return softSelHandle.asBool();
}

void UsdMayaProxyShape::postConstructor()
{
    ParentClass::postConstructor();

    if (!MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
        // This shape uses Hydra for imaging, so make sure that the
        // pxrHdImagingShape is setup.
        PxrMayaHdImagingShape::GetOrCreateInstance();
    }
}

/// Delegate for returning whether object soft-select mode is currently on
/// Technically, we could make ProxyShape track this itself, but then that would
/// be making two callbacks to track the same thing... so we use BatchRenderer
/// implementation
bool UsdMayaGL_ObjectSoftSelectEnabled()
{
    return UsdMayaGLBatchRenderer::GetInstance().GetObjectSoftSelectEnabled();
}

TF_REGISTRY_FUNCTION(UsdMayaProxyShape)
{
    UsdMayaProxyShape::SetObjectSoftSelectEnabledDelegate(UsdMayaGL_ObjectSoftSelectEnabled);
}

PXR_NAMESPACE_CLOSE_SCOPE
