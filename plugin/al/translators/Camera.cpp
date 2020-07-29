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

#include "Camera.h"

#include "AL/usdmaya/fileio/AnimationTranslator.h"
#include "AL/usdmaya/fileio/translators/DgNodeTranslator.h"
#include "AL/usdmaya/utils/DgNodeHelper.h"

#include <pxr/usd/usdGeom/camera.h>

#include <maya/M3dView.h>
#include <maya/MDagPath.h>
#include <maya/MDistance.h>
#include <maya/MFileIO.h>
#include <maya/MFnCamera.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MNodeClass.h>
#include <maya/MTime.h>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

AL_USDMAYA_DEFINE_TRANSLATOR(Camera, UsdGeomCamera)

//----------------------------------------------------------------------------------------------------------------------
MObject Camera::m_orthographic;
MObject Camera::m_horizontalFilmAperture;
MObject Camera::m_verticalFilmAperture;
MObject Camera::m_horizontalFilmApertureOffset;
MObject Camera::m_verticalFilmApertureOffset;
MObject Camera::m_focalLength;
MObject Camera::m_nearDistance;
MObject Camera::m_farDistance;
MObject Camera::m_fstop;
MObject Camera::m_focusDistance;
MObject Camera::m_lensSqueezeRatio;

//----------------------------------------------------------------------------------------------------------------------
MStatus Camera::initialize()
{
    MNodeClass nodeClass("camera");
    m_orthographic = nodeClass.attribute("o");
    m_horizontalFilmAperture = nodeClass.attribute("hfa");
    m_verticalFilmAperture = nodeClass.attribute("vfa");
    m_horizontalFilmApertureOffset = nodeClass.attribute("hfo");
    m_verticalFilmApertureOffset = nodeClass.attribute("vfo");
    m_focalLength = nodeClass.attribute("fl");
    m_nearDistance = nodeClass.attribute("ncp");
    m_farDistance = nodeClass.attribute("fcp");
    m_fstop = nodeClass.attribute("fs");
    m_focusDistance = nodeClass.attribute("fd");
    m_lensSqueezeRatio = nodeClass.attribute("lsr");
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
void Camera::checkCurrentCameras(MObject cameraNode)
{
    MSelectionList sl;
    sl.add("perspShape");
    MDagPath path;
    sl.getDagPath(0, path);
    M3dView  view;
    uint32_t nviews = M3dView::numberOf3dViews();
    for (uint32_t i = 0; i < nviews; ++i) {
        if (M3dView::get3dView(i, view)) {
            MDagPath camera;
            if (view.getCamera(camera)) {
                if (camera.node() == cameraNode) {
                    if (!view.setCamera(path)) {
                        MGlobal::displayError("Cannot change the camera that is being deleted. "
                                              "Maya will probably crash in a sec!");
                    }
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Camera::updateAttributes(MObject to, const UsdPrim& prim)
{
    UsdGeomCamera     usdCamera(prim);
    const char* const errorString = "CameraTranslator: error setting maya camera parameters";
    const float       mm_to_inches = 0.0393701f;
    UsdTimeCode       timeCode = UsdTimeCode::EarliestTime();
    bool              forceDefaultRead = false;
    if (context() && context()->getForceDefaultRead()) {
        timeCode = UsdTimeCode::Default();
        forceDefaultRead = true;
    }

    TfToken projection;
    usdCamera.GetProjectionAttr().Get(&projection, timeCode);
    bool isOrthographic = (projection == UsdGeomTokens->orthographic);
    AL_MAYA_CHECK_ERROR(DgNodeTranslator::setBool(to, m_orthographic, isOrthographic), errorString);

    NewNodesCollector collector { context(), prim };

    // Horizontal film aperture
    auto horizontalApertureAttr = usdCamera.GetHorizontalApertureAttr();
    if (!horizontalApertureAttr.GetNumTimeSamples() || forceDefaultRead) {
        float horizontalAperture;
        horizontalApertureAttr.Get(&horizontalAperture, timeCode);
        AL_MAYA_CHECK_ERROR(
            DgNodeTranslator::setDouble(
                to, m_horizontalFilmAperture, mm_to_inches * horizontalAperture),
            errorString);
    } else {
        DgNodeTranslator::setFloatAttrAnim(
            to,
            m_horizontalFilmAperture,
            horizontalApertureAttr,
            mm_to_inches,
            collector.nodeContainerPtr());
    }

    // Vertical film aperture
    auto verticalApertureAttr = usdCamera.GetVerticalApertureAttr();
    if (!verticalApertureAttr.GetNumTimeSamples() || forceDefaultRead) {
        float verticalAperture;
        verticalApertureAttr.Get(&verticalAperture, timeCode);
        AL_MAYA_CHECK_ERROR(
            DgNodeTranslator::setDouble(
                to, m_verticalFilmAperture, mm_to_inches * verticalAperture),
            errorString);
    } else {
        DgNodeTranslator::setFloatAttrAnim(
            to,
            m_verticalFilmAperture,
            verticalApertureAttr,
            mm_to_inches,
            collector.nodeContainerPtr());
    }

    // Horizontal film aperture offset
    auto horizontalApertureOffsetAttr = usdCamera.GetHorizontalApertureOffsetAttr();
    if (!horizontalApertureOffsetAttr.GetNumTimeSamples() || forceDefaultRead) {
        float horizontalApertureOffset;
        horizontalApertureOffsetAttr.Get(&horizontalApertureOffset, timeCode);
        AL_MAYA_CHECK_ERROR(
            DgNodeTranslator::setDouble(
                to, m_horizontalFilmApertureOffset, mm_to_inches * horizontalApertureOffset),
            errorString);
    } else {
        DgNodeTranslator::setFloatAttrAnim(
            to,
            m_horizontalFilmApertureOffset,
            horizontalApertureOffsetAttr,
            mm_to_inches,
            collector.nodeContainerPtr());
    }

    // Vertical film aperture offset
    auto verticalApertureOffsetAttr = usdCamera.GetVerticalApertureOffsetAttr();
    if (!verticalApertureOffsetAttr.GetNumTimeSamples() || forceDefaultRead) {
        float verticalApertureOffset;
        verticalApertureOffsetAttr.Get(&verticalApertureOffset);
        AL_MAYA_CHECK_ERROR(
            DgNodeTranslator::setDouble(
                to, m_verticalFilmApertureOffset, mm_to_inches * verticalApertureOffset),
            errorString);
    } else {
        DgNodeTranslator::setFloatAttrAnim(
            to,
            m_verticalFilmApertureOffset,
            verticalApertureOffsetAttr,
            mm_to_inches,
            collector.nodeContainerPtr());
    }

    // Focal length
    auto focalLengthAttr = usdCamera.GetFocalLengthAttr();
    if (!focalLengthAttr.GetNumTimeSamples() || forceDefaultRead) {
        float focalLength;
        focalLengthAttr.Get(&focalLength, timeCode);
        AL_MAYA_CHECK_ERROR(
            DgNodeTranslator::setDouble(to, m_focalLength, focalLength), errorString);
    } else {
        DgNodeTranslator::setFloatAttrAnim(
            to, m_focalLength, focalLengthAttr, 1.0f, collector.nodeContainerPtr());
    }

    // Near/far clip planes
    auto clippingRangeAttr = usdCamera.GetClippingRangeAttr();
    if (!clippingRangeAttr.GetNumTimeSamples() || forceDefaultRead) {
        GfVec2f clippingRange;
        clippingRangeAttr.Get(&clippingRange, timeCode);
        AL_MAYA_CHECK_ERROR(
            DgNodeTranslator::setDistance(
                to, m_nearDistance, MDistance(clippingRange[0], MDistance::kCentimeters)),
            errorString);
        AL_MAYA_CHECK_ERROR(
            DgNodeTranslator::setDistance(
                to, m_farDistance, MDistance(clippingRange[1], MDistance::kCentimeters)),
            errorString);
    } else {
        DgNodeTranslator::setClippingRangeAttrAnim(
            to, m_nearDistance, m_farDistance, clippingRangeAttr, collector.nodeContainerPtr());
    }

    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Camera::update(const UsdPrim& prim)
{
    MObjectHandle handle;
    if (context() && !context()->getMObject(prim, handle, MFn::kCamera)) {
        MGlobal::displayError("unable to locate camera node");
        return MS::kFailure;
    }
    MObject to = handle.object();
    return updateAttributes(to, prim);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Camera::import(const UsdPrim& prim, MObject& parent, MObject& createdObj)
{
    const char* const errorString = "CameraTranslator: error setting maya camera parameters";
    UsdGeomCamera     usdCamera(prim);

    MStatus    status;
    MFnDagNode fn;
    MString    name(prim.GetName().GetText() + MString("Shape"));
    MObject    to = fn.create("camera", name, parent, &status);
    createdObj = to;
    TranslatorContextPtr ctx = context();
    NewNodesCollector    collector { ctx, prim };
    UsdTimeCode          timeCode = UsdTimeCode::EarliestTime();
    bool                 forceDefaultRead = false;
    if (ctx) {
        ctx->insertItem(prim, to);
        if (ctx->getForceDefaultRead()) {
            timeCode = UsdTimeCode::Default();
            forceDefaultRead = true;
        }
    }

    // F-Stop
    if (!DgNodeTranslator::setFloatAttrAnim(
            to, m_fstop, usdCamera.GetFStopAttr(), 1.0f, collector.nodeContainerPtr())) {
        float fstop;
        usdCamera.GetFStopAttr().Get(&fstop, timeCode);
        AL_MAYA_CHECK_ERROR(DgNodeTranslator::setDouble(to, m_fstop, fstop), errorString);
    }

    // Focus distance
    if (usdCamera.GetFocusDistanceAttr().GetNumTimeSamples() && !forceDefaultRead) {
        // TODO: What unit here?
        MDistance one(1.0, MDistance::kCentimeters);
        double    conversionFactor = one.as(MDistance::kCentimeters);
        DgNodeTranslator::setFloatAttrAnim(
            to,
            m_focusDistance,
            usdCamera.GetFocusDistanceAttr(),
            conversionFactor,
            collector.nodeContainerPtr());
    } else {
        float focusDistance;
        usdCamera.GetFocusDistanceAttr().Get(&focusDistance, timeCode);
        AL_MAYA_CHECK_ERROR(
            DgNodeTranslator::setDistance(
                to, m_focusDistance, MDistance(focusDistance, MDistance::kCentimeters)),
            errorString);
    }
    return updateAttributes(to, prim);
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim Camera::exportObject(
    UsdStageRefPtr        stage,
    MDagPath              dagPath,
    const SdfPath&        usdPath,
    const ExporterParams& params)
{
    UsdGeomCamera usdCamera = UsdGeomCamera::Define(stage, usdPath);
    UsdPrim       prim = usdCamera.GetPrim();
    writePrim(prim, dagPath, params);
    return prim;
}

//----------------------------------------------------------------------------------------------------------------------
void Camera::writePrim(UsdPrim& prim, MDagPath dagPath, const ExporterParams& params)
{
    UsdGeomCamera usdCamera(prim);

    MStatus   status;
    MFnCamera fnCamera(dagPath, &status);
    AL_MAYA_CHECK_ERROR2(status, "Export: Failed to create cast into a MFnCamera.");

    MObject cameraObject = fnCamera.object(&status);
    AL_MAYA_CHECK_ERROR2(status, "Export: Failed to retrieve object.");

    const char* const errorString = "CameraTranslator: error getting maya camera parameters";
    const float       inches_to_mm = 1.0f / 0.0393701f;

    // set orthographic
    bool      isOrthographic;
    double    squeezeRatio;
    double    horizontalAperture;
    double    verticalAperture;
    double    horizontalApertureOffset;
    double    verticalApertureOffset;
    double    focalLength;
    double    fstop;
    MDistance nearDistance;
    MDistance farDistance;
    MDistance focusDistance;

    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getBool(cameraObject, m_orthographic, isOrthographic),
        errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getDouble(
            cameraObject, m_horizontalFilmAperture, horizontalAperture),
        errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getDouble(
            cameraObject, m_verticalFilmAperture, verticalAperture),
        errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getDouble(
            cameraObject, m_horizontalFilmApertureOffset, horizontalApertureOffset),
        errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getDouble(
            cameraObject, m_verticalFilmApertureOffset, verticalApertureOffset),
        errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getDouble(cameraObject, m_focalLength, focalLength),
        errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getDistance(cameraObject, m_nearDistance, nearDistance),
        errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getDistance(cameraObject, m_farDistance, farDistance),
        errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getDouble(cameraObject, m_fstop, fstop), errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getDistance(cameraObject, m_focusDistance, focusDistance),
        errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getDouble(cameraObject, m_lensSqueezeRatio, squeezeRatio),
        errorString);

    usdCamera.GetProjectionAttr().Set(
        isOrthographic ? UsdGeomTokens->orthographic : UsdGeomTokens->perspective);
    usdCamera.GetHorizontalApertureAttr().Set(
        float(horizontalAperture * squeezeRatio * inches_to_mm));
    usdCamera.GetVerticalApertureAttr().Set(float(verticalAperture * squeezeRatio * inches_to_mm));
    usdCamera.GetHorizontalApertureOffsetAttr().Set(
        float(horizontalApertureOffset * squeezeRatio * inches_to_mm));
    usdCamera.GetVerticalApertureOffsetAttr().Set(
        float(verticalApertureOffset * squeezeRatio * inches_to_mm));
    usdCamera.GetFocalLengthAttr().Set(float(focalLength));
    usdCamera.GetClippingRangeAttr().Set(
        GfVec2f(nearDistance.as(MDistance::kCentimeters), farDistance.as(MDistance::kCentimeters)));
    usdCamera.GetFStopAttr().Set(float(fstop));
    usdCamera.GetFocusDistanceAttr().Set(float(focusDistance.as(MDistance::kCentimeters)));

    AnimationTranslator* animTranslator = params.m_animTranslator;
    if (animTranslator) {
        //
        animTranslator->addPlug(
            MPlug(cameraObject, m_horizontalFilmAperture),
            usdCamera.GetHorizontalApertureAttr(),
            squeezeRatio * inches_to_mm,
            true);
        animTranslator->addPlug(
            MPlug(cameraObject, m_verticalFilmAperture),
            usdCamera.GetVerticalApertureAttr(),
            squeezeRatio * inches_to_mm,
            true);
        animTranslator->addPlug(
            MPlug(cameraObject, m_horizontalFilmApertureOffset),
            usdCamera.GetHorizontalApertureOffsetAttr(),
            squeezeRatio * inches_to_mm,
            true);
        animTranslator->addPlug(
            MPlug(cameraObject, m_verticalFilmApertureOffset),
            usdCamera.GetVerticalApertureOffsetAttr(),
            squeezeRatio * inches_to_mm,
            true);
        animTranslator->addPlug(
            MPlug(cameraObject, m_focalLength), usdCamera.GetFocalLengthAttr(), true);
        animTranslator->addPlug(MPlug(cameraObject, m_fstop), usdCamera.GetFStopAttr(), true);
        animTranslator->addPlug(
            MPlug(cameraObject, m_focusDistance), usdCamera.GetFocusDistanceAttr(), true);
        // near-far clipping range are special: these two Maya attributes need to be mapped to one
        // Usd attribute
        animTranslator->addMultiPlugs(
            { MPlug(cameraObject, m_nearDistance), MPlug(cameraObject, m_farDistance) },
            usdCamera.GetClippingRangeAttr(),
            true);
    }
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Camera::tearDown(const SdfPath& path)
{
    MObjectHandle obj;
    context()->getMObject(path, obj, MFn::kCamera);
    checkCurrentCameras(obj.object());
    context()->removeItems(path);
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
