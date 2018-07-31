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

#include "hdmaya/mayaAttrs.h"

#include <pxr/base/tf/diagnostic.h>

#include <maya/MNodeClass.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace MayaAttrs {

// dagNode
MObject visibility;
MObject worldMatrix;
MObject intermediateObject;
MObject instObjGroups;

// nonAmbientLightShapeNode
MObject decayRate;
MObject emitDiffuse;
MObject emitSpecular;

// nonExtendedLightShapeNode
MObject dmapResolution;
MObject dmapBias;
MObject dmapFilterSize;
MObject useDepthMapShadows;

// spotLight
MObject coneAngle;
MObject dropoff;

// surfaceShape
MObject doubleSided;

// mesh
MObject pnts;
MObject inMesh;

// shadingEngine
MObject surfaceShader;

MStatus initialize() {
    MStatus status;
    {
        MNodeClass nodeClass("dagNode");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        visibility = nodeClass.attribute("visibility", &status);
        if (!TF_VERIFY(status)) { return status; }

        worldMatrix = nodeClass.attribute("worldMatrix", &status);
        if (!TF_VERIFY(status)) { return status; }

        intermediateObject = nodeClass.attribute("intermediateObject", &status);
        if (!TF_VERIFY(status)) { return status; }

        instObjGroups = nodeClass.attribute("intermediateObject", &status);
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        MNodeClass nodeClass("nonAmbientLightShapeNode");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        decayRate = nodeClass.attribute("decayRate", &status);
        if (!TF_VERIFY(status)) { return status; }

        emitDiffuse = nodeClass.attribute("emitDiffuse", &status);
        if (!TF_VERIFY(status)) { return status; }

        emitSpecular = nodeClass.attribute("emitSpecular", &status);
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        MNodeClass nodeClass("nonExtendedLightShapeNode");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        dmapResolution = nodeClass.attribute("dmapResolution", &status);
        if (!TF_VERIFY(status)) { return status; }

        dmapBias = nodeClass.attribute("dmapBias", &status);
        if (!TF_VERIFY(status)) { return status; }

        dmapFilterSize = nodeClass.attribute("dmapFilterSize", &status);
        if (!TF_VERIFY(status)) { return status; }

        useDepthMapShadows = nodeClass.attribute("useDepthMapShadows", &status);
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        MNodeClass nodeClass("spotLight");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        coneAngle = nodeClass.attribute("coneAngle", &status);
        if (!TF_VERIFY(status)) { return status; }

        dropoff = nodeClass.attribute("dropoff", &status);
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        MNodeClass nodeClass("surfaceShape");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        doubleSided = nodeClass.attribute("doubleSided", &status);
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        MNodeClass nodeClass("mesh");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        pnts = nodeClass.attribute("pnts", &status);
        if (!TF_VERIFY(status)) { return status; }

        inMesh = nodeClass.attribute("inMesh", &status);
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        MNodeClass nodeClass("shadingEngine");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        surfaceShader = nodeClass.attribute("surfaceShader", &status);
        if (!TF_VERIFY(status)) { return status; }
    }
    return MStatus::kSuccess;
}

}  // namespace MayaAttrs

PXR_NAMESPACE_CLOSE_SCOPE
