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

/// \file usdMaya/meshUtil.h

#ifndef PXRUSDMAYA_MESH_UTIL_H
#define PXRUSDMAYA_MESH_UTIL_H

#include "../../base/api.h"

#include "pxr/pxr.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"

#include <maya/MFnMesh.h>
#include <maya/MString.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdGeomMesh;

#define PXRUSDMAYA_MESH_COLOR_SET_TOKENS \
    ((DisplayColorColorSetName, "displayColor")) \
    ((DisplayOpacityColorSetName, "displayOpacity"))

TF_DECLARE_PUBLIC_TOKENS(UsdMayaMeshColorSetTokens,
    MAYAUSD_CORE_PUBLIC,
    PXRUSDMAYA_MESH_COLOR_SET_TOKENS);

/// Utilities for dealing with USD and RenderMan for Maya mesh/subdiv tags.
namespace UsdMayaMeshUtil
{
    /// Gets the internal emit-normals tag on the Maya \p mesh, placing it in
    /// \p value. Returns true if the tag exists on the mesh, and false if not.
    MAYAUSD_CORE_PUBLIC
    bool GetEmitNormalsTag(const MFnMesh &mesh, bool* value);

    /// Sets the internal emit-normals tag on the Maya \p mesh.
    /// This value indicates to the exporter whether it should write out the
    /// normals for the mesh to USD.
    MAYAUSD_CORE_PUBLIC
    void SetEmitNormalsTag(MFnMesh &meshFn, const bool emitNormals);

    /// Helper method for getting Maya mesh normals as a VtVec3fArray.
    MAYAUSD_CORE_PUBLIC
    bool GetMeshNormals(
        const MObject& mesh,
        VtArray<GfVec3f>* normalsArray,
        TfToken* interpolation);

    /// Gets the subdivision scheme tagged for the Maya mesh by consulting the
    /// adaptor for \c UsdGeomMesh.subdivisionSurface, and then falling back to
    /// the RenderMan for Maya attribute.
    MAYAUSD_CORE_PUBLIC
    TfToken GetSubdivScheme(const MFnMesh &mesh);

    /// Gets the subdivision interpolate boundary tagged for the Maya mesh by
    /// consulting the adaptor for \c UsdGeomMesh.interpolateBoundary, and then
    /// falling back to the RenderMan for Maya attribute.
    MAYAUSD_CORE_PUBLIC
    TfToken GetSubdivInterpBoundary(const MFnMesh &mesh);

    /// Gets the subdivision face-varying linear interpolation tagged for the
    /// Maya mesh by consulting the adaptor for
    /// \c UsdGeomMesh.faceVaryingLinearInterpolation, and then falling back to
    /// the OpenSubdiv2-style tagging.
    MAYAUSD_CORE_PUBLIC
    TfToken GetSubdivFVLinearInterpolation(const MFnMesh& mesh);

} // namespace UsdMayaMeshUtil


PXR_NAMESPACE_CLOSE_SCOPE

#endif
