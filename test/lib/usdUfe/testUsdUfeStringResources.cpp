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
#include <usdUfe/base/stringResources.h>
#include <usdUfe/ufe/Utils.h>

#include <gtest/gtest.h>

#include <map>
#include <string>

namespace {

namespace UsdUfeSR = UsdUfe::StringResources;

// Starts every test without string hooks and clears them afterwards, so that none leaks into the
// next test.
class ScopedStringFns
{
public:
    ScopedStringFns() { clear(); }
    ~ScopedStringFns() { clear(); }

    ScopedStringFns(const ScopedStringFns&) = delete;
    ScopedStringFns& operator=(const ScopedStringFns&) = delete;

private:
    static void clear()
    {
        UsdUfe::setRegisterStringFn(nullptr);
        UsdUfe::setTranslateStringFn(nullptr);
    }
};

// Number of entries in the UsdUfe list. Counting the list itself, rather than the map built from
// it, is what exposes duplicate keys.
size_t usdUfeListSize()
{
    size_t count = 0;
#define COUNT_USDUFE_STR(key, value) ++count;
    USDUFE_STRING_LIST(COUNT_USDUFE_STR)
#undef COUNT_USDUFE_STR
    return count;
}

// The UsdUfe hooks are plain function pointers, so what they see is recorded in statics.
std::map<std::string, std::string> gRegistered;
std::string                        gSeenKey;
std::string                        gSeenSourceText;

void resetRecorded()
{
    gRegistered.clear();
    gSeenKey.clear();
    gSeenSourceText.clear();
}

bool registerAndSucceed(const std::string& key, const std::string& value)
{
    gRegistered[key] = value;
    return true;
}

bool registerButFailLoadLabel(const std::string& key, const std::string& value)
{
    gRegistered[key] = value;
    return key != "kUSDLoadLabel";
}

std::string translateAndRecord(const std::string& key, const std::string& sourceText)
{
    gSeenKey = key;
    gSeenSourceText = sourceText;
    return "from DCC";
}

} // namespace

TEST(UsdUfeStringResources, allResourcesMatchesTheList)
{
    const auto& resources = UsdUfeSR::allResources();

    // A duplicate key would collapse two list entries into one map entry.
    EXPECT_EQ(usdUfeListSize(), resources.size());

    for (const auto& entry : resources) {
        EXPECT_EQ(entry.first, entry.second._key);
        EXPECT_FALSE(entry.second._value.empty()) << "empty value for " << entry.first;
        // The Qt fallback looks the text up in this context.
        EXPECT_EQ("UsdUfe", entry.second._module) << "wrong context for " << entry.first;
    }
}

TEST(UsdUfeStringResources, resourcesAreNamedAfterTheirVariable)
{
    const auto& resources = UsdUfeSR::allResources();
    ASSERT_EQ(1u, resources.count("kUSDLoadLabel"));
    EXPECT_EQ("kUSDLoadLabel", UsdUfeSR::kUSDLoadLabel._key);
    EXPECT_EQ("Load", UsdUfeSR::kUSDLoadLabel._value);
}

TEST(UsdUfeStringResources, bulkEditLabelsFormatWithTheirPlaceholders)
{
    // UsdContextOps fills these in with StringResourceBase::format(): the count in ^1s and, for
    // the same-type label, the prim type in ^2s.
    EXPECT_EQ(
        "3 Prims Selected",
        StringResourceBase::format(GET_USDUFE_STR(kBulkEditMixedTypeLabel), std::to_string(3)));
    EXPECT_EQ(
        "3 Xform Prims Selected",
        StringResourceBase::format(
            GET_USDUFE_STR(kBulkEditSameTypeLabel), std::to_string(3), std::string("Xform")));
}

TEST(UsdUfeStringResources, bulkEditLabelsHaveNoPrintfSpecifiers)
{
    // They used to be printf format strings. A leftover specifier would show up verbatim.
    for (const auto* resource :
         { &UsdUfeSR::kBulkEditMixedTypeLabel, &UsdUfeSR::kBulkEditSameTypeLabel })
        EXPECT_EQ(std::string::npos, resource->_value.find('%')) << resource->_key;
}

TEST(UsdUfeStringResources, registerStringsFailsWithoutARegisterHook)
{
    ScopedStringFns scopedFns;

    EXPECT_FALSE(UsdUfeSR::registerStrings());
}

