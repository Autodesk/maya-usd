//
// Copyright 2016 Pixar
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
#ifndef PXRUSDTRANSLATORS_MAYAREFERENCE_UPDATER_H
#define PXRUSDTRANSLATORS_MAYAREFERENCE_UPDATER_H

#include <mayaUsd/fileio/primUpdater.h>

#include <pxr/pxr.h>

#include <maya/MFnDependencyNode.h>

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;

/// Pull & Push support for MayaReference
class PxrUsdTranslators_MayaReferenceUpdater : public UsdMayaPrimUpdater
{
public:
    PxrUsdTranslators_MayaReferenceUpdater(
        const UsdMayaPrimUpdaterContext& context,
        const MFnDependencyNode&         depNodeFn,
        const Ufe::Path&                 path);

    // Behavior of discardEdits() is still T.B.D.  PPT, 6-Dec-2021.
    MAYAUSD_CORE_PUBLIC
    bool discardEdits() override;

    // Unload the Maya reference.
    MAYAUSD_CORE_PUBLIC
    bool pushEnd() override;

    /// Only auto-pull when Maya Reference path is set and it is explicitly requested via an
    /// attribute on a prim
    MAYAUSD_CORE_PUBLIC
    bool shouldAutoEdit() const override;

    /// Query to determine if the prim corresponding to this updater can be
    /// edited as Maya.  The implementation in this class returns true.
    MAYAUSD_CORE_PUBLIC
    bool canEditAsMaya() const override;

protected:
    MAYAUSD_CORE_PUBLIC
    PushCopySpecs pushCopySpecs(
        UsdStageRefPtr srcStage,
        SdfLayerRefPtr srcLayer,
        const SdfPath& srcSdfPath,
        UsdStageRefPtr dtsStage,
        SdfLayerRefPtr dstLayer,
        const SdfPath& dstSdfPath) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
