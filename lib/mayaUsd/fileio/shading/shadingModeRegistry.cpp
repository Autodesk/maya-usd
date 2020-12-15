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

#include <mayaUsd/fileio/registryHelper.h>
#include <mayaUsd/fileio/shading/shadingModeExporter.h>
#include <mayaUsd/fileio/shading/shadingModeExporterContext.h>
#include <mayaUsd/fileio/shading/shadingModeImporter.h>

#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>

#include <map>
#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdMayaShadingModeTokens, PXRUSDMAYA_SHADINGMODE_TOKENS);

namespace {
static const std::string _kEmptyString;
}

struct _ExportShadingMode
{
    std::string                       _niceName;
    std::string                       _description;
    UsdMayaShadingModeExporterCreator _fn;
};
using _ExportRegistry = std::map<TfToken, _ExportShadingMode>;
static _ExportRegistry _exportReg;

TF_DEFINE_PUBLIC_TOKENS(UsdMayaPreferredMaterialTokens, PXRUSDMAYA_SHADINGCONVERSION_TOKENS);

using _MaterialConversionRegistry
    = std::unordered_map<TfToken, UsdMayaShadingModeRegistry::ConversionInfo, TfToken::HashFunctor>;
static _MaterialConversionRegistry _conversionReg;

bool UsdMayaShadingModeRegistry::RegisterExporter(
    const std::string&                name,
    std::string                       niceName,
    std::string                       description,
    UsdMayaShadingModeExporterCreator fn)
{
    const TfToken                                    nameToken(name);
    std::pair<_ExportRegistry::const_iterator, bool> insertStatus
        = _exportReg.insert(_ExportRegistry::value_type(
            nameToken, _ExportShadingMode { std::move(niceName), std::move(description), fn }));
    if (insertStatus.second) {
        UsdMaya_RegistryHelper::AddUnloader([nameToken]() { _exportReg.erase(nameToken); });
    } else {
        TF_CODING_ERROR("Multiple shading exporters named '%s'", name.c_str());
    }
    return insertStatus.second;
}

UsdMayaShadingModeExporterCreator UsdMayaShadingModeRegistry::_GetExporter(const TfToken& name)
{
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeExportContext>();
    const auto it = _exportReg.find(name);
    return it == _exportReg.end() ? nullptr : it->second._fn;
}

const std::string& UsdMayaShadingModeRegistry::_GetExporterNiceName(const TfToken& name)
{
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeExportContext>();
    const auto it = _exportReg.find(name);
    return it == _exportReg.end() ? _kEmptyString : it->second._niceName;
}

const std::string& UsdMayaShadingModeRegistry::_GetExporterDescription(const TfToken& name)
{
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeExportContext>();
    const auto it = _exportReg.find(name);
    return it == _exportReg.end() ? _kEmptyString : it->second._description;
}

struct _ImportShadingMode
{
    std::string                _niceName;
    std::string                _description;
    UsdMayaShadingModeImporter _fn;
};
using _ImportRegistry = std::map<TfToken, _ImportShadingMode>;
static _ImportRegistry _importReg;

bool UsdMayaShadingModeRegistry::RegisterImporter(
    const std::string&         name,
    std::string                niceName,
    std::string                description,
    UsdMayaShadingModeImporter fn)
{
    const TfToken                                    nameToken(name);
    std::pair<_ImportRegistry::const_iterator, bool> insertStatus
        = _importReg.insert(_ImportRegistry::value_type(
            nameToken, _ImportShadingMode { std::move(niceName), std::move(description), fn }));
    if (insertStatus.second) {
        UsdMaya_RegistryHelper::AddUnloader([nameToken]() { _importReg.erase(nameToken); });
    } else {
        TF_CODING_ERROR("Multiple shading importers named '%s'", name.c_str());
    }
    return insertStatus.second;
}

UsdMayaShadingModeImporter UsdMayaShadingModeRegistry::_GetImporter(const TfToken& name)
{
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeImportContext>();
    const auto it = _importReg.find(name);
    return it == _importReg.end() ? nullptr : it->second._fn;
}

const std::string& UsdMayaShadingModeRegistry::_GetImporterNiceName(const TfToken& name)
{
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeImportContext>();
    const auto it = _importReg.find(name);
    return it == _importReg.end() ? _kEmptyString : it->second._niceName;
}

const std::string& UsdMayaShadingModeRegistry::_GetImporterDescription(const TfToken& name)
{
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeImportContext>();
    const auto it = _importReg.find(name);
    return it == _importReg.end() ? _kEmptyString : it->second._description;
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

TfTokenVector UsdMayaShadingModeRegistry::_ListImporters()
{
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
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
    _MaterialConversionRegistry::iterator insertionPoint;
    bool                                  hasInserted;
    std::tie(insertionPoint, hasInserted)
        = _conversionReg.insert(_MaterialConversionRegistry::value_type(
            materialConversion,
            ConversionInfo { renderContext, niceName, description, TfToken(), true, false }));
    if (!hasInserted) {
        // Update the info:
        if (insertionPoint->second.exportDescription.IsEmpty()) {
            insertionPoint->second.exportDescription = description;
        }
        insertionPoint->second.hasExporter = true;
    }
}

void UsdMayaShadingModeRegistry::RegisterImportConversion(
    const TfToken& materialConversion,
    const TfToken& renderContext,
    const TfToken& niceName,
    const TfToken& description)
{
    // It is perfectly valid to register the same material conversion more than once,
    // especially if importers for a conversion are split across multiple libraries.
    // We will keep the first niceName registered.
    _MaterialConversionRegistry::iterator insertionPoint;
    bool                                  hasInserted;
    std::tie(insertionPoint, hasInserted)
        = _conversionReg.insert(_MaterialConversionRegistry::value_type(
            materialConversion,
            ConversionInfo { renderContext, niceName, TfToken(), description, false, true }));
    if (!hasInserted) {
        // Update the info:
        if (insertionPoint->second.importDescription.IsEmpty()) {
            insertionPoint->second.importDescription = description;
        }
        insertionPoint->second.hasImporter = true;
    }
}

TfTokenVector UsdMayaShadingModeRegistry::_ListMaterialConversions()
{
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeExportContext>();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeImportContext>();
    TfTokenVector ret;
    ret.reserve(_conversionReg.size());
    for (const auto& e : _conversionReg) {
        ret.push_back(e.first);
    }
    return ret;
}

const UsdMayaShadingModeRegistry::ConversionInfo&
UsdMayaShadingModeRegistry::_GetMaterialConversionInfo(const TfToken& materialConversion)
{
    UsdMaya_RegistryHelper::LoadShadingModePlugins();
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShadingModeExportContext>();
    auto                        it = _conversionReg.find(materialConversion);
    static const ConversionInfo _emptyInfo;
    return it != _conversionReg.end() ? it->second : _emptyInfo;
}

TF_INSTANTIATE_SINGLETON(UsdMayaShadingModeRegistry);

UsdMayaShadingModeRegistry& UsdMayaShadingModeRegistry::GetInstance()
{
    return TfSingleton<UsdMayaShadingModeRegistry>::GetInstance();
}

UsdMayaShadingModeRegistry::UsdMayaShadingModeRegistry() { }

UsdMayaShadingModeRegistry::~UsdMayaShadingModeRegistry() { }

PXR_NAMESPACE_CLOSE_SCOPE
