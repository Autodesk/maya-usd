//
// Copyright 2019 Luma Pictures
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
#include "delegateCtx.h"

#include <hdmaya/hdmaya.h>

#include <pxr/base/gf/frustum.h>
#include <pxr/base/gf/plane.h>
#include <pxr/base/gf/range1d.h>

#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/rprim.h>

#ifdef HDMAYA_USD_001905_BUILD
#include <pxr/imaging/hio/glslfx.h>
#else
#include <pxr/imaging/glf/glslfx.h>
#endif // HDMAYA_USD_001905_BUILD

#include "../../../utils/util.h"

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

HdMayaDelegateCtx::HdMayaDelegateCtx(const InitData& initData)
    : HdSceneDelegate(initData.renderIndex, initData.delegateID),
      HdMayaDelegate(initData),
      _rprimPath(
          initData.delegateID.AppendPath(SdfPath(std::string("rprims")))),
      _sprimPath(
          initData.delegateID.AppendPath(SdfPath(std::string("sprims")))),
      _materialPath(
          initData.delegateID.AppendPath(SdfPath(std::string("materials")))) {
    GetChangeTracker().AddCollection(TfToken("visible"));
}

void HdMayaDelegateCtx::InsertRprim(
    const TfToken& typeId, const SdfPath& id, HdDirtyBits initialBits,
    const SdfPath& instancerId) {
    if (!instancerId.IsEmpty()) {
        GetRenderIndex().InsertInstancer(this, instancerId);
        GetChangeTracker().InstancerInserted(id);
    }
    GetRenderIndex().InsertRprim(typeId, this, id, instancerId);
    GetChangeTracker().RprimInserted(id, initialBits);
}

void HdMayaDelegateCtx::InsertSprim(
    const TfToken& typeId, const SdfPath& id, HdDirtyBits initialBits) {
    GetRenderIndex().InsertSprim(typeId, this, id);
    GetChangeTracker().SprimInserted(id, initialBits);
}

void HdMayaDelegateCtx::RemoveRprim(const SdfPath& id) {
    GetRenderIndex().RemoveRprim(id);
}

void HdMayaDelegateCtx::RemoveSprim(const TfToken& typeId, const SdfPath& id) {
    GetRenderIndex().RemoveSprim(typeId, id);
}

void HdMayaDelegateCtx::RemoveInstancer(const SdfPath& id) {
    GetRenderIndex().RemoveInstancer(id);
}

SdfPath HdMayaDelegateCtx::GetPrimPath(const MDagPath& dg, bool isLight) {
    if (isLight) {
        return _GetPrimPath(_sprimPath, dg);
    } else {
        return _GetPrimPath(_rprimPath, dg);
    }
}

SdfPath HdMayaDelegateCtx::GetMaterialPath(const MObject& obj) {
    return _GetMaterialPath(_materialPath, obj);
}

PXR_NAMESPACE_CLOSE_SCOPE
