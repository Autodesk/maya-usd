//
// Copyright 2026 Autodesk
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
#include <mayaUsd/base/stringResources.h>
#include <mayaUsdUI/ui/stringResources.h>
#include <usdLayerEditor/ui/stringResources.h>

#include <usdUfe/base/stringResources.h>

#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

// All four libraries register their strings in the same DCC catalog (Maya's "mayaUsdPlugin"), so a
// key defined twice, within one list or across lists, would make one string overwrite or shadow
// the other. This reads the X-macro lists directly and does not need the libraries to be loaded.

namespace {

struct KeyOwner
{
    std::string library;
    std::string key;
};

std::vector<KeyOwner> allKeys()
{
    std::vector<KeyOwner> keys;

#define ADD_USDUFE_KEY(key, value) keys.push_back({ "UsdUfe", #key });
    USDUFE_STRING_LIST(ADD_USDUFE_KEY)
#undef ADD_USDUFE_KEY

#define ADD_LAYEREDITOR_KEY(key, value) keys.push_back({ "UsdLayerEditor", #key });
    USDLAYEREDITOR_STRING_LIST(ADD_LAYEREDITOR_KEY)
#undef ADD_LAYEREDITOR_KEY

#define ADD_MAYAUSD_KEY(key, value) keys.push_back({ "MayaUsd", #key });
    MAYAUSD_STRING_LIST(ADD_MAYAUSD_KEY)
#undef ADD_MAYAUSD_KEY

#define ADD_MAYAUSDUI_KEY(key, value) keys.push_back({ "MayaUsdUI", #key });
    MAYAUSDUI_STRING_LIST(ADD_MAYAUSDUI_KEY)
#undef ADD_MAYAUSDUI_KEY

    return keys;
}

size_t countKeysOf(const std::vector<KeyOwner>& keys, const std::string& library)
{
    size_t count = 0;
    for (const auto& entry : keys)
        count += entry.library == library ? 1 : 0;
    return count;
}

} // namespace

TEST(StringResourceKeys, everyLibraryContributesKeys)
{
    // Guards the test itself: a list that expanded to nothing would pass the duplicate check.
    const auto keys = allKeys();
    for (const char* library : { "UsdUfe", "UsdLayerEditor", "MayaUsd", "MayaUsdUI" })
        EXPECT_GT(countKeysOf(keys, library), 0u) << "no keys from " << library;
}

TEST(StringResourceKeys, noKeyIsDefinedTwice)
{
    std::map<std::string, std::string> firstOwner;
    for (const auto& entry : allKeys()) {
        const auto inserted = firstOwner.emplace(entry.key, entry.library);
        if (!inserted.second) {
            ADD_FAILURE() << "key " << entry.key << " is defined in " << inserted.first->second
                          << " and again in " << entry.library;
        }
    }
}
