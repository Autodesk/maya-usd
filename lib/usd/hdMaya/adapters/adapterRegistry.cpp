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
#include "adapterRegistry.h"

#include <mayaUsd/nodes/proxyShapeBase.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/tf/type.h>

#include <maya/MFnDependencyNode.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(HdMayaAdapterRegistry);

namespace
{
	std::string gsRenderItemTypeSuffix = "RenderItemType";
}

void HdMayaAdapterRegistry::RegisterRenderItemAdapter(const TfToken& type, RenderItemAdapterCreator creator)
{
	GetInstance()._renderItemAdapters.insert({ type, creator });
}

HdMayaAdapterRegistry::RenderItemAdapterCreator
HdMayaAdapterRegistry::GetRenderItemAdapterCreator(const MRenderItem& ri)
{
	//MFnDependencyNode   depNode(ri.node());
	RenderItemAdapterCreator ret = nullptr;
	TfMapLookup(GetInstance()._renderItemAdapters, TfToken(gsRenderItemTypeSuffix + std::to_string(ri.type())), &ret);

	return ret;
}

void HdMayaAdapterRegistry::RegisterShapeAdapter(const TfToken& type, ShapeAdapterCreator creator)
{
    GetInstance()._dagAdapters.insert({ type, creator });
}

HdMayaAdapterRegistry::ShapeAdapterCreator
HdMayaAdapterRegistry::GetShapeAdapterCreator(const MDagPath& dag)
{
    MFnDependencyNode   depNode(dag.node());
    ShapeAdapterCreator ret = nullptr;
    TfMapLookup(GetInstance()._dagAdapters, TfToken(depNode.typeName().asChar()), &ret);

    return ret;
}

HdMayaAdapterRegistry::ShapeAdapterCreator
HdMayaAdapterRegistry::GetProxyShapeAdapterCreator(const MDagPath& dag)
{
    MFnDependencyNode   depNode(dag.node());
    ShapeAdapterCreator ret = nullptr;

    // Temporary workaround for proxy shapes which may not have exact matching type in registration
    auto* proxyBase = dynamic_cast<MayaUsdProxyShapeBase*>(depNode.userNode());
    if (proxyBase) {
        TfMapLookup(
            GetInstance()._dagAdapters, TfToken(MayaUsdProxyShapeBase::typeName.asChar()), &ret);
    }

    return ret;
}

void HdMayaAdapterRegistry::RegisterLightAdapter(const TfToken& type, LightAdapterCreator creator)
{
    GetInstance()._lightAdapters.insert({ type, creator });
}

HdMayaAdapterRegistry::LightAdapterCreator
HdMayaAdapterRegistry::GetLightAdapterCreator(const MDagPath& dag)
{
    MFnDependencyNode   depNode(dag.node());
    LightAdapterCreator ret = nullptr;
    TfMapLookup(GetInstance()._lightAdapters, TfToken(depNode.typeName().asChar()), &ret);
    return ret;
}

void HdMayaAdapterRegistry::RegisterCameraAdapter(const TfToken& type, CameraAdapterCreator creator)
{
    GetInstance()._cameraAdapters.insert({ type, creator });
}

HdMayaAdapterRegistry::CameraAdapterCreator
HdMayaAdapterRegistry::GetCameraAdapterCreator(const MDagPath& dag)
{
    MFnDependencyNode    depNode(dag.node());
    CameraAdapterCreator ret = nullptr;
    TfMapLookup(GetInstance()._cameraAdapters, TfToken(depNode.typeName().asChar()), &ret);
    return ret;
}

void HdMayaAdapterRegistry::RegisterMaterialAdapter(
    const TfToken&         type,
    MaterialAdapterCreator creator)
{
    GetInstance()._materialAdapters.insert({ type, creator });
}

HdMayaAdapterRegistry::MaterialAdapterCreator
HdMayaAdapterRegistry::GetMaterialAdapterCreator(const MObject& node)
{
    MFnDependencyNode      depNode(node);
    MaterialAdapterCreator ret = nullptr;
    TfMapLookup(GetInstance()._materialAdapters, TfToken(depNode.typeName().asChar()), &ret);
    return ret;
}

void HdMayaAdapterRegistry::LoadAllPlugin()
{
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
