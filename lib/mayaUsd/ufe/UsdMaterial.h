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
#pragma once

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

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

    UsdMaterial(const UsdSceneItem::Ptr& item);
    ~UsdMaterial() override;

    //@{
    //! Delete the copy/move constructors assignment operators.
    UsdMaterial(const UsdMaterial&) = delete;
    UsdMaterial& operator=(const UsdMaterial&) = delete;
    UsdMaterial(UsdMaterial&&) = delete;
    UsdMaterial& operator=(UsdMaterial&&) = delete;
    //@}

    //! Create a UsdMaterial.
    static UsdMaterial::Ptr create(const UsdSceneItem::Ptr& item);

    std::vector<Ufe::SceneItem::Ptr> getMaterials() const override;

#if (UFE_PREVIEW_VERSION_NUM >= 5003)
    bool hasMaterial() const override;
#endif

#if (UFE_PREVIEW_VERSION_NUM >= 5005)
    bool canAssignMaterial() const override;
#endif

private:
    UsdSceneItem::Ptr _item;
}; // UsdMaterial

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
