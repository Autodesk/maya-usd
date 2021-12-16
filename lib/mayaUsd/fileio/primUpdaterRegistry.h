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

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primUpdater.h>
#include <mayaUsd/fileio/primUpdaterContext.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MFnDependencyNode.h>

#include <functional>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMayaPrimUpdaterRegistry
/// \brief Provides functionality to register and lookup USD updater plugins
/// for Maya nodes.
///
/// Use PXRUSDMAYA_REGISTER_UPDATER(usdTypeName, mayaTypeName, updaterClass, supports)
/// to register a updater class with the registry.
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
    using UpdaterFactoryFn
        = std::function<UsdMayaPrimUpdaterSharedPtr(const MFnDependencyNode&, const Ufe::Path&)>;

    using RegisterItem = std::tuple<UsdMayaPrimUpdater::Supports, UpdaterFactoryFn>;

    /// \brief Register \p fn as a factory function providing a
    /// UsdMayaPrimUpdater subclass that can be used to update \p tfType \ \p mayaType.
    /// If you can't provide a valid UsdMayaPrimUpdater for the given arguments,
    /// return a null pointer from the factory function \p fn.
    ///
    /// Example for registering a updater factory in your custom plugin:
    /// \code{.cpp}
    /// class MyUpdater : public UsdMayaPrimUpdater {
    ///     static UsdMayaPrimUpdaterSharedPtr Create(
    ///             const MFnDependencyNode& depNodeFn,
    ///             const Ufe::Path& ufePath);
    /// };
    /// TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimUpdaterRegistry, MyUpdater) {
    ///     UsdMayaPrimUpdaterRegistry::Register(UsdMayaPrimUpdater::Supports,
    ///             MyUpdater::Create);
    /// }
    /// \endcode
    MAYAUSD_CORE_PUBLIC
    static void Register(
        const TfType&                tfType,
        const std::string&           mayaType,
        UsdMayaPrimUpdater::Supports sup,
        UpdaterFactoryFn             fn,
        bool                         fromPython = false);

    /// \brief Register \p fn as an updater provider for \p T.
    ///
    /// Example for registering an updater factory in your custom plugin, assuming
    /// that MyType is registered with the TfType system:
    /// \code{.cpp}
    /// class MyUpdater : public UsdMayaPrimUpdater {
    ///     static UsdMayaPrimReaderSharedPtr Create(
    ///             const UsdMayaPrimReaderArgs&);
    /// };
    /// TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimUpdaterRegistry, MyType) {
    ///     UsdMayaPrimUpdaterRegistry::Register<MyType>(MyReader::Create);
    /// }
    /// \endcode
    template <typename T>
    static void
    Register(const std::string& mayaType, UsdMayaPrimUpdater::Supports sup, UpdaterFactoryFn fn)
    {
        if (TfType t = TfType::Find<T>()) {
            Register(t, mayaType, sup, fn);
        } else {
            TF_CODING_ERROR("Cannot register unknown TfType: %s.", ArchGetDemangled<T>().c_str());
        }
    }

    /// \brief Finds a updater factory if one exists for \p usdTypeName or returns a fallback
    /// updater.
    ///
    /// \p usdTypeName should be a usd typeName, for example,
    /// \code
    /// prim.GetTypeName()
    /// \endcode
    MAYAUSD_CORE_PUBLIC
    static RegisterItem FindOrFallback(const TfToken& usdTypeName);

    /// \brief Finds a updater if one exists for \p mayaTypeName or returns a fallback updater.
    MAYAUSD_CORE_PUBLIC
    static RegisterItem FindOrFallback(const std::string& mayaTypeName);
};

/// \brief Registers a pre-existing updater class for the given Maya type;
/// the updater class should be a subclass of UsdMayaPrimUpdater with a three-place
/// constructor that takes <tt>(const MFnDependencyNode& depNodeFn,
/// const Ufe::Path& path,UsdMayaPrimUpdater::Supports sup)</tt> as arguments.
///
/// Example:
/// \code{.cpp}
/// class MyUpdater : public UsdMayaPrimUpdater {
///     MyUpdater(
///             const MFnDependencyNode& depNodeFn,
///             const Ufe::Path& path,
///             UsdMayaPrimUpdater::Supports sup) {
///         // ...
///     }
/// };
/// PXRUSDMAYA_REGISTER_UPDATER(myUsdTypeName, myMayaTypeName, MyUpdater,
///                    (UsdMayaPrimUpdater::Supports::Push | UsdMayaPrimUpdater::Supports::Clear)));
/// \endcode
#define PXRUSDMAYA_REGISTER_UPDATER(usdTypeName, mayaTypeName, updaterClass, supports)      \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimUpdaterRegistry, usdTypeName##_##updaterClass) \
    {                                                                                       \
        UsdMayaPrimUpdaterRegistry::Register<usdTypeName>(                                  \
            #mayaTypeName,                                                                  \
            supports,                                                                       \
            [](const MFnDependencyNode& depNodeFn, const Ufe::Path& path) {                 \
                return std::make_shared<updaterClass>(depNodeFn, path);                     \
            });                                                                             \
    }

PXR_NAMESPACE_CLOSE_SCOPE

#endif
