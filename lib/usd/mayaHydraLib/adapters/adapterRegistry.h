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
#ifndef MAYAHYDRALIB_ADAPTER_REGISTRY_H
#define MAYAHYDRALIB_ADAPTER_REGISTRY_H

#include <mayaHydraLib/adapters/cameraAdapter.h>
#include <mayaHydraLib/adapters/lightAdapter.h>
#include <mayaHydraLib/adapters/materialAdapter.h>
#include <mayaHydraLib/adapters/shapeAdapter.h>
#include <mayaHydraLib/adapters/renderItemAdapter.h>
#include <mayaHydraLib/delegates/delegateCtx.h>

#include <pxr/base/tf/singleton.h>
#include <pxr/pxr.h>

#include <maya/MFn.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class MayaHydraAdapterRegistry : public TfSingleton<MayaHydraAdapterRegistry>
{
    friend class TfSingleton<MayaHydraAdapterRegistry>;
    MAYAHYDRALIB_API
    MayaHydraAdapterRegistry() = default;

public:

	// Shape Adapter
	
	using ShapeAdapterCreator
        = std::function<MayaHydraShapeAdapterPtr(MayaHydraDelegateCtx*, const MDagPath&)>;
    MAYAHYDRALIB_API
    static void RegisterShapeAdapter(const TfToken& type, ShapeAdapterCreator creator);

    MAYAHYDRALIB_API
    static ShapeAdapterCreator GetShapeAdapterCreator(const MDagPath& dag);

    using LightAdapterCreator
        = std::function<MayaHydraLightAdapterPtr(MayaHydraDelegateCtx*, const MDagPath&)>;
    MAYAHYDRALIB_API
    static void RegisterLightAdapter(const TfToken& type, LightAdapterCreator creator);

    MAYAHYDRALIB_API
    static LightAdapterCreator GetLightAdapterCreator(const MDagPath& dag);

    MAYAHYDRALIB_API
    static LightAdapterCreator GetLightAdapterCreator(const MObject& dag);

    using MaterialAdapterCreator = std::function<
        MayaHydraMaterialAdapterPtr(const SdfPath&, MayaHydraDelegateCtx*, const MObject&)>;
    MAYAHYDRALIB_API
    static void RegisterMaterialAdapter(const TfToken& type, MaterialAdapterCreator creator);

    MAYAHYDRALIB_API
    static MaterialAdapterCreator GetMaterialAdapterCreator(const MObject& node);

    using CameraAdapterCreator
        = std::function<MayaHydraCameraAdapterPtr(MayaHydraDelegateCtx*, const MDagPath&)>;
    MAYAHYDRALIB_API
    static void RegisterCameraAdapter(const TfToken& type, CameraAdapterCreator creator);

    MAYAHYDRALIB_API
    static CameraAdapterCreator GetCameraAdapterCreator(const MDagPath& dag);

    // Find all MayaHydraAdapter plugins, and load them all
    MAYAHYDRALIB_API
    static void LoadAllPlugin();

private:	
    std::unordered_map<TfToken, ShapeAdapterCreator, TfToken::HashFunctor>    _dagAdapters;
    std::unordered_map<TfToken, LightAdapterCreator, TfToken::HashFunctor>    _lightAdapters;
    std::unordered_map<TfToken, MaterialAdapterCreator, TfToken::HashFunctor> _materialAdapters;
    std::unordered_map<TfToken, CameraAdapterCreator, TfToken::HashFunctor>   _cameraAdapters;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_ADAPTER_REGISTRY_H
