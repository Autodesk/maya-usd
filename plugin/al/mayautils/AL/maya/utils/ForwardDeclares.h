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
#include <cstdint>
#include <maya/MTypes.h>
#include <string>

#if MAYA_API_VERSION >= 201800

// Maya 2018 introduced a namespace for the maya api... this also forward declares
// the classes, and adds "using" directives
#include <maya/MApiNamespace.h>

#else  // MAYA_API_VERSION >= 201800

// forward declare maya api types
class MAngle;
class MArgList;
class MArgDatabase;
class MBoundingBox;
class MColor;
class MDagPath;
class MDagModifier;
class MDataBlock;
class MDistance;
class MDGModifier;
class MEulerRotation;
class MFloatPoint;
class MFloatVector;
class MFloatMatrix;
class MMatrix;
class MFnDependencyNode;
class MFnDagNode;
class MFnMesh;
class MFnTransform;
class MMatrix;
class MObject;
class MPlug;
class MPoint;
class MPxData;
class MStatus;
class MString;
class MSyntax;
class MTime;
class MVector;
class MUserData;

#endif // MAYA_API_VERSION >= 201800

