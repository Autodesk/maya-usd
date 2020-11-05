//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYA_BLOCK_SCENE_MODIFICATION_CONTEXT_H
#define PXRUSDMAYA_BLOCK_SCENE_MODIFICATION_CONTEXT_H

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Utility class for wrapping a scope of Maya operations such that the
/// modification status of the Maya scene is preserved.
class UsdMayaBlockSceneModificationContext
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMayaBlockSceneModificationContext();

    MAYAUSD_CORE_PUBLIC
    virtual ~UsdMayaBlockSceneModificationContext();

private:
    /// Modification status of the scene when the context was created.
    bool _sceneWasModified;

    UsdMayaBlockSceneModificationContext(const UsdMayaBlockSceneModificationContext&) = delete;
    UsdMayaBlockSceneModificationContext& operator=(const UsdMayaBlockSceneModificationContext&)
        = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
