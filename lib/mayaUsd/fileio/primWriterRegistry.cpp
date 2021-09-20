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
#include "primWriterRegistry.h"

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/fileio/functorPrimWriter.h>
#include <mayaUsd/fileio/registryHelper.h>

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

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (UsdMaya)
        (PrimWriter)
);
// clang-format on

typedef std::map<std::string, UsdMayaPrimWriterRegistry::WriterFactoryFn> _Registry;
static _Registry                                                          _reg;

/* static */
void UsdMayaPrimWriterRegistry::Register(
    const std::string&                         mayaTypeName,
    UsdMayaPrimWriterRegistry::WriterFactoryFn fn,
    bool                                       fromPython)
{
    TF_DEBUG(PXRUSDMAYA_REGISTRY)
        .Msg("Registering UsdMayaPrimWriter for maya type %s.\n", mayaTypeName.c_str());

    std::pair<_Registry::iterator, bool> insertStatus
        = _reg.insert(std::make_pair(mayaTypeName, fn));
    if (insertStatus.second) {
        UsdMaya_RegistryHelper::AddUnloader(
            [mayaTypeName]() { _reg.erase(mayaTypeName); }, fromPython);
    } else {
        TF_CODING_ERROR("Multiple writers for type %s", mayaTypeName.c_str());
    }
}

/* static */
void UsdMayaPrimWriterRegistry::RegisterRaw(
    const std::string&                  mayaTypeName,
    UsdMayaPrimWriterRegistry::WriterFn fn)
{
    Register(mayaTypeName, UsdMaya_FunctorPrimWriter::CreateFactory(fn));
}

/* static */
UsdMayaPrimWriterRegistry::WriterFactoryFn
UsdMayaPrimWriterRegistry::Find(const std::string& mayaTypeName)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaPrimWriterRegistry>();

    // unfortunately, usdTypeName is diff from the tfTypeName which we use to
    // register.  do the conversion here.
    WriterFactoryFn ret = nullptr;
    if (TfMapLookup(_reg, mayaTypeName, &ret)) {
        return ret;
    }

    static const TfTokenVector SCOPE = { _tokens->UsdMaya, _tokens->PrimWriter };
    UsdMaya_RegistryHelper::FindAndLoadMayaPlug(SCOPE, mayaTypeName);

    // ideally something just registered itself.  if not, we at least put it in
    // the registry in case we encounter it again.
    if (!TfMapLookup(_reg, mayaTypeName, &ret)) {
        TF_DEBUG(PXRUSDMAYA_REGISTRY)
            .Msg(
                "No usdMaya writer plugin for maya type %s. No maya plugin found.\n",
                mayaTypeName.c_str());
        _reg[mayaTypeName] = nullptr;
    }

    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
