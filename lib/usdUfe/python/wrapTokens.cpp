//
// Copyright 2023 Autodesk
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

#include <usdUfe/base/tokens.h>

#include <pxr/base/tf/pyStaticTokens.h>
#include <pxr/pxr.h>

void wrapTokens()
{
    PXR_NAMESPACE_USING_DIRECTIVE
    TF_PY_WRAP_PUBLIC_TOKENS(
        "EditRoutingTokens", UsdUfe::EditRoutingTokens, USDUFE_EDIT_ROUTING_TOKENS);
    TF_PY_WRAP_PUBLIC_TOKENS("Tokens", UsdUfe::GenericTokens, USDUFE_GENERIC_TOKENS);
    TF_PY_WRAP_PUBLIC_TOKENS("MetadataTokens", UsdUfe::MetadataTokens, USDUFE_METADATA_TOKENS);
}
