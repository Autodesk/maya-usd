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
#include "MergePrimsOptions.h"

#include <mutex>

namespace MayaUsdUtils {

TF_DEFINE_PUBLIC_TOKENS(UsdMayaMergeOptionsTokens, USDMAYA_MERGE_OPTIONS_TOKENS);

namespace {

/// Extracts a bool at \p key from \p options, or false if it can't extract.
bool parseBoolean(const VtDictionary& options, const TfToken& key)
{
    if (!VtDictionaryIsHolding<bool>(options, key)) {
        TF_CODING_ERROR(
            "Dictionary is missing required key '%s' or key is "
            "not bool type",
            key.GetText());
        return false;
    }
    return VtDictionaryGet<bool>(options, key);
}

/// Extracts a MergeVerbosity array of tokens at \p key from \p options, or Default if it can't
/// extract.
MergeVerbosity parseVerbosity(
    const VtDictionary&  options,
    const TfToken&       key,
    const MergeVerbosity def = MergeVerbosity::Default)
{
    if (!VtDictionaryIsHolding<std::vector<VtValue>>(options, key)) {
        TF_CODING_ERROR(
            "Dictionary is missing required key '%s' or key is "
            "not a vector of tokens",
            key.GetText());
        return def;
    }

    MergeVerbosity verbosity = MergeVerbosity::None;

    const auto tokens = VtDictionaryGet<std::vector<VtValue>>(options, key);
    for (const auto& token : tokens) {
        if (UsdMayaMergeOptionsTokens->None == token)
            verbosity = verbosity | MergeVerbosity::None;
        if (UsdMayaMergeOptionsTokens->Same == token)
            verbosity = verbosity | MergeVerbosity::Same;
        if (UsdMayaMergeOptionsTokens->Differ == token)
            verbosity = verbosity | MergeVerbosity::Differ;
        if (UsdMayaMergeOptionsTokens->Child == token)
            verbosity = verbosity | MergeVerbosity::Child;
        if (UsdMayaMergeOptionsTokens->Children == token)
            verbosity = verbosity | MergeVerbosity::Children;
        if (UsdMayaMergeOptionsTokens->Failure == token)
            verbosity = verbosity | MergeVerbosity::Failure;
        if (UsdMayaMergeOptionsTokens->Default == token)
            verbosity = verbosity | MergeVerbosity::Default;
    }

    return verbosity;
}

/// Extracts a MergeMissing array of tokens at \p key from \p options, or All if it can't
/// extract.
MergeMissing parseMissingHandling(
    const VtDictionary& options,
    const TfToken&      key,
    MergeMissing        def = MergeMissing::All)
{
    if (!VtDictionaryIsHolding<std::vector<VtValue>>(options, key)) {
        TF_CODING_ERROR(
            "Dictionary is missing required key '%s' or key is "
            "not a vector of tokens",
            key.GetText());
        return def;
    }

    MergeMissing missingHandling = MergeMissing::None;

    const auto tokens = VtDictionaryGet<std::vector<VtValue>>(options, key);
    for (const auto& token : tokens) {
        if (UsdMayaMergeOptionsTokens->None == token)
            missingHandling = missingHandling | MergeMissing::None;
        if (UsdMayaMergeOptionsTokens->Create == token)
            missingHandling = missingHandling | MergeMissing::Create;
        if (UsdMayaMergeOptionsTokens->Preserve == token)
            missingHandling = missingHandling | MergeMissing::Preserve;
        if (UsdMayaMergeOptionsTokens->All == token)
            missingHandling = missingHandling | MergeMissing::All;
    }

    return missingHandling;
}

} // namespace

/* static */
const VtDictionary& MergePrimsOptions::getDefaultDictionary()
{
    static VtDictionary   d;
    static std::once_flag once;
    std::call_once(once, []() {
        d[UsdMayaMergeOptionsTokens->verbosity]
            = VtValue(std::vector<VtValue>({ VtValue(UsdMayaMergeOptionsTokens->Default) }));

        d[UsdMayaMergeOptionsTokens->mergeChildren] = false;
        d[UsdMayaMergeOptionsTokens->ignoreUpperLayerOpinions] = false;

        static const TfToken handlingTokens[] = { UsdMayaMergeOptionsTokens->propertiesHandling,
                                                  UsdMayaMergeOptionsTokens->primsHandling,
                                                  UsdMayaMergeOptionsTokens->connectionsHandling,
                                                  UsdMayaMergeOptionsTokens->relationshipsHandling,
                                                  UsdMayaMergeOptionsTokens->variantsHandling,
                                                  UsdMayaMergeOptionsTokens->variantSetsHandling,
                                                  UsdMayaMergeOptionsTokens->expressionsHandling,
                                                  UsdMayaMergeOptionsTokens->mappersHandling,
                                                  UsdMayaMergeOptionsTokens->mapperArgsHandling,
                                                  UsdMayaMergeOptionsTokens->propMetadataHandling,
                                                  UsdMayaMergeOptionsTokens->primMetadataHandling };

        for (const auto& ht : handlingTokens) {
            d[ht] = VtValue(std::vector<VtValue>({ VtValue(UsdMayaMergeOptionsTokens->All) }));
        }
    });

    return d;
}

MergePrimsOptions::MergePrimsOptions(const VtDictionary& options)
{
    // Make sure we have all options filled by merging over the default dictionary.
    const VtDictionary optionsWithDef = VtDictionaryOver(options, getDefaultDictionary());

    verbosity = parseVerbosity(optionsWithDef, UsdMayaMergeOptionsTokens->verbosity);

    mergeChildren = parseBoolean(optionsWithDef, UsdMayaMergeOptionsTokens->mergeChildren);

    ignoreUpperLayerOpinions
        = parseBoolean(optionsWithDef, UsdMayaMergeOptionsTokens->ignoreUpperLayerOpinions);

    const struct
    {
        TfToken       handlingToken;
        MergeMissing& handlingValue;
    } missingHandlings[]
        = { { UsdMayaMergeOptionsTokens->propertiesHandling, this->propertiesHandling },
            { UsdMayaMergeOptionsTokens->primsHandling, this->primsHandling },
            { UsdMayaMergeOptionsTokens->connectionsHandling, this->connectionsHandling },
            { UsdMayaMergeOptionsTokens->relationshipsHandling, this->relationshipsHandling },
            { UsdMayaMergeOptionsTokens->variantsHandling, this->variantsHandling },
            { UsdMayaMergeOptionsTokens->variantSetsHandling, this->variantSetsHandling },
            { UsdMayaMergeOptionsTokens->expressionsHandling, this->expressionsHandling },
            { UsdMayaMergeOptionsTokens->mappersHandling, this->mappersHandling },
            { UsdMayaMergeOptionsTokens->mapperArgsHandling, this->mapperArgsHandling },
            { UsdMayaMergeOptionsTokens->propMetadataHandling, this->propMetadataHandling },
            { UsdMayaMergeOptionsTokens->primMetadataHandling, this->primMetadataHandling } };

    for (const auto& handling : missingHandlings) {
        handling.handlingValue = parseMissingHandling(optionsWithDef, handling.handlingToken);
    }
}

MergePrimsOptions::MergePrimsOptions()
    : MergePrimsOptions(MergePrimsOptions::getDefaultDictionary())
{
}

} // namespace MayaUsdUtils
