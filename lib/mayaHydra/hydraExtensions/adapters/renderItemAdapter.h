//
// Copyright 2023 Autodesk, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an “AS IS” BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef MAYAHYDRALIB_RENDER_ITEM_ADAPTER_H
#define MAYAHYDRALIB_RENDER_ITEM_ADAPTER_H

#include <mayaHydraLib/adapters/adapter.h>
#include <mayaHydraLib/adapters/adapterDebugCodes.h>
#include <mayaHydraLib/adapters/materialNetworkConverter.h>
#include <mayaHydraLib/utils.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/meshTopology.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/imaging/hdx/renderTask.h>
#include <pxr/pxr.h>

#include <maya/MDagPath.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MMatrix.h>

#include <functional>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
std::string kRenderItemTypeName = "renderItem";

static constexpr const char* kPointSize = "pointSize";

static const SdfPath kInvalidMaterial = SdfPath("InvalidMaterial");
} // namespace

using MayaHydraRenderItemAdapterPtr = std::shared_ptr<class MayaHydraRenderItemAdapter>;

/**
 * \brief MayaHydraRenderItemAdapter is used to translate from a render item to hydra.
 * This is where we translate from Maya shapes (such as meshes) to hydra.
 */
class MayaHydraRenderItemAdapter : public MayaHydraAdapter
{
public:
    MAYAHYDRALIB_API
    MayaHydraRenderItemAdapter(
        const MDagPath&       dagPath,
        const SdfPath&        slowId,
        int                   fastId,
        MayaHydraDelegateCtx* del,
        const MRenderItem&    ri);

    MAYAHYDRALIB_API
    virtual ~MayaHydraRenderItemAdapter();

    MAYAHYDRALIB_API
    virtual void RemovePrim() override { }

    MAYAHYDRALIB_API
    virtual void Populate() override { }

    MAYAHYDRALIB_API
    bool HasType(const TfToken& typeId) const override { return typeId == HdPrimTypeTokens->mesh; }

    MAYAHYDRALIB_API
    virtual bool IsSupported() const override;

    MAYAHYDRALIB_API
    bool GetDoubleSided() const override { return false; };

    MAYAHYDRALIB_API
    virtual void MarkDirty(HdDirtyBits dirtyBits) override;

    MAYAHYDRALIB_API
    VtValue Get(const TfToken& key) override;

    MAYAHYDRALIB_API
    VtValue GetMaterialResource();

    MAYAHYDRALIB_API
    void SetPlaybackChanged();

    MAYAHYDRALIB_API
    bool GetVisible() override;

    MAYAHYDRALIB_API
    void SetVisible(bool val) { _visible = val; }

    MAYAHYDRALIB_API
    const MColor& GetWireframeColor() const { return _wireframeColor; }

    MAYAHYDRALIB_API
    MHWRender::DisplayStatus GetDisplayStatus() const { return _displayStatus; }

    MAYAHYDRALIB_API
    GfMatrix4d GetTransform() override { return _transform[0]; }

    MAYAHYDRALIB_API
    void InvalidateTransform() { }

    MAYAHYDRALIB_API
    bool IsInstanced() const { return false; }

    MAYAHYDRALIB_API
    HdPrimvarDescriptorVector GetPrimvarDescriptors(HdInterpolation interpolation) override;

    MAYAHYDRALIB_API
    void UpdateTransform(MRenderItem& ri);

    /// Class used to pass data to the UpdateFromDelta method, so we can extend the parameters in
    /// the future if needed.
    class UpdateFromDeltaData
    {
    public:
        UpdateFromDeltaData(
            MRenderItem&             ri,
            unsigned int             flags,
            const MColor&            wireframeColor,
            MHWRender::DisplayStatus displayStatus)
            : _ri(ri)
            , _flags(flags)
            , _wireframeColor(wireframeColor)
            , _displayStatus(displayStatus)
        {
        }

        MRenderItem&             _ri;
        unsigned int             _flags;
        const MColor&            _wireframeColor;
        MHWRender::DisplayStatus _displayStatus;
    };

    /// We receive in that function the changes made in the Maya viewport between the last frame
    /// rendered and the current frame
    MAYAHYDRALIB_API
    void UpdateFromDelta(const UpdateFromDeltaData& data);

    MAYAHYDRALIB_API
    HdMeshTopology GetMeshTopology() override;

    MAYAHYDRALIB_API
    HdBasisCurvesTopology GetBasisCurvesTopology() override;

    MAYAHYDRALIB_API
    virtual TfToken GetRenderTag() const override;

    MAYAHYDRALIB_API
    SdfPath& GetMaterial() { return _material; }

    MAYAHYDRALIB_API
    void SetMaterial(const SdfPath& val) { _material = val; }

    MAYAHYDRALIB_API
    int GetFastID() const { return _fastId; }

    MAYAHYDRALIB_API
    const MDagPath& GetDagPath() const { return _dagPath; }

    MAYAHYDRALIB_API
    MGeometry::Primitive GetPrimitive() const { return _primitive; }

private:
    MAYAHYDRALIB_API
    void _RemoveRprim();

    MAYAHYDRALIB_API
    void _InsertRprim();

    SdfPath                     _material;
    MDagPath                    _dagPath;
    std::unique_ptr<HdTopology> _topology = nullptr;
    VtVec3fArray                _positions = {};
    VtVec2fArray                _uvs = {};
    MGeometry::Primitive        _primitive;
    MString                     _name;
    GfMatrix4d                  _transform[2];
    int                         _fastId = 0;
    bool                        _visible = false;
    MColor                      _wireframeColor = { 1.f, 1.f, 1.f, 1.f };
    bool                        _isHideOnPlayback = false;
    MHWRender::DisplayStatus    _displayStatus = MHWRender::DisplayStatus::kNoStatus;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_RENDER_ITEM_ADAPTER_H
