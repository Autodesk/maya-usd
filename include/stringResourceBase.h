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
#ifndef STRING_RESOURCE_BASE_H
#define STRING_RESOURCE_BASE_H

// Types shared by the string-resource headers of UsdUfe, UsdLayerEditor, MayaUsd and MayaUsdUI.
// Header-only and free of Maya, Qt and USD dependencies, so any of those libraries can use it.
//
// Each library keeps its own X-macro list of strings (X(key, "source text")), declares each one
// `extern const Resource key;` in its header and defines it once in its .cpp. Defining it once
// matters: a `const` at namespace scope in a header has internal linkage, so each translation unit
// would otherwise build its own copy.

#include <initializer_list>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace StringResourceBase {

// \brief Create a vector of strings from a variadic list of arguments.
// \param Ts The types of the arguments, which must be convertible to std::string.
template <typename... Ts> std::vector<std::string> createVector(Ts&&... ts)
{
    return { std::string(std::forward<Ts>(ts))... };
}

// \brief Format a string with arguments.
// \param format The string to format, with placeholders like ^1s, ^2s, etc.
// \param args The arguments to replace the placeholders with.
// \return The formatted string.
// \note Only supports up to 9 arguments of type std::string, const char*, or any type that can be
// converted to std::string.
// \note A placeholder may appear multiple times; every occurrence is replaced.
// \note The format string is scanned once, so text inserted for an argument is never scanned for
// placeholders: an argument that contains "^2s" is kept as is.
// \note A placeholder without a matching argument is left in the output.
template <typename... Args> std::string format(const std::string& format, Args... args)
{
    static_assert(sizeof...(Args) < 10, "This function accepts up to 9 extra arguments!");

    const std::vector<std::string> argVector = createVector(args...);

    std::string formattedString;
    formattedString.reserve(format.size());
    for (size_t i = 0; i < format.size(); ++i) {
        // A placeholder is '^', a digit from 1 to 9, and 's'.
        if (format[i] == '^' && i + 2 < format.size() && format[i + 2] == 's') {
            const size_t argIndex = static_cast<size_t>(format[i + 1] - '1');
            if (format[i + 1] >= '1' && format[i + 1] <= '9' && argIndex < argVector.size()) {
                formattedString += argVector[argIndex];
                i += 2;
                continue;
            }
        }
        formattedString += format[i];
    }
    return formattedString;
}

// A translatable string:
// - _module [optional]: the Qt translation context
// - _key: identifies the string to the DCC
// - _value: is the source text, also used when no translation is found
struct Resource
{
    std::string _module;
    std::string _key;
    std::string _value;
};

using ResourceMap = std::map<std::string, Resource>;

// Build a map keyed by Resource::_key from pointers to resources.
inline ResourceMap makeResourceMap(std::initializer_list<const Resource*> resources)
{
    ResourceMap map;
    for (const Resource* resource : resources)
        map.emplace(resource->_key, *resource);
    return map;
}

// Build a ResourceMap from an X-macro list, e.g. STRING_RESOURCE_BASE_MAKE_MAP(MY_STRING_LIST).
// The list is the name of a macro taking X(key, value), and each key must be a Resource in scope.
#define STRING_RESOURCE_BASE_ADDRESS_OF(key, value) &key,
#define STRING_RESOURCE_BASE_MAKE_MAP(LIST) \
    StringResourceBase::makeResourceMap({ LIST(STRING_RESOURCE_BASE_ADDRESS_OF) })

// Call registerOne(const Resource&) -> bool on every resource, even after a failure.
// Returns false if any call failed.
template <class RegisterFn> bool registerAll(const ResourceMap& resources, RegisterFn registerOne)
{
    bool ok { true };
    for (const auto& entry : resources) {
        if (!registerOne(entry.second))
            ok = false;
    }
    return ok;
}

} // namespace StringResourceBase

#endif // STRING_RESOURCE_BASE_H
