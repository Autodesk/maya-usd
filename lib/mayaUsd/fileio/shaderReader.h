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
#ifndef PXRUSDMAYA_SHADER_READER_H
#define PXRUSDMAYA_SHADER_READER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReader.h>

#include <pxr/pxr.h>
#include <pxr/usd/usdshade/shader.h>

#include <maya/MObject.h>
#include <maya/MPlug.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;
class UsdMayaPrimReaderArgs;
class UsdMayaShadingModeImportContext;
class UsdShadeShader;

/// Base class for USD prim readers that import USD shader prims as Maya shading nodes.
class UsdMayaShaderReader : public UsdMayaPrimReader
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMayaShaderReader(const UsdMayaPrimReaderArgs&);

    /// The level of support a reader can offer for a given context
    ///
    /// A basic reader that gives correct results across most contexts should
    /// report `Fallback`, while a specialized reader that really shines in a
    /// given context should report `Supported` when the context is right and
    /// `Unsupported` if the context is not as expected.
    enum class ContextSupport
    {
        Supported,
        Fallback,
        Unsupported
    };

    /// This static function is expected for all shader readers and allows
    /// declaring how well this class can support the current context:
    MAYAUSD_CORE_PUBLIC
    static ContextSupport CanImport(const UsdMayaJobImportArgs& importArgs);

    /// Get the Maya shading plug on \p mayaObject that corresponds to the USD
    /// attribute named \p usdAttrName.
    ///
    /// The default implementation always returns an empty plug, which
    /// effectively prevents any connections from being authored to or from
    /// the imported shader nodes. Derived classes should override this and
    /// return the corresponding plugs for the USD attributes that should be
    /// considered for connections.
    ///
    /// Converters can potentially refine the plug returned by the downstream
    /// reader.
    MAYAUSD_CORE_PUBLIC
    virtual MPlug
    GetMayaPlugForUsdAttrName(const TfToken& usdAttrName, const MObject& mayaObject) const;

    /// Get the name of the Maya shading attribute that corresponds to the
    /// USD attribute named \p usdAttrName.
    ///
    /// The default implementation always returns an empty string, which
    /// effectively prevents any connections from being authored to or from
    /// the imported shader nodes. Derived classes should override this and
    /// return the corresponding attribute names for the USD attributes
    /// that should be considered for connections.
    MAYAUSD_CORE_PUBLIC
    virtual TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const;

    /// Callback after the shading context reader is done connecting all inputs
    MAYAUSD_CORE_PUBLIC
    virtual void PostConnectSubtree(UsdMayaPrimReaderContext* context);

    struct IsConverterResult
    {
        IsConverterResult(UsdShadeShader pdownstreamSchema, TfToken pdownstreamOutputName)
        {
            downstreamSchema = pdownstreamSchema;
            downstreamOutputName = pdownstreamOutputName;
        }
        UsdShadeShader downstreamSchema;
        TfToken        downstreamOutputName;
    };

    /// Is this a converter importer.
    ///
    /// Converters do not create any Maya object. They represent a UsdShade node which functions as
    /// a conversion utility (swizzle, typecasting) and can therefore not return any valid plug
    /// until a downstream non-converter node has been created.
    ///
    /// If the call returns true, the downstream node needed to complete the connection will be
    /// returned in \p downstreamSchema and the requested output will be in \p downstreamOutputName
    ///
    MAYAUSD_CORE_PUBLIC
    virtual boost::optional<IsConverterResult> IsConverter();

    /// Sets a downstream converter to use for caching calls to GetCreatedObject and
    /// GetMayaPlugForUsdAttrName
    ///
    MAYAUSD_CORE_PUBLIC
    virtual void SetDownstreamReader(std::shared_ptr<UsdMayaShaderReader> downstreamReader);

    /// Gets the Maya object that was created by this reader
    ///
    /// The default implementation always returns the object created for the
    /// prim found in the initial arguments.
    ///
    /// Converters are expected to pass the request to the downstream reader
    MAYAUSD_CORE_PUBLIC
    virtual MObject
    GetCreatedObject(const UsdMayaShadingModeImportContext&, const UsdPrim& prim) const;
};

typedef std::shared_ptr<UsdMayaShaderReader> UsdMayaShaderReaderSharedPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
