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
#include "AL/usdmaya/fileio/ImportTranslator.h"

#include "AL/usdmaya/fileio/Import.h"

#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>

namespace AL {
namespace usdmaya {
namespace fileio {

PluginTranslatorOptionsContext   ImportTranslator::m_pluginContext;
PluginTranslatorOptions*         ImportTranslator::m_compatPluginOptions;
PluginTranslatorOptionsInstance* ImportTranslator::m_pluginInstance;

//----------------------------------------------------------------------------------------------------------------------
MStatus ImportTranslator::reader(
    const MFileObject&                    file,
    const AL::maya::utils::OptionsParser& options,
    FileAccessMode                        mode)
{
    m_params.m_parser = (maya::utils::OptionsParser*)&options;
    MString parentPath = options.getString(kParentPath);
    m_params.m_parentPath = MDagPath();
    if (parentPath.length()) {
        MSelectionList sl, sl2;
        MGlobal::getActiveSelectionList(sl);
        MGlobal::selectByName(parentPath, MGlobal::kReplaceList);
        MGlobal::getActiveSelectionList(sl2);
        MGlobal::setActiveSelectionList(sl);
        if (sl2.length()) {
            sl2.getDagPath(0, m_params.m_parentPath);
        }
    }

    m_params.m_primPath = options.getString(kPrimPath);

    m_params.m_activateAllTranslators = options.getBool(kActivateAllTranslators);
    MStringArray strings;
    options.getString(kActiveTranslatorList).split(',', strings);
    for (uint32_t i = 0, n = strings.length(); i < n; ++i) {
        m_params.m_activePluginTranslators.emplace_back(strings[i].asChar());
    }
    strings.setLength(0);
    options.getString(kInactiveTranslatorList).split(',', strings);
    for (uint32_t i = 0, n = strings.length(); i < n; ++i) {
        m_params.m_inactivePluginTranslators.emplace_back(strings[i].asChar());
    }

    m_params.m_fileName = file.fullName();
    m_params.m_animations = options.getBool(kAnimations);
    m_params.m_stageUnloaded = options.getBool(kStageUnload);
    m_params.m_forceDefaultRead = options.getBool(kReadDefaultValues);

    if (m_pluginInstance) {
        m_pluginInstance->toOptionVars("ImportTranslator");
    }

    Import importer(m_params);

    return importer ? MS::kSuccess : MS::kFailure; // done!
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
