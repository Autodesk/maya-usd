// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "mesh.h"
#include "draw_item.h"
#include "material.h"
#include "instancer.h"
#include "proxyRenderDelegate.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/pxOsd/tokens.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/imaging/hd/vertexAdjacency.h"

#include <maya/MMatrix.h>
#include <maya/MProfiler.h>
#include <maya/MSelectionMask.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

    //! Helper utility function to get number of draw items required for given representation
    size_t _GetNumDrawItemsForDesc(const HdMeshReprDesc& reprDesc)
    {
        // By default, each repr desc item maps to 1 draw item
        size_t numDrawItems = 1;

        // Different representations may require different number of draw items
        // See HdSt for an example.
        switch (reprDesc.geomStyle) {
        case HdMeshGeomStyleInvalid:
            numDrawItems = 0;
            break;
        default:
            break;
        }

        return numDrawItems;
    }

    int _profilerCategory = MProfiler::addCategory("HdVP2RenderDelegate", "HdVP2RenderDelegate");   //!< Profiler category
} //namespace


//! \brief  Constructor
HdVP2Mesh::HdVP2Mesh(HdVP2RenderDelegate* delegate, const SdfPath& id, const SdfPath& instancerId)
 : HdMesh(id, instancerId)
 , _delegate(delegate) {
    const MHWRender::MVertexBufferDescriptor vbDesc(
        "", MHWRender::MGeometry::kPosition, MHWRender::MGeometry::kFloat, 3);
    _positionsBuffer.reset(new MHWRender::MVertexBuffer(vbDesc));
}

//! \brief  Destructor
HdVP2Mesh::~HdVP2Mesh() {
}

//! \brief  Synchronize VP2 state with scene delegate state based on dirty bits and representation
void HdVP2Mesh::Sync(
    HdSceneDelegate* delegate, HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits, const TfToken& reprToken) {

    MProfilingScope profilingScope(_profilerCategory, MProfiler::kColorC_L2, GetId().GetText(), "HdVP2Mesh Sync");

    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        _SetMaterialId(delegate->GetRenderIndex().GetChangeTracker(),
            delegate->GetMaterialId(GetId()));
    }

    _UpdateRepr(delegate, reprToken, dirtyBits);

    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

/*! \brief  Returns the minimal set of dirty bits to place in the
            change tracker for use in the first sync of this prim.
*/
HdDirtyBits HdVP2Mesh::GetInitialDirtyBitsMask() const {
    return HdChangeTracker::Clean | HdChangeTracker::InitRepr |
        HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyTopology |
        HdChangeTracker::DirtyTransform | HdChangeTracker::DirtyMaterialId |
        HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyVisibility;
}

