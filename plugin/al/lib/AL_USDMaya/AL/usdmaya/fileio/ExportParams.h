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

#include <AL/usdmaya/ForwardDeclares.h>
#include <mayaUsdUtils/ForwardDeclares.h>

#include <pxr/usd/usd/timeCode.h>

#include <maya/MSelectionList.h>
#include <maya/MStringArray.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace fileio {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  parameters for the exporter. These parameters are constructed by any command or file
/// translator that wishes
///         to export data from maya, which are then passed to the AL::usdmaya::fileio::Export class
///         to perform the actual export work.
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
struct ExporterParams
{
    maya::utils::OptionsParser* m_parser = 0;
    MSelectionList              m_nodes;          ///< the selected node to be exported
    MString                     m_fileName;       ///< the filename of the file we will be exporting
    double                      m_minFrame = 0.0; ///< the start frame for the animation export
    double                      m_maxFrame = 1.0; ///< the end frame of the animation export
    uint32_t                    m_subSamples = 1; ///< the number of subsample steps to export
    bool m_selected = false; ///< are we exporting selected objects (true) or all objects (false)
    bool m_dynamicAttributes
        = true; ///< if true export any dynamic attributes found on the nodes we are exporting
    bool m_duplicateInstances = true; ///< if true, instances will be exported as duplicates. As of
                                      ///< 23/01/17, nothing will be exported if set to false.
    bool m_mergeTransforms
        = true; ///< if true, shapes will be merged into their parent transforms in the exported
                ///< data. If false, the transform and shape will be exported seperately
    bool m_mergeOffsetParentMatrix
        = false; ///< if true, offset parent matrix would be merged to produce local space matrix;
                 ///< if false, offset parent matrix would be exported separately in USD.
    bool m_animation = false;        ///< if true, animation will be exported.
    bool m_useTimelineRange = false; ///< if true, then the export uses Maya's timeline range.
    bool m_filterSample = false; ///< if true, duplicate sample of attribute will be filtered out
    bool m_exportInWorldSpace = false; ///< if true, transform will be baked at the root prim,
                                       ///< children under the root will be untouched.
    AnimationTranslator* m_animTranslator
        = 0; ///< the animation translator to help exporting the animation data
    bool m_extensiveAnimationCheck
        = true; ///< if true, extensive animation check will be performed on transform nodes.
    int m_exportAtWhichTime = 0; ///< controls where the data will be written to: 0 = default time,
                                 ///< 1 = earliest time, 2 = current time
    UsdTimeCode m_timeCode = UsdTimeCode::Default();

    bool          m_activateAllTranslators = true;
    TfTokenVector m_activePluginTranslators;
    TfTokenVector m_inactivePluginTranslators;

    /// \brief  Given the text name of an option, returns the boolean value for that option.
    /// \param  str the name of the option
    /// \return the option value
    bool getBool(const char* const str) const
    {
        if (m_parser)
            return m_parser->getBool(str);
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

    /// \brief  Sets the value of the specified option, to the specified value
    /// \param  str the name of the option to set
    /// \param  value the option value
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
