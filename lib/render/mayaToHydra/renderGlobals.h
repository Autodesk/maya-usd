//
// Copyright 2019 Luma Pictures
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
#ifndef MTOH_RENDER_GLOBALS_H
#define MTOH_RENDER_GLOBALS_H

#include <pxr/pxr.h>

#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>

#include <maya/MObject.h>

#include "tokens.h"
#include "utils.h"
#include "../../usd/hdMaya/delegates/params.h"

#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

struct MtohRenderGlobals {
    MtohRenderGlobals();
    ~MtohRenderGlobals() = default;
    HdMayaParams delegateParams;
    GfVec4f colorSelectionHighlightColor = GfVec4f(1.0f, 1.0f, 0.0f, 0.5f);
    TfToken selectionOverlay = MtohTokens->UseVp2;
    bool colorSelectionHighlight = true;
    bool wireframeSelectionHighlight = true;
    struct RenderParam {
        template <typename T>
        RenderParam(const TfToken& k, const T& v) : key(k), value(v) {}
        TfToken key;
        VtValue value;
    };
    std::unordered_map<TfToken, std::vector<RenderParam>, TfToken::HashFunctor>
        rendererSettings;
};

// Creating render globals attributes on "defaultRenderGlobals"
MObject MtohCreateRenderGlobals();
// Returning the settings stored on "defaultRenderGlobals"
MtohRenderGlobals MtohGetRenderGlobals();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MTOH_RENDER_GLOBALS_H
