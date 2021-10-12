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
#include "AL/usdmaya/nodes/Transform.h"

#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/TypeIDs.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"

#include <mayaUsd/nodes/stageData.h>

#include <pxr/usd/usdGeom/scope.h>

#include <maya/MBoundingBox.h>
#include <maya/MGlobal.h>
#include <maya/MProfiler.h>
#include <maya/MTime.h>

namespace {
const int _transformProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
    "Transform",
    "Transform"
#else
    "Transform"
#endif
);
} // namespace

namespace {
// Simple RAII class to ensure boolean gets set to false when done.
struct TempBoolLock
{
    TempBoolLock(bool& theVal)
        : theRef(theVal)
    {
        theRef = true;
    }

    ~TempBoolLock() { theRef = false; }

    bool& theRef;
};
} // namespace

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_NODE(Transform, AL_USDMAYA_TRANSFORM, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MObject Transform::m_time = MObject::kNullObj;
MObject Transform::m_timeOffset = MObject::kNullObj;
MObject Transform::m_timeScalar = MObject::kNullObj;
MObject Transform::m_outTime = MObject::kNullObj;
MObject Transform::m_localTranslateOffset = MObject::kNullObj;
MObject Transform::m_pushToPrim = MObject::kNullObj;
MObject Transform::m_readAnimatedValues = MObject::kNullObj;

// I may need to worry about transforms being deleted accidentally.
// I'm not sure how best to do this
//----------------------------------------------------------------------------------------------------------------------
void Transform::postConstructor()
{
    transform()->setMObject(thisMObject());
    getTransMatrix()->enablePushToPrim(pushToPrimPlug().asBool());
}

//----------------------------------------------------------------------------------------------------------------------
Transform::Transform() { }

//----------------------------------------------------------------------------------------------------------------------
Transform::~Transform() { }

//----------------------------------------------------------------------------------------------------------------------
MPxTransformationMatrix* Transform::createTransformationMatrix()
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("Transform::createTransformationMatrix\n");
    return new TransformationMatrix;
}

