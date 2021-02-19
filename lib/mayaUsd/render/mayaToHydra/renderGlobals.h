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

#include "tokens.h"
#include "utils.h"

#include <hdMaya/delegates/params.h>

#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>

#include <maya/MObject.h>

#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class MtohRenderGlobals
{
public:
    MtohRenderGlobals();
    ~MtohRenderGlobals() = default;

    struct GlobalParams
    {
        const TfToken filter = {};
        // Is the filter above only a renderer, or a renderer.attribute
        const bool filterIsRenderer = false;
        // If creating the attribute for the first time, immediately set to a user default
        const bool fallbackToUserDefaults = true;
        // TODO: Extend this and mtoh with a setting to ignore scene settings

        GlobalParams() = default;
        GlobalParams(const TfToken f, bool fir, bool ftud)
            : filter(f)
            , filterIsRenderer(fir)
            , fallbackToUserDefaults(ftud)
        {
        }
    };

    // Creating render globals attributes on "defaultRenderGlobals"
    static MObject CreateAttributes(const GlobalParams&);

    // Returning the settings stored on "defaultRenderGlobals"
    static const MtohRenderGlobals& GetInstance(bool storeUserSettings = false);

    // Inform mtoh one of the settings stored on "defaultRenderGlobals" has changed
    static const MtohRenderGlobals&
    GlobalChanged(const GlobalParams&, bool storeUserSetting = false);

    // Check that the attribute given affects the renderer given.
    static bool AffectsRenderer(const TfToken& mangledAttr, const TfToken& rendererName);

    // Add common UI code for options-menu boxes
    static void OptionsPreamble();

    // Build a UI options menu for the renderer and it's settings list
    static void BuildOptionsMenu(
        const MtohRendererDescription&       rendererDesc,
        const HdRenderSettingDescriptorList& rendererSettingDescriptors);

    // Apply the given setting (or all of a delegate's settings when attrNames is empty) to the
    // given renderDelegate
    bool ApplySettings(
        HdRenderDelegate*    delegate,
        const TfToken&       rendererName,
        const TfTokenVector& attrNames = {}) const;

private:
    static const MtohRenderGlobals& GetInstance(const GlobalParams&, bool storeUserSetting);

    class MtohSettingFilter;
    using RendererSettings = std::unordered_map<TfToken, VtValue, TfToken::HashFunctor>;
    std::unordered_map<TfToken, RendererSettings, TfToken::HashFunctor> _rendererSettings;

public:
    HdMayaParams delegateParams;
    GfVec4f      colorSelectionHighlightColor = GfVec4f(1.0f, 1.0f, 0.0f, 0.5f);
    bool         colorSelectionHighlight = true;
    bool         wireframeSelectionHighlight = true;
#if PXR_VERSION >= 2005
    float outlineSelectionWidth = 4.f;
#endif
#if PXR_VERSION <= 2005
    float enableColorQuantization = false;
#endif
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MTOH_RENDER_GLOBALS_H
