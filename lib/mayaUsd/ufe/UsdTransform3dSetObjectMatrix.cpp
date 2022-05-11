//
// Copyright 2021 Autodesk
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

#include "UsdTransform3dSetObjectMatrix.h"

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/ufe/XformOpUtils.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdTransform3dSetObjectMatrix::UsdTransform3dSetObjectMatrix(
    const Ufe::Transform3d::Ptr& wrapped,
    const PXR_NS::GfMatrix4d&    mlInv,
    const PXR_NS::GfMatrix4d&    mrInv)
    : UsdTransform3dBase(std::dynamic_pointer_cast<UsdSceneItem>(wrapped->sceneItem()))
    , _wrapped(wrapped)
    , _mlInv(mlInv)
    , _mrInv(mrInv)
{
}

/* static */
UsdTransform3dSetObjectMatrix::Ptr UsdTransform3dSetObjectMatrix::create(
    const Ufe::Transform3d::Ptr& wrapped,
    const PXR_NS::GfMatrix4d&    mlInv,
    const PXR_NS::GfMatrix4d&    mrInv)
{
    return std::make_shared<UsdTransform3dSetObjectMatrix>(wrapped, mlInv, mrInv);
}

Ufe::Vector3d UsdTransform3dSetObjectMatrix::translation() const
{
    // Must extract whole-object translation from the whole object's local
    // matrix.  Base class matrix() considers all the prim's transform ops,
    // which is exactly what we want.
    return getTranslation(UsdTransform3dBase::matrix());
}

Ufe::Vector3d UsdTransform3dSetObjectMatrix::rotation() const
{
    // See translation() comments.
    return getRotation(UsdTransform3dBase::matrix());
}

Ufe::Vector3d UsdTransform3dSetObjectMatrix::scale() const
{
    // See translation() comments.
    return getScale(UsdTransform3dBase::matrix());
}

Ufe::Vector3d UsdTransform3dSetObjectMatrix::rotatePivot() const { return _wrapped->rotatePivot(); }

Ufe::Vector3d UsdTransform3dSetObjectMatrix::scalePivot() const { return _wrapped->scalePivot(); }

Ufe::Vector3d UsdTransform3dSetObjectMatrix::rotatePivotTranslation() const
{
    return _wrapped->rotatePivotTranslation();
}

Ufe::Vector3d UsdTransform3dSetObjectMatrix::scalePivotTranslation() const
{
    return _wrapped->scalePivotTranslation();
}

Ufe::SetMatrix4dUndoableCommand::Ptr
UsdTransform3dSetObjectMatrix::setMatrixCmd(const Ufe::Matrix4d& m)
{
    return _wrapped->setMatrixCmd(mw(m));
}

void UsdTransform3dSetObjectMatrix::setMatrix(const Ufe::Matrix4d& m)
{
    _wrapped->setMatrix(mw(m));
}

Ufe::Matrix4d UsdTransform3dSetObjectMatrix::mw(const Ufe::Matrix4d& m) const
{
    // Compute the matrix required for our wrapped Transform3d.  As per
    // https://graphics.pixar.com/usd/docs/api/class_gf_matrix4d.html#details
    // matrix multiplication order is such that the matrix to the left of the
    // multiplication is more local than the one to the right.  Since inv(Mr)
    // is the most local and inv(Ml) the least local, we express the
    // multiplication as inv(Mr) x M x inv(Ml).
    return toUfe(_mrInv * toUsd(m) * _mlInv);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
