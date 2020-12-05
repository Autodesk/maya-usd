//
// Copyright 2021 Apple
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
#ifndef IMPORTCHASERREGISTRY_H
#define IMPORTCHASERREGISTRY_H

#include "importChaser.h"

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>

#include <pxr/base/tf/declarePtrs.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/primFlags.h>

#include <maya/MDagPathArray.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(UsdMayaImportChaserRegistry);

/// \class UsdMayaImportChaserRegistry
/// \brief Registry for import chaser plugins.
///
/// We allow sites to register new chaser scripts that can be enabled on post-import.
///
/// Use USDMAYA_DEFINE_IMPORT_CHASER_FACTORY(name, ctx) to register a new chaser.
class UsdMayaImportChaserRegistry : public TfWeakBase
{
public:
    /// \brief Holds data that can be accessed when constructing a
    /// \p UsdMayaImportChaser object.
    ///
    /// This class allows plugin code to only know about the context object
    /// during construction. All the data it needs during construction should be passed in here.
    class FactoryContext
    {
    public:
        MAYAUSD_CORE_PUBLIC
        FactoryContext(
            Usd_PrimFlagsPredicate&     returnPredicate,
            const UsdStagePtr&          stage,
            const MDagPathArray&        dagPaths,
            const SdfPathVector&        sdfPaths,
            const UsdMayaJobImportArgs& jobArgs);

        /// \brief Returns a pointer to the imported stage object.
        MAYAUSD_CORE_PUBLIC
        UsdStagePtr GetStage() const;

        /// \brief Returns the top-level imported DAG paths that were imported.
        MAYAUSD_CORE_PUBLIC
        const MDagPathArray& GetImportedDagPaths() const;

        /// \brief Returns the top-level prims that were imported into the Maya scene.
        MAYAUSD_CORE_PUBLIC
        const SdfPathVector& GetImportedPrims() const;

        /// \brief Returns the arguments used for the import operation.
        MAYAUSD_CORE_PUBLIC
        const UsdMayaJobImportArgs& GetImportJobArgs() const;

    private:
        UsdStagePtr                 _stage;
        const MDagPathArray&        _dagPaths;
        const SdfPathVector&        _sdfPaths;
        const UsdMayaJobImportArgs& _jobArgs;
    };

    typedef std::function<UsdMayaImportChaser*(const FactoryContext&)> FactoryFn;

    /// \brief Register an import chaser factory.
    ///
    /// Please use the \p USDMAYA_DEFINE_IMPORT_CHASER_FACTORY instead of calling
    /// this directly.
    MAYAUSD_CORE_PUBLIC
    bool RegisterFactory(const char* name, FactoryFn fn);

    /// \brief Creates an import chaser using the factoring registered to \p name.
    MAYAUSD_CORE_PUBLIC
    UsdMayaImportChaserRefPtr Create(const char* name, const FactoryContext& context) const;

    /// \brief Returns the names of all registered import chasers.
    MAYAUSD_CORE_PUBLIC
    std::vector<std::string> GetAllRegisteredChasers() const;

    /// \brief Returns the registry that contains information about all registered import chasers.
    /// This registry is a global singleton instance.
    MAYAUSD_CORE_PUBLIC
    static UsdMayaImportChaserRegistry& GetInstance();

private:
    UsdMayaImportChaserRegistry();
    ~UsdMayaImportChaserRegistry();
    friend class TfSingleton<UsdMayaImportChaserRegistry>;
};

/// \brief define a factory for the import chaser \p name.  the \p contextArgName will
/// be type \p UsdImportMayaChaserRegistry::FactoryContext .  The following code
/// block should return a \p UsdMayaImportChaser*.  There are no guarantees about
/// the lifetime of \p contextArgName.
#define USDMAYA_DEFINE_IMPORT_CHASER_FACTORY(name, contextArgName)   \
    static UsdMayaImportChaser* _ImportChaserFactory_##name(         \
        const UsdMayaImportChaserRegistry::FactoryContext&);         \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaImportChaserRegistry, name) \
    {                                                                \
        UsdMayaImportChaserRegistry::GetInstance().RegisterFactory(  \
            #name, &_ImportChaserFactory_##name);                    \
    }                                                                \
    UsdMayaImportChaser* _ImportChaserFactory_##name(                \
        const UsdMayaImportChaserRegistry::FactoryContext& contextArgName)

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* IMPORTCHASERREGISTRY_H */
