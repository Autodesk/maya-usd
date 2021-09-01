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
#include "schemaApiWriterRegistry.h"

#include <mayaUsd/base/debugCodes.h>
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
        (SchemaApiWriter)
);
// clang-format on

typedef std::map<std::string, UsdMayaSchemaApiWriterRegistry::WriterFactoryFnMap> _Registry;
static _Registry                                                                  _reg;

/* static */
void UsdMayaSchemaApiWriterRegistry::Register(
    const std::string&                              mayaTypeName,
    const std::string&                              schemaApiName,
    UsdMayaSchemaApiWriterRegistry::WriterFactoryFn fn)
{
    TF_DEBUG(PXRUSDMAYA_REGISTRY)
        .Msg(
            "Registering UsdMayaSchemaApiWriter for maya type %s and api %s.\n",
            mayaTypeName.c_str(),
            schemaApiName.c_str());

    _Registry::iterator mayaTypeIt = _reg.find(mayaTypeName);

    if (mayaTypeIt == _reg.cend()) {
        mayaTypeIt = _reg.insert(std::make_pair(mayaTypeName, WriterFactoryFnMap())).first;
    }

    std::pair<WriterFactoryFnMap::iterator, bool> insertStatus
        = mayaTypeIt->second.insert(std::make_pair(schemaApiName, fn));
    if (insertStatus.second) {
        UsdMaya_RegistryHelper::AddUnloader([mayaTypeName, schemaApiName]() {
            auto it = _reg.find(mayaTypeName);
            if (it != _reg.end()) {
                it->second.erase(schemaApiName);
                if (it->second.empty()) {
                    _reg.erase(mayaTypeName);
                }
            }
        });
    } else {
        TF_CODING_ERROR(
            "Multiple writers for type %s and api %s", mayaTypeName.c_str(), schemaApiName.c_str());
    }
}

/* static */
UsdMayaSchemaApiWriterRegistry::WriterFactoryFnMap
UsdMayaSchemaApiWriterRegistry::Find(const std::string& mayaTypeName)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaSchemaApiWriterRegistry>();

    // unfortunately, usdTypeName is diff from the tfTypeName which we use to
    // register.  do the conversion here.
    WriterFactoryFnMap ret;
    if (TfMapLookup(_reg, mayaTypeName, &ret)) {
        return ret;
    }

    static const TfTokenVector SCOPE = { _tokens->UsdMaya, _tokens->SchemaApiWriter };
    UsdMaya_RegistryHelper::FindAndLoadMayaPlug(SCOPE, mayaTypeName);

    // ideally something just registered itself.  if not, we at least put it in
    // the registry in case we encounter it again.
    if (!TfMapLookup(_reg, mayaTypeName, &ret)) {
        TF_DEBUG(PXRUSDMAYA_REGISTRY)
            .Msg(
                "No usdMaya writer plugin for maya type %s. No maya plugin found.\n",
                mayaTypeName.c_str());
        _reg[mayaTypeName] = WriterFactoryFnMap();
    }

    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
