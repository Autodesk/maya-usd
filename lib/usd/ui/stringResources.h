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
#ifndef MAYAUSD_UI_STRING_RESOURCES_H
#define MAYAUSD_UI_STRING_RESOURCES_H

#include <mayaUsd/base/stringResources.h>
#include <mayaUsdUI/ui/api.h>
#include <stringResourceBase.h>

#include <QtCore/QString>

namespace MayaUsdUI {
namespace StringResources {

using StringResourceBase::Resource;
using StringResourceBase::ResourceMap;

// Register all MayaUsdUI strings.
MAYAUSD_UI_PUBLIC bool registerStrings();

// Retrieve a string resource, translated by Maya. Falls back to the source text if Maya has no
// entry for the key (for example if registration failed or has not happened yet).
MAYAUSD_UI_PUBLIC QString getAsQString(const Resource& strResID);

// Shorthands taking the bare resource name, e.g. GET_MAYAUSDUI_QSTR(kCancel).
#define GET_MAYAUSDUI_QSTR(key) \
    MayaUsdUI::StringResources::getAsQString(MayaUsdUI::StringResources::key)
#define GET_MAYAUSDUI_MSTR(key) \
    MayaUsd::StringResources::getAsMString(MayaUsdUI::StringResources::key)
#define GET_MAYAUSDUI_STR(key) \
    MayaUsd::StringResources::getAsString(MayaUsdUI::StringResources::key)

// -------------------------------------------------------------
//
// clang-format off

// The list of all resources. Add new strings here only: the variable name matches its key.
// Each is declared extern below and defined once in stringResources.cpp, so a Resource is built
// once per string rather than once per translation unit including this header (C++14: no inline
// variables).
#define MAYAUSDUI_STRING_LIST(X) \
    /* layerEditor: */ \
    X(kConfigureEditForwardingTitle, "USD Edit Forwarding Configuration") \
    X(kUsdLayerIdentifier, "USD Layer identifier: ^1s") \
    X(kRealPath, "Real Path: ^1s") \
    /* importDialog: */ \
    X(kImportName, "Name") \
    X(kImportType, "Type") \
    X(kImportVariantSetAndVariant, "Variant Set and Variant") \
    /* importDialog: from USDImportDialog.ui */ \
    X(kImportDialogTitle, "Scope and Variants: Hierarchy View") \
    X(kImportUsdFilePath, "USD File Path:") \
    X(kImportSelectPrims, "Select the prim(s) you'd like to import. Switch variants accordingly.") \
    X(kImportPrimsInSelectedScope, "Prims in selected scope:") \
    X(kImportVariantsSwitchedInSelectedScope, "Variants switched in selected scope:") \
    X(kImportOk, "OK") \
    X(kImportCancel, "Cancel") \
    X(kImportMenuEdit, "Edit") \
    X(kImportMenuHelp, "Help") \
    X(kImportResetFile, "Reset File") \
    X(kImportHelpOnHierarchyView, "Help on Hierarchy View") \
    /* renderSetup: */ \
    X(kRenderSetupWindowTitle, "USD Render Setup") \
    X(kRenderSetupOptions, "Options") \
    X(kRenderSetupDisplayUsdHierarchy, "Display USD Hierarchy") \
    X(kRenderSetupMayaSettings, "Maya Settings") \
    /* assetResolver: */ \
    X(kARSelectFolder, "Select Folder")

// Only valid inside namespace StringResources; undef to avoid polluting the global namespace.
#define DECLARE_MAYAUSDUI_STR(key, value) MAYAUSD_UI_PUBLIC extern const Resource key;
MAYAUSDUI_STRING_LIST(DECLARE_MAYAUSDUI_STR)
#undef DECLARE_MAYAUSDUI_STR

// clang-format on
//
// -------------------------------------------------------------

} // namespace StringResources
} // namespace MayaUsdUI

#endif // MAYAUSD_UI_STRING_RESOURCES_H