TEST(UsdUfeStringResources, registerStringsRegistersEveryResource)
{
    ScopedStringFns scopedFns;
    resetRecorded();
    UsdUfe::setRegisterStringFn(registerAndSucceed);

    EXPECT_TRUE(UsdUfeSR::registerStrings());

    const auto& resources = UsdUfeSR::allResources();
    ASSERT_EQ(resources.size(), gRegistered.size());
    for (const auto& entry : resources) {
        const auto found = gRegistered.find(entry.first);
        ASSERT_NE(gRegistered.end(), found) << "not registered: " << entry.first;
        EXPECT_EQ(entry.second._value, found->second);
    }
}

TEST(UsdUfeStringResources, registerStringsKeepsGoingAfterAFailure)
{
    ScopedStringFns scopedFns;
    resetRecorded();
    UsdUfe::setRegisterStringFn(registerButFailLoadLabel);

    EXPECT_FALSE(UsdUfeSR::registerStrings());

    // The failing resource did not stop the others from being registered.
    EXPECT_EQ(UsdUfeSR::allResources().size(), gRegistered.size());
}

TEST(UsdUfeStringResources, registerResourcesRegistersEachResourceOfTheGivenMap)
{
    ScopedStringFns scopedFns;
    resetRecorded();
    UsdUfe::setRegisterStringFn(registerAndSucceed);

    // The layer editor uses this to register its own strings through the UsdUfe hook.
    const StringResourceBase::Resource one { "ctx", "kOne", "One" };
    const StringResourceBase::Resource two { "ctx", "kTwo", "Two" };
    EXPECT_TRUE(UsdUfeSR::registerResources(StringResourceBase::makeResourceMap({ &one, &two })));

    EXPECT_EQ(2u, gRegistered.size());
    EXPECT_EQ("One", gRegistered["kOne"]);
    EXPECT_EQ("Two", gRegistered["kTwo"]);
}

TEST(UsdUfeStringResources, registerResourcesFailsWithoutARegisterHook)
{
    ScopedStringFns scopedFns;

    const StringResourceBase::Resource one { "ctx", "kOne", "One" };
    EXPECT_FALSE(UsdUfeSR::registerResources(StringResourceBase::makeResourceMap({ &one })));
}

TEST(UsdUfeStringResources, getAsStringFallsBackToSourceTextWithoutAHook)
{
    ScopedStringFns scopedFns;

    EXPECT_EQ(UsdUfeSR::kUSDLoadLabel._value, GET_USDUFE_STR(kUSDLoadLabel));
}

TEST(UsdUfeStringResources, getAsStringUsesTheTranslateHook)
{
    ScopedStringFns scopedFns;
    resetRecorded();
    UsdUfe::setTranslateStringFn(translateAndRecord);

    EXPECT_EQ("from DCC", GET_USDUFE_STR(kUSDLoadLabel));
    EXPECT_EQ("kUSDLoadLabel", gSeenKey);
    EXPECT_EQ(UsdUfeSR::kUSDLoadLabel._value, gSeenSourceText);
}

TEST(UsdUfeStringHooks, registerStringWithoutHookReturnsFalse)
{
    ScopedStringFns scopedFns;

    EXPECT_FALSE(UsdUfe::registerString("kKey", "text"));
}

TEST(UsdUfeStringHooks, registerStringForwardsToTheHook)
{
    ScopedStringFns scopedFns;
    resetRecorded();
    UsdUfe::setRegisterStringFn(registerAndSucceed);

    EXPECT_TRUE(UsdUfe::registerString("kKey", "text"));
    EXPECT_EQ("text", gRegistered["kKey"]);
}

TEST(UsdUfeStringHooks, translateStringWithoutHookReturnsSourceText)
{
    ScopedStringFns scopedFns;

    EXPECT_FALSE(UsdUfe::haveTranslateStringFn());
    EXPECT_EQ("source", UsdUfe::translateString("kKey", "source"));
}

TEST(UsdUfeStringHooks, translateStringForwardsToTheHook)
{
    ScopedStringFns scopedFns;
    resetRecorded();
    UsdUfe::setTranslateStringFn(translateAndRecord);

    EXPECT_TRUE(UsdUfe::haveTranslateStringFn());
    EXPECT_EQ("from DCC", UsdUfe::translateString("kKey", "source"));
    EXPECT_EQ("kKey", gSeenKey);
    EXPECT_EQ("source", gSeenSourceText);

    // Clearing the hook restores the pass-through behaviour.
    UsdUfe::setTranslateStringFn(nullptr);
    EXPECT_FALSE(UsdUfe::haveTranslateStringFn());
    EXPECT_EQ("source", UsdUfe::translateString("kKey", "source"));
}
