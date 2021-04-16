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

MObject
UsdMayaShadingModeImportContext::CreateShadingEngine(const std::string& surfaceNodeName) const
{
    const TfToken shadingEngineName = GetShadingEngineName(surfaceNodeName);
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
        /* createNamespace = */ false,
        &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    return shadingEngine;
}

TfToken
UsdMayaShadingModeImportContext::GetShadingEngineName(const std::string& surfaceNodeName) const
{
    if (!_shadeMaterial && !_boundPrim) {
        return TfToken();
    }

    TfToken shadingEngineName;
    if (_shadeMaterial) {
        shadingEngineName = _shadeMaterial.GetPrim().GetName();
    } else if (_boundPrim) {
        // We have no material, so we definitely want to use the surfaceNodeName here:
        shadingEngineName = TfToken(UsdMayaUtil::SanitizeName(surfaceNodeName + "SG").c_str());
    }

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
