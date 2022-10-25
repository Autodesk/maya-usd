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
#ifndef MAYA_USD_UTIL_DICTIONARY_H
#define MAYA_USD_UTIL_DICTIONARY_H

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/property.h>
#include <pxr/usd/usd/stage.h>

#include <vector>

namespace MAYAUSD_NS_DEF {

namespace DictUtils {

/// \brief Extracts a bool at \p key from \p userArgs, or false if it can't extract.
MAYAUSD_CORE_PUBLIC
bool extractBoolean(const PXR_NS::VtDictionary& userArgs, const PXR_NS::TfToken& key);

/// \brief Extracts a pointer at \p key from \p userArgs, or nullptr if it can't extract.
MAYAUSD_CORE_PUBLIC
PXR_NS::UsdStageRefPtr
extractUsdStageRefPtr(const PXR_NS::VtDictionary& userArgs, const PXR_NS::TfToken& key);

/// \brief Extracts a double at \p key from \p userArgs, or defaultValue if it can't extract.
MAYAUSD_CORE_PUBLIC
double extractDouble(
    const PXR_NS::VtDictionary& userArgs,
    const PXR_NS::TfToken&      key,
    double                      defaultValue);

/// \brief Extracts a string at \p key from \p userArgs, or "" if it can't extract.
MAYAUSD_CORE_PUBLIC
std::string extractString(const PXR_NS::VtDictionary& userArgs, const PXR_NS::TfToken& key);

/// \brief Extracts a token at \p key from \p userArgs.
/// If the token value is not either \p defaultToken or one of the
/// \p otherTokens, then returns \p defaultToken instead.
MAYAUSD_CORE_PUBLIC
PXR_NS::TfToken extractToken(
    const PXR_NS::VtDictionary&         userArgs,
    const PXR_NS::TfToken&              key,
    const PXR_NS::TfToken&              defaultToken,
    const std::vector<PXR_NS::TfToken>& otherTokens);

/// \brief Extracts an absolute path at \p key from \p userArgs, or the empty path if
/// it can't extract.
MAYAUSD_CORE_PUBLIC
PXR_NS::SdfPath
extractAbsolutePath(const PXR_NS::VtDictionary& userArgs, const PXR_NS::TfToken& key);

/// \brief Extracts an vector<T> from the vector<VtValue> at \p key in \p userArgs.
/// Returns an empty vector if it can't convert the entire value at \p key into
/// a vector<T>.
template <typename T>
MAYAUSD_CORE_PUBLIC std::vector<T>
                    extractVector(const PXR_NS::VtDictionary& userArgs, const PXR_NS::TfToken& key);

/// \brief Convenience function that takes the result of extractVector and converts it to a
/// TfToken::Set.
MAYAUSD_CORE_PUBLIC
PXR_NS::TfToken::Set
extractTokenSet(const PXR_NS::VtDictionary& userArgs, const PXR_NS::TfToken& key);

// Implementation of the templated function declared above.
template <typename T>
MAYAUSD_CORE_PUBLIC std::vector<T>
                    extractVector(const PXR_NS::VtDictionary& userArgs, const PXR_NS::TfToken& key)
{
    // Using declaration is necessary for the TF_ macros to compile as they assume
    // to be in that namespace.
    PXR_NAMESPACE_USING_DIRECTIVE

    // Check that vector exists.
    if (VtDictionaryIsHolding<std::vector<T>>(userArgs, key)) {
        std::vector<T> vals = VtDictionaryGet<std::vector<T>>(userArgs, key);
        return vals;
    }

    if (!VtDictionaryIsHolding<std::vector<VtValue>>(userArgs, key)) {
        TF_CODING_ERROR(
            "Dictionary is missing required key '%s' or key is "
            "not vector type",
            key.GetText());
        return std::vector<T>();
    }

    // Check that vector is correctly-typed.
    std::vector<VtValue> vals = VtDictionaryGet<std::vector<VtValue>>(userArgs, key);
    if (!std::all_of(vals.begin(), vals.end(), [](const VtValue& v) { return v.IsHolding<T>(); })) {
        TF_CODING_ERROR(
            "Vector at dictionary key '%s' contains elements of "
            "the wrong type",
            key.GetText());
        return std::vector<T>();
    }

    // Extract values.
    std::vector<T> result;
    for (const VtValue& v : vals) {
        result.push_back(v.UncheckedGet<T>());
    }
    return result;
}

} // namespace DictUtils

} // namespace MAYAUSD_NS_DEF

#endif // MAYA_USD_UTIL_DICTIONARY_H
