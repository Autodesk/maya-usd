#ifndef HD_VP2_MESH
#define HD_VP2_MESH

// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "pxr/pxr.h"
#include "pxr/imaging/hd/mesh.h"

#include "render_delegate.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdVP2DrawItem;
struct HdMeshReprDesc;

/*! \brief  VP2 representation of poly-mesh object.
    \class  HdVP2Mesh

    The prim object's main function is to bridge the scene description and the
    renderable representation. The Hydra image generation algorithm will call
    HdRenderIndex::SyncAll() before any drawing; this, in turn, will call
    Sync() for each mesh with new data.

    Sync() is passed a set of dirtyBits, indicating which scene buffers are
    dirty. It uses these to pull all of the new scene data and constructs
    updated embree geometry objects.  Commit of changed buffers to GPU happens
    in HdVP2RenderDelegate::CommitResources(), which runs on main-thread after
    all prims have been updated.
*/
class HdVP2Mesh final : public HdMesh {
public:
    HdVP2Mesh(HdVP2RenderDelegate*, const SdfPath&, const SdfPath& instancerId = SdfPath());

    ~HdVP2Mesh() override;

    void Sync(
        HdSceneDelegate*, HdRenderParam*,
        HdDirtyBits*, const TfToken& reprToken) override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

private:
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits) const override;

    void _InitRepr(const TfToken&, HdDirtyBits*) override;

    void _UpdateRepr(HdSceneDelegate*, const TfToken&, HdDirtyBits*);

    void _UpdateDrawItem(
        HdSceneDelegate*, HdVP2DrawItem*, HdDirtyBits*,
        const HdMeshReprDesc&, bool requireSmoothNormals, bool requireFlatNormals);

    //! Custom dirty bits used by this mesh
    enum DirtyBits : HdDirtyBits {
        DirtySmoothNormals = HdChangeTracker::CustomBitsBegin,
        DirtyFlatNormals = (DirtySmoothNormals << 1),
        DirtyIndices = (DirtyFlatNormals << 1),
        DirtyHullIndices = (DirtyIndices << 1),
        DirtyPointsIndices = (DirtyHullIndices << 1),
        DirtyBoundingBox = (DirtyPointsIndices << 1)
    };
    
    HdVP2RenderDelegate* _delegate{ nullptr };          //!< VP2 render delegate for which this material was created
    HdDirtyBits          _customDirtyBitsInUse{ 0 };    //!< Storage for custom dirty bits. See _PropagateDirtyBits for details.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
