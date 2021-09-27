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
#ifndef PXRUSDMAYA_SHADER_WRITER_REGISTRY_H
#define PXRUSDMAYA_SHADER_WRITER_REGISTRY_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primWriterArgs.h>
#include <mayaUsd/fileio/primWriterContext.h>
#include <mayaUsd/fileio/shaderWriter.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MFnDependencyNode.h>

#include <functional>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMayaShaderWriterRegistry
/// \brief Provides functionality to register and lookup USD writer plugins
/// for Maya shader nodes.
///
/// PXRUSDMAYA_REGISTER_SHADER_WRITER(mayaTypeName, writerClass) to register a writer
/// class with the registry.
///
/// The plugin is expected to create a shader at <tt>ctx->GetAuthorPath()</tt>.
///
/// In order for the core system to discover the plugin, you need a
/// \c plugInfo.json that contains the Maya type name:
/// \code
/// {
///   "Plugins": [
///     {
///       "Info": {
///         "UsdMaya": {
///         "ShaderWriter": {
///             "mayaPlugin": "myMayaPlugin", // (optional)
///             "providesTranslator": [
///                 "myMayaShaderType"
///             ]
///           }
///         }
///       },
///       "Name": "myUsdPlugin",
///       "LibraryPath": "../myUsdPlugin.[dll|dylib|so]",
///       "Type": "library"
///     }
///   ]
/// }
/// \endcode
///
/// If a mayaPlugin entry is provided, the plugin will be loaded via a call to loadPlugin inside
/// Maya. Otherwise, the plugin at LibraryPath will be loaded via the regular USD plugin loading
/// mechanism.
///
/// The registry contains information for both Maya built-in node types
/// and for any user-defined plugin types. If mayaUSD does not ship with a
/// writer plugin for some Maya built-in type, you can register your own
/// plugin for that Maya built-in type.
struct UsdMayaShaderWriterRegistry
{
    /// Writer factory function, i.e. a function that creates a shader writer
    /// for the given Maya node/USD paths and context.
    using WriterFactoryFn = std::function<UsdMayaShaderWriterSharedPtr(
        const MFnDependencyNode&,
        const SdfPath&,
        UsdMayaWriteJobContext&)>;

    /// Predicate function, i.e. a function that can tell the level of support
    /// the writer function will provide for a given set of export options.
    using ContextPredicateFn = std::function<
        UsdMayaShaderWriter::ContextSupport(const UsdMayaJobExportArgs&, const TfToken&)>;

    /// \brief Register \p fn as a factory function providing a
    /// UsdMayaShaderWriter subclass that can be used to write \p mayaType.
    /// If you can't provide a valid UsdMayaShaderWriter for the given arguments,
    /// return a null pointer from the factory function \p fn.
    MAYAUSD_CORE_PUBLIC
    static void Register(
        const TfToken&     mayaType,
        ContextPredicateFn pred,
        WriterFactoryFn    fn,
        bool               fromPython = false);

    /// \brief Finds a writer if one exists for \p mayaTypeName using the context found in
    /// \p exportArgs while exporting to \p currentMaterialConversion
    ///
    /// If there is no writer plugin for \p mayaTypeName, returns nullptr.
    MAYAUSD_CORE_PUBLIC
    static WriterFactoryFn Find(
        const TfToken&              mayaTypeName,
        const UsdMayaJobExportArgs& exportArgs,
        const TfToken&              currentMaterialConversion);
};

/// SFINAE utility class to detect the presence of a CanExport static function
/// inside a writer class. Used by the registration macro for basic writers.
template <typename T> class HasCanExport
{
    typedef char _One;
    struct _Two
    {
        char _x[2];
    };

    template <typename C> static _One _Test(decltype(&C::CanExport));
    template <typename C> static _Two _Test(...);

public:
    enum
    {
        value = sizeof(_Test<T>(0)) == sizeof(char)
    };
};

/// \brief Registers a pre-existing writer class for the given Maya type;
/// the writer class should be a subclass of UsdMayaShaderWriter with a three-place
/// constructor that takes <tt>(const MFnDependencyNode& depNodeFn,
/// const SdfPath& usdPath, UsdMayaWriteJobContext& jobCtx)</tt> as arguments.
/// The shader writer should also be able to declare which rendering contexts it
/// supports via an implementation of CanExport(const UsdMayaJobExportArgs&,
///                                             const TfToken&).
///
/// Example:
/// \code{.cpp}
/// class MyWriter : public UsdMayaShaderWriter {
///     MyWriter(
///             const MFnDependencyNode& depNodeFn,
///             const SdfPath& usdPath,
///             UsdMayaWriteJobContext& jobCtx) {
///         // ...
///     }
///     static ContextSupport CanExport(const UsdMayaJobExportArgs& exportArgs,
///                                     const TfToken& currentMaterialConversion) {
///         return renderContextIsRight ? ContextSupport::Supported
///                                     : ContextSupport::Unsupported
///     }
/// };
/// PXRUSDMAYA_REGISTER_SHADER_WRITER(myCustomMayaNode, MyWriter);
/// \endcode
#define PXRUSDMAYA_REGISTER_SHADER_WRITER(mayaTypeName, writerClass)                         \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaShaderWriterRegistry, mayaTypeName##_##writerClass) \
    {                                                                                        \
        static_assert(                                                                       \
            std::is_base_of<UsdMayaShaderWriter, writerClass>::value,                        \
            #writerClass " must derive from UsdMayaShaderWriter");                           \
        static_assert(                                                                       \
            HasCanExport<writerClass>::value,                                                \
            #writerClass " must define a static CanExport() function");                      \
        UsdMayaShaderWriterRegistry::Register(                                               \
            TfToken(#mayaTypeName),                                                          \
            &writerClass::CanExport,                                                         \
            [](const MFnDependencyNode& depNodeFn,                                           \
               const SdfPath&           usdPath,                                             \
               UsdMayaWriteJobContext&  jobCtx) {                                             \
                return std::make_shared<writerClass>(depNodeFn, usdPath, jobCtx);            \
            });                                                                              \
    }

PXR_NAMESPACE_CLOSE_SCOPE

#endif
