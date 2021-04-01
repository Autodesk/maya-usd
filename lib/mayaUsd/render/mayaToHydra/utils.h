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
#ifndef MTOH_UTILS_H
#define MTOH_UTILS_H

#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/pxr.h>

#include <maya/MFrameContext.h>
#include <maya/MString.h>

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

constexpr auto MTOH_RENDER_OVERRIDE_PREFIX = "mtohRenderOverride_";

struct MtohRendererDescription
{
    MtohRendererDescription(const TfToken& rn, const TfToken& on, const TfToken& dn)
        : rendererName(rn)
        , overrideName(on)
        , displayName(dn)
    {
    }

    TfToken rendererName;
    TfToken overrideName;
    TfToken displayName;
};

using MtohRendererDescriptionVector = std::vector<MtohRendererDescription>;

// Map from MtohRendererDescription::rendererName to it's a HdRenderSettingDescriptorList
using MtohRendererSettings
    = std::unordered_map<TfToken, HdRenderSettingDescriptorList, TfToken::HashFunctor>;

// Defining these in header so don't need to link to use
inline bool IsMtohRenderOverrideName(const MString& overrideName)
{
    // See if the override is an mtoh one - ie, it starts with the right prefix

    // VS2017 msvc didn't accept this as a constexpr
    const auto prefixLen = strlen(MTOH_RENDER_OVERRIDE_PREFIX);
    if (overrideName.length() < prefixLen) {
        return false;
    }

    return overrideName.substring(0, prefixLen - 1) == MTOH_RENDER_OVERRIDE_PREFIX;
}

inline bool IsMtohRenderOverride(const MHWRender::MFrameContext& frameContext)
{
    MHWRender::MFrameContext::RenderOverrideInformation overrideInfo;
    frameContext.getRenderOverrideInformation(overrideInfo);
    return IsMtohRenderOverrideName(overrideInfo.overrideName);
}

std::string                          MtohGetRendererPluginDisplayName(const TfToken& id);
const MtohRendererDescriptionVector& MtohGetRendererDescriptions();
const MtohRendererSettings&          MtohGetRendererSettings();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MTOH_UTILS_H
