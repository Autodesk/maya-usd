//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef __MTOH_RENDER_GLOBALS_H__
#define __MTOH_RENDER_GLOBALS_H__

#include <pxr/pxr.h>

#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>

#include <maya/MObject.h>

#include "../../usd/hdmaya/delegates/params.h"

#include <tuple>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

struct MtohRenderGlobals {
    MtohRenderGlobals();
    ~MtohRenderGlobals() = default;
    HdMayaParams delegateParams;
    GfVec4f colorSelectionHighlightColor = GfVec4f(1.0f, 1.0f, 0.0f, 0.5f);
    TfToken selectionOverlay;
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

// Reading renderer delegate attributes and generating UI code.
void MtohInitializeRenderGlobals();
// Creating render globals attributes on "defaultRenderGlobals"
MObject MtohCreateRenderGlobals();
// Returning the settings stored on "defaultRenderGlobals"
MtohRenderGlobals MtohGetRenderGlobals();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __MTOH_RENDER_GLOBALS_H__
