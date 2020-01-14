//
// Copyright 2016 Pixar
// Copyright 2019 Autodesk
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
#ifndef PXRUSDMAYA_PRIM_UPDATER_REGISTRY_H
#define PXRUSDMAYA_PRIM_UPDATER_REGISTRY_H

#include "pxr/pxr.h"
#include "../base/api.h"

#include "primUpdater.h"
#include "primUpdaterContext.h"

#include "pxr/usd/sdf/path.h"

#include <maya/MFnDependencyNode.h>

#include <functional>
#include <string>


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdMayaPrimUpdaterRegistry
/// \brief Provides functionality to register and lookup USD updater plugins
/// for Maya nodes.
///
/// Use PXRUSDMAYA_REGISTER_UPDATER(mayaTypeName, updaterClass) to register a updater
/// class with the registry.
///
/// The plugin is expected to update a prim at <tt>ctx->GetAuthorPath()</tt>.
///
/// In order for the core system to discover the plugin, you need a
/// \c plugInfo.json that contains the Maya type name and the Maya plugin to
/// load:
/// \code
/// {
///     "UsdMaya": {
///         "PrimUpdater": {
///             "mayaPlugin": "myMayaPlugin",
///             "providesTranslator": [
///                 "MyUsdType"
///             ]
///         }
///     }
/// }
/// \endcode
///
/// The registry contains information for both Maya built-in node types
/// and for any user-defined plugin types. If UsdMaya does not ship with a
/// updater plugin for some Maya built-in type, you can register your own
/// plugin for that Maya built-in type.
struct UsdMayaPrimUpdaterRegistry
{
    /// Updater factory function, i.e. a function that creates a prim updater
    /// for the given Maya node/USD paths and context.
    using UpdaterFactoryFn = std::function< UsdMayaPrimUpdaterSharedPtr (
            const MFnDependencyNode&,
            const SdfPath&) >;

    using RegisterItem = std::tuple<UsdMayaPrimUpdater::Supports, UpdaterFactoryFn>;

    /// \brief Register \p fn as a factory function providing a
    /// UsdMayaPrimUpdater subclass that can be used to update \p mayaType.
    /// If you can't provide a valid UsdMayaPrimUpdater for the given arguments,
    /// return a null pointer from the factory function \p fn.
    ///
    /// Example for registering a updater factory in your custom plugin:
    /// \code{.cpp}
    /// class MyUpdater : public UsdMayaPrimUpdater {
    ///     static UsdMayaPrimUpdaterSharedPtr Create(
    ///             const MFnDependencyNode& depNodeFn,
    ///             const SdfPath& usdPath);
    /// };
    /// TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimUpdaterRegistry, MyUpdater) {
    ///     UsdMayaPrimUpdaterRegistry::Register(UsdMayaPrimUpdater::Supports,
    ///             MyUpdater::Create);
    /// }
    /// \endcode
    MAYAUSD_CORE_PUBLIC
    static void Register(const TfType& type, UsdMayaPrimUpdater::Supports sup, UpdaterFactoryFn fn);

    /// \brief Register \p fn as a reader provider for \p T.
    ///
    /// Example for registering a reader factory in your custom plugin, assuming
    /// that MyType is registered with the TfType system:
    /// \code{.cpp}
    /// class MyReader : public UsdMayaPrimUpdater {
    ///     static UsdMayaPrimReaderSharedPtr Create(
    ///             const UsdMayaPrimReaderArgs&);
    /// };
    /// TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimUpdaterRegistry, MyType) {
    ///     UsdMayaPrimUpdaterRegistry::Register<MyType>(MyReader::Create);
    /// }
    /// \endcode
    template <typename T>
    static void Register(UsdMayaPrimUpdater::Supports sup, UpdaterFactoryFn fn)
    {
        if (TfType t = TfType::Find<T>()) {
            Register(t, sup, fn);
        }
        else {
            TF_CODING_ERROR("Cannot register unknown TfType: %s.",
                ArchGetDemangled<T>().c_str());
        }
    }

    // takes a usdType (i.e. prim.GetTypeName())
    /// \brief Finds a updater factory if one exists for \p usdTypeName.
    ///
    /// \p usdTypeName should be a usd typeName, for example, 
    /// \code
    /// prim.GetTypeName()
    /// \endcode
    MAYAUSD_CORE_PUBLIC
    static RegisterItem Find(const TfToken& usdTypeName);
};

/// \brief Registers a pre-existing updater class for the given Maya type;
/// the updater class should be a subclass of UsdMayaPrimUpdater with a three-place
/// constructor that takes <tt>(const MFnDependencyNode& depNodeFn,
/// const SdfPath& usdPath, UsdMayaWriteJobContext& jobCtx)</tt> as arguments.
///
/// Example:
/// \code{.cpp}
/// class MyUpdater : public UsdMayaPrimUpdater {
///     MyUpdater(
///             const MFnDependencyNode& depNodeFn,
///             const SdfPath& usdPath,
///             UsdMayaPrimUpdater::Supports sup) {
///         // ...
///     }
/// };
/// PXRUSDMAYA_REGISTER_UPDATER(myUsdTypeName, MyUpdater);
/// \endcode
#define PXRUSDMAYA_REGISTER_UPDATER(usdTypeName, updaterClass, supports) \
TF_REGISTRY_FUNCTION_WITH_TAG( \
        UsdMayaPrimUpdaterRegistry, \
        usdTypeName##_##updaterClass) \
{ \
    UsdMayaPrimUpdaterRegistry::Register<usdTypeName>( \
        supports, \
        []( \
                const MFnDependencyNode& depNodeFn, \
                const SdfPath& usdPath) { \
            return std::make_shared<updaterClass>(depNodeFn, usdPath); \
        }); \
}


PXR_NAMESPACE_CLOSE_SCOPE


#endif
