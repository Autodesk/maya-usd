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

    auto setAttrObj = [&status](MObject& attrObj,
            MNodeClass& nodeClass,
            const MString& name) {
        attrObj = nodeClass.attribute(name, &status);
        if (!TF_VERIFY(status)) { return; }
        if (!TF_VERIFY(!attrObj.isNull())) {
            status = MS::kFailure;
            MString errMsg("Error finding '");
            errMsg += nodeClass.typeName();
            errMsg += ".";
            errMsg += name;
            errMsg += "' attribute";
            status.perror(errMsg);
            return;
        }
    };
    {
        MNodeClass nodeClass("dagNode");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        setAttrObj(visibility, nodeClass, "visibility");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(worldMatrix, nodeClass, "worldMatrix");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(intermediateObject, nodeClass, "intermediateObject");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(instObjGroups, nodeClass, "intermediateObject");
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        MNodeClass nodeClass("nonAmbientLightShapeNode");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        setAttrObj(decayRate, nodeClass, "decayRate");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(emitDiffuse, nodeClass, "emitDiffuse");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(emitSpecular, nodeClass, "emitSpecular");
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        MNodeClass nodeClass("nonExtendedLightShapeNode");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        setAttrObj(dmapResolution, nodeClass, "dmapResolution");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(dmapBias, nodeClass, "dmapBias");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(dmapFilterSize, nodeClass, "dmapFilterSize");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(useDepthMapShadows, nodeClass, "useDepthMapShadows");
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        MNodeClass nodeClass("spotLight");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        setAttrObj(coneAngle, nodeClass, "coneAngle");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(dropoff, nodeClass, "dropoff");
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        MNodeClass nodeClass("surfaceShape");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        setAttrObj(doubleSided, nodeClass, "doubleSided");
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        MNodeClass nodeClass("mesh");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        setAttrObj(pnts, nodeClass, "pnts");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(inMesh, nodeClass, "inMesh");
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        MNodeClass nodeClass("shadingEngine");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        setAttrObj(surfaceShader, nodeClass, "surfaceShader");
        if (!TF_VERIFY(status)) { return status; }
    }
    return MStatus::kSuccess;
}

}  // namespace MayaAttrs

PXR_NAMESPACE_CLOSE_SCOPE
