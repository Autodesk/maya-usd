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
#ifndef PXRUSDMAYA_SCHEMA_API_WRITER_REGISTRY_H
#define PXRUSDMAYA_SCHEMA_API_WRITER_REGISTRY_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/schemaApiWriter.h>

#include <pxr/pxr.h>

#include <functional>
#include <map>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMayaSchemaApiWriterRegistry
/// \brief Provides functionality to register and lookup USD writer plugins
/// for Maya nodes.
///
/// PXRUSDMAYA_REGISTER_SCHEMA_API_WRITER(mayaTypeName, schemaApiName, writerClass) to register a
/// writer class with the registry.
///
/// The plugin is expected to add schema API to a prim previously written by an UsdMayaPrimWriter.
///
/// In order for the core system to discover the plugin, you need a
/// \c plugInfo.json that contains the Maya type name and the Maya plugin to
/// load:
/// \code
/// {
///     "UsdMaya": {
///         "SchemaApiWriter": {
///             "mayaPlugin": "myMayaPlugin",
///             "providesTranslator": [
///                 "myMayaType"
///             ]
///         }
///     }
/// }
/// \endcode
///
/// The registry contains information for both Maya built-in node types
/// and for any user-defined plugin types. If UsdMaya does not ship with a
/// writer plugin for some Maya built-in type, you can register your own
/// plugin for that Maya built-in type.
struct UsdMayaSchemaApiWriterRegistry
{
    /// Writer factory function, i.e. a function that creates a schema API writer for the given prim
    /// and context.
    using WriterFactoryFn = std::function<UsdMayaSchemaApiWriterSharedPtr(
        const UsdMayaPrimWriterSharedPtr&,
        UsdMayaWriteJobContext&)>;

    /// Map of all the SchemaApi and writers supported for a given mayaType name:
    using WriterFactoryFnMap = std::map<std::string, WriterFactoryFn>;

    /// \brief Register \p fn as a factory function providing a
    /// UsdMayaSchemaApiWriter subclass that can be used to write the \p schameApiName part of \p
    /// mayaType. If you can't provide a valid UsdMayaSchemaApiWriter for the given arguments,
    /// return a null pointer from the factory function \p fn.
    ///
    /// Example for registering a writer factory in your custom plugin:
    /// \code{.cpp}
    /// class MyWriter : public UsdMayaSchemaApiWriter {
    ///     static UsdMayaSchemaApiWriterSharedPtr Create(
    ///             const UsdMayaPrimWriterSharedPtr&,
    ///             UsdMayaWriteJobContext& jobCtx);
    /// };
    /// TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaSchemaApiWriterRegistry, MyWriter) {
    ///     UsdMayaSchemaApiWriterRegistry::Register("myCustomMayaNode", "SchemaApiName",
    ///             MyWriter::Create);
    /// }
    /// \endcode
    MAYAUSD_CORE_PUBLIC
    static void
    Register(const std::string& mayaType, const std::string& schemaApiName, WriterFactoryFn fn);

    /// \brief Finds all the schema api writers for a given \p mayaTypeName.
    ///
    MAYAUSD_CORE_PUBLIC
    static WriterFactoryFnMap Find(const std::string& mayaTypeName);
};

/// \brief Registers a pre-existing writer class for the given Maya type and API schema name;
/// the writer class should be a subclass of UsdMayaSchemaApiWriter with a two-place
/// constructor that takes <tt>(const UsdMayaPrimWriterSharedPtr&,
/// UsdMayaWriteJobContext& jobCtx)</tt> as arguments.
///
/// Example:
/// \code{.cpp}
/// class MyWriter : public UsdMayaPrimWriter {
///     MyWriter(
///             const UsdMayaPrimWriterSharedPtr&,
///             UsdMayaWriteJobContext& jobCtx) {
///         // ...
///     }
/// };
/// PXRUSDMAYA_REGISTER_SCHEMA_API_WRITER(myCustomMayaNode, schemaApiName, MyWriter);
/// \endcode
#define PXRUSDMAYA_REGISTER_SCHEMA_API_WRITER(mayaTypeName, schemaApiName, writerClass)         \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaSchemaApiWriterRegistry, mayaTypeName##_##writerClass) \
    {                                                                                           \
        UsdMayaSchemaApiWriterRegistry::Register(                                               \
            #mayaTypeName,                                                                      \
            #schemaApiName,                                                                     \
            [](const UsdMayaPrimWriterSharedPtr& primWriter, UsdMayaWriteJobContext& jobCtx) {  \
                return std::make_shared<writerClass>(primWriter, jobCtx);                       \
            });                                                                                 \
    }

PXR_NAMESPACE_CLOSE_SCOPE

#endif
