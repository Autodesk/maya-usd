//
// Copyright 2022 Autodesk
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

#include "UsdShaderAttributeDef.h"

#include <usdUfe/base/tokens.h>
#include <usdUfe/ufe/Global.h>
#include <usdUfe/utils/Utils.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/ndr/declare.h>
#include <pxr/usd/sdr/shaderProperty.h>

#include <map>
#include <vector>

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::AttributeDef, UsdShaderAttributeDef);

PXR_NAMESPACE_USING_DIRECTIVE

UsdShaderAttributeDef::UsdShaderAttributeDef(const SdrShaderPropertyConstPtr& shaderAttributeDef)
    : Ufe::AttributeDef()
    , _shaderAttributeDef(shaderAttributeDef)
{
    if (!TF_VERIFY(_shaderAttributeDef)) {
        throw std::runtime_error("Invalid shader attribute definition");
    }
}

std::string UsdShaderAttributeDef::name() const
{
    TF_DEV_AXIOM(_shaderAttributeDef);
    return _shaderAttributeDef->GetName().GetString();
}

std::string UsdShaderAttributeDef::type() const
{
    TF_DEV_AXIOM(_shaderAttributeDef);
    return usdTypeToUfe(_shaderAttributeDef);
}

std::string UsdShaderAttributeDef::defaultValue() const
{
    TF_DEV_AXIOM(_shaderAttributeDef);
    std::ostringstream defaultValue;
    defaultValue << _shaderAttributeDef->GetDefaultValue();
    return defaultValue.str();
}

Ufe::AttributeDef::IOType UsdShaderAttributeDef::ioType() const
{
    TF_DEV_AXIOM(_shaderAttributeDef);
    return _shaderAttributeDef->IsOutput() ? Ufe::AttributeDef::OUTPUT_ATTR
                                           : Ufe::AttributeDef::INPUT_ATTR;
}

namespace {
typedef std::unordered_map<std::string, std::function<Ufe::Value(const PXR_NS::SdrShaderProperty&)>>
                         MetadataMap;
static const MetadataMap _metaMap = {
    // Conversion map between known USD metadata and its MaterialX equivalent:
    { UsdUfe::MetadataTokens->UIName.GetString(),
      [](const PXR_NS::SdrShaderProperty& p) {
          return !p.GetLabel().IsEmpty() ? p.GetLabel().GetString()
                                         : UsdUfe::prettifyName(p.GetName().GetString());
      } },
    { UsdUfe::MetadataTokens->UIDoc,
      [](const PXR_NS::SdrShaderProperty& p) {
          return !p.GetHelp().empty() ? p.GetHelp() : Ufe::Value();
      } },
    { UsdUfe::MetadataTokens->UIFolder.GetString(),
      [](const PXR_NS::SdrShaderProperty& p) {
          return !p.GetPage().IsEmpty() ? p.GetPage().GetString() : Ufe::Value();
      } },
    { UsdUfe::MetadataTokens->UIEnumLabels,
      [](const PXR_NS::SdrShaderProperty& p) {
          std::string r;
          for (auto&& opt : p.GetOptions()) {
              if (!r.empty()) {
                  r += ", ";
              }
              r += opt.first.GetString();
          }
          return !r.empty() ? r : Ufe::Value();
      } },
    { UsdUfe::MetadataTokens->UIEnumValues,
      [](const PXR_NS::SdrShaderProperty& p) {
          std::string r;
          for (auto&& opt : p.GetOptions()) {
              if (opt.second.IsEmpty()) {
                  continue;
              }
              if (!r.empty()) {
                  r += ", ";
              }
              r += opt.second.GetString();
          }
          return !r.empty() ? r : Ufe::Value();
      } },
    { UsdUfe::MetadataTokens->UISoftMin
          .GetString(), // Maya has 0-100 sliders. In rendering, sliders are 0-1.
      [](const PXR_NS::SdrShaderProperty& p) {
          // Will only be returned if the metadata does not exist.
          static const auto defaultSoftMin = std::unordered_map<std::string, Ufe::Value> {
              { Ufe::Attribute::kFloat, std::string { "0" } },
              { Ufe::Attribute::kFloat3, std::string { "0,0,0" } },
              { Ufe::Attribute::kColorFloat3, std::string { "0,0,0" } },
              { Ufe::Attribute::kDouble, std::string { "0" } },
#ifdef UFE_V4_FEATURES_AVAILABLE
              { Ufe::Attribute::kFloat2, std::string { "0,0" } },
              { Ufe::Attribute::kFloat4, std::string { "0,0,0,0" } },
              { Ufe::Attribute::kColorFloat4, std::string { "0,0,0,0" } },
#endif
          };
          // If there is a UIMin value, use it as the soft min.
          const NdrTokenMap& metadata = p.GetMetadata();
          auto               it = metadata.find(UsdUfe::MetadataTokens->UIMin);
          if (it != metadata.cend()) {
              return Ufe::Value(it->second);
          }
          auto itDefault = defaultSoftMin.find(usdTypeToUfe(&p));
          return itDefault != defaultSoftMin.end() ? itDefault->second : Ufe::Value();
      } },
    { UsdUfe::MetadataTokens->UISoftMax
          .GetString(), // Maya has 0-100 sliders. In rendering, sliders are 0-1.
      [](const PXR_NS::SdrShaderProperty& p) {
          // Will only be returned if the metadata does not exist.
          static const auto defaultSoftMax = std::unordered_map<std::string, Ufe::Value> {
              { Ufe::Attribute::kFloat, std::string { "1" } },
              { Ufe::Attribute::kFloat3, std::string { "1,1,1" } },
              { Ufe::Attribute::kColorFloat3, std::string { "1,1,1" } },
              { Ufe::Attribute::kDouble, std::string { "1" } },
#ifdef UFE_V4_FEATURES_AVAILABLE
              { Ufe::Attribute::kFloat2, std::string { "1,1" } },
              { Ufe::Attribute::kFloat4, std::string { "1,1,1,1" } },
              { Ufe::Attribute::kColorFloat4, std::string { "1,1,1,1" } },
#endif
          };
          // If there is a UIMax value, use it as the soft max.
          const NdrTokenMap& metadata = p.GetMetadata();
          auto               it = metadata.find(UsdUfe::MetadataTokens->UIMax);
          if (it != metadata.cend()) {
              return Ufe::Value(it->second);
          }
          auto itDefault = defaultSoftMax.find(usdTypeToUfe(&p));
          return itDefault != defaultSoftMax.end() ? itDefault->second : Ufe::Value();
      } },
    // If Ufe decides to use another completely different convention, it can be added here:
};
} // namespace

