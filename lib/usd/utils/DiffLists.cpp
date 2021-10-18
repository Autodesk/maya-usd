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

namespace MayaUsdUtils {

using TfToken = PXR_NS::TfToken;
using SdfPath = PXR_NS::SdfPath;
using SdfReference = PXR_NS::SdfReference;
using SdfPayload = PXR_NS::SdfPayload;

template <class ITEM>
std::map<ITEM, DiffResult>
compareLists(const std::vector<ITEM>& modified, const std::vector<ITEM>& baseline)
{
    std::map<ITEM, DiffResult> results;

    // Note: doing comparisons of two lists would require a general diff algorithm to find the
    // minimal diff. On top of that, the lists in USD are composed and are built from prepend,
    // append, remove and reorder operations on top of lower-level compositions. We assume that
    // changes will mostly be additions at the beginning or end of the list, with a central
    // unchanged core.

    // Target lists won't change from now on, so cache the ends.
    const auto baselineEnd = baseline.end();
    const auto modifiedEnd = modified.end();

    auto baselineIter = baseline.begin();
    auto modifiedIter = modified.begin();

    // Find all initial modified targets that don't match the baseline first target.
    // These will correspond to prepend.
    // TODO: improve by finding the first baseline target that matches, mark previous ones absent?
    for (; modifiedIter != modifiedEnd; ++modifiedIter) {
        if (baselineIter == baselineEnd) {
            results[*modifiedIter] = DiffResult::Prepended;
            continue;
        } else if (*baselineIter == *modifiedIter) {
            break;
        } else {
            results[*modifiedIter] = DiffResult::Prepended;
        }
    }

    // Find matching middle part.
    for (; modifiedIter != modifiedEnd; ++modifiedIter) {
        if (baselineIter == baselineEnd) {
            break;
        } else if (*baselineIter == *modifiedIter) {
            results[*modifiedIter] = DiffResult::Same;
            ++baselineIter;
        } else {
            break;
        }
    }

    // Add all final modified targets that didn't match the baseline.
    // These will correspond to append.
    for (; modifiedIter != modifiedEnd; ++modifiedIter) {
        results[*modifiedIter] = DiffResult::Appended;
    }

    // Add all remaining baseline targets that didn't match the modified targets
    // and which don't have a result yet as absent. Otherwise, we add them as
    // reordered since an item needs to both be removed at a position and inserted
    // at a different position.
    for (; baselineIter != baselineEnd; ++baselineIter) {
        if (results.find(*baselineIter) == results.end()) {
            results[*baselineIter] = DiffResult::Absent;
        } else {
            results[*baselineIter] = DiffResult::Reordered;
        }
    }

    return results;
}

template MAYA_USD_UTILS_PUBLIC std::map<int, DiffResult>
compareLists<int>(const std::vector<int>& modified, const std::vector<int>& baseline);

template MAYA_USD_UTILS_PUBLIC std::map<unsigned int, DiffResult> compareLists<unsigned int>(
    const std::vector<unsigned int>& modified,
    const std::vector<unsigned int>& baseline);

template MAYA_USD_UTILS_PUBLIC std::map<int64_t, DiffResult>
compareLists<int64_t>(const std::vector<int64_t>& modified, const std::vector<int64_t>& baseline);

template MAYA_USD_UTILS_PUBLIC std::map<uint64_t, DiffResult> compareLists<uint64_t>(
    const std::vector<uint64_t>& modified,
    const std::vector<uint64_t>& baseline);

template MAYA_USD_UTILS_PUBLIC std::map<TfToken, DiffResult>
compareLists<TfToken>(const std::vector<TfToken>& modified, const std::vector<TfToken>& baseline);

template MAYA_USD_UTILS_PUBLIC std::map<std::string, DiffResult> compareLists<std::string>(
    const std::vector<std::string>& modified,
    const std::vector<std::string>& baseline);

template MAYA_USD_UTILS_PUBLIC std::map<SdfPath, DiffResult>
compareLists<SdfPath>(const std::vector<SdfPath>& modified, const std::vector<SdfPath>& baseline);

template MAYA_USD_UTILS_PUBLIC std::map<SdfReference, DiffResult> compareLists<SdfReference>(
    const std::vector<SdfReference>& modified,
    const std::vector<SdfReference>& baseline);

template MAYA_USD_UTILS_PUBLIC std::map<SdfPayload, DiffResult> compareLists<SdfPayload>(
    const std::vector<SdfPayload>& modified,
    const std::vector<SdfPayload>& baseline);

} // namespace MayaUsdUtils
