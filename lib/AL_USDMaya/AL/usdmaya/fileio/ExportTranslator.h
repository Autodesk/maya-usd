//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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
#include "AL/maya/utils/FileTranslatorBase.h"

namespace AL {
namespace usdmaya {
namespace fileio {

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
  static constexpr const char* const kMeshes = "Meshes"; ///< export mesh geometry option name
  static constexpr const char* const kNurbsCurves = "Nurbs Curves"; ///< export nurbs curves option name
  static constexpr const char* const kDuplicateInstances = "Duplicate Instances"; ///< export instances option name
  static constexpr const char* const kUseAnimalSchema = "Use Animal Schema"; ///< export using animal schema option name
  static constexpr const char* const kMergeTransforms = "Merge Transforms"; ///< export by merging transforms and shapes option name
  static constexpr const char* const kAnimation = "Animation"; ///< export animation data option name
  static constexpr const char* const kUseTimelineRange = "Use Timeline Range"; ///< export using the timeline range option name
  static constexpr const char* const kFrameMin = "Frame Min"; ///< specify min time frame option name
  static constexpr const char* const kFrameMax = "Frame Max"; ///< specify max time frame option name
  static constexpr const char* const kFilterSample = "Filter Sample"; ///< export filter sample option name

  /// \brief  provide a method to specify the export options
  /// \param  options a set of options that are constructed and later used for option parsing
  /// \return MS::kSuccess if ok
  static MStatus specifyOptions(AL::maya::utils::FileTranslatorOptions& options)
  {
    ExporterParams defaultValues;
    if(!options.addFrame("AL USD Exporter Options")) return MS::kFailure;
    if(!options.addBool(kDynamicAttributes, defaultValues.m_dynamicAttributes)) return MS::kFailure;
    if(!options.addBool(kMeshes, defaultValues.m_meshes)) return MS::kFailure;
    if(!options.addBool(kNurbsCurves, defaultValues.m_nurbsCurves)) return MS::kFailure;
    if(!options.addBool(kDuplicateInstances, defaultValues.m_duplicateInstances)) return MS::kFailure;
    if(!options.addBool(kUseAnimalSchema, defaultValues.m_useAnimalSchema)) return MS::kFailure;
    if(!options.addBool(kMergeTransforms, defaultValues.m_mergeTransforms)) return MS::kFailure;
    if(!options.addBool(kAnimation, defaultValues.m_animation)) return MS::kFailure;
    if(!options.addBool(kUseTimelineRange, defaultValues.m_useTimelineRange)) return MS::kFailure;
    if(!options.addFloat(kFrameMin, defaultValues.m_minFrame)) return MS::kFailure;
    if(!options.addFloat(kFrameMax, defaultValues.m_maxFrame)) return MS::kFailure;
    if(!options.addBool(kFilterSample, defaultValues.m_filterSample)) return MS::kFailure;
    return MS::kSuccess;
  }

private:
  MStatus writer(const MFileObject& file, const AL::maya::utils::OptionsParser& options, FileAccessMode mode);

AL_MAYA_TRANSLATOR_END();

//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
