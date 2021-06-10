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

#include "UsdTransform3dUndoableCommands.h"

#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>
#include <ufe/transform3d.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdSetMatrix4dUndoableCommand::UsdSetMatrix4dUndoableCommand(
    const Ufe::Path&     path,
    const Ufe::Matrix4d& newM)
    : UsdUndoableCommand<Ufe::SetMatrix4dUndoableCommand>(path)
{
    // Decompose new matrix to extract TRS.  Neither GfMatrix4d::Factor
    // nor GfTransform decomposition provide results that match Maya,
    // so use MTransformationMatrix.
    MMatrix m;
    std::memcpy(m[0], &newM.matrix[0][0], sizeof(double) * 16);
    MTransformationMatrix                xformM(m);
    auto                                 t = xformM.getTranslation(MSpace::kTransform);
    double                               r[3];
    double                               s[3];
    MTransformationMatrix::RotationOrder rotOrder;
    xformM.getRotation(r, rotOrder);
    xformM.getScale(s, MSpace::kTransform);
    constexpr double radToDeg = 57.295779506;

    _newT = Ufe::Vector3d(t[0], t[1], t[2]);
    _newR = Ufe::Vector3d(r[0] * radToDeg, r[1] * radToDeg, r[2] * radToDeg);
    _newS = Ufe::Vector3d(s[0], s[1], s[2]);
}

UsdSetMatrix4dUndoableCommand::~UsdSetMatrix4dUndoableCommand() { }

bool UsdSetMatrix4dUndoableCommand::set(const Ufe::Matrix4d&)
{
    // No-op: Maya does not set matrices through interactive manipulation.
    TF_WARN("Illegal call to UsdSetMatrix4dUndoableCommand::set()");
    return true;
}

void UsdSetMatrix4dUndoableCommand::executeUndoBlock()
{
    // transform3d() and editTransform3d() are equivalent for a normal Maya
    // transform stack, but not for a fallback Maya transform stack, and
    // both can be edited by this command.
    auto t3d = Ufe::Transform3d::editTransform3d(sceneItem());
    t3d->translate(_newT.x(), _newT.y(), _newT.z());
    t3d->rotate(_newR.x(), _newR.y(), _newR.z());
    t3d->scale(_newS.x(), _newS.y(), _newS.z());
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
