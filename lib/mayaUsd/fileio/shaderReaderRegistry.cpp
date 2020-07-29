//
// Copyright 2020 Autodesk
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
#include "shaderReaderRegistry.h"

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/fileio/functorPrimReader.h>
#include <mayaUsd/fileio/registryHelper.h>

#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stl.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

#include <map>
#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(_tokens, (UsdMaya)(ShaderReader));

typedef TfHashMap<TfToken, UsdMayaShaderReaderRegistry::ReaderFactoryFn, TfToken::HashFunctor>
                 _Registry;
static _Registry _reg;

/* static */
void UsdMayaShaderReaderRegistry::Register(
    TfToken                                      usdInfoId,
    UsdMayaShaderReaderRegistry::ReaderFactoryFn fn)
{
    TF_DEBUG(PXRUSDMAYA_REGISTRY)
        .Msg("Registering UsdMayaShaderReader for info:id %s.\n", usdInfoId.GetText());

    std::pair<_Registry::iterator, bool> insertStatus = _reg.insert(std::make_pair(usdInfoId, fn));
    if (insertStatus.second) {
        UsdMaya_RegistryHelper::AddUnloader([usdInfoId]() { _reg.erase(usdInfoId); });
    } else {
        TF_CODING_ERROR("Multiple readers for id %s", usdInfoId.GetText());
    }
}

/* static */
UsdMayaShaderReaderRegistry::ReaderFactoryFn
UsdMayaShaderReaderRegistry::Find(const TfToken& usdInfoId)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShaderReaderRegistry>();

    ReaderFactoryFn ret = nullptr;
    if (TfMapLookup(_reg, usdInfoId, &ret)) {
        return ret;
    }

    static const TfTokenVector SCOPE = { _tokens->UsdMaya, _tokens->ShaderReader };
    UsdMaya_RegistryHelper::FindAndLoadMayaPlug(SCOPE, usdInfoId);

    // ideally something just registered itself.  if not, we at least put it in
    // the registry in case we encounter it again.
    if (!TfMapLookup(_reg, usdInfoId, &ret)) {
        TF_DEBUG(PXRUSDMAYA_REGISTRY)
            .Msg(
                "No usdMaya reader plugin for info:id %s. No maya plugin found.\n",
                usdInfoId.GetText());
        _reg[usdInfoId] = nullptr;
    }

    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
