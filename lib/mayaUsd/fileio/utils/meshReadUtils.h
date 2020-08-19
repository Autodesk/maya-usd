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
// Modifications copyright (C) 2020 Autodesk
//
#ifndef PXRUSDMAYA_MESH_READ_UTILS_H
#define PXRUSDMAYA_MESH_READ_UTILS_H

#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>
#include <maya/MObject.h>
#include <maya/MString.h>

#include <pxr/pxr.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/mesh.h>

#include <mayaUsd/base/api.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdGeomMesh;

#define PXRUSDMAYA_MESH_PRIMVAR_TOKENS \
    ((DisplayColorColorSetName, "displayColor")) \
    ((DisplayOpacityColorSetName, "displayOpacity")) \
    ((DefaultMayaTexcoordName, "map1"))
    

TF_DECLARE_PUBLIC_TOKENS(UsdMayaMeshPrimvarTokens,
    MAYAUSD_CORE_PUBLIC,
    PXRUSDMAYA_MESH_PRIMVAR_TOKENS);

/// Utilities for dealing with USD and RenderMan for Maya mesh/subdiv tags.
namespace UsdMayaMeshReadUtils
{
    /// Gets the internal emit-normals tag on the Maya \p mesh, placing it in
    /// \p value. Returns true if the tag exists on the mesh, and false if not.
    MAYAUSD_CORE_PUBLIC
    bool getEmitNormalsTag(const MFnMesh &mesh, bool* value);

    /// Sets the internal emit-normals tag on the Maya \p mesh.
    /// This value indicates to the exporter whether it should write out the
    /// normals for the mesh to USD.
    MAYAUSD_CORE_PUBLIC
    void setEmitNormalsTag(MFnMesh &meshFn, const bool emitNormals);

    MAYAUSD_CORE_PUBLIC
    void assignPrimvarsToMesh(const UsdGeomMesh& mesh, 
                              const MObject& meshObj, 
                              const TfToken::Set& excludePrimvarSet);

    MAYAUSD_CORE_PUBLIC
    void assignInvisibleFaces(const UsdGeomMesh& mesh, const MObject& meshObj);

    MAYAUSD_CORE_PUBLIC
    MStatus assignSubDivTagsToMesh(const UsdGeomMesh&, MObject&, MFnMesh&);

} // namespace UsdMayaMeshReadUtils


PXR_NAMESPACE_CLOSE_SCOPE

#endif
