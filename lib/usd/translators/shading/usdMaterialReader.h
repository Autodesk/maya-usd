//
// Copyright 2020 Autodesk
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
#ifndef PXRUSDTRANSLATORS_MATERIAL_READER_H
#define PXRUSDTRANSLATORS_MATERIAL_READER_H

/// \file

#include <mayaUsd/fileio/shaderReader.h>

#include <pxr/pxr.h>

#include <maya/MFnDependencyNode.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaPrimReaderArgs;
class TfToken;
struct UsdMayaJobImportArgs;
class UsdShadeShader;

/// Shader reader for importing UsdPreviewSurface to Maya's native material nodes
class PxrUsdTranslators_MaterialReader : public UsdMayaShaderReader
{
public:
    PxrUsdTranslators_MaterialReader(const UsdMayaPrimReaderArgs&);

    /// Get the name of the Maya shading attribute that corresponds to the
    /// USD attribute named \p usdAttrName.
    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;

    /// Generic read function that traverses a schema and reads everything.
    bool Read(UsdMayaPrimReaderContext* context) override;

protected:
    /// What is the Maya node type name we want to convert to:
    virtual const TfToken& _GetMayaNodeTypeName() const = 0;

    /// Callback called before the attribute \p mayaAttribute is read from UsdShade. This allows
    /// setting back values in \p shaderFn that were lost during the export phase.
    virtual void
    _OnBeforeReadAttribute(const TfToken& mayaAttrName, MFnDependencyNode& shaderFn) const;

    /// Convert the value in \p usdValue from USD back to Maya following rules
    /// for attribute \p mayaAttrName
    virtual void _ConvertToMaya(const TfToken& mayaAttrName, VtValue& usdValue) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
