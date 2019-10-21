//
// Copyright 2016 Pixar
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
#ifndef PXRUSDTRANSLATORS_MESH_WRITER_H
#define PXRUSDTRANSLATORS_MESH_WRITER_H

/// \file

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/gprim.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/primvar.h>
#include <pxr/usd/usdSkel/animation.h>

#include <maya/MBoundingBox.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MPlugArray.h>
#include <maya/MString.h>

#include <set>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Exports Maya mesh objects (MFnMesh)as UsdGeomMesh prims, taking into account
/// subd/poly, skinning, reference objects, UVs, and color sets.
class PxrUsdTranslators_MeshWriter : public UsdMayaPrimWriter
{
public:
    PxrUsdTranslators_MeshWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;
    bool ExportsGprims() const override;
    void PostExport() override;

private:
    bool writeMeshAttrs(const UsdTimeCode& usdTime, UsdGeomMesh& primSchema);

    void writeMotionVectors(
        UsdGeomMesh&       primSchema,
        const UsdTimeCode& usdTime,
        MFnMesh&           mesh,
        const std::string& colorSetName);

    /// Cleans up any extra data authored by SetPrimvar().
    void cleanupPrimvars();

    /// Whether the mesh is animated. For the time being, meshes on which
    /// skinning is being exported are considered to be non-animated.
    /// XXX In theory you could have an animated input mesh before the
    /// skinCluster is applied but we don't support that right now.
    bool isMeshAnimated() const;

    MObject writeBlendShapeData(UsdGeomMesh& primSchema);
    bool    writeBlendShapeAnimation(const UsdTimeCode& usdTime);
    bool    writeAnimatedMeshExtents(const MObject& deformedMesh, const UsdTimeCode& usdTime);

    /// Names for color sets that are interpreted as motion vectors.
    static const std::vector<std::string> motionVectorNames;

    /// Input mesh before any skeletal deformations, cached between iterations.
    MObject _skelInputMesh;

    /// The animated plugs of any blendshape nodes involved in mesh deformation.
    MPlugArray _animBlendShapeWeightPlugs;

    /// The previous sample for the mesh extents. Cached between iterations.
    VtVec3fArray _prevMeshExtentsSample;

    UsdSkelAnimation _skelAnim;

    /// Set of color sets that should be excluded.
    /// Intermediate processes may alter this set prior to writeMeshAttrs().
    std::set<std::string> _excludeColorSets;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