Ufe::Value UsdShaderAttributeDef::getMetadata(const std::string& key) const
{
    TF_DEV_AXIOM(_shaderAttributeDef);

#ifdef UFE_HAS_NATIVE_TYPE_METADATA
    if (key == Ufe::AttributeDef::kNativeType) {
        // We return the Sdf type as that is more meaningful than the Sdr type.
#if PXR_VERSION <= 2408
        const auto sdfTypeTuple = _shaderAttributeDef->GetTypeAsSdfType();
        if (sdfTypeTuple.second.IsEmpty()) {
            return Ufe::Value(sdfTypeTuple.first.GetAsToken().GetString());
        } else {
            return Ufe::Value(sdfTypeTuple.second.GetString());
        }
#else
        const auto sdfTypeIndicator = _shaderAttributeDef->GetTypeAsSdfType();
        if (sdfTypeIndicator.HasSdfType()) {
            return Ufe::Value(sdfTypeIndicator.GetSdfType().GetAsToken().GetString());
        } else {
            return Ufe::Value(sdfTypeIndicator.GetNdrType().GetString());
        }
#endif // PXR_VERSION
    }
#endif // UFE_HAS_NATIVE_TYPE_METADATA

    const NdrTokenMap& metadata = _shaderAttributeDef->GetMetadata();
    auto               it = metadata.find(TfToken(key));
    if (it != metadata.cend()) {
        return Ufe::Value(it->second);
    }

    const NdrTokenMap& hints = _shaderAttributeDef->GetHints();
    it = hints.find(TfToken(key));
    if (it != hints.cend()) {
        return Ufe::Value(it->second);
    }

    MetadataMap::const_iterator itMapper = _metaMap.find(key);
    if (itMapper != _metaMap.end()) {
        return itMapper->second(*_shaderAttributeDef);
    }

    return {};
}

bool UsdShaderAttributeDef::hasMetadata(const std::string& key) const
{
    TF_DEV_AXIOM(_shaderAttributeDef);

#ifdef UFE_HAS_NATIVE_TYPE_METADATA
    if (key == Ufe::AttributeDef::kNativeType) {
        return true;
    }
#endif

    const NdrTokenMap& metadata = _shaderAttributeDef->GetMetadata();
    auto               it = metadata.find(TfToken(key));
    if (it != metadata.cend()) {
        return true;
    }

    const NdrTokenMap& hints = _shaderAttributeDef->GetHints();
    it = hints.find(TfToken(key));
    if (it != hints.cend()) {
        return true;
    }

    MetadataMap::const_iterator itMapper = _metaMap.find(key);
    if (itMapper != _metaMap.end() && !itMapper->second(*_shaderAttributeDef).empty()) {
        return true;
    }

    return false;
}

} // namespace USDUFE_NS_DEF
