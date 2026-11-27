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
// Modifications copyright (C) 2026 Sony Interactive Entertainment
//
#include "schemaApiAdaptorRegistry.h"

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/fileio/registryHelper.h>

#include <pxr/base/plug/notice.h>
#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stl.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/pxr.h>

#include <map>
#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (UsdMaya)
        (SchemaApiAdaptor)
);
// clang-format on

using _AdaptorFactoryFnMap = UsdMayaSchemaApiAdaptorRegistry::AdaptorFactoryFnMap;
using _Registry = std::map<std::string, _AdaptorFactoryFnMap>;

// Adaptors registered per Maya type.
static _Registry _reg;
static _Registry _findCache;

// Clears _findCache when USD registers new plugins.
class _PluginRegistrationListener : public TfWeakBase
{
public:
    _PluginRegistrationListener()
    {
        TfNotice::Register(
            TfCreateWeakPtr(this), &_PluginRegistrationListener::_OnDidRegisterPlugins);
    }

private:
    void _OnDidRegisterPlugins(const PlugNotice::DidRegisterPlugins&) { _findCache.clear(); }
};

/* static */
void UsdMayaSchemaApiAdaptorRegistry::Register(
    const std::string&                                mayaTypeName,
    const std::string&                                schemaApiName,
    UsdMayaSchemaApiAdaptorRegistry::AdaptorFactoryFn fn,
    bool                                              fromPython)
{
    TF_DEBUG(PXRUSDMAYA_REGISTRY)
        .Msg(
            "Registering UsdMayaSchemaApiAdaptor for maya type %s and api %s.\n",
            mayaTypeName.c_str(),
            schemaApiName.c_str());

    _Registry::iterator mayaTypeIt = _reg.find(mayaTypeName);
    if (mayaTypeIt == _reg.cend()) {
        mayaTypeIt = _reg.insert(std::make_pair(mayaTypeName, AdaptorFactoryFnMap())).first;
    }

    AdaptorFactoryFnMap::iterator schemaNameIt = mayaTypeIt->second.find(schemaApiName);
    if (schemaNameIt == mayaTypeIt->second.end()) {
        schemaNameIt
            = mayaTypeIt->second.insert(std::make_pair(schemaApiName, AdaptorFactoryFnList()))
                  .first;
    }

    size_t fnIndex = schemaNameIt->second.size();
    schemaNameIt->second.push_back(fn);
    _findCache.clear();

    UsdMaya_RegistryHelper::AddUnloader(
        [mayaTypeName, schemaApiName, fnIndex]() {
            _findCache.clear();
            auto itMaya = _reg.find(mayaTypeName);
            if (itMaya != _reg.end()) {
                auto itSchema = itMaya->second.find(schemaApiName);
                if (itSchema != itMaya->second.end()) {
                    if (fnIndex < itSchema->second.size()) {
                        itSchema->second[fnIndex] = nullptr;
                    }
                }
            }
        },
        fromPython);
}

static _AdaptorFactoryFnMap _FindUncached(const std::string& mayaTypeName)
{
    // unfortunately, usdTypeName is diff from the tfTypeName which we use to
    // register.  do the conversion here.
    _AdaptorFactoryFnMap ret;
    if (TfMapLookup(_reg, mayaTypeName, &ret)) {
        return ret;
    }

    static const TfTokenVector SCOPE = { _tokens->UsdMaya, _tokens->SchemaApiAdaptor };
    UsdMaya_RegistryHelper::FindAndLoadMayaPlug(SCOPE, mayaTypeName);

    // Did we register an exact type match?
    if (TfMapLookup(_reg, mayaTypeName, &ret)) {
        return ret;
    }

    // Check the type registry. We might have a more generic adaptor:
    std::vector<std::string> ancestors(UsdMayaUtil::GetAllAncestorMayaNodeTypes(mayaTypeName));
    // Need to iterate in reverse order:
    auto&& it = ancestors.rbegin();
    // and skip the mayaTypeName found at the end (if any; the list is empty for unknown types).
    if (it != ancestors.rend()) {
        ++it;
    }
    for (; it != ancestors.rend(); ++it) {
        if (TfMapLookup(_reg, *it, &ret)) {
            return ret;
        }
    }

    // ideally something just registered itself.  if not, we at least put it in
    // the registry in case we encounter it again.
    if (!TfMapLookup(_reg, mayaTypeName, &ret)) {
        TF_DEBUG(PXRUSDMAYA_REGISTRY)
            .Msg(
                "No usdMaya adaptor plugin for maya type %s. No maya plugin found.\n",
                mayaTypeName.c_str());
        _reg[mayaTypeName] = _AdaptorFactoryFnMap();
    }

    return ret;
}

UsdMayaSchemaApiAdaptorRegistry::AdaptorFactoryFnMap
UsdMayaSchemaApiAdaptorRegistry::Find(const std::string& mayaTypeName)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaSchemaApiAdaptorRegistry>();
    static _PluginRegistrationListener pluginListener;

    const auto cached = _findCache.find(mayaTypeName);
    if (cached != _findCache.end()) {
        return cached->second;
    }
    // _FindUncached may load a plugin whose registration clears the cache, so insert afterwards.
    const _AdaptorFactoryFnMap found = _FindUncached(mayaTypeName);
    _findCache[mayaTypeName] = found;
    return found;
}

UsdMayaSchemaApiAdaptorRegistry::AdaptorFactoryFnList UsdMayaSchemaApiAdaptorRegistry::Find(
    const std::string& mayaTypeName,
    const std::string& schemaName)
{
    auto factoryMap = Find(mayaTypeName);

    AdaptorFactoryFnList ret;
    TfMapLookup(factoryMap, schemaName, &ret);
    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
