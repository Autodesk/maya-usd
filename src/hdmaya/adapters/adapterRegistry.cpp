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
#include <hdmaya/adapters/adapterRegistry.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/tf/type.h>

#include <maya/MFnDependencyNode.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(HdMayaAdapterRegistry);

void HdMayaAdapterRegistry::RegisterShapeAdapter(const TfToken& type, ShapeAdapterCreator creator) {
    GetInstance()._dagAdapters.insert({type, creator});
}

HdMayaAdapterRegistry::ShapeAdapterCreator HdMayaAdapterRegistry::GetShapeAdapterCreator(
    const MDagPath& dag) {
    MFnDependencyNode depNode(dag.node());
    ShapeAdapterCreator ret = nullptr;
    TfMapLookup(GetInstance()._dagAdapters, TfToken(depNode.typeName().asChar()), &ret);
    return ret;
}

void HdMayaAdapterRegistry::RegisterLightAdapter(const TfToken& type, LightAdapterCreator creator) {
    GetInstance()._lightAdapters.insert({type, creator});
}

HdMayaAdapterRegistry::LightAdapterCreator HdMayaAdapterRegistry::GetLightAdapterCreator(
    const MDagPath& dag) {
    MFnDependencyNode depNode(dag.node());
    LightAdapterCreator ret = nullptr;
    TfMapLookup(GetInstance()._lightAdapters, TfToken(depNode.typeName().asChar()), &ret);
    return ret;
}

void HdMayaAdapterRegistry::RegisterMaterialAdapter(
    const TfToken& type, MaterialAdapterCreator creator) {
    GetInstance()._materialAdapters.insert({type, creator});
}

HdMayaAdapterRegistry::MaterialAdapterCreator HdMayaAdapterRegistry::GetMaterialAdapterCreator(
    const MObject& node) {
    MFnDependencyNode depNode(node);
    MaterialAdapterCreator ret = nullptr;
    TfMapLookup(GetInstance()._materialAdapters, TfToken(depNode.typeName().asChar()), &ret);
    return ret;
}

void HdMayaAdapterRegistry::LoadAllPlugin() {
    static std::once_flag loadAllOnce;
    std::call_once(loadAllOnce, []() {
        TfRegistryManager::GetInstance().SubscribeTo<HdMayaAdapterRegistry>();

        const TfType& adapterType = TfType::Find<HdMayaAdapter>();
        if (adapterType.IsUnknown()) {
            TF_CODING_ERROR("Could not find HdMayaAdapter type");
            return;
        }

        std::set<TfType> adapterTypes;
        adapterType.GetAllDerivedTypes(&adapterTypes);

        PlugRegistry& plugReg = PlugRegistry::GetInstance();

        for (auto& subType : adapterTypes) {
            const PlugPluginPtr pluginForType = plugReg.GetPluginForType(subType);
            if (!pluginForType) {
                TF_CODING_ERROR("Could not find plugin for '%s'", subType.GetTypeName().c_str());
                return;
            }
            pluginForType->Load();
        }
    });
}

PXR_NAMESPACE_CLOSE_SCOPE
