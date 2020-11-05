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

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

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

std::string                          MtohGetRendererPluginDisplayName(const TfToken& id);
const MtohRendererDescriptionVector& MtohGetRendererDescriptions();
const MtohRendererSettings&          MtohGetRendererSettings();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MTOH_UTILS_H
