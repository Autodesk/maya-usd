//
// Copyright 2024 Autodesk
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
#include "mergePrimsOptions.h"

#include <mutex>

namespace USDUFE_NS_DEF {

TF_DEFINE_PUBLIC_TOKENS(MergeOptionsTokens, USDUFE_MERGE_OPTIONS_TOKENS);

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
        if (MergeOptionsTokens->None == token)
            verbosity = verbosity | MergeVerbosity::None;
        if (MergeOptionsTokens->Same == token)
            verbosity = verbosity | MergeVerbosity::Same;
        if (MergeOptionsTokens->Differ == token)
            verbosity = verbosity | MergeVerbosity::Differ;
        if (MergeOptionsTokens->Child == token)
            verbosity = verbosity | MergeVerbosity::Child;
        if (MergeOptionsTokens->Children == token)
            verbosity = verbosity | MergeVerbosity::Children;
        if (MergeOptionsTokens->Failure == token)
            verbosity = verbosity | MergeVerbosity::Failure;
        if (MergeOptionsTokens->Default == token)
            verbosity = verbosity | MergeVerbosity::Default;
        if (MergeOptionsTokens->All == token)
            verbosity = verbosity | MergeVerbosity::All;
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
        if (MergeOptionsTokens->None == token)
            missingHandling = missingHandling | MergeMissing::None;
        if (MergeOptionsTokens->Create == token)
            missingHandling = missingHandling | MergeMissing::Create;
        if (MergeOptionsTokens->Preserve == token)
            missingHandling = missingHandling | MergeMissing::Preserve;
        if (MergeOptionsTokens->All == token)
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
        d[MergeOptionsTokens->verbosity]
            = VtValue(std::vector<VtValue>({ VtValue(MergeOptionsTokens->Default) }));

        d[MergeOptionsTokens->mergeChildren] = false;
        d[MergeOptionsTokens->ignoreUpperLayerOpinions] = false;

        static const TfToken handlingTokens[]
            = { MergeOptionsTokens->propertiesHandling,  MergeOptionsTokens->primsHandling,
                MergeOptionsTokens->connectionsHandling, MergeOptionsTokens->relationshipsHandling,
                MergeOptionsTokens->variantsHandling,    MergeOptionsTokens->variantSetsHandling,
                MergeOptionsTokens->expressionsHandling, MergeOptionsTokens->mappersHandling,
                MergeOptionsTokens->mapperArgsHandling,  MergeOptionsTokens->propMetadataHandling,
                MergeOptionsTokens->primMetadataHandling };

        for (const auto& ht : handlingTokens) {
            d[ht] = VtValue(std::vector<VtValue>({ VtValue(MergeOptionsTokens->All) }));
        }
    });

    return d;
}

MergePrimsOptions::MergePrimsOptions(const VtDictionary& options)
{
    // Make sure we have all options filled by merging over the default dictionary.
    const VtDictionary optionsWithDef = VtDictionaryOver(options, getDefaultDictionary());

    verbosity = parseVerbosity(optionsWithDef, MergeOptionsTokens->verbosity);

    mergeChildren = parseBoolean(optionsWithDef, MergeOptionsTokens->mergeChildren);

    ignoreUpperLayerOpinions
        = parseBoolean(optionsWithDef, MergeOptionsTokens->ignoreUpperLayerOpinions);

    const struct
    {
        TfToken       handlingToken;
        MergeMissing& handlingValue;
    } missingHandlings[]
        = { { MergeOptionsTokens->propertiesHandling, this->propertiesHandling },
            { MergeOptionsTokens->primsHandling, this->primsHandling },
            { MergeOptionsTokens->connectionsHandling, this->connectionsHandling },
            { MergeOptionsTokens->relationshipsHandling, this->relationshipsHandling },
            { MergeOptionsTokens->variantsHandling, this->variantsHandling },
            { MergeOptionsTokens->variantSetsHandling, this->variantSetsHandling },
            { MergeOptionsTokens->expressionsHandling, this->expressionsHandling },
            { MergeOptionsTokens->mappersHandling, this->mappersHandling },
            { MergeOptionsTokens->mapperArgsHandling, this->mapperArgsHandling },
            { MergeOptionsTokens->propMetadataHandling, this->propMetadataHandling },
            { MergeOptionsTokens->primMetadataHandling, this->primMetadataHandling } };

    for (const auto& handling : missingHandlings) {
        handling.handlingValue = parseMissingHandling(optionsWithDef, handling.handlingToken);
    }
}

MergePrimsOptions::MergePrimsOptions()
    : MergePrimsOptions(MergePrimsOptions::getDefaultDictionary())
{
}

} // namespace USDUFE_NS_DEF
