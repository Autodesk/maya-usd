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
#ifndef __HDMAYA_DELEGATE_BASE_H__
#define __HDMAYA_DELEGATE_BASE_H__

#include <pxr/pxr.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/rprimCollection.h>
#include <pxr/imaging/hd/sceneDelegate.h>

#include <pxr/usd/sdf/path.h>

#include <maya/MDagPath.h>

#include <hdmaya/delegates/delegate.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDelegateCtx : public HdSceneDelegate, public HdMayaDelegate {
protected:
    HDMAYA_API
    HdMayaDelegateCtx(HdRenderIndex* renderIndex, const SdfPath& delegateID);

public:
    using HdSceneDelegate::GetRenderIndex;
    HdChangeTracker& GetChangeTracker() {
        return GetRenderIndex().GetChangeTracker();
    }

    HDMAYA_API
    void InsertRprim(
        const TfToken& typeId, const SdfPath& id, HdDirtyBits initialBits);
    HDMAYA_API
    void InsertSprim(
        const TfToken& typeId, const SdfPath& id, HdDirtyBits initialBits);
    HDMAYA_API
    void RemoveRprim(const SdfPath& id);
    HDMAYA_API
    void RemoveSprim(const TfToken& typeId, const SdfPath& id);
    virtual void RemoveAdapter(const SdfPath& id) = 0;
    const HdRprimCollection& GetRprimCollection() { return _rprimCollection; }
    HDMAYA_API
    SdfPath GetPrimPath(const MDagPath& dg);
    HDMAYA_API
    SdfPath GetMaterialPath(const MObject& obj);

    /// Fit the frustum's near/far value to contain all
    /// the rprims inside the render index;
    HDMAYA_API
    void FitFrustumToRprims(GfFrustum& frustum, const GfMatrix4d& lightToWorld);

    inline bool GetNeedsGLGLSFX() { return _needsGLSLFX; }

private:
    HdRprimCollection _rprimCollection;
    SdfPath _rprimPath;
    SdfPath _sprimPath;
    SdfPath _materialPath;

    bool _needsGLSLFX;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_BASE_H__
