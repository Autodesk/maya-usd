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
#ifndef PXRUSDTRANSLATORS_MATERIAL_WRITER_H
#define PXRUSDTRANSLATORS_MATERIAL_WRITER_H

/// \file

#include <mayaUsd/fileio/shaderWriter.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/valueTypeName.h>

#include <maya/MFnDependencyNode.h>

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;
class TfToken;
class UsdMayaWriteJobContext;
class UsdShadeShader;
class UsdTimeCode;

/// Shader writer for exporting Maya's material shading nodes to USD.
class PxrUsdTranslators_MaterialWriter : public UsdMayaShaderWriter
{
    public:
        PxrUsdTranslators_MaterialWriter(
                const MFnDependencyNode& depNodeFn,
                const SdfPath& usdPath,
                UsdMayaWriteJobContext& jobCtx);

        static ContextSupport CanExport(const UsdMayaJobExportArgs&);

        TfToken GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName) override;

    protected:
        /// Adds the schema attribute \p shaderInputName to the schema \p shaderSchema if the
        /// Maya attribute \p shadingNodeAttrName in dependency node \p depNodeFn has been modified
        /// or has an incoming connection at \p usdTime.
        ///
        /// If a specific SdfValueTypeName is desired for the created
        /// UsdShadeInput, it can be provided with the optional
        /// \p inputTypeName parameter. This is useful in cases where the role
        /// of the value type name may not be discoverable strictly from
        /// inspecting the Maya attribute plug (for example, determining that
        /// the "normalCamera" attributes of Maya shaders should be exported as
        /// Normal3f rather than just Float3).
        bool AuthorShaderInputFromShadingNodeAttr(
                const MFnDependencyNode& depNodeFn,
                const TfToken& shadingNodeAttrName,
                UsdShadeShader& shaderSchema,
                const TfToken& shaderInputName,
                const UsdTimeCode usdTime,
                const SdfValueTypeName& inputTypeName = SdfValueTypeName());

        /// Same as AuthorShaderInputFromShadingNodeAttr, but allows scaling the value using a float
        /// value found in the attribute \p scalingAttrName of the dependency node \p depNodeFn.
        ///
        bool AuthorShaderInputFromScaledShadingNodeAttr(
                const MFnDependencyNode& depNodeFn,
                const TfToken& shadingNodeAttrName,
                UsdShadeShader& shaderSchema,
                const TfToken& shaderInputName,
                const UsdTimeCode usdTime,
                const TfToken& scalingAttrName,
                const SdfValueTypeName& inputTypeName = SdfValueTypeName());

};

PXR_NAMESPACE_CLOSE_SCOPE


#endif
