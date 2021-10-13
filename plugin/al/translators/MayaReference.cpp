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
#include "MayaReference.h"

#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/fileio/translators/DgNodeTranslator.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/utils/DgNodeHelper.h"
#include "AL/usdmaya/utils/Utils.h"

#include <mayaUsd/fileio/translators/translatorMayaReference.h>
#include <mayaUsd_Schemas/ALMayaReference.h>
#include <mayaUsd_Schemas/MayaReference.h>

#include <pxr/usd/usd/attribute.h>

#include <maya/MDGModifier.h>
#include <maya/MFileIO.h>
#include <maya/MFnCamera.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTransform.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MNodeClass.h>
#include <maya/MPlug.h>
#include <maya/MProfiler.h>
#include <maya/MSelectionList.h>

namespace {
const int _mayaReferenceProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
    "MayaReference",
    "MayaReference"
#else
    "MayaReference"
#endif
);
} // namespace

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

AL_USDMAYA_DEFINE_TRANSLATOR(MayaReference, MayaUsd_SchemasMayaReference)
AL_USDMAYA_DEFINE_TRANSLATOR(ALMayaReference, MayaUsd_SchemasALMayaReference)

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReference::initialize()
{
    // Initialise all the class plugs
    return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReference::import(const UsdPrim& prim, MObject& parent, MObject& createdObj)
{
    MProfilingScope profilerScope(_mayaReferenceProfilerCategory, MProfiler::kColorE_L3, "Import");

    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("MayaReference::import prim=%s\n", prim.GetPath().GetText());
    return UsdMayaTranslatorMayaReference::update(prim, parent);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReference::tearDown(const SdfPath& primPath)
{
    MProfilingScope profilerScope(
        _mayaReferenceProfilerCategory, MProfiler::kColorE_L3, "Tear down");

    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReference::tearDown prim=%s\n", primPath.GetText());
    MObject       mayaObject;
    MObjectHandle handle;
    context()->getTransform(primPath, handle);
    mayaObject = handle.object();
    UsdMayaTranslatorMayaReference::UnloadMayaReference(mayaObject, primPath);
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReference::update(const UsdPrim& prim)
{
    MProfilingScope profilerScope(_mayaReferenceProfilerCategory, MProfiler::kColorE_L3, "Update");

    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("MayaReference::update prim=%s\n", prim.GetPath().GetText());
    MObjectHandle handle;
    if (!context()->getTransform(prim, handle)) {
        MGlobal::displayError(
            MString("MayaReference::update unable to find the transform node for prim: ")
            + prim.GetPath().GetText());
    }
    MObject parent = handle.object();
    return UsdMayaTranslatorMayaReference::update(prim, parent);
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
