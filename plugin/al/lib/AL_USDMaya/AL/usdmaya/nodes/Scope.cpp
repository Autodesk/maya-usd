//
// Copyright 2017 Animal Logic
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
#include "AL/usdmaya/nodes/Scope.h"

#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/TypeIDs.h"
#include "AL/usdmaya/nodes/ProxyShape.h"

#include <mayaUsd/nodes/stageData.h>

#include <maya/MBoundingBox.h>
#include <maya/MGlobal.h>
#include <maya/MTime.h>

namespace AL {
namespace usdmaya {
namespace nodes {

AL_MAYA_DEFINE_NODE(Scope, AL_USDMAYA_SCOPE, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MObject Scope::m_primPath = MObject::kNullObj;
MObject Scope::m_inStageData = MObject::kNullObj;

// I may need to worry about transforms being deleted accidentally.
// I'm not sure how best to do this
//----------------------------------------------------------------------------------------------------------------------
void Scope::postConstructor()
{
    transform()->setMObject(thisMObject());
    MPlug(thisMObject(), MPxTransform::translate).setLocked(true);
    MPlug(thisMObject(), MPxTransform::rotate).setLocked(true);
    MPlug(thisMObject(), MPxTransform::scale).setLocked(true);
    MPlug(thisMObject(), MPxTransform::transMinusRotatePivot).setLocked(true);
    MPlug(thisMObject(), MPxTransform::rotateAxis).setLocked(true);
    MPlug(thisMObject(), MPxTransform::scalePivotTranslate).setLocked(true);
    MPlug(thisMObject(), MPxTransform::scalePivot).setLocked(true);
    MPlug(thisMObject(), MPxTransform::rotatePivotTranslate).setLocked(true);
    MPlug(thisMObject(), MPxTransform::rotatePivot).setLocked(true);
    MPlug(thisMObject(), MPxTransform::shearXY).setLocked(true);
    MPlug(thisMObject(), MPxTransform::shearXZ).setLocked(true);
    MPlug(thisMObject(), MPxTransform::shearYZ).setLocked(true);
}

//----------------------------------------------------------------------------------------------------------------------
Scope::Scope() { }

//----------------------------------------------------------------------------------------------------------------------
Scope::~Scope() { }

//----------------------------------------------------------------------------------------------------------------------
MPxTransformationMatrix* Scope::createTransformationMatrix()
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("Scope::createTransformationMatrix\n");
    return new BasicTransformationMatrix;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Scope::initialise()
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("Scope::initialise\n");
    const char* const errorString = "Scope::initialise";
    try {
        setNodeType(kTypeName);

        addFrame("USD Prim Information");
        m_primPath = addStringAttr(
            "primPath",
            "pp",
            kReadable | kWritable | kStorable | kConnectable | kAffectsWorldSpace,
            true);
        m_inStageData = addDataAttr(
            "inStageData",
            "isd",
            MayaUsdStageData::mayaTypeId,
            kWritable | kStorable | kConnectable | kHidden | kAffectsWorldSpace);

        mustCallValidateAndSet(m_primPath);
        mustCallValidateAndSet(m_inStageData);

        for (auto& inAttr : { m_primPath, m_inStageData }) {
            AL_MAYA_CHECK_ERROR(attributeAffects(inAttr, translate), errorString);
            AL_MAYA_CHECK_ERROR(attributeAffects(inAttr, rotate), errorString);
            AL_MAYA_CHECK_ERROR(attributeAffects(inAttr, rotateOrder), errorString);
            AL_MAYA_CHECK_ERROR(attributeAffects(inAttr, scale), errorString);
            AL_MAYA_CHECK_ERROR(attributeAffects(inAttr, shear), errorString);
            AL_MAYA_CHECK_ERROR(attributeAffects(inAttr, rotatePivot), errorString);
            AL_MAYA_CHECK_ERROR(attributeAffects(inAttr, rotatePivotTranslate), errorString);
            AL_MAYA_CHECK_ERROR(attributeAffects(inAttr, scalePivot), errorString);
            AL_MAYA_CHECK_ERROR(attributeAffects(inAttr, scalePivotTranslate), errorString);
            // Maya 2018 (checked 2018.2 and 2018.3) has a bug where, if any loaded plugin has an
            // MPxTransform subclass that has ANY attribute that connected to rotateAxis, it will
            // cause the rotateAxis to evaluate INCORRECTLY, even on the BASE transform class! See
            // this gist for full reproduction details:
            //   https://gist.github.com/elrond79/f9ddb277da3eab2948d27ddb1f84aba0
#if MAYA_API_VERSION >= 20180600
            AL_MAYA_CHECK_ERROR(attributeAffects(inAttr, rotateAxis), errorString);
#endif
            AL_MAYA_CHECK_ERROR(attributeAffects(inAttr, matrix), errorString);
            AL_MAYA_CHECK_ERROR(attributeAffects(inAttr, worldMatrix), errorString);
            AL_MAYA_CHECK_ERROR(attributeAffects(inAttr, inverseMatrix), errorString);
            AL_MAYA_CHECK_ERROR(attributeAffects(inAttr, worldInverseMatrix), errorString);
        }
    } catch (const MStatus& status) {
        return status;
    }

    addBaseTemplate("AEtransformMain");
    addBaseTemplate("AEtransformNoScroll");
    addBaseTemplate("AEtransformSkinCluster");
    generateAETemplate();

    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Scope::compute(const MPlug& plug, MDataBlock& dataBlock)
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("Scope::compute %s\n", plug.name().asChar());
    if (plug == m_inStageData) {
        // This should only be computed if there's no connection, so set it to an empty stage
        // create new stage data
        MObject           data;
        MayaUsdStageData* usdStageData
            = createData<MayaUsdStageData>(MayaUsdStageData::mayaTypeId, data);
        if (!usdStageData) {
            return MS::kFailure;
        }

        // set the cached output value, and flush
        MStatus status = outputDataValue(dataBlock, m_inStageData, usdStageData);
        if (!status) {
            return MS::kFailure;
        }
        return status;
    }

    return MPxTransform::compute(plug, dataBlock);
}

//----------------------------------------------------------------------------------------------------------------------
MBoundingBox Scope::boundingBox() const
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("Scope::boundingBox\n");

