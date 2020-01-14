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
#include "shadingModeRegistry.h"

#include "../registryHelper.h"
#include "shadingModeExporter.h"
#include "shadingModeExporterContext.h"
#include "shadingModeImporter.h"

#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"

#include <map>
#include <string>
#include <utility>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(UsdMayaShadingModeTokens,
    PXRUSDMAYA_SHADINGMODE_TOKENS);


typedef std::map<TfToken, UsdMayaShadingModeExporterCreator> _ExportRegistry;
static _ExportRegistry _exportReg;

bool
UsdMayaShadingModeRegistry::RegisterExporter(
        const std::string& name,
        UsdMayaShadingModeExporterCreator fn)
{
    const TfToken nameToken(name);
    std::pair<_ExportRegistry::const_iterator, bool> insertStatus =
        _exportReg.insert(
            std::make_pair(nameToken, fn));
    if (insertStatus.second) {
        UsdMaya_RegistryHelper::AddUnloader([nameToken]() {
            _exportReg.erase(nameToken);
        });
    }
    else {
        TF_CODING_ERROR("Multiple shading exporters named '%s'", name.c_str());
    }
    return insertStatus.second;
}

UsdMayaShadingModeExporterCreator
UsdMayaShadingModeRegistry::_GetExporter(const TfToken& name)
{
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeExportContext>();
    const auto it = _exportReg.find(name);
    return it == _exportReg.end() ? nullptr : it->second;
}

typedef std::map<TfToken, UsdMayaShadingModeImporter> _ImportRegistry;
static _ImportRegistry _importReg;

bool
UsdMayaShadingModeRegistry::RegisterImporter(
        const std::string& name,
        UsdMayaShadingModeImporter fn)
{
    const TfToken nameToken(name);
    std::pair<_ImportRegistry::const_iterator, bool> insertStatus =
        _importReg.insert(
            std::make_pair(nameToken, fn));
    if (insertStatus.second) {
        UsdMaya_RegistryHelper::AddUnloader([nameToken]() {
            _importReg.erase(nameToken);
        });
    }
    else {
        TF_CODING_ERROR("Multiple shading importers named '%s'", name.c_str());
    }
    return insertStatus.second;
}

UsdMayaShadingModeImporter
UsdMayaShadingModeRegistry::_GetImporter(const TfToken& name)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeImportContext>();
    return _importReg[name];
}

TfTokenVector
UsdMayaShadingModeRegistry::_ListExporters() {
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeExportContext>();
    TfTokenVector ret;
    ret.reserve(_exportReg.size());
    for (const auto& e : _exportReg) {
        ret.push_back(e.first);
    }
    return ret;
}

TfTokenVector
UsdMayaShadingModeRegistry::_ListImporters() {
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeImportContext>();
    TfTokenVector ret;
    ret.reserve(_importReg.size());
    for (const auto& e : _importReg) {
        ret.push_back(e.first);
    }
    return ret;
}

TF_INSTANTIATE_SINGLETON(UsdMayaShadingModeRegistry);

UsdMayaShadingModeRegistry&
UsdMayaShadingModeRegistry::GetInstance()
{
    return TfSingleton<UsdMayaShadingModeRegistry>::GetInstance();
}

UsdMayaShadingModeRegistry::UsdMayaShadingModeRegistry()
{
}

UsdMayaShadingModeRegistry::~UsdMayaShadingModeRegistry()
{
}


PXR_NAMESPACE_CLOSE_SCOPE
