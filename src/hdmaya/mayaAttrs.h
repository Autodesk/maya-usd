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
#ifndef __HDMAYA_ATTRS_H__
#define __HDMAYA_ATTRS_H__

#include <pxr/pxr.h>

#include <maya/MObject.h>
#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace MayaAttrs {

namespace dagNode {

extern MObject visibility;
extern MObject worldMatrix;
extern MObject intermediateObject;
extern MObject instObjGroups;

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
extern MObject useDepthMapShadows;

} // namespace nonExtendedLightShapeNode

namespace spotLight {

using namespace nonExtendedLightShapeNode;
extern MObject coneAngle;
extern MObject dropoff;

} // namespace spotLight

namespace surfaceShape {

using namespace dagNode;
extern MObject doubleSided;

} // namespace surfaceShape

// mesh

namespace mesh {

using namespace surfaceShape;
extern MObject pnts;
extern MObject inMesh;

} // namespace mesh

namespace shadingEngine {

extern MObject surfaceShader;

} // namespace shadingEngine

namespace file {

extern MObject computedFileTextureNamePattern;
extern MObject fileTextureNamePattern;
extern MObject uvTilingMode;

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

MStatus initialize();

} // namespace MayaAttrs

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_ATTRS_H__
