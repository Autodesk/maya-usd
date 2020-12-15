//
// Copyright 2016 Pixar
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
#include <mayaUsd/fileio/translators/translatorXformable.h>

#include <pxr/base/gf/math.h>
#include <pxr/base/gf/matrix3d.h>
#include <pxr/base/gf/transform.h>
#include <pxr/pxr.h>

#include <maya/MVector.h>

PXR_NAMESPACE_OPEN_SCOPE

// XXX:
// This implementation is ported from MfTransformablePrim::
// MatrixToVectorsWithPivotInvariant; needs to be generalized for arbitrary
// rotationOrder (which means lofting that concept to Gf), orthonormalization,
// etc.

static GfMatrix3d _EulerXYZToMatrix3d(GfVec3d eulerXYZ)
{
    GfMatrix3d result(
        GfRotation(GfVec3d::XAxis(), eulerXYZ[0]) * GfRotation(GfVec3d::YAxis(), eulerXYZ[1])
        * GfRotation(GfVec3d::ZAxis(), eulerXYZ[2]));
    return result;
}

// Assumes rotationOrder is XYZ.
static void _RotMatToRotTriplet(const GfMatrix4d& rotMat, GfVec3d* rotTriplet)
{
    GfRotation rot = rotMat.ExtractRotation();
    GfVec3d    angles = rot.Decompose(GfVec3d::ZAxis(), GfVec3d::YAxis(), GfVec3d::XAxis());
    (*rotTriplet)[0] = angles[2];
    (*rotTriplet)[1] = angles[1];
    (*rotTriplet)[2] = angles[0];
}

static void _MatrixToVectorsWithPivotInvariant(
    const GfMatrix4d& m,
    const GfVec3d&    pivotPosition,
    const GfVec3d&    pivotOrientation,
    GfVec3d*          translation,
    GfVec3d*          rotation,
    GfVec3d*          scale,
    GfVec3d*          scaleOrientation)
{

    GfMatrix3d pivotOrientMat = _EulerXYZToMatrix3d(pivotOrientation);

    GfMatrix4d pp = GfMatrix4d(1.0).SetTranslate(pivotPosition);
    GfMatrix4d ppInv = GfMatrix4d(1.0).SetTranslate(-pivotPosition);
    GfMatrix4d po = GfMatrix4d(1.0).SetRotate(pivotOrientMat);
    GfMatrix4d poInv = GfMatrix4d(1.0).SetRotate(pivotOrientMat.GetInverse());

    GfMatrix4d factorMe = po * pp * m * ppInv;

    GfMatrix4d scaleOrientMat, factoredRotMat, perspMat;

    factorMe.Factor(&scaleOrientMat, scale, &factoredRotMat, translation, &perspMat);

    GfMatrix4d rotMat = factoredRotMat * poInv;

    if (!rotMat.Orthonormalize(/* issueWarning */ false))
        TF_WARN("Failed to orthonormalize rotMat.");

    _RotMatToRotTriplet(rotMat, rotation);

    if (!scaleOrientMat.Orthonormalize(/* issueWarning */ false))
        TF_WARN("Failed to orthonormalize scaleOrientMat.");

    _RotMatToRotTriplet(scaleOrientMat, scaleOrientation);
}

bool UsdMayaTranslatorXformable::ConvertUsdMatrixToComponents(
    const GfMatrix4d& usdMatrix,
    GfVec3d*          trans,
    GfVec3d*          rot,
    GfVec3d*          scale)
{
    GfVec3d rotation, scaleVec, scaleOrientation;
    _MatrixToVectorsWithPivotInvariant(
        usdMatrix,
        // const TransformRotationOrder rotationOrder, XYZ
        GfVec3d(0, 0, 0), // const GfVec3d pivotPosition
        GfVec3d(0, 0, 0), // const GfVec3d pivotOrientation
        trans,
        &rotation,
        &scaleVec,
        &scaleOrientation);

    for (int i = 0; i < 3; ++i) {
        // Note that setting rotation via Maya API takes radians, even though
        // the MEL attribute itself is encoded in degrees.  #wtf
        (*rot)[i] = GfDegreesToRadians(rotation[i]);

        (*scale)[i] = scaleVec[i];
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
