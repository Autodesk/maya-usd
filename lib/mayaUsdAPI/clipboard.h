//
// Copyright 2024 Autodesk
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
#ifndef MAYAUSDAPI_CLIPBOARD_H
#define MAYAUSDAPI_CLIPBOARD_H

#include <mayaUsdAPI/api.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/clipboardHandler.h>

#include <string>

namespace MAYAUSDAPI_NS_DEF {

//! Helper function to test if the clipboard contains an item of a given type.
//! The caller provides a test function which will be called for each top-level
//! prim in the clipboard. As an example:
//
//     bool isNodeGraph(const PXR_NS::UsdPrim& prim)
//     {
//         return bool(PXR_NS::UsdShadeNodeGraph(prim));
//     }
//     clipboardHandler->hasItemToPaste(&isNodeGraph);
//
//! \note If the input clipboard handler is not a UsdClipboardHandler, returns false.
typedef bool (*HasItemToPasteTestFn)(const PXR_NS::UsdPrim& prim);
MAYAUSD_API_PUBLIC
bool hasItemToPaste(const Ufe::ClipboardHandler::Ptr&, HasItemToPasteTestFn testFn);

//! Sets the absolute path (with filename) for saving clipboard data to.
//! \note If the input clipboard handler is not a UsdClipboardHandler, does nothing.
MAYAUSD_API_PUBLIC
void setClipboardFilePath(const Ufe::ClipboardHandler::Ptr&, const std::string& clipboardPath);

//! Sets the USD file format for the clipboard file.
//! \param[in] formatTag USD file format to save. Must be either "usda" or "usdc".
//! \note If the input clipboard handler is not a UsdClipboardHandler, does nothing.
MAYAUSD_API_PUBLIC
void setClipboardFileFormat(const Ufe::ClipboardHandler::Ptr&, const std::string& formatTag);

} // namespace MAYAUSDAPI_NS_DEF

#endif // MAYAUSDAPI_CLIPBOARD_H
