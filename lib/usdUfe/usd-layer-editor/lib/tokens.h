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

#ifndef USDLAYEREDITOR_TOKENS_H
#define USDLAYEREDITOR_TOKENS_H

#include "layerEditorAPI.h"

#include <pxr/base/tf/staticTokens.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

// Tokens that are used as options in the USD Layer Editor.
//
// clang-format off
#define USDLAYEREDITOR_OPTIONVAR_TOKENS                  \
    /* When saving as .usd, should the internal format be binary */ \
    ((SaveLayerFormatArgBinaryOption, "UsdLayerEditor_SaveLayerFormatArgBinaryOption")) \
    /* Option for what to do with Usd edits when the current */ \
    /* DCC scene is about to be saved.  optionVar values are: */ \
    /*    1: save all edits back to usd files. */ \
    /*    2: export the dirty usd layers to some DCC string attributes */ \
    /*       to be serialized to the DCC scene file. */ \
    /*    3: ignore all Usd edits. */ \
    ((SerializedUsdEditsLocation, "UsdLayerEditor_SerializedUsdEditsLocation")) \
    /* optionVar to force a prompt on every save */ \
    ((SerializedUsdEditsLocationPrompt, "UsdLayerEditor_SerializedUsdEditsLocationPrompt")) \
    /* optionVar to control if comfirmation dialog will be show when overriding file */ \
    ((ConfirmExistingFileSave, "UsdLayerEditor_ConfirmExistingFileSave")) \
    /* option var to remember if the stage in the layer editor is pinned. */ \
    ((PinLayerEditorStage, "UsdLayerEditor_PinLayerEditorStage")) \
    /* option to control if the session layer should be hidden. */ \
    ((AutoHideSessionLayer, "UsdLayerEditor_AutoHideSessionLayer"))
// clang-format on

TF_DECLARE_PUBLIC_TOKENS(UsdLayerEditorOptionVars, LayerEditorAPI, USDLAYEREDITOR_OPTIONVAR_TOKENS);

// Tokens that are used as metadata on layers
//
// clang-format off
#define USDLAYEREDITOR_METADATA_TOKENS \
    /* Referenced layers.                             */ \
    /* TODO LE-EXTRACT : mayaSharedLayers -> dccSharedLayers, do we need to support cross DCC metadata? */ \
    ((ReferencedLayers, "adskSharedLayers"))
// clang-format on

TF_DECLARE_PUBLIC_TOKENS(UsdLayerEditorMetadata, LayerEditorAPI, USDLAYEREDITOR_METADATA_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
