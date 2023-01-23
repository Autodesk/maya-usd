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

#if defined(MAYAUSD_VERSION)
    #include <mayaUsd/nodes/proxyShapeBase.h>
#endif

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/tf/type.h>

#include <maya/MFnDependencyNode.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(MayaHydraAdapterRegistry);

void MayaHydraAdapterRegistry::RegisterShapeAdapter(const TfToken& type, ShapeAdapterCreator creator)
{
    GetInstance()._dagAdapters.insert({ type, creator });
}

MayaHydraAdapterRegistry::ShapeAdapterCreator
MayaHydraAdapterRegistry::GetShapeAdapterCreator(const MDagPath& dag)
{
    MFnDependencyNode   depNode(dag.node());
    ShapeAdapterCreator ret = nullptr;
    TfMapLookup(GetInstance()._dagAdapters, TfToken(depNode.typeName().asChar()), &ret);

    return ret;
}

void MayaHydraAdapterRegistry::RegisterLightAdapter(const TfToken& type, LightAdapterCreator creator)
{
    GetInstance()._lightAdapters.insert({ type, creator });
}

MayaHydraAdapterRegistry::LightAdapterCreator
MayaHydraAdapterRegistry::GetLightAdapterCreator(const MDagPath& dag)
{
    return GetLightAdapterCreator(dag.node());
}

MayaHydraAdapterRegistry::LightAdapterCreator
MayaHydraAdapterRegistry::GetLightAdapterCreator(const MObject& node)
{
    MFnDependencyNode   depNode(node);
    LightAdapterCreator ret = nullptr;
    TfMapLookup(GetInstance()._lightAdapters, TfToken(depNode.typeName().asChar()), &ret);
    return ret;
}

void MayaHydraAdapterRegistry::RegisterCameraAdapter(const TfToken& type, CameraAdapterCreator creator)
{
    GetInstance()._cameraAdapters.insert({ type, creator });
}

MayaHydraAdapterRegistry::CameraAdapterCreator
MayaHydraAdapterRegistry::GetCameraAdapterCreator(const MDagPath& dag)
{
    MFnDependencyNode    depNode(dag.node());
    CameraAdapterCreator ret = nullptr;
    TfMapLookup(GetInstance()._cameraAdapters, TfToken(depNode.typeName().asChar()), &ret);
    return ret;
}

void MayaHydraAdapterRegistry::RegisterMaterialAdapter(
    const TfToken&         type,
    MaterialAdapterCreator creator)
{
    GetInstance()._materialAdapters.insert({ type, creator });
}

MayaHydraAdapterRegistry::MaterialAdapterCreator
MayaHydraAdapterRegistry::GetMaterialAdapterCreator(const MObject& node)
{
    MFnDependencyNode      depNode(node);
    MaterialAdapterCreator ret = nullptr;
    TfMapLookup(GetInstance()._materialAdapters, TfToken(depNode.typeName().asChar()), &ret);
    return ret;
}

void MayaHydraAdapterRegistry::LoadAllPlugin()
{
    static std::once_flag loadAllOnce;
    std::call_once(loadAllOnce, []() {
        TfRegistryManager::GetInstance().SubscribeTo<MayaHydraAdapterRegistry>();

        const TfType& adapterType = TfType::Find<MayaHydraAdapter>();
        if (adapterType.IsUnknown()) {
            TF_CODING_ERROR("Could not find MayaHydraAdapter type");
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
