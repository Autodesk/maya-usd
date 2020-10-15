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

#include <mayaUsd/ufe/Utils.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/log.h>
#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

#include <mayaUsdUtils/util.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoDuplicateCommand::UsdUndoDuplicateCommand(
    const UsdPrim&   srcPrim,
    const Ufe::Path& ufeSrcPath)
    : Ufe::UndoableCommand()
    , fSrcPrim(srcPrim)
    , fUfeSrcPath(ufeSrcPath)
{
    fStage = fSrcPrim.GetStage();
    primInfo(srcPrim, fUsdDstPath, fLayer);
}

UsdUndoDuplicateCommand::~UsdUndoDuplicateCommand() { }

/*static*/
UsdUndoDuplicateCommand::Ptr
UsdUndoDuplicateCommand::create(const UsdPrim& srcPrim, const Ufe::Path& ufeSrcPath)
{
    return std::make_shared<UsdUndoDuplicateCommand>(srcPrim, ufeSrcPath);
}

const SdfPath& UsdUndoDuplicateCommand::usdDstPath() const { return fUsdDstPath; }

/*static*/
void UsdUndoDuplicateCommand::primInfo(
    const UsdPrim&  srcPrim,
    SdfPath&        usdDstPath,
    SdfLayerHandle& srcLayer)
{
    auto             parent = srcPrim.GetParent();
    TfToken::HashSet childrenNames;
    for (auto child : parent.GetFilteredChildren(UsdPrimIsDefined && !UsdPrimIsAbstract)) {
        childrenNames.insert(child.GetName());
    }

    // Find a unique name for the destination.  If the source name already
    // has a numerical suffix, increment it, otherwise append "1" to it.
    auto dstName = uniqueName(childrenNames, srcPrim.GetName());
    usdDstPath = parent.GetPath().AppendChild(TfToken(dstName));

    // Iterate over the layer stack, starting at the highest-priority layer.
    // The source layer is the one in which there exists a def primSpec, not
    // an over.  An alternative would have beeen to call Sdf.CopySpec for
    // each layer in which there is an over or a def, until we reach the
    // layer with a def primSpec.  This would preserve the visual appearance
    // of the duplicate.  PPT, 12-Jun-2018.
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
        createSiblingSceneItem(fUfeSrcPath, fUsdDstPath.GetElementString()));

#ifdef UFE_V2_FEATURES_AVAILABLE
    Ufe::Scene::instance().notify(notification);
#else
    Ufe::Scene::notifyObjectDelete(notification);
#endif

    fStage->RemovePrim(fUsdDstPath);
}

void UsdUndoDuplicateCommand::redo()
{
    // MAYA-92264: Pixar bug prevents redo from working.  Try again with USD
    // version 0.8.5 or later.  PPT, 28-May-2018.
    try {
        duplicate(fLayer, fSrcPrim.GetPath(), fUsdDstPath);
    } catch (const std::exception& e) {
        UFE_LOG(e.what());
        throw; // re-throw the same exception
    }
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