/*! \brief  Add additional dirty bits

    This callback from Rprim gives the prim an opportunity to set
    additional dirty bits based on those already set.  This is done
    before the dirty bits are passed to the scene delegate, so can be
    used to communicate that extra information is needed by the prim to
    process the changes.

    The return value is the new set of dirty bits, which replaces the bits
    passed in.

    See HdRprim::PropagateRprimDirtyBits()
*/
HdDirtyBits HdVP2Mesh::_PropagateDirtyBits(HdDirtyBits bits) const {
    // If subdiv tags are dirty, topology needs to be recomputed.
    // The latter implies we'll need to recompute all primvar data.
    // Any data fetched by the scene delegate should be marked dirty here.
    if (bits & HdChangeTracker::DirtySubdivTags) {
        bits |= (HdChangeTracker::DirtyPoints |
            HdChangeTracker::DirtyNormals |
            HdChangeTracker::DirtyPrimvar |
            HdChangeTracker::DirtyTopology |
            HdChangeTracker::DirtyDisplayStyle);
    }
    else if (bits & HdChangeTracker::DirtyTopology) {
        // Unlike basis curves, we always request refineLevel when topology is
        // dirty
        bits |= HdChangeTracker::DirtySubdivTags |
            HdChangeTracker::DirtyDisplayStyle;
    }

    // A change of material means that the Quadrangulate state may have
    // changed.
    if (bits & HdChangeTracker::DirtyMaterialId) {
        bits |= (HdChangeTracker::DirtyPoints |
            HdChangeTracker::DirtyNormals |
            HdChangeTracker::DirtyPrimvar |
            HdChangeTracker::DirtyTopology);
    }

    // If points, display style, or topology changed, recompute normals.
    if (bits & (HdChangeTracker::DirtyPoints |
        HdChangeTracker::DirtyDisplayStyle |
        HdChangeTracker::DirtyTopology)) {
        bits |= _customDirtyBitsInUse &
            (DirtySmoothNormals | DirtyFlatNormals);
    }

    // If the topology is dirty, recompute custom indices resources.
    if (bits & HdChangeTracker::DirtyTopology) {
        bits |= _customDirtyBitsInUse &
            (DirtyIndices |
                DirtyHullIndices |
                DirtyPointsIndices);
    }

    // If normals are dirty and we are doing CPU normals
    // then the normals computation needs the points primvar
    // so mark points as dirty, so that the scene delegate will provide
    // the data.
    if ((bits & (DirtySmoothNormals | DirtyFlatNormals))/* &&
        !HdStGLUtils::IsGpuComputeEnabled()*/) {
        bits |= HdChangeTracker::DirtyPoints;
    }

    // Sometimes we don't get dirty extent notification
    if (bits & (HdChangeTracker::DirtyPoints)) {
        bits |= HdChangeTracker::DirtyExtent;
    }

    return bits;
}

/*! \brief  Initialize the given representation of this Rprim.

    This is called prior to syncing the prim, the first time the repr
    is used.

    \param  reprToken   the name of the repr to initalize.  HdRprim has already
                        resolved the reprName to its final value.

    \param  dirtyBits   an in/out value.  It is initialized to the dirty bits
                        from the change tracker.  InitRepr can then set additional
                        dirty bits if additional data is required from the scene
                        delegate when this repr is synced.

    InitRepr occurs before dirty bit propagation.

    See HdRprim::InitRepr()
*/
void HdVP2Mesh::_InitRepr(const TfToken& reprToken, HdDirtyBits* dirtyBits) {
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(), _ReprComparator(reprToken));

    // If it's not new, nothing to init
    if (it != _reprs.end())
        return;

    // else we have a new repr
    _reprs.emplace_back(reprToken, boost::make_shared<HdRepr>());
    HdReprSharedPtr &repr = _reprs.back().second;

    // set dirty bit to say we need to sync a new repr (buffer array
    // ranges may change)
    *dirtyBits |= HdChangeTracker::NewRepr;

    auto* const param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    MSubSceneContainer* subSceneContainer = param->GetContainer();
    if (!subSceneContainer)
        return;

    _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);

    for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
        const HdMeshReprDesc& desc = descs[descIdx];

        size_t numDrawItems = _GetNumDrawItemsForDesc(desc);
        if (numDrawItems == 0) continue;

        for (size_t itemId = 0; itemId < numDrawItems; itemId++) {
            auto* drawItem = new HdVP2DrawItem(&_sharedData, desc);
            repr->AddDrawItem(drawItem);

            const MString& renderItemName = drawItem->GetRenderItemName();

            MHWRender::MRenderItem* const renderItem =
                (desc.geomStyle == HdMeshGeomStylePoints) ?
                _CreatePointsRenderItem(renderItemName) :
                _CreateSmoothHullRenderItem(renderItemName);

            _delegate->GetVP2ResourceRegistry().EnqueueCommit(
                [subSceneContainer, renderItem]() {
                    subSceneContainer->add(renderItem);
                }
            );
        }

        if (desc.geomStyle != HdMeshGeomStylePoints) {
            if (desc.flatShadingEnabled) {
                if (!(_customDirtyBitsInUse & DirtyFlatNormals)) {
                    _customDirtyBitsInUse |= DirtyFlatNormals;
                    *dirtyBits |= DirtyFlatNormals;
                }
            }
            else {
                if (!(_customDirtyBitsInUse & DirtySmoothNormals)) {
                    _customDirtyBitsInUse |= DirtySmoothNormals;
                    *dirtyBits |= DirtySmoothNormals;
                }
            }
        }
    }
}

