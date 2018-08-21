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
#ifndef __HDMAYA_ADAPTER_REGISTRY_H__
#define __HDMAYA_ADAPTER_REGISTRY_H__

#include <pxr/base/tf/singleton.h>
#include <pxr/pxr.h>

#include <maya/MFn.h>

#include <hdmaya/adapters/lightAdapter.h>
#include <hdmaya/adapters/materialAdapter.h>
#include <hdmaya/adapters/shapeAdapter.h>
#include <hdmaya/delegates/delegateCtx.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaAdapterRegistry : public TfSingleton<HdMayaAdapterRegistry> {
    friend class TfSingleton<HdMayaAdapterRegistry>;
    HDMAYA_API
    HdMayaAdapterRegistry() = default;

public:
    using ShapeAdapterCreator = std::function<HdMayaShapeAdapterPtr(
        HdMayaDelegateCtx*, const MDagPath&)>;
    HDMAYA_API
    static void RegisterShapeAdapter(
        const TfToken& type, ShapeAdapterCreator creator);

    HDMAYA_API
    static ShapeAdapterCreator GetShapeAdapterCreator(const MDagPath& dag);

    using LightAdapterCreator = std::function<HdMayaLightAdapterPtr(
        HdMayaDelegateCtx*, const MDagPath&)>;
    HDMAYA_API
    static void RegisterLightAdapter(
        const TfToken& type, LightAdapterCreator creator);

    HDMAYA_API
    static LightAdapterCreator GetLightAdapterCreator(const MDagPath& dag);

    using MaterialAdapterCreator = std::function<HdMayaMaterialAdapterPtr(
        const SdfPath&, HdMayaDelegateCtx*, const MObject&)>;
    HDMAYA_API
    static void RegisterMaterialAdapter(
        const TfToken& type, MaterialAdapterCreator creator);

    HDMAYA_API
    static MaterialAdapterCreator GetMaterialAdapterCreator(
        const MObject& node);

    // Find all HdMayaAdapter plugins, and load them all
    HDMAYA_API
    static void LoadAllPlugin();

private:
    std::unordered_map<TfToken, ShapeAdapterCreator, TfToken::HashFunctor>
        _dagAdapters;
    std::unordered_map<TfToken, LightAdapterCreator, TfToken::HashFunctor>
        _lightAdapters;
    std::unordered_map<TfToken, MaterialAdapterCreator, TfToken::HashFunctor>
        _materialAdapters;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_ADAPTER_REGISTRY_H__
