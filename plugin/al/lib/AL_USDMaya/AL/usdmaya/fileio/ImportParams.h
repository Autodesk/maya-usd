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
#include "AL/maya/utils/FileTranslatorOptions.h"

#include <pxr/usd/sdf/layer.h>

#include <maya/MDagPath.h>
#include <maya/MObjectArray.h>
#include <maya/MString.h>

#include <mayaUsdUtils/ForwardDeclares.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace fileio {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  parameters for the importer
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
struct ImporterParams
{
    AL::maya::utils::OptionsParser* m_parser = 0;
    MDagPath m_parentPath; ///< the parent transform under which the USD file will be imported
    MString  m_primPath;   ///< the prim path which importing will start from
    MString  m_fileName;   ///< the name of the file to import
    bool     m_animations
        = true; ///< true to import animation data, false to ignore animation data import
    bool m_dynamicAttributes = true; ///< if true, attributes in the USD file marked as 'custom'
                                     ///< will be imported as dynamic attributes.
    bool m_stageUnloaded
        = true; ///< if true, the USD stage will be opened with the UsdStage::LoadNone flag. If
                ///< false the stage will be loaded with the UsdStage::LoadAll flag
    bool           m_forceDefaultRead = false; ///< true to explicit read default values
    SdfLayerRefPtr m_rootLayer;                ///< \todo  Remove?
    SdfLayerRefPtr m_sessionLayer;             ///< \todo  Remove?

    bool          m_activateAllTranslators = true;
    TfTokenVector m_activePluginTranslators;
    TfTokenVector m_inactivePluginTranslators;

    mutable MObjectArray
        m_newAnimCurves; ///< to contain the possible created new animCurves for future management.

    /// \brief  Given the text name of an option, returns the boolean value for that option.
    /// \param  str the name of the option
    /// \return the option value
    bool getBool(const char* const str) const
    {
        if (m_parser) {
            return m_parser->getBool(str);
        }
        return false;
    }

    /// \brief  Given the text name of an option, returns the integer value for that option.
    /// \param  str the name of the option
    /// \return the option value
    int getInt(const char* const str) const
    {
        if (m_parser)
            return m_parser->getInt(str);
        return 0;
    }

    /// \brief  Given the text name of an option, returns the floating point value for that option.
    /// \param  str the name of the option
    /// \return the option value
    float getFloat(const char* const str) const
    {
        if (m_parser)
            return m_parser->getFloat(str);
        return 0.0f;
    }

    /// \brief  Given the text name of an option, returns the string value for that option.
    /// \param  str the name of the option
    /// \return the option value
    MString getString(const char* const str) const
    {
        if (m_parser)
            return m_parser->getString(str);
        return MString();
    }

    /// \brief  Sets the value of a boolean option
    /// \param  str the name of the option to set
    /// \param  value the new value for the option
    void setBool(const char* const str, bool value)
    {
        if (m_parser)
            m_parser->setBool(str, value);
    }
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