/*! \brief  Update the named repr object for this Rprim.

    Repr objects are created to support specific reprName tokens, and contain a list of
    HdVP2DrawItems and corresponding RenderItems.
*/
void HdVP2Mesh::_UpdateRepr(HdSceneDelegate *sceneDelegate, const TfToken& reprToken, HdDirtyBits *dirtyBits) {

    HdReprSharedPtr const &curRepr = _GetRepr(reprToken);
    if (!curRepr) {
        return;
    }

    _MeshReprConfig::DescArray reprDescs = _GetReprDesc(reprToken);

    // Iterate through all reprdescs for the current repr to figure out if any
    // of them requires smooth normals or flat normals. If either (or both)
    // are required, we will calculate them once and clean the bits.
    bool requireSmoothNormals = false;
    bool requireFlatNormals = false;
    for (size_t descIdx = 0; descIdx < reprDescs.size(); ++descIdx) {
        const HdMeshReprDesc &desc = reprDescs[descIdx];
        if (desc.geomStyle != HdMeshGeomStylePoints) {
            if (desc.flatShadingEnabled) {
                requireFlatNormals = true;
            }
            else {
                requireSmoothNormals = true;
            }
        }
    }

    // For each relevant draw item, update dirty buffer sources.
    int drawItemIndex = 0;
    for (size_t descIdx = 0; descIdx < reprDescs.size(); ++descIdx) {
        const HdMeshReprDesc &desc = reprDescs[descIdx];
        size_t numDrawItems = _GetNumDrawItemsForDesc(desc);
        if (numDrawItems == 0) continue;

        for (size_t itemId = 0; itemId < numDrawItems; itemId++) {
            auto* drawItem = static_cast<HdVP2DrawItem*>(
                curRepr->GetDrawItem(drawItemIndex++)
            );

            if (HdChangeTracker::IsDirty(*dirtyBits)) {
                _UpdateDrawItem(sceneDelegate, drawItem, dirtyBits, desc,
                    requireSmoothNormals, requireFlatNormals);
            }
        }
    }
}

namespace {
    //! \brief  Helper struct used to package all the changes into single commit task
    //!         (such commit task will be executed on main-thread)
    struct CommitState {
        HdVP2DrawItem::RenderItemData& _drawItemData;

        //! If valid, new position buffer data to commit
        float*	_positionBufferData{ nullptr };
        //! If valid, new index buffer data to commit
        int*	_indexBufferData{ nullptr };
        //! If valid, new normals buffer data to commit
        float*	_normalsBufferData{ nullptr };
        //! If valid, new UV buffer data to commit
        float*	_uvBufferData{ nullptr };
        //! If valid, new shader instance to set
        MHWRender::MShaderInstance* _surfaceShader{ nullptr };
        //! Instancing doesn't have dirty bits, every time we do update, we must update instance transforms
        MMatrixArray	_instanceTransforms;
        //! Is this object transparent
        bool			_isTransparent{ false };
        //! Capture of what has changed on this rprim
        HdDirtyBits _dirtyBits;

        //! No empty constructor, we need draw item and dirty bits.
        CommitState() = delete;
        //! Construct valid commit state
        CommitState(HdVP2DrawItem& drawItem, HdDirtyBits& dirtyBits)
            : _drawItemData(drawItem.GetRenderItemData())
            , _dirtyBits(dirtyBits) {}
    };
}

