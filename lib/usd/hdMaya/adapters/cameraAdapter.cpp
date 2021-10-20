//
// Copyright 2021 Autodesk
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
#include "cameraAdapter.h"

#include <hdMaya/adapters/adapterDebugCodes.h>
#include <hdMaya/adapters/adapterRegistry.h>
#include <hdMaya/adapters/mayaAttrs.h>

#include <pxr/base/gf/interval.h>
#include <pxr/imaging/hd/camera.h>

#include <maya/MDagMessage.h>
#include <maya/MFnCamera.h>
#include <maya/MNodeMessage.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaCameraAdapter, TfType::Bases<HdMayaShapeAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, camera)
{
    HdMayaAdapterRegistry::RegisterCameraAdapter(
        HdPrimTypeTokens->camera,
        [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> HdMayaCameraAdapterPtr {
            return HdMayaCameraAdapterPtr(new HdMayaCameraAdapter(delegate, dag));
        });
}

} // namespace

HdMayaCameraAdapter::HdMayaCameraAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
    : HdMayaShapeAdapter(delegate->GetPrimPath(dag, true), delegate, dag)
{
}

HdMayaCameraAdapter::~HdMayaCameraAdapter() { }

TfToken HdMayaCameraAdapter::CameraType() { return HdPrimTypeTokens->camera; }

bool HdMayaCameraAdapter::IsSupported() const
{
    return GetDelegate()->GetRenderIndex().IsSprimTypeSupported(CameraType());
}

void HdMayaCameraAdapter::Populate()
{
    if (_isPopulated) {
        return;
    }
    GetDelegate()->InsertSprim(CameraType(), GetID(), HdCamera::AllDirty);
    _isPopulated = true;
}

void HdMayaCameraAdapter::MarkDirty(HdDirtyBits dirtyBits)
{
    if (_isPopulated && dirtyBits != 0) {
#if PXR_VERSION < 2102
        if (dirtyBits & HdChangeTracker::DirtyTransform) {
            dirtyBits |= HdCamera::DirtyViewMatrix;
        }
#endif
        dirtyBits = dirtyBits & HdCamera::AllDirty;
        GetDelegate()->GetChangeTracker().MarkSprimDirty(GetID(), dirtyBits);
    }
}

void HdMayaCameraAdapter::CreateCallbacks()
{
    MStatus status;
    auto    dag = GetDagPath();
    auto    obj = dag.node();

    auto paramsChanged = MNodeMessage::addNodeDirtyCallback(
        obj,
        +[](MObject& obj, void* clientData) {
            auto* adapter = reinterpret_cast<HdMayaCameraAdapter*>(clientData);
            // Dirty everything rather than track complex param and fit to projection dependencies.
            adapter->MarkDirty(
                HdCamera::DirtyParams | HdCamera::DirtyProjMatrix | HdCamera::DirtyWindowPolicy);
        },
        reinterpret_cast<void*>(this),
        &status);
    if (status) {
        AddCallback(paramsChanged);
    }

    auto xformChanged = MDagMessage::addWorldMatrixModifiedCallback(
        dag,
        +[](MObject& transformNode, MDagMessage::MatrixModifiedFlags& modified, void* clientData) {
            auto* adapter = reinterpret_cast<HdMayaCameraAdapter*>(clientData);
#if PXR_VERSION < 2102
            adapter->MarkDirty(HdCamera::DirtyTransform | HdCamera::DirtyViewMatrix);
#else
            adapter->MarkDirty(HdCamera::DirtyTransform);
#endif
            adapter->InvalidateTransform();
        },
        reinterpret_cast<void*>(this),
        &status);
    if (status) {
        AddCallback(xformChanged);
    }

    // Skip over HdMayaShapeAdapter's CreateCallbacks
    HdMayaAdapter::CreateCallbacks();
}

void HdMayaCameraAdapter::RemovePrim()
{
    if (!_isPopulated) {
        return;
    }
    GetDelegate()->RemoveSprim(CameraType(), GetID());
    _isPopulated = false;
}

bool HdMayaCameraAdapter::HasType(const TfToken& typeId) const { return typeId == CameraType(); }

VtValue HdMayaCameraAdapter::Get(const TfToken& key) { return HdMayaShapeAdapter::Get(key); }

