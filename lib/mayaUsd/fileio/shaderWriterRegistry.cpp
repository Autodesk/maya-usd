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
#include "shaderWriterRegistry.h"

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

#include <string>
#include <unordered_map>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (UsdMaya)(ShaderWriter));

namespace {
struct _RegistryEntry
{
    UsdMayaShaderWriterRegistry::ContextPredicateFn _pred;
    UsdMayaShaderWriterRegistry::WriterFactoryFn    _writer;
    int                                             _index;
};

using _Registry = std::unordered_multimap<TfToken, _RegistryEntry, TfToken::HashFunctor>;
static _Registry _reg;
static int       _indexCounter = 0;

_Registry::const_iterator _Find(const TfToken& usdInfoId, const UsdMayaJobExportArgs& exportArgs)
{
    using ContextSupport = UsdMayaShaderWriter::ContextSupport;

    _Registry::const_iterator ret = _reg.cend();
    _Registry::const_iterator first, last;
    std::tie(first, last) = _reg.equal_range(usdInfoId);
    while (first != last) {
        ContextSupport support = first->second._pred(exportArgs);
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
void UsdMayaShaderWriterRegistry::Register(
    const TfToken&                                  mayaTypeName,
    UsdMayaShaderWriterRegistry::ContextPredicateFn pred,
    UsdMayaShaderWriterRegistry::WriterFactoryFn    fn)
{
    int index = _indexCounter++;
    TF_DEBUG(PXRUSDMAYA_REGISTRY)
        .Msg(
            "Registering UsdMayaShaderWriter for maya type %s with index %d.\n",
            mayaTypeName.GetText(),
            index);

    _reg.insert(std::make_pair(mayaTypeName, _RegistryEntry { pred, fn, index }));

    // The unloader uses the index to know which entry to erase when there are
    // more than one for the same mayaTypeName.
    UsdMaya_RegistryHelper::AddUnloader([mayaTypeName, index]() {
        _Registry::const_iterator it, itEnd;
        std::tie(it, itEnd) = _reg.equal_range(mayaTypeName);
        for (; it != itEnd; ++it) {
            if (it->second._index == index) {
                _reg.erase(it);
                break;
            }
        }
    });
}

/* static */
UsdMayaShaderWriterRegistry::WriterFactoryFn UsdMayaShaderWriterRegistry::Find(
    const TfToken&              mayaTypeName,
    const UsdMayaJobExportArgs& exportArgs)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaShaderWriterRegistry>();

    _Registry::const_iterator it = _Find(mayaTypeName, exportArgs);

    if (it != _reg.end()) {
        return it->second._writer;
    }

    // Try adding more writers via plugin load:
    static const TfTokenVector SCOPE = { _tokens->UsdMaya, _tokens->ShaderWriter };
    UsdMaya_RegistryHelper::FindAndLoadMayaPlug(SCOPE, mayaTypeName);

    it = _Find(mayaTypeName, exportArgs);

    if (it != _reg.end()) {
        return it->second._writer;
    }

    if (_reg.count(mayaTypeName) == 0) {
        // Nothing registered at all, remember that:
        _reg.insert(std::make_pair(
            mayaTypeName,
            _RegistryEntry { [](const UsdMayaJobExportArgs&) {
                                return UsdMayaShaderWriter::ContextSupport::Fallback;
                            },
                             nullptr,
                             -1 }));
    }

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
