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
#pragma once

#include "layerEditorDCCFunctions.h"

namespace UsdLayerEditor {

// Installs registry state on construction and restores the previous state on
// destruction, so tests that exercise component / EF / DCC-object behavior do
// not leak global registry state between cases.
class ScopedLayerEditorDCCFunctions
{
public:
    ScopedLayerEditorDCCFunctions()
        : _saved(layerEditorDCCFunctions())
    {
    }
    ~ScopedLayerEditorDCCFunctions() { setLayerEditorDCCFunctions(_saved); }

    ScopedLayerEditorDCCFunctions(const ScopedLayerEditorDCCFunctions&) = delete;
    ScopedLayerEditorDCCFunctions& operator=(const ScopedLayerEditorDCCFunctions&) = delete;

private:
    LayerEditorDCCFunctions _saved;
};

} // namespace UsdLayerEditor
