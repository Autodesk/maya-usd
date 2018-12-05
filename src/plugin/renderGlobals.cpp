//
// Copyright 2018 Luma Pictures
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
#include "renderGlobals.h"

#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/rendererPluginRegistry.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>

#include "utils.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

std::unordered_map<TfToken, HdRenderSettingDescriptorList, TfToken::HashFunctor>
    _rendererAttributes;

constexpr auto _defaultRenderGlobalsName = "defaultRenderGlobals";
} // namespace

void MtohInitializeRenderGlobals() {
    for (const auto& rendererPluginName : MtohGetRendererPlugins()) {
        auto* rendererPlugin =
            HdxRendererPluginRegistry::GetInstance().GetRendererPlugin(
                rendererPluginName);
        if (rendererPlugin == nullptr) { continue; }
        auto* renderDelegate = rendererPlugin->CreateRenderDelegate();
        if (renderDelegate == nullptr) { continue; }
        _rendererAttributes[rendererPluginName] =
            renderDelegate->GetRenderSettingDescriptors();
        delete renderDelegate;
    }
}

MObject MtohCreateRenderGlobals() {
    MSelectionList slist;
    slist.add(MString(_defaultRenderGlobalsName));
    MObject ret;
    if (slist.length() == 0 || !slist.getDependNode(0, ret)) { return ret; }
    return ret;
}

MtohRenderGlobals MtohReadRenderGlobals() {
    const auto obj = MtohCreateRenderGlobals();
    MtohRenderGlobals ret;
    if (obj.isNull()) { return ret; }
    MStatus status;
    MFnDependencyNode node(obj, &status);
    if (!status) { return ret; }
    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
