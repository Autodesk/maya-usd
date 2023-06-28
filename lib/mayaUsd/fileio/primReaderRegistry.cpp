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
#include "primReaderRegistry.h"

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/fileio/fallbackPrimReader.h>
#include <mayaUsd/fileio/functorPrimReader.h>
#include <mayaUsd/fileio/registryHelper.h>

#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stl.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/type.h>
#include <pxr/usd/usd/schemaBase.h>

#include <map>
#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (UsdMaya)
    (PrimReader)
);
// clang-format on

namespace {
struct _RegistryEntry
{
    UsdMayaPrimReaderRegistry::ContextPredicateFn _pred;
    UsdMayaPrimReaderRegistry::ReaderFactoryFn    _fn;
    int                                           _index;
};

typedef std::unordered_multimap<TfToken, _RegistryEntry, TfToken::HashFunctor> _Registry;
static _Registry                                                               _reg;
static int                                                                     _indexCounter = 0;

_Registry::const_iterator
_Find(const TfToken& typeName, const UsdMayaJobImportArgs& importArgs, const UsdPrim& importPrim)
{
    using ContextSupport = UsdMayaPrimReader::ContextSupport;

    _Registry::const_iterator ret = _reg.cend();
    _Registry::const_iterator first, last;

    std::tie(first, last) = _reg.equal_range(typeName);
    while (first != last) {
        ContextSupport support = first->second._pred(importArgs, importPrim);
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
void UsdMayaPrimReaderRegistry::Register(
    const TfType&                              t,
    UsdMayaPrimReaderRegistry::ReaderFactoryFn fn,
    bool                                       fromPython)
{
    TfToken tfTypeName(t.GetTypeName());
    int     index = _indexCounter++;
    TF_DEBUG(PXRUSDMAYA_REGISTRY)
        .Msg("Registering UsdMayaPrimReader for TfType %s.\n", tfTypeName.GetText());

    // Use default ContextSupport if not specified
    _reg.insert(std::make_pair(
        tfTypeName,
        _RegistryEntry { [](const UsdMayaJobImportArgs&, const UsdPrim) {
                            return UsdMayaPrimReader::ContextSupport::Fallback;
                        },
            fn,
            index }));

    if (fn) {
        UsdMaya_RegistryHelper::AddUnloader(
            [tfTypeName, index]() {
                _Registry::const_iterator it, itEnd;
                std::tie(it, itEnd) = _reg.equal_range(tfTypeName);
                for (; it != itEnd; ++it) {
                    if (it->second._index == index) {
                        _reg.erase(it);
                        break;
                    }
                }
            },
            fromPython);
    }
}

/* static */
void UsdMayaPrimReaderRegistry::Register(
    const TfType&                                 t,
    UsdMayaPrimReaderRegistry::ContextPredicateFn pred,
    UsdMayaPrimReaderRegistry::ReaderFactoryFn    fn,
    bool                                          fromPython)
{
    TfToken tfTypeName(t.GetTypeName());
    int     index = _indexCounter++;
    TF_DEBUG(PXRUSDMAYA_REGISTRY)
        .Msg("Registering UsdMayaPrimReader for TfType %s.\n", tfTypeName.GetText());
    _reg.insert(std::make_pair(tfTypeName, _RegistryEntry { pred, fn, index }));

    // The unloader uses the index to know which entry to erase when there are
    // more than one for the same usdInfoId.
    if (fn) {
        UsdMaya_RegistryHelper::AddUnloader(
            [tfTypeName, index]() {
                _Registry::const_iterator it, itEnd;
                std::tie(it, itEnd) = _reg.equal_range(tfTypeName);
                for (; it != itEnd; ++it) {
                    if (it->second._index == index) {
                        _reg.erase(it);
                        break;
                    }
                }
            },
            fromPython);
    }
}

/* static */
void UsdMayaPrimReaderRegistry::RegisterRaw(const TfType& t, UsdMayaPrimReaderRegistry::ReaderFn fn)
{
    Register(t, UsdMaya_FunctorPrimReader::CreateFactory(fn));
}

/* static */
UsdMayaPrimReaderRegistry::ReaderFactoryFn UsdMayaPrimReaderRegistry::Find(
    const TfToken&              usdTypeName,
    const UsdMayaJobImportArgs& importArgs,
    const UsdPrim&              importPrim)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaPrimReaderRegistry>();

    // unfortunately, usdTypeName is diff from the tfTypeName which we use to
    // register.  do the conversion here.
    TfType          tfType = PlugRegistry::FindDerivedTypeByName<UsdSchemaBase>(usdTypeName);
    std::string     typeNameStr = tfType.GetTypeName();
    TfToken         typeName(typeNameStr);
    ReaderFactoryFn ret = nullptr;

    // For that we are using unordered_multimap now, we can't use TfMapLookup anymore
    _Registry::const_iterator it = _Find(typeName, importArgs, importPrim);

    if (it != _reg.end()) {
        return it->second._fn;
    }

    static const TfTokenVector SCOPE = { _tokens->UsdMaya, _tokens->PrimReader };
    UsdMaya_RegistryHelper::FindAndLoadMayaPlug(SCOPE, typeNameStr);

    // ideally something just registered itself.  if not, we at least put it in
    // the registry in case we encounter it again.
    if (_reg.count(typeName) == 0) {
        TF_DEBUG(PXRUSDMAYA_REGISTRY)
            .Msg("No usdMaya reader plugin for TfType %s. No maya plugin.\n", typeName.GetText());
        //_reg[typeName] = nullptr;
        Register(
            tfType,
            [](const UsdMayaJobImportArgs&, const UsdPrim&) { return UsdMayaPrimReader::ContextSupport::Fallback; },
            nullptr);
    }

    return nullptr;
}

/* static */
UsdMayaPrimReaderRegistry::ReaderFactoryFn UsdMayaPrimReaderRegistry::FindOrFallback(
    const TfToken&              usdTypeName,
    const UsdMayaJobImportArgs& importArgs,
    const UsdPrim&              importPrim)
{
    if (ReaderFactoryFn fn = Find(usdTypeName, importArgs, importPrim)) {
        return fn;
    }

    return UsdMaya_FallbackPrimReader::CreateFactory();
}

PXR_NAMESPACE_CLOSE_SCOPE
