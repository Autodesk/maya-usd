//
// Copyright 2019 Luma Pictures
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
#include <hdMaya/adapters/mayaAttrs.h>

#include <pxr/base/tf/diagnostic.h>

#include <maya/MNodeClass.h>

#define SET_NODE_CLASS(nodeTypeName)           \
    using namespace nodeTypeName;              \
    MNodeClass nodeClass(#nodeTypeName);       \
    if (!TF_VERIFY(nodeClass.typeId() != 0)) { \
        return MStatus::kFailure;              \
    }

#define SET_ATTR_OBJ(attr)              \
    setAttrObj(attr, nodeClass, #attr); \
    if (!TF_VERIFY(status)) {           \
        return status;                  \
    }

PXR_NAMESPACE_OPEN_SCOPE

namespace MayaAttrs {

namespace node {

MObject message;

} // namespace node

namespace dagNode {

MObject visibility;
MObject worldMatrix;
MObject intermediateObject;
MObject instObjGroups;
MObject overrideEnabled;
MObject overrideVisibility;

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

namespace directionalLight {

MObject lightAngle;
}

namespace surfaceShape {

MObject doubleSided;

} // namespace surfaceShape

// mesh

namespace mesh {

MObject pnts;
MObject inMesh;
MObject uvPivot;
MObject displaySmoothMesh;
MObject smoothLevel;

} // namespace mesh

// nurbsCurve

namespace nurbsCurve {

MObject controlPoints;

} // namespace nurbsCurve

namespace shadingEngine {

MObject surfaceShader;

} // namespace shadingEngine

namespace file {

MObject computedFileTextureNamePattern;
MObject fileTextureName;
MObject fileTextureNamePattern;
MObject uvTilingMode;
MObject uvCoord;
MObject wrapU;
MObject wrapV;
MObject mirrorU;
MObject mirrorV;

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

MStatus initialize()
{
    MStatus status;

    auto setAttrObj = [&status](MObject& attrObj, MNodeClass& nodeClass, const MString& name) {
        attrObj = nodeClass.attribute(name, &status);
        if (!TF_VERIFY(status)) {
            return;
        }
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
        SET_NODE_CLASS(node);

        SET_ATTR_OBJ(message);
    }
    {
        SET_NODE_CLASS(dagNode);

        SET_ATTR_OBJ(visibility);
        SET_ATTR_OBJ(worldMatrix);
        SET_ATTR_OBJ(intermediateObject);
        SET_ATTR_OBJ(instObjGroups);
        SET_ATTR_OBJ(overrideEnabled);
        SET_ATTR_OBJ(overrideVisibility);
    }

    {
        SET_NODE_CLASS(nonAmbientLightShapeNode);

        SET_ATTR_OBJ(decayRate);
        SET_ATTR_OBJ(emitDiffuse);
        SET_ATTR_OBJ(emitSpecular);
    }

    {
        SET_NODE_CLASS(nonExtendedLightShapeNode);

        SET_ATTR_OBJ(dmapResolution);
        SET_ATTR_OBJ(dmapBias);
        SET_ATTR_OBJ(dmapFilterSize);
    }

    {
        SET_NODE_CLASS(spotLight);

        SET_ATTR_OBJ(coneAngle);
        SET_ATTR_OBJ(dropoff);
    }

    {
        SET_NODE_CLASS(directionalLight);

        SET_ATTR_OBJ(lightAngle);
    }

    {
        SET_NODE_CLASS(surfaceShape);

        SET_ATTR_OBJ(doubleSided);
    }

    {
        SET_NODE_CLASS(mesh);

        SET_ATTR_OBJ(pnts);
        SET_ATTR_OBJ(inMesh);
        SET_ATTR_OBJ(uvPivot);
        SET_ATTR_OBJ(displaySmoothMesh);
        SET_ATTR_OBJ(smoothLevel);
    }

    {
        SET_NODE_CLASS(nurbsCurve);

        SET_ATTR_OBJ(controlPoints);
    }

    {
        SET_NODE_CLASS(shadingEngine);

        SET_ATTR_OBJ(surfaceShader);
    }

    {
        SET_NODE_CLASS(file);

        SET_ATTR_OBJ(computedFileTextureNamePattern);
        SET_ATTR_OBJ(fileTextureName);
        SET_ATTR_OBJ(fileTextureNamePattern);
        SET_ATTR_OBJ(uvTilingMode);
        SET_ATTR_OBJ(uvCoord);
        SET_ATTR_OBJ(wrapU);
        SET_ATTR_OBJ(wrapV);
        SET_ATTR_OBJ(mirrorU);
        SET_ATTR_OBJ(mirrorV);
    }

    {
        SET_NODE_CLASS(imagePlane);

        SET_ATTR_OBJ(displayMode);
        SET_ATTR_OBJ(imageName);
        SET_ATTR_OBJ(useFrameExtension);
        SET_ATTR_OBJ(frameOffset);
        SET_ATTR_OBJ(frameExtension);
        SET_ATTR_OBJ(fit);
        SET_ATTR_OBJ(coverage);
        SET_ATTR_OBJ(coverageOrigin);
        SET_ATTR_OBJ(depth);
        SET_ATTR_OBJ(rotate);
        SET_ATTR_OBJ(size);
        SET_ATTR_OBJ(offset);
        SET_ATTR_OBJ(width);
        SET_ATTR_OBJ(height);
        SET_ATTR_OBJ(imageCenter);
    }

    return MStatus::kSuccess;
}

} // namespace MayaAttrs

PXR_NAMESPACE_CLOSE_SCOPE
