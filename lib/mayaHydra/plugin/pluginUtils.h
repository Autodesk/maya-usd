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

#include <mayaHydraLib/mayaHydra.h>

#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/pxr.h>

#include <maya/MFrameContext.h>
#include <maya/MString.h>

#include <string>
#include <vector>

namespace MAYAHYDRA_NS_DEF {

constexpr auto MTOH_RENDER_OVERRIDE_PREFIX = "mayaHydraRenderOverride_";

struct MtohRendererDescription
{
    MtohRendererDescription(const pxr::TfToken& rn, const pxr::TfToken& on, const pxr::TfToken& dn)
        : rendererName(rn)
        , overrideName(on)
        , displayName(dn)
    {
    }

    pxr::TfToken rendererName;
    pxr::TfToken overrideName;
    pxr::TfToken displayName;
};

using MtohRendererDescriptionVector = std::vector<MtohRendererDescription>;

/// Map from MtohRendererDescription::rendererName to it's a HdRenderSettingDescriptorList
using MtohRendererSettings
    = std::unordered_map<pxr::TfToken, pxr::HdRenderSettingDescriptorList, pxr::TfToken::HashFunctor>;

std::string                          MtohGetRendererPluginDisplayName(const pxr::TfToken& id);
const MtohRendererDescriptionVector& MtohGetRendererDescriptions();
const MtohRendererSettings&          MtohGetRendererSettings();

} // namespace MAYAHYDRA_NS_DEF

#endif // MTOH_UTILS_H
