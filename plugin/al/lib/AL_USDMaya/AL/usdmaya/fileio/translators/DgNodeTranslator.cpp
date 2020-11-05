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
#include "AL/usdmaya/fileio/translators/DgNodeTranslator.h"

#include "AL/maya/utils/NodeHelper.h"
#include "AL/usdmaya/fileio/ExportParams.h"
#include "AL/usdmaya/fileio/ImportParams.h"

#include <mayaUsdUtils/SIMD.h>

#include <maya/MFloatMatrix.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMatrixArrayData.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MMatrixArray.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <unordered_map>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::registerType() { return MS::kSuccess; }

//----------------------------------------------------------------------------------------------------------------------
MObject DgNodeTranslator::createNode(
    const UsdPrim&        from,
    MObject               parent,
    const char*           nodeType,
    const ImporterParams& params)
{
    MFnDependencyNode fn;
    MObject           to = fn.create(nodeType);

    MStatus status = copyAttributes(from, to, params);
    AL_MAYA_CHECK_ERROR_RETURN_NULL_MOBJECT(status, "Dg node translator: unable to get attributes");

    return to;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
DgNodeTranslator::copyAttributes(const UsdPrim& from, MObject to, const ImporterParams& params)
{
    if (params.m_dynamicAttributes) {
        const std::vector<UsdAttribute> attributes = from.GetAttributes();
        for (size_t i = 0; i < attributes.size(); ++i) {
            if (attributes[i].IsAuthored() && attributes[i].HasValue()
                && attributes[i].IsCustom()) {
                if (!attributeHandled(attributes[i]))
                    addDynamicAttribute(to, attributes[i]);
            }
        }
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
DgNodeTranslator::copyAttributes(const MObject& from, UsdPrim& to, const ExporterParams& params)
{
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
bool DgNodeTranslator::attributeHandled(const UsdAttribute& usdAttr) { return false; }

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
