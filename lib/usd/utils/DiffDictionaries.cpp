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

using VtDictionary = PXR_NS::VtDictionary;

#define USD_MAYA_RETURN_QUICK_RESULT(result)           \
    do {                                               \
        if (quickDiff && result != DiffResult::Same) { \
            *quickDiff = result;                       \
            return results;                            \
        }                                              \
    } while (false)

DiffResultPerKey compareDictionaries(
    const VtDictionary& modified,
    const VtDictionary& baseline,
    DiffResult*         quickDiff)
{
    DiffResultPerKey results;

    if (quickDiff)
        *quickDiff = DiffResult::Same;

    // Compare the values to find created or changed ones.
    // Baseline dictionary map won't change, so cache the end.
    {
        const auto baselineEnd = baseline.end();
        for (const auto& keyAndValue : modified) {
            const std::string& key = keyAndValue.first;
            const auto         iter = baseline.find(key);
            if (iter == baselineEnd) {
                USD_MAYA_RETURN_QUICK_RESULT(DiffResult::Created);
                results[key] = DiffResult::Created;
            } else {
                const DiffResult result = compareValues(keyAndValue.second, iter->second);
                USD_MAYA_RETURN_QUICK_RESULT(result);
                results[key] = result;
            }
        }
    }

    // Identify values that are absent in the new dictionary.
    // Modified dictionary map won't change, so cache the end.
    {
        const auto modifiedEnd = modified.end();
        for (const auto& keyAndAttr : baseline) {
            const auto& key = keyAndAttr.first;
            if (modified.find(key) == modifiedEnd) {
                USD_MAYA_RETURN_QUICK_RESULT(DiffResult::Absent);
                results[key] = DiffResult::Absent;
            }
        }
    }

    return results;
}

} // namespace MayaUsdUtils
