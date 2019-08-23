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
#ifndef PXRUSDMAYAGL_SCENE_DELEGATE_H
#define PXRUSDMAYAGL_SCENE_DELEGATE_H

/// \file pxrUsdMayaGL/sceneDelegate.h

#include "pxr/pxr.h"

#include "../../base/api.h"
#include "./renderParams.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/usd/sdf/path.h"

#include <maya/MDrawContext.h>

#include <memory>
#include <unordered_map>


PXR_NAMESPACE_OPEN_SCOPE

struct PxrMayaHdPrimFilter {
    HdRprimCollection collection;
    TfTokenVector     renderTags;
};

using PxrMayaHdPrimFilterVector = std::vector<PxrMayaHdPrimFilter>;

class PxrMayaHdSceneDelegate : public HdSceneDelegate
{
    public:
        MAYAUSD_CORE_PUBLIC
        PxrMayaHdSceneDelegate(
                HdRenderIndex* renderIndex,
                const SdfPath& delegateID);

        // HdSceneDelegate interface
        MAYAUSD_CORE_PUBLIC
        VtValue Get(const SdfPath& id, const TfToken& key) override;

        MAYAUSD_CORE_PUBLIC
        VtValue GetCameraParamValue(
                SdfPath const& cameraId,
                TfToken const& paramName) override;

        MAYAUSD_CORE_PUBLIC
        TfTokenVector GetTaskRenderTags(SdfPath const& taskId) override;

        MAYAUSD_CORE_PUBLIC
        void SetCameraState(
                const GfMatrix4d& worldToViewMatrix,
                const GfMatrix4d& projectionMatrix,
                const GfVec4d& viewport);

        // VP 1.0 only.
        MAYAUSD_CORE_PUBLIC
        void SetLightingStateFromVP1(
                const GfMatrix4d& worldToViewMatrix,
                const GfMatrix4d& projectionMatrix);

        // VP 2.0 only.
        MAYAUSD_CORE_PUBLIC
        void SetLightingStateFromMayaDrawContext(
                const MHWRender::MDrawContext& context);

        MAYAUSD_CORE_PUBLIC
        HdTaskSharedPtrVector GetSetupTasks();

        MAYAUSD_CORE_PUBLIC
        HdTaskSharedPtrVector GetRenderTasks(
                const size_t hash,
                const PxrMayaHdRenderParams& renderParams,
                const PxrMayaHdPrimFilterVector& primFilters);

        MAYAUSD_CORE_PUBLIC
        HdTaskSharedPtrVector GetPickingTasks(
                const TfTokenVector& renderTags);

    protected:
        MAYAUSD_CORE_PUBLIC
        void _SetLightingStateFromLightingContext();

        template <typename T>
        const T& _GetValue(const SdfPath& id, const TfToken& key) {
            VtValue vParams = _valueCacheMap[id][key];
            if (!TF_VERIFY(vParams.IsHolding<T>(),
                           "For Id = %s, Key = %s",
                           id.GetText(), key.GetText())) {
                static T ERROR_VALUE;
                return ERROR_VALUE;
            }
            return vParams.UncheckedGet<T>();
        }

        template <typename T>
        void _SetValue(const SdfPath& id, const TfToken& key, const T& value) {
            _valueCacheMap[id][key] = value;
        }

    private:
        SdfPath _rootId;

        SdfPath _cameraId;
        GfVec4d _viewport;

        SdfPath _simpleLightTaskId;
        SdfPathVector _lightIds;
        GlfSimpleLightingContextRefPtr _lightingContext;

        SdfPath _shadowTaskId;

        // XXX: While this is correct, that we are using
        // hash in forming the task id, so the map is valid.
        // It is possible for the hash to collide, so the id
        // formed from the combination of hash and collection name is not
        // necessarily unique.
        struct _RenderTaskIdMapKey
        {
            size_t                hash;
            TfToken               collectionName;

            struct HashFunctor {
                size_t operator()(const  _RenderTaskIdMapKey& value) const;
            };

            bool operator==(const  _RenderTaskIdMapKey& other) const;
        };

        typedef std::unordered_map<
                _RenderTaskIdMapKey,
                SdfPath,
                _RenderTaskIdMapKey::HashFunctor> _RenderTaskIdMap;

        typedef std::unordered_map<size_t, SdfPath> _RenderParamTaskIdMap;


       
        _RenderParamTaskIdMap _renderSetupTaskIdMap;
        _RenderTaskIdMap      _renderTaskIdMap;
        _RenderParamTaskIdMap _selectionTaskIdMap;

		SdfPath _pickingTaskId;

        typedef TfHashMap<TfToken, VtValue, TfToken::HashFunctor> _ValueCache;
        typedef TfHashMap<SdfPath, _ValueCache, SdfPath::Hash> _ValueCacheMap;
        _ValueCacheMap _valueCacheMap;
};

typedef std::shared_ptr<PxrMayaHdSceneDelegate> PxrMayaHdSceneDelegateSharedPtr;


PXR_NAMESPACE_CLOSE_SCOPE


#endif
