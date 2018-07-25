#include <hdmaya/delegates/delegateCtx.h>

#include <pxr/base/gf/frustum.h>
#include <pxr/base/gf/plane.h>
#include <pxr/base/gf/range1d.h>

#include <pxr/imaging/hd/rprim.h>
#include <pxr/imaging/hd/renderDelegate.h>

#include <pxr/imaging/glf/glslfx.h>

#include <usdMaya/util.h>

#include <maya/MFnLight.h>

#include <array>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

SdfPath
_GetPrimPath(const SdfPath& base, const MDagPath& dg) {
    const auto mayaPath = PxrUsdMayaUtil::MDagPathToUsdPath(dg, false, false);
    if (mayaPath.IsEmpty()) { return {}; }
    const auto* chr = mayaPath.GetText();
    if (chr == nullptr) { return {}; };
    std::string s(chr + 1);
    if (s.empty()) { return {}; }
    return base.AppendPath(SdfPath(s));
}

SdfPath
_GetMaterialPath(const SdfPath& base, const MObject& obj) {
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

}

HdMayaDelegateCtx::HdMayaDelegateCtx(
    HdRenderIndex* renderIndex,
    const SdfPath& delegateID)
    : HdSceneDelegate(renderIndex, delegateID),
     _rprimPath(delegateID.AppendPath(SdfPath(std::string("rprims")))),
     _sprimPath(delegateID.AppendPath(SdfPath(std::string("sprims")))),
     _materialPath(delegateID.AppendPath(SdfPath(std::string("materials")))) {
    _rprimCollection.SetName(TfToken("visible"));
    _rprimCollection.SetRootPath(_rprimPath);
    _rprimCollection.SetRenderTags({HdTokens->geometry});
    GetChangeTracker().AddCollection(TfToken("visible"));

    _needsGLSLFX = renderIndex->GetRenderDelegate()->GetMaterialNetworkSelector()
        == GlfGLSLFXTokens->glslfx;
}

void
HdMayaDelegateCtx::InsertRprim(const TfToken& typeId, const SdfPath& id, HdDirtyBits initialBits) {
    GetRenderIndex().InsertRprim(typeId, this, id);
    GetChangeTracker().RprimInserted(id, initialBits);
}

void
HdMayaDelegateCtx::InsertSprim(const TfToken& typeId, const SdfPath& id, HdDirtyBits initialBits) {
    GetRenderIndex().InsertSprim(typeId, this, id);
    GetChangeTracker().SprimInserted(id, initialBits);
}

void
HdMayaDelegateCtx::RemoveRprim(const SdfPath& id) {
    GetRenderIndex().RemoveRprim(id);
}

void
HdMayaDelegateCtx::RemoveSprim(const TfToken& typeId, const SdfPath& id) {
    GetRenderIndex().RemoveSprim(typeId, id);
}

SdfPath
HdMayaDelegateCtx::GetPrimPath(const MDagPath& dg) {
    if (dg.hasFn(MFn::kLight)) {
        return _GetPrimPath(_sprimPath, dg);
    } else {
        return _GetPrimPath(_rprimPath, dg);
    }
}

SdfPath
HdMayaDelegateCtx::GetMaterialPath(const MObject& obj) {
    return _GetMaterialPath(_materialPath, obj);
}

void
HdMayaDelegateCtx::FitFrustumToRprims(GfFrustum& frustum) {
    auto getInverse = [](const GfMatrix4d& mat) {
        const double PRECISION_LIMIT = 1.0e-13;
        double det;
        auto ret = mat.GetInverse(&det, PRECISION_LIMIT);
        if (GfAbs(det) <= PRECISION_LIMIT) {
            return GfMatrix4d(1.0);
        }
        return ret;
    };
    // TODO: Cache these queries and handle dirtying.
    // TODO: Take visibility and shadow casting parameters into account.
    // This slightly differs from how you would make a calculation on a traditional frustum,
    // since we don't have a far plane. For simplicity we set the near plane to
    // 0.1 to cull anything behind the light.
    // We sum all the bounding boxes inside the open-ended frustum,
    // and use that bounding box to calculate the closest and farthest away points.

    std::array<GfPlane, 5> planes;
    GfRange1d nearFar;
    const auto& rotation = frustum.GetRotation();
    const auto direction = rotation.TransformDir(GfVec3d(0.0, 0.0, -1.0)).GetNormalized();
    const auto right = rotation.TransformDir(GfVec3d(1.0, 0.0, 0.0)).GetNormalized();
    const auto up = rotation.TransformDir(GfVec3d(0.0, 1.0, 0.0)).GetNormalized();
    const auto position = frustum.GetPosition();
    planes[0].Set(direction, position);

    if (frustum.GetProjectionType() == GfFrustum::Perspective) {
        const auto windowSize = frustum.GetWindow().GetSize();
        const auto vfov = GfRadiansToDegrees(atan((windowSize[1] / 2.0) / GfFrustum::GetReferencePlaneDepth()));
        const auto hfov = GfRadiansToDegrees(atan((windowSize[0] / 2.0) / GfFrustum::GetReferencePlaneDepth()));
        // Right plane
        planes[1].Set(GfRotation(up, -hfov).TransformDir(-right).GetNormalized(), position);
        // Left plane
        planes[2].Set(GfRotation(up, hfov).TransformDir(right).GetNormalized(), position);
        // Top plane
        planes[3].Set(GfRotation(right, vfov).TransformDir(-up).GetNormalized(), position);
        // Bottom plane
        planes[4].Set(GfRotation(right, -vfov).TransformDir(up).GetNormalized(), position);
    } else if (frustum.GetProjectionType() == GfFrustum::Orthographic) {
        const auto& window = frustum.GetWindow();
        // Right plane
        planes[1].Set(-right, position + right * window.GetMax()[0]);
        // Left plane
        planes[2].Set(right, position + right * window.GetMin()[0]);
        // Top plane
        planes[3].Set(-up, position + up * window.GetMax()[1]);
        // Bottom plane
        planes[4].Set(up, position + up * window.GetMin()[1]);
    } else {
        return;
    }

    auto isBoxInside = [&planes] (const GfRange3d& extent, const GfMatrix4d& worldToLocal) -> bool {
        for (const auto& plane : planes) {
            auto localPlane = plane;
            localPlane.Transform(worldToLocal);
            if (!localPlane.IntersectsPositiveHalfSpace(extent)) {
                return false;
            }
        }
        return true;
    };

    for (const auto& id : GetRenderIndex().GetRprimIds()) {
        auto* rprim = GetRenderIndex().GetRprim(id);
        if (rprim == nullptr) { continue; }
        auto* delegate = GetRenderIndex().GetSceneDelegateForRprim(id);
        if (delegate == nullptr) { continue; }
        const auto extent = delegate->GetExtent(id);
        if (extent.IsEmpty()) {
            continue;
        }
        const auto localToWorld = delegate->GetTransform(id);

        if (isBoxInside(extent, getInverse(localToWorld))) {
            for (size_t i = 0; i < 8u; ++i) {
                const auto corner = localToWorld.Transform(extent.GetCorner(i));
                // Project position into direction
                nearFar.ExtendBy((corner - position) * direction);
            }
        }
    }

    nearFar.SetMin(std::max(0.1, nearFar.GetMin()));
    frustum.SetNearFar(nearFar);
}

PXR_NAMESPACE_CLOSE_SCOPE
