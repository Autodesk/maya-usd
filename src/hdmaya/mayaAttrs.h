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

// dagNode
extern MObject visibility;
extern MObject worldMatrix;
extern MObject intermediateObject;
extern MObject instObjGroups;

// nonAmbientLightShapeNode
extern MObject decayRate;
extern MObject emitDiffuse;
extern MObject emitSpecular;

// nonExtendedLightShapeNode
extern MObject dmapResolution;
extern MObject dmapBias;
extern MObject dmapFilterSize;
extern MObject useDepthMapShadows;

// spotLight
extern MObject coneAngle;
extern MObject dropoff;

// surfaceShape
extern MObject doubleSided;

// mesh
extern MObject pnts;
extern MObject inMesh;

MStatus initialize();

}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_ATTRS_H__
