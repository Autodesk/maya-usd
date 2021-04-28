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
#ifndef PXRUSDMAYA_SHADING_MODE_IMPORTER_H
#define PXRUSDMAYA_SHADING_MODE_IMPORTER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>

#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/gprim.h>
#include <pxr/usd/usdShade/material.h>

#include <maya/MObject.h>

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaShadingModeImportContext
{
public:
    const UsdShadeMaterial& GetShadeMaterial() const { return _shadeMaterial; }
    const UsdGeomGprim&     GetBoundPrim() const { return _boundPrim; }

    UsdMayaShadingModeImportContext(
        const UsdShadeMaterial&   shadeMaterial,
        const UsdGeomGprim&       boundPrim,
        UsdMayaPrimReaderContext* context)
        : _shadeMaterial(shadeMaterial)
        , _boundPrim(boundPrim)
        , _context(context)
        , _surfaceShaderPlugName("surfaceShader")
        , _volumeShaderPlugName("volumeShader")
        , _displacementShaderPlugName("displacementShader")
    {
    }

    /// \name Reuse Objects on Import
    /// @{
    /// For example, if a shader node is used by multiple other nodes, you can
    /// use these functions to ensure that only 1 gets created.
    ///
    /// If your importer wants to try to re-use objects that were created by
    /// an earlier invocation (or by other things in the importer), you can
    /// add/retrieve them using these functions.

    /// This will return true and \p obj will be set to the
    /// previously created MObject.  Otherwise, this returns false;
    /// If \p prim is an invalid UsdPrim, this will return false.
    MAYAUSD_CORE_PUBLIC
    bool GetCreatedObject(const UsdPrim& prim, MObject* obj) const;

    /// If you want to register a prim so that other parts of the import
    /// uses them, this function registers \obj as being created.
    /// If \p prim is an invalid UsdPrim, nothing will get stored and \p obj
    /// will be returned.
    MAYAUSD_CORE_PUBLIC
    MObject AddCreatedObject(const UsdPrim& prim, const MObject& obj);

    /// If you want to register a path so that other parts of the import
    /// uses them, this function registers \obj as being created.
    /// If \p path is an empty SdfPath, nothing will get stored and \p obj
    /// will be returned.
    MAYAUSD_CORE_PUBLIC
    MObject AddCreatedObject(const SdfPath& path, const MObject& obj);
    /// @}

    /// Creates a shading engine (an MFnSet with the kRenderableOnly
    /// restriction).
    ///
    /// The shading engine's name is set using the value returned by
    /// GetShadingEngineName().
    MAYAUSD_CORE_PUBLIC
    MObject CreateShadingEngine(const std::string& surfaceNodeName) const;

    /// Gets the name of the shading engine that will be created for this
    /// context.
    ///
    /// If a shading engine name has been explicitly set on the context, that
    /// will be returned. Otherwise, the shading engine name is computed based
    /// on the context's material and/or bound prim.
    MAYAUSD_CORE_PUBLIC
    TfToken GetShadingEngineName(const std::string& surfaceNodeName) const;

    MAYAUSD_CORE_PUBLIC
    TfToken GetSurfaceShaderPlugName() const;
    MAYAUSD_CORE_PUBLIC
    TfToken GetVolumeShaderPlugName() const;
    MAYAUSD_CORE_PUBLIC
    TfToken GetDisplacementShaderPlugName() const;

    MAYAUSD_CORE_PUBLIC
    void SetSurfaceShaderPlugName(const TfToken& surfaceShaderPlugName);
    MAYAUSD_CORE_PUBLIC
    void SetVolumeShaderPlugName(const TfToken& volumeShaderPlugName);
    MAYAUSD_CORE_PUBLIC
    void SetDisplacementShaderPlugName(const TfToken& displacementShaderPlugName);

    /// Returns the primitive reader context for this shading mode import.
    ///
    MAYAUSD_CORE_PUBLIC
    UsdMayaPrimReaderContext* GetPrimReaderContext() const;

private:
    const UsdShadeMaterial&   _shadeMaterial;
    const UsdGeomGprim&       _boundPrim;
    UsdMayaPrimReaderContext* _context;

    TfToken _surfaceShaderPlugName;
    TfToken _volumeShaderPlugName;
    TfToken _displacementShaderPlugName;
};
typedef std::function<MObject(UsdMayaShadingModeImportContext*, const UsdMayaJobImportArgs&)>
    UsdMayaShadingModeImporter;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
