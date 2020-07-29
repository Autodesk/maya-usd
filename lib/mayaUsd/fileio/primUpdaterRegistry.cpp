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

TF_DEFINE_PRIVATE_TOKENS(_tokens, (UsdMaya)(PrimUpdater));

typedef std::map<TfToken, UsdMayaPrimUpdaterRegistry::RegisterItem> _Registry;
static _Registry                                                    _reg;

/* static */
void UsdMayaPrimUpdaterRegistry::Register(
    const TfType&                                t,
    UsdMayaPrimUpdater::Supports                 sup,
    UsdMayaPrimUpdaterRegistry::UpdaterFactoryFn fn)
{
    TfToken tfTypeName(t.GetTypeName());

    TF_DEBUG(PXRUSDMAYA_REGISTRY)
        .Msg("Registering UsdMayaPrimWriter for TfType type %s.\n", tfTypeName.GetText());

    std::pair<_Registry::iterator, bool> insertStatus
        = _reg.insert(std::make_pair(tfTypeName, std::make_tuple(sup, fn)));
    if (insertStatus.second) {
        UsdMaya_RegistryHelper::AddUnloader([tfTypeName]() { _reg.erase(tfTypeName); });
    } else {
        TF_CODING_ERROR("Multiple updaters for TfType %s", tfTypeName.GetText());
    }
}

/* static */
UsdMayaPrimUpdaterRegistry::RegisterItem
UsdMayaPrimUpdaterRegistry::Find(const TfToken& usdTypeName)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaPrimUpdaterRegistry>();

    // unfortunately, usdTypeName is diff from the tfTypeName which we use to
    // register.  do the conversion here.
    TfType      tfType = PlugRegistry::FindDerivedTypeByName<UsdSchemaBase>(usdTypeName);
    std::string typeNameStr = tfType.GetTypeName();
    TfToken     typeName(typeNameStr);

    RegisterItem ret;
    if (TfMapLookup(_reg, typeName, &ret)) {
        return ret;
    }

    static const TfTokenVector SCOPE = { _tokens->UsdMaya, _tokens->PrimUpdater };
    UsdMaya_RegistryHelper::FindAndLoadMayaPlug(SCOPE, typeNameStr);

    // ideally something just registered itself.  if not, we at least put it in
    // the registry in case we encounter it again.
    if (!TfMapLookup(_reg, typeName, &ret)) {
        TF_DEBUG(PXRUSDMAYA_REGISTRY)
            .Msg(
                "No usdMaya updater plugin for TfType %s. No maya plugin found.\n",
                typeName.GetText());
        _reg[typeName] = {};
    }

    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
