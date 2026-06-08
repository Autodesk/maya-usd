//
// Copyright 2020 Autodesk
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

namespace UsdLayerEditor {
namespace String {

template <typename... Ts> std::vector<std::string> createVector(Ts&&... ts)
{
    return { std::string( std::forward<Ts>(ts))... };
}

template <typename... Args> std::string format(const std::string& format, Args... args)
{
    auto                     formattedString = format;
    std::vector<std::string> argVector = createVector(args...);

    for (size_t i = 0; i < argVector.size(); ++i) {
        auto   token = std::string("^") + std::to_string(i + 1) + std::string("s");
        size_t startPos = formattedString.find(token);
        if (startPos != std::wstring::npos) {
            formattedString.replace(startPos, token.length(), argVector[i]);
        }
    }
    return formattedString;
}

} // namespace String
} // namespace UsdLayerEditor