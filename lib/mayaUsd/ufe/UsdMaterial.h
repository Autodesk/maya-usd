//
// Copyright 2022 Autodesk
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
#ifndef MAYAUSD_USDMATERIAL_H
#define MAYAUSD_USDMATERIAL_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UsdSceneItem.h>

#include <ufe/material.h>
#include <ufe/sceneItem.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time material interface
/*!
    This class implements the Material interface for USD prims.
*/
class MAYAUSD_CORE_PUBLIC UsdMaterial : public Ufe::Material
{
public:
    using Ptr = std::shared_ptr<UsdMaterial>;

    UsdMaterial(const UsdUfe::UsdSceneItem::Ptr& item);

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdMaterial);

    //! Create a UsdMaterial.
    static UsdMaterial::Ptr create(const UsdUfe::UsdSceneItem::Ptr& item);

    std::vector<Ufe::SceneItem::Ptr> getMaterials() const override;

#ifdef UFE_MATERIAL_HAS_HASMATERIAL
    bool hasMaterial() const override;
#endif

private:
    UsdUfe::UsdSceneItem::Ptr _item;
}; // UsdMaterial

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_USDMATERIAL_H
