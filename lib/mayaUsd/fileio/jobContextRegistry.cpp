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
#include <unordered_set>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
struct _HashInfo
{
    unsigned long operator()(const UsdMayaJobContextRegistry::ContextInfo& info) const
    {
        return info.jobContext.Hash();
    }
};

struct _EqualInfo
{
    bool operator()(
        const UsdMayaJobContextRegistry::ContextInfo& lhs,
        const UsdMayaJobContextRegistry::ContextInfo& rhs) const
    {
        return rhs.jobContext == lhs.jobContext;
    }
};

using _JobContextRegistry
    = std::unordered_set<UsdMayaJobContextRegistry::ContextInfo, _HashInfo, _EqualInfo>;
static _JobContextRegistry _jobContextReg;
} // namespace

void UsdMayaJobContextRegistry::RegisterExportJobContext(
    const std::string& jobContext,
    const std::string& niceName,
    const std::string& description,
    EnablerFn          enablerFct)
{
    TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg("Registering export job context %s.\n", jobContext.c_str());
    TfToken     key(jobContext);
    ContextInfo newInfo { key, TfToken(niceName), TfToken(description), enablerFct, {}, {} };
    auto        itFound = _jobContextReg.find(newInfo);
    if (itFound == _jobContextReg.end()) {
        _jobContextReg.insert(newInfo);
        UsdMaya_RegistryHelper::AddUnloader([key]() {
            ContextInfo toErase { key, {}, {}, {}, {}, {} };
            _jobContextReg.erase(toErase);
        });
    } else {
        if (!itFound->exportEnablerCallback) {
            if (niceName != itFound->niceName) {
                TF_CODING_ERROR(
                    "Export enabler has differing nice name: %s != %s",
                    niceName.c_str(),
                    itFound->niceName.GetText());
            }
            // Fill the export part:
            ContextInfo updatedInfo {
                key,        itFound->niceName,          TfToken(description),
                enablerFct, itFound->importDescription, itFound->importEnablerCallback
            };
            _jobContextReg.erase(updatedInfo);
            _jobContextReg.insert(updatedInfo);
        } else {
            TF_CODING_ERROR("Multiple enablers for export job context %s", jobContext.c_str());
        }
    }
}

void UsdMayaJobContextRegistry::RegisterImportJobContext(
    const std::string& jobContext,
    const std::string& niceName,
    const std::string& description,
    EnablerFn          enablerFct)
{
    TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg("Registering import job context %s.\n", jobContext.c_str());
    TfToken     key(jobContext);
    ContextInfo newInfo { key, TfToken(niceName), {}, {}, TfToken(description), enablerFct };
    auto        itFound = _jobContextReg.find(newInfo);
    if (itFound == _jobContextReg.end()) {
        _jobContextReg.insert(newInfo);
        UsdMaya_RegistryHelper::AddUnloader([key]() {
            ContextInfo toErase { key, {}, {}, {}, {}, {} };
            _jobContextReg.erase(toErase);
        });
    } else {
        if (!itFound->importEnablerCallback) {
            if (niceName != itFound->niceName) {
                TF_CODING_ERROR(
                    "Import enabler has differing nice name: %s != %s",
                    niceName.c_str(),
                    itFound->niceName.GetText());
            }
            // Fill the import part:
            ContextInfo updatedInfo { key,
                                      itFound->niceName,
                                      itFound->exportDescription,
                                      itFound->exportEnablerCallback,
                                      TfToken(description),
                                      enablerFct };
            _jobContextReg.erase(updatedInfo);
            _jobContextReg.insert(updatedInfo);
        } else {
            TF_CODING_ERROR("Multiple enablers for import job context %s", jobContext.c_str());
        }
    }
}

TfTokenVector UsdMayaJobContextRegistry::_ListExportJobContexts()
{
    UsdMaya_RegistryHelper::LoadJobContextPlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaJobContextRegistry>();
    TfTokenVector ret;
    ret.reserve(_jobContextReg.size());
    for (const auto& e : _jobContextReg) {
        if (e.exportEnablerCallback) {
            ret.push_back(e.jobContext);
        }
    }
    return ret;
}

TfTokenVector UsdMayaJobContextRegistry::_ListImportJobContexts()
{
    UsdMaya_RegistryHelper::LoadJobContextPlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaJobContextRegistry>();
    TfTokenVector ret;
    ret.reserve(_jobContextReg.size());
    for (const auto& e : _jobContextReg) {
        if (e.importEnablerCallback) {
            ret.push_back(e.jobContext);
        }
    }
    return ret;
}

const UsdMayaJobContextRegistry::ContextInfo&
UsdMayaJobContextRegistry::_GetJobContextInfo(const TfToken& jobContext)
{
    UsdMaya_RegistryHelper::LoadJobContextPlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaJobContextRegistry>();
    ContextInfo              key { jobContext, {}, {}, {}, {}, {} };
    auto                     it = _jobContextReg.find(key);
    static const ContextInfo _emptyInfo;
    return it != _jobContextReg.end() ? *it : _emptyInfo;
}

TF_INSTANTIATE_SINGLETON(UsdMayaJobContextRegistry);

UsdMayaJobContextRegistry& UsdMayaJobContextRegistry::GetInstance()
{
    return TfSingleton<UsdMayaJobContextRegistry>::GetInstance();
}

UsdMayaJobContextRegistry::UsdMayaJobContextRegistry() { }

UsdMayaJobContextRegistry::~UsdMayaJobContextRegistry() { }

PXR_NAMESPACE_CLOSE_SCOPE
