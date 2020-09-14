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

#include <map>
#include <string>
#include <utility>

#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>

#include <mayaUsd/fileio/registryHelper.h>
#include <mayaUsd/fileio/shading/shadingModeExporter.h>
#include <mayaUsd/fileio/shading/shadingModeExporterContext.h>
#include <mayaUsd/fileio/shading/shadingModeImporter.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdMayaShadingModeTokens,
    PXRUSDMAYA_SHADINGMODE_TOKENS);

namespace {
    static const std::string _kEmptyString;
}

struct _ShadingMode {
    std::string _niceName;
    std::string _description;
    UsdMayaShadingModeExporterCreator _fn;
};
using _ExportRegistry = std::map<TfToken, _ShadingMode>;
static _ExportRegistry _exportReg;

TF_DEFINE_PUBLIC_TOKENS(UsdMayaShadingConversionTokens,
    PXRUSDMAYA_SHADINGCONVERSION_TOKENS);

using _MaterialConversionRegistry = std::
    unordered_map<TfToken, UsdMayaShadingModeRegistry::ExportConversionInfo, TfToken::HashFunctor>;
static _MaterialConversionRegistry _conversionReg;

bool
UsdMayaShadingModeRegistry::RegisterExporter(
        const std::string& name,
        std::string niceName,
        std::string description,
        UsdMayaShadingModeExporterCreator fn)
{
    const TfToken nameToken(name);
    std::pair<_ExportRegistry::const_iterator, bool> insertStatus
        = _exportReg.insert(_ExportRegistry::value_type(
            nameToken, _ShadingMode { std::move(niceName), std::move(description), fn }));
    if (insertStatus.second) {
        UsdMaya_RegistryHelper::AddUnloader([nameToken]() {
            _exportReg.erase(nameToken);
        });
    } else {
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
    return it == _exportReg.end() ? nullptr : it->second._fn;
}

const std::string&
UsdMayaShadingModeRegistry::_GetExporterNiceName(const TfToken& name) {
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeExportContext>();
    const auto it = _exportReg.find(name);
    return it == _exportReg.end() ? _kEmptyString : it->second._niceName;
}

const std::string&
UsdMayaShadingModeRegistry::_GetExporterDescription(const TfToken& name) {
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeExportContext>();
    const auto it = _exportReg.find(name);
    return it == _exportReg.end() ? _kEmptyString : it->second._description;
}

using _ImportRegistry = std::map<TfToken, UsdMayaShadingModeImporter>;
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

TfTokenVector UsdMayaShadingModeRegistry::_ListExporters()
{
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

void UsdMayaShadingModeRegistry::RegisterExportConversion(
    const TfToken& materialConversion,
    const TfToken& renderContext,
    const TfToken& niceName,
    const TfToken& description)
{
    // It is perfectly valid to register the same material conversion more than once,
    // especially if exporters for a conversion are split across multiple libraries.
    // We will keep the first niceName registered.
    _conversionReg.insert(_MaterialConversionRegistry::value_type(
        materialConversion,
        ExportConversionInfo { renderContext, niceName, description }));
}

TfTokenVector
UsdMayaShadingModeRegistry::_ListExportConversions() {
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeExportContext>();
    TfTokenVector ret;
    ret.reserve(_conversionReg.size());
    for (const auto& e : _conversionReg) {
        ret.push_back(e.first);
    }
    return ret;
}

const UsdMayaShadingModeRegistry::ExportConversionInfo&
UsdMayaShadingModeRegistry::_GetExportConversionInfo(const TfToken& materialConversion)
{
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeExportContext>();
    auto it = _conversionReg.find(materialConversion);
    static const ExportConversionInfo _emptyInfo;
    return it != _conversionReg.end() ? it->second : _emptyInfo;
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
