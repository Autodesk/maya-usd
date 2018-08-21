//
// Copyright 2018 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include <hdmaya/delegates/delegateCtx.h>

#include <pxr/base/gf/frustum.h>
#include <pxr/base/gf/plane.h>
#include <pxr/base/gf/range1d.h>

#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/rprim.h>

#include <pxr/imaging/glf/glslfx.h>

#include <usdMaya/util.h>

#include <maya/MFnLight.h>

#include <array>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

SdfPath _GetPrimPath(const SdfPath& base, const MDagPath& dg) {
    const auto mayaPath = UsdMayaUtil::MDagPathToUsdPath(dg, false, false);
    if (mayaPath.IsEmpty()) { return {}; }
    const auto* chr = mayaPath.GetText();
    if (chr == nullptr) { return {}; };
    std::string s(chr + 1);
    if (s.empty()) { return {}; }
    return base.AppendPath(SdfPath(s));
}

SdfPath _GetMaterialPath(const SdfPath& base, const MObject& obj) {
    MStatus status;
    MFnDependencyNode node(obj, &status);
    if (!status) { return {}; }
    const auto* chr = node.name().asChar();
    if (chr == nullptr || chr[0] == '\0') { return {}; }
    std::string usdPathStr(chr);
    // replace namespace ":" with "_"
    std::replace(usdPathStr.begin(), usdPathStr.end(), ':', '_');
    return base.AppendPath(SdfPath(usdPathStr));
}

} // namespace

HdMayaDelegateCtx::HdMayaDelegateCtx(HdRenderIndex* renderIndex, const SdfPath& delegateID)
    : HdSceneDelegate(renderIndex, delegateID),
      _rprimPath(delegateID.AppendPath(SdfPath(std::string("rprims")))),
      _sprimPath(delegateID.AppendPath(SdfPath(std::string("sprims")))),
      _materialPath(delegateID.AppendPath(SdfPath(std::string("materials")))) {
    _rprimCollection.SetName(TfToken("visible"));
    _rprimCollection.SetRootPath(_rprimPath);
    _rprimCollection.SetRenderTags({HdTokens->geometry});
    GetChangeTracker().AddCollection(TfToken("visible"));

    _needsGLSLFX =
        renderIndex->GetRenderDelegate()->GetMaterialNetworkSelector() == GlfGLSLFXTokens->glslfx;
}

void HdMayaDelegateCtx::InsertRprim(
    const TfToken& typeId, const SdfPath& id, HdDirtyBits initialBits) {
    GetRenderIndex().InsertRprim(typeId, this, id);
    GetChangeTracker().RprimInserted(id, initialBits);
}

void HdMayaDelegateCtx::InsertSprim(
    const TfToken& typeId, const SdfPath& id, HdDirtyBits initialBits) {
    GetRenderIndex().InsertSprim(typeId, this, id);
    GetChangeTracker().SprimInserted(id, initialBits);
}

void HdMayaDelegateCtx::RemoveRprim(const SdfPath& id) { GetRenderIndex().RemoveRprim(id); }

void HdMayaDelegateCtx::RemoveSprim(const TfToken& typeId, const SdfPath& id) {
    GetRenderIndex().RemoveSprim(typeId, id);
}

SdfPath HdMayaDelegateCtx::GetPrimPath(const MDagPath& dg) {
    if (dg.hasFn(MFn::kLight)) {
        return _GetPrimPath(_sprimPath, dg);
    } else {
        return _GetPrimPath(_rprimPath, dg);
    }
}

SdfPath HdMayaDelegateCtx::GetMaterialPath(const MObject& obj) {
    return _GetMaterialPath(_materialPath, obj);
}

void HdMayaDelegateCtx::FitFrustumToRprims(GfFrustum& frustum, const GfMatrix4d& lightToWorld) {
    auto getInverse = [](const GfMatrix4d& mat) {
        const double PRECISION_LIMIT = 1.0e-13;
        double det;
        auto ret = mat.GetInverse(&det, PRECISION_LIMIT);
        if (GfAbs(det) <= PRECISION_LIMIT) { return GfMatrix4d(1.0); }
        return ret;
    };
    GfMatrix4d worldToLight = getInverse(lightToWorld);

    // TODO: Cache these queries and handle dirtying.
    // TODO: Take visibility and shadow casting parameters into account.
    // This slightly differs from how you would make a calculation on a traditional frustum,
    // since we don't have a far plane. For simplicity we set the near plane to
    // 0.1 to cull anything behind the light.
    // We sum all the bounding boxes inside the open-ended frustum,
    // and use that bounding box to calculate the closest and farthest away points.

    std::array<GfPlane, 5> planes;
    GfRange1d nearFar;

    const auto direction = GfVec3d(0.0, 0.0, -1.0);
    const auto right = GfVec3d(1.0, 0.0, 0.0);
    const auto up = GfVec3d(0.0, 1.0, 0.0);
    planes[0].Set(direction, 0);

    if (frustum.GetProjectionType() == GfFrustum::Perspective) {
        const auto windowSize = frustum.GetWindow().GetSize();
        const auto vfov =
            GfRadiansToDegrees(atan((windowSize[1] / 2.0) / GfFrustum::GetReferencePlaneDepth()));
        const auto hfov =
            GfRadiansToDegrees(atan((windowSize[0] / 2.0) / GfFrustum::GetReferencePlaneDepth()));
        // Right plane
        planes[1].Set(GfRotation(up, -hfov).TransformDir(-right).GetNormalized(), 0);
        // Left plane
        planes[2].Set(GfRotation(up, hfov).TransformDir(right).GetNormalized(), 0);
        // Top plane
        planes[3].Set(GfRotation(right, vfov).TransformDir(-up).GetNormalized(), 0);
        // Bottom plane
        planes[4].Set(GfRotation(right, -vfov).TransformDir(up).GetNormalized(), 0);
    } else if (frustum.GetProjectionType() == GfFrustum::Orthographic) {
        const auto& window = frustum.GetWindow();
        // Right plane
        planes[1].Set(-right, right * window.GetMax()[0]);
        // Left plane
        planes[2].Set(right, right * window.GetMin()[0]);
        // Top plane
        planes[3].Set(-up, up * window.GetMax()[1]);
        // Bottom plane
        planes[4].Set(up, up * window.GetMin()[1]);
    } else {
        return;
    }

    for (auto& plane : planes) { plane.Transform(lightToWorld); }

    auto isBoxInside = [&planes](const GfRange3d& extent, const GfMatrix4d& worldToLocal) -> bool {
        for (const auto& plane : planes) {
            auto localPlane = plane;
            localPlane.Transform(worldToLocal);
            if (!localPlane.IntersectsPositiveHalfSpace(extent)) { return false; }
        }
        return true;
    };

    for (const auto& id : GetRenderIndex().GetRprimIds()) {
        auto* delegate = GetRenderIndex().GetSceneDelegateForRprim(id);
        if (delegate == nullptr) { continue; }
        const auto extent = delegate->GetExtent(id);
        if (extent.IsEmpty()) { continue; }
        const auto localToWorld = delegate->GetTransform(id);

        if (isBoxInside(extent, getInverse(localToWorld))) {
            auto localToLight = localToWorld * worldToLight;
            for (size_t i = 0; i < 8u; ++i) {
                const auto corner = localToLight.Transform(extent.GetCorner(i));
                // Project position into direction
                nearFar.ExtendBy(-corner[2]);
            }
        }
    }

    nearFar.SetMin(std::max(0.1, nearFar.GetMin()));
    frustum.SetNearFar(nearFar);
}

PXR_NAMESPACE_CLOSE_SCOPE
