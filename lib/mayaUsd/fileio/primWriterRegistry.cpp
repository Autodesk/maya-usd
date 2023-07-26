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

namespace {
struct _RegistryEntry
{
    UsdMayaPrimWriterRegistry::ContextPredicateFn _pred;
    UsdMayaPrimWriterRegistry::WriterFactoryFn    _writer;
    int                                           _index;
};

// typedef std::map<std::string, UsdMayaPrimWriterRegistry::WriterFactoryFn> _Registry;
typedef std::unordered_multimap<std::string, _RegistryEntry> _Registry;
static _Registry                                             _reg;
static std::set<std::string>                                 _mayaTypesThatDoNotCreatePrims;
static int                                                   _indexCounter = 0;

_Registry::const_iterator _Find(
    const std::string&          mayaTypeName,
    const UsdMayaJobExportArgs& exportArgs,
    const MObject&              exportObj)
{
    using ContextSupport = UsdMayaPrimWriter::ContextSupport;

    _Registry::const_iterator ret = _reg.cend();
    _Registry::const_iterator first, last;
    std::tie(first, last) = _reg.equal_range(mayaTypeName);
    while (first != last) {
        ContextSupport support = first->second._pred(exportArgs, exportObj);
        // Look for a "Supported" reader. If no "Supported" reader is found, use a "Fallback" reader
        if (support == ContextSupport::Supported) {
            ret = first;
            break;
        } else if (support == ContextSupport::Fallback && ret == _reg.end()) {
            ret = first;
        }
        ++first;
    }

    return ret;
}

} // namespace

/* static */
void UsdMayaPrimWriterRegistry::Register(
    const std::string&                            mayaTypeName,
    UsdMayaPrimWriterRegistry::ContextPredicateFn pred,
    UsdMayaPrimWriterRegistry::WriterFactoryFn    fn,
    bool                                          fromPython)
{
    TF_DEBUG(PXRUSDMAYA_REGISTRY)
        .Msg("Registering UsdMayaPrimWriter for maya type %s.\n", mayaTypeName.c_str());

    int index = _indexCounter++;
    _reg.insert(std::make_pair(mayaTypeName, _RegistryEntry { pred, fn, index }));

    // The unloader uses the index to know which entry to erase when there are
    // more than one for the same mayaTypeName.
    UsdMaya_RegistryHelper::AddUnloader(
        [mayaTypeName, index]() {
            _Registry::const_iterator it, itEnd;
            std::tie(it, itEnd) = _reg.equal_range(mayaTypeName);
            for (; it != itEnd; ++it) {
                if (it->second._index == index) {
                    _reg.erase(it);
                    break;
                }
            }
        },
        fromPython);
}

/* static */
void UsdMayaPrimWriterRegistry::Register(
    const std::string&                         mayaTypeName,
    UsdMayaPrimWriterRegistry::WriterFactoryFn fn,
    bool                                       fromPython)
{
    TF_DEBUG(PXRUSDMAYA_REGISTRY)
        .Msg("Registering UsdMayaPrimWriter for maya type %s.\n", mayaTypeName.c_str());

    int index = _indexCounter++;

    // Use default ContextSupport if not specified
    _reg.insert(std::make_pair(
        mayaTypeName,
        _RegistryEntry { [](const UsdMayaJobExportArgs&, const MObject&) {
                            return UsdMayaPrimWriter::ContextSupport::Fallback;
                        },
                         fn,
                         index }));

    // The unloader uses the index to know which entry to erase when there are
    // more than one for the same mayaTypeName.
    UsdMaya_RegistryHelper::AddUnloader(
        [mayaTypeName, index]() {
            _Registry::const_iterator it, itEnd;
            std::tie(it, itEnd) = _reg.equal_range(mayaTypeName);
            for (; it != itEnd; ++it) {
                if (it->second._index == index) {
                    _reg.erase(it);
                    break;
                }
            }
        },
        fromPython);
}

/* static */
void UsdMayaPrimWriterRegistry::RegisterRaw(
    const std::string&                  mayaTypeName,
    UsdMayaPrimWriterRegistry::WriterFn fn)
{
    Register(mayaTypeName, UsdMaya_FunctorPrimWriter::CreateFactory(fn));
}

/* static */
UsdMayaPrimWriterRegistry::WriterFactoryFn UsdMayaPrimWriterRegistry::Find(
    const std::string&          mayaTypeName,
    const UsdMayaJobExportArgs& exportArgs,
    const MObject&              exportObj)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaPrimWriterRegistry>();

    _Registry::const_iterator it = _Find(mayaTypeName, exportArgs, exportObj);

    if (it != _reg.end()) {
        return it->second._writer;
    }

    static const TfTokenVector SCOPE = { _tokens->UsdMaya, _tokens->PrimWriter };
    UsdMaya_RegistryHelper::FindAndLoadMayaPlug(SCOPE, mayaTypeName);

    it = _Find(mayaTypeName, exportArgs, exportObj);

    if (it != _reg.end()) {
        return it->second._writer;
    }

    // if nothing was found and nothing was registered, we at least put it in
    // the registry in case we encounter it again.
    TF_DEBUG(PXRUSDMAYA_REGISTRY)
        .Msg("No usdMaya writer plugin for TfType %s. No maya plugin.\n", mayaTypeName.c_str());
    if (_reg.count(mayaTypeName) == 0) {
        // Nothing registered at all, remember that:
        _reg.insert(std::make_pair(
            mayaTypeName,
            _RegistryEntry { [](const UsdMayaJobExportArgs&, const MObject&) {
                                return UsdMayaPrimWriter::ContextSupport::Fallback;
                            },
                             nullptr,
                             -1 }));
    }

    return nullptr;
}

/* static */
void UsdMayaPrimWriterRegistry::CheckForWriterPlugin(const std::string& mayaTypeName)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaPrimWriterRegistry>();

    _Registry::const_iterator first, last;
    std::tie(first, last) = _reg.equal_range(mayaTypeName);

    if (first == last) {
        // If the type name is not currently in our registry, check for plugin registry
        static const TfTokenVector SCOPE = { _tokens->UsdMaya, _tokens->PrimWriter };
        UsdMaya_RegistryHelper::FindAndLoadMayaPlug(SCOPE, mayaTypeName);
    }
}

/* static */
void UsdMayaPrimWriterRegistry::RegisterPrimless(const std::string& mayaTypeName)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaPrimWriterRegistry>();
    _mayaTypesThatDoNotCreatePrims.insert(mayaTypeName);
}

/* static */
bool UsdMayaPrimWriterRegistry::IsPrimless(const std::string& mayaTypeName)
{
    return _mayaTypesThatDoNotCreatePrims.count(mayaTypeName) == 0;
}

PXR_NAMESPACE_CLOSE_SCOPE
