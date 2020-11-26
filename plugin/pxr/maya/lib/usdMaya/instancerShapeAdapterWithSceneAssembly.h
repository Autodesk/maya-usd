//
// Copyright 2019 Pixar
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
#ifndef PXRUSDMAYAGL_INSTANCER_SHAPE_ADAPTER_WITH_SCENE_ASSEMBLY_H
#define PXRUSDMAYAGL_INSTANCER_SHAPE_ADAPTER_WITH_SCENE_ASSEMBLY_H

/// \file pxrUsdMayaGL/instancerShapeAdapterWithSceneAssembly.h

#include <mayaUsd/render/pxrUsdMayaGL/instancerShapeAdapter.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Class to manage translation of native Maya instancers into
/// UsdGeomPointInstancers for imaging with Hydra.
/// This adapter will translate instancer prototypes that are USD reference
/// assemblies into UsdGeomPointInstancer prototypes, ignoring any prototypes
/// that are not reference assemblies.
class UsdMayaGL_InstancerShapeAdapterWithSceneAssembly : public UsdMayaGL_InstancerShapeAdapter
{
public:
    ~UsdMayaGL_InstancerShapeAdapterWithSceneAssembly() override;

    /// Construct a new uninitialized UsdMayaGL_InstancerShapeAdapterWithSceneAssembly.
    UsdMayaGL_InstancerShapeAdapterWithSceneAssembly(bool isViewport2);

private:
    // For each prototype in _SyncInstancerPrototypes(), perform scene
    // assembly node processing.
    void SyncInstancerPerPrototypePostHook(
        const MPlug&              hierarchyPlug,
        UsdPrim&                  prototypePrim,
        std::vector<std::string>& layerIdsToMute) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