/*! \brief  Update the draw item

    This call happens on worker threads and results of the change are collected
    in CommitState and enqueued for Commit on main-thread using CommitTasks
*/
void HdVP2Mesh::_UpdateDrawItem(
    HdSceneDelegate* sceneDelegate,
    HdVP2DrawItem* drawItem,
    HdDirtyBits* dirtyBits,
    const HdMeshReprDesc &desc,
    bool requireSmoothNormals,
    bool requireFlatNormals
) {
    auto* const param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    MSubSceneContainer* subSceneContainer = param->GetContainer();
    if (!subSceneContainer)
        return;

    CommitState stateToCommit(*drawItem, *dirtyBits);
    HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._drawItemData;

    static constexpr bool writeOnly = true;

    VtValue valuePoints;
    HdMeshTopology meshTopology;
    bool hasMeshTopology = false;

    auto getTopologyFn = [
        this, sceneDelegate, &meshTopology, &hasMeshTopology
    ]() -> HdMeshTopology&
    {
        if (!hasMeshTopology) {
            meshTopology = GetMeshTopology(sceneDelegate);
            hasMeshTopology = true;
        }

        return meshTopology;
    };

    // Position buffer is shared among all render items of this rprim and will
    // be updated only once.
    const auto& meshId = GetId();
    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, meshId, HdTokens->points)) {
        valuePoints = sceneDelegate->Get(meshId, HdTokens->points);
        if(ARCH_LIKELY(valuePoints.IsHolding<VtVec3fArray>())) {
            const auto& points = valuePoints.Get<VtVec3fArray>();
            const GfVec3f* pointsData = points.cdata();

            stateToCommit._positionBufferData = static_cast<float*>(
                _positionsBuffer->acquire(points.size(), writeOnly)
            );
            memcpy(
                stateToCommit._positionBufferData,
                reinterpret_cast<const float*>(pointsData),
                points.size() * sizeof(float) * 3
            );
        }
    }

    // Points don't need an index buffer.
    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, meshId) && (desc.geomStyle != HdMeshGeomStylePoints)) {
        HdMeshTopology& topology = getTopologyFn();

        HdMeshUtil meshUtil(&topology, meshId);
        VtVec3iArray trianglesFaceVertexIndices;
        VtIntArray primitiveParam;
        VtVec3iArray trianglesEdgeIndices;
        meshUtil.ComputeTriangleIndices(&trianglesFaceVertexIndices, &primitiveParam, &trianglesEdgeIndices);

        const int numIndex = trianglesFaceVertexIndices.size() * 3;
        stateToCommit._indexBufferData = static_cast<int*>(
            drawItemData._indexBuffer->acquire(numIndex, writeOnly)
        );
        memcpy(stateToCommit._indexBufferData, trianglesFaceVertexIndices.data(), numIndex * sizeof(int));
    }

    if (requireSmoothNormals && (*dirtyBits & DirtySmoothNormals) && drawItemData._normalsBuffer) {
        // note: normals gets dirty when points are marked as dirty,
        // at change tracker.
        // clear DirtySmoothNormals (this is not a scene dirtybit)
        *dirtyBits &= ~DirtySmoothNormals;

        if (!valuePoints.IsHolding<VtVec3fArray>())
            valuePoints = sceneDelegate->Get(meshId, HdTokens->points);

        HdMeshTopology& topology = getTopologyFn();

        if (ARCH_LIKELY(valuePoints.IsHolding<VtVec3fArray>())) {
            const auto& points = valuePoints.Get<VtVec3fArray>();
            const GfVec3f* pointsData = points.cdata();

            Hd_VertexAdjacencySharedPtr adjacency(new Hd_VertexAdjacency());
            HdBufferSourceSharedPtr adjacencyComputation =
                adjacency->GetSharedAdjacencyBuilderComputation(&topology);

            adjacencyComputation->Resolve(); // IS the adjacency updated now?

            VtArray<GfVec3f> smoothNormals = Hd_SmoothNormals::ComputeSmoothNormals(
                adjacency.get(), topology.GetNumPoints(), pointsData
            );

            if(smoothNormals.size() == points.size())
            {
                stateToCommit._normalsBufferData = static_cast<float*>(
                    drawItemData._normalsBuffer->acquire(points.size(), writeOnly)
                    );
                memcpy(
                    stateToCommit._normalsBufferData,
                    reinterpret_cast<const float*>(smoothNormals.cdata()),
                    points.size() * sizeof(float) * 3
                );
            }
        }
    }

    auto material = drawItemData._uvBuffer ? 
        static_cast<const HdVP2Material*>(sceneDelegate->GetRenderIndex().GetSprim(HdPrimTypeTokens->material, GetMaterialId())) :
        nullptr;

    if (material)
    {
        const std::set<TfToken>& uvPrimvarNames =
            material->GetUVPrimvarNames();

        if (!uvPrimvarNames.empty()) {
            auto createUVs = [
                sceneDelegate, &meshId, dirtyBits,
                &stateToCommit, &drawItemData, &uvPrimvarNames
            ](
                HdInterpolation interpolation
            ) -> bool
            {
                for (const HdPrimvarDescriptor& primvar :
                    sceneDelegate->GetPrimvarDescriptors(meshId, interpolation)
                ) {
                    if (uvPrimvarNames.find(primvar.name) != uvPrimvarNames.end()
                        && HdChangeTracker::IsPrimvarDirty(
                            *dirtyBits, meshId, primvar.name
                        )
                    ) {
                        const auto& vt = sceneDelegate->Get(meshId, primvar.name);
                        if (vt.IsHolding<VtVec2fArray>()) {
                            const auto& uvArray = vt.UncheckedGet<VtVec2fArray>();
                            const GfVec2f* uvData = uvArray.cdata();

                            stateToCommit._uvBufferData = static_cast<float*>(
                                drawItemData._uvBuffer->acquire(
                                    uvArray.size(), writeOnly
                                )
                            );
                            memcpy(
                                stateToCommit._uvBufferData,
                                reinterpret_cast<const float*>(uvData),
                                uvArray.size() * sizeof(float) * 2
                            );

                            return true;
                        }
                    }
                }

                return false;
            };

            if (createUVs(HdInterpolation::HdInterpolationVertex)
                // MAYA-98542:
                // || createUVs(HdInterpolation::HdInterpolationFaceVarying)
            ) {
                drawItemData._hasUVs = true;
            }
        }
    }

    if ((desc.shadingTerminal == HdMeshReprDescTokens->surfaceShader) &&
        ((*dirtyBits & (HdChangeTracker::DirtyMaterialId | HdChangeTracker::NewRepr))
        || HdChangeTracker::IsPrimvarDirty(*dirtyBits, meshId, HdTokens->displayColor)
        || HdChangeTracker::IsPrimvarDirty(*dirtyBits, meshId, HdTokens->displayOpacity))
    ) {
        if (material && drawItemData._hasUVs)
        {
            stateToCommit._surfaceShader = material->GetSurfaceShader();
            // TODO: how to determine transparency for a Hydra material?
            stateToCommit._isTransparent = false;
        }

        if (!stateToCommit._surfaceShader)
        {
            bool foundColor = false;
            //bool foundOpacity = false;

            MColor mayaColor;

            const VtValue colorValue = sceneDelegate->Get(meshId, HdTokens->displayColor);
            if (!colorValue.IsEmpty()) {
                const VtVec3fArray colors = colorValue.Get<VtVec3fArray>();
                if (!colors.empty()) {
                    const float* colorPtr = colors.front().data();
                    mayaColor = MColor(colorPtr[0], colorPtr[1], colorPtr[2]);
                    foundColor = true;
                }
            }
#ifdef KXL_TO_FINISH // Something weird is happening when no displayOpacity is set on a prim, we are getting array with [0.0] instead of an empty array
            const VtValue opacityValue = sceneDelegate->Get(meshId, HdTokens->displayOpacity);
            if (!opacityValue.IsEmpty()) {
                const VtFloatArray opacitArray = opacityValue.Get<VtFloatArray>();
                if (!opacitArray.empty()) {
                    mayaColor.a = opacitArray[0];
                    foundOpacity = true;
                }
            }
#endif

            if (foundColor) {
                stateToCommit._surfaceShader = _delegate->GetFallbackShader(mayaColor);
                stateToCommit._isTransparent = (mayaColor.a < 1.0f);
            }
        }
    }

    // Bounding box is per-prim shared data.
    GfRange3d range;

    if (HdChangeTracker::IsExtentDirty(*dirtyBits, meshId)) {
        range = GetExtent(sceneDelegate);
        if (!range.IsEmpty()) {
            _sharedData.bounds.SetRange(range);
        }

        *dirtyBits &= ~HdChangeTracker::DirtyExtent;
    }
    else {
        range = _sharedData.bounds.GetRange();
    }

    const GfVec3d& min = range.GetMin();
    const GfVec3d& max = range.GetMax();
    const MBoundingBox bounds(MPoint(min[0], min[1], min[2]), MPoint(max[0], max[1], max[2]));

    // World matrix is per-prim shared data.
    GfMatrix4d transform;

    if (HdChangeTracker::IsTransformDirty(*dirtyBits, meshId)) {
        transform = sceneDelegate->GetTransform(meshId);
        _sharedData.bounds.SetMatrix(transform);

        *dirtyBits &= ~HdChangeTracker::DirtyTransform;
    }
    else {
        transform = _sharedData.bounds.GetMatrix();
    }

    MMatrix worldMatrix;
    transform.Get(worldMatrix.matrix);

    if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, meshId)) {
        _UpdateVisibility(sceneDelegate, dirtyBits);
    }

    // If the mesh is instanced, create one new instance per transform.
    // The current instancer invalidation tracking makes it hard for
    // us to tell whether transforms will be dirty, so this code
    // pulls them every time something changes.
    if (!GetInstancerId().IsEmpty()) {

        // Retrieve instance transforms from the instancer.
        HdRenderIndex& renderIndex = sceneDelegate->GetRenderIndex();
        HdInstancer *instancer =
            renderIndex.GetInstancer(GetInstancerId());
        VtMatrix4dArray transforms =
            static_cast<HdVP2Instancer*>(instancer)->
            ComputeInstanceTransforms(GetId());

        double instanceMatrix[4][4];
        for (size_t i = 0; i < transforms.size(); ++i) {
            transforms[i].Get(instanceMatrix);
            stateToCommit._instanceTransforms.append(MMatrix(instanceMatrix));
        }
    }

    // Capture class member for lambda
    MHWRender::MVertexBuffer* const positionsBuffer = _positionsBuffer.get();

    _delegate->GetVP2ResourceRegistry().EnqueueCommit(
        [drawItem, stateToCommit, param, positionsBuffer, bounds, worldMatrix]()
    {
        MProfilingScope profilingScope(
            _profilerCategory, MProfiler::kColorC_L2,
            drawItem->GetRenderItemId().GetText(), "HdVP2Mesh Commit Buffers"
        );

        MHWRender::MRenderItem* renderItem = param->GetContainer()->find(drawItem->GetRenderItemName());
        if(!renderItem) {
            TF_CODING_ERROR("Invalid render item: %s\n", drawItem->GetRenderItemId().GetText());
            return;
        }

        MHWRender::MVertexBuffer* normalsBuffer = stateToCommit._drawItemData._normalsBuffer.get();

        MHWRender::MVertexBuffer* uvBuffer = stateToCommit._drawItemData._hasUVs
            ? stateToCommit._drawItemData._uvBuffer.get() : nullptr;

        MHWRender::MIndexBuffer* indexBuffer = stateToCommit._drawItemData._indexBuffer.get();

        MHWRender::MVertexBufferArray vertexBuffers;
        vertexBuffers.addBuffer("positions", positionsBuffer);

        if (normalsBuffer)
            vertexBuffers.addBuffer("normals", normalsBuffer);

        if (uvBuffer)
            vertexBuffers.addBuffer("UVs", uvBuffer);

        // If available, something changed
        if(stateToCommit._positionBufferData)
            positionsBuffer->commit(stateToCommit._positionBufferData);

        // If available, something changed
        if (stateToCommit._normalsBufferData && normalsBuffer)
            normalsBuffer->commit(stateToCommit._normalsBufferData);

        // If available, something changed
        if (stateToCommit._uvBufferData && uvBuffer)
            uvBuffer->commit(stateToCommit._uvBufferData);

        // If available, something changed
        if (stateToCommit._indexBufferData && indexBuffer)
            indexBuffer->commit(stateToCommit._indexBufferData);

        // If available, something changed
        if (stateToCommit._surfaceShader) {
            renderItem->setShader(stateToCommit._surfaceShader);
            renderItem->setTreatAsTransparent(stateToCommit._isTransparent);
        }

        if ((stateToCommit._dirtyBits & HdChangeTracker::DirtyVisibility) != 0) {
            renderItem->enable(drawItem->GetVisible());
        }

        param->GetDrawScene().setGeometryForRenderItem(*renderItem, vertexBuffers, *indexBuffer, &bounds);
        renderItem->setMatrix(&worldMatrix);

        // Important, update instance transforms after setting geometry on render items!
        auto& oldInstanceCount = stateToCommit._drawItemData._instanceCount;
        auto newInstanceCount = stateToCommit._instanceTransforms.length();
        if(newInstanceCount > 0) {
            renderItem->setWantConsolidation(false);
            if(oldInstanceCount == newInstanceCount) {
                for (unsigned int i = 0; i < newInstanceCount; i++) {
                    param->GetDrawScene().updateInstanceTransform(*renderItem, i, stateToCommit._instanceTransforms[i]);
                }
            } else {
                param->GetDrawScene().setInstanceTransformArray(*renderItem, stateToCommit._instanceTransforms);
            }
            oldInstanceCount = newInstanceCount;
        } else if(oldInstanceCount > 0) {
            renderItem->setWantConsolidation(true);
            param->GetDrawScene().removeAllInstances(*renderItem);
            oldInstanceCount = 0;
        }
    });
}

