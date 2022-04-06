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
#include "shadingTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(TrUsdTokens, TR_USD_COMMON TR_USD_TEXTURE TR_USD_PRIMVAR TR_USD_XFORM);

TF_DEFINE_PUBLIC_TOKENS(
    TrMayaTokens,
    TR_MAYA_MATERIALS TR_MAYA_NODES TR_MAYA_STANDARD_SURFACE TR_MAYA_FILE TR_MAYA_UV
        TR_MAYA_PRIMVAR);

#ifdef WANT_MATERIALX_TRANSLATORS
TF_DEFINE_PUBLIC_TOKENS(
    TrMtlxTokens,
    TR_MTLX_COMMON TR_MTLX_NODE_DEFS TR_MTLX_STANDARD_SURFACE TR_MTLX_IMAGE TR_MTLX_ATTRIBUTES);
#endif

PXR_NAMESPACE_CLOSE_SCOPE