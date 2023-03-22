//
// Copyright 2019 Luma Pictures
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
// Copyright 2023 Autodesk, Inc. All rights reserved.
//
#include "delegateCtx.h"

#include <mayaHydraLib/utils.h>

#include <pxr/base/gf/frustum.h>
#include <pxr/base/gf/plane.h>
#include <pxr/base/gf/range1d.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/rprim.h>
#include <pxr/imaging/hio/glslfx.h>

#include <maya/MFnLight.h>
#include <maya/MShaderManager.h>

#include <array>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

static const SdfPath solidPath = SdfPath(std::string("Solid"));

SdfPath _GetRenderItemMayaPrimPath(const MRenderItem& ri)
{
    if (ri.InternalObjectId() == 0)
        return {};

    SdfPath mayaPath = MAYAHYDRA_NS::RenderItemToSdfPath(ri, false, false);
    if (mayaPath.IsEmpty())
        return {};

    const std::string theString = std::string(mayaPath.GetText());
    if (theString.size() < 2) {
        return {}; // Error
    }

    // We cannot apppend an absolute path (I.e : starting with "/")
    if (mayaPath.IsAbsolutePath()) {
        mayaPath = mayaPath.MakeRelativePath(SdfPath::AbsoluteRootPath());
    }

    if (MHWRender::MGeometry::Primitive::kLines != ri.primitive()
        && MHWRender::MGeometry::Primitive::kLineStrip != ri.primitive()
        && MHWRender::MGeometry::Primitive::kPoints != ri.primitive()) {
        // Prefix with "Solid" when it's not a line/points primitive to be able to use only solid
        // primitives in lighting/shadowing by their root path
        mayaPath = solidPath.AppendPath(mayaPath);
    }

    return mayaPath;
}

SdfPath _GetPrimPath(const SdfPath& base, const MDagPath& dg)
{
    SdfPath mayaPath = MAYAHYDRA_NS::DagPathToSdfPath(dg, false, false);
    if (mayaPath.IsEmpty()) {
        return {};
    }
    const std::string theString = std::string(mayaPath.GetText());
    if (theString.size() < 2) {
        return {}; // Error
    }

    // We cannot apppend an absolute path (I.e : starting with "/")
    if (mayaPath.IsAbsolutePath()) {
        mayaPath = mayaPath.MakeRelativePath(SdfPath::AbsoluteRootPath());
    }

    return base.AppendPath(mayaPath);
}

SdfPath _GetRenderItemPrimPath(const SdfPath& base, const MRenderItem& ri)
{
    return base.AppendPath(_GetRenderItemMayaPrimPath(ri));
}

SdfPath _GetRenderItemShaderPrimPath(const SdfPath& base, const MRenderItem& ri)
{
    return _GetRenderItemPrimPath(base, ri);
}

SdfPath _GetMaterialPath(const SdfPath& base, const MObject& obj)
{
    MStatus           status;
    MFnDependencyNode node(obj, &status);
    if (!status) {
        return {};
    }
    const auto* chr = node.name().asChar();
    if (chr == nullptr || chr[0] == '\0') {
        return {};
    }
    std::string usdPathStr(chr);
    // replace namespace ":" with "_"
    std::replace(usdPathStr.begin(), usdPathStr.end(), ':', '_');
    return base.AppendPath(SdfPath(usdPathStr));
}

} // namespace

// MayaHydraDelegateCtx is a set of common functions, and it is the aggregation of our
// MayaHydraDelegate base class and the hydra custom scene delegate class : HdSceneDelegate.
MayaHydraDelegateCtx::MayaHydraDelegateCtx(const InitData& initData)
    : HdSceneDelegate(initData.renderIndex, initData.delegateID)
    , MayaHydraDelegate(initData)
    , _rprimPath(initData.delegateID.AppendPath(SdfPath(std::string("rprims"))))
    , _sprimPath(initData.delegateID.AppendPath(SdfPath(std::string("sprims"))))
    , _materialPath(initData.delegateID.AppendPath(SdfPath(std::string("materials"))))
{
    GetChangeTracker().AddCollection(TfToken("visible"));
}

void MayaHydraDelegateCtx::InsertRprim(
    const TfToken& typeId,
    const SdfPath& id,
    const SdfPath& instancerId)
{
    if (!instancerId.IsEmpty()) {
        GetRenderIndex().InsertInstancer(this, instancerId);
    }
    GetRenderIndex().InsertRprim(typeId, this, id);
}

void MayaHydraDelegateCtx::InsertSprim(
    const TfToken& typeId,
    const SdfPath& id,
    HdDirtyBits    initialBits)
{
    GetRenderIndex().InsertSprim(typeId, this, id);
    GetChangeTracker().SprimInserted(id, initialBits);
}

void MayaHydraDelegateCtx::RemoveRprim(const SdfPath& id) { GetRenderIndex().RemoveRprim(id); }

void MayaHydraDelegateCtx::RemoveSprim(const TfToken& typeId, const SdfPath& id)
{
    GetRenderIndex().RemoveSprim(typeId, id);
}

void MayaHydraDelegateCtx::RemoveInstancer(const SdfPath& id)
{
    GetRenderIndex().RemoveInstancer(id);
}

SdfPath MayaHydraDelegateCtx::GetRprimPath() const { return _rprimPath; }

SdfPath MayaHydraDelegateCtx::GetPrimPath(const MDagPath& dg, bool isSprim)
{
    if (isSprim) {
        return _GetPrimPath(_sprimPath, dg);
    } else {
        return _GetPrimPath(_rprimPath, dg);
    }
}

SdfPath MayaHydraDelegateCtx::GetRenderItemPrimPath(const MRenderItem& ri)
{
    return _GetRenderItemPrimPath(_rprimPath, ri);
}

SdfPath MayaHydraDelegateCtx::GetRenderItemShaderPrimPath(const MRenderItem& ri)
{
    return _GetRenderItemShaderPrimPath(_rprimPath, ri);
}

SdfPath MayaHydraDelegateCtx::GetMaterialPath(const MObject& obj)
{
    return _GetMaterialPath(_materialPath, obj);
}

SdfPath MayaHydraDelegateCtx::GetSolidPrimsRootPath() const
{
    return _rprimPath.AppendPath(solidPath);
}

PXR_NAMESPACE_CLOSE_SCOPE