    MFnDagNode                      fn;
    MPlug                           sourcePlug = inStageDataPlug().source();
    MFnDagNode                      proxyShape(sourcePlug.node());
    AL::usdmaya::nodes::ProxyShape* foundShape
        = (AL::usdmaya::nodes::ProxyShape*)proxyShape.userNode();

    UsdTimeCode usdTime; // Use default time if can't find a connected proxy shape;
    if (foundShape) {
        auto  outTimePlug = foundShape->outTimePlug();
        MTime outTime = outTimePlug.asMTime();
        usdTime = UsdTimeCode(outTime.as(MTime::uiUnit()));
    }

    // Compute Maya bounding box first. Some nodes can contain both Maya and USD boundable
    // descendants.
    MBoundingBox bbox = MPxTransform::boundingBox();

    UsdPrim prim = transform()->prim();
    if (prim) {
        // Get purpose draw states
        bool drawRenderPurpose = false;
        bool drawProxyPurpose = false;
        bool drawGuidePurpose = false;
        CHECK_MSTATUS(
            MPlug(getProxyShape(), ProxyShape::drawRenderPurposeAttr).getValue(drawRenderPurpose));
        CHECK_MSTATUS(
            MPlug(getProxyShape(), ProxyShape::drawProxyPurposeAttr).getValue(drawProxyPurpose));
        CHECK_MSTATUS(
            MPlug(getProxyShape(), ProxyShape::drawGuidePurposeAttr).getValue(drawGuidePurpose));
        const TfToken purpose1 = UsdGeomTokens->default_;
        const TfToken purpose2 = drawRenderPurpose ? UsdGeomTokens->render : TfToken();
        const TfToken purpose3 = drawProxyPurpose ? UsdGeomTokens->proxy : TfToken();
        const TfToken purpose4 = drawGuidePurpose ? UsdGeomTokens->guide : TfToken();

        // Compute bounding box
        UsdGeomImageable imageable(prim);
        const GfBBox3d   box
            = imageable.ComputeLocalBound(usdTime, purpose1, purpose2, purpose3, purpose4);
        const GfRange3d range = box.GetRange();
        const GfVec3d   usdMin = range.IsEmpty() ? GfVec3d(0.0, 0.0, 0.0) : range.GetMin();
        const GfVec3d   usdMax = range.IsEmpty() ? GfVec3d(0.0, 0.0, 0.0) : range.GetMax();
        bbox.expand(MPoint(usdMin[0], usdMin[1], usdMin[2]));
        bbox.expand(MPoint(usdMax[0], usdMax[1], usdMax[2]));
        MMatrix       mayaMx;
        const double* matrixArray = box.GetMatrix().GetArray();
        std::copy(matrixArray, matrixArray + 16, mayaMx[0]);
        bbox.transformUsing(mayaMx);
    }

