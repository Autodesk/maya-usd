//
// Copyright 2023 Autodesk, Inc. All rights reserved.
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

#ifndef MAYAHYDRALIB_INTERFACE_H
#define MAYAHYDRALIB_INTERFACE_H

#include <mayaHydraLib/api.h>

#include <pxr/imaging/hd/sceneIndex.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

using SceneIndicesVector = std::vector<HdSceneIndexBasePtr>;

/// In order to access this interface, call the function GetMayaHydraLibInterface()
class MayaHydraLibInterface
{
public:
    /**
     * @brief Register a terminal scene index
     *
     * @usage Register a terminal scene index into the Hydra plugin.
     *
     * @param[in] sceneIndex is a pointer to the SceneIndex to be registered.
     */
    virtual void RegisterTerminalSceneIndex(HdSceneIndexBasePtr sceneIndex) = 0;

    /**
     * @brief Unregister a terminal scene index
     *
     * @usage Unregister a terminal scene index from the Hydra plugin.
     *
     * @param[in] sceneIndex is a pointer to the SceneIndex to be unregistered.
     */
    virtual void UnregisterTerminalSceneIndex(HdSceneIndexBasePtr sceneIndex) = 0;

    /**
     * @brief Clear the list of registered terminal scene indices
     *
     * @usage Clear the list of registered terminal scene indices. This does not delete them, but
     * just unregisters them.
     */
    virtual void ClearTerminalSceneIndices() = 0;

    /**
     * @brief Retrieve the list of registered terminal scene indices
     *
     * @usage Retrieve the list of registered terminal scene indices from the Hydra plugin.
     *
     * @return A reference to the vector of registered terminal scene indices.
     */
    virtual const SceneIndicesVector& GetTerminalSceneIndices() const = 0;
};

/// Access the MayaHydraLibInterface instance
MAYAHYDRALIB_API
MayaHydraLibInterface& GetMayaHydraLibInterface();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_INTERFACE_H
