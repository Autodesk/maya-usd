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
#ifndef HDMAYA_ADAPTER_REGISTRY_H
#define HDMAYA_ADAPTER_REGISTRY_H

#include <hdMaya/adapters/cameraAdapter.h>
#include <hdMaya/adapters/lightAdapter.h>
#include <hdMaya/adapters/materialAdapter.h>
#include <hdMaya/adapters/shapeAdapter.h>
#include <hdMaya/delegates/delegateCtx.h>

#include <pxr/base/tf/singleton.h>
#include <pxr/pxr.h>

#include <maya/MFn.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaAdapterRegistry : public TfSingleton<HdMayaAdapterRegistry>
{
    friend class TfSingleton<HdMayaAdapterRegistry>;
    HDMAYA_API
    HdMayaAdapterRegistry() = default;

public:
    using ShapeAdapterCreator
        = std::function<HdMayaShapeAdapterPtr(HdMayaDelegateCtx*, const MDagPath&)>;
    HDMAYA_API
    static void RegisterShapeAdapter(const TfToken& type, ShapeAdapterCreator creator);

    HDMAYA_API
    static ShapeAdapterCreator GetShapeAdapterCreator(const MDagPath& dag);
    HDMAYA_API
    static ShapeAdapterCreator GetProxyShapeAdapterCreator(const MDagPath& dag);

    using LightAdapterCreator
        = std::function<HdMayaLightAdapterPtr(HdMayaDelegateCtx*, const MDagPath&)>;
    HDMAYA_API
    static void RegisterLightAdapter(const TfToken& type, LightAdapterCreator creator);

    HDMAYA_API
    static LightAdapterCreator GetLightAdapterCreator(const MDagPath& dag);

    using MaterialAdapterCreator = std::function<
        HdMayaMaterialAdapterPtr(const SdfPath&, HdMayaDelegateCtx*, const MObject&)>;
    HDMAYA_API
    static void RegisterMaterialAdapter(const TfToken& type, MaterialAdapterCreator creator);

    HDMAYA_API
    static MaterialAdapterCreator GetMaterialAdapterCreator(const MObject& node);

    using CameraAdapterCreator
        = std::function<HdMayaCameraAdapterPtr(HdMayaDelegateCtx*, const MDagPath&)>;
    HDMAYA_API
    static void RegisterCameraAdapter(const TfToken& type, CameraAdapterCreator creator);

    HDMAYA_API
    static CameraAdapterCreator GetCameraAdapterCreator(const MDagPath& dag);

    // Find all HdMayaAdapter plugins, and load them all
    HDMAYA_API
    static void LoadAllPlugin();

private:
    std::unordered_map<TfToken, ShapeAdapterCreator, TfToken::HashFunctor>    _dagAdapters;
    std::unordered_map<TfToken, LightAdapterCreator, TfToken::HashFunctor>    _lightAdapters;
    std::unordered_map<TfToken, MaterialAdapterCreator, TfToken::HashFunctor> _materialAdapters;
    std::unordered_map<TfToken, CameraAdapterCreator, TfToken::HashFunctor>   _cameraAdapters;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_ADAPTER_REGISTRY_H
