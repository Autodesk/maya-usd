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
#include "exportContextRegistry.h"

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/fileio/registryHelper.h>

#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>

#include <unordered_map>
#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

using _ExportContextRegistry
    = std::unordered_map<TfToken, UsdMayaExportContextRegistry::ContextInfo, TfToken::HashFunctor>;
static _ExportContextRegistry _exportContextReg;

void UsdMayaExportContextRegistry::RegisterExportContext(
    const TfToken& exportContext,
    const TfToken& niceName,
    const TfToken& description,
    EnablerFn      enablerFct)
{
    TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg("Registering export context %s.\n", exportContext.GetText());

    std::pair<_ExportContextRegistry::iterator, bool> insertStatus
        = _exportContextReg.insert(_ExportContextRegistry::value_type(
            exportContext, ContextInfo { niceName, description, enablerFct }));
    if (insertStatus.second) {
        UsdMaya_RegistryHelper::AddUnloader(
            [exportContext]() { _exportContextReg.erase(exportContext); });
    } else {
        TF_CODING_ERROR("Multiple enablers for export context %s", exportContext.GetText());
    }
}

TfTokenVector UsdMayaExportContextRegistry::_ListExportContexts()
{
    UsdMaya_RegistryHelper::LoadExportContextPlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaExportContextRegistry>();
    TfTokenVector ret;
    ret.reserve(_exportContextReg.size());
    for (const auto& e : _exportContextReg) {
        ret.push_back(e.first);
    }
    return ret;
}

const UsdMayaExportContextRegistry::ContextInfo&
UsdMayaExportContextRegistry::_GetExportContextInfo(const TfToken& exportContext)
{
    UsdMaya_RegistryHelper::LoadExportContextPlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaExportContextRegistry>();
    auto                     it = _exportContextReg.find(exportContext);
    static const ContextInfo _emptyInfo;
    return it != _exportContextReg.end() ? it->second : _emptyInfo;
}

TF_INSTANTIATE_SINGLETON(UsdMayaExportContextRegistry);

UsdMayaExportContextRegistry& UsdMayaExportContextRegistry::GetInstance()
{
    return TfSingleton<UsdMayaExportContextRegistry>::GetInstance();
}

UsdMayaExportContextRegistry::UsdMayaExportContextRegistry() { }

UsdMayaExportContextRegistry::~UsdMayaExportContextRegistry() { }

PXR_NAMESPACE_CLOSE_SCOPE
