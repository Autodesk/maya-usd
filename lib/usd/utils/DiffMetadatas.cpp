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
#include "DiffPrims.h"

#include <pxr/base/tf/type.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/sdf/schema.h>
#include <pxr/usd/sdf/valueTypeName.h>

namespace MayaUsdUtils {

using UsdObject = PXR_NS::UsdObject;
using TfToken = PXR_NS::TfToken;
using VtValue = PXR_NS::VtValue;
using UsdMetadataValueMap = PXR_NS::UsdMetadataValueMap;

std::unordered_set<TfToken, TfToken::HashFunctor>& getIgnoredMetadata()
{
    // clang-format off
    static std::unordered_set<TfToken, TfToken::HashFunctor> ignored({
        PXR_NS::SdfFieldKeys->Specifier,            // The prim specifier: def, class, over, etc. Must not diff nor copy.
        PXR_NS::SdfFieldKeys->AllowedTokens,        // Tokens allowed on a specific attribute. We must not mess this up.
        PXR_NS::SdfFieldKeys->Default,              // We should not be modifying defaults.
        PXR_NS::SdfFieldKeys->DefaultPrim,          // Prim used for missing payload. We probably don't want to mess this up during merge.
        PXR_NS::SdfFieldKeys->TimeSamples,          // Map of time (double) to data. We already manage time samples, so don't compare them as metadata.
        PXR_NS::SdfFieldKeys->SubLayers,            // List of sub-layers names, we should not to deal with this when merging.
        PXR_NS::SdfFieldKeys->SubLayerOffsets,      // Time offset and scaling for the sub-layers. We treat animation data at the level it is already applied.
        PXR_NS::SdfFieldKeys->TypeName,             // Property data type. We should not have to copy this over by hand.
    });

    // These other build-in metadata are allowed to be compared and merged:
    //
    // SdfFieldKeys->Active,                // USD pseudo-delete.
    // SdfFieldKeys->AssetInfo,             // Information for asset-management systems.
    // SdfFieldKeys->ColorConfiguration,    // Color-menanagement.
    // SdfFieldKeys->ColorManagementSystem, // Color-menanagement.
    // SdfFieldKeys->ColorSpace,            // Color-menanagement.
    // SdfFieldKeys->Comment,               // User-comments.
    // SdfFieldKeys->ConnectionPaths,       // The connections to other prims, used for attributes. (path list op)
    // SdfFieldKeys->Custom,                // Marks an attribute as being user custom data.
    // SdfFieldKeys->CustomData,            // User custom metadata on prims, attributes, etc.
    // SdfFieldKeys->CustomLayerData,       // User custom metadata on layers.
    // SdfFieldKeys->DisplayGroup,          // UI hinting to group properties.
    // SdfFieldKeys->DisplayGroupOrder,     // Order of display group. (No docs)
    // SdfFieldKeys->DisplayName,           // UI name of a property.
    // SdfFieldKeys->DisplayUnit,           // UI dsplay unit of an attribute.
    // SdfFieldKeys->Documentation,         // User-written documentation for any field.
    // SdfFieldKeys->EndFrame,              // Deprecated end-time of a layer. (Replaced by EndTimeCode)
    // SdfFieldKeys->EndTimeCode,           // End-time of a layer.
    // SdfFieldKeys->FramePrecision,        // Frame precision (related to frame rate, but is an integer)
    // SdfFieldKeys->FramesPerSecond,       // Frame per second for playback, superseded by TimeCodesPerSecond. (FPS is advisory)
    // SdfFieldKeys->Hidden,                // If a prim or field is hidden.
    // SdfFieldKeys->HasOwnedSubLayers,     // If a layer sub-layers are owned.
    // SdfFieldKeys->InheritPaths,          // The inherited prim classes. (path list op)
    // SdfFieldKeys->Instanceable,          // Prim is instanceable.
    // SdfFieldKeys->Kind,                  // The prim kind, an extendable taxonomy of prims.
    // SdfFieldKeys->Owner,                 // Owner of a layer.
    // SdfFieldKeys->PrimOrder,             // Order of prim children.
    // SdfFieldKeys->NoLoadHint,            // Hint to not load a payload. (No docs)
    // SdfFieldKeys->Payload,               // Payload references. (payload list op)
    // SdfFieldKeys->Permission,            // Public / private setting: private prim can only be accessed within the local layer.
    // SdfFieldKeys->Prefix,                // Property prefix. (No docs)
    // SdfFieldKeys->PrefixSubstitutions,   // Property prefix substitions dictionary. (No docs)
    // SdfFieldKeys->PropertyOrder,         // Order of properties in a prim.
    // SdfFieldKeys->References,            // References to other prims. (ref list op)
    // SdfFieldKeys->SessionOwner,          // Layer session owner.
    // SdfFieldKeys->TargetPaths,           // Target of a relation. (path list op)
    // SdfFieldKeys->Relocates,             // Map of path to path of relocations.
    // SdfFieldKeys->Specializes,           // Specialize connection (path list op)
    // SdfFieldKeys->StartFrame,            // Deprecated start-time of a layer. (Replaced by StartTimeCode)
    // SdfFieldKeys->StartTimeCode,         // Start-time of a layer.
    // SdfFieldKeys->Suffix,                // Property suffix. (No docs)
    // SdfFieldKeys->SuffixSubstitutions,   // Property suffix substitions dictionary. (No docs)
    // SdfFieldKeys->SymmetricPeer,         // Property symmetry. (No docs)
    // SdfFieldKeys->SymmetryArgs,          // Property symmetry. (No docs)
    // SdfFieldKeys->SymmetryArguments,     // Property symmetry. (No docs)
    // SdfFieldKeys->SymmetryFunction,      // Property symmetry. (No docs)
    // SdfFieldKeys->TimeCodesPerSecond,    // Time code per second for playback (TCPS is advisory)
    // SdfFieldKeys->Variability,           // Control if the property can be animated.
    // SdfFieldKeys->VariantSetNames,       // The merge/copy process will take care of copying the selected variant.
    // SdfFieldKeys->VariantSelection,      // The merge/copy process will take care of copying the selected variant.
    // clang-format on
    return ignored;
}

DiffResult compareMetadatas(
    const PXR_NS::UsdObject& modified,
    const PXR_NS::UsdObject& baseline,
    const PXR_NS::TfToken&   metadata)
{
    VtValue modifiedData;
    modified.GetMetadata(metadata, &modifiedData);

    VtValue baselineData;
    baseline.GetMetadata(metadata, &baselineData);

    return compareValues(modifiedData, baselineData);
}

#define USD_MAYA_RETURN_QUICK_RESULT(result)           \
    do {                                               \
        if (quickDiff && result != DiffResult::Same) { \
            *quickDiff = result;                       \
            return results;                            \
        }                                              \
    } while (false)

DiffResultPerToken
compareObjectsMetadatas(const UsdObject& modified, const UsdObject& baseline, DiffResult* quickDiff)
{
    DiffResultPerToken results;

    if (quickDiff)
        *quickDiff = DiffResult::Same;

    // Retrieve the list of ignored metadatas and cache its end iterator.
    const auto& ignored = getIgnoredMetadata();
    const auto  ignoredEnd = ignored.end();
    const auto  isIgnored = [&ignored, &ignoredEnd](const TfToken& token) {
        return ignored.find(token) != ignoredEnd;
    };

    // Create a map of baseline metadata indexed by name to rapidly verify
    // if it exists and be able to compare metadatas.
    const UsdMetadataValueMap baselineMetadatas = baseline.GetAllAuthoredMetadata();

    // Compare the metadatas from the new object.
    // Baseline metadata map won't change from now on, so cache the end.
    {
        const auto baselineEnd = baselineMetadatas.end();
        for (const auto& nameAndValue : modified.GetAllAuthoredMetadata()) {
            const TfToken& name = nameAndValue.first;
            if (isIgnored(name))
                continue;
            const auto iter = baselineMetadatas.find(name);
            if (iter == baselineEnd) {
                USD_MAYA_RETURN_QUICK_RESULT(DiffResult::Created);
                results[name] = DiffResult::Created;
            } else {
                const DiffResult result = compareValues(nameAndValue.second, iter->second);
                USD_MAYA_RETURN_QUICK_RESULT(result);
                results[name] = result;
            }
        }
    }

    // Identify metadatas that are absent in the new object.
    for (const auto& nameAndAttr : baselineMetadatas) {
        const auto& name = nameAndAttr.first;
        if (isIgnored(name))
            continue;
        if (results.find(name) == results.end()) {
            USD_MAYA_RETURN_QUICK_RESULT(DiffResult::Absent);
            results[name] = DiffResult::Absent;
        }
    }

    return results;
}

} // namespace MayaUsdUtils
