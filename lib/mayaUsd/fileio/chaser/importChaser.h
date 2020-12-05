//
// Copyright 2021 Apple
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
#ifndef IMPORTCHASER_H
#define IMPORTCHASER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>

#include <pxr/base/tf/declarePtrs.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/primFlags.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MDagPathArray.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdMayaImportChaser);

/// \brief base class for plugin import chasers which are plugins that run after the
/// core `mayaUSDImport` functionality.
///
/// Import chaser objects will be constructed after the import operation has finished.
/// Chasers should save off necessary data when they are constructed.
/// Afterwards, the chasers will be invoked in order of their registration.
///
/// The key difference between these and the mel/python postScripts is that a
/// chaser can have direct access to the core `mayaUSDImport` context.
///
/// Chasers need to be very careful that they do not conflict with each other and end up
/// creating cycles or other undesirable setups in the DG.
class UsdMayaImportChaser : public TfRefBase
{
public:
    virtual ~UsdMayaImportChaser() override { }

    MAYAUSD_CORE_PUBLIC
    virtual bool PostImport(
        Usd_PrimFlagsPredicate&     returnPredicate,
        const UsdStagePtr&          stage,
        const MDagPathArray&        dagPaths,
        const SdfPathVector&        sdfPaths,
        const UsdMayaJobImportArgs& jobArgs);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* IMPORTCHASER_H */
