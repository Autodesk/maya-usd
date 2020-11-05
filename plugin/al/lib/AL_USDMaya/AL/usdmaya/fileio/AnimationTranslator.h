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

#include <AL/maya/utils/Utils.h>
#include <AL/usdmaya/Api.h>
#include <AL/usdmaya/fileio/translators/TranslatorBase.h>
#include <AL/usdmaya/utils/AnimationTranslator.h>

#include <pxr/usd/usd/attribute.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace fileio {

using usdmaya::utils::AnimationCheckTransformAttributes;
using usdmaya::utils::MeshAttrVector;
using usdmaya::utils::PlugAttrScaledVector;
using usdmaya::utils::PlugAttrVector;
using usdmaya::utils::ScaledPair;
using usdmaya::utils::WorldSpaceAttrVector;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A utility class to help with exporting animated plugs from maya
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
class AnimationTranslator : public usdmaya::utils::AnimationTranslator
{
public:
    /// \brief  After the scene has been exported, call this method to export the animation data on
    /// various attributes \param  params the export options
    AL_USDMAYA_PUBLIC
    void exportAnimation(const ExporterParams& params);

    /// \brief  insert a prim into the anim translator for custom anim export.
    /// \param  translator the plugin translator to handle the export of anim data for the node
    /// \param  dagPath the maya dag path for the maya object to export
    /// \param  usdPrim the prim to write the data into
    inline void
    addCustomAnimNode(translators::TranslatorBase* translator, MDagPath dagPath, UsdPrim usdPrim)
    {
        m_animatedNodes.push_back({ translator, dagPath, usdPrim });
    }

private:
    struct NodeExportInfo
    {
        translators::TranslatorBase* m_translator;
        MDagPath                     m_path;
        UsdPrim                      m_prim;
    };
    std::vector<NodeExportInfo> m_animatedNodes;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
