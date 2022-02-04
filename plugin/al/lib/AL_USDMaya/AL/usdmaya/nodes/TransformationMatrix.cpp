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

#include "AL/usdmaya/nodes/TransformationMatrix.h"

#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/TypeIDs.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/utils/AttributeType.h"
#include "AL/usdmaya/utils/Utils.h"

#include <maya/MFileIO.h>
#include <maya/MFnTransform.h>
#include <maya/MProfiler.h>
#include <maya/MViewport2Renderer.h>

namespace {
const int _transformationMatrixProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
    "TransformationMatrix",
    "TransformationMatrix"
#else
    "TransformationMatrix"
#endif
);
} // namespace

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {
namespace {
using AL::usdmaya::utils::UsdDataType;

//----------------------------------------------------------------------------------------------------------------------
UsdTimeCode getTimeCodeForOp(const UsdGeomXformOp& op, const UsdTimeCode& time)
{
    // Return the current time code if xform op has samples, return default time code otherwise
    // This function could be made as an option in proxy node
    if (op.GetNumTimeSamples()) {
        return time;
    }
    return UsdTimeCode::Default();
}

//----------------------------------------------------------------------------------------------------------------------
bool hasEmptyDefaultValue(const UsdGeomXformOp& op, UsdTimeCode time)
{
    SdfPropertySpecHandleVector propSpecs = op.GetAttr().GetPropertyStack(time);
    for (auto propSpec : propSpecs) {
        auto def = propSpec->GetDefaultValue();
        if (def.IsEmpty()) {
            return true;
        }
    }
    return false;
}
} // namespace

//----------------------------------------------------------------------------------------------------------------------
const MTypeId TransformationMatrix::kTypeId(AL_USDMAYA_TRANSFORMATION_MATRIX);

//----------------------------------------------------------------------------------------------------------------------
MPxTransformationMatrix* TransformationMatrix::creator() { return new TransformationMatrix; }

//----------------------------------------------------------------------------------------------------------------------
TransformationMatrix::TransformationMatrix()
    : BasicTransformationMatrix()
    , m_xform()
    , m_time(UsdTimeCode::Default())
    , m_scaleTweak(0, 0, 0)
    , m_rotationTweak(0, 0, 0)
    , m_translationTweak(0, 0, 0)
    , m_shearTweak(0, 0, 0)
    , m_scalePivotTweak(0, 0, 0)
    , m_scalePivotTranslationTweak(0, 0, 0)
    , m_rotatePivotTweak(0, 0, 0)
    , m_rotatePivotTranslationTweak(0, 0, 0)
    , m_rotateOrientationTweak(0, 0, 0, 1.0)
    , m_scaleFromUsd(1.0, 1.0, 1.0)
    , m_rotationFromUsd(0, 0, 0)
    , m_translationFromUsd(0.0, 0.0, 0.0)
    , m_shearFromUsd(0, 0, 0)
    , m_scalePivotFromUsd(0, 0, 0)
    , m_scalePivotTranslationFromUsd(0, 0, 0)
    , m_rotatePivotFromUsd(0, 0, 0)
    , m_rotatePivotTranslationFromUsd(0, 0, 0)
    , m_rotateOrientationFromUsd(0, 0, 0, 1.0)
    , m_localTranslateOffset(0, 0, 0)
    , m_flags(0)
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::TransformationMatrix\n");
}

