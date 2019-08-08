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
#ifndef __HDMAYA_DELEGATE_BASE_H__
#define __HDMAYA_DELEGATE_BASE_H__

#include <pxr/pxr.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/sceneDelegate.h>

#include <pxr/usd/sdf/path.h>

#include <maya/MDagPath.h>

#include "delegate.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDelegateCtx : public HdSceneDelegate, public HdMayaDelegate {
protected:
    HDMAYA_API
    HdMayaDelegateCtx(const InitData& initData);

public:
    enum RebuildFlags : uint32_t {
        RebuildFlagPrim = 1 << 1,
        RebuildFlagCallbacks = 1 << 2,
    };

    using HdSceneDelegate::GetRenderIndex;
    HdChangeTracker& GetChangeTracker() {
        return GetRenderIndex().GetChangeTracker();
    }

    HDMAYA_API
    void InsertRprim(
        const TfToken& typeId, const SdfPath& id, HdDirtyBits initialBits,
        const SdfPath& instancerId = {});
    HDMAYA_API
    void InsertSprim(
        const TfToken& typeId, const SdfPath& id, HdDirtyBits initialBits);
    HDMAYA_API
    void RemoveRprim(const SdfPath& id);
    HDMAYA_API
    void RemoveSprim(const TfToken& typeId, const SdfPath& id);
    HDMAYA_API
    void RemoveInstancer(const SdfPath& id);
    virtual void RemoveAdapter(const SdfPath& id) {}
    virtual void RecreateAdapter(const SdfPath& id, const MObject& obj) {}
    virtual void RecreateAdapterOnIdle(const SdfPath& id, const MObject& obj) {}
    virtual void RebuildAdapterOnIdle(const SdfPath& id, uint32_t flags) {}
    /// \brief Notifies the scene delegate when a material tag changes.
    ///
    /// \param id Id of the Material that changed its tag.
    virtual void MaterialTagChanged(const SdfPath& id) {}
    HDMAYA_API
    SdfPath GetPrimPath(const MDagPath& dg, bool isLight);
    HDMAYA_API
    SdfPath GetMaterialPath(const MObject& obj);

private:
    SdfPath _rprimPath;
    SdfPath _sprimPath;
    SdfPath _materialPath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_BASE_H__
