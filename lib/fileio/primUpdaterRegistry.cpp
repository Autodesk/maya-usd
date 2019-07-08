//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/pxr.h"
#include "primUpdaterRegistry.h"

#include "../base/debugCodes.h"
#include "registryHelper.h"

#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/token.h"

#include <map>
#include <string>
#include <utility>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(_tokens,
    (UsdMaya)
        (PrimUpdater)
);

typedef std::map<TfToken, UsdMayaPrimUpdaterRegistry::RegisterItem> _Registry;
static _Registry _reg;


/* static */
void
UsdMayaPrimUpdaterRegistry::Register(
        const TfType& t, 
        UsdMayaPrimUpdater::Supports sup,
        UsdMayaPrimUpdaterRegistry::UpdaterFactoryFn fn)
{
    TfToken tfTypeName(t.GetTypeName());

    TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg(
        "Registering UsdMayaPrimWriter for TfType type %s.\n",
        tfTypeName.GetText());

    std::pair< _Registry::iterator, bool> insertStatus =
        _reg.insert(std::make_pair(tfTypeName, std::make_tuple(sup,fn)));
    if (insertStatus.second) {
        UsdMaya_RegistryHelper::AddUnloader([tfTypeName]() {
            _reg.erase(tfTypeName);
        });
    }
    else {
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
    TfType tfType = PlugRegistry::FindDerivedTypeByName<UsdSchemaBase>(usdTypeName);
    std::string typeNameStr = tfType.GetTypeName();
    TfToken typeName(typeNameStr);

    RegisterItem ret;
    if (TfMapLookup(_reg, typeName, &ret)) {
        return ret;
    }

    static const TfTokenVector SCOPE = {
        _tokens->UsdMaya,
        _tokens->PrimUpdater
    };
    UsdMaya_RegistryHelper::FindAndLoadMayaPlug(SCOPE, typeNameStr);

    // ideally something just registered itself.  if not, we at least put it in
    // the registry in case we encounter it again.
    if (!TfMapLookup(_reg, typeName, &ret)) {
        TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg(
            "No usdMaya updater plugin for TfType %s. No maya plugin found.\n",
            typeName.GetText());
        _reg[typeName] = {};
    }

    return ret;
}


PXR_NAMESPACE_CLOSE_SCOPE
