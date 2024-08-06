//
// Copyright 2022 Autodesk
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
#ifndef MAYAUSD_PROXYSHAPESCENESEGMENTHANDLER_H
#define MAYAUSD_PROXYSHAPESCENESEGMENTHANDLER_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UfeVersionCompat.h>

#include <ufe/sceneSegmentHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Maya run-time scene segment handler with support for USD gateway node.
/*!
    This scene segment handler is NOT a USD run-time scene segment handler: it is a
    Maya run-time scene segment handler.  It decorates the standard Maya run-time
    scene segment handler and augments it.
 */
class MAYAUSD_CORE_PUBLIC ProxyShapeSceneSegmentHandler : public Ufe::SceneSegmentHandler
{
public:
    typedef std::shared_ptr<ProxyShapeSceneSegmentHandler> Ptr;

    ProxyShapeSceneSegmentHandler(const Ufe::SceneSegmentHandler::Ptr& mayaSceneSegmentHandler);

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(ProxyShapeSceneSegmentHandler);

    //! Create a ProxyShapeSceneSegmentHandler from a UFE scene segment handler.
    static ProxyShapeSceneSegmentHandler::Ptr
    create(const Ufe::SceneSegmentHandler::Ptr& mayaSceneSegmentHandler);

    // Ufe::SceneSegmentHandler overrides
    Ufe::Selection findGatewayItems_(const Ufe::Path& path) const override;
    UFE_V4(Ufe::Selection findGatewayItems_(const Ufe::Path& path, Ufe::Rtid nestedRtid)
               const override;)
    bool isGateway_(const Ufe::Path& path) const override;

private:
    Ufe::SceneSegmentHandler::Ptr _mayaSceneSegmentHandler;

}; // ProxyShapeSceneSegmentHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_PROXYSHAPESCENESEGMENTHANDLER_H
