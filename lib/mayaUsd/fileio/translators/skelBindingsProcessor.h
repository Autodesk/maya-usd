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
#ifndef PXRUSDMAYA_SKEL_BINDINGS_PROCESSOR_H
#define PXRUSDMAYA_SKEL_BINDINGS_PROCESSOR_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/utils/util.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/pathTable.h>

#include <maya/MDagPath.h>

#include <set>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

/// This class encapsulates all of the logic for writing or modifying
/// SkelRoot prims for all scopes that have skel bindings.
class UsdMaya_SkelBindingsProcessor
{
public:
    UsdMaya_SkelBindingsProcessor();

    /// Mark \p path as containing bindings utilizing the skeleton
    /// at \p skelPath.
    /// Bindings are marked so that SkelRoots may be post-processed.
    /// Valid values for \p config are:
    /// - UsdMayaJobExportArgsTokens->explicit_: search for an existing SkelRoot
    /// - UsdMayaJobExportArgsTokens->auto_: create a SkelRoot if needed
    /// UsdMayaJobExportArgsTokens->none is not valid for \p config; it will
    /// mark an invalid binding.
    void MarkBindings(const SdfPath& path, const SdfPath& skelPath, const TfToken& config);

    /// Performs final processing for skel bindings.
    bool PostProcessSkelBindings(const UsdStagePtr& stage) const;

private:
    bool _VerifyOrMakeSkelRoots(const UsdStagePtr& stage) const;

    using _Entry = std::pair<SdfPath, TfToken>;

    std::unordered_map<SdfPath, _Entry, SdfPath::Hash> _bindingToSkelMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
