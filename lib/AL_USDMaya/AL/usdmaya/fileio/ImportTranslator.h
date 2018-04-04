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
#include "AL/usdmaya/fileio/ImportParams.h"

namespace AL {
namespace usdmaya {
namespace fileio {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A USD importer into Maya (partially supporting Animal Logic specific things)
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_TRANSLATOR_BEGIN(ImportTranslator, "AL usdmaya import", true, false, "*.usda", "*.usdc;*.usda;*.usd;*.usdt");

  /// \fn     void* creator()
  /// \brief  creates an instance of the file translator
  /// \return a new file translator instance

  /// \var    const char* const kTranslatorName
  /// \brief  the name of the file translator

  /// \var    const char* const kClassName
  /// \brief  the name of the C++ file translator class

  // specify the option names (These will uniquely identify the exporter options)
  static constexpr const char* const kParentPath = "Parent Path"; ///< the parent path option name
  static constexpr const char* const kMeshes = "Import Meshes"; ///< the import meshes option name
  static constexpr const char* const kNurbsCurves = "Import Curves"; ///< the import curves option name
  static constexpr const char* const kAnimations = "Import Animations"; ///< the import animation option name
  static constexpr const char* const kDynamicAttributes = "Import Dynamic Attributes"; ///< the import dynamic attributes option name

  /// \brief  provide a method to specify the import options
  /// \param  options a set of options that are constructed and later used for option parsing
  /// \return MS::kSuccess if ok
  static MStatus specifyOptions(AL::maya::utils::FileTranslatorOptions& options)
  {
    if(!options.addFrame("AL USD Importer Options")) return MS::kFailure;
      if(!options.addString(kParentPath, "")) return MS::kFailure;
      if(!options.addBool(kMeshes, true)) return MS::kFailure;
      if(!options.addBool(kNurbsCurves, true)) return MS::kFailure;
      if(!options.addBool(kAnimations, true)) return MS::kFailure;
      if(!options.addBool(kDynamicAttributes, true)) return MS::kFailure;
    return MS::kSuccess;
  }

private:
  MStatus reader(const MFileObject& file, const AL::maya::utils::OptionsParser& options, FileAccessMode mode);
  ImporterParams m_params;

AL_MAYA_TRANSLATOR_END();

//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------

