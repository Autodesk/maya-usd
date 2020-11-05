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
#include "AL/maya/utils/NodeHelper.h"

#include <maya/MPxNode.h>

namespace AL {
namespace maya {
namespace tests {
namespace utils {
//----------------------------------------------------------------------------------------------------------------------
// Maya node to test the NodeHelper class
//----------------------------------------------------------------------------------------------------------------------
#ifndef AL_GENERATING_DOCS
class NodeHelperUnitTest
    : public AL::maya::utils::NodeHelper
    , public MPxNode
{
public:
    static void*         creator();
    static MStatus       initialise();
    static const MString kTypeName;
    static const MTypeId kTypeId;
    static void          runUnitTest();

private:
    MStatus compute(const MPlug& plug, MDataBlock& datablock) override;

    static MObject loadFilename;
    static MObject saveFilename;
    static MObject directoryWithFile;
    static MObject directory;
    static MObject multiFile;
    static MObject inPreFrame;
    static MObject inBool;
    static MObject inBoolHidden;
    static MObject inInt8;
    static MObject inInt8Hidden;
    static MObject inInt16;
    static MObject inInt16Hidden;
    static MObject inInt32;
    static MObject inInt32Hidden;
    static MObject inInt64;
    static MObject inInt64Hidden;
    static MObject inFloat;
    static MObject inFloatHidden;
    static MObject inDouble;
    static MObject inDoubleHidden;
    static MObject inPoint;
    static MObject inPointHidden;
    static MObject inFloatPoint;
    static MObject inFloatPointHidden;
    static MObject inVector;
    static MObject inVectorHidden;
    static MObject inFloatVector;
    static MObject inFloatVectorHidden;
    static MObject inString;
    static MObject inStringHidden;
    static MObject inColour;
    static MObject inColourHidden;
    static MObject inMatrix;
    static MObject inMatrixHidden;
    static MObject outBool;
    static MObject outInt8;
    static MObject outInt16;
    static MObject outInt32;
    static MObject outInt64;
    static MObject outFloat;
    static MObject outDouble;
    static MObject outPoint;
    static MObject outFloatPoint;
    static MObject outVector;
    static MObject outFloatVector;
    static MObject outString;
    static MObject outColour;
    static MObject outMatrix;
};
#endif
//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace tests
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
