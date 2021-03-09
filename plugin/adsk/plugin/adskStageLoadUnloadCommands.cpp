//
// Copyright 2021 Autodesk
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
#include "adskStageLoadUnloadCommands.h"

#include <mayaUsd/utils/query.h>

#include <maya/MArgParser.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>

namespace MAYAUSD_NS_DEF {

const MString ADSKMayaUsdStageLoadAllCommand::commandName("mayaUsdStageLoadAll");
const MString ADSKMayaUsdStageUnloadAllCommand::commandName("mayaUsdStageUnloadAll");

//-----------------------------------------------------------------------------
// ADSKMayaUsdStageLoadUnloadBase
//-----------------------------------------------------------------------------

/*static*/
MSyntax ADSKMayaUsdStageLoadUnloadBase::createSyntax()
{
    MSyntax syntax;
    syntax.enableQuery(false);
    syntax.enableEdit(false);
    syntax.setObjectType(MSyntax::kStringObjects, 0, 1);
    return syntax;
}

MStatus ADSKMayaUsdStageLoadUnloadBase::doIt(const MArgList& args)
{
    MStatus    st;
    MArgParser argData(syntax(), args, &st);
    if (!st)
        return st;

    MStringArray proxyArray;
    st = argData.getObjects(proxyArray);
    if (!st || proxyArray.length() != 1)
        return MS::kInvalidParameter;

    auto prim = PXR_NS::UsdMayaQuery::GetPrim(proxyArray[0].asChar());
    if (prim.IsValid()) {
        _stage = prim.GetStage();
        _oldLoadSet = _stage->GetLoadSet();
        return redoIt();
    }

    return MS::kInvalidParameter;
}

//-----------------------------------------------------------------------------
// ADSKMayaUsdStageLoadAllCommand
//-----------------------------------------------------------------------------

/*static*/
void* ADSKMayaUsdStageLoadAllCommand::creator() { return new ADSKMayaUsdStageLoadAllCommand(); }

MStatus ADSKMayaUsdStageLoadAllCommand::redoIt()
{
    if (!_stage)
        return MS::kFailure;
    _stage->Load(); // root path, with descendants
    return MS::kSuccess;
}

MStatus ADSKMayaUsdStageLoadAllCommand::undoIt()
{
    if (!_stage)
        return MS::kFailure;
    _stage->LoadAndUnload(_oldLoadSet, PXR_NS::SdfPathSet({ PXR_NS::SdfPath::AbsoluteRootPath() }));
    return MS::kSuccess;
}

//-----------------------------------------------------------------------------
// ADSKMayaUsdStageUnloadAllCommand
//-----------------------------------------------------------------------------

/*static*/
void* ADSKMayaUsdStageUnloadAllCommand::creator() { return new ADSKMayaUsdStageUnloadAllCommand(); }

MStatus ADSKMayaUsdStageUnloadAllCommand::redoIt()
{
    if (!_stage)
        return MS::kFailure;
    _stage->Unload(); // root path
    return MS::kSuccess;
}

MStatus ADSKMayaUsdStageUnloadAllCommand::undoIt()
{
    if (!_stage)
        return MS::kFailure;
    _stage->LoadAndUnload(_oldLoadSet, PXR_NS::SdfPathSet());
    return MS::kSuccess;
}

} // namespace MAYAUSD_NS_DEF
