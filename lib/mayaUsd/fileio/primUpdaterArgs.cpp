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

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdMayaPrimUpdaterArgsTokens, PXRUSDMAYA_UPDATER_ARGS_TOKENS);

/// Extracts a bool at \p key from \p userArgs, or false if it can't extract.
static bool _Boolean(const VtDictionary& userArgs, const TfToken& key)
{
    if (!VtDictionaryIsHolding<bool>(userArgs, key)) {
        TF_CODING_ERROR(
            "Dictionary is missing required key '%s' or key is "
            "not bool type",
            key.GetText());
        return false;
    }
    return VtDictionaryGet<bool>(userArgs, key);
}

UsdMayaPrimUpdaterArgs::UsdMayaPrimUpdaterArgs(const VtDictionary& userArgs)
 : _copyOperation(_Boolean(userArgs, UsdMayaPrimUpdaterArgsTokens->copyOperation))
{
}

/*static*/
UsdMayaPrimUpdaterArgs UsdMayaPrimUpdaterArgs::createFromDictionary(const VtDictionary& userArgs)
{
    return UsdMayaPrimUpdaterArgs(
        VtDictionaryOver(userArgs, getDefaultDictionary()));
}

/*static*/
const VtDictionary& UsdMayaPrimUpdaterArgs::getDefaultDictionary()
{
    static VtDictionary   d;
    static std::once_flag once;
    std::call_once(once, []() {
        d[UsdMayaPrimUpdaterArgsTokens->copyOperation] = false;
    });

    return d;
}

PXR_NAMESPACE_CLOSE_SCOPE
