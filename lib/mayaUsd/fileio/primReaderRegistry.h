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
#ifndef PXRUSDMAYA_PRIMREADERREGISTRY_H
#define PXRUSDMAYA_PRIMREADERREGISTRY_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReader.h>
#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>

#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMayaPrimReaderRegistry
/// \brief Provides functionality to register and lookup usd Maya reader
/// plugins.
///
/// Use PXRUSDMAYA_DEFINE_READER(MyUsdType, args, ctx) to register a new reader
/// for maya.
///
/// In order for the core system to discover the plugin, you should also
/// have a plugInfo.json file that contains the type and maya plugin to load:
/// \code
/// {
///     "UsdMaya": {
///         "PrimReader": {
///             "mayaPlugin": "myMayaPlugin",
///             "providesTranslator": [
///                 "MyUsdType"
///             ]
///         }
///     }
/// }
/// \endcode
struct UsdMayaPrimReaderRegistry
{
    /// Reader factory function, i.e. a function that creates a prim reader
    /// for the given prim reader args.
    typedef std::function<UsdMayaPrimReaderSharedPtr(const UsdMayaPrimReaderArgs&)> ReaderFactoryFn;

    /// Reader function, i.e. a function that reads a prim. This is the
    /// signature of the function declared in the PXRUSDMAYA_DEFINE_READER
    /// macro.
    typedef std::function<bool(const UsdMayaPrimReaderArgs&, UsdMayaPrimReaderContext&)> ReaderFn;

    /// \brief Register \p fn as a reader provider for \p type.
    MAYAUSD_CORE_PUBLIC
    static void Register(const TfType& type, ReaderFactoryFn fn, bool fromPython = false);

    /// \brief Register \p fn as a reader provider for \p T.
    ///
    /// Example for registering a reader factory in your custom plugin, assuming
    /// that MyType is registered with the TfType system:
    /// \code{.cpp}
    /// class MyReader : public UsdMayaPrimReader {
    ///     static UsdMayaPrimReaderSharedPtr Create(
    ///             const UsdMayaPrimReaderArgs&);
    /// };
    /// TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimReaderRegistry, MyType) {
    ///     UsdMayaPrimReaderRegistry::Register<MyType>(MyReader::Create);
    /// }
    /// \endcode
    template <typename T> static void Register(ReaderFactoryFn fn, bool fromPython = false)
    {
        if (TfType t = TfType::Find<T>()) {
            Register(t, fn, fromPython);
        } else {
            TF_CODING_ERROR("Cannot register unknown TfType: %s.", ArchGetDemangled<T>().c_str());
        }
    }

    /// \brief Wraps \p fn in a ReaderFactoryFn and registers that factory
    /// function as a reader provider for \p T.
    /// This is a helper method for the macro PXRUSDMAYA_DEFINE_READER;
    /// you probably want to use PXRUSDMAYA_DEFINE_READER directly instead.
    MAYAUSD_CORE_PUBLIC
    static void RegisterRaw(const TfType& type, ReaderFn fn);

    // takes a usdType (i.e. prim.GetTypeName())
    /// \brief Finds a reader factory if one exists for \p usdTypeName.
    ///
    /// \p usdTypeName should be a usd typeName, for example,
    /// \code
    /// prim.GetTypeName()
    /// \endcode
    MAYAUSD_CORE_PUBLIC
    static ReaderFactoryFn Find(const TfToken& usdTypeName);

    /// Similar to Find(), but returns a "fallback" prim reader factory if none
    /// can be found for \p usdTypeName. Thus, this always returns a valid
    /// reader factory.
    MAYAUSD_CORE_PUBLIC
    static ReaderFactoryFn FindOrFallback(const TfToken& usdTypeName);
};

// Lookup TfType by name instead of static C++ type when
// registering prim reader functions.
#define PXRUSDMAYA_DEFINE_READER(T, argsVarName, ctxVarName)                                     \
    static bool UsdMaya_PrimReader_##T(const UsdMayaPrimReaderArgs&, UsdMayaPrimReaderContext&); \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimReaderRegistry, T)                                  \
    {                                                                                            \
        if (TfType t = TfType::FindByName(#T)) {                                                 \
            UsdMayaPrimReaderRegistry::RegisterRaw(t, UsdMaya_PrimReader_##T);                   \
        } else {                                                                                 \
            TF_CODING_ERROR("Cannot register unknown TfType: %s.", #T);                          \
        }                                                                                        \
    }                                                                                            \
    bool UsdMaya_PrimReader_##T(                                                                 \
        const UsdMayaPrimReaderArgs& argsVarName, UsdMayaPrimReaderContext& ctxVarName)

// Lookup TfType by name instead of static C++ type when
// registering prim reader functions. This allows readers to be
// registered for codeless schemas, which are declared in the
// TfType system but have no corresponding C++ code.
#define PXRUSDMAYA_DEFINE_READER_FOR_USD_TYPE(T, argsVarName, ctxVarName)                        \
    static bool UsdMaya_PrimReader_##T(const UsdMayaPrimReaderArgs&, UsdMayaPrimReaderContext&); \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimReaderRegistry, T)                                  \
    {                                                                                            \
        const TfType& tfType = PlugRegistry::FindDerivedTypeByName<UsdSchemaBase>(#T);           \
        if (tfType) {                                                                            \
            UsdMayaPrimReaderRegistry::RegisterRaw(tfType, UsdMaya_PrimReader_##T);              \
        } else {                                                                                 \
            TF_CODING_ERROR("Cannot register unknown TfType for usdType: %s.", #T);              \
        }                                                                                        \
    }                                                                                            \
    bool UsdMaya_PrimReader_##T(                                                                 \
        const UsdMayaPrimReaderArgs& argsVarName, UsdMayaPrimReaderContext& ctxVarName)

PXR_NAMESPACE_CLOSE_SCOPE

#endif
