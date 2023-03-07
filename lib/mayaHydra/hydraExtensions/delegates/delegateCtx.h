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
#ifndef MAYAHYDRALIB_DELEGATE_BASE_H
#define MAYAHYDRALIB_DELEGATE_BASE_H

#include <mayaHydraLib/delegates/delegate.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MDagPath.h>
#include <maya/MHWGeometryUtilities.h>

PXR_NAMESPACE_OPEN_SCOPE

/**
 * \brief MayaHydraDelegateCtx is a set of common functions, and it is the aggregation of our
 * MayaHydraDelegate base class and the hydra custom scene delegate class : HdSceneDelegate.
 */
class MayaHydraDelegateCtx
    : public HdSceneDelegate
    , public MayaHydraDelegate
{
protected:
    MAYAHYDRALIB_API
    MayaHydraDelegateCtx(const InitData& initData);

public:
    enum RebuildFlags : uint32_t
    {
        RebuildFlagPrim = 1 << 1,
        RebuildFlagCallbacks = 1 << 2,
    };

    using HdSceneDelegate::GetRenderIndex;
    HdChangeTracker& GetChangeTracker() { return GetRenderIndex().GetChangeTracker(); }

    MAYAHYDRALIB_API
    void InsertRprim(const TfToken& typeId, const SdfPath& id, const SdfPath& instancerId = {});
    MAYAHYDRALIB_API
    void InsertSprim(const TfToken& typeId, const SdfPath& id, HdDirtyBits initialBits);
    MAYAHYDRALIB_API
    void RemoveRprim(const SdfPath& id);
    MAYAHYDRALIB_API
    void RemoveSprim(const TfToken& typeId, const SdfPath& id);
    MAYAHYDRALIB_API
    void         RemoveInstancer(const SdfPath& id);
    virtual void RemoveAdapter(const SdfPath& id) { }
    virtual void RecreateAdapter(const SdfPath& id, const MObject& obj) { }
    virtual void RecreateAdapterOnIdle(const SdfPath& id, const MObject& obj) { }
    virtual void RebuildAdapterOnIdle(const SdfPath& id, uint32_t flags) { }
    virtual void UpdateDisplayStatusMaterial(
        MHWRender::DisplayStatus displayStatus,
        const MColor&            wireframecolor)
    {
    }
    virtual void AddArnoldLight(const MDagPath& dag) { }
    virtual void RemoveArnoldLight(const MDagPath& dag) { }

    /// \brief Notifies the scene delegate when a material tag changes.
    ///
    /// \param id Id of the Material that changed its tag.
    virtual void MaterialTagChanged(const SdfPath& id) { }
    MAYAHYDRALIB_API
    SdfPath GetPrimPath(const MDagPath& dg, bool isSprim);
    MAYAHYDRALIB_API
    SdfPath GetRenderItemPrimPath(const MRenderItem& ri);
    MAYAHYDRALIB_API
    SdfPath GetRenderItemShaderPrimPath(const MRenderItem& ri);
    MAYAHYDRALIB_API
    SdfPath GetMaterialPath(const MObject& obj);

    MAYAHYDRALIB_API
    SdfPath
    GetSolidPrimsRootPath() const; // Get the root path for non lines and non points primitives

private:
    SdfPath _rprimPath;
    SdfPath _sprimPath;
    SdfPath _materialPath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_DELEGATE_BASE_H