//----------------------------------------------------------------------------------------------------------------------
TransformationMatrix::TransformationMatrix(const UsdPrim& prim)
    : BasicTransformationMatrix(prim)
    , m_xform(prim)
    , m_time(UsdTimeCode::Default())
    , m_scaleTweak(0, 0, 0)
    , m_rotationTweak(0, 0, 0)
    , m_translationTweak(0, 0, 0)
    , m_shearTweak(0, 0, 0)
    , m_scalePivotTweak(0, 0, 0)
    , m_scalePivotTranslationTweak(0, 0, 0)
    , m_rotatePivotTweak(0, 0, 0)
    , m_rotatePivotTranslationTweak(0, 0, 0)
    , m_rotateOrientationTweak(0, 0, 0, 1.0)
    , m_scaleFromUsd(1.0, 1.0, 1.0)
    , m_rotationFromUsd(0, 0, 0)
    , m_translationFromUsd(0, 0, 0)
    , m_shearFromUsd(0, 0, 0)
    , m_scalePivotFromUsd(0, 0, 0)
    , m_scalePivotTranslationFromUsd(0, 0, 0)
    , m_rotatePivotFromUsd(0, 0, 0)
    , m_rotatePivotTranslationFromUsd(0, 0, 0)
    , m_rotateOrientationFromUsd(0, 0, 0, 1.0)
    , m_localTranslateOffset(0, 0, 0)
    , m_flags(0)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::TransformationMatrix\n");
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::setPrim(const UsdPrim& prim, Scope* transformNode)
{
    MProfilingScope profilerScope(
        _transformationMatrixProfilerCategory, MProfiler::kColorE_L3, "Set prim");

    m_enableUsdWriteback = false;
    if (prim.IsValid()) {
        TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
            .Msg("TransformationMatrix::setPrim %s\n", prim.GetName().GetText());
        m_prim = prim;
        UsdGeomXformable xform(prim);
        m_xform = xform;
    } else {
        TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::setPrim null\n");
        m_prim = UsdPrim();
        m_xform = UsdGeomXformable();
    }
    // Most of these flags are calculated based on reading the usd prim; however, a few are driven
    // "externally" (ie, from attributes on the controlling transform node), and should NOT be reset
    // when we're re-initializing
    m_flags &= kPreservationMask;
    m_scaleTweak = MVector(0, 0, 0);
    m_rotationTweak = MEulerRotation(0, 0, 0);
    m_translationTweak = MVector(0, 0, 0);
    m_shearTweak = MVector(0, 0, 0);
    m_scalePivotTweak = MPoint(0, 0, 0);
    m_scalePivotTranslationTweak = MVector(0, 0, 0);
    m_rotatePivotTweak = MPoint(0, 0, 0);
    m_rotatePivotTranslationTweak = MVector(0, 0, 0);
    m_rotateOrientationTweak = MQuaternion(0, 0, 0, 1.0);
    m_localTranslateOffset = MVector(0, 0, 0);

    if (m_prim.IsValid()) {
        m_scaleFromUsd = MVector(1.0, 1.0, 1.0);
        m_rotationFromUsd = MEulerRotation(0, 0, 0);
        m_translationFromUsd = MVector(0, 0, 0);
        m_shearFromUsd = MVector(0, 0, 0);
        m_scalePivotFromUsd = MPoint(0, 0, 0);
        m_scalePivotTranslationFromUsd = MVector(0, 0, 0);
        m_rotatePivotFromUsd = MPoint(0, 0, 0);
        m_rotatePivotTranslationFromUsd = MVector(0, 0, 0);
        m_rotateOrientationFromUsd = MQuaternion(0, 0, 0, 1.0);
        initialiseToPrim(!MFileIO::isReadingFile(), transformNode);
        MPxTransformationMatrix::scaleValue = m_scaleFromUsd;
        MPxTransformationMatrix::rotationValue = m_rotationFromUsd;
        MPxTransformationMatrix::translationValue = m_translationFromUsd;
        MPxTransformationMatrix::shearValue = m_shearFromUsd;
        MPxTransformationMatrix::scalePivotValue = m_scalePivotFromUsd;
        MPxTransformationMatrix::scalePivotTranslationValue = m_scalePivotTranslationFromUsd;
        MPxTransformationMatrix::rotatePivotValue = m_rotatePivotFromUsd;
        MPxTransformationMatrix::rotatePivotTranslationValue = m_rotatePivotTranslationFromUsd;
        MPxTransformationMatrix::rotateOrientationValue = m_rotateOrientationFromUsd;
    }
    m_enableUsdWriteback = true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::readVector(
    MVector&              result,
    const UsdGeomXformOp& op,
    UsdTimeCode           timeCode)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::readVector\n");
    const SdfValueTypeName vtn = op.GetTypeName();
    UsdDataType            attr_type = AL::usdmaya::utils::getAttributeType(vtn);
    switch (attr_type) {
    case UsdDataType::kVec3d: {
        GfVec3d    value;
        const bool retValue = op.GetAs<GfVec3d>(&value, timeCode);
        if (!retValue) {
            return false;
        }
        result.x = value[0];
        result.y = value[1];
        result.z = value[2];
    } break;

    case UsdDataType::kVec3f: {
        GfVec3f    value;
        const bool retValue = op.GetAs<GfVec3f>(&value, timeCode);
        if (!retValue) {
            return false;
        }
        result.x = double(value[0]);
        result.y = double(value[1]);
        result.z = double(value[2]);
    } break;

    case UsdDataType::kVec3h: {
        GfVec3h    value;
        const bool retValue = op.GetAs<GfVec3h>(&value, timeCode);
        if (!retValue) {
            return false;
        }
        result.x = double(value[0]);
        result.y = double(value[1]);
        result.z = double(value[2]);
    } break;

    case UsdDataType::kVec3i: {
        GfVec3i    value;
        const bool retValue = op.GetAs<GfVec3i>(&value, timeCode);
        if (!retValue) {
            return false;
        }
        result.x = double(value[0]);
        result.y = double(value[1]);
        result.z = double(value[2]);
    } break;

    default: return false;
    }

    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg(
            "TransformationMatrix::readVector %f %f %f\n%s\n",
            result.x,
            result.y,
            result.z,
            op.GetOpName().GetText());
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::pushVector(
    const MVector&  result,
    UsdGeomXformOp& op,
    UsdTimeCode     timeCode)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg(
            "TransformationMatrix::pushVector %f %f %f [@%f]\n%s\n",
            result.x,
            result.y,
            result.z,
            timeCode.GetValue(),
            op.GetOpName().GetText());
    auto attr = op.GetAttr();
    if (!attr) {
        return false;
    }

    if (timeCode.IsDefault() && op.GetNumTimeSamples()) {
        if (!hasEmptyDefaultValue(op, timeCode)) {
            return false;
        }
    }

    TfToken typeName;
    attr.GetMetadata(SdfFieldKeys->TypeName, &typeName);
    SdfValueTypeName vtn = SdfSchema::GetInstance().FindType(typeName);
    UsdDataType      attr_type = AL::usdmaya::utils::getAttributeType(vtn);
    switch (attr_type) {
    case UsdDataType::kVec3d: {
        GfVec3d value(result.x, result.y, result.z);
        GfVec3d oldValue;
        op.Get(&oldValue, timeCode);
        if (value != oldValue) {
            op.Set(value, getTimeCodeForOp(op, getTimeCodeForOp(op, timeCode)));
        }
    } break;

    case UsdDataType::kVec3f: {
        GfVec3f value(result.x, result.y, result.z);
        GfVec3f oldValue;
        op.Get(&oldValue, timeCode);
        if (value != oldValue) {
            op.Set(value, getTimeCodeForOp(op, getTimeCodeForOp(op, timeCode)));
        }
    } break;

    case UsdDataType::kVec3h: {
        GfVec3h value(result.x, result.y, result.z);
        GfVec3h oldValue;
        op.Get(&oldValue, timeCode);
        if (value != oldValue) {
            op.Set(value, getTimeCodeForOp(op, getTimeCodeForOp(op, timeCode)));
        }
    } break;

    case UsdDataType::kVec3i: {
        GfVec3i value(result.x, result.y, result.z);
        GfVec3i oldValue;
        op.Get(&oldValue, timeCode);
        if (value != oldValue) {
            op.Set(value, getTimeCodeForOp(op, getTimeCodeForOp(op, timeCode)));
        }
    } break;

    default: return false;
    }

    return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::pushShear(
    const MVector&  result,
    UsdGeomXformOp& op,
    UsdTimeCode     timeCode)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg(
            "TransformationMatrix::pushShear %f %f %f\n%s\n",
            result.x,
            result.y,
            result.z,
            op.GetOpName().GetText());

    if (timeCode.IsDefault() && op.GetNumTimeSamples()) {
        if (!hasEmptyDefaultValue(op, timeCode)) {
            return false;
        }
    }

    const SdfValueTypeName vtn = op.GetTypeName();
    UsdDataType            attr_type = AL::usdmaya::utils::getAttributeType(vtn);
    switch (attr_type) {
    case UsdDataType::kMatrix4d: {
        GfMatrix4d m(
            1.0,
            0.0,
            0.0,
            0.0,
            result.x,
            1.0,
            0.0,
            0.0,
            result.y,
            result.z,
            1.0,
            0.0,
            0.0,
            0.0,
            0.0,
            1.0);
        GfMatrix4d oldValue;
        op.Get(&oldValue, timeCode);
        if (m != oldValue)
            op.Set(m, getTimeCodeForOp(op, timeCode));
    } break;

    default: return false;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::readShear(
    MVector&              result,
    const UsdGeomXformOp& op,
    UsdTimeCode           timeCode)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::readShear\n");
    const SdfValueTypeName vtn = op.GetTypeName();
    UsdDataType            attr_type = AL::usdmaya::utils::getAttributeType(vtn);
    switch (attr_type) {
    case UsdDataType::kMatrix4d: {
        GfMatrix4d value;
        const bool retValue = op.GetAs<GfMatrix4d>(&value, timeCode);
        if (!retValue) {
            return false;
        }
        result.x = value[1][0];
        result.y = value[2][0];
        result.z = value[2][1];
    } break;

    default: return false;
    }
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg(
            "TransformationMatrix::readShear %f %f %f\n%s\n",
            result.x,
            result.y,
            result.z,
            op.GetOpName().GetText());
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::readPoint(MPoint& result, const UsdGeomXformOp& op, UsdTimeCode timeCode)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::readPoint\n");
    const SdfValueTypeName vtn = op.GetTypeName();
    UsdDataType            attr_type = AL::usdmaya::utils::getAttributeType(vtn);
    switch (attr_type) {
    case UsdDataType::kVec3d: {
        GfVec3d    value;
        const bool retValue = op.GetAs<GfVec3d>(&value, timeCode);
        if (!retValue) {
            return false;
        }
        result.x = value[0];
        result.y = value[1];
        result.z = value[2];
    } break;

    case UsdDataType::kVec3f: {
        GfVec3f    value;
        const bool retValue = op.GetAs<GfVec3f>(&value, timeCode);
        if (!retValue) {
            return false;
        }
        result.x = double(value[0]);
        result.y = double(value[1]);
        result.z = double(value[2]);
    } break;

    case UsdDataType::kVec3h: {
        GfVec3h    value;
        const bool retValue = op.GetAs<GfVec3h>(&value, timeCode);
        if (!retValue) {
            return false;
        }
        result.x = double(value[0]);
        result.y = double(value[1]);
        result.z = double(value[2]);
    } break;

    case UsdDataType::kVec3i: {
        GfVec3i    value;
        const bool retValue = op.GetAs<GfVec3i>(&value, timeCode);
        if (!retValue) {
            return false;
        }
        result.x = double(value[0]);
        result.y = double(value[1]);
        result.z = double(value[2]);
    } break;

    default: return false;
    }
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg(
            "TransformationMatrix::readPoint %f %f %f\n%s\n",
            result.x,
            result.y,
            result.z,
            op.GetOpName().GetText());

    return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::readMatrix(
    MMatrix&              result,
    const UsdGeomXformOp& op,
    UsdTimeCode           timeCode)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::readMatrix\n");
    const SdfValueTypeName vtn = op.GetTypeName();
    UsdDataType            attr_type = AL::usdmaya::utils::getAttributeType(vtn);
    switch (attr_type) {
    case UsdDataType::kMatrix4d: {
        GfMatrix4d value;
        const bool retValue = op.GetAs<GfMatrix4d>(&value, timeCode);
        if (!retValue) {
            return false;
        }
        auto vtemp = (const void*)&value;
        auto mtemp = (const MMatrix*)vtemp;
        result = *mtemp;
    } break;

    default: return false;
    }

    return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::pushMatrix(
    const MMatrix&  result,
    UsdGeomXformOp& op,
    UsdTimeCode     timeCode)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushMatrix\n");
    if (timeCode.IsDefault() && op.GetNumTimeSamples()) {
        if (!hasEmptyDefaultValue(op, timeCode)) {
            return false;
        }
    }

    const SdfValueTypeName vtn = op.GetTypeName();
    UsdDataType            attr_type = AL::usdmaya::utils::getAttributeType(vtn);
    switch (attr_type) {
    case UsdDataType::kMatrix4d: {
        const GfMatrix4d& value = *(const GfMatrix4d*)(&result);
        GfMatrix4d        oldValue;
        op.Get(&oldValue, timeCode);
        if (value != oldValue) {
            const bool retValue = op.Set<GfMatrix4d>(value, getTimeCodeForOp(op, timeCode));
            if (!retValue) {
                return false;
            }
        }
    } break;

    default: return false;
    }

    return true;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::setFromMatrix(MObject thisNode, const MMatrix& m)
{
    MProfilingScope profilerScope(
        _transformationMatrixProfilerCategory, MProfiler::kColorE_L3, "Set from matrix");

    double         S[3];
    MEulerRotation R;
    double         T[3];
    utils::matrixToSRT(*(const GfMatrix4d*)&m, S, R, T);
    m_scaleFromUsd.x = S[0];
    m_scaleFromUsd.y = S[1];
    m_scaleFromUsd.z = S[2];
    m_rotationFromUsd.x = R.x;
    m_rotationFromUsd.y = R.y;
    m_rotationFromUsd.z = R.z;
    m_translationFromUsd.x = T[0];
    m_translationFromUsd.y = T[1];
    m_translationFromUsd.z = T[2];
    MPlug(thisNode, MPxTransform::scaleX).setValue(m_scaleFromUsd.x);
    MPlug(thisNode, MPxTransform::scaleY).setValue(m_scaleFromUsd.y);
    MPlug(thisNode, MPxTransform::scaleZ).setValue(m_scaleFromUsd.z);
    MPlug(thisNode, MPxTransform::rotateX).setValue(m_rotationFromUsd.x);
    MPlug(thisNode, MPxTransform::rotateY).setValue(m_rotationFromUsd.y);
    MPlug(thisNode, MPxTransform::rotateZ).setValue(m_rotationFromUsd.z);
    MPlug(thisNode, MPxTransform::translateX).setValue(m_translationFromUsd.x);
    MPlug(thisNode, MPxTransform::translateY).setValue(m_translationFromUsd.y);
    MPlug(thisNode, MPxTransform::translateZ).setValue(m_translationFromUsd.z);
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::pushPoint(const MPoint& result, UsdGeomXformOp& op, UsdTimeCode timeCode)
{
    MProfilingScope profilerScope(
        _transformationMatrixProfilerCategory, MProfiler::kColorE_L3, "Push point");

    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg(
            "TransformationMatrix::pushPoint %f %f %f\n%s\n",
            result.x,
            result.y,
            result.z,
            op.GetOpName().GetText());

    if (timeCode.IsDefault() && op.GetNumTimeSamples()) {
        if (!hasEmptyDefaultValue(op, getTimeCodeForOp(op, timeCode))) {
            return false;
        }
    }

    const SdfValueTypeName vtn = op.GetTypeName();
    UsdDataType            attr_type = AL::usdmaya::utils::getAttributeType(vtn);
    switch (attr_type) {
    case UsdDataType::kVec3d: {
        GfVec3d value(result.x, result.y, result.z);
        GfVec3d oldValue;
        op.Get(&oldValue, timeCode);
        if (value != oldValue)
            op.Set(value, getTimeCodeForOp(op, timeCode));
    } break;

    case UsdDataType::kVec3f: {
        GfVec3f value(result.x, result.y, result.z);
        GfVec3f oldValue;
        op.Get(&oldValue, timeCode);
        if (value != oldValue)
            op.Set(value, getTimeCodeForOp(op, timeCode));
    } break;

    case UsdDataType::kVec3h: {
        GfVec3h value(result.x, result.y, result.z);
        GfVec3h oldValue;
        op.Get(&oldValue, timeCode);
        if (value != oldValue)
            op.Set(value, getTimeCodeForOp(op, timeCode));
    } break;

    case UsdDataType::kVec3i: {
        GfVec3i value(result.x, result.y, result.z);
        GfVec3i oldValue;
        op.Get(&oldValue, timeCode);
        if (value != oldValue)
            op.Set(value, getTimeCodeForOp(op, timeCode));
    } break;

    default: return false;
    }

    return true;
}

//----------------------------------------------------------------------------------------------------------------------
double TransformationMatrix::readDouble(const UsdGeomXformOp& op, UsdTimeCode timeCode)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::readDouble\n");
    double      result = 0;
    UsdDataType attr_type = AL::usdmaya::utils::getAttributeType(op.GetTypeName());
    switch (attr_type) {
    case UsdDataType::kHalf: {
        GfHalf     value;
        const bool retValue = op.Get<GfHalf>(&value, timeCode);
        if (retValue) {
            result = float(value);
        }
    } break;

    case UsdDataType::kFloat: {
        float      value;
        const bool retValue = op.Get<float>(&value, timeCode);
        if (retValue) {
            result = double(value);
        }
    } break;

    case UsdDataType::kDouble: {
        double     value;
        const bool retValue = op.Get<double>(&value, timeCode);
        if (retValue) {
            result = value;
        }
    } break;

    case UsdDataType::kInt: {
        int32_t    value;
        const bool retValue = op.Get<int32_t>(&value, timeCode);
        if (retValue) {
            result = double(value);
        }
    } break;

    default: break;
    }
    TF_DEBUG(ALUSDMAYA_EVALUATION)
        .Msg("TransformationMatrix::readDouble %f\n%s\n", result, op.GetOpName().GetText());
    return result;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushDouble(const double value, UsdGeomXformOp& op, UsdTimeCode timeCode)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::pushDouble %f\n%s\n", value, op.GetOpName().GetText());

    if (timeCode.IsDefault() && op.GetNumTimeSamples()) {
        if (!hasEmptyDefaultValue(op, timeCode)) {
            return;
        }
    }

    UsdDataType attr_type = AL::usdmaya::utils::getAttributeType(op.GetTypeName());
    switch (attr_type) {
    case UsdDataType::kHalf: {
        GfHalf oldValue;
        op.Get(&oldValue);
        if (oldValue != GfHalf(value))
            op.Set(GfHalf(value), getTimeCodeForOp(op, timeCode));
    } break;

    case UsdDataType::kFloat: {
        float oldValue;
        op.Get(&oldValue);
        if (oldValue != float(value))
            op.Set(float(value), getTimeCodeForOp(op, timeCode));
    } break;

    case UsdDataType::kDouble: {
        double oldValue;
        op.Get(&oldValue);
        if (oldValue != double(value))
            op.Set(double(value), getTimeCodeForOp(op, timeCode));
    } break;

    case UsdDataType::kInt: {
        int32_t oldValue;
        op.Get(&oldValue);
        if (oldValue != int32_t(value))
            op.Set(int32_t(value), getTimeCodeForOp(op, timeCode));
    } break;

    default: break;
    }
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::readRotation(
    MEulerRotation&       result,
    const UsdGeomXformOp& op,
    UsdTimeCode           timeCode)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg(
            "TransformationMatrix::readRotation %f %f %f\n%s\n",
            result.x,
            result.y,
            result.z,
            op.GetOpName().GetText());
    const double degToRad = M_PI / 180.0;
    switch (op.GetOpType()) {
    case UsdGeomXformOp::TypeRotateX: {
        result.x = readDouble(op, timeCode) * degToRad;
        result.y = 0.0;
        result.z = 0.0;
        result.order = MEulerRotation::kXYZ;
    } break;

    case UsdGeomXformOp::TypeRotateY: {
        result.x = 0.0;
        result.y = readDouble(op, timeCode) * degToRad;
        result.z = 0.0;
        result.order = MEulerRotation::kXYZ;
    } break;

    case UsdGeomXformOp::TypeRotateZ: {
        result.x = 0.0;
        result.y = 0.0;
        result.z = readDouble(op, timeCode) * degToRad;
        result.order = MEulerRotation::kXYZ;
    } break;

    case UsdGeomXformOp::TypeRotateXYZ: {
        MVector v;
        if (readVector(v, op, timeCode)) {
            result.x = v.x * degToRad;
            result.y = v.y * degToRad;
            result.z = v.z * degToRad;
            result.order = MEulerRotation::kXYZ;
        } else
            return false;
    } break;

    case UsdGeomXformOp::TypeRotateXZY: {
        MVector v;
        if (readVector(v, op, timeCode)) {
            result.x = v.x * degToRad;
            result.y = v.y * degToRad;
            result.z = v.z * degToRad;
            result.order = MEulerRotation::kXZY;
        } else
            return false;
    } break;

    case UsdGeomXformOp::TypeRotateYXZ: {
        MVector v;
        if (readVector(v, op, timeCode)) {
            result.x = v.x * degToRad;
            result.y = v.y * degToRad;
            result.z = v.z * degToRad;
            result.order = MEulerRotation::kYXZ;
        } else
            return false;
    } break;

    case UsdGeomXformOp::TypeRotateYZX: {
        MVector v;
        if (readVector(v, op, timeCode)) {
            result.x = v.x * degToRad;
            result.y = v.y * degToRad;
            result.z = v.z * degToRad;
            result.order = MEulerRotation::kYZX;
        } else
            return false;
    } break;

    case UsdGeomXformOp::TypeRotateZXY: {
        MVector v;
        if (readVector(v, op, timeCode)) {
            result.x = v.x * degToRad;
            result.y = v.y * degToRad;
            result.z = v.z * degToRad;
            result.order = MEulerRotation::kZXY;
        } else
            return false;
    } break;

    case UsdGeomXformOp::TypeRotateZYX: {
        MVector v;
        if (readVector(v, op, timeCode)) {
            result.x = v.x * degToRad;
            result.y = v.y * degToRad;
            result.z = v.z * degToRad;
            result.order = MEulerRotation::kZYX;
        } else
            return false;
    } break;

    default: return false;
    }
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::pushRotation(
    const MEulerRotation& value,
    UsdGeomXformOp&       op,
    UsdTimeCode           timeCode)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg(
            "TransformationMatrix::pushRotation %f %f %f\n%s\n",
            value.x,
            value.y,
            value.z,
            op.GetOpName().GetText());

    if (timeCode.IsDefault() && op.GetNumTimeSamples()) {
        if (!hasEmptyDefaultValue(op, timeCode)) {
            return false;
        }
    }

    const double radToDeg = 180.0 / M_PI;
    switch (op.GetOpType()) {
    case UsdGeomXformOp::TypeRotateX: {
        pushDouble(value.x * radToDeg, op, timeCode);
    } break;

    case UsdGeomXformOp::TypeRotateY: {
        pushDouble(value.y * radToDeg, op, timeCode);
    } break;

    case UsdGeomXformOp::TypeRotateZ: {
        pushDouble(value.z * radToDeg, op, timeCode);
    } break;

    case UsdGeomXformOp::TypeRotateXYZ:
    case UsdGeomXformOp::TypeRotateXZY:
    case UsdGeomXformOp::TypeRotateYXZ:
    case UsdGeomXformOp::TypeRotateYZX:
    case UsdGeomXformOp::TypeRotateZYX:
    case UsdGeomXformOp::TypeRotateZXY: {
        MVector v(value.x, value.y, value.z);
        v *= radToDeg;
        return pushVector(v, op, timeCode);
    } break;

    default: return false;
    }
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::initialiseToPrim(bool readFromPrim, Scope* transformNode)
{
    MProfilingScope profilerScope(
        _transformationMatrixProfilerCategory, MProfiler::kColorE_L3, "Initialise to prim");

    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::initialiseToPrim\n");

    // if not yet initialized, do not execute this code! (It will crash!).
    if (!m_prim)
        return;

    bool resetsXformStack = false;
    m_xformops = m_xform.GetOrderedXformOps(&resetsXformStack);
    m_orderedOps.resize(m_xformops.size());

    if (!resetsXformStack) {
        m_flags |= kInheritsTransform;
    }

    if (matchesMayaProfile(m_xformops.begin(), m_xformops.end(), m_orderedOps.begin())) {
        m_flags |= kFromMayaSchema;
    } else {
    }

    {
        // We want to disable push to prim if enabled, otherwise MPlug value queries
        // and setting in the switch statement below will trigger pushing to the prim,
        // which creates undesirable "over"s.  To accomplish this, we create an object
        // which disables push to prim when created and resets that state back to what
        // it was originally when it goes out of scope.
        //
        // (Note that, because of m_flags processing at the end of this method, we need
        // to create a scope for disableNow which ends before that processing happens
        // instead of simply letting disableNow go out of scope when the function
        // exits.)
        //
        Scoped_DisablePushToPrim disableNow(*this);

        auto opIt = m_orderedOps.begin();
        for (std::vector<UsdGeomXformOp>::const_iterator it = m_xformops.begin(),
                                                         e = m_xformops.end();
             it != e;
             ++it, ++opIt) {
            const UsdGeomXformOp& op = *it;
            switch (*opIt) {
            case kTranslate: {
                m_flags |= kPrimHasTranslation;
                if (op.GetNumTimeSamples() > 1) {
                    m_flags |= kAnimatedTranslation;
                }
                if (readFromPrim) {
                    MVector tempTranslation;
                    internal_readVector(tempTranslation, op);
                    if (transformNode) {
                        MPlug(transformNode->thisMObject(), MPxTransform::translateX)
                            .setValue(tempTranslation.x);
                        MPlug(transformNode->thisMObject(), MPxTransform::translateY)
                            .setValue(tempTranslation.y);
                        MPlug(transformNode->thisMObject(), MPxTransform::translateZ)
                            .setValue(tempTranslation.z);
                        m_translationTweak[0] = m_translationTweak[1] = m_translationTweak[2] = 0;
                        m_translationFromUsd = tempTranslation;
                    }
                }
            } break;

            case kPivot: {
                m_flags |= kPrimHasPivot;
                if (readFromPrim) {
                    internal_readPoint(m_scalePivotFromUsd, op);
                    m_rotatePivotFromUsd = m_scalePivotFromUsd;
                    if (transformNode) {
                        MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotX)
                            .setValue(m_rotatePivotFromUsd.x);
                        MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotY)
                            .setValue(m_rotatePivotFromUsd.y);
                        MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotZ)
                            .setValue(m_rotatePivotFromUsd.z);
                        MPlug(transformNode->thisMObject(), MPxTransform::scalePivotX)
                            .setValue(m_scalePivotFromUsd.x);
                        MPlug(transformNode->thisMObject(), MPxTransform::scalePivotY)
                            .setValue(m_scalePivotFromUsd.y);
                        MPlug(transformNode->thisMObject(), MPxTransform::scalePivotZ)
                            .setValue(m_scalePivotFromUsd.z);
                    }
                }
            } break;

            case kRotatePivotTranslate: {
                m_flags |= kPrimHasRotatePivotTranslate;
                if (readFromPrim) {
                    internal_readVector(m_rotatePivotTranslationFromUsd, op);
                    if (transformNode) {
                        MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotTranslateX)
                            .setValue(m_rotatePivotTranslationFromUsd.x);
                        MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotTranslateY)
                            .setValue(m_rotatePivotTranslationFromUsd.y);
                        MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotTranslateZ)
                            .setValue(m_rotatePivotTranslationFromUsd.z);
                    }
                }
            } break;

            case kRotatePivot: {
                m_flags |= kPrimHasRotatePivot;
                if (readFromPrim) {
                    internal_readPoint(m_rotatePivotFromUsd, op);
                    if (transformNode) {
                        MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotX)
                            .setValue(m_rotatePivotFromUsd.x);
                        MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotY)
                            .setValue(m_rotatePivotFromUsd.y);
                        MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotZ)
                            .setValue(m_rotatePivotFromUsd.z);
                    }
                }
            } break;

            case kRotate: {
                m_flags |= kPrimHasRotation;
                if (op.GetNumTimeSamples() > 1) {
                    m_flags |= kAnimatedRotation;
                }
                if (readFromPrim) {
                    internal_readRotation(m_rotationFromUsd, op);
                    if (transformNode) {
                        m_rotationTweak[0] = m_rotationTweak[1] = m_rotationTweak[2] = 0;
                        // attempting to set the rotation via the attributes can end up failing when
                        // using zxy rotation orders. The only reliable way to set this value would
                        // appeear to be via MFnTransform :(
                        MFnTransform fn(m_transformNode.object());
                        fn.setRotation(m_rotationFromUsd);
                    }
                }
            } break;

            case kRotateAxis: {
                m_flags |= kPrimHasRotateAxes;
                if (readFromPrim) {
                    MEulerRotation eulers;
                    internal_readRotation(eulers, op);
                    m_rotateOrientationFromUsd = eulers.asQuaternion();
                    if (transformNode) {
                        MPlug(transformNode->thisMObject(), MPxTransform::rotateAxisX)
                            .setValue(eulers.x);
                        MPlug(transformNode->thisMObject(), MPxTransform::rotateAxisY)
                            .setValue(eulers.y);
                        MPlug(transformNode->thisMObject(), MPxTransform::rotateAxisZ)
                            .setValue(eulers.z);
                    }
                }
            } break;

            case kRotatePivotInv: {
            } break;

            case kScalePivotTranslate: {
                m_flags |= kPrimHasScalePivotTranslate;
                if (readFromPrim) {
                    internal_readVector(m_scalePivotTranslationFromUsd, op);
                    if (transformNode) {
                        MPlug(transformNode->thisMObject(), MPxTransform::scalePivotTranslateX)
                            .setValue(m_scalePivotTranslationFromUsd.x);
                        MPlug(transformNode->thisMObject(), MPxTransform::scalePivotTranslateY)
                            .setValue(m_scalePivotTranslationFromUsd.y);
                        MPlug(transformNode->thisMObject(), MPxTransform::scalePivotTranslateZ)
                            .setValue(m_scalePivotTranslationFromUsd.z);
                    }
                }
            } break;

            case kScalePivot: {
                m_flags |= kPrimHasScalePivot;
                if (readFromPrim) {
                    internal_readPoint(m_scalePivotFromUsd, op);
                    if (transformNode) {
                        MPlug(transformNode->thisMObject(), MPxTransform::scalePivotX)
                            .setValue(m_scalePivotFromUsd.x);
                        MPlug(transformNode->thisMObject(), MPxTransform::scalePivotY)
                            .setValue(m_scalePivotFromUsd.y);
                        MPlug(transformNode->thisMObject(), MPxTransform::scalePivotZ)
                            .setValue(m_scalePivotFromUsd.z);
                    }
                }
            } break;

            case kShear: {
                m_flags |= kPrimHasShear;
                if (op.GetNumTimeSamples() > 1) {
                    m_flags |= kAnimatedShear;
                }
                if (readFromPrim) {
                    MVector tempShear;
                    internal_readShear(tempShear, op);
                    if (transformNode) {
                        MPlug(transformNode->thisMObject(), MPxTransform::shearXY)
                            .setValue(tempShear.x);
                        MPlug(transformNode->thisMObject(), MPxTransform::shearXZ)
                            .setValue(tempShear.y);
                        MPlug(transformNode->thisMObject(), MPxTransform::shearYZ)
                            .setValue(tempShear.z);
                        m_shearTweak[0] = m_shearTweak[1] = m_shearTweak[2] = 0;
                        m_shearFromUsd = tempShear;
                    }
                }
            } break;

            case kScale: {
                m_flags |= kPrimHasScale;
                if (op.GetNumTimeSamples() > 1) {
                    m_flags |= kAnimatedScale;
                }
                if (readFromPrim) {
                    MVector tempScale(1.0, 1.0, 1.0);
                    internal_readVector(tempScale, op);
                    if (transformNode) {
                        MPlug(transformNode->thisMObject(), MPxTransform::scaleX)
                            .setValue(tempScale.x);
                        MPlug(transformNode->thisMObject(), MPxTransform::scaleY)
                            .setValue(tempScale.y);
                        MPlug(transformNode->thisMObject(), MPxTransform::scaleZ)
                            .setValue(tempScale.z);
                        m_scaleTweak[0] = m_scaleTweak[1] = m_scaleTweak[2] = 0;
                        m_scaleFromUsd = tempScale;
                    }
                }
            } break;

            case kScalePivotInv: {
            } break;

            case kPivotInv: {
            } break;

            case kTransform: {
                m_flags |= kPrimHasTransform;
                m_flags |= kFromMatrix;
                m_flags |= kPushPrimToMatrix;
                if (op.GetNumTimeSamples() > 1) {
                    m_flags |= kAnimatedMatrix;
                }

                if (readFromPrim) {
                    MMatrix m;
                    internal_readMatrix(m, op);
                    setFromMatrix(transformNode->thisMObject(), m);
                }
            } break;

            case kUnknownOp: {
            } break;
            }
        }

        // Push to prim will now be reset to its original state as the
        // disableNow variable goes out of scope here...
    }

    if (m_flags & kAnimationMask) {
        m_flags &= ~kPushToPrimEnabled;
        m_flags |= kReadAnimatedValues;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::updateToTime(const UsdTimeCode& time)
{
    MProfilingScope profilerScope(
        _transformationMatrixProfilerCategory, MProfiler::kColorE_L3, "Update to time");

    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::updateToTime %f\n", time.GetValue());
    // if not yet initialized, do not execute this code! (It will crash!).
    if (!m_prim) {
        return;
    }
    if (m_time != time) {
        m_time = time;
        {
            auto opIt = m_orderedOps.begin();
            for (std::vector<UsdGeomXformOp>::const_iterator it = m_xformops.begin(),
                                                             e = m_xformops.end();
                 it != e;
                 ++it, ++opIt) {
                const UsdGeomXformOp& op = *it;
                switch (*opIt) {
                case kTranslate: {
                    if (op.GetNumTimeSamples() >= 1) {
                        m_flags |= kAnimatedTranslation;
                        internal_readVector(m_translationFromUsd, op);
                        MPxTransformationMatrix::translationValue
                            = m_translationFromUsd + m_translationTweak;
                    }
                } break;

                case kRotate: {
                    if (op.GetNumTimeSamples() >= 1) {
                        m_flags |= kAnimatedRotation;
                        internal_readRotation(m_rotationFromUsd, op);
                        MPxTransformationMatrix::rotationValue = m_rotationFromUsd;
                        MPxTransformationMatrix::rotationValue.x += m_rotationTweak.x;
                        MPxTransformationMatrix::rotationValue.y += m_rotationTweak.y;
                        MPxTransformationMatrix::rotationValue.z += m_rotationTweak.z;
                    }
                } break;

                case kScale: {
                    if (op.GetNumTimeSamples() >= 1) {
                        m_flags |= kAnimatedScale;
                        internal_readVector(m_scaleFromUsd, op);
                        MPxTransformationMatrix::scaleValue = m_scaleFromUsd + m_scaleTweak;
                    }
                } break;

                case kShear: {
                    if (op.GetNumTimeSamples() >= 1) {
                        m_flags |= kAnimatedShear;
                        internal_readShear(m_shearFromUsd, op);
                        MPxTransformationMatrix::shearValue = m_shearFromUsd + m_shearTweak;
                    }
                } break;

                case kTransform: {
                    if (op.GetNumTimeSamples() >= 1) {
                        m_flags |= kAnimatedMatrix;
                        GfMatrix4d matrix;
                        op.Get<GfMatrix4d>(&matrix, getTimeCode());
                        double T[3], S[3];
                        AL::usdmaya::utils::matrixToSRT(matrix, S, m_rotationFromUsd, T);
                        m_scaleFromUsd.x = S[0];
                        m_scaleFromUsd.y = S[1];
                        m_scaleFromUsd.z = S[2];
                        m_translationFromUsd.x = T[0];
                        m_translationFromUsd.y = T[1];
                        m_translationFromUsd.z = T[2];
                        MPxTransformationMatrix::rotationValue.x
                            = m_rotationFromUsd.x + m_rotationTweak.x;
                        MPxTransformationMatrix::rotationValue.y
                            = m_rotationFromUsd.y + m_rotationTweak.y;
                        MPxTransformationMatrix::rotationValue.z
                            = m_rotationFromUsd.z + m_rotationTweak.z;
                        MPxTransformationMatrix::translationValue
                            = m_translationFromUsd + m_translationTweak;
                        MPxTransformationMatrix::scaleValue = m_scaleFromUsd + m_scaleTweak;
                    }
                } break;

                default: break;
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Translation
//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertTranslateOp()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertTranslateOp\n");
    // generate our translate op, and insert into the correct stack location
    UsdGeomXformOp op = m_xform.AddTranslateOp(UsdGeomXformOp::PrecisionDouble);
    m_xformops.insert(m_xformops.begin(), op);
    m_orderedOps.insert(m_orderedOps.begin(), kTranslate);
    m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
    m_flags |= kPrimHasTranslation;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::translateTo(const MVector& vector, MSpace::Space space)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::translateTo %f %f %f\n", vector.x, vector.y, vector.z);
    if (isTranslateLocked())
        return MPxTransformationMatrix::translateTo(vector, space);

    MStatus status = MPxTransformationMatrix::translateTo(vector, space);
    if (status) {
        m_translationTweak = MPxTransformationMatrix::translationValue - m_translationFromUsd;
    }

    if (pushToPrimAvailable()) {
        // if the prim does not contain a translation, make sure we insert a transform op for that.
        if (primHasTranslation()) {
            // helping the branch predictor
        } else if (!pushPrimToMatrix() && vector != MVector(0.0, 0.0, 0.0)) {
            insertTranslateOp();
        }

        // Push new value to prim, but only if it's changing, otherwise extra work and unintended
        // side effects will happen.
        //
        if (!vector.isEquivalent(m_translationFromUsd)) {
            pushTranslateToPrim();
        }
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
// Scale
//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertScaleOp()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertScaleOp\n");

    // generate our translate op, and insert into the correct stack location
    UsdGeomXformOp op = m_xform.AddScaleOp(UsdGeomXformOp::PrecisionFloat);

    auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kScale);
    auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());

    m_xformops.insert(posInXfm, op);
    m_orderedOps.insert(posInOps, kScale);
    m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
    m_flags |= kPrimHasScale;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::scaleTo(const MVector& scale, MSpace::Space space)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::scaleTo %f %f %f\n", scale.x, scale.y, scale.z);
    if (isScaleLocked())
        return MPxTransformationMatrix::scaleTo(scale, space);

    MStatus status = MPxTransformationMatrix::scaleTo(scale, space);
    if (status) {
        m_scaleTweak = MPxTransformationMatrix::scaleValue - m_scaleFromUsd;
    }
    if (pushToPrimAvailable()) {
        if (primHasScale()) {
            // helping the branch predictor
        } else if (!pushPrimToMatrix() && scale != MVector(1.0, 1.0, 1.0)) {
            // rare case: add a new scale op into the prim
            insertScaleOp();
        }
        // Push new value to prim, but only if it's changing.
        if (!scale.isEquivalent(m_scaleFromUsd)) {
            pushScaleToPrim();
        }
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
// Shear
//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertShearOp()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertShearOp\n");
    // generate our translate op, and insert into the correct stack location
    UsdGeomXformOp op = m_xform.AddTransformOp(UsdGeomXformOp::PrecisionDouble, TfToken("shear"));

    auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kShear);
    auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());

    m_xformops.insert(posInXfm, op);
    m_orderedOps.insert(posInOps, kShear);
    m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
    m_flags |= kPrimHasShear;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::shearTo(const MVector& shear, MSpace::Space space)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::shearTo %f %f %f\n", shear.x, shear.y, shear.z);
    if (isShearLocked())
        return MPxTransformationMatrix::shearTo(shear, space);
    MStatus status = MPxTransformationMatrix::shearTo(shear, space);
    if (status) {
        m_shearTweak = MPxTransformationMatrix::shearValue - m_shearFromUsd;
    }
    if (pushToPrimAvailable()) {
        if (primHasShear()) {
            // helping the branch predictor
        } else if (!pushPrimToMatrix() && shear != MVector(0.0, 0.0, 0.0)) {
            // rare case: add a new scale op into the prim
            insertShearOp();
        }
        // Push new value to prim, but only if it's changing.
        if (!shear.isEquivalent(m_shearFromUsd)) {
            pushShearToPrim();
        }
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertScalePivotOp()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertScalePivotOp\n");
    // generate our translate op, and insert into the correct stack location
    UsdGeomXformOp op
        = m_xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivot"));
    UsdGeomXformOp opinv
        = m_xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivot"), true);

    {
        auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kScalePivot);
        auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());
        m_xformops.insert(posInXfm, op);
        m_orderedOps.insert(posInOps, kScalePivot);
    }
    {
        auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kScalePivotInv);
        auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());
        m_xformops.insert(posInXfm, opinv);
        m_orderedOps.insert(posInOps, kScalePivotInv);
    }
    m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
    m_flags |= kPrimHasScalePivot;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setScalePivot(const MPoint& sp, MSpace::Space space, bool balance)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::setScalePivot %f %f %f\n", sp.x, sp.y, sp.z);
    MStatus status = MPxTransformationMatrix::setScalePivot(sp, space, balance);
    if (status) {
        m_scalePivotTweak = MPxTransformationMatrix::scalePivotValue - m_scalePivotFromUsd;
    }
    if (pushToPrimAvailable()) {
        // Do not insert a scale pivot op if the input prim has a generic pivot.
        if (primHasScalePivot() || primHasPivot()) {
        } else if (!pushPrimToMatrix() && sp != MPoint(0.0, 0.0, 0.0)) {
            insertScalePivotOp();
        }
        // Push new value to prim, but only if it's changing.
        if (!sp.isEquivalent(m_scalePivotFromUsd)) {
            pushScalePivotToPrim();
        }
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertScalePivotTranslationOp()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::insertScalePivotTranslationOp\n");
    // generate our translate op, and insert into the correct stack location
    UsdGeomXformOp op
        = m_xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivotTranslate"));

    auto posInOps
        = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kScalePivotTranslate);
    auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());

    m_xformops.insert(posInXfm, op);
    m_orderedOps.insert(posInOps, kScalePivotTranslate);
    m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
    m_flags |= kPrimHasScalePivotTranslate;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setScalePivotTranslation(const MVector& sp, MSpace::Space space)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::setScalePivotTranslation %f %f %f\n", sp.x, sp.y, sp.z);
    MStatus status = MPxTransformationMatrix::setScalePivotTranslation(sp, space);
    if (status) {
        m_scalePivotTranslationTweak
            = MPxTransformationMatrix::scalePivotTranslationValue - m_scalePivotTranslationFromUsd;
    }
    if (pushToPrimAvailable()) {
        if (primHasScalePivotTranslate()) {
        } else if (!pushPrimToMatrix() && sp != MVector(0.0, 0.0, 0.0)) {
            insertScalePivotTranslationOp();
        }
        // Push new value to prim, but only if it's changing.
        if (!sp.isEquivalent(m_scalePivotTranslationFromUsd)) {
            pushScalePivotTranslateToPrim();
        }
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertRotatePivotOp()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertRotatePivotOp\n");
    // generate our translate op, and insert into the correct stack location
    UsdGeomXformOp op
        = m_xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivot"));
    UsdGeomXformOp opinv
        = m_xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivot"), true);

    {
        auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kRotatePivot);
        auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());
        m_xformops.insert(posInXfm, op);
        m_orderedOps.insert(posInOps, kRotatePivot);
    }
    {
        auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kRotatePivotInv);
        auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());
        m_xformops.insert(posInXfm, opinv);
        m_orderedOps.insert(posInOps, kRotatePivotInv);
    }
    m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
    m_flags |= kPrimHasRotatePivot;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setRotatePivot(const MPoint& pivot, MSpace::Space space, bool balance)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::setRotatePivot %f %f %f\n", pivot.x, pivot.y, pivot.z);
    MStatus status = MPxTransformationMatrix::setRotatePivot(pivot, space, balance);
    if (status) {
        m_rotatePivotTweak = MPxTransformationMatrix::rotatePivotValue - m_rotatePivotFromUsd;
    }
    if (pushToPrimAvailable()) {
        // Do not insert a rotate pivot op if the input prim has a generic pivot.
        if (primHasRotatePivot() || primHasPivot()) {
        } else if (!pushPrimToMatrix() && pivot != MPoint(0.0, 0.0, 0.0)) {
            insertRotatePivotOp();
        }
        // Push new value to prim, but only if it's changing.
        if (!pivot.isEquivalent(m_rotatePivotFromUsd)) {
            pushRotatePivotToPrim();
        }
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertRotatePivotTranslationOp()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::insertRotatePivotTranslationOp\n");
    // generate our translate op, and insert into the correct stack location
    UsdGeomXformOp op
        = m_xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivotTranslate"));

    auto posInOps
        = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kRotatePivotTranslate);
    auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());

    m_xformops.insert(posInXfm, op);
    m_orderedOps.insert(posInOps, kRotatePivotTranslate);
    m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
    m_flags |= kPrimHasRotatePivotTranslate;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setRotatePivotTranslation(const MVector& vector, MSpace::Space space)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg(
            "TransformationMatrix::setRotatePivotTranslation %f %f %f\n",
            vector.x,
            vector.y,
            vector.z);
    MStatus status = MPxTransformationMatrix::setRotatePivotTranslation(vector, space);
    if (status) {
        m_rotatePivotTranslationTweak = MPxTransformationMatrix::rotatePivotTranslationValue
            - m_rotatePivotTranslationFromUsd;
    }
    if (pushToPrimAvailable()) {
        if (primHasRotatePivotTranslate()) {
        } else if (!pushPrimToMatrix() && vector != MVector(0.0, 0.0, 0.0)) {
            insertRotatePivotTranslationOp();
        }
        // Push new value to prim, but only if it's changing.
        if (!vector.isEquivalent(m_rotatePivotTranslationFromUsd)) {
            pushRotatePivotTranslateToPrim();
        }
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertRotateOp()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertRotateOp\n");
    // generate our translate op, and insert into the correct stack location
    UsdGeomXformOp op;

    switch (rotationOrder()) {
    case MTransformationMatrix::kXYZ:
        op = m_xform.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat);
        break;

    case MTransformationMatrix::kXZY:
        op = m_xform.AddRotateXZYOp(UsdGeomXformOp::PrecisionFloat);
        break;

    case MTransformationMatrix::kYXZ:
        op = m_xform.AddRotateYXZOp(UsdGeomXformOp::PrecisionFloat);
        break;

    case MTransformationMatrix::kYZX:
        op = m_xform.AddRotateYZXOp(UsdGeomXformOp::PrecisionFloat);
        break;

    case MTransformationMatrix::kZXY:
        op = m_xform.AddRotateZXYOp(UsdGeomXformOp::PrecisionFloat);
        break;

    case MTransformationMatrix::kZYX:
        op = m_xform.AddRotateZYXOp(UsdGeomXformOp::PrecisionFloat);
        break;

    default:
        TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
            .Msg("TransformationMatrix::insertRotateOp - got invalid rotation order; assuming XYZ");
        op = m_xform.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat);
        break;
    }

    auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kRotate);
    auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());

    m_xformops.insert(posInXfm, op);
    m_orderedOps.insert(posInOps, kRotate);
    m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
    m_flags |= kPrimHasRotation;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::rotateTo(const MQuaternion& q, MSpace::Space space)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::rotateTo %f %f %f %f\n", q.x, q.y, q.z, q.w);
    if (isRotateLocked())
        return MPxTransformationMatrix::rotateTo(q, space);
    ;
    MStatus status = MPxTransformationMatrix::rotateTo(q, space);
    if (status) {
        m_rotationTweak.x = MPxTransformationMatrix::rotationValue.x - m_rotationFromUsd.x;
        m_rotationTweak.y = MPxTransformationMatrix::rotationValue.y - m_rotationFromUsd.y;
        m_rotationTweak.z = MPxTransformationMatrix::rotationValue.z - m_rotationFromUsd.z;
    }
    if (pushToPrimAvailable()) {
        if (primHasRotation()) {
        } else if (!pushPrimToMatrix() && q != MQuaternion(0.0, 0.0, 0.0, 1.0)) {
            insertRotateOp();
        }
        if (m_enableUsdWriteback) {
            // Push new value to prim, but only if it's changing.
            if (!MPxTransformationMatrix::rotationValue.isEquivalent(m_rotationFromUsd)) {
                pushRotateQuatToPrim();
            }
        }
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::rotateTo(const MEulerRotation& e, MSpace::Space space)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::rotateTo %f %f %f\n", e.x, e.y, e.z);
    if (isRotateLocked())
        return MPxTransformationMatrix::rotateTo(e, space);
    ;
    MStatus status = MPxTransformationMatrix::rotateTo(e, space);
    if (status) {
        m_rotationTweak.x = MPxTransformationMatrix::rotationValue.x - m_rotationFromUsd.x;
        m_rotationTweak.y = MPxTransformationMatrix::rotationValue.y - m_rotationFromUsd.y;
        m_rotationTweak.z = MPxTransformationMatrix::rotationValue.z - m_rotationFromUsd.z;
    }
    if (pushToPrimAvailable()) {
        if (primHasRotation()) {
        } else if (
            !pushPrimToMatrix() && e != MEulerRotation(0.0, 0.0, 0.0, MEulerRotation::kXYZ)) {
            insertRotateOp();
        }
        if (m_enableUsdWriteback) {
            // Push new value to prim, but only if it's changing.
            if (!e.isEquivalent(m_rotationFromUsd)) {
                pushRotateToPrim();
            }
        }
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setRotationOrder(MTransformationMatrix::RotationOrder, bool)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::setRotationOrder\n");
    // do not allow people to change the rotation order here.
    // It's too hard for my feeble brain to figure out how to remap that to the USD data.
    return MS::kFailure;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertRotateAxesOp()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertRotateAxesOp\n");
    // generate our translate op, and insert into the correct stack location
    UsdGeomXformOp op
        = m_xform.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotateAxis"));

    auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kRotateAxis);
    auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());

    m_xformops.insert(posInXfm, op);
    m_orderedOps.insert(posInOps, kRotateAxis);
    m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
    m_flags |= kPrimHasRotateAxes;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
