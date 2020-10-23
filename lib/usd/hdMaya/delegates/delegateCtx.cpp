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
#include "delegateCtx.h"

#include "mayaUsd/utils/util.h"

#include <pxr/base/gf/frustum.h>
#include <pxr/base/gf/plane.h>
#include <pxr/base/gf/range1d.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/rprim.h>
#include <pxr/imaging/hio/glslfx.h>

#include <maya/MFnLight.h>

#include <array>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

SdfPath _GetPrimPath(const SdfPath& base, const MDagPath& dg)
{
    const auto mayaPath = UsdMayaUtil::MDagPathToUsdPath(dg, false, false);
    if (mayaPath.IsEmpty()) {
        return {};
    }
    const auto* chr = mayaPath.GetText();
    if (chr == nullptr) {
        return {};
    };
    std::string s(chr + 1);
    if (s.empty()) {
        return {};
    }
    return base.AppendPath(SdfPath(s));
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

HdMayaDelegateCtx::HdMayaDelegateCtx(const InitData& initData)
    : HdSceneDelegate(initData.renderIndex, initData.delegateID)
    , HdMayaDelegate(initData)
    , _rprimPath(initData.delegateID.AppendPath(SdfPath(std::string("rprims"))))
    , _sprimPath(initData.delegateID.AppendPath(SdfPath(std::string("sprims"))))
    , _materialPath(initData.delegateID.AppendPath(SdfPath(std::string("materials"))))
{
    GetChangeTracker().AddCollection(TfToken("visible"));
}

void HdMayaDelegateCtx::InsertRprim(
    const TfToken& typeId,
    const SdfPath& id,
    const SdfPath& instancerId)
{
    if (!instancerId.IsEmpty()) {
        GetRenderIndex().InsertInstancer(this, instancerId);
    }
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
    GetRenderIndex().InsertRprim(typeId, this, id);
#else
    GetRenderIndex().InsertRprim(typeId, this, id, instancerId);
#endif
}

void HdMayaDelegateCtx::InsertSprim(
    const TfToken& typeId,
    const SdfPath& id,
    HdDirtyBits    initialBits)
{
    GetRenderIndex().InsertSprim(typeId, this, id);
    GetChangeTracker().SprimInserted(id, initialBits);
}

void HdMayaDelegateCtx::RemoveRprim(const SdfPath& id) { GetRenderIndex().RemoveRprim(id); }

void HdMayaDelegateCtx::RemoveSprim(const TfToken& typeId, const SdfPath& id)
{
    GetRenderIndex().RemoveSprim(typeId, id);
}

void HdMayaDelegateCtx::RemoveInstancer(const SdfPath& id) { GetRenderIndex().RemoveInstancer(id); }

SdfPath HdMayaDelegateCtx::GetPrimPath(const MDagPath& dg, bool isLight)
{
    if (isLight) {
        return _GetPrimPath(_sprimPath, dg);
    } else {
        return _GetPrimPath(_rprimPath, dg);
    }
}

SdfPath HdMayaDelegateCtx::GetMaterialPath(const MObject& obj)
{
    return _GetMaterialPath(_materialPath, obj);
}

PXR_NAMESPACE_CLOSE_SCOPE
