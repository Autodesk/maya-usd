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
#pragma once

#include <AL/maya/utils/FileTranslatorBase.h>
#include <AL/maya/utils/PluginTranslatorOptions.h>
#include <AL/usdmaya/Api.h>
#include <AL/usdmaya/fileio/ExportParams.h>

namespace AL {
namespace usdmaya {
namespace fileio {

using AL::maya::utils::PluginTranslatorOptionsContext;
using AL::maya::utils::PluginTranslatorOptionsInstance;
using AL::maya::utils::PluginTranslatorOptions;
using AL::maya::utils::PluginTranslatorOptionsContextManager;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A Maya export plugin that writes out USD files from Maya (this is largely optimised for the needs of the
///         AnimalLogic pipeline).
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_TRANSLATOR_BEGIN(ExportTranslator, "AL usdmaya export", false, true, "usda", "*.usdc;*.usda;*.usd;*.usdt");

  /// \fn     void* creator()
  /// \brief  creates an instance of the file translator
  /// \return a new file translator instance

  /// \var    const char* const kTranslatorName
  /// \brief  the name of the file translator

  /// \var    const char* const kClassName
  /// \brief  the name of the C++ file translator class

  // specify the option names (These will uniquely identify the exporter options)
  static constexpr const char* const kDynamicAttributes = "Dynamic Attributes"; ///< export dynamic attributes option name
  static constexpr const char* const kDuplicateInstances = "Duplicate Instances"; ///< export instances option name
  static constexpr const char* const kMergeTransforms = "Merge Transforms"; ///< export by merging transforms and shapes option name
  static constexpr const char* const kAnimation = "Animation"; ///< export animation data option name
  static constexpr const char* const kUseTimelineRange = "Use Timeline Range"; ///< export using the timeline range option name
  static constexpr const char* const kFrameMin = "Frame Min"; ///< specify min time frame option name
  static constexpr const char* const kFrameMax = "Frame Max"; ///< specify max time frame option name
  static constexpr const char* const kSubSamples = "Sub Samples"; ///< specify the number of sub samples to export
  static constexpr const char* const kFilterSample = "Filter Sample"; ///< export filter sample option name
  static constexpr const char* const kExportAtWhichTime = "Export At Which Time"; ///< which time code should be used for default values?
  static constexpr const char* const kExportInWorldSpace = "Export In World Space"; ///< should selected transforms be output in world space?
  static constexpr const char* const kActivateAllTranslators = "Activate all Plugin Translators"; ///< if true, all translator plugins will be enabled by default
  static constexpr const char* const kActiveTranslatorList = "Active Translator List"; ///< A comma seperated list of translator plugins that should be activated for export
  static constexpr const char* const kInactiveTranslatorList = "Inactive Translator List"; ///< A comma seperated list of translator plugins that should be inactive for export

  AL_USDMAYA_PUBLIC
  static const char* const compactionLevels[];

  AL_USDMAYA_PUBLIC
  static const char* const timelineLevel[];

  /// \brief  provide a method to specify the export options
  /// \param  options a set of options that are constructed and later used for option parsing
  /// \return MS::kSuccess if ok
  static MStatus specifyOptions(AL::maya::utils::FileTranslatorOptions& options)
  {
    ExporterParams defaultValues;
    if(!options.addFrame("AL USD Exporter Options")) return MS::kFailure;
    if(!options.addBool(kDynamicAttributes, defaultValues.m_dynamicAttributes)) return MS::kFailure;
    if(!options.addBool(kDuplicateInstances, defaultValues.m_duplicateInstances)) return MS::kFailure;
    if(!options.addBool(kMergeTransforms, defaultValues.m_mergeTransforms)) return MS::kFailure;
    if(!options.addBool(kAnimation, defaultValues.m_animation)) return MS::kFailure;
    if(!options.addBool(kUseTimelineRange, defaultValues.m_useTimelineRange)) return MS::kFailure;
    if(!options.addFloat(kFrameMin, defaultValues.m_minFrame)) return MS::kFailure;
    if(!options.addFloat(kFrameMax, defaultValues.m_maxFrame)) return MS::kFailure;
    if(!options.addInt(kSubSamples, defaultValues.m_subSamples)) return MS::kFailure;
    if(!options.addBool(kFilterSample, defaultValues.m_filterSample)) return MS::kFailure;
    if(!options.addEnum(kExportAtWhichTime, timelineLevel, defaultValues.m_exportAtWhichTime)) return MS::kFailure;
    if(!options.addBool(kExportInWorldSpace, defaultValues.m_exportInWorldSpace)) return MS::kFailure;
    if(!options.addBool(kActivateAllTranslators, true)) return MS::kFailure;
    if(!options.addString(kActiveTranslatorList, "")) return MS::kFailure;
    if(!options.addString(kInactiveTranslatorList, "")) return MS::kFailure;
    
    // register the export translator context
    PluginTranslatorOptionsContextManager::registerContext("ExportTranslator", &m_pluginContext);
 
    return MS::kSuccess;
  }

  /// \brief clean up the options registered for this translator
  /// \param  options a set of options that are constructed and later used for option parsing
  /// \return MS::kSuccess if ok
  static MStatus cleanupOptions(AL::maya::utils::FileTranslatorOptions& options)
  {
    if(!options.removeFrame("AL USD Exporter Options")) return MS::kFailure;

    // unregister the export translator context
    PluginTranslatorOptionsContextManager::unregisterContext("ExportTranslator");
    return MS::kSuccess;
  }

  void prepPluginOptions() override
  {
    // I need to possibly recreate this when dirty (i.e. when new optins have been registered/unregistered)
    if(m_pluginContext.dirty())
    {
      delete m_pluginInstance;
      m_pluginInstance = new PluginTranslatorOptionsInstance(m_pluginContext);
      setPluginOptionsContext(m_pluginInstance);
    }
  }

  static PluginTranslatorOptionsContext& pluginContext()
    { return m_pluginContext; }

private:
  AL_USDMAYA_PUBLIC
  static PluginTranslatorOptionsContext m_pluginContext;
  static PluginTranslatorOptions* m_compatPluginOptions;
  AL_USDMAYA_PUBLIC
  static PluginTranslatorOptionsInstance* m_pluginInstance;
  AL_USDMAYA_PUBLIC
  MStatus writer(const MFileObject& file, const AL::maya::utils::OptionsParser& options, FileAccessMode mode) override;

AL_MAYA_TRANSLATOR_END();

//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
