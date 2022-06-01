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

#include "Global.h"
#include "Utils.h"

#include <pxr/base/tf/token.h>
#include <pxr/usd/ndr/declare.h>
#include <pxr/usd/sdr/shaderProperty.h>

#include <map>
#include <vector>

namespace MAYAUSD_NS_DEF {
namespace ufe {

PXR_NAMESPACE_USING_DIRECTIVE

UsdShaderAttributeDef::UsdShaderAttributeDef(const SdrShaderPropertyConstPtr& shaderAttributeDef)
    : Ufe::AttributeDef()
    , fShaderAttributeDef(shaderAttributeDef)
{
    if (!TF_VERIFY(fShaderAttributeDef)) {
        throw std::runtime_error("Invalid shader attribute definition");
    }
}

UsdShaderAttributeDef::~UsdShaderAttributeDef() { }

std::string UsdShaderAttributeDef::name() const
{
    TF_AXIOM(fShaderAttributeDef);
    return fShaderAttributeDef->GetName().GetString();
}

std::string UsdShaderAttributeDef::type() const
{
    TF_AXIOM(fShaderAttributeDef);
    return usdTypeToUfe(fShaderAttributeDef);
}

std::string UsdShaderAttributeDef::defaultValue() const
{
    TF_AXIOM(fShaderAttributeDef);
    std::ostringstream defaultValue;
    defaultValue << fShaderAttributeDef->GetDefaultValue();
    return defaultValue.str();
}

Ufe::AttributeDef::IOType UsdShaderAttributeDef::ioType() const
{
    TF_AXIOM(fShaderAttributeDef);
    return fShaderAttributeDef->IsOutput() ? Ufe::AttributeDef::OUTPUT_ATTR
                                           : Ufe::AttributeDef::INPUT_ATTR;
}

Ufe::Value UsdShaderAttributeDef::getMetadata(const std::string& key) const
{
    TF_AXIOM(fShaderAttributeDef);
    const NdrTokenMap& metadata = fShaderAttributeDef->GetMetadata();
    auto               it = metadata.find(TfToken(key));
    if (it != metadata.cend()) {
        return Ufe::Value(it->second);
    }
    // TODO: Adapt UI metadata information found in SdrShaderProperty to Ufe standards
    // TODO: Fix Mtlx parser in USD to populate UI metadata in SdrShaderProperty
    return {};
}

bool UsdShaderAttributeDef::hasMetadata(const std::string& key) const
{
    TF_AXIOM(fShaderAttributeDef);
    const NdrTokenMap& metadata = fShaderAttributeDef->GetMetadata();
    auto               it = metadata.find(TfToken(key));
    return (it != metadata.cend());
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
