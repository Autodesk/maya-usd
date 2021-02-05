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

#ifndef PXRUSDMAYA_TOKENS_H
#define PXRUSDMAYA_TOKENS_H

#include "api.h"

#include <pxr/base/tf/staticTokens.h>
#include <pxr/pxr.h>

// See lib/mayaUsd/utils/util.h for a TfToken -> MString conversion
//
PXR_NAMESPACE_OPEN_SCOPE

// Tokens that are used as optionVars in MayaUSD
//
// clang-format off
#define MAYA_USD_OPTIONVAR_TOKENS                        \
    /* Always target a session layer on a mayaUsdProxy*/ \
    (mayaUsd_ProxyTargetsSessionLayerOnOpen)             \
    /* The kind to be selected when viewport picking. */ \
    /* After resolving the picked prim, a search from */ \
    /* that prim up the USD namespace hierarchy will  */ \
    /* be performed looking for a prim that matches   */ \
    /* the kind in the optionVar. If no prim matches, */ \
    /* or if the selection kind is unspecified or     */ \
    /* empty, the exact prim picked in the viewport   */ \
    /* is selected.                                   */ \
    (mayaUsd_SelectionKind)
// clang-format on

TF_DECLARE_PUBLIC_TOKENS(MayaUsdOptionVars, MAYAUSD_CORE_PUBLIC, MAYA_USD_OPTIONVAR_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
