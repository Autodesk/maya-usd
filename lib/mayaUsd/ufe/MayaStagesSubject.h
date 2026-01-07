//
// Copyright 2023 Autodesk
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
#ifndef MAYAUSD_MAYASTAGESSUBJECT_H
#define MAYAUSD_MAYASTAGESSUBJECT_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/listeners/proxyShapeNotice.h>

#include <usdUfe/ufe/StagesSubject.h>

#include <pxr/base/tf/hash.h>

#include <maya/MCallbackIdArray.h>
#include <ufe/path.h>

#include <unordered_set>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Subject class to observe Maya scene.
/*!
        This class observes Maya file new/open, to register a USD observer on each
        stage the Maya scene contains.
 */
class MAYAUSD_CORE_PUBLIC MayaStagesSubject : public UsdUfe::StagesSubject
{
public:
    typedef PXR_NS::TfWeakPtr<MayaStagesSubject> Ptr;
    typedef PXR_NS::TfRefPtr<MayaStagesSubject>  RefPtr;

    //! Constructor
    MayaStagesSubject();

    //! Destructor
    ~MayaStagesSubject() override;

    //! Create the MayaStagesSubject.
    static MayaStagesSubject::RefPtr create();

protected:
    void stageChanged(
        PXR_NS::UsdNotice::ObjectsChanged const& notice,
        PXR_NS::UsdStageWeakPtr const&           sender) override;

    bool isInNewScene() const;
    void setInNewScene(bool b);

    void beforeOpen();

private:
    // Maya scene message callbacks
    static void beforeNewCallback(void* clientData);
    static void beforeOpenCallback(void* clientData);
    static void afterNewCallback(void* clientData);
    static void afterOpenCallback(void* clientData);

private:
    // Notice listener method for proxy stage set
    void onStageSet(const PXR_NS::MayaUsdProxyStageSetNotice& notice);

    void setupListeners();
    void clearListeners();

    // Notice listener method for proxy stage invalidate.
    void onStageInvalidate(const PXR_NS::MayaUsdProxyStageInvalidateNotice& notice);

    // Array of Notice::Key for registered listener
    using NoticeKeys = std::array<PXR_NS::TfNotice::Key, 2>;

    // Map of per-stage listeners, indexed by stage.
    typedef PXR_NS::TfHashMap<PXR_NS::UsdStageWeakPtr, NoticeKeys, PXR_NS::TfHash> StageListenerMap;
    StageListenerMap                                                               _stageListeners;

    /*! \brief  Store invalidated ufe paths during dirty propagation.

       We need to delay notification till stage changes, but at that time it could be too costly to
       discover what changed in the stage map. Instead, we store all gateway notes that changed
       during dirty propagation and send invalidation from compute, when the new stage is set. This
       cache is only useful between onStageInvalidate and onStageSet notifications.
    */
    std::unordered_set<Ufe::Path> _invalidStages;

    bool _isInNewScene = false;

    MCallbackIdArray _cbIds;

}; // MayaStagesSubject

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_MAYASTAGESSUBJECT_H
