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
#ifndef HDMAYA_ATTRS_H
#define HDMAYA_ATTRS_H

#include <hdMaya/api.h>

#include <pxr/pxr.h>

#include <maya/MObject.h>
#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE

#ifdef __GNUC__
#pragma GCC visibility push(hidden)
#endif

namespace MayaAttrs {

namespace node {

extern MObject message;

} // namespace node

namespace dagNode {

using namespace node;
extern MObject visibility;
extern MObject worldMatrix;
extern MObject intermediateObject;
extern MObject instObjGroups;
extern MObject overrideEnabled;
extern MObject overrideVisibility;

} // namespace dagNode

namespace nonAmbientLightShapeNode {

using namespace dagNode;
extern MObject decayRate;
extern MObject emitDiffuse;
extern MObject emitSpecular;

} // namespace nonAmbientLightShapeNode

namespace nonExtendedLightShapeNode {

using namespace nonAmbientLightShapeNode;
extern MObject dmapResolution;
extern MObject dmapBias;
extern MObject dmapFilterSize;

} // namespace nonExtendedLightShapeNode

namespace spotLight {

using namespace nonExtendedLightShapeNode;
extern MObject coneAngle;
extern MObject dropoff;

} // namespace spotLight

namespace directionalLight {

using namespace nonExtendedLightShapeNode;
extern MObject lightAngle;

} // namespace directionalLight

namespace surfaceShape {

using namespace dagNode;
extern MObject doubleSided;

} // namespace surfaceShape

// mesh

namespace mesh {

using namespace surfaceShape;
extern MObject pnts;
extern MObject inMesh;
extern MObject uvPivot;
extern MObject displaySmoothMesh;
extern MObject smoothLevel;

} // namespace mesh

// curve

namespace nurbsCurve {

using namespace surfaceShape;
extern MObject controlPoints;

} // namespace nurbsCurve

namespace shadingEngine {

using namespace node;
extern MObject surfaceShader;

} // namespace shadingEngine

namespace file {

using namespace node;
extern MObject computedFileTextureNamePattern;
extern MObject fileTextureName;
extern MObject fileTextureNamePattern;
extern MObject uvTilingMode;
extern MObject uvCoord;
extern MObject wrapU;
extern MObject wrapV;
extern MObject mirrorU;
extern MObject mirrorV;

} // namespace file

namespace imagePlane {

using namespace dagNode;
extern MObject imageName;
extern MObject useFrameExtension;
extern MObject frameOffset;
extern MObject frameExtension;
extern MObject displayMode;

extern MObject fit;
extern MObject coverage;
extern MObject coverageOrigin;
extern MObject depth;
extern MObject rotate;
extern MObject size;
extern MObject offset;
extern MObject width;
extern MObject height;
extern MObject imageCenter;

} // namespace imagePlane

HDMAYA_API
MStatus initialize();

} // namespace MayaAttrs

#ifdef __GNUC__
#pragma GCC visibility pop
#endif

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_ATTRS_H
