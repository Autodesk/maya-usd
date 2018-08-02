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

#include <hdmaya/adapters/mayaAttrs.h>

#include <pxr/base/tf/diagnostic.h>

#include <maya/MNodeClass.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace MayaAttrs {

namespace dagNode {

MObject visibility;
MObject worldMatrix;
MObject intermediateObject;
MObject instObjGroups;

} // namespace dagNode

namespace nonAmbientLightShapeNode {

MObject decayRate;
MObject emitDiffuse;
MObject emitSpecular;

} // namespace nonAmbientLightShapeNode

namespace nonExtendedLightShapeNode {

MObject dmapResolution;
MObject dmapBias;
MObject dmapFilterSize;
MObject useDepthMapShadows;

} // namespace nonExtendedLightShapeNode

namespace spotLight {

MObject coneAngle;
MObject dropoff;

} // namespace spotLight

namespace surfaceShape {

MObject doubleSided;

} // namespace surfaceShape

// mesh

namespace mesh {

MObject pnts;
MObject inMesh;

} // namespace mesh

namespace shadingEngine {

MObject surfaceShader;

} // namespace shadingEngine

namespace file {

MObject computedFileTextureNamePattern;
MObject fileTextureNamePattern;
MObject uvTilingMode;

} // namespace file

namespace imagePlane {
    
MObject imageName;
MObject useFrameExtension;
MObject frameOffset;
MObject frameExtension;
MObject displayMode;

MObject fit;
MObject coverage;
MObject coverageOrigin;
MObject depth;
MObject rotate;
MObject size;
MObject offset;
MObject width;
MObject height;
MObject imageCenter;

} // namespace imagePlane

MStatus initialize() {
    MStatus status;

    auto setAttrObj = [&status](MObject& attrObj, MNodeClass& nodeClass, const MString& name) {
        if (!TF_VERIFY(attrObj.isNull())) {
            // This attempts to avoid copy-paste mistakes - ie, I had done:
            //      setAttrObj(instObjGroups, nodeClass, "instObjGroups");
            //      setAttrObj(instObjGroups, nodeClass, "intermediateObject");
            // ...which this would catch...
            status = MS::kFailure;
            MString errMsg("Attempted to assign the attribute '");
            errMsg += nodeClass.typeName();
            errMsg += ".";
            errMsg += name;
            errMsg +=
                "' to a non-empty MObject - check to ensure you're "
                "assigning it to the correct object, and that you're not "
                "running initialize() twice";
            status.perror(errMsg);
            return;
        }
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
        using namespace dagNode;
        MNodeClass nodeClass("dagNode");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        setAttrObj(visibility, nodeClass, "visibility");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(worldMatrix, nodeClass, "worldMatrix");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(intermediateObject, nodeClass, "intermediateObject");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(instObjGroups, nodeClass, "instObjGroups");
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        using namespace nonAmbientLightShapeNode;
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
        using namespace nonExtendedLightShapeNode;
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
        using namespace spotLight;
        MNodeClass nodeClass("spotLight");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        setAttrObj(coneAngle, nodeClass, "coneAngle");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(dropoff, nodeClass, "dropoff");
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        using namespace surfaceShape;
        MNodeClass nodeClass("surfaceShape");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        setAttrObj(doubleSided, nodeClass, "doubleSided");
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        using namespace mesh;
        MNodeClass nodeClass("mesh");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        setAttrObj(pnts, nodeClass, "pnts");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(inMesh, nodeClass, "inMesh");
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        using namespace shadingEngine;
        MNodeClass nodeClass("shadingEngine");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        setAttrObj(surfaceShader, nodeClass, "surfaceShader");
        if (!TF_VERIFY(status)) { return status; }
    }

    {
        using namespace file;
        MNodeClass nodeClass("file");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        setAttrObj(computedFileTextureNamePattern, nodeClass, "computedFileTextureNamePattern");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(fileTextureNamePattern, nodeClass, "fileTextureNamePattern");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(uvTilingMode, nodeClass, "uvTilingMode");
        if (!TF_VERIFY(status)) { return status; }
    }
    {
        using namespace imagePlane;
        MNodeClass nodeClass("imagePlane");
        if (!TF_VERIFY(nodeClass.typeId() != 0)) { return MStatus::kFailure; }

        setAttrObj(displayMode, nodeClass, "displayMode");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(imageName, nodeClass, "imageName");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(useFrameExtension, nodeClass, "useFrameExtension");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(frameOffset, nodeClass, "frameOffset");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(frameExtension, nodeClass, "frameExtension");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(fit, nodeClass, "fit");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(coverage, nodeClass, "coverage");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(coverageOrigin, nodeClass, "coverageOrigin");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(depth, nodeClass, "depth");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(rotate, nodeClass, "rotate");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(size, nodeClass, "size");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(offset, nodeClass, "offset");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(width, nodeClass, "width");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(height, nodeClass, "height");
        if (!TF_VERIFY(status)) { return status; }

        setAttrObj(imageCenter, nodeClass, "imageCenter");
        if (!TF_VERIFY(status)) { return status; }

    }
    return MStatus::kSuccess;
}

} // namespace MayaAttrs

PXR_NAMESPACE_CLOSE_SCOPE
