//
// Copyright 2018 Pixar
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
#include "stageNode.h"

#include <mayaUsd/nodes/stageData.h>
#include <mayaUsd/utils/stageCache.h>

#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/stageCacheContext.h>

#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnData.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdMayaStageNodeTokens, PXRUSDMAYA_STAGE_NODE_TOKENS);

const MTypeId UsdMayaStageNode::typeId(0x00126400);
const MString UsdMayaStageNode::typeName(UsdMayaStageNodeTokens->MayaTypeName.GetText());

// Attributes
MObject UsdMayaStageNode::filePathAttr;
MObject UsdMayaStageNode::outUsdStageAttr;

/* static */
void* UsdMayaStageNode::creator() { return new UsdMayaStageNode(); }

/* static */
MStatus UsdMayaStageNode::initialize()
{
    MStatus status;

    MFnTypedAttribute typedAttrFn;

    MFnStringData stringDataFn;
    const MObject defaultStringDataObj = stringDataFn.create("");

    filePathAttr
        = typedAttrFn.create("filePath", "fp", MFnData::kString, defaultStringDataObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = typedAttrFn.setUsedAsFilename(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(filePathAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    outUsdStageAttr = typedAttrFn.create(
        "outUsdStage", "os", MayaUsdStageData::mayaTypeId, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = typedAttrFn.setWritable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = typedAttrFn.setStorable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(outUsdStageAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = attributeAffects(filePathAttr, outUsdStageAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return status;
}

/* virtual */
MStatus UsdMayaStageNode::compute(const MPlug& plug, MDataBlock& dataBlock)
{
    MStatus status = MS::kUnknownParameter;

    if (plug == outUsdStageAttr) {
        const MDataHandle filePathHandle = dataBlock.inputValue(filePathAttr, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        const std::string usdFile = TfStringTrim(filePathHandle.asString().asChar());

        UsdStageRefPtr usdStage;

        if (SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(usdFile)) {
            const bool           loadAll = true;
            UsdStageCacheContext ctx(UsdMayaStageCache::Get(loadAll));
            usdStage = UsdStage::Open(rootLayer, ArGetResolver().GetCurrentContext());

            usdStage->SetEditTarget(usdStage->GetRootLayer());
        }

        SdfPath primPath;
        if (usdStage) {
            primPath = usdStage->GetPseudoRoot().GetPath();
        }

        // Create the output stage data object.
        MFnPluginData pluginDataFn;
        MObject       stageDataObj = pluginDataFn.create(MayaUsdStageData::mayaTypeId, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MayaUsdStageData* stageData
            = reinterpret_cast<MayaUsdStageData*>(pluginDataFn.data(&status));
        CHECK_MSTATUS_AND_RETURN_IT(status);

        stageData->stage = usdStage;
        stageData->primPath = primPath;

        MDataHandle outUsdStageHandle = dataBlock.outputValue(outUsdStageAttr, &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        outUsdStageHandle.set(stageData);
        outUsdStageHandle.setClean();

        status = MS::kSuccess;
    }

    return status;
}

UsdMayaStageNode::UsdMayaStageNode()
    : MPxNode()
{
}

/* virtual */
UsdMayaStageNode::~UsdMayaStageNode() { }

PXR_NAMESPACE_CLOSE_SCOPE