    return bbox;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Scope::connectionMade(const MPlug& plug, const MPlug& otherPlug, bool asSrc)
{
    if (!asSrc && plug == m_inStageData) {
        MFnDependencyNode otherNode(otherPlug.node());
        if (otherNode.typeId() == ProxyShape::kTypeId) {
            proxyShapeHandle = otherPlug.node();
        }
    }
    return MPxTransform::connectionMade(plug, otherPlug, asSrc);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Scope::connectionBroken(const MPlug& plug, const MPlug& otherPlug, bool asSrc)
{
    if (!asSrc && plug == m_inStageData) {
        MFnDependencyNode otherNode(otherPlug.node());
        if (otherNode.typeId() == ProxyShape::kTypeId) {
            proxyShapeHandle = MObject();
        }
    }
    return MPxTransform::connectionBroken(plug, otherPlug, asSrc);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
Scope::validateAndSetValue(const MPlug& plug, const MDataHandle& handle, const MDGContext& context)
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("Scope::validateAndSetValue %s\n", plug.name().asChar());

    if (plug.isNull())
        return MS::kFailure;

    if (plug.isLocked())
        return MS::kSuccess;

    if (plug.isChild() && plug.parent().isLocked())
        return MS::kSuccess;

    if (plug == m_inStageData) {
        MDataBlock        dataBlock = forceCache(*(MDGContext*)&context);
        MayaUsdStageData* data = inputDataValue<MayaUsdStageData>(dataBlock, m_inStageData);
        if (data && data->stage) {
            MString path = inputStringValue(dataBlock, m_primPath);
            SdfPath primPath;
            UsdPrim usdPrim;
            if (path.length()) {
                primPath = SdfPath(path.asChar());
                usdPrim = data->stage->GetPrimAtPath(primPath);
            }
            transform()->setPrim(usdPrim, this);
        } else {
            transform()->setPrim(UsdPrim(), this);
        }
        return MS::kSuccess;
    } else if (plug == m_primPath) {
        MDataBlock dataBlock = forceCache(*(MDGContext*)&context);
        MString    path = handle.asString();
        outputStringValue(dataBlock, m_primPath, path);

        MayaUsdStageData* data = inputDataValue<MayaUsdStageData>(dataBlock, m_inStageData);
        if (data && data->stage) {
            SdfPath primPath;
            UsdPrim usdPrim;
            if (path.length()) {
                primPath = SdfPath(path.asChar());
                usdPrim = UsdPrim(data->stage->GetPrimAtPath(primPath));
            }
            transform()->setPrim(usdPrim, this);
        } else {
            if (path.length() > 0) {
                TF_DEBUG(ALUSDMAYA_EVALUATION)
                    .Msg(
                        "Could not set '%s' to '%s' - could not retrieve stage\n",
                        plug.name().asChar(),
                        path.asChar());
            }
            transform()->setPrim(UsdPrim(), this);
        }
        return MS::kSuccess;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
