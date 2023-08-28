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
#ifndef MAYAHYDRALIB_MATERIAL_ADAPTER_H
#define MAYAHYDRALIB_MATERIAL_ADAPTER_H

#include <mayaHydraLib/adapters/adapter.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

class MayaHydraSceneProducer;

/**
 * \brief MayaHydraMaterialAdapter is used to handle the translation from a Maya material to hydra.
 * If you are looking for how we translate the Maya shaders to hydra and how we do the parameters
 * mapping, please see MayaHydraMaterialNetworkConverter::initialize().
 */
class MayaHydraMaterialAdapter : public MayaHydraAdapter
{
public:
    MAYAHYDRALIB_API
    MayaHydraMaterialAdapter(
        const SdfPath&        id,
        MayaHydraSceneProducer* producer,
        const MObject&        node);
    MAYAHYDRALIB_API
    virtual ~MayaHydraMaterialAdapter() = default;

    MAYAHYDRALIB_API
    bool IsSupported() const override;

    MAYAHYDRALIB_API
    bool HasType(const TfToken& typeId) const override;

    MAYAHYDRALIB_API
    void MarkDirty(HdDirtyBits dirtyBits) override;
    MAYAHYDRALIB_API
    void RemovePrim() override;
    MAYAHYDRALIB_API
    void Populate() override;

    MAYAHYDRALIB_API
    void EnableXRayShadingMode(bool enable);

    MAYAHYDRALIB_API
    virtual VtValue GetMaterialResource();

    /// \brief Updates the material tag for the material.
    /// \return True if the material tag have changed, false otherwise.
    virtual bool UpdateMaterialTag() { return false; }

    MAYAHYDRALIB_API
    static VtValue GetPreviewMaterialResource(const SdfPath& materialID);

protected:
    /// Are we in viewport XRay shading mode ?
    bool _enableXRayShadingMode = false;
};

using MayaHydraMaterialAdapterPtr = std::shared_ptr<MayaHydraMaterialAdapter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_MATERIAL_ADAPTER_H
