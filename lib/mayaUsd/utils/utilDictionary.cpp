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
#include "utilDictionary.h"

namespace MAYAUSD_NS_DEF {

namespace DictUtils {

PXR_NAMESPACE_USING_DIRECTIVE

/// Extracts a bool at \p key from \p userArgs, or false if it can't extract.
bool extractBoolean(const VtDictionary& userArgs, const TfToken& key)
{
    if (!VtDictionaryIsHolding<bool>(userArgs, key)) {
        TF_CODING_ERROR(
            "Dictionary is missing required key '%s' or key is "
            "not bool type",
            key.GetText());
        return false;
    }
    return VtDictionaryGet<bool>(userArgs, key);
}

/// Extracts a pointer at \p key from \p userArgs, or nullptr if it can't extract.
UsdStageRefPtr extractUsdStageRefPtr(const VtDictionary& userArgs, const TfToken& key)
{
    if (!VtDictionaryIsHolding<UsdStageRefPtr>(userArgs, key)) {
        TF_CODING_ERROR(
            "Dictionary is missing required key '%s' or key is "
            "not pointer type",
            key.GetText());
        return nullptr;
    }
    return VtDictionaryGet<UsdStageRefPtr>(userArgs, key);
}

/// Extracts a double at \p key from \p userArgs, or defaultValue if it can't extract.
double extractDouble(const VtDictionary& userArgs, const TfToken& key, double defaultValue)
{
    if (VtDictionaryIsHolding<double>(userArgs, key))
        return VtDictionaryGet<double>(userArgs, key);

    // Since user dictionary can be provided from Python and in Python it is easy to
    // mix int and double, especially since value literal will take the simplest value
    // they can, for example 0 will be an int, support receiving the value as an integer.
    if (VtDictionaryIsHolding<int>(userArgs, key))
        return VtDictionaryGet<int>(userArgs, key);

    TF_CODING_ERROR(
        "Dictionary is missing required key '%s' or key is "
        "not double type",
        key.GetText());
    return defaultValue;
}

/// Extracts a string at \p key from \p userArgs, or "" if it can't extract.
std::string extractString(const VtDictionary& userArgs, const TfToken& key)
{
    if (!VtDictionaryIsHolding<std::string>(userArgs, key)) {
        TF_CODING_ERROR(
            "Dictionary is missing required key '%s' or key is "
            "not string type",
            key.GetText());
        return std::string();
    }
    return VtDictionaryGet<std::string>(userArgs, key);
}

/// Extracts a token at \p key from \p userArgs.
/// If the token value is not either \p defaultToken or one of the
/// \p otherTokens, then returns \p defaultToken instead.
TfToken extractToken(
    const VtDictionary&         userArgs,
    const TfToken&              key,
    const TfToken&              defaultToken,
    const std::vector<TfToken>& otherTokens)
{
    const TfToken tok(extractString(userArgs, key));
    for (const TfToken& allowedTok : otherTokens) {
        if (tok == allowedTok) {
            return tok;
        }
    }

    // Empty token will silently be promoted to default value.
    // Warning for non-empty tokens that don't match.
    if (tok != defaultToken && !tok.IsEmpty()) {
        TF_WARN(
            "Value '%s' is not allowed for flag '%s'; using fallback '%s' "
            "instead",
            tok.GetText(),
            key.GetText(),
            defaultToken.GetText());
    }
    return defaultToken;
}

/// Extracts an absolute path at \p key from \p userArgs, or the empty path if
/// it can't extract.
SdfPath extractAbsolutePath(const VtDictionary& userArgs, const TfToken& key)
{
    const std::string s = extractString(userArgs, key);
    // Assume that empty strings are empty paths. (This might be an error case.)
    if (s.empty()) {
        return SdfPath();
    }
    // Make all relative paths into absolute paths.
    SdfPath path(s);
    if (path.IsAbsolutePath()) {
        return path;
    } else {
        return SdfPath::AbsoluteRootPath().AppendPath(path);
    }
}

/// Convenience function that takes the result of extractVector and converts it to a
/// TfToken::Set.
TfToken::Set extractTokenSet(const VtDictionary& userArgs, const TfToken& key)
{
    const std::vector<std::string> vec = extractVector<std::string>(userArgs, key);
    TfToken::Set                   result;
    for (const std::string& s : vec) {
        result.insert(TfToken(s));
    }
    return result;
}

} // namespace DictUtils

} // namespace MAYAUSD_NS_DEF
