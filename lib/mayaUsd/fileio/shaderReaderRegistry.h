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
#ifndef PXRUSDMAYA_SHADER_READER_REGISTRY_H
#define PXRUSDMAYA_SHADER_READER_REGISTRY_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/shaderReader.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MFnDependencyNode.h>

#include <functional>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMayaShaderReaderRegistry
/// \brief Provides functionality to register and lookup USD shader reader
/// plugins for Maya nodes.
///
/// Use PXRUSDMAYA_DEFINE_SHADER_READER(usdInfoId, args, ctx) to define a new
/// reader function, or use
/// PXRUSDMAYA_REGISTER_SHADER_READER(usdInfoId, readerClass) to register a
/// reader class with the registry.
///
/// In order for the core system to discover the plugin, you need a
/// \c plugInfo.json that contains the usdInfoId and the Maya plugin to load:
/// \code
/// {
///     "UsdMaya": {
///         "ShaderReader": {
///             "mayaPlugin": "myMayaPlugin",
///             "providesTranslator": [
///                 "myCustomShaderId"
///             ]
///         }
///     }
/// }
/// \endcode
///
/// The registry contains information for both Maya built-in node types
/// and for any user-defined plugin types. If UsdMaya does not ship with a
/// reader plugin for some Maya built-in type, you can register your own
/// plugin for that Maya built-in type.
struct UsdMayaShaderReaderRegistry {
    /// Reader factory function, i.e. a function that creates a prim reader
    /// for the given prim reader args.
    typedef std::function<UsdMayaPrimReaderSharedPtr(const UsdMayaPrimReaderArgs&)> ReaderFactoryFn;

    /// Reader function, i.e. a function that reads a prim. This is the
    /// signature of the function declared in the PXRUSDMAYA_DEFINE_READER
    /// macro.
    typedef std::function<bool(const UsdMayaPrimReaderArgs&, UsdMayaPrimReaderContext*)> ReaderFn;

    /// \brief Register \p fn as a factory function providing a
    /// UsdMayaShaderReader subclass that can be used to read \p usdInfoId.
    /// If you can't provide a valid UsdMayaShaderReader for the given arguments,
    /// return a null pointer from the factory function \p fn.
    ///
    /// Example for registering a reader factory in your custom plugin:
    /// \code{.cpp}
    /// class MyReader : public UsdMayaShaderReader {
    ///     static UsdMayaPrimReaderSharedPtr Create(
    ///             const UsdMayaPrimReaderArgs&);
    /// };
    /// TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaShaderReaderRegistry, MyReader) {
    ///     UsdMayaShaderReaderRegistry::Register("myCustomInfoId",
    ///             MyReader::Create);
    /// }
    /// \endcode
    MAYAUSD_CORE_PUBLIC
    static void Register(const std::string& usdInfoId, ReaderFactoryFn fn);

    /// \brief Wraps \p fn in a ReaderFactoryFn and registers the wrapped
    /// function as a prim reader provider.
    /// This is a helper method for the macro PXRUSDMAYA_DEFINE_SHADER_READER;
    /// you probably want to use PXRUSDMAYA_DEFINE_SHADER_READER directly
    /// instead.
    MAYAUSD_CORE_PUBLIC
    static void RegisterRaw(const std::string& usdInfoId, ReaderFn fn);

    /// \brief Finds a reader if one exists for \p usdInfoId.
    ///
    /// If there is no reader plugin for \p usdInfoId, returns nullptr.
    MAYAUSD_CORE_PUBLIC
    static ReaderFactoryFn Find(const std::string& usdInfoId);
};

// Note, TF_REGISTRY_FUNCTION_WITH_TAG needs a type to register with so we
// create a dummy struct in the macro.

/// \brief Registers a pre-existing reader class for the given USD info:id;
/// the reader class should be a subclass of UsdMayaShaderReader with a
/// constructor that takes <tt>(const UsdMayaPrimReaderArgs& readerArgs)</tt>
/// as argument.
///
/// Example:
/// \code{.cpp}
/// class MyReader : public UsdMayaPrimReader {
///     MyReader(
///             const MFnDependencyNode& depNodeFn,
///             const SdfPath& usdPath,
///             UsdMayaReadeJobContext& jobCtx) {
///         // ...
///     }
/// };
/// PXRUSDMAYA_REGISTER_WRITER(myCustomMayaNode, MyReader);
/// \endcode
#define PXRUSDMAYA_REGISTER_SHADER_READER(usdInfoId, readerClass)                         \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaShaderReaderRegistry, usdInfoId##_##readerClass) \
    {                                                                                     \
        UsdMayaShaderReaderRegistry::Register(                                            \
            #usdInfoId, [](const UsdMayaPrimReaderArgs& readerArgs) {                     \
                return std::make_shared<readerClass>(readerArgs);                         \
            });                                                                           \
    }

PXR_NAMESPACE_CLOSE_SCOPE

#endif
