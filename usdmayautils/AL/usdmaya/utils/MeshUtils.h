//
// Copyright 2017 Animal Logic
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

#include "./Api.h"

#include "maya/MVectorArray.h"
#include "maya/MFloatPointArray.h"
#include "maya/MIntArray.h"
#include "maya/MUintArray.h"
#include "maya/MFnMesh.h"
#include "maya/MDoubleArray.h"
#include "maya/MItMeshVertex.h"
#include "maya/MPlug.h"


#include "pxr/usd/usd/attribute.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/gf/vec3f.h"

#include "pxr/usd/usdGeom/mesh.h"

#include "AL/maya/utils/MayaHelperMacros.h"

PXR_NAMESPACE_USING_DIRECTIVE

constexpr auto _alusd_colour = "alusd_colour_";

namespace AL {
namespace usdmaya {
namespace utils {

AL_USDMAYA_UTILS_PUBLIC
void floatToDouble(double* output, const float* const input, size_t count);
AL_USDMAYA_UTILS_PUBLIC
void doubleToFloat(float* output, const double* const input, size_t count);
AL_USDMAYA_UTILS_PUBLIC
void convert3DArrayTo4DArray(const float* const input, float* const output, size_t count);
AL_USDMAYA_UTILS_PUBLIC
void gatherFaceConnectsAndVertices(const UsdGeomMesh& mesh,
                                   MFloatPointArray& points, MVectorArray &normals,
                                   MIntArray& counts, MIntArray& connects,
                                   const bool leftHanded,
                                   UsdTimeCode timeCode = UsdTimeCode::EarliestTime());
AL_USDMAYA_UTILS_PUBLIC
void convertFloatVec3ArrayToDoubleVec3Array(const float* const input, double* const output, size_t count);
AL_USDMAYA_UTILS_PUBLIC
void generateIncrementingIndices(MIntArray& indices, const size_t count);
AL_USDMAYA_UTILS_PUBLIC
void applyHoleFaces(const UsdGeomMesh& mesh, MFnMesh& fnMesh,
                    UsdTimeCode timeCode = UsdTimeCode::EarliestTime());
AL_USDMAYA_UTILS_PUBLIC
void unzipUVs(const float* const uv, float* const u, float* const v, const size_t count);
AL_USDMAYA_UTILS_PUBLIC
bool applyEdgeCreases(const UsdGeomMesh& from, MFnMesh& fnMesh,
                      UsdTimeCode timeCode = UsdTimeCode::EarliestTime());
AL_USDMAYA_UTILS_PUBLIC
void applyGlimpseSubdivParams(const UsdPrim& from, MFnMesh& fnMesh);
AL_USDMAYA_UTILS_PUBLIC
void applyGlimpseUserDataParams(const UsdPrim& from, MFnMesh& fnMesh);
AL_USDMAYA_UTILS_PUBLIC
void applyPrimVars(const UsdGeomMesh& mesh, MFnMesh& fnMesh, const MIntArray& counts, const MIntArray& connects,
                   UsdTimeCode timeCode = UsdTimeCode::EarliestTime());
AL_USDMAYA_UTILS_PUBLIC
bool applyVertexCreases(const UsdGeomMesh& from, MFnMesh& fnMesh,
                        UsdTimeCode timeCode = UsdTimeCode::EarliestTime());
AL_USDMAYA_UTILS_PUBLIC
void copyGlimpseTesselationAttributes(UsdGeomMesh& mesh, const MFnMesh& fnMesh);
AL_USDMAYA_UTILS_PUBLIC
void copyAnimalFaceColours(UsdGeomMesh& mesh, const MFnMesh& fnMesh);
AL_USDMAYA_UTILS_PUBLIC
void copyCreaseEdges(UsdGeomMesh& mesh, const MFnMesh& fnMesh, uint32_t mask = 0xFFFFFFFF);
AL_USDMAYA_UTILS_PUBLIC
void copyVertexData(const MFnMesh& fnMesh, const UsdAttribute& pointsAttr, UsdTimeCode time = UsdTimeCode::Default());
AL_USDMAYA_UTILS_PUBLIC
void copyNormalData(const MFnMesh& fnMesh, const UsdAttribute& normalsAttr, UsdTimeCode time = UsdTimeCode::Default());
AL_USDMAYA_UTILS_PUBLIC
void copyCreaseVertices(UsdGeomMesh& mesh, const MFnMesh& fnMesh, uint32_t mask = 0xFFFFFFFF);
AL_USDMAYA_UTILS_PUBLIC
void copyFaceConnectsAndPolyCounts(UsdGeomMesh& mesh, const MFnMesh& fnMesh, uint32_t mask = 0xFFFFFFFF);
AL_USDMAYA_UTILS_PUBLIC
void copyUvSetData(UsdGeomMesh& mesh, const MFnMesh& fnMesh, const bool leftHanded, bool performDiff = false);
AL_USDMAYA_UTILS_PUBLIC
void copyColourSetData(UsdGeomMesh& mesh, MFnMesh& fnMesh, bool performDiff = false);
AL_USDMAYA_UTILS_PUBLIC
void copyInvisibleHoles(UsdGeomMesh& mesh, const MFnMesh& fnMesh);
AL_USDMAYA_UTILS_PUBLIC
bool isUvSetDataSparse(const int32_t* uvCounts, const uint32_t count);
AL_USDMAYA_UTILS_PUBLIC
void zipUVs(const float* u, const float* v, float* uv, const size_t count);
AL_USDMAYA_UTILS_PUBLIC
void interleaveIndexedUvData(float* output, const float* u, const float* v, const int32_t* indices, const uint32_t numIndices);

//----------------------------------------------------------------------------------------------------------------------
} // utils
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
