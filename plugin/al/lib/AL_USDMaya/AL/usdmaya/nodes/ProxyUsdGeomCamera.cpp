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
#include "ProxyUsdGeomCamera.h"

#include "AL/usdmaya/TypeIDs.h"

#include <AL/maya/utils/MayaHelperMacros.h>
#include <mayaUsd/nodes/stageData.h>

#include <pxr/usd/usd/api.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MDistance.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnPluginData.h>
#include <maya/MGlobal.h>
#include <maya/MPlugArray.h>
#include <maya/MTime.h>

namespace AL {
namespace usdmaya {
namespace nodes {

#define TO_MAYA_ENUM(e) static_cast<int16_t>(e)

AL_MAYA_DEFINE_NODE(ProxyUsdGeomCamera, AL_USDMAYA_USDGEOMCAMERAPROXY, AL_usd);

MObject ProxyUsdGeomCamera::m_path = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_stage = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_time = MObject::kNullObj;

MObject ProxyUsdGeomCamera::m_clippingRange = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_focalLength = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_focusDistance = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_fStop = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_cameraApertureMm = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_cameraApertureInch = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_filmOffset = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_horizontalAperture = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_horizontalApertureOffset = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_projection = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_shutterClose = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_shutterOpen = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_stereoRole = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_verticalAperture = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_verticalApertureOffset = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_nearClipPlane = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_farClipPlane = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_orthographic = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_horizontalFilmAperture = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_horizontalFilmOffset = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_verticalFilmAperture = MObject::kNullObj;
MObject ProxyUsdGeomCamera::m_verticalFilmOffset = MObject::kNullObj;

UsdGeomCamera ProxyUsdGeomCamera::getCamera() const
{
    MStatus status;
    MPlug   stagePlug(thisMObject(), m_stage);
    MObject stageObject;
    status = stagePlug.getValue(stageObject);
    AL_MAYA_CHECK_ERROR2(status, "Failed to get 'stage' attr");

    if (status == MStatus::kSuccess) {
        // Pull in stage data
        MFnPluginData fnData(stageObject);

        auto* stageData = static_cast<MayaUsdStageData*>(fnData.data());
        if (stageData != nullptr) {
            // Get prim path
            MPlug   pathPlug(thisMObject(), m_path);
            MString path;
            status = pathPlug.getValue(path);
            AL_MAYA_CHECK_ERROR2(status, "Failed to get 'path' attr");

            return UsdGeomCamera(stageData->stage->GetPrimAtPath(SdfPath(path.asChar())));
        }
    }
    return UsdGeomCamera();
}

UsdTimeCode ProxyUsdGeomCamera::getTime() const
{
    MTime time;
    MPlug(thisMObject(), m_time).getValue(time);
    return UsdTimeCode(time.as(MTime::uiUnit()));
}

// USD -> Maya
bool ProxyUsdGeomCamera::getInternalValue(const MPlug& plug, MDataHandle& dataHandle)
{
    const float mm_to_inches = 0.0393701f;

    MStatus status;
    bool    handledAttribute = false;

    UsdTimeCode usdTime(getTime());

    const UsdGeomCamera camera(getCamera());
    if (!camera)
        return false;

    if (plug == m_nearClipPlane) {
        const UsdAttribute attr(camera.CreateClippingRangeAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        GfVec2f value;
        if (attr.Get(&value, usdTime)) {
            dataHandle.setMDistance(MDistance(value[0], MDistance::kCentimeters));
            handledAttribute = true;
        }
    } else if (plug == m_farClipPlane) {
        const UsdAttribute attr(camera.GetClippingRangeAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        GfVec2f value;
        if (attr.Get(&value, usdTime)) {
            dataHandle.setMDistance(MDistance(value[1], MDistance::kCentimeters));
            handledAttribute = true;
        }
    } else if (plug == m_focalLength) {
        const UsdAttribute attr(camera.GetFocalLengthAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        float value;
        if (attr.Get(&value, usdTime)) {
            dataHandle.setFloat(value);
            handledAttribute = true;
        }
    } else if (plug == m_focusDistance) {
        const UsdAttribute attr(camera.GetFocusDistanceAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        float value;
        if (attr.Get(&value, usdTime)) {
            dataHandle.setMDistance(MDistance(value, MDistance::kCentimeters));
            handledAttribute = true;
        }
    } else if (plug == m_fStop) {
        const UsdAttribute attr(camera.GetFStopAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        float value;
        if (attr.Get(&value, usdTime)) {
            dataHandle.setFloat(value);
            handledAttribute = true;
        }
    } else if (plug == m_horizontalAperture) {
        const UsdAttribute attr(camera.GetHorizontalApertureAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        float value;
        if (attr.Get(&value, usdTime)) {
            dataHandle.setFloat(value);
            handledAttribute = true;
        }
    } else if (plug == m_horizontalFilmAperture) {
        const UsdAttribute attr(camera.GetHorizontalApertureAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        float value;
        if (attr.Get(&value, usdTime)) {
            dataHandle.setDouble((double)(mm_to_inches * value));
            handledAttribute = true;
        }
    } else if (plug == m_horizontalApertureOffset) {
        const UsdAttribute attr(camera.GetHorizontalApertureOffsetAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        float value;
        if (attr.Get(&value, usdTime)) {
            dataHandle.setFloat(value);
            handledAttribute = true;
        }
    } else if (plug == m_horizontalFilmOffset) {
        const UsdAttribute attr(camera.GetHorizontalApertureOffsetAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        float value;
        if (attr.Get(&value, usdTime)) {
            dataHandle.setDouble((double)(mm_to_inches * value));
            handledAttribute = true;
        }
    } else if (plug == m_projection) {
        const UsdAttribute attr(camera.GetProjectionAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        TfToken value;
        if (attr.Get(&value, usdTime)) {
            if (value == UsdGeomTokens->perspective) {
                dataHandle.setShort(TO_MAYA_ENUM(Projection::Perspective));
                handledAttribute = true;
            } else if (value == UsdGeomTokens->orthographic) {
                dataHandle.setShort(TO_MAYA_ENUM(Projection::Orthographic));
                handledAttribute = true;
            }
        }
    } else if (plug == m_orthographic) {
        const UsdAttribute attr(camera.GetProjectionAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        TfToken value;
        if (attr.Get(&value, usdTime)) {
            const bool orthographic = value == UsdGeomTokens->orthographic;
            dataHandle.setBool(orthographic);
            handledAttribute = true;
        }
    } else if (plug == m_shutterClose) {
        const UsdAttribute attr(camera.GetShutterCloseAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        double value;
        if (attr.Get(&value, usdTime)) {
            dataHandle.setDouble(value);
            handledAttribute = true;
        }
    } else if (plug == m_shutterOpen) {
        const UsdAttribute attr(camera.GetShutterOpenAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        double value;
        if (attr.Get(&value, usdTime)) {
            dataHandle.setDouble(value);
            handledAttribute = true;
        }
    } else if (plug == m_stereoRole) {
        const UsdAttribute attr(camera.GetStereoRoleAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        TfToken value;
        if (attr.Get(&value, usdTime)) {
            if (value == UsdGeomTokens->mono) {
                dataHandle.setShort(TO_MAYA_ENUM(StereoRole::Mono));
                handledAttribute = true;
            } else if (value == UsdGeomTokens->left) {
                dataHandle.setShort(TO_MAYA_ENUM(StereoRole::Left));
                handledAttribute = true;
            } else if (value == UsdGeomTokens->right) {
                dataHandle.setShort(TO_MAYA_ENUM(StereoRole::Right));
                handledAttribute = true;
            }
        }
    } else if (plug == m_verticalAperture) {
        const UsdAttribute attr(camera.GetVerticalApertureAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        float value;
        if (attr.Get(&value, usdTime)) {
            dataHandle.setFloat(value);
            handledAttribute = true;
        }
    } else if (plug == m_verticalFilmAperture) {
        const UsdAttribute attr(camera.GetVerticalApertureAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        float value;
        if (attr.Get(&value, usdTime)) {
            dataHandle.setDouble((double)(mm_to_inches * value));
            handledAttribute = true;
        }
    } else if (plug == m_verticalApertureOffset) {
        const UsdAttribute attr(camera.GetVerticalApertureOffsetAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        float value;
        if (attr.Get(&value, usdTime)) {
            dataHandle.setFloat(value);
            handledAttribute = true;
        }
    } else if (plug == m_verticalFilmOffset) {
        const UsdAttribute attr(camera.GetVerticalApertureOffsetAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        float value;
        if (attr.Get(&value, usdTime)) {
            dataHandle.setDouble((double)(mm_to_inches * value));
            handledAttribute = true;
        }
    }

    return handledAttribute;
}

// Maya -> USD
bool ProxyUsdGeomCamera::setInternalValue(const MPlug& plug, const MDataHandle& dataHandle)
{
    const float inches_to_mm = 1.0f / 0.0393701f;

    bool    handledAttribute = false;
    MStatus status;

    UsdTimeCode usdTime(getTime());

    const UsdGeomCamera camera(getCamera());
    if (!camera)
        return false;

    if (plug == m_nearClipPlane) {
        const UsdAttribute attr(camera.CreateClippingRangeAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        GfVec2f usdValue;
        if (!attr.Get(&usdValue, usdTime))
            usdValue = GfVec2f(0.1f, 10000.0f);

        usdValue[0] = dataHandle.asDistance().value();
        handledAttribute = attr.Set(VtValue(usdValue), usdTime);
    } else if (plug == m_farClipPlane) {
        const UsdAttribute attr(camera.CreateClippingRangeAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        GfVec2f usdValue;
        if (!attr.Get(&usdValue, usdTime))
            usdValue = GfVec2f(0.1f, 10000.0f);

        usdValue[1] = dataHandle.asDistance().value();
        handledAttribute = attr.Set(VtValue(usdValue), usdTime);
    } else if (plug == m_focalLength) {
        const UsdAttribute attr(camera.CreateFocalLengthAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        handledAttribute = attr.Set(dataHandle.asFloat(), usdTime);
    } else if (plug == m_focusDistance) {
        const UsdAttribute attr(camera.CreateFocusDistanceAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        handledAttribute = attr.Set((float)dataHandle.asDistance().value(), usdTime);
    } else if (plug == m_fStop) {
        const UsdAttribute attr(camera.CreateFStopAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        handledAttribute = attr.Set(dataHandle.asFloat(), usdTime);
    } else if (plug == m_horizontalAperture) {
        const UsdAttribute attr(camera.CreateHorizontalApertureAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        handledAttribute = attr.Set(dataHandle.asFloat(), usdTime);
    } else if (plug == m_horizontalFilmAperture) {
        const UsdAttribute attr(camera.CreateHorizontalApertureAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        handledAttribute = attr.Set((float)(inches_to_mm * dataHandle.asDouble()), usdTime);
    } else if (plug == m_horizontalApertureOffset) {
        const UsdAttribute attr(camera.CreateHorizontalApertureOffsetAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        handledAttribute = attr.Set(dataHandle.asFloat(), usdTime);
    } else if (plug == m_horizontalFilmOffset) {
        const UsdAttribute attr(camera.CreateHorizontalApertureOffsetAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        handledAttribute = attr.Set((float)(inches_to_mm * dataHandle.asDouble()), usdTime);
    } else if (plug == m_projection) {
        const UsdAttribute attr(camera.CreateProjectionAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        Projection mode = static_cast<Projection>(dataHandle.asShort());
        switch (mode) {
        case (Projection::Perspective):
            handledAttribute = attr.Set(UsdGeomTokens->perspective, usdTime);
            break;
        case (Projection::Orthographic):
            handledAttribute = attr.Set(UsdGeomTokens->orthographic, usdTime);
            break;
        }
    } else if (plug == m_orthographic) {
        const UsdAttribute attr(camera.CreateProjectionAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        const bool orthographic = dataHandle.asBool();
        if (orthographic)
            handledAttribute = attr.Set(UsdGeomTokens->orthographic, usdTime);
        else
            handledAttribute = attr.Set(UsdGeomTokens->perspective, usdTime);
    } else if (plug == m_shutterClose) {
        const UsdAttribute attr(camera.CreateShutterCloseAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        handledAttribute = attr.Set(dataHandle.asDouble(), usdTime);
    } else if (plug == m_shutterOpen) {
        const UsdAttribute attr(camera.CreateShutterOpenAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        handledAttribute = attr.Set(dataHandle.asDouble(), usdTime);
    } else if (plug == m_stereoRole) {
        const UsdAttribute attr(camera.CreateStereoRoleAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        StereoRole mode = static_cast<StereoRole>(dataHandle.asShort());
        switch (mode) {
        case (StereoRole::Mono): handledAttribute = attr.Set(UsdGeomTokens->mono, usdTime); break;
        case (StereoRole::Left): handledAttribute = attr.Set(UsdGeomTokens->left, usdTime); break;
        case (StereoRole::Right): handledAttribute = attr.Set(UsdGeomTokens->right, usdTime); break;
        }
    } else if (plug == m_verticalAperture) {
        const UsdAttribute attr(camera.CreateVerticalApertureAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        handledAttribute = attr.Set(dataHandle.asFloat(), usdTime);
    } else if (plug == m_verticalFilmAperture) {
        const UsdAttribute attr(camera.CreateVerticalApertureAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        handledAttribute = attr.Set((float)(inches_to_mm * dataHandle.asDouble()), usdTime);
    } else if (plug == m_verticalApertureOffset) {
        const UsdAttribute attr(camera.CreateVerticalApertureOffsetAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        handledAttribute = attr.Set(dataHandle.asFloat(), usdTime);
    } else if (plug == m_verticalFilmOffset) {
        const UsdAttribute attr(camera.CreateVerticalApertureOffsetAttr());
        if (attr.GetVariability() != SdfVariability::SdfVariabilityVarying)
            usdTime = UsdTimeCode::Default();
        handledAttribute = attr.Set((float)(inches_to_mm * dataHandle.asDouble()), usdTime);
    }

    return handledAttribute;
}

MStatus ProxyUsdGeomCamera::initialise()
{
    setNodeType(kTypeName);

    const uint32_t defaultFlags = kReadable | kWritable | kConnectable | kInternal;
    const uint32_t readOnlyFlags = kReadable | kConnectable | kInternal;

    addFrame("USD Prim");

    m_path = addStringAttr("path", "p", "", kStorable | kWritable);

    m_stage = addDataAttr(
        "stage",
        "s",
        MayaUsdStageData::mayaTypeId,
        kWritable | kHidden | kConnectable,
        MFnNumericAttribute::kReset);

    m_time = addTimeAttr(
        "time",
        "tm",
        MTime(0.0),
        kCached | kConnectable | kReadable | kWritable | kHidden | kStorable | kAffectsAppearance);

    // Hidden attributes for connecting to the maya camera attributes.

    m_orthographic = addBoolAttr("orthographic", "o", false, readOnlyFlags);
    attributeAffects(m_path, m_orthographic);

    addFrame("Camera Attributes");

    const char* const projectionKeys[] = { "Perspective", "Orthographic", nullptr };
    const int16_t     projectionValues[]
        = { TO_MAYA_ENUM(Projection::Perspective), TO_MAYA_ENUM(Projection::Orthographic) };
    m_projection
        = addEnumAttr("projection", "pron", defaultFlags, projectionKeys, projectionValues);
    MFnEnumAttribute(m_projection).setDefault(TO_MAYA_ENUM(Projection::Perspective));
    attributeAffects(m_path, m_projection);

    const char* const stereoRoleKeys[] = { "Mono", "Left", "Right", nullptr };
    const int16_t     stereoRoleValues[] = { TO_MAYA_ENUM(StereoRole::Mono),
                                         TO_MAYA_ENUM(StereoRole::Left),
                                         TO_MAYA_ENUM(StereoRole::Right) };
    m_stereoRole
        = addEnumAttr("stereoRole", "stee", defaultFlags, stereoRoleKeys, stereoRoleValues);
    MFnEnumAttribute(m_stereoRole).setDefault(TO_MAYA_ENUM(StereoRole::Mono));
    attributeAffects(m_path, m_stereoRole);

    m_focalLength = addFloatAttr("focalLength", "fl", 50.0f, defaultFlags);
    MFnNumericAttribute(m_focalLength).setMin(2.5f);
    MFnNumericAttribute(m_focalLength).setMax(100000.0f);
    MFnNumericAttribute(m_focalLength).setSoftMax(400.0f);
    attributeAffects(m_path, m_focalLength);

    m_nearClipPlane = addDistanceAttr(
        "nearClipPlane", "ncp", MDistance(0.1, MDistance::kCentimeters), defaultFlags | kHidden);
    MFnNumericAttribute(m_nearClipPlane).setMin(0.001);
    attributeAffects(m_path, m_nearClipPlane);

    m_farClipPlane = addDistanceAttr(
        "farClipPlane", "fcp", MDistance(10000.0, MDistance::kCentimeters), defaultFlags | kHidden);
    MFnNumericAttribute(m_farClipPlane).setMin(0.001);
    attributeAffects(m_path, m_farClipPlane);

    m_clippingRange
        = addCompoundAttr("clippingRange", "cr", defaultFlags, { m_nearClipPlane, m_farClipPlane });
    MFnAttribute(m_clippingRange).setNiceNameOverride("Clipping Range");

    addFrame("Film Back");

    m_horizontalFilmAperture
        = addDoubleAttr("horizontalFilmAperture", "hfa", 0.0, defaultFlags | kHidden);
    MFnAttribute(m_horizontalFilmAperture).setNiceNameOverride("Horizontal Aperture (inch)");
    MFnNumericAttribute(m_horizontalFilmAperture).setMin(0.001f);
    attributeAffects(m_path, m_horizontalFilmAperture);

    m_verticalFilmAperture
        = addDoubleAttr("verticalFilmAperture", "vfa", 0.0, defaultFlags | kHidden);
    MFnAttribute(m_verticalFilmAperture).setNiceNameOverride("Horizontal Aperture Offset (inch)");
    MFnNumericAttribute(m_verticalFilmAperture).setMin(0.001f);
    attributeAffects(m_path, m_verticalFilmAperture);

    m_cameraApertureInch = addCompoundAttr(
        "cameraAperture",
        "cap",
        defaultFlags,
        { m_horizontalFilmAperture, m_verticalFilmAperture });
    MFnAttribute(m_cameraApertureInch).setNiceNameOverride("Camera Aperture (inch)");

    m_horizontalAperture = addFloatAttr("horizontalAperture", "ha", 36.0f, readOnlyFlags | kHidden);
    MFnAttribute(m_horizontalAperture).setNiceNameOverride("Horizontal Aperture (mm)");
    MFnNumericAttribute(m_horizontalAperture).setMin(0.001f);
    attributeAffects(m_path, m_horizontalAperture);

    m_verticalAperture = addFloatAttr("verticalAperture", "va", 24.0f, readOnlyFlags | kHidden);
    MFnAttribute(m_verticalAperture).setNiceNameOverride("Vertical Aperture (mm)");
    MFnNumericAttribute(m_verticalAperture).setMin(0.001f);
    attributeAffects(m_path, m_verticalAperture);

    m_cameraApertureMm = addCompoundAttr(
        "cameraApertureMm", "capm", defaultFlags, { m_horizontalAperture, m_verticalAperture });
    MFnAttribute(m_cameraApertureMm).setNiceNameOverride("Camera Aperture (mm)");

    m_horizontalFilmOffset
        = addDoubleAttr("horizontalFilmOffset", "hfo", 0.0, defaultFlags | kHidden);
    MFnAttribute(m_horizontalFilmOffset).setNiceNameOverride("Vertical Aperture (inch)");
    attributeAffects(m_path, m_horizontalFilmOffset);

    m_verticalFilmOffset = addDoubleAttr("verticalFilmOffset", "vfo", 0.0, defaultFlags | kHidden);
    MFnAttribute(m_verticalFilmOffset).setNiceNameOverride("Vertical Aperture Offset (inch)");
    attributeAffects(m_path, m_verticalFilmOffset);

    m_filmOffset = addCompoundAttr(
        "filmOffset", "fio", defaultFlags, { m_horizontalFilmOffset, m_verticalFilmOffset });
    MFnAttribute(m_filmOffset).setNiceNameOverride("Film Offset (inch)");

    m_horizontalApertureOffset
        = addFloatAttr("horizontalApertureOffset", "hao", 0.0f, readOnlyFlags | kHidden);
    MFnAttribute(m_horizontalApertureOffset).setNiceNameOverride("Horizontal Aperture Offset (mm)");
    attributeAffects(m_path, m_horizontalApertureOffset);

    m_verticalApertureOffset
        = addFloatAttr("verticalApertureOffset", "vao", 0.0f, readOnlyFlags | kHidden);
    MFnAttribute(m_verticalApertureOffset).setNiceNameOverride("Vertical Aperture Offset (mm)");
    attributeAffects(m_path, m_verticalApertureOffset);

    addFrame("Depth of Field");

    m_focusDistance = addDistanceAttr(
        "focusDistance", "fd", MDistance(5.0, MDistance::kCentimeters), defaultFlags);
    MFnNumericAttribute(m_focusDistance).setMin(0.0);
    attributeAffects(m_path, m_focusDistance);

    m_fStop = addFloatAttr("fStop", "fs", 5.6f, defaultFlags);
    MFnNumericAttribute(m_fStop).setMin(1.0f);
    MFnNumericAttribute(m_fStop).setMax(64.0f);
    attributeAffects(m_path, m_fStop);

    addFrame("Motion Blur");

    m_shutterOpen = addDoubleAttr("shutterOpen", "shun", 0.0, defaultFlags);
    attributeAffects(m_path, m_shutterOpen);

    m_shutterClose = addDoubleAttr("shutterClose", "shue", 0.0, defaultFlags);
    attributeAffects(m_path, m_shutterClose);

    // Translation Attribute Connections

    attributeAffects(m_projection, m_orthographic);
    attributeAffects(m_orthographic, m_projection);

    attributeAffects(m_horizontalAperture, m_horizontalFilmAperture);
    attributeAffects(m_horizontalFilmAperture, m_horizontalAperture);
    attributeAffects(m_horizontalApertureOffset, m_horizontalFilmOffset);
    attributeAffects(m_horizontalFilmOffset, m_horizontalApertureOffset);

    attributeAffects(m_verticalAperture, m_verticalFilmAperture);
    attributeAffects(m_verticalFilmAperture, m_verticalAperture);
    attributeAffects(m_verticalApertureOffset, m_verticalFilmOffset);
    attributeAffects(m_verticalFilmOffset, m_verticalApertureOffset);

    generateAETemplate();

    return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
