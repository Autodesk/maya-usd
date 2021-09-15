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

#include <mayaUsd/base/tokens.h>

#include <pxr/base/tf/pyStaticTokens.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_USING_DIRECTIVE

void wrapTokens()
{
    TF_PY_WRAP_PUBLIC_TOKENS("OptionVarTokens", MayaUsdOptionVars, MAYA_USD_OPTIONVAR_TOKENS);
    TF_PY_WRAP_PUBLIC_TOKENS("MetadataTokens", MayaUsdMetadata, MAYA_USD_METADATA_TOKENS);
    TF_PY_WRAP_PUBLIC_TOKENS("Tokens", MayaUsdTokens, MAYA_USD_GENERIC_TOKENS);
}
