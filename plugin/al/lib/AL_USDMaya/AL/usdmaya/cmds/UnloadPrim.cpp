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
#include "AL/usdmaya/cmds/UnloadPrim.h"

#include "AL/maya/utils/CommandGuiHelper.h"
#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/fileio/TransformIterator.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"

#include <pxr/base/tf/type.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/usd/kind/registry.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/variantSets.h>

#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>

namespace AL {
namespace usdmaya {
namespace cmds {

AL_MAYA_DEFINE_COMMAND(ChangeVariant, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax ChangeVariant::createSyntax()
{
    MSyntax syntax;
    syntax.useSelectionAsDefault(true);
    syntax.setObjectType(MSyntax::kSelectionList, 0);
    syntax.addFlag("-pp", "-primPath", MSyntax::kString);
    syntax.addFlag("-vs", "-variantSet", MSyntax::kString);
    syntax.addFlag("-v", "-variant", MSyntax::kString);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
bool ChangeVariant::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MStatus ChangeVariant::doIt(const MArgList& args)
{
    try {
        TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ChangeVariant::doIt\n");
        MStatus      status;
        MArgDatabase database(syntax(), args, &status);
        if (!status)
            return status;

        MString pp;
        MString vset;
        MString variant;

        if (!database.isFlagSet("-pp") || !database.isFlagSet("-vs") || !database.isFlagSet("-v")) {
            MGlobal::displayError("Not enough information to set variant");
            return MS::kFailure;
        }

        if (!database.getFlagArgument("-pp", 0, pp) || !database.getFlagArgument("-vs", 0, vset)
            || !database.getFlagArgument("-v", 0, variant)) {
            MGlobal::displayError("Not enough information to set variant");
            return MS::kFailure;
        }

        /// find the proxy shape node
        nodes::ProxyShape* proxy = getShapeNode(database);
        if (proxy) {
            auto stage = proxy->usdStage();
            if (stage) {
                UsdPrim prim = stage->GetPrimAtPath(SdfPath(AL::maya::utils::convert(pp)));
                if (prim) {
                    UsdVariantSet actualSet = prim.GetVariantSet(AL::maya::utils::convert(vset));
                    if (actualSet) {
                        if (!actualSet.SetVariantSelection(AL::maya::utils::convert(variant))) {
                            MGlobal::displayError("could not switch variant");
                        }
                    }
                }
            }
        }
    } catch (MStatus&) {
        std::cout << "Error" << std::endl;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(ActivatePrim, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax ActivatePrim::createSyntax()
{
    MSyntax syntax;
    syntax.useSelectionAsDefault(true);
    syntax.setObjectType(MSyntax::kSelectionList, 0);
    syntax.addFlag("-pp", "-primPath", MSyntax::kString);
    syntax.addFlag("-a", "-activate", MSyntax::kBoolean);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
bool ActivatePrim::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MStatus ActivatePrim::doIt(const MArgList& args)
{
    try {
        TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("ActivatePrim::doIt\n");
        MStatus      status;
        MArgDatabase database(syntax(), args, &status);
        if (!status)
            return status;

        MString pp;
        bool    active = false;

        if (!database.isFlagSet("-pp") || !database.isFlagSet("-a")) {
            MGlobal::displayError("Not enough information to activate prim");
            return MS::kFailure;
        }

        if (!database.getFlagArgument("-pp", 0, pp) || !database.getFlagArgument("-a", 0, active)) {
            MGlobal::displayError("Not enough information to activate prim");
            return MS::kFailure;
        }

        /// find the proxy shape node
        nodes::ProxyShape* proxy = getShapeNode(database);
        if (proxy) {
            auto stage = proxy->usdStage();
            if (stage) {
                UsdPrim prim = stage->GetPrimAtPath(SdfPath(AL::maya::utils::convert(pp)));
                if (prim) {
                    prim.SetActive(active);
                } else {
                    std::cout << "prim not found" << std::endl;
                }
            }
        }
    } catch (MStatus) {
        std::cout << "Error" << std::endl;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace cmds
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
