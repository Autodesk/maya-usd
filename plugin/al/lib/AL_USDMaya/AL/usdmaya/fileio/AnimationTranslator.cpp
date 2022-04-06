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

#include "AL/usdmaya/fileio/AnimationTranslator.h"

#include "AL/usdmaya/fileio/translators/DgNodeTranslator.h"
#include "AL/usdmaya/fileio/translators/TransformTranslator.h"
#include "AL/usdmaya/utils/MeshUtils.h"

#include <maya/MAnimControl.h>
#include <maya/MAnimUtil.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnDagNode.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MMatrix.h>
#include <maya/MNodeClass.h>
#include <maya/MTypes.h> // For MAYA_APP_VERSION

namespace AL {
namespace usdmaya {
namespace fileio {

//----------------------------------------------------------------------------------------------------------------------
void AnimationTranslator::exportAnimation(const ExporterParams& params)
{
    auto const startAttrib = m_animatedPlugs.begin();
    auto const endAttrib = m_animatedPlugs.end();
    auto const startAttribScaled = m_scaledAnimatedPlugs.begin();
    auto const endAttribScaled = m_scaledAnimatedPlugs.end();
    auto const startTransformAttrib = m_animatedTransformPlugs.begin();
    auto const endTransformAttrib = m_animatedTransformPlugs.end();
    auto const startMultiAttrib = m_animatedMultiPlugs.begin();
    auto const endMultiAttrib = m_animatedMultiPlugs.end();
    auto const startMesh = m_animatedMeshes.begin();
    auto const endMesh = m_animatedMeshes.end();
    auto const startWSM = m_worldSpaceOutputs.begin();
    auto const endWSM = m_worldSpaceOutputs.end();

    if ((startAttrib != endAttrib) || (startAttribScaled != endAttribScaled)
        || (startTransformAttrib != endTransformAttrib) || (startMultiAttrib != endMultiAttrib)
        || (startMesh != endMesh) || (startWSM != endWSM) || (!m_animatedNodes.empty())) {
        double increment = 1.0 / std::max(1U, params.m_subSamples);
        for (double t = params.m_minFrame, e = params.m_maxFrame + 1e-3f; t < e; t += increment) {
            MAnimControl::setCurrentTime(t);
            UsdTimeCode timeCode(t);
            for (auto it = startAttrib; it != endAttrib; ++it) {
                /// \todo This feels wrong. Split the DgNodeTranslator class into 3 ...
                ///         maya::Dg
                ///         usdmaya::Dg
                ///         usdmaya::fileio::translator::Dg
#if MAYA_APP_VERSION > 2019
                translators::TransformTranslator::copyAttributeValue(
                    it->first, it->second, timeCode, params.m_mergeOffsetParentMatrix);
#else
                translators::DgNodeTranslator::copyAttributeValue(it->first, it->second, timeCode);
#endif
            }
            for (auto it = startAttribScaled; it != endAttribScaled; ++it) {
                /// \todo This feels wrong. Split the DgNodeTranslator class into 3 ...
                ///         maya::Dg
                ///         usdmaya::Dg
                ///         usdmaya::fileio::translator::Dg
#if MAYA_APP_VERSION > 2019
                translators::TransformTranslator::copyAttributeValue(
                    it->first,
                    it->second.attr,
                    it->second.scale,
                    timeCode,
                    params.m_mergeOffsetParentMatrix);
#else
                translators::DgNodeTranslator::copyAttributeValue(
                    it->first, it->second.attr, it->second.scale, timeCode);
#endif
            }
            for (auto it = startTransformAttrib; it != endTransformAttrib; ++it) {
                translators::TransformTranslator::copyAttributeValue(
                    it->first, it->second, timeCode);
            }
            for (auto it = startMultiAttrib; it != endMultiAttrib; ++it) {
                // Note: so far there is only one attribute need to be treated specially
                //       we do this special handling for this particular attribute atm,
                //       will see if we need to generalize once have more requests
                if (it->first.GetName() == UsdGeomTokens->clippingRange && it->second.size() == 2) {
                    const auto& plugs(it->second);
                    MDistance   nearDistance;
                    MDistance   farDistance;
                    if (plugs[0].getValue(nearDistance) == MStatus::kSuccess
                        && plugs[1].getValue(farDistance) == MStatus::kSuccess) {
                        GfVec2f clippingRange {
                            static_cast<float>(nearDistance.as(MDistance::kCentimeters)),
                            static_cast<float>(farDistance.as(MDistance::kCentimeters))
                        };
                        it->first.Set(clippingRange, timeCode);
                    }
                }
            }
            for (auto it = startMesh; it != endMesh; ++it) {
                UsdGeomMesh                           mesh(it->second.GetPrim());
                AL::usdmaya::utils::MeshExportContext context(it->first, mesh, timeCode);
                context.copyVertexData(timeCode);
            }
            for (auto nodeAnim : m_animatedNodes) {
                nodeAnim.m_translator->exportCustomAnim(nodeAnim.m_path, nodeAnim.m_prim, timeCode);
            }
            for (auto it = startWSM; it != endWSM; ++it) {
                MMatrix mat = it->first.inclusiveMatrix();
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
                it->second.Set(*(const GfMatrix4d*)&mat, timeCode);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
