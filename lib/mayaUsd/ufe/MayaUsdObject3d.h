//
// Copyright 2023 Autodesk
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

#include <usdUfe/ufe/UsdObject3d.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time 3D object interface
/*!
    This class implements the Object3d interface for USD prims.
*/
class MAYAUSD_CORE_PUBLIC MayaUsdObject3d : public UsdUfe::UsdObject3d
{
public:
    using Ptr = std::shared_ptr<MayaUsdObject3d>;

    MayaUsdObject3d(const UsdSceneItem::Ptr& item);
    ~MayaUsdObject3d() override;

    // Delete the copy/move constructors assignment operators.
    MayaUsdObject3d(const MayaUsdObject3d&) = delete;
    MayaUsdObject3d& operator=(const MayaUsdObject3d&) = delete;
    MayaUsdObject3d(MayaUsdObject3d&&) = delete;
    MayaUsdObject3d& operator=(MayaUsdObject3d&&) = delete;

    //! Create a MayaUsdObject3d.
    static MayaUsdObject3d::Ptr create(const UsdSceneItem::Ptr& item);

    // UsdObject3d overrides
    PXR_NS::TfTokenVector getPurposes(const Ufe::Path& path) const override;
    void adjustBBoxExtents(PXR_NS::GfBBox3d& bbox, const PXR_NS::UsdTimeCode time) const override;
    Ufe::BBox3d
    adjustAlignedBBox(const Ufe::BBox3d& bbox, const PXR_NS::UsdTimeCode time) const override;

}; // MayaUsdObject3d

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
