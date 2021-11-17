//
// Copyright 2021 Autodesk
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
#ifndef PXRUSDMAYA_SCHEMA_API_ADAPTOR_REGISTRY_H
#define PXRUSDMAYA_SCHEMA_API_ADAPTOR_REGISTRY_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/schemaApiAdaptor.h>

#include <pxr/pxr.h>

#include <functional>
#include <map>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMayaSchemaApiAdaptorRegistry
/// \brief Provides functionality to register and lookup USD adaptor plugins for Maya nodes.
///
/// PXRUSDMAYA_REGISTER_SCHEMA_API_ADAPTOR(mayaTypeName, schemaApiName, adaptorClass) to register a
/// adaptor class with the registry.
///
/// The plugin is expected to add schema API to a prim previously written by an UsdMayaPrimAdaptor.
///
/// In order for the core system to discover the plugin, you need a
/// \c plugInfo.json that contains the Maya type name and the Maya plugin to
/// load:
/// \code
/// {
///     "UsdMaya": {
///         "SchemaApiAdaptor": {
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
/// adaptor plugin for some Maya built-in type, you can register your own
/// plugin for that Maya built-in type.
struct UsdMayaSchemaApiAdaptorRegistry
{
    /// Adaptor factory function, i.e. a function that creates a schema API adaptor for the given
    /// prim and context.
    using AdaptorFactoryFn = std::function<UsdMayaSchemaApiAdaptorPtr(
        const MObjectHandle&     object,
        const TfToken&           schemaName,
        const UsdPrimDefinition* schemaPrimDef)>;

    /// Map of all the SchemaApi and adaptors supported for a given mayaType name and API schema:
    using AdaptorFactoryFnList = std::vector<AdaptorFactoryFn>;
    using AdaptorFactoryFnMap = std::map<std::string, AdaptorFactoryFnList>;

    /// \brief Register \p fn as a factory function providing a
    /// UsdMayaSchemaApiAdaptor subclass that can be used to write the \p schameApiName part of \p
    /// mayaType. If you can't provide a valid UsdMayaSchemaApiAdaptor for the given arguments,
    /// return a null pointer from the factory function \p fn.
    ///
    /// Example for registering a adaptor factory in your custom plugin:
    /// \code{.cpp}
    /// class MyAdaptor : public UsdMayaSchemaApiAdaptor {
    ///     static UsdMayaSchemaApiAdaptorPtr Create(
    ///             const MObjectHandle&     object,
    ///             const TfToken&           schemaName,
    ///             const UsdPrimDefinition* schemaPrimDef);
    /// };
    /// TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaSchemaApiAdaptorRegistry, MyAdaptor) {
    ///     UsdMayaSchemaApiAdaptorRegistry::Register("myCustomMayaNode", "SchemaApiName",
    ///             MyAdaptor::Create);
    /// }
    /// \endcode
    MAYAUSD_CORE_PUBLIC
    static void Register(
        const std::string& mayaType,
        const std::string& schemaApiName,
        AdaptorFactoryFn   fn,
        bool               fromPython = false);

    /// \brief Finds all the schema api adaptors for a given \p mayaTypeName.
    ///
    MAYAUSD_CORE_PUBLIC
    static AdaptorFactoryFnMap Find(const std::string& mayaTypeName);

    /// \brief Finds all the schema api adaptors for a given \p mayaTypeName and a given \p
    /// schemaApiName.
    ///
    MAYAUSD_CORE_PUBLIC
    static AdaptorFactoryFnList
    Find(const std::string& mayaTypeName, const std::string& schemaApiName);
};

/// \brief Registers a pre-existing adaptor class for the given Maya type and API schema name;
/// the adaptor class should be a subclass of UsdMayaSchemaApiAdaptor with a two-place
/// constructor that takes <tt>(const UsdMayaPrimAdaptorSharedPtr&,
/// UsdMayaWriteJobContext& jobCtx)</tt> as arguments.
///
/// Example:
/// \code{.cpp}
/// class MyAdaptor : public UsdMayaPrimAdaptor {
///     MyAdaptor(
///             const UsdMayaPrimAdaptorSharedPtr&,
///             UsdMayaWriteJobContext& jobCtx) {
///         // ...
///     }
/// };
/// PXRUSDMAYA_REGISTER_SCHEMA_API_ADAPTOR(myCustomMayaNode, schemaApiName, MyAdaptor);
/// \endcode
#define PXRUSDMAYA_REGISTER_SCHEMA_API_ADAPTOR(mayaTypeName, schemaApiName, adaptorClass)         \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaSchemaApiAdaptorRegistry, mayaTypeName##_##adaptorClass) \
    {                                                                                             \
        UsdMayaSchemaApiAdaptorRegistry::Register(                                                \
            #mayaTypeName,                                                                        \
            #schemaApiName,                                                                       \
            [](const MObjectHandle&     object,                                                   \
               const TfToken&           schemaName,                                               \
               const UsdPrimDefinition* schemaPrimDef) {                                          \
                return std::make_shared<adaptorClass>(object, schemaName, schemaPrimDef);         \
            });                                                                                   \
    }

PXR_NAMESPACE_CLOSE_SCOPE

#endif
