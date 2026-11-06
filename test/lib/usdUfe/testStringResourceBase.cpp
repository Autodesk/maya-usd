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
#include <stringResourceBase.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace StringResourceBase;

TEST(StringResourceBase, createVectorConvertsEachArgument)
{
    const std::string        two("two");
    std::vector<std::string> expected { "one", "two", "three" };
    EXPECT_EQ(expected, createVector("one", two, std::string("three")));
    EXPECT_TRUE(createVector().empty());
}

TEST(StringResourceBase, formatReplacesPlaceholders)
{
    EXPECT_EQ("Save a.usda in layers", format("Save ^1s in ^2s", std::string("a.usda"), "layers"));
}

TEST(StringResourceBase, formatHonoursPlaceholderOrder)
{
    // A translation may reorder the placeholders relative to the source text.
    EXPECT_EQ("second first", format("^2s ^1s", "first", "second"));
}

TEST(StringResourceBase, formatReplacesEveryOccurrence)
{
    EXPECT_EQ("Layer x (x)", format("Layer ^1s (^1s)", "x"));
}

TEST(StringResourceBase, formatWithoutArgumentsLeavesTextUntouched)
{
    EXPECT_EQ("plain text", format("plain text"));
    EXPECT_EQ("keeps ^1s", format("keeps ^1s"));
}

TEST(StringResourceBase, formatLeavesPlaceholderWithoutArgument)
{
    EXPECT_EQ("a ^2s", format("^1s ^2s", "a"));
}

TEST(StringResourceBase, formatAcceptsEmptyReplacement)
{
    EXPECT_EQ("Path not found: ", format("Path not found: ^1s", ""));
}

TEST(StringResourceBase, formatDoesNotRescanInsertedText)
{
    // The replacement contains its own placeholder: this must terminate and keep it verbatim.
    EXPECT_EQ("x^1s and x^1s", format("^1s and ^1s", "x^1s"));
    EXPECT_EQ("^1s", format("^1s", "^1s"));
}

TEST(StringResourceBase, formatDoesNotExpandPlaceholdersInsideAnArgument)
{
    // A layer named "a^2sb" must come out verbatim, not have its "^2s" replaced by the next
    // argument.
    EXPECT_EQ("Save a^2sb: msg", format("Save ^1s: ^2s", "a^2sb", "msg"));
    EXPECT_EQ("msg: a^2sb", format("^2s: ^1s", "a^2sb", "msg"));
}

TEST(StringResourceBase, formatLeavesMalformedPlaceholdersAlone)
{
    EXPECT_EQ("^0s ^s ^1 ^a ^", format("^0s ^s ^1 ^a ^", "x"));
    EXPECT_EQ("x ^10s x0", format("^1s ^10s ^1s0", "x"));
    EXPECT_EQ("100% ^", format("100% ^", "x"));
}

TEST(StringResourceBase, formatSupportsNineArguments)
{
    EXPECT_EQ(
        "123456789",
        format("^1s^2s^3s^4s^5s^6s^7s^8s^9s", "1", "2", "3", "4", "5", "6", "7", "8", "9"));
}

TEST(StringResourceBase, makeResourceMapIsKeyedByResourceKey)
{
    const Resource one { "ctx", "kOne", "One" };
    const Resource two { "ctx", "kTwo", "Two" };

    const ResourceMap map = makeResourceMap({ &one, &two });
    ASSERT_EQ(2u, map.size());
    EXPECT_EQ("One", map.at("kOne")._value);
    EXPECT_EQ("ctx", map.at("kTwo")._module);
    EXPECT_EQ("kTwo", map.at("kTwo")._key);
}

TEST(StringResourceBase, makeResourceMapKeepsFirstOfDuplicateKeys)
{
    const Resource first { "ctx", "kSame", "first" };
    const Resource second { "ctx", "kSame", "second" };

    const ResourceMap map = makeResourceMap({ &first, &second });
    ASSERT_EQ(1u, map.size());
    EXPECT_EQ("first", map.at("kSame")._value);
}

TEST(StringResourceBase, makeResourceMapOfNothingIsEmpty)
{
    EXPECT_TRUE(makeResourceMap({}).empty());
}

TEST(StringResourceBase, registerAllSucceedsWhenEveryRegistrationSucceeds)
{
    const Resource one { "ctx", "kOne", "One" };
    const Resource two { "ctx", "kTwo", "Two" };
    const auto     map = makeResourceMap({ &one, &two });

    std::vector<std::string> seen;
    const bool               ok = registerAll(map, [&seen](const Resource& resource) {
        seen.push_back(resource._key);
        return true;
    });

    EXPECT_TRUE(ok);
    // The map is ordered by key.
    EXPECT_EQ((std::vector<std::string> { "kOne", "kTwo" }), seen);
}

TEST(StringResourceBase, registerAllKeepsGoingAfterAFailure)
{
    const Resource one { "ctx", "kOne", "One" };
    const Resource two { "ctx", "kTwo", "Two" };
    const Resource three { "ctx", "kThree", "Three" };
    const auto     map = makeResourceMap({ &one, &two, &three });

    size_t     attempts = 0;
    const bool ok = registerAll(map, [&attempts](const Resource& resource) {
        ++attempts;
        return resource._key != "kOne";
    });

    EXPECT_FALSE(ok);
    EXPECT_EQ(3u, attempts);
}

TEST(StringResourceBase, registerAllOfEmptyMapSucceeds)
{
    EXPECT_TRUE(registerAll(ResourceMap(), [](const Resource&) { return false; }));
}
