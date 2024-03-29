//
// Copyright 2016 Pixar
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
#include "tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

MayaUsd_SchemasTokensType::MayaUsd_SchemasTokensType()
    : mayaAutoEdit("mayaAutoEdit", TfToken::Immortal)
    , mayaNamespace("mayaNamespace", TfToken::Immortal)
    , mayaReference("mayaReference", TfToken::Immortal)
    , ALMayaReference("ALMayaReference", TfToken::Immortal)
    , MayaReference("MayaReference", TfToken::Immortal)
    , allTokens({ mayaAutoEdit, mayaNamespace, mayaReference, ALMayaReference, MayaReference })
{
}

TfStaticData<MayaUsd_SchemasTokensType> MayaUsd_SchemasTokens;

PXR_NAMESPACE_CLOSE_SCOPE