/*! \brief  Create render item for points repr.
*/
MHWRender::MRenderItem* HdVP2Mesh::_CreatePointsRenderItem(const MString& name) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name,
        MHWRender::MRenderItem::DecorationItem,
        MHWRender::MGeometry::kPoints
    );

    renderItem->setDrawMode(MHWRender::MGeometry::kSelectionOnly);
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setWantConsolidation(true);
    renderItem->setShader(_delegate->Get3dFatPointShader());

    MSelectionMask selectionMask(MSelectionMask::kSelectPointsForGravity);
    selectionMask.addMask(MSelectionMask::kSelectMeshVerts);
    renderItem->setSelectionMask(selectionMask);

    return renderItem;
}

/*! \brief  Create render item for smoothHull repr.
*/
MHWRender::MRenderItem* HdVP2Mesh::_CreateSmoothHullRenderItem(const MString& name) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name,
        MHWRender::MRenderItem::MaterialSceneItem,
        MHWRender::MGeometry::kTriangles
    );

    renderItem->setExcludedFromPostEffects(false);
    renderItem->castsShadows(true);
    renderItem->receivesShadows(true);
    renderItem->setWantConsolidation(true);
    renderItem->setShader(_delegate->GetFallbackShader());
    renderItem->setSelectionMask(MSelectionMask::kSelectMeshes);
    return renderItem;
}

PXR_NAMESPACE_CLOSE_SCOPE