//----------------------------------------------------------------------------------------------------------------------
bool Transform::setInternalValue(const MPlug& plug, const MDataHandle& dataHandle)
{
    if (plug == m_pushToPrim) {
        getTransMatrix()->enablePushToPrim(dataHandle.asBool());
    }
    return MPxTransform::setInternalValue(plug, dataHandle);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Transform::initialise()
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("Transform::initialise\n");
    const char* const errorString = "Transform::initialise";
    try {
        setNodeType(kTypeName);
        inheritAttributesFrom("AL_usdmaya_Scope");

        addFrame("USD Prim Information");
        addFrameAttr(
            "primPath", kReadable | kWritable | kStorable | kConnectable | kAffectsWorldSpace);
        addFrameAttr(
            "inStageData", kWritable | kStorable | kConnectable | kHidden | kAffectsWorldSpace);

        addFrame("USD Timing Information");
        m_time = addTimeAttr(
            "time",
            "tm",
            MTime(0.0),
            kKeyable | kConnectable | kReadable | kWritable | kStorable | kAffectsWorldSpace);
        m_timeOffset = addTimeAttr(
            "timeOffset",
            "tmo",
            MTime(0.0),
            kKeyable | kConnectable | kReadable | kWritable | kStorable | kAffectsWorldSpace);
        m_timeScalar = addDoubleAttr(
            "timeScalar",
            "tms",
            1.0,
            kKeyable | kConnectable | kReadable | kWritable | kStorable | kAffectsWorldSpace);
        m_outTime = addTimeAttr(
            "outTime", "otm", MTime(0.0), kConnectable | kReadable | kAffectsWorldSpace);

        addFrame("USD Experimental Features");
        m_localTranslateOffset = addVectorAttr(
            "localTranslateOffset",
            "lto",
            MVector(0, 0, 0),
            kReadable | kWritable | kStorable | kConnectable | kAffectsWorldSpace);
        m_pushToPrim = addBoolAttr(
            "pushToPrim", "ptp", false, kReadable | kWritable | kStorable | kInternal);
        m_readAnimatedValues = addBoolAttr(
            "readAnimatedValues",
            "rav",
            true,
            kReadable | kWritable | kStorable | kAffectsWorldSpace);

        mustCallValidateAndSet(m_time);
        mustCallValidateAndSet(m_timeOffset);
        mustCallValidateAndSet(m_timeScalar);
        mustCallValidateAndSet(m_localTranslateOffset);
        mustCallValidateAndSet(m_pushToPrim);
        mustCallValidateAndSet(m_primPath);
        mustCallValidateAndSet(m_readAnimatedValues);
        mustCallValidateAndSet(m_inStageData);

        AL_MAYA_CHECK_ERROR(attributeAffects(m_time, m_outTime), errorString);
        AL_MAYA_CHECK_ERROR(attributeAffects(m_timeOffset, m_outTime), errorString);
        AL_MAYA_CHECK_ERROR(attributeAffects(m_timeScalar, m_outTime), errorString);

        for (auto& inAttr : { m_time, m_timeOffset, m_timeScalar, m_readAnimatedValues }) {
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
MStatus Transform::compute(const MPlug& plug, MDataBlock& dataBlock)
{
    MProfilingScope profilerScope(
        _transformProfilerCategory, MProfiler::kColorE_L3, "Compute plug");

    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("Transform::compute %s\n", plug.name().asChar());
    if (plug == m_time || plug == m_timeOffset || plug == m_timeScalar || plug == m_outTime) {
        updateTransform(dataBlock);
        return MS::kSuccess;
    } else if (plug == m_inStageData) {
        // This should only be computed if there's no connection, so set it to an empty stage
        // create new stage data
        MObject data;
        auto*   usdStageData = createData<MayaUsdStageData>(MayaUsdStageData::mayaTypeId, data);
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

    // If the time is dirty, we need to make sure we calculate / update that BEFORE calculating our
    // transform. Otherwise, we may read info for the wrong time from usd - and even worse, we may
    // push that out-of-date info back to usd! So, we always trigger a compute of outTime to make
    // sure it's up to date...

    if (!dataBlock.isClean(m_outTime) && !plug.isNull()) {
        // instead of checking if the attr is one in a giant list of attrs affected by m_time, I
        // just check if it's affected by world space.
        MStatus      status;
        MFnAttribute plugAttr(plug.attribute(), &status);
        AL_MAYA_CHECK_ERROR(status, "error retrieving attribute");
        if (plugAttr.isAffectsWorldSpace()) {
            // NOTE: initially thought that I could just fetch value of "m_time" with
            // inputTimeValue...but it appears there's a bug where validateAndSetValue is not called
            // if there's an incoming connection to m_time, and we're not in GUI mode. So we just
            // get the outTime, and rely on it's compute; since it's not writable, it's compute
            // should always be called.
            inputTimeValue(dataBlock, m_outTime);
        }
    }
    return Scope::compute(plug, dataBlock);
}

//----------------------------------------------------------------------------------------------------------------------
void Transform::updateTransform(MDataBlock& dataBlock)
{
    MProfilingScope profilerScope(
        _transformProfilerCategory, MProfiler::kColorE_L3, "Update transform");

    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("Transform::updateTransform\n");

    // It's possible that the calls to inputTimeValue below will ALSO trigger a call to
    // updateTransform; since there's no need to run this twice, we check for this and
    // abort if we're already in the middle of running.
    // Note that we don't bother using, say, std::atomic_flag, because multithreaded
    // collisions here seem unlikely, and the worst case is just that this function runs twice.
    if (updateTransformInProgress)
        return;

    TempBoolLock updateTransformLock(updateTransformInProgress);

    // compute updated time value
    MTime theTime = (inputTimeValue(dataBlock, m_time) - inputTimeValue(dataBlock, m_timeOffset))
        * inputDoubleValue(dataBlock, m_timeScalar);
    outputTimeValue(dataBlock, m_outTime, theTime);

    UsdTimeCode usdTime(theTime.as(MTime::uiUnit()));

    // update the transformation matrix to the values at the specified time
    TransformationMatrix* m = getTransMatrix();
    m->updateToTime(usdTime);

    // if translation animation is present, update the translate attribute (or just flag it as clean
    // if no animation exists)
    if (m->hasAnimatedTranslation()) {
        outputVectorValue(dataBlock, MPxTransform::translate, m->translation(MSpace::kTransform));
    } else {
        dataBlock.setClean(MPxTransform::translate);
    }

    // if rotation animation is present, update the rotate attribute (or just flag it as clean if no
    // animation exists)
    if (m->hasAnimatedRotation()) {
        outputEulerValue(dataBlock, MPxTransform::rotate, m->eulerRotation(MSpace::kTransform));
    } else {
        dataBlock.setClean(MPxTransform::rotate);
    }

    // if scale animation is present, update the scale attribute (or just flag it as clean if no
    // animation exists)
    if (m->hasAnimatedScale()) {
        outputVectorValue(dataBlock, MPxTransform::scale, m->scale(MSpace::kTransform));
    } else {
        dataBlock.setClean(MPxTransform::scale);
    }

    // if translation animation is present, update the translate attribute (or just flag it as clean
    // if no animation exists)
    if (m->hasAnimatedMatrix()) {
        outputVectorValue(dataBlock, MPxTransform::scale, m->scale(MSpace::kTransform));
        outputEulerValue(dataBlock, MPxTransform::rotate, m->eulerRotation(MSpace::kTransform));
        outputVectorValue(dataBlock, MPxTransform::translate, m->translation(MSpace::kTransform));
    } else {
        dataBlock.setClean(MPxTransform::scale);
        dataBlock.setClean(MPxTransform::rotate);
        dataBlock.setClean(MPxTransform::translate);
    }
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Transform::connectionMade(const MPlug& plug, const MPlug& otherPlug, bool asSrc)
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
MStatus Transform::connectionBroken(const MPlug& plug, const MPlug& otherPlug, bool asSrc)
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
// If any value changes, that affects the resulting transform (the non-animated
// m_localTranslateOffset value is a good example), then it only needs to be set here. If an
// attribute drives one of the TRS components (e.g. 'time' modifies the translate / rotate / scale
// values), then it needs to be set here, and it also needs to be handled in the compute method.
// That doesn't feel quite right to me, to that is how it appears to work? (If you have any better
// ideas, I'm all ears!).
//
MStatus Transform::validateAndSetValue(
    const MPlug&       plug,
    const MDataHandle& handle,
    const MDGContext&  context)
{
    MProfilingScope profilerScope(
        _transformProfilerCategory, MProfiler::kColorE_L3, "Validate and set value");

    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("Transform::validateAndSetValue %s\n", plug.name().asChar());

    if (plug.isNull())
        return MS::kFailure;

    if (plug.isLocked())
        return MS::kSuccess;

    if (plug.isChild() && plug.parent().isLocked())
        return MS::kSuccess;

    // If the time values are changed, store the new values, and then update the transform
    if (plug == m_time || plug == m_timeOffset || plug == m_timeScalar) {
        MDataBlock dataBlock = forceCache(*(MDGContext*)&context);
        if (plug == m_time) {
            outputTimeValue(dataBlock, m_time, handle.asTime());
        } else if (plug == m_timeOffset) {
            outputTimeValue(dataBlock, m_timeOffset, handle.asTime());
        } else if (plug == m_timeScalar) {
            outputDoubleValue(dataBlock, m_timeScalar, handle.asDouble());
        }

        updateTransform(dataBlock);
        return MS::kSuccess;
    } else
        // The local translate offset doesn't drive the TRS, so set the value here, and the
        // transformation update should be handled by the MPxTransform without any additional
        // faffing around in compute.
        if (plug == m_localTranslateOffset || plug.parent() == m_localTranslateOffset) {
        MPlug   parentPlug = plug.parent();
        MVector offset;
        // getting access to the X/Y/Z components of the translation offset is a bit of a faff
        if (plug == m_localTranslateOffset) {
            offset = handle.asVector();
        } else if (parentPlug.child(0) == plug) {
            offset.x = handle.asDouble();
        } else if (parentPlug.child(1) == plug) {
            offset.y = handle.asDouble();
        } else if (parentPlug.child(2) == plug) {
            offset.z = handle.asDouble();
        }

        MDataBlock dataBlock = forceCache(*(MDGContext*)&context);
        outputVectorValue(dataBlock, m_localTranslateOffset, offset);
        getTransMatrix()->setLocalTranslationOffset(offset);
        return MS::kSuccess;
    } else if (plug == m_pushToPrim) {
        MDataBlock dataBlock = forceCache(*(MDGContext*)&context);
        getTransMatrix()->enablePushToPrim(handle.asBool());
        outputBoolValue(dataBlock, m_pushToPrim, handle.asBool());
        return MS::kSuccess;
    } else if (plug == m_readAnimatedValues) {
        MDataBlock dataBlock = forceCache(*(MDGContext*)&context);
        getTransMatrix()->enableReadAnimatedValues(handle.asBool());
        outputBoolValue(dataBlock, m_readAnimatedValues, handle.asBool());
        updateTransform(dataBlock);
        return MS::kSuccess;
    } else if (plug == m_inStageData) {
        MDataBlock dataBlock = forceCache(*(MDGContext*)&context);
        auto*      data = inputDataValue<MayaUsdStageData>(dataBlock, m_inStageData);
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

        auto* data = inputDataValue<MayaUsdStageData>(dataBlock, m_inStageData);
        if (data && data->stage) {
            SdfPath primPath;
            UsdPrim usdPrim;
            if (path.length()) {
                primPath = SdfPath(path.asChar());
                usdPrim = UsdPrim(data->stage->GetPrimAtPath(primPath));
            }
            transform()->setPrim(usdPrim, this);
            if (usdPrim)
                updateTransform(dataBlock);
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

    return MPxTransform::validateAndSetValue(plug, handle, context);
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
