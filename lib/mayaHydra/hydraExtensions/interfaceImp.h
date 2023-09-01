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

#ifndef MAYAHYDRALIB_INTERFACE_IMP_H
#define MAYAHYDRALIB_INTERFACE_IMP_H

#include <mayaHydraLib/api.h>
#include <mayaHydraLib/interface.h>

#include <pxr/imaging/hd/sceneIndex.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class MayaHydraLibInterfaceImp : public MayaHydraLibInterface
{
public:
    MayaHydraLibInterfaceImp() = default;
    virtual ~MayaHydraLibInterfaceImp() = default;

    void                      RegisterTerminalSceneIndex(HdSceneIndexBasePtr) override;
    void                      UnregisterTerminalSceneIndex(HdSceneIndexBasePtr) override;
    void                      ClearTerminalSceneIndices() override;
    const SceneIndicesVector& GetTerminalSceneIndices() const override;

private:
    SceneIndicesVector _sceneIndices;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_INTERFACE_IMP_H