TransformationMatrix::setRotateOrientation(const MQuaternion& q, MSpace::Space space, bool balance)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::setRotateOrientation %f %f %f %f\n", q.x, q.y, q.z, q.w);
    MStatus status = MPxTransformationMatrix::setRotateOrientation(q, space, balance);
    if (status) {
        m_rotateOrientationFromUsd
            = MPxTransformationMatrix::rotateOrientationValue * m_rotateOrientationTweak.inverse();
    }
    if (pushToPrimAvailable()) {
        if (primHasRotateAxes()) {
        } else if (!pushPrimToMatrix() && q != MQuaternion(0.0, 0.0, 0.0, 1.0)) {
            insertRotateAxesOp();
        }
        if (m_enableUsdWriteback) {
            pushRotateAxisToPrim();
        }
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setRotateOrientation(
    const MEulerRotation& euler,
    MSpace::Space         space,
    bool                  balance)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::setRotateOrientation %f %f %f\n", euler.x, euler.y, euler.z);
    MStatus status = MPxTransformationMatrix::setRotateOrientation(euler, space, balance);
    if (status) {
        m_rotateOrientationFromUsd
            = MPxTransformationMatrix::rotateOrientationValue * m_rotateOrientationTweak.inverse();
    }
    if (pushToPrimAvailable()) {
        if (primHasRotateAxes()) {
        } else if (
            !pushPrimToMatrix() && euler != MEulerRotation(0.0, 0.0, 0.0, MEulerRotation::kXYZ)) {
            insertRotateAxesOp();
        }
        if (m_enableUsdWriteback) {
            pushRotateAxisToPrim();
        }
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::notifyProxyShapeOfRedraw(GfMatrix4d& oldMatrix, bool oldResetsStack)
{
    // Anytime we update the xform, we need to tell the proxy shape that it
    // needs to redraw itself
    MObject tn(m_transformNode.object());
    if (!tn.isNull()) {
        MStatus           status;
        MFnDependencyNode mfn(tn, &status);
        if (status && mfn.typeId() == Transform::kTypeId) {
            auto    xform = static_cast<Transform*>(mfn.userNode());
            MObject proxyObj = xform->getProxyShape();
            if (!proxyObj.isNull()) {
                MFnDependencyNode proxyMfn(proxyObj);
                if (proxyMfn.typeId() == ProxyShape::kTypeId) {
                    // We check that the matrix actually HAS changed, as this function will be
                    // called when, ie, pushToPrim is toggled, which often happens on node
                    // creation, when nothing has actually changed
                    GfMatrix4d newMatrix;
                    bool       newResetsStack;
                    m_xform.GetLocalTransformation(&newMatrix, &newResetsStack, getTimeCode());
                    if (newMatrix != oldMatrix || newResetsStack != oldResetsStack) {
                        MHWRender::MRenderer::setGeometryDrawDirty(proxyObj);
                    }
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushTranslateToPrim()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushTranslateToPrim\n");
    auto opIt = m_orderedOps.begin();
    for (std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end();
         it != e;
         ++it, ++opIt) {
        if (*opIt == kTranslate) {
            UsdGeomXformOp& op = *it;
            MVector         tempTranslation;
            internal_readVector(tempTranslation, op);
            // only write back if data has changed significantly
            if (!tempTranslation.isEquivalent(MPxTransformationMatrix::translationValue)) {
                internal_pushVector(MPxTransformationMatrix::translationValue, op);
                m_translationFromUsd = MPxTransformationMatrix::translationValue;
                m_translationTweak = MVector(0, 0, 0);
            }
            return;
        }
    }
    if (m_enableUsdWriteback) {
        pushTransformToPrim();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushPivotToPrim()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushPivotToPrim\n");
    auto opIt = m_orderedOps.begin();
    for (std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end();
         it != e;
         ++it, ++opIt) {
        if (*opIt == kPivot) {
            UsdGeomXformOp& op = *it;
            MPoint          tempPivot;
            internal_readPoint(tempPivot, op);

            // only write back if data has changed significantly
            if (!tempPivot.isEquivalent(MPxTransformationMatrix::rotatePivotValue)) {
                internal_pushPoint(MPxTransformationMatrix::rotatePivotValue, op);
                m_rotatePivotFromUsd = MPxTransformationMatrix::rotatePivotValue;
                m_rotatePivotTweak = MPoint(0, 0, 0);
                m_scalePivotFromUsd = MPxTransformationMatrix::scalePivotValue;
                m_scalePivotTweak = MVector(0, 0, 0);
            }
            return;
        }
    }
    if (m_enableUsdWriteback) {
        pushTransformToPrim();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushRotatePivotToPrim()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushRotatePivotToPrim\n");
    auto opIt = m_orderedOps.begin();
    for (std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end();
         it != e;
         ++it, ++opIt) {
        if (*opIt == kRotatePivot) {
            UsdGeomXformOp& op = *it;
            MPoint          tempPivot;
            internal_readPoint(tempPivot, op);
            // only write back if data has changed significantly
            if (!tempPivot.isEquivalent(MPxTransformationMatrix::rotatePivotValue)) {
                internal_pushPoint(MPxTransformationMatrix::rotatePivotValue, op);
                m_rotatePivotFromUsd = MPxTransformationMatrix::rotatePivotValue;
                m_rotatePivotTweak = MVector(0, 0, 0);
            }
            return;
        }
    }
    if (m_enableUsdWriteback) {
        pushTransformToPrim();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushRotatePivotTranslateToPrim()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::pushRotatePivotTranslateToPrim\n");
    auto opIt = m_orderedOps.begin();
    for (std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end();
         it != e;
         ++it, ++opIt) {
        if (*opIt == kRotatePivotTranslate) {
            UsdGeomXformOp& op = *it;
            MVector         tempPivotTranslation;
            internal_readVector(tempPivotTranslation, op);
            // only write back if data has changed significantly
            if (!tempPivotTranslation.isEquivalent(
                    MPxTransformationMatrix::rotatePivotTranslationValue)) {
                internal_pushPoint(MPxTransformationMatrix::rotatePivotTranslationValue, op);
                m_rotatePivotTranslationFromUsd
                    = MPxTransformationMatrix::rotatePivotTranslationValue;
                m_rotatePivotTranslationTweak = MVector(0, 0, 0);
            }
            return;
        }
    }
    if (m_enableUsdWriteback) {
        pushTransformToPrim();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushRotateToPrim()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushRotateToPrim\n");
    auto opIt = m_orderedOps.begin();
    for (std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end();
         it != e;
         ++it, ++opIt) {
        if (*opIt == kRotate) {
            UsdGeomXformOp& op = *it;
            MEulerRotation  tempRotate;
            internal_readRotation(tempRotate, op);

            // only write back if data has changed significantly
            // Note: the rotation values are converted to quaternion form to avoid
            //       checking in Euler space, twisted values e.g.:
            //       (180, -2.31718, 180) == (0, 182.317, 0)
            //       are in fact the same although their raw values look different.
            //       Also notice that we lower the tolerance for comparison since
            //       the values are converted from Euler.
            if (!tempRotate.asQuaternion().isEquivalent(
                    MPxTransformationMatrix::rotationValue.asQuaternion(), 1e-5)) {
                internal_pushRotation(MPxTransformationMatrix::rotationValue, op);
                m_rotationFromUsd = MPxTransformationMatrix::rotationValue;
                m_rotationTweak = MEulerRotation(0, 0, 0);
            }
            return;
        }
    }
    if (m_enableUsdWriteback) {
        pushTransformToPrim();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushRotateQuatToPrim()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushRotateQuatToPrim\n");
    auto opIt = m_orderedOps.begin();
    for (std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end();
         it != e;
         ++it, ++opIt) {
        if (*opIt == kRotate) {
            UsdGeomXformOp& op = *it;
            MEulerRotation  tempRotate;
            internal_readRotation(tempRotate, op);

            // only write back if data has changed significantly
            if (!tempRotate.asQuaternion().isEquivalent(
                    MPxTransformationMatrix::rotationValue.asQuaternion())) {
                internal_pushRotation(MPxTransformationMatrix::rotationValue, op);
                m_rotationFromUsd = MPxTransformationMatrix::rotationValue;
                m_rotationTweak = MEulerRotation(0, 0, 0);
            }
            return;
        }
    }
    if (m_enableUsdWriteback) {
        pushTransformToPrim();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushRotateAxisToPrim()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushRotateAxisToPrim\n");
    auto opIt = m_orderedOps.begin();
    for (std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end();
         it != e;
         ++it, ++opIt) {
        if (*opIt == kRotateAxis) {
            UsdGeomXformOp& op = *it;
            MVector         tempRotateAxis;
            internal_readVector(tempRotateAxis, op);
            tempRotateAxis *= (M_PI / 180.0);

            MEulerRotation temp(tempRotateAxis.x, tempRotateAxis.y, tempRotateAxis.z);

            // only write back if data has changed significantly
            if (!(temp.asQuaternion()).isEquivalent(m_rotateOrientationFromUsd)) {
                const double   radToDeg = 180.0 / M_PI;
                MEulerRotation e = m_rotateOrientationFromUsd.asEulerRotation();
                MVector        vec(e.x * radToDeg, e.y * radToDeg, e.z * radToDeg);
                internal_pushVector(vec, op);
            }
            return;
        }
    }
    if (m_enableUsdWriteback) {
        pushTransformToPrim();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushScalePivotTranslateToPrim()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::pushScalePivotTranslateToPrim\n");
    auto opIt = m_orderedOps.begin();
    for (std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end();
         it != e;
         ++it, ++opIt) {
        if (*opIt == kScalePivotTranslate) {
            UsdGeomXformOp& op = *it;
            MVector         tempPivotTranslation;
            internal_readVector(tempPivotTranslation, op);
            // only write back if data has changed significantly
            if (!tempPivotTranslation.isEquivalent(
                    MPxTransformationMatrix::scalePivotTranslationValue)) {
                internal_pushVector(MPxTransformationMatrix::scalePivotTranslationValue, op);
                m_scalePivotTranslationFromUsd
                    = MPxTransformationMatrix::scalePivotTranslationValue;
                m_scalePivotTranslationTweak = MVector(0, 0, 0);
            }
            return;
        }
    }
    if (m_enableUsdWriteback) {
        pushTransformToPrim();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushScalePivotToPrim()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushScalePivotToPrim\n");
    auto opIt = m_orderedOps.begin();
    for (std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end();
         it != e;
         ++it, ++opIt) {
        if (*opIt == kScalePivot) {
            UsdGeomXformOp& op = *it;
            MPoint          tempPivot;
            internal_readPoint(tempPivot, op);
            // only write back if data has changed significantly
            if (!tempPivot.isEquivalent(MPxTransformationMatrix::scalePivotValue)) {
                internal_pushPoint(MPxTransformationMatrix::scalePivotValue, op);
                m_scalePivotFromUsd = MPxTransformationMatrix::scalePivotValue;
                m_scalePivotTweak = MPoint(0, 0, 0);
            }
            return;
        }
    }
    if (m_enableUsdWriteback) {
        pushTransformToPrim();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushScaleToPrim()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushScaleToPrim\n");
    auto opIt = m_orderedOps.begin();
    for (std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end();
         it != e;
         ++it, ++opIt) {
        if (*opIt == kScale) {
            UsdGeomXformOp& op = *it;
            MVector         tempScale(1.0, 1.0, 1.0);
            internal_readVector(tempScale, op);
            // only write back if data has changed significantly
            if (!tempScale.isEquivalent(MPxTransformationMatrix::scaleValue)) {
                internal_pushVector(MPxTransformationMatrix::scaleValue, op);
                m_scaleFromUsd = MPxTransformationMatrix::scaleValue;
                m_scaleTweak = MVector(0, 0, 0);
            }
            return;
        }
    }
    if (m_enableUsdWriteback) {
        pushTransformToPrim();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushShearToPrim()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushShearToPrim\n");
    auto opIt = m_orderedOps.begin();
    for (std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end();
         it != e;
         ++it, ++opIt) {
        if (*opIt == kShear) {
            UsdGeomXformOp& op = *it;
            internal_pushShear(MPxTransformationMatrix::shearValue, op);
            m_shearFromUsd = MPxTransformationMatrix::shearValue;
            m_shearTweak = MVector(0, 0, 0);
            return;
        }
    }
    if (m_enableUsdWriteback) {
        pushTransformToPrim();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushTransformToPrim()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushTransformToPrim\n");
    auto opIt = m_orderedOps.begin();
    for (std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end();
         it != e;
         ++it, ++opIt) {
        if (*opIt == kTransform) {
            UsdGeomXformOp& op = *it;
            if (pushPrimToMatrix()) {
                internal_pushMatrix(asMatrix(), op);
            }
            return;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushToPrim()
{
    // if not yet intiaialised, do not execute this code! (It will crash!).
    if (!m_prim)
        return;
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushToPrim\n");

    GfMatrix4d oldMatrix;
    bool       oldResetsStack;
    m_xform.GetLocalTransformation(&oldMatrix, &oldResetsStack, getTimeCode());

    auto opIt = m_orderedOps.begin();
    for (std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end();
         it != e;
         ++it, ++opIt) {
        UsdGeomXformOp& op = *it;
        switch (*opIt) {
        case kTranslate: {
            internal_pushVector(MPxTransformationMatrix::translationValue, op);
            m_translationFromUsd = MPxTransformationMatrix::translationValue;
            m_translationTweak = MVector(0, 0, 0);
        } break;

        case kPivot: {
            // is this a bug?
            internal_pushPoint(MPxTransformationMatrix::rotatePivotValue, op);
            m_rotatePivotFromUsd = MPxTransformationMatrix::rotatePivotValue;
            m_rotatePivotTweak = MPoint(0, 0, 0);
            m_scalePivotFromUsd = MPxTransformationMatrix::scalePivotValue;
            m_scalePivotTweak = MVector(0, 0, 0);
        } break;

        case kRotatePivotTranslate: {
            internal_pushPoint(MPxTransformationMatrix::rotatePivotTranslationValue, op);
            m_rotatePivotTranslationFromUsd = MPxTransformationMatrix::rotatePivotTranslationValue;
            m_rotatePivotTranslationTweak = MVector(0, 0, 0);
        } break;

        case kRotatePivot: {
            internal_pushPoint(MPxTransformationMatrix::rotatePivotValue, op);
            m_rotatePivotFromUsd = MPxTransformationMatrix::rotatePivotValue;
            m_rotatePivotTweak = MPoint(0, 0, 0);
        } break;

        case kRotate: {
            internal_pushRotation(MPxTransformationMatrix::rotationValue, op);
            m_rotationFromUsd = MPxTransformationMatrix::rotationValue;
            m_rotationTweak = MEulerRotation(0, 0, 0);
        } break;

        case kRotateAxis: {
            const double   radToDeg = 180.0 / M_PI;
            MEulerRotation e = m_rotateOrientationFromUsd.asEulerRotation();
            MVector        vec(e.x * radToDeg, e.y * radToDeg, e.z * radToDeg);
            internal_pushVector(vec, op);
        } break;

        case kRotatePivotInv: {
        } break;

        case kScalePivotTranslate: {
            internal_pushVector(MPxTransformationMatrix::scalePivotTranslationValue, op);
            m_scalePivotTranslationFromUsd = MPxTransformationMatrix::scalePivotTranslationValue;
            m_scalePivotTranslationTweak = MVector(0, 0, 0);
        } break;

        case kScalePivot: {
            internal_pushPoint(MPxTransformationMatrix::scalePivotValue, op);
            m_scalePivotFromUsd = MPxTransformationMatrix::scalePivotValue;
            m_scalePivotTweak = MPoint(0, 0, 0);
        } break;

        case kShear: {
            internal_pushShear(MPxTransformationMatrix::shearValue, op);
            m_shearFromUsd = MPxTransformationMatrix::shearValue;
            m_shearTweak = MVector(0, 0, 0);
        } break;

        case kScale: {
            internal_pushVector(MPxTransformationMatrix::scaleValue, op);
            m_scaleFromUsd = MPxTransformationMatrix::scaleValue;
            m_scaleTweak = MVector(0, 0, 0);
        } break;

        case kScalePivotInv: {
        } break;

        case kPivotInv: {
        } break;

        case kTransform: {
            if (pushPrimToMatrix()) {
                internal_pushMatrix(asMatrix(), op);
            }
        } break;

        case kUnknownOp: {
        } break;
        }
    }
    notifyProxyShapeOfRedraw(oldMatrix, oldResetsStack);
}

//----------------------------------------------------------------------------------------------------------------------
MMatrix TransformationMatrix::asMatrix() const
{
    MMatrix m = MPxTransformationMatrix::asMatrix();

    const double x = m_localTranslateOffset.x;
    const double y = m_localTranslateOffset.y;
    const double z = m_localTranslateOffset.z;

    m[3][0] += m[0][0] * x;
    m[3][1] += m[0][1] * x;
    m[3][2] += m[0][2] * x;
    m[3][0] += m[1][0] * y;
    m[3][1] += m[1][1] * y;
    m[3][2] += m[1][2] * y;
    m[3][0] += m[2][0] * z;
    m[3][1] += m[2][1] * z;
    m[3][2] += m[2][2] * z;

    // Let Maya know what the matrix should be
    return m;
}

//----------------------------------------------------------------------------------------------------------------------
MMatrix TransformationMatrix::asMatrix(double percent) const
{
    MMatrix m = MPxTransformationMatrix::asMatrix(percent);

    const double x = m_localTranslateOffset.x * percent;
    const double y = m_localTranslateOffset.y * percent;
    const double z = m_localTranslateOffset.z * percent;

    m[3][0] += m[0][0] * x;
    m[3][1] += m[0][1] * x;
    m[3][2] += m[0][2] * x;
    m[3][0] += m[1][0] * y;
    m[3][1] += m[1][1] * y;
    m[3][2] += m[1][2] * y;
    m[3][0] += m[2][0] * z;
    m[3][1] += m[2][1] * z;
    m[3][2] += m[2][2] * z;

    return m;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::enableReadAnimatedValues(bool enabled)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::enableReadAnimatedValues\n");
    if (enabled)
        m_flags |= kReadAnimatedValues;
    else
        m_flags &= ~kReadAnimatedValues;

    // if not yet intiaialised, do not execute this code! (It will crash!).
    if (!m_prim)
        return;

    // if we are enabling push to prim, we need to see if anything has changed on the transform
    // since the last time the values were synced. I'm assuming that if a given transform attribute
    // is not the same as the default, or the prim already has a transform op for that attribute,
    // then just call a method to make a minor adjustment of nothing. This will call my code that
    // will magically construct the transform ops in the right order.
    if (enabled) {
        const MVector     nullVec(0, 0, 0);
        const MVector     oneVec(1.0, 1.0, 1.0);
        const MPoint      nullPoint(0, 0, 0);
        const MQuaternion nullQuat(0, 0, 0, 1.0);

        if (!pushPrimToMatrix()) {
            if (primHasTranslation() || translation() != nullVec)
                translateBy(nullVec);

            if (primHasScale() || scale() != oneVec)
                scaleBy(oneVec);

            if (primHasShear() || shear() != nullVec)
                shearBy(nullVec);

            if (primHasScalePivot() || scalePivot() != nullPoint)
                setScalePivot(scalePivot(), MSpace::kTransform, false);

            if (primHasScalePivotTranslate() || scalePivotTranslation() != nullVec)
                setScalePivotTranslation(scalePivotTranslation(), MSpace::kTransform);

            if (primHasRotatePivot() || rotatePivot() != nullPoint)
                setRotatePivot(rotatePivot(), MSpace::kTransform, false);

            if (primHasRotatePivotTranslate() || rotatePivotTranslation() != nullVec)
                setRotatePivotTranslation(rotatePivotTranslation(), MSpace::kTransform);

            if (primHasRotation() || rotation() != nullQuat)
                rotateBy(nullQuat);

            if (primHasRotateAxes() || rotateOrientation() != nullQuat)
                setRotateOrientation(rotateOrientation(), MSpace::kTransform, false);
        } else if (primHasTransform()) {
            auto transformIt = std::find(m_orderedOps.begin(), m_orderedOps.end(), kTransform);
            if (transformIt != m_orderedOps.end()) {
                internal_pushMatrix(
                    asMatrix(), m_xformops[std::distance(m_orderedOps.begin(), transformIt)]);
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::enablePushToPrim(bool enabled)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("TransformationMatrix::enablePushToPrim %d\n", enabled);
    if (enabled)
        m_flags |= kPushToPrimEnabled;
    else
        m_flags &= ~kPushToPrimEnabled;

    // if not yet intiaialised, do not execute this code! (It will crash!).
    if (!m_prim)
        return;

    // if we are enabling push to prim, we need to see if anything has changed on the transform
    // since the last time the values were synced. I'm assuming that if a given transform attribute
    // is not the same as the default, or the prim already has a transform op for that attribute,
    // then just call a method to make a minor adjustment of nothing. This will call my code that
    // will magically construct the transform ops in the right order.
    if (enabled && getTimeCode() == UsdTimeCode::Default()) {
        const MVector     nullVec(0, 0, 0);
        const MVector     oneVec(1.0, 1.0, 1.0);
        const MPoint      nullPoint(0, 0, 0);
        const MQuaternion nullQuat(0, 0, 0, 1.0);

        if (!pushPrimToMatrix()) {
            if (primHasTranslation() || translation() != nullVec)
                translateTo(translation());

            if (primHasScale() || scale() != oneVec)
                scaleTo(scale());

            if (primHasShear() || shear() != nullVec)
                shearTo(shear());

            if (primHasScalePivot() || scalePivot() != nullPoint)
                setScalePivot(scalePivot(), MSpace::kTransform, false);

            if (primHasScalePivotTranslate() || scalePivotTranslation() != nullVec)
                setScalePivotTranslation(scalePivotTranslation(), MSpace::kTransform);

            if (primHasRotatePivot() || rotatePivot() != nullPoint)
                setRotatePivot(rotatePivot(), MSpace::kTransform, false);

            if (primHasRotatePivotTranslate() || rotatePivotTranslation() != nullVec)
                setRotatePivotTranslation(rotatePivotTranslation(), MSpace::kTransform);

            if (primHasRotation() || rotation() != nullQuat)
                rotateTo(rotation());

            if (primHasRotateAxes() || rotateOrientation() != nullQuat)
                setRotateOrientation(rotateOrientation(), MSpace::kTransform, false);
        } else if (primHasTransform()) {
            auto transformIt = std::find(m_orderedOps.begin(), m_orderedOps.end(), kTransform);
            if (transformIt != m_orderedOps.end()) {
                internal_pushMatrix(
                    asMatrix(), m_xformops[std::distance(m_orderedOps.begin(), transformIt)]);
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
