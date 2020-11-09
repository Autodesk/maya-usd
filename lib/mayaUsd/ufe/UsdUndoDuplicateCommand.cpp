//
// Copyright 2019 Autodesk
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
#include "UsdUndoDuplicateCommand.h"

#include "private/UfeNotifGuard.h"
#include "private/Utils.h"

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsdUtils/util.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/log.h>
#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoDuplicateCommand::UsdUndoDuplicateCommand(
    const UsdPrim&   srcPrim,
    const Ufe::Path& ufeSrcPath)
    : Ufe::UndoableCommand()
    , _srcPrim(srcPrim)
    , _ufeSrcPath(ufeSrcPath)
{
    primInfo(srcPrim, _usdDstPath, _layer);
}

UsdUndoDuplicateCommand::~UsdUndoDuplicateCommand() { }

/*static*/
UsdUndoDuplicateCommand::Ptr
UsdUndoDuplicateCommand::create(const UsdPrim& srcPrim, const Ufe::Path& ufeSrcPath)
{
    return std::make_shared<UsdUndoDuplicateCommand>(srcPrim, ufeSrcPath);
}

const SdfPath& UsdUndoDuplicateCommand::usdDstPath() const { return _usdDstPath; }

/*static*/
void UsdUndoDuplicateCommand::primInfo(
    const UsdPrim&  srcPrim,
    SdfPath&        usdDstPath,
    SdfLayerHandle& srcLayer)
{
    ufe::applyCommandRestriction(srcPrim, "duplicate");

    auto parent = srcPrim.GetParent();
    auto dstName = uniqueChildName(parent, srcPrim.GetName());
    usdDstPath = parent.GetPath().AppendChild(TfToken(dstName));

    srcLayer = MayaUsdUtils::defPrimSpecLayer(srcPrim);
    if (!srcLayer) {
        std::string err
            = TfStringPrintf("No prim found at %s", srcPrim.GetPath().GetString().c_str());
        throw std::runtime_error(err.c_str());
    }
}

/*static*/
bool UsdUndoDuplicateCommand::duplicate(
    const SdfLayerHandle& layer,
    const SdfPath&        usdSrcPath,
    const SdfPath&        usdDstPath)
{
    // We use the source layer as the destination.  An alternate workflow
    // would be the edit target layer be the destination:
    // layer = self._stage.GetEditTarget().GetLayer()
    MayaUsd::ufe::InAddOrDeleteOperation ad;
    return SdfCopySpec(layer, usdSrcPath, layer, usdDstPath);
}

//------------------------------------------------------------------------------
// UsdUndoDuplicateCommand overrides
//------------------------------------------------------------------------------

void UsdUndoDuplicateCommand::undo()
{
    // USD sends a ResyncedPaths notification after the prim is removed, but
    // at that point the prim is no longer valid, and thus a UFE post delete
    // notification is no longer possible.  To respect UFE object delete
    // notification semantics, which require the object to be alive when
    // the notification is sent, we send a pre delete notification here.
    Ufe::ObjectPreDelete notification(
        createSiblingSceneItem(_ufeSrcPath, _usdDstPath.GetElementString()));

#ifdef UFE_V2_FEATURES_AVAILABLE
    Ufe::Scene::instance().notify(notification);
#else
    Ufe::Scene::notifyObjectDelete(notification);
#endif

    _srcPrim.GetStage()->RemovePrim(_usdDstPath);
}

void UsdUndoDuplicateCommand::redo()
{
    // MAYA-92264: Pixar bug prevents redo from working.  Try again with USD
    // version 0.8.5 or later.  PPT, 28-May-2018.
    try {
        duplicate(_layer, _srcPrim.GetPath(), _usdDstPath);
    } catch (const std::exception& e) {
        UFE_LOG(e.what());
        throw; // re-throw the same exception
    }
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
