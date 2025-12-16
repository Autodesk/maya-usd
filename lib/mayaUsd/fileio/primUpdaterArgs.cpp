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
#include "primUpdaterArgs.h"

#include <mayaUsd/utils/utilDictionary.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdMayaPrimUpdaterArgsTokens, PXRUSDMAYA_UPDATER_ARGS_TOKENS);

using namespace MayaUsd::DictUtils;

UsdMayaPrimUpdaterArgs::UsdMayaPrimUpdaterArgs(const VtDictionary& userArgs)
    : _copyOperation(extractBoolean(userArgs, UsdMayaPrimUpdaterArgsTokens->copyOperation))
    , _ignoreVariants(extractBoolean(userArgs, UsdMayaPrimUpdaterArgsTokens->ignoreVariants))
    , _pushNodeList(
          extractVector<std::string>(userArgs, UsdMayaPrimUpdaterArgsTokens->pushNodeList))
{
}

/*static*/
UsdMayaPrimUpdaterArgs UsdMayaPrimUpdaterArgs::createFromDictionary(const VtDictionary& userArgs)
{
    return UsdMayaPrimUpdaterArgs(VtDictionaryOver(userArgs, getDefaultDictionary()));
}

/*static*/
const VtDictionary& UsdMayaPrimUpdaterArgs::getDefaultDictionary()
{
    static VtDictionary   d;
    static std::once_flag once;
    std::call_once(once, []() {
        d[UsdMayaPrimUpdaterArgsTokens->copyOperation] = false;
        d[UsdMayaPrimUpdaterArgsTokens->ignoreVariants] = false;
        d[UsdMayaPrimUpdaterArgsTokens->pushNodeList] = std::vector<VtValue>();
    });

    return d;
}

PXR_NAMESPACE_CLOSE_SCOPE
