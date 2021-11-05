//
// Copyright 2021 Autodesk
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
#ifndef PXRUSDMAYA_MAYAPRIMUPDATER_MANAGER_H
#define PXRUSDMAYA_MAYAPRIMUPDATER_MANAGER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primUpdaterContext.h>
#include <mayaUsd/listeners/proxyShapeNotice.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/registryManager.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MCallbackIdArray.h>
#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <ufe/path.h>
#include <ufe/sceneItem.h>

PXR_NAMESPACE_OPEN_SCOPE

class PrimUpdaterManager : public PXR_NS::TfWeakBase
{
public:
    MAYAUSD_CORE_PUBLIC
    ~PrimUpdaterManager();

    MAYAUSD_CORE_PUBLIC
    bool push(const MFnDependencyNode& depNodeFn, const Ufe::Path& pulledPath);

    MAYAUSD_CORE_PUBLIC
    bool pull(const Ufe::Path& path);

    MAYAUSD_CORE_PUBLIC
    bool discardEdits(const Ufe::Path& path);

    MAYAUSD_CORE_PUBLIC
    bool copyBetween(const Ufe::Path& srcPath, const Ufe::Path& dstPath);

    /// \brief Returns the singleton prim updater manager
    MAYAUSD_CORE_PUBLIC
    static PrimUpdaterManager& getInstance();

    MAYAUSD_CORE_PUBLIC
    static bool readPullInformation(const PXR_NS::UsdPrim& prim, std::string& dagPathStr);
    MAYAUSD_CORE_PUBLIC
    static bool readPullInformation(const PXR_NS::UsdPrim& prim, Ufe::SceneItem::Ptr& dagPathItem);
    MAYAUSD_CORE_PUBLIC
    static bool readPullInformation(const Ufe::Path& ufePath, MDagPath& dagPath);
    MAYAUSD_CORE_PUBLIC
    static bool readPullInformation(const MDagPath& dagpath, Ufe::Path& ufePath);

private:
    PrimUpdaterManager();

    void onProxyContentChanged(const MayaUsdProxyStageObjectsChangedNotice& notice);

    //! Ensure the Dag pull root exists.  This is the child of the Maya world
    //! node under which all pulled nodes are created.
    bool findOrCreatePullRoot();

    //! Create the pull parent for the pulled hierarchy.  This is the node
    //! which receives the pulled node's parent transformation.
    MObject createPullParent(const Ufe::Path& pulledPath);

    //! Remove the pull parent for the pulled hierarchy.
    bool removePullParent(const MDagPath& pullParent);

    //! Create the pull parent and set it into the prim updater context.
    MDagPath setupPullParent(const Ufe::Path& pulledPath, VtDictionary& args);

    //! Maya scene message callback.
    static void beforeNewOrOpenCallback(void* clientData);

    friend class TfSingleton<PrimUpdaterManager>;

    bool _inPushPull { false };

    // Initialize pull root MObject to null object.
    MObject _pullRoot {};

    // Reset pull root on file new / file open.
    MCallbackIdArray _cbIds;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
