//
// Copyright 2019 Animal Logic
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
#include "AL/usdmaya/nodes/BasicTransformationMatrix.h"

#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/TypeIDs.h"

namespace AL {
namespace usdmaya {
namespace nodes {

class Scope;

BasicTransformationMatrix::BasicTransformationMatrix()
    : MPxTransformationMatrix()
    , m_prim()
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("BasicTransformationMatrix::BasicTransformationMatrix\n");
}

BasicTransformationMatrix::BasicTransformationMatrix(const UsdPrim& prim)
    : MPxTransformationMatrix()
    , m_prim(prim)
{
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
        .Msg("BasicTransformationMatrix::BasicTransformationMatrix\n");
}

MPxTransformationMatrix* BasicTransformationMatrix::creator()
{
    return new BasicTransformationMatrix;
}

const MTypeId BasicTransformationMatrix::kTypeId(AL_USDMAYA_IDENTITY_MATRIX);

void BasicTransformationMatrix::setPrim(const UsdPrim& prim, Scope* scopeNode)
{
    if (prim.IsValid()) {
        TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX)
            .Msg("BasicTransformationMatrix::setPrim %s\n", prim.GetName().GetText());
        m_prim = prim;
        UsdGeomScope scope(prim);
    } else {
        TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("BasicTransformationMatrix::setPrim null\n");
        m_prim = UsdPrim();
    }
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
