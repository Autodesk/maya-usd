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

#ifndef MAYAUSD_TRANSLATORMESH_H
#define MAYAUSD_TRANSLATORMESH_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReaderRegistry.h>

#include <pxr/pxr.h>
#include <pxr/usd/usdGeom/mesh.h>

#include <maya/MObject.h>
#include <maya/MString.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

/// Provides helper functions for translating UsdGeomMesh prims into Maya
/// meshes.
class MAYAUSD_CORE_PUBLIC TranslatorMeshRead
{
public:
    TranslatorMeshRead(
        const UsdGeomMesh&        mesh,
        const UsdPrim&            prim,
        const MObject&            transformObj,
        const MObject&            stageNode,
        const GfInterval&         frameRange,
        bool                      wantCacheAnimation,
        UsdMayaPrimReaderContext* context,
        MStatus*                  status = nullptr);

    ~TranslatorMeshRead() = default;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(TranslatorMeshRead);

    MObject meshObject() const;

    MObject blendObject() const;
    MObject pointBasedDeformerNode() const;
    MString pointBasedDeformerName() const;
    size_t  pointsNumTimeSamples() const;

    SdfPath shapePath() const;

private:
    MStatus setPointBasedDeformerForMayaNode(const MObject&, const MObject&, const UsdPrim&);

    static bool isPrimitiveLeftHanded(const UsdGeomGprim& prim);

private:
    MObject m_meshObj;
    MObject m_meshBlendObj;
    MObject m_pointBasedDeformerNode;
    MString m_newPointBasedDeformerName;
    bool    m_wantCacheAnimation;
    size_t  m_pointsNumTimeSamples;

    SdfPath m_shapePath;
};

class MAYAUSD_CORE_PUBLIC TranslatorMeshWrite
{
public:
    TranslatorMeshWrite(
        const MFnDependencyNode&,
        const UsdStageRefPtr&,
        const SdfPath&,
        const MDagPath&);

    ~TranslatorMeshWrite() = default;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(TranslatorMeshWrite);

    UsdGeomMesh usdMesh() const;

private:
    UsdGeomMesh m_usdMesh;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_TRANSLATORMESH_H
