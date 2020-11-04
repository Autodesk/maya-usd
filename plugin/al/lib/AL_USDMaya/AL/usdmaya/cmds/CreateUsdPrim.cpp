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
#include "AL/usdmaya/cmds/CreateUsdPrim.h"

#include "AL/usdmaya/nodes/ProxyShape.h"

#include <pxr/usd/usd/modelAPI.h>

#include <maya/MArgDatabase.h>
#include <maya/MFnDagNode.h>
#include <maya/MSyntax.h>

namespace AL {
namespace usdmaya {
namespace cmds {

//----------------------------------------------------------------------------------------------------------------------
nodes::ProxyShape* getShapeNode(const MArgDatabase& args)
{
    MSelectionList sl;
    args.getObjects(sl);
    MDagPath path;
    MStatus  status = sl.getDagPath(0, path);
    if (!status) {
        MGlobal::displayError("Argument is not a proxy shape");
        throw status;
    }

    if (path.node().hasFn(MFn::kTransform)) {
        path.extendToShape();
    }

    if (path.node().hasFn(MFn::kPluginShape)) {
        MFnDagNode fn(path);
        if (fn.typeId() == nodes::ProxyShape::kTypeId) {
            return (nodes::ProxyShape*)fn.userNode();
        } else {
            MGlobal::displayError("No usd proxy shape selected");
        }
    } else {
        MGlobal::displayError("No usd proxy shape selected");
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(CreateUsdPrim, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax CreateUsdPrim::createSyntax()
{
    MSyntax syn;
    syn.addFlag("-k", "-kind", MSyntax::kString);
    syn.addFlag("-h", "-help");
    syn.addArg(MSyntax::kString);
    syn.addArg(MSyntax::kString);
    syn.useSelectionAsDefault(false);
    syn.setObjectType(MSyntax::kSelectionList, 0, 1);
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus CreateUsdPrim::doIt(const MArgList& args)
{
    try {
        MStatus      status;
        MArgDatabase db(syntax(), args, &status);
        if (!status)
            throw status;
        AL_MAYA_COMMAND_HELP(db, g_helpText);

        nodes::ProxyShape* node = getShapeNode(db);
        if (!node)
            throw MS::kFailure;

        MString primPath, primType, kind;
        db.getCommandArgument(0, primPath);
        db.getCommandArgument(1, primType);

        if (db.isFlagSet("-k")) {
            db.getFlagArgument("-k", 0, kind);
        }

        auto stage = node->usdStage();

        SdfPath path(primPath.asChar());
        TfToken type(primType.asChar());

        UsdPrim prim = stage->DefinePrim(path, type);
        if (prim) {
            if (kind.length()) {
                UsdModelAPI modelAPI(prim);
                modelAPI.SetKind(TfToken(kind.asChar()));
            }
            setResult(true);
        } else {
            setResult(false);
        }

        return MS::kSuccess;
    } catch (MStatus status) {
        return status;
    }
    return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
const char* const CreateUsdPrim::g_helpText = R"(
    AL_usdmaya_CreateUsdPrim Overview:

      This command allows you to create a new prim of a specific point at a path within the stage represented by a proxy
      shape. So for example, to create a UsdLuxDiskLight prim, specify the prim path, the type, and the proxy shape to
      create the prim within.

        AL_usdmaya_CreateUsdPrim "/path/to/create" "UsdLuxDiskLight" "AL_usdmaya_ProxyShape1";

      It is also possible to use the -k/-kind flag to specify a 'Kind' which can be queried by the UsdModelAPI.

        AL_usdmaya_CreateUsdPrim -k "MyCustomKind" "/path/to/create" "UsdLuxDiskLight" "AL_usdmaya_ProxyShape1";
)";

//----------------------------------------------------------------------------------------------------------------------
} // namespace cmds
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
