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

DiffResultPerKey compareDictionaries(const VtDictionary& modified, const VtDictionary& baseline)
{
    DiffResultPerKey results;

    // Compare the values to find created or changed ones.
    // Baseline dictionary map won't change, so cache the end.
    {
        const auto baselineEnd = baseline.end();
        for (const auto& keyAndValue : modified) {
            const std::string& key = keyAndValue.first;
            const auto iter = baseline.find(key);
            if (iter == baselineEnd) {
                results[key] = DiffResult::Created;
            } else {
                results[key] = compareValues(keyAndValue.second, iter->second);
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
                results[key] = DiffResult::Absent;
            }
        }
    }

    return results;
}

} // keyspace MayaUsdUtils
