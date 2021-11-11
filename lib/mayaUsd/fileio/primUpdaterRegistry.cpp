//
// Copyright 2016 Pixar
// Copyright 2019 Autodesk
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
#include "primUpdaterRegistry.h"

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/fileio/fallbackPrimUpdater.h>
#include <mayaUsd/fileio/registryHelper.h>

#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stl.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/schemaBase.h>

#include <map>
#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (UsdMaya)
        (PrimUpdater)
);
// clang-format on

namespace {
typedef std::map<TfToken, UsdMayaPrimUpdaterRegistry::RegisterItem> _RegistryWithTfType;
static _RegistryWithTfType                                          _regTfType;

typedef std::map<std::string, UsdMayaPrimUpdaterRegistry::RegisterItem> _RegistryWithMayaType;
static _RegistryWithMayaType                                            _regMayaType;
} // namespace

/* static */
void UsdMayaPrimUpdaterRegistry::Register(
    const TfType&                                tfType,
    const std::string&                           mayaType,
    UsdMayaPrimUpdater::Supports                 sup,
    UsdMayaPrimUpdaterRegistry::UpdaterFactoryFn fn)
{
    TfToken tfTypeName(tfType.GetTypeName());

    TF_DEBUG(PXRUSDMAYA_REGISTRY)
        .Msg("Registering UsdMayaPrimWriter for TfType type %s.\n", tfTypeName.GetText());

    auto insertStatus = _regTfType.insert(std::make_pair(tfTypeName, std::make_tuple(sup, fn)));
    if (insertStatus.second) {
        // register lookup by maya type
        _regMayaType.insert(std::make_pair(mayaType, std::make_tuple(sup, fn)));

        // cleanup both registries when tftype gets unloaded
        UsdMaya_RegistryHelper::AddUnloader([tfTypeName, mayaType]() {
            _regTfType.erase(tfTypeName);
            _regMayaType.erase(mayaType);
        });
    } else {
        TF_CODING_ERROR("Multiple updaters for TfType %s", tfTypeName.GetText());
    }
}

/* static */
UsdMayaPrimUpdaterRegistry::RegisterItem
UsdMayaPrimUpdaterRegistry::FindOrFallback(const TfToken& usdTypeName)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaPrimUpdaterRegistry>();

    // unfortunately, usdTypeName is diff from the tfTypeName which we use to
    // register.  do the conversion here.
    TfType      tfType = PlugRegistry::FindDerivedTypeByName<UsdSchemaBase>(usdTypeName);
    std::string typeNameStr = tfType.GetTypeName();
    TfToken     typeName(typeNameStr);

    RegisterItem ret;
    if (TfMapLookup(_regTfType, typeName, &ret)) {
        return ret;
    }

    static const TfTokenVector SCOPE = { _tokens->UsdMaya, _tokens->PrimUpdater };
    UsdMaya_RegistryHelper::FindAndLoadMayaPlug(SCOPE, typeNameStr);

    // ideally something just registered itself.  if not, we at least put it in
    // the registry in case we encounter it again.
    if (!TfMapLookup(_regTfType, typeName, &ret)) {
        TF_DEBUG(PXRUSDMAYA_REGISTRY)
            .Msg(
                "No usdMaya updater plugin for TfType %s. No maya plugin found.\n",
                typeName.GetText());

        // use fallback
        ret = std::make_tuple(
            UsdMayaPrimUpdater::Supports::All,
            [](const MFnDependencyNode& depNodeFn, const Ufe::Path& path) {
                return std::make_shared<FallbackPrimUpdater>(depNodeFn, path);
            });

        _regTfType[typeName] = ret;
    }

    return ret;
}

/* static */
UsdMayaPrimUpdaterRegistry::RegisterItem
UsdMayaPrimUpdaterRegistry::FindOrFallback(const std::string& mayaTypeName)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaPrimUpdaterRegistry>();

    RegisterItem ret;
    if (TfMapLookup(_regMayaType, mayaTypeName, &ret)) {
        return ret;
    } else {
        // use fallback
        ret = std::make_tuple(
            UsdMayaPrimUpdater::Supports::All,
            [](const MFnDependencyNode& depNodeFn, const Ufe::Path& path) {
                return std::make_shared<FallbackPrimUpdater>(depNodeFn, path);
            });

        _regMayaType[mayaTypeName] = ret;
    }

    /*
    static const TfTokenVector SCOPE = { _tokens->UsdMaya, _tokens->PrimUpdater };
    UsdMaya_RegistryHelper::FindAndLoadMayaPlug(SCOPE, mayaTypeName);

    // ideally something just registered itself.  if not, we at least put it in
    // the registry in case we encounter it again.
    if (!TfMapLookup(_regTfType, typeName, &ret)) {
        TF_DEBUG(PXRUSDMAYA_REGISTRY)
            .Msg(
                "No usdMaya updater plugin for TfType %s. No maya plugin found.\n",
                typeName.GetText());
        _regTfType[typeName] = {};
    }
    */

    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
