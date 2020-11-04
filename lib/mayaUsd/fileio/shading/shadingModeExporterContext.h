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
#ifndef PXRUSDMAYA_SHADING_MODE_EXPORTER_CONTEXT_H
#define PXRUSDMAYA_SHADING_MODE_EXPORTER_CONTEXT_H

#include <string>
#include <utility>
#include <vector>

#include <maya/MObject.h>
#include <maya/MPlug.h>

#include <pxr/pxr.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/types.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaShadingModeExportContext
{
public:
    void SetShadingEngine(const MObject& shadingEngine) {
        _shadingEngine = shadingEngine;
    }
    MObject GetShadingEngine() const { return _shadingEngine; }

    const UsdStageRefPtr& GetUsdStage() const { return _stage; }

    UsdMayaWriteJobContext& GetWriteJobContext() const {
        return _writeJobContext;
    }

    const UsdMayaJobExportArgs& GetExportArgs() const {
        return _writeJobContext.GetArgs();
    }

    bool GetMergeTransformAndShape() const {
        return GetExportArgs().mergeTransformAndShape;
    }
    const SdfPath& GetOverrideRootPath() const {
        return GetExportArgs().usdModelRootOverridePath;
    }
    const SdfPathSet& GetBindableRoots() const {
        return _bindableRoots;
    }

    MAYAUSD_CORE_PUBLIC
    void SetSurfaceShaderPlugName(const TfToken& surfaceShaderPlugName);
    MAYAUSD_CORE_PUBLIC
    void SetVolumeShaderPlugName(const TfToken& volumeShaderPlugName);
    MAYAUSD_CORE_PUBLIC
    void SetDisplacementShaderPlugName(const TfToken& displacementShaderPlugName);

    const UsdMayaUtil::MDagPathMap<SdfPath>& GetDagPathToUsdMap() const {
        return _dagPathToUsdMap;
    }

    MAYAUSD_CORE_PUBLIC
    MPlug GetSurfaceShaderPlug() const;
    MAYAUSD_CORE_PUBLIC
    MObject GetSurfaceShader() const;
    MAYAUSD_CORE_PUBLIC
    MPlug GetVolumeShaderPlug() const;
    MAYAUSD_CORE_PUBLIC
    MObject GetVolumeShader() const;
    MAYAUSD_CORE_PUBLIC
    MPlug GetDisplacementShaderPlug() const;
    MAYAUSD_CORE_PUBLIC
    MObject GetDisplacementShader() const;

    /// An assignment contains a bound prim path and a list of faceIndices.
    /// If the list of faceIndices is non-empty, then it is a partial assignment
    /// targeting a subset of the bound prim's faces.
    /// If the list of faceIndices is empty, it means the assignment targets
    /// all the faces in the bound prim or the entire bound prim.
    struct Assignment
    {
        SdfPath    boundPrimPath;
        VtIntArray faceIndices;
        TfToken    shapeName;
    };

    /// Vector of assignments.
    typedef std::vector<Assignment> AssignmentVector;

    /// Returns a vector of binding assignments associated with the shading
    /// engine.
    MAYAUSD_CORE_PUBLIC
    AssignmentVector GetAssignments() const;

    /// Use this function to create a UsdShadeMaterial prim at a "standard"
    /// location computed by browsing the \p assignmentsToBind
    MAYAUSD_CORE_PUBLIC
    UsdPrim MakeStandardMaterialPrim(
        const AssignmentVector& assignmentsToBind,
        const std::string&      name = std::string()) const;

    /// Use this function to bind a UsdShadeMaterial prim to all assignments provided.
    ///
    /// If \p boundPrimPaths is not NULL, it is populated with the set of
    /// prim paths that were bound to the created material prim, based on the
    /// given \p assignmentsToBind.
    MAYAUSD_CORE_PUBLIC
    void BindStandardMaterialPrim(
            const UsdPrim & materialPrim,
            const AssignmentVector& assignmentsToBind,
            SdfPathSet* const boundPrimPaths=nullptr) const;

    MAYAUSD_CORE_PUBLIC
    UsdMayaShadingModeExportContext(
            const MObject& shadingEngine,
            UsdMayaWriteJobContext& writeJobContext,
            const UsdMayaUtil::MDagPathMap<SdfPath>& dagPathToUsdMap);

private:
    MObject _shadingEngine;
    const UsdStageRefPtr& _stage;
    const UsdMayaUtil::MDagPathMap<SdfPath>& _dagPathToUsdMap;
    UsdMayaWriteJobContext& _writeJobContext;
    TfToken _surfaceShaderPlugName;
    TfToken _volumeShaderPlugName;
    TfToken _displacementShaderPlugName;

    /// Shaders that are bound to prims under \p _bindableRoot paths will get
    /// exported. If \p bindableRoots is empty, it will export all.
    SdfPathSet _bindableRoots;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
