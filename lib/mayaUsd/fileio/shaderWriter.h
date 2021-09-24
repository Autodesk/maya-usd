//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYA_SHADER_WRITER_H
#define PXRUSDMAYA_SHADER_WRITER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MFnDependencyNode.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class UsdAttribute;

/// Base class for USD prim writers that export Maya shading nodes as USD
/// shader prims.
class UsdMayaShaderWriter : public UsdMayaPrimWriter
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMayaShaderWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    /// The level of support a writer can offer for a given context
    ///
    /// A basic writer that gives correct results across most contexts should
    /// report `Fallback`, while a specialized writer that really shines in a
    /// given context should report `Supported` when the context is right and
    /// `Unsupported` if the context is not as expected.
    enum class ContextSupport
    {
        Supported,
        Fallback,
        Unsupported
    };

    /// A static function is expected for all shader writers and allows
    /// declaring how well this class can support the current context.
    ///
    /// The prototype is:
    ///
    /// static ContextSupport CanExport(const UsdMayaJobExportArgs& exportArgs,
    ///                                 const TfToken& currentMaterialConversion);

    /// Get the name of the USD shading attribute that corresponds to the
    /// Maya attribute named \p mayaAttrName.
    ///
    /// The attribute name should be the fully namespaced name in USD (e.g.
    /// "inputs:myInputAttribute" or "outputs:myOutputAttribute" for shader
    /// input and output attributes, respectively).
    ///
    /// The default implementation always returns an empty token, which
    /// effectively prevents any connections from being authored to or from
    /// the exported prims in USD. Derived classes should override this and
    /// return the corresponding attribute names for the Maya attributes
    /// that should be considered for connections.
    MAYAUSD_CORE_PUBLIC
    virtual TfToken GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName);

    /// Get the USD shading attribute that corresponds to the Maya attribute
    /// named \p mayaAttrName which will be connected to an attribute of type
    /// \p typeName. We pass the type of the other plug to allow exporters to
    /// add the proper conversion nodes if their syntax do not allow automatic
    /// typecasting.
    ///
    /// The default implementation calls
    /// GetShadingAttributeNameForMayaAttrName() with the given
    /// \p mayaAttrName and then attempts to get the USD attribute with that
    /// name from the shader writer's USD prim. Note that this means this
    /// method will only return valid USD attribute that the shader writer
    /// has already authored on its privately held UsdPrim, so this method
    /// should only be called after Write() has been called at least once.
    MAYAUSD_CORE_PUBLIC
    virtual UsdAttribute GetShadingAttributeForMayaAttrName(
        const TfToken&          mayaAttrName,
        const SdfValueTypeName& typeName);
};

typedef std::shared_ptr<UsdMayaShaderWriter> UsdMayaShaderWriterSharedPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
