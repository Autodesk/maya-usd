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
// mayaUsd_ProxyTargetsSessionLayerOnOpen: Always target a session layer on a mayaUsdProxy
// mayaUsd_SaveLayerFormatArgBinaryOption: When saving as .usd, should the format be binary
// mayaUsd_SerializedUsdEditsLocation: Option for what to do with Usd edits when the current
// Maya scene is about to be saved.  optionVar values are:
//    1: save all edits back to usd files.
//    2: export the dirty usd layers to Maya string attributes to be serialized to the Maya file.
//    3: ignore all Usd edits.
// mayaUsd_SerializedUsdEditsLocationPrompt: optionVar to force a prompt on every save
#define MAYA_USD_OPTIONVAR_TOKENS                                                     \
    (mayaUsd_ProxyTargetsSessionLayerOnOpen)(mayaUsd_SaveLayerFormatArgBinaryOption)( \
        mayaUsd_SerializedUsdEditsLocation)(mayaUsd_SerializedUsdEditsLocationPrompt)

TF_DECLARE_PUBLIC_TOKENS(MayaUsdOptionVars, MAYAUSD_CORE_PUBLIC, MAYA_USD_OPTIONVAR_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
