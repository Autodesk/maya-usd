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
#pragma once

#include <mayaUsdUtils/Api.h>

#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>

namespace MayaUsdUtils {

//----------------------------------------------------------------------------------------------------------------------
/// MergeVerbosity level flags.

enum class MergeVerbosity
{
    None = 0,
    Same = 1 << 0,
    Differ = 1 << 1,
    Child = 1 << 2,
    Children = 1 << 3,
    Failure = 1 << 4,
    Default = Differ | Child | Children | Failure,
};

inline MergeVerbosity operator|(MergeVerbosity a, MergeVerbosity b)
{
    return MergeVerbosity(uint32_t(a) | uint32_t(b));
}

inline MergeVerbosity operator&(MergeVerbosity a, MergeVerbosity b)
{
    return MergeVerbosity(uint32_t(a) & uint32_t(b));
}

inline MergeVerbosity operator^(MergeVerbosity a, MergeVerbosity b)
{
    return MergeVerbosity(uint32_t(a) ^ uint32_t(b));
}

inline bool contains(MergeVerbosity a, MergeVerbosity b) { return (a & b) != MergeVerbosity::None; }

//----------------------------------------------------------------------------------------------------------------------
// Missing field handling flags.

enum class MergeMissing
{
    None = 0,

    // If set, attributes found only in the source are created in the destination.
    Create = 1 << 0,

    // If set, attributes missing from the source are preserved in the destination.
    Preserve = 1 << 1,

    All = Create | Preserve,
};

inline MergeMissing operator|(MergeMissing a, MergeMissing b)
{
    return MergeMissing(uint32_t(a) | uint32_t(b));
}

inline MergeMissing operator&(MergeMissing a, MergeMissing b)
{
    return MergeMissing(uint32_t(a) & uint32_t(b));
}

inline MergeMissing operator^(MergeMissing a, MergeMissing b)
{
    return MergeMissing(uint32_t(a) ^ uint32_t(b));
}

inline bool contains(MergeMissing a, MergeMissing b) { return (a & b) != MergeMissing::None; }

//----------------------------------------------------------------------------------------------------------------------
// Options to control prims merging.
//
// To simplify the constructors and initialization in the unit test,
// the individual member variables are not declared const, but the
// whole structure is passed const to functions receiving it.

struct MergePrimsOptions
{
    // How much logging is done during the merge.
    MergeVerbosity verbosity { MergeVerbosity::Default };

    // if true, merges children too, otherwise merge only the given prim.
    bool mergeChildren { false };

    // If true, the merge is done in a temporary layer so to ignore opinions
    // from upper layers (and children of upper layers).
    bool ignoreUpperLayerOpinions { false };

    // How missing attributes are handled.
    MergeMissing propertiesHandling { MergeMissing::All };

    // How missing prim children are handled.
    MergeMissing primsHandling { MergeMissing::All };

    // How missing connections are handled.
    MergeMissing connectionsHandling { MergeMissing::All };

    // How missing relationships are handled.
    MergeMissing relationshipsHandling { MergeMissing::All };

    // How missing variants are handled.
    MergeMissing variantsHandling { MergeMissing::All };

    // How missing variant sets are handled.
    MergeMissing variantSetsHandling { MergeMissing::All };

    // How missing expressions are handled.
    MergeMissing expressionsHandling { MergeMissing::All };

    // How missing mappers are handled.
    MergeMissing mappersHandling { MergeMissing::All };

    // How missing mapper argumentss are handled.
    MergeMissing mapperArgsHandling { MergeMissing::All };

    // Create a VtDictionary containing the default values for the merge options.
    MAYA_USD_UTILS_PUBLIC
    static const PXR_NS::VtDictionary& getDefaultDictionary();

    // Constructs a MergePrimsOptions with the given options.
    // Not all options need to be filled, missing ones will use the defaults.
    MAYA_USD_UTILS_PUBLIC
    MergePrimsOptions(const PXR_NS::VtDictionary& options);

    // Constructs a MergePrimsOptions with the default options.
    MAYA_USD_UTILS_PUBLIC
    MergePrimsOptions();
};

//----------------------------------------------------------------------------------------------------------------------
// Options tokens used in the default options dictionary.

// clang-format off
#define USDMAYA_MERGE_OPTIONS_TOKENS    \
    /* Dictionary keys */               \
    (verbosity)                         \
                                        \
    (None)                              \
    (Same)                              \
    (Differ)                            \
    (Child)                             \
    (Children)                          \
    (Failure)                           \
    (Default)                           \
                                        \
    (mergeChildren)                     \
    (ignoreUpperLayerOpinions)          \
                                        \
    (propertiesHandling)                \
    (primsHandling)                     \
    (connectionsHandling)               \
    (relationshipsHandling)             \
    (variantsHandling)                  \
    (variantSetsHandling)               \
    (expressionsHandling)               \
    (mappersHandling)                   \
    (mapperArgsHandling)                \
                                        \
    (Create)                            \
    (Preserve)                          \
    (All)                               \
                                        \
// clang-format on

// Note: the macro below does not qualify the types it uses with the PXR namespace,
// so we're forced to use the types it uses here.
PXR_NAMESPACE_USING_DIRECTIVE

TF_DECLARE_PUBLIC_TOKENS(
    UsdMayaMergeOptionsTokens,
    MAYA_USD_UTILS_PUBLIC,
    USDMAYA_MERGE_OPTIONS_TOKENS);

} // namespace MayaUsdUtils