VtValue HdMayaCameraAdapter::GetCameraParamValue(const TfToken& paramName)
{
    constexpr double mayaInchToHydraCentimeter = 0.254;
    constexpr double mayaInchToHydraMillimeter = 0.0254;
    constexpr double mayaFocaLenToHydra = 0.01;

    MStatus status;

    auto convertFit = [&](const MFnCamera& camera) -> CameraUtilConformWindowPolicy {
        const auto mayaFit = camera.filmFit(&status);
        if (mayaFit == MFnCamera::kHorizontalFilmFit)
            return CameraUtilConformWindowPolicy::CameraUtilMatchHorizontally;
        if (mayaFit == MFnCamera::kVerticalFilmFit)
            return CameraUtilConformWindowPolicy::CameraUtilMatchVertically;

        const auto fitMatcher = camera.horizontalFilmAperture() > camera.verticalFilmAperture()
            ? MFnCamera::kOverscanFilmFit
            : MFnCamera::kFillFilmFit;
        return mayaFit == fitMatcher ? CameraUtilConformWindowPolicy::CameraUtilMatchHorizontally
                                     : CameraUtilConformWindowPolicy::CameraUtilMatchVertically;
    };

    auto apertureConvert = [&](const MFnCamera& camera, double glApertureX, double glApertureY) {
        const auto   usdFit = convertFit(camera);
        const double aperture = usdFit == CameraUtilConformWindowPolicy::CameraUtilMatchHorizontally
            ? camera.horizontalFilmAperture()
            : camera.verticalFilmAperture();
        const double glAperture
            = usdFit == CameraUtilConformWindowPolicy::CameraUtilMatchHorizontally ? glApertureX
                                                                                   : glApertureY;
        return (0.02 / aperture) * (aperture / glAperture);
    };

    auto viewParameters = [&](const MFnCamera& camera,
                              const GfVec4d*   viewport,
                              double&          apertureX,
                              double&          apertureY,
                              double&          offsetX,
                              double&          offsetY) -> MStatus {
        double aspectRatio = viewport
            ? ((*viewport)[2] - (*viewport)[0]) / ((*viewport)[3] - (*viewport)[1])
            : camera.aspectRatio();
        return camera.getViewParameters(
            aspectRatio, apertureX, apertureY, offsetX, offsetY, true, false, true);
    };

#if PXR_VERSION < 2102

    auto projectionMatrix
        = [&](const MFnCamera& camera, bool isOrtho, const GfVec4d* viewport) -> GfMatrix4d {
        double left, right, bottom, top, cameraNear = camera.nearClippingPlane(),
                                         cameraFar = camera.farClippingPlane(),
                                         cameraFarMinusNear = cameraFar - cameraNear,
                                         aspectRatio = viewport
            ? (((*viewport)[2] - (*viewport)[0]) / ((*viewport)[3] - (*viewport)[1]))
            : camera.aspectRatio();

        status = camera.getViewingFrustum(aspectRatio, left, right, bottom, top, true, false, true);

        if (isOrtho) {
            // Skip over extraneous double-precision math in the common symmetric case
            if (right == -left && top == -bottom)
                return GfMatrix4d(
                    1.0 / right,
                    0,
                    0,
                    0,
                    0,
                    1.0 / top,
                    0,
                    0,
                    0,
                    0,
                    -2.0 / cameraFarMinusNear,
                    0,
                    0,
                    0,
                    -(cameraFar + cameraNear) / cameraFarMinusNear,
                    1);

            return GfMatrix4d(
                2.0 / (right - left),
                0,
                0,
                0,
                0,
                2.0 / (top - bottom),
                0,
                0,
                0,
                0,
                -2.0 / cameraFarMinusNear,
                0,
                -(right + left) / (right - left),
                -(top + bottom) / (top - bottom),
                -(cameraFar + cameraNear) / cameraFarMinusNear,
                1);
        }

        // Skip over extraneous double-precision math in the common symmetric case
        if (right == -left && top == -bottom)
            return GfMatrix4d(
                cameraNear / right,
                0,
                0,
                0,
                0,
                cameraNear / top,
                0,
                0,
                0,
                0,
                -(cameraFar + cameraNear) / cameraFarMinusNear,
                -1,
                0,
                0,
                (-2.0 * cameraFar * cameraNear) / cameraFarMinusNear,
                0);

        return GfMatrix4d(
            (2.0 * cameraNear) / (right - left),
            0,
            0,
            0,
            0,
            (2.0 * cameraNear) / (top - bottom),
            0,
            0,
            (right + left) / (right - left),
            (top + bottom) / (top - bottom),
            -(cameraFar + cameraNear) / cameraFarMinusNear,
            -1,
            0,
            0,
            (2.0 * cameraNear * -cameraFar) / cameraFarMinusNear,
            0);
    };

#endif

    auto hadError = [&](MStatus& status) -> bool {
        if (ARCH_LIKELY(status))
            return false;
        TF_WARN(
            "Error in HdMayaCameraAdapter::GetCameraParamValue(%s): %s",
            paramName.GetText(),
            status.errorString().asChar());
        return false;
    };

    MFnCamera camera(GetDagPath(), &status);
    if (hadError(status))
        return {};

    const bool isOrtho = camera.isOrtho(&status);
    if (hadError(status)) {
        return {};
    }

#if PXR_VERSION < 2102
    if (paramName == HdCameraTokens->projectionMatrix) {
        const auto projMatrix = projectionMatrix(camera, isOrtho, _viewport.get());
        if (hadError(status))
            return {};
        return VtValue(projMatrix);
    }
    if (paramName == HdCameraTokens->worldToViewMatrix) {
        return VtValue(GetTransform().GetInverse());
    }
#endif

    if (paramName == HdCameraTokens->shutterOpen) {
        // No motion samples, instantaneous shutter
        if (!GetDelegate()->GetParams().motionSamplesEnabled())
            return VtValue(double(0));
        return VtValue(double(GetDelegate()->GetCurrentTimeSamplingInterval().GetMin()));
    }
    if (paramName == HdCameraTokens->shutterClose) {
        // No motion samples, instantaneous shutter
        if (!GetDelegate()->GetParams().motionSamplesEnabled())
            return VtValue(double(0));
        const auto shutterAngle = camera.shutterAngle(&status);
        if (hadError(status))
            return {};
        auto constexpr maxRadians = M_PI * 2.0;
        auto shutterClose = std::min(std::max(0.0, shutterAngle), maxRadians) / maxRadians;
        auto interval = GetDelegate()->GetCurrentTimeSamplingInterval();
        return VtValue(double(interval.GetMin() + interval.GetSize() * shutterClose));
    }

    // Don't bother with anything else for orthographic cameras
    if (isOrtho) {
        return {};
    }
    if (paramName == HdCameraTokens->focusDistance) {
        auto focusDistance = camera.focusDistance(&status);
        if (hadError(status))
            return {};
        return VtValue(float(focusDistance * mayaInchToHydraCentimeter));
    }
    if (paramName == HdCameraTokens->focalLength) {
        const double aspectRatio = _viewport
            ? (((*_viewport)[2] - (*_viewport)[0]) / ((*_viewport)[3] - (*_viewport)[1]))
            : camera.aspectRatio();

        double left, right, bottom, top;
        status = camera.getViewingFrustum(aspectRatio, left, right, bottom, top, true, false, true);

        const double cameraNear = camera.nearClippingPlane();

        const double focalLen
            = (convertFit(camera) == CameraUtilConformWindowPolicy::CameraUtilMatchVertically)
            ? (2.0 * cameraNear) / (top - bottom)
            : (2.0 * cameraNear) / (right - left);
        return VtValue(float(focalLen * mayaFocaLenToHydra));
    }
    if (paramName == HdCameraTokens->fStop) {
        // For USD/Hydra fStop=0 should disable depthOfField
        if (!camera.isDepthOfField())
            return VtValue(0.f);
        const auto fStop = camera.fStop(&status);
        if (hadError(status))
            return {};
        return VtValue(float(fStop));
    }
    if (paramName == HdCameraTokens->horizontalAperture) {
        double apertureX, apertureY, offsetX, offsetY;
        status = viewParameters(camera, _viewport.get(), apertureX, apertureY, offsetX, offsetY);
        if (hadError(status))
            return {};
        return VtValue(float(apertureX * apertureConvert(camera, apertureX, apertureY)));
    }
    if (paramName == HdCameraTokens->verticalAperture) {
        double apertureX, apertureY, offsetX, offsetY;
        status = viewParameters(camera, _viewport.get(), apertureX, apertureY, offsetX, offsetY);
        if (hadError(status))
            return {};
        return VtValue(float(apertureY * apertureConvert(camera, apertureX, apertureY)));
    }
    if (paramName == HdCameraTokens->horizontalApertureOffset) {
        double apertureX, apertureY, offsetX, offsetY;
        status = viewParameters(camera, _viewport.get(), apertureX, apertureY, offsetX, offsetY);
        if (hadError(status))
            return {};
        return VtValue(float(offsetX * mayaInchToHydraMillimeter));
    }
    if (paramName == HdCameraTokens->verticalApertureOffset) {
        double apertureX, apertureY, offsetX, offsetY;
        status = viewParameters(camera, _viewport.get(), apertureX, apertureY, offsetX, offsetY);
        if (hadError(status))
            return {};
        return VtValue(float(offsetY * mayaInchToHydraMillimeter));
    }
    if (paramName == HdCameraTokens->windowPolicy) {
        const auto windowPolicy = convertFit(camera);
        if (hadError(status))
            return {};
        return VtValue(windowPolicy);
    }
#if PXR_VERSION >= 2102
    if (paramName == HdCameraTokens->projection) {
        if (isOrtho) {
            return VtValue(HdCamera::Orthographic);
        } else {
            return VtValue(HdCamera::Perspective);
        }
    }

#endif

    return {};
}

void HdMayaCameraAdapter::SetViewport(const GfVec4d& viewport)
{
    if (!_viewport) {
        _viewport.reset(new GfVec4d);
    }
    *_viewport = viewport;
}

PXR_NAMESPACE_CLOSE_SCOPE
