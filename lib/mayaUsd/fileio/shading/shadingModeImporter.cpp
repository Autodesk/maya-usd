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
#include "shadingModeImporter.h"

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>

#include <maya/MFnSet.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdMayaShadingModeImporterTokens, PXRUSDMAYA_SHADING_MODE_IMPORTER_TOKENS);

bool UsdMayaShadingModeImportContext::GetCreatedObject(const UsdPrim& prim, MObject* obj) const
{
    if (!prim) {
        return false;
    }

    MObject node = _context->GetMayaNode(prim.GetPath(), false);
    if (!node.isNull()) {
        *obj = node;
        return true;
    }

    return false;
}

MObject UsdMayaShadingModeImportContext::AddCreatedObject(const UsdPrim& prim, const MObject& obj)
{
    if (prim) {
        return AddCreatedObject(prim.GetPath(), obj);
    }

    return obj;
}

MObject UsdMayaShadingModeImportContext::AddCreatedObject(const SdfPath& path, const MObject& obj)
{
    if (!path.IsEmpty()) {
        _context->RegisterNewMayaNode(path.GetString(), obj);
    }

    return obj;
}

MObject UsdMayaShadingModeImportContext::CreateShadingEngine() const
{
    const TfToken shadingEngineName = GetShadingEngineName();
    if (shadingEngineName.IsEmpty()) {
        return MObject();
    }

    MStatus        status;
    MFnSet         fnSet;
    MSelectionList tmpSelList;
    MObject        shadingEngine = fnSet.create(tmpSelList, MFnSet::kRenderableOnly, &status);
    if (status != MS::kSuccess) {
        TF_RUNTIME_ERROR("Failed to create shadingEngine: %s", shadingEngineName.GetText());
        return MObject();
    }

    fnSet.setName(
        shadingEngineName.GetText(),
        /* createNamespace = */ true,
        &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    return shadingEngine;
}

TfToken UsdMayaShadingModeImportContext::GetShadingEngineName() const
{
    if (!_shadeMaterial && !_boundPrim) {
        return TfToken();
    }

    if (!_shadingEngineName.IsEmpty()) {
        return _shadingEngineName;
    }

    TfToken primName;
    if (_shadeMaterial) {
        primName = _shadeMaterial.GetPrim().GetName();
    } else if (_boundPrim) {
        primName = _boundPrim.GetPrim().GetName();
    }

    // To make sure that the shadingEngine object names do not collide with
    // Maya transform or shape node names, we put the shadingEngine objects
    // into their own namespace.
    const TfToken shadingEngineName(TfStringPrintf(
        "%s:%s",
        UsdMayaShadingModeImporterTokens->MayaMaterialNamespace.GetText(),
        primName.GetText()));

    return shadingEngineName;
}

TfToken UsdMayaShadingModeImportContext::GetSurfaceShaderPlugName() const
{
    return _surfaceShaderPlugName;
}

TfToken UsdMayaShadingModeImportContext::GetVolumeShaderPlugName() const
{
    return _volumeShaderPlugName;
}

TfToken UsdMayaShadingModeImportContext::GetDisplacementShaderPlugName() const
{
    return _displacementShaderPlugName;
}

void UsdMayaShadingModeImportContext::SetShadingEngineName(const TfToken& shadingEngineName)
{
    _shadingEngineName = shadingEngineName;
}

void UsdMayaShadingModeImportContext::SetSurfaceShaderPlugName(const TfToken& surfaceShaderPlugName)
{
    _surfaceShaderPlugName = surfaceShaderPlugName;
}

void UsdMayaShadingModeImportContext::SetVolumeShaderPlugName(const TfToken& volumeShaderPlugName)
{
    _volumeShaderPlugName = volumeShaderPlugName;
}

void UsdMayaShadingModeImportContext::SetDisplacementShaderPlugName(
    const TfToken& displacementShaderPlugName)
{
    _displacementShaderPlugName = displacementShaderPlugName;
}

UsdMayaPrimReaderContext* UsdMayaShadingModeImportContext::GetPrimReaderContext() const
{
    return _context;
}

PXR_NAMESPACE_CLOSE_SCOPE
