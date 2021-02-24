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
#ifndef PXRUSDMAYA_CHASER_H
#define PXRUSDMAYA_CHASER_H

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/declarePtrs.h>
#include <pxr/base/tf/refPtr.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/timeCode.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdMayaExportChaser);

/// \brief base class for plugin chasers which are plugins that run after the
/// core usdExport out of maya.
///
/// Chaser objects will be constructed after the initial "unvarying" export.
/// Chasers should save off necessary data when they are constructed.
/// Afterwards, the chasers will be invoked to export Defaults.  For each frame,
/// after the core process the given frame, all the chasers will be invoked to
/// process that frame.
///
/// The key difference between these and the mel/python postScripts is that a
/// chaser can have direct access to the core usdExport context.
///
/// Chasers need to be very careful as to not modify the structure of the usd
/// file.  This should ideally be used to make small changes or to add
/// attributes in a non-destructive way.
class UsdMayaExportChaser : public TfRefBase
{
public:
    ~UsdMayaExportChaser() override { }

    /// Do custom processing after UsdMaya has exported data at the default
    /// time.
    /// The stage will be incomplete; any animated data will not have
    /// been exported yet.
    /// Returning false will terminate the whole export.
    MAYAUSD_CORE_PUBLIC
    virtual bool ExportDefault();

    /// Do custom processing after UsdMaya has exported data at \p time.
    /// The stage will be incomplete; any future animated frames will not
    /// have been exported yet.
    /// Returning false will terminate the whole export.
    MAYAUSD_CORE_PUBLIC
    virtual bool ExportFrame(const UsdTimeCode& time);

    /// Do custom post-processing that needs to run after the main UsdMaya
    /// export loop.
    /// At this point, all data has been authored to the stage (except for
    /// any custom data that you'll author in this step).
    /// Returning false will terminate the whole export.
    MAYAUSD_CORE_PUBLIC
    virtual bool PostExport();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
