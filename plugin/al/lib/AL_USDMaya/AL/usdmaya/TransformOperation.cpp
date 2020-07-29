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
#include "AL/usdmaya/TransformOperation.h"

#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <string>
#include <vector>

namespace AL {
namespace usdmaya {

//----------------------------------------------------------------------------------------------------------------------
// usd transform components (e.g. rotate, scale, etc) are all stringly typed. There is no 'simple'
// way of determining how these transforms should be interpreted (e.g. should we use a maya
// transform, a joint, etc). The original maya USD bridge performs an O(n^2) comparison between two
// string arrays. If all the strings in one array, are found in the second array, then the transform
// is interpreted as a maya type. String compares seems to be the only way to do this, so this code
// attempts to improve on the original bridge by being a little bit more cautious in how those
// strcmps are performed.
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
static const std::string g_opNames[] = { std::string("translate"),
                                         std::string("pivot"),
                                         std::string("rotatePivotTranslate"),
                                         std::string("rotatePivot"),
                                         std::string("rotate"),
                                         std::string("rotateAxis"),
                                         std::string("rotatePivotINV"),
                                         std::string("scalePivotTranslate"),
                                         std::string("scalePivot"),
                                         std::string("shear"),
                                         std::string("scale"),
                                         std::string("scalePivotINV"),
                                         std::string("pivotINV"),
                                         std::string("transform"),
                                         std::string("unknown") };

//----------------------------------------------------------------------------------------------------------------------
TransformOperation xformOpToEnum(const std::string& opName)
{
#define TestType(X) ((opName == g_opNames[X]) ? X : kUnknownOp)
    switch (opName[0]) {
    case 'p': {
        switch (opName.size()) {
        case 5: return TestType(kPivot);
        case 8: return TestType(kPivotInv);
        }
    } break;

    case 'r': {
        switch (opName.size()) {
        case 6: return TestType(kRotate);
        case 7:
        case 9: {
            if (opName.compare(0, 6, "rotate") == 0) {
                return kRotate;
            }
        }
        case 10: return TestType(kRotateAxis);
        case 11: return TestType(kRotatePivot);
        case 14: return TestType(kRotatePivotInv);
        case 20: return TestType(kRotatePivotTranslate);
        }
    } break;

    case 's': {
        switch (opName.size()) {
        case 5: {
            if (opName[1] == 'c')
                return TestType(kScale);
            return TestType(kShear);
        }
        case 10: return TestType(kScalePivot);
        case 13: return TestType(kScalePivotInv);
        case 19: return TestType(kScalePivotTranslate);
        }
    } break;

    case 't': {
        if (opName == g_opNames[kTransform])
            return kTransform;
        return TestType(kTranslate);
    }
    }
#undef TestType
    return kUnknownOp;
}

//----------------------------------------------------------------------------------------------------------------------
bool matchesMayaProfile(
    std::vector<UsdGeomXformOp>::const_iterator it,
    std::vector<UsdGeomXformOp>::const_iterator end,
    std::vector<TransformOperation>::iterator   output)
{
    bool matchesMaya = true;

    int32_t last = -1;
    for (; it != end; ++it, ++output) {
        std::string attrName = it->GetBaseName();
        if (it->IsInverseOp())
            attrName += "INV";
        const TransformOperation thisOp = xformOpToEnum(attrName);
        if (thisOp == kPivot || thisOp == kPivotInv || thisOp == kTransform)
            matchesMaya = false;
        if (last >= thisOp)
            matchesMaya = false;
        *output = thisOp;
    }
    return matchesMaya;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
