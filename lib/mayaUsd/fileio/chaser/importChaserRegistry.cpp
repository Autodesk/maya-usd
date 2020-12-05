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
#include "importChaserRegistry.h"

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/fileio/registryHelper.h>

#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/tf/refPtr.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/pxr.h>

#include <__threading_support>
#include <iterator>
#include <map>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaImportChaserRegistry::FactoryContext::FactoryContext(
    Usd_PrimFlagsPredicate&     returnPredicate,
    const UsdStagePtr&          stage,
    const MDagPathArray&        dagPaths,
    const SdfPathVector&        sdfPaths,
    const UsdMayaJobImportArgs& jobArgs)
    : _stage(stage)
    , _dagPaths(dagPaths)
    , _sdfPaths(sdfPaths)
    , _jobArgs(jobArgs)
{
}

UsdStagePtr UsdMayaImportChaserRegistry::FactoryContext::GetStage() const { return this->_stage; }

const MDagPathArray& UsdMayaImportChaserRegistry::FactoryContext::GetImportedDagPaths() const
{
    return this->_dagPaths;
}

const SdfPathVector& UsdMayaImportChaserRegistry::FactoryContext::GetImportedPrims() const
{
    return this->_sdfPaths;
}

const UsdMayaJobImportArgs& UsdMayaImportChaserRegistry::FactoryContext::GetImportJobArgs() const
{
    return this->_jobArgs;
}

TF_INSTANTIATE_SINGLETON(UsdMayaImportChaserRegistry);

std::map<std::string, UsdMayaImportChaserRegistry::FactoryFn> _factoryImportRegistry;

UsdMayaImportChaserRegistry::UsdMayaImportChaserRegistry() { }
UsdMayaImportChaserRegistry::~UsdMayaImportChaserRegistry() { }

bool UsdMayaImportChaserRegistry::RegisterFactory(const char* name, FactoryFn fn)
{
    TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg("Registering import chaser '%s'.\n", name);
    std::pair<std::string, UsdMayaImportChaserRegistry::FactoryFn> p
        = std::make_pair(std::string(name), fn);
    auto ret = _factoryImportRegistry.insert(std::make_pair(std::string(name), fn));
    if (ret.second) {
        UsdMaya_RegistryHelper::AddUnloader([name]() { _factoryImportRegistry.erase(name); });
    }

    return ret.second;
}

UsdMayaImportChaserRefPtr
UsdMayaImportChaserRegistry::Create(const char* name, const FactoryContext& context) const
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaImportChaserRegistry>();
    if (UsdMayaImportChaserRegistry::FactoryFn fn = _factoryImportRegistry[name]) {
        return TfCreateRefPtr(fn(context));
    } else {
        return TfNullPtr;
    }
}

std::vector<std::string> UsdMayaImportChaserRegistry::GetAllRegisteredChasers() const
{
    std::vector<std::string> ret;
    for (const auto& p : _factoryImportRegistry) {
        ret.push_back(p.first);
    }
    return ret;
}

UsdMayaImportChaserRegistry& UsdMayaImportChaserRegistry::GetInstance()
{
    return TfSingleton<UsdMayaImportChaserRegistry>::GetInstance();
}

PXR_NAMESPACE_CLOSE_SCOPE
