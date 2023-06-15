//
// Copyright 2023 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Utils.h"

#include <pxr/base/tf/stringUtils.h>

#include <cctype>
#include <regex>
#include <string>
#include <unordered_map>

namespace USDUFE_NS_DEF {

std::string sanitizeName(const std::string& name)
{
    return PXR_NS::TfStringReplace(name, ":", "_");
}

std::string prettifyName(const std::string& name)
{
    std::string prettyName(1, std::toupper(name[0]));
    size_t      nbChars = name.size();
    bool        capitalizeNext = false;
    for (size_t i = 1; i < nbChars; ++i) {
        unsigned char nextLetter = name[i];
        if (capitalizeNext) {
            nextLetter = std::toupper(nextLetter);
            capitalizeNext = false;
        }
        if (std::isupper(name[i]) && !std::isdigit(name[i - 1])) {
            if (((i < (nbChars - 1)) && !std::isupper(name[i + 1])) || std::islower(name[i - 1])) {
                prettyName += ' ';
            }
            prettyName += nextLetter;
        } else if (name[i] == '_' || name[i] == ':') {
            prettyName += " ";
            capitalizeNext = true;
        } else {
            prettyName += nextLetter;
        }
    }
    // Manual substitutions for custom capitalisations. Will be searched as case-insensitive.
    static const std::unordered_map<std::string, std::string> subs = {
        { "usd", "USD" },
        { "mtlx", "MaterialX" },
        { "gltf pbr", "glTF PBR" },
    };

    static const auto subRegexes = []() {
        std::vector<std::pair<std::regex, std::string>> regexes;
        regexes.reserve(subs.size());
        for (auto&& kv : subs) {
            regexes.emplace_back(
                std::regex { "\\b" + kv.first + "\\b", std::regex_constants::icase }, kv.second);
        }
        return regexes;
    }();

    for (auto&& re : subRegexes) {
        prettyName = regex_replace(prettyName, re.first, re.second);
    }

    return prettyName;
}

} // namespace USDUFE_NS_DEF
