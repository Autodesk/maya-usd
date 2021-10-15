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
#include "jobContextRegistry.h"

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/fileio/registryHelper.h>

#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>

#include <string>
#include <unordered_map>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

using _JobContextRegistry
    = std::unordered_map<TfToken, UsdMayaJobContextRegistry::ContextInfo, TfToken::HashFunctor>;
static _JobContextRegistry _jobContextReg;

void UsdMayaJobContextRegistry::RegisterExportJobContext(
    const std::string& jobContext,
    const std::string& niceName,
    const std::string& description,
    EnablerFn          enablerFct)
{
    TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg("Registering export job context %s.\n", jobContext.c_str());
    TfToken                                        key(jobContext);
    std::pair<_JobContextRegistry::iterator, bool> insertStatus
        = _jobContextReg.insert(_JobContextRegistry::value_type(
            key, ContextInfo { niceName, description, enablerFct, {}, {} }));
    if (insertStatus.second) {
        UsdMaya_RegistryHelper::AddUnloader([key]() { _jobContextReg.erase(key); });
    } else {
        TF_CODING_ERROR("Multiple enablers for export job context %s", jobContext.c_str());
    }
}

TfTokenVector UsdMayaJobContextRegistry::_ListExportJobContexts()
{
    UsdMaya_RegistryHelper::LoadJobContextPlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaJobContextRegistry>();
    TfTokenVector ret;
    ret.reserve(_jobContextReg.size());
    for (const auto& e : _jobContextReg) {
        if (e.second.exportEnablerCallback) {
            ret.push_back(e.first);
        }
    }
    return ret;
}

const UsdMayaJobContextRegistry::ContextInfo&
UsdMayaJobContextRegistry::_GetJobContextInfo(const TfToken& jobContext)
{
    UsdMaya_RegistryHelper::LoadJobContextPlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaJobContextRegistry>();
    auto                     it = _jobContextReg.find(jobContext);
    static const ContextInfo _emptyInfo;
    return it != _jobContextReg.end() ? it->second : _emptyInfo;
}

TF_INSTANTIATE_SINGLETON(UsdMayaJobContextRegistry);

UsdMayaJobContextRegistry& UsdMayaJobContextRegistry::GetInstance()
{
    return TfSingleton<UsdMayaJobContextRegistry>::GetInstance();
}

UsdMayaJobContextRegistry::UsdMayaJobContextRegistry() { }

UsdMayaJobContextRegistry::~UsdMayaJobContextRegistry() { }

PXR_NAMESPACE_CLOSE_SCOPE
