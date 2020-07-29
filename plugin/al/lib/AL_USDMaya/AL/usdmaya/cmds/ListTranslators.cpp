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
#include "AL/usdmaya/cmds/ListTranslators.h"

#include "AL/usdmaya/fileio/translators/TranslatorBase.h"

#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/debug.h>
#include <pxr/pxr.h>

#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace cmds {

AL_MAYA_DEFINE_COMMAND(ListTranslators, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax ListTranslators::createSyntax()
{
    MSyntax syn;
    syn.addFlag("-h", "-help", MSyntax::kNoArg);
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool ListTranslators::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MStatus ListTranslators::doIt(const MArgList& argList)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("AL_usdmaya_ListTranslators::doIt\n");
    try {
        MStatus      status;
        MArgDatabase args(syntax(), argList, &status);
        if (!status)
            return status;

        AL_MAYA_COMMAND_HELP(args, g_helpText);

        auto contextPtr = fileio::translators::TranslatorContext::create(0);

        std::set<TfType> derivedTypes;
        PlugRegistry::GetAllDerivedTypes<fileio::translators::TranslatorBase>(&derivedTypes);

        MStringArray strings;
        for (const TfType& t : derivedTypes) {
            if (auto* factory = t.GetFactory<fileio::translators::TranslatorFactoryBase>()) {
                if (fileio::translators::TranslatorRefPtr ptr = factory->create(contextPtr)) {
                    strings.append(ptr->getTranslatedType().GetTypeName().c_str());
                }
            }
        }

        setResult(strings);
    } catch (const MStatus&) {
    }
    return MS::kSuccess;
}

const char* const ListTranslators::g_helpText = R"(
    AL_usdmaya_ListTranslators Overview:

      This command returns an array of strings which correspond to the translator plugins registered
    with AL_USDMaya. These strings can be passed (as a semi-colon seperated list) to the Active/Inactive
    translator lists for export/import.

)";

//----------------------------------------------------------------------------------------------------------------------
} // namespace cmds
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
