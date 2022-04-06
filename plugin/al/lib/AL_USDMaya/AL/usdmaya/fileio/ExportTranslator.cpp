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
#include "AL/usdmaya/fileio/ExportTranslator.h"

#include "AL/usdmaya/fileio/AnimationTranslator.h"
#include "AL/usdmaya/fileio/Export.h"

#include <maya/MAnimControl.h>
#include <maya/MDagPath.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MSelectionList.h>

#include <unordered_set>

namespace AL {
namespace usdmaya {
namespace fileio {

PluginTranslatorOptionsContext   ExportTranslator::m_pluginContext;
PluginTranslatorOptions*         ExportTranslator::m_compatPluginOptions;
PluginTranslatorOptionsInstance* ExportTranslator::m_pluginInstance;

//----------------------------------------------------------------------------------------------------------------------
const char* const ExportTranslator::compactionLevels[]
    = { "None", "Basic", "Medium", "Extensive", 0 };

//----------------------------------------------------------------------------------------------------------------------
const char* const ExportTranslator::timelineLevel[]
    = { "Default Time", "Earliest Time", "Current Time", "Min Timeline Range", 0 };

//----------------------------------------------------------------------------------------------------------------------
MStatus ExportTranslator::writer(
    const MFileObject&                    file,
    const AL::maya::utils::OptionsParser& options,
    FileAccessMode                        mode)
{
    static const std::unordered_set<std::string> ignoredNodes { "persp", "front", "top", "side" };

    ExporterParams params;
    params.m_dynamicAttributes = options.getBool(kDynamicAttributes);
    params.m_duplicateInstances = options.getBool(kDuplicateInstances);
    params.m_mergeTransforms = options.getBool(kMergeTransforms);
#if MAYA_APP_VERSION > 2019
    params.m_mergeOffsetParentMatrix = options.getBool(kMergeOffsetParentMatrix);
#endif
    params.m_fileName = file.fullName();
    params.m_selected = mode == MPxFileTranslator::kExportActiveAccessMode;
    params.m_animation = options.getBool(kAnimation);
    params.m_exportAtWhichTime = options.getBool(kExportAtWhichTime);
    params.m_exportInWorldSpace = options.getBool(kExportInWorldSpace);
    params.m_subSamples = options.getInt(kSubSamples);
    params.m_parser = (maya::utils::OptionsParser*)&options;
    params.m_activateAllTranslators = options.getBool(kActivateAllTranslators);
    MStringArray strings;
    options.getString(kActiveTranslatorList).split(',', strings);
    for (uint32_t i = 0, n = strings.length(); i < n; ++i) {
        params.m_activePluginTranslators.emplace_back(strings[i].asChar());
    }
    strings.setLength(0);
    options.getString(kInactiveTranslatorList).split(',', strings);
    for (uint32_t i = 0, n = strings.length(); i < n; ++i) {
        params.m_inactivePluginTranslators.emplace_back(strings[i].asChar());
    }

    if (m_pluginInstance) {
        m_pluginInstance->toOptionVars("ExportTranslator");
    }

    if (params.m_animation) {
        if (options.getBool(kUseTimelineRange)) {
            params.m_minFrame = MAnimControl::minTime().value();
            params.m_maxFrame = MAnimControl::maxTime().value();
        } else {
            params.m_minFrame = options.getFloat(kFrameMin);
            params.m_maxFrame = options.getFloat(kFrameMax);
        }
        params.m_animTranslator = new AnimationTranslator;
    }
    params.m_filterSample = options.getBool(kFilterSample);
    if (params.m_selected) {
        MGlobal::getActiveSelectionList(params.m_nodes);
    } else {
        MDagPath path;
        MItDag   it(MItDag::kDepthFirst, MFn::kTransform);
        while (!it.isDone()) {
            it.getPath(path);
            const MString     name = path.partialPathName();
            const std::string s(name.asChar(), name.length());
            if (ignoredNodes.find(s) == ignoredNodes.end()) {
                params.m_nodes.add(path);
            }
            it.prune();
            it.next();
        }
    }

    switch (params.m_exportAtWhichTime) {
    case 0: params.m_timeCode = UsdTimeCode::Default(); break;
    case 1: params.m_timeCode = UsdTimeCode::EarliestTime(); break;
    case 2: params.m_timeCode = UsdTimeCode(MAnimControl::currentTime().value()); break;
    case 3: params.m_timeCode = UsdTimeCode(params.m_minFrame); break;
    }

    Export exporter(params);

    if (params.m_animTranslator) {
        delete params.m_animTranslator;
    }

    // import your data

    return MS::kSuccess; // done!
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
