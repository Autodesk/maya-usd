//
// Copyright 2026 Autodesk
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
#ifndef LAYER_EDITOR_DCC_FUNCTIONS_H
#define LAYER_EDITOR_DCC_FUNCTIONS_H

#include "layerEditorAPI.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

#include <functional>
#include <string>
#include <vector>

namespace UsdLayerEditor {

// std::function typedefs use the EXACT signatures of the former base-class overrides.
using SaveComponentFn    = std::function<void(const PXR_NS::UsdStageRefPtr&, const std::string&)>;
using ReloadComponentFn  = std::function<void(const std::string&)>;
using RenameProxyShapeFn = std::function<void(const std::string&, const std::string&)>;
using IsStageAComponentFn  = std::function<bool(const std::string&)>;
using IsUnsavedComponentFn = std::function<bool(const PXR_NS::UsdStageRefPtr&)>;
using ShouldDisplayComponentInitialSaveDialogFn
    = std::function<bool(const PXR_NS::UsdStageRefPtr&, const std::string&)>;
using SceneFolderFn = std::function<std::string()>;
using MoveComponentFn
    = std::function<std::string(const std::string&, const std::string&, const std::string&)>;
using PreviewComponentSaveFn
    = std::function<std::string(const std::string&, const std::string&, const std::string&)>;
using GetComponentLayersToSaveFn = std::function<std::vector<std::string>(const std::string&)>;

using SupportsEditForwardingFn = std::function<bool()>;
using EchoEditForwardingFn     = std::function<bool()>;
using SetEchoEditForwardingFn  = std::function<void(bool)>;

using IsDccObjectStageIncomingFn = std::function<bool(const std::string&)>;
using IsDccObjectSharedStageFn   = std::function<bool(const std::string&)>;

struct ComponentFns
{
    SaveComponentFn                           saveComponent;
    ReloadComponentFn                         reloadComponent;
    RenameProxyShapeFn                        renameProxyShape;
    IsStageAComponentFn                       isStageAComponent;
    IsUnsavedComponentFn                      isUnsavedComponent;
    ShouldDisplayComponentInitialSaveDialogFn shouldDisplayComponentInitialSaveDialog;
    SceneFolderFn                             sceneFolder;
    MoveComponentFn                           moveComponent;
    PreviewComponentSaveFn                    previewComponentSave;
    GetComponentLayersToSaveFn                getComponentLayersToSave;
};

struct EditForwardingFns
{
    SupportsEditForwardingFn supportsEditForwarding;
    EchoEditForwardingFn     echoEditForwarding;
    SetEchoEditForwardingFn  setEchoEditForwarding;
};

struct DccObjectFns
{
    IsDccObjectStageIncomingFn isDccObjectStageIncoming;
    IsDccObjectSharedStageFn   isDccObjectSharedStage;
};

struct LayerEditorDCCFunctions
{
    ComponentFns      component;
    EditForwardingFns editForwarding;
    DccObjectFns      dccObject;
};

// Registration API — per-group setters (play cleanly with #ifdef guards), plus a
// full-struct setter and a getter used by the test RAII helper.
LayerEditorAPI void setComponentFns(const ComponentFns&);
LayerEditorAPI void setEditForwardingFns(const EditForwardingFns&);
LayerEditorAPI void setDccObjectFns(const DccObjectFns&);
LayerEditorAPI void setLayerEditorDCCFunctions(const LayerEditorDCCFunctions&);
LayerEditorAPI const LayerEditorDCCFunctions& layerEditorDCCFunctions();

// Accessor free functions — callers never null-check; an unset std::function
// yields the documented default (false / empty / no-op, except
// isDccObjectSharedStage which defaults to true).
LayerEditorAPI void        saveComponent(const PXR_NS::UsdStageRefPtr&, const std::string&);
LayerEditorAPI void        reloadComponent(const std::string&);
LayerEditorAPI void        renameProxyShape(const std::string&, const std::string&);
LayerEditorAPI bool        isStageAComponent(const std::string&);
LayerEditorAPI bool        isUnsavedComponent(const PXR_NS::UsdStageRefPtr&);
LayerEditorAPI bool        shouldDisplayComponentInitialSaveDialog(
           const PXR_NS::UsdStageRefPtr&,
           const std::string&);
LayerEditorAPI std::string sceneFolder();
LayerEditorAPI std::string
moveComponent(const std::string&, const std::string&, const std::string&);
LayerEditorAPI std::string
previewComponentSave(const std::string&, const std::string&, const std::string&);
LayerEditorAPI std::vector<std::string> getComponentLayersToSave(const std::string&);

LayerEditorAPI bool supportsEditForwarding();
LayerEditorAPI bool echoEditForwarding();
LayerEditorAPI void setEchoEditForwarding(bool);

LayerEditorAPI bool isDccObjectStageIncoming(const std::string&);
LayerEditorAPI bool isDccObjectSharedStage(const std::string&);

} // namespace UsdLayerEditor

#endif // LAYER_EDITOR_DCC_FUNCTIONS_H
