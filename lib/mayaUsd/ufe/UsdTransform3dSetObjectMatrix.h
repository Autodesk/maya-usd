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
#pragma once

#include <mayaUsd/ufe/UfeVersionCompat.h>
#include <mayaUsd/ufe/UsdTransform3dBase.h>

#include <pxr/base/gf/matrix4d.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to set the matrix of objects in 3D.
//
// The UsdTransform3dMatrixOp and UsdTransform3dFallbackMayaXformStack classes
// implement the Transform3d interface for a single matrix transform op and
// for a fallback Maya transform stack, respectively.
//
// In both cases the complete transform stack will have additional transform
// ops that define the complete 3D transformation for the whole object, while
// only part of the complete transform stack (a single matrix and the fallback
// Maya transform stack) is editable.
//
// Certain Maya operations (such as parent -absolute) require the capability to
// edit the matrix transform of the whole object.  This class wraps a
// UsdTransform3dMatrixOp or UsdTransform3dFallbackMayaXformStack and converts
// the whole object matrix into a matrix appropriate for UsdTransform3dMatrixOp
// or UsdTransform3dFallbackMayaXformStack.
//
// The matrix algebra is simple: given a wrapped (decorated) Transform3d Mw
// whose matrix we wish to change, we can express it as a part of the complete
// transform stack M in the following way:
//
// M = M  M  M
//      l  w  r
//
// then
//       -1     -1
// M  = M   M  M
//  w    l      r
//
// Therefore, given M as an argument, we can compute Mw given the fixed
// matrices inv(Ml) and inv(Mr).  In the case of the Maya fallback transform
// stack, Mr is the identity by definition, as the Maya fallback transform
// stack must be the last group of transform ops in the transform stack.
//
// Here is an example given a Maya fallback transform stack:
//
// ["xformOp:translate", "xformOp:rotateXYZ", "xformOp:rotateX",
//  "xformOp:translate:maya_fallback", "xformOp:rotateXYZ:maya_fallback",
//  "xformOp:scale:maya_fallback"]
//
// Note how there are two rotation transform ops in the original stack, which
// does not match a standard Maya transform stack, and forces the use of a
// fallback Maya transform stack.  For fallback Maya transform stacks, Mr is
// always the identity, and Ml is the multiplication of all transform ops
// before the fallback Maya transform stack, i.e. here
// "xformOp:translate" "xformOp:rotateXYZ" "xformOp:rotateX"
// .  Mw in this case is the entire fallback Maya transform stack, our target.
//
// Here are three examples given a transform stack with multiple matrix
// transform ops:
//
// ["xformOp:transform:A", "xformOp:transform:B", "xformOp:transform:C"]
//
// If we are targeting matrix transform op Mw == "xformOp:transform:A", then
// Ml is the identity matrix, and Mr is
// "xformOp:transform:B" "xformOp:transform:C".
//
// If we are targeting matrix transform op Mw == "xformOp:transform:B", then
// Ml is "xformOp:transform:A", and Mr is "xformOp:transform:C".
//
// If we are targeting matrix transform op Mw == "xformOp:transform:C", then
// Ml is "xformOp:transform:A" "xformOp:transform:B", and Mr is the identity
// matrix.

class MAYAUSD_CORE_PUBLIC UsdTransform3dSetObjectMatrix : public UsdTransform3dBase
{
public:
    typedef std::shared_ptr<UsdTransform3dSetObjectMatrix> Ptr;

    UsdTransform3dSetObjectMatrix(
        const Ufe::Transform3d::Ptr& wrapped,
        const GfMatrix4d&            mlInv,
        const GfMatrix4d&            mrInv);
    ~UsdTransform3dSetObjectMatrix() override = default;

    //! Create a UsdTransform3dSetObjectMatrix.
    static UsdTransform3dSetObjectMatrix::Ptr
    create(const Ufe::Transform3d::Ptr& wrapped, const GfMatrix4d& mlInv, const GfMatrix4d& mrInv);

    // Implementation for base class pure virtuals (illegal calls).
    Ufe::Vector3d translation() const override;
    Ufe::Vector3d rotation() const override;
    Ufe::Vector3d scale() const override;

    Ufe::SetMatrix4dUndoableCommand::Ptr setMatrixCmd(const Ufe::Matrix4d& m) override;
    void                                 setMatrix(const Ufe::Matrix4d& m) override;

private:
    Ufe::Matrix4d mw(const Ufe::Matrix4d& m) const;

    Ufe::Transform3d::Ptr _wrapped;
    const GfMatrix4d      _mlInv { 1 };
    const GfMatrix4d      _mrInv { 1 };

}; // UsdTransform3dSetObjectMatrix

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
