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
#include "exportChaserRegistry.h"

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/fileio/registryHelper.h>

#include <pxr/base/tf/instantiateSingleton.h>

#include <map>

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaExportChaserRegistry::FactoryContext::FactoryContext(
    const UsdStagePtr&          stage,
    const DagToUsdMap&          dagToUsdMap,
    const UsdMayaJobExportArgs& jobArgs)
    : _stage(stage)
    , _dagToUsdMap(dagToUsdMap)
    , _jobArgs(jobArgs)
{
}

UsdStagePtr UsdMayaExportChaserRegistry::FactoryContext::GetStage() const { return _stage; }

const UsdMayaExportChaserRegistry::FactoryContext::DagToUsdMap&
UsdMayaExportChaserRegistry::FactoryContext::GetDagToUsdMap() const
{
    return _dagToUsdMap;
}

const UsdMayaJobExportArgs& UsdMayaExportChaserRegistry::FactoryContext::GetJobArgs() const
{
    return _jobArgs;
}

TF_INSTANTIATE_SINGLETON(UsdMayaExportChaserRegistry);

std::map<std::string, UsdMayaExportChaserRegistry::FactoryFn> _factoryRegistry;

bool UsdMayaExportChaserRegistry::RegisterFactory(
    const std::string& name,
    FactoryFn          fn,
    bool               fromPython)
{
    TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg("Registering chaser '%s'.\n", name.c_str());
    auto ret = _factoryRegistry.insert(std::make_pair(name, fn));
    if (ret.second) {
        UsdMaya_RegistryHelper::AddUnloader([name]() { _factoryRegistry.erase(name); }, fromPython);
    }
    return ret.second;
}

UsdMayaExportChaserRefPtr
UsdMayaExportChaserRegistry::Create(const std::string& name, const FactoryContext& context) const
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaExportChaserRegistry>();
    if (UsdMayaExportChaserRegistry::FactoryFn fn = _factoryRegistry[name]) {
        return TfCreateRefPtr(fn(context));
    } else {
        return TfNullPtr;
    }
}

std::vector<std::string> UsdMayaExportChaserRegistry::GetAllRegisteredChasers() const
{
    std::vector<std::string> ret;
    for (const auto& p : _factoryRegistry) {
        ret.push_back(p.first);
    }
    return ret;
}

// static
UsdMayaExportChaserRegistry& UsdMayaExportChaserRegistry::GetInstance()
{
    return TfSingleton<UsdMayaExportChaserRegistry>::GetInstance();
}

UsdMayaExportChaserRegistry::UsdMayaExportChaserRegistry() { }

UsdMayaExportChaserRegistry::~UsdMayaExportChaserRegistry() { }

PXR_NAMESPACE_CLOSE_SCOPE
