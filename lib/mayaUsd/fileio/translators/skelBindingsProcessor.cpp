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
#include "skelBindingsProcessor.h"

#include <mayaUsd/fileio/translators/translatorUtil.h>

#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdSkel/root.h>
#include <pxr/usd/usdUtils/authoring.h>

PXR_NAMESPACE_OPEN_SCOPE

UsdMaya_SkelBindingsProcessor::UsdMaya_SkelBindingsProcessor() { }

/// Finds the rootmost ancestor of the prim at \p path that is an Xform
/// or SkelRoot type prim. The result may be the prim itself.
static UsdPrim _FindRootmostXformOrSkelRoot(const UsdStagePtr& stage, const SdfPath& path)
{
    UsdPrim currentPrim = stage->GetPrimAtPath(path);
    UsdPrim rootmost;
    while (currentPrim) {
        if (currentPrim.IsA<UsdGeomXform>()) {
            rootmost = currentPrim;
        } else if (currentPrim.IsA<UsdSkelRoot>()) {
            rootmost = currentPrim;
        }
        currentPrim = currentPrim.GetParent();
    }

    return rootmost;
}

/// Finds the existing SkelRoot which is shared by all \p paths.
/// If no SkelRoot is found, and \p config is "auto", then attempts to
/// find a common ancestor of \p paths which can be converted to SkelRoot.
/// \p outMadeSkelRoot must be non-null; it will be set to indicate whether
/// any auto-typename change actually occurred (true) or whether there was
/// already a SkelRoot, so no renaming was necessary (false).
/// If an existing, common SkelRoot cannot be found for all paths, and if
/// it's not possible to create one, returns an empty SdfPath.
static SdfPath _VerifyOrMakeSkelRoot(
    const UsdStagePtr& stage,
    const SdfPath&     path,
    const TfToken&     config,
    const SdfPath&     rootPrimPath)
{
    if (config != UsdMayaJobExportArgsTokens->auto_
        && config != UsdMayaJobExportArgsTokens->explicit_) {
        return SdfPath();
    }

    // Only try to auto-rename to SkelRoot if we're not already a
    // descendant of one. Otherwise, verify that the user tagged it in a sane
    // way.

    if (UsdSkelRoot root = UsdSkelRoot::Find(stage->GetPrimAtPath(path))) {

        // Verify that the SkelRoot isn't nested in another SkelRoot.
        // This is necessary because UsdSkel doesn't handle nested skel roots
        // very well currently; this restriction may be loosened in the future.
        if (UsdSkelRoot root2 = UsdSkelRoot::Find(root.GetPrim().GetParent())) {
            TF_RUNTIME_ERROR(
                "The SkelRoot <%s> is nested inside another "
                "SkelRoot <%s>. This might cause unexpected behavior.",
                root.GetPath().GetText(),
                root2.GetPath().GetText());
            return SdfPath();
        } else {
            return root.GetPath();
        }
    } else if (config == UsdMayaJobExportArgsTokens->auto_) {
        // If auto-generating the SkelRoot, find the rootmost
        // UsdGeomXform and turn it into a SkelRoot.
        // XXX: It might be good to also consider model hierarchy here, and not
        // go past our ancestor component when trying to generate the SkelRoot.
        // (Example: in a scene with /World, /World/Char_1, /World/Char_2, we
        // might want SkelRoots to stop at Char_1 and Char_2.) Unfortunately,
        // the current structure precludes us from accessing model hierarchy
        // here.
        if (UsdPrim root = _FindRootmostXformOrSkelRoot(stage, path)) {
            UsdSkelRoot::Define(stage, root.GetPath());
            return root.GetPath();
        } else {
            if (path.IsRootPrimPath()) {
                if (!rootPrimPath.IsEmpty()) {
                    return rootPrimPath;
                }
                // This is the most common problem when we can't obtain a
                // SkelRoot.
                // Show a nice error with useful information about root prims.
                TF_RUNTIME_ERROR(
                    "The prim <%s> is a root prim, so it has no "
                    "ancestors that can be converted to a SkelRoot. (USD "
                    "requires that skinned meshes and skeletons be "
                    "encapsulated under a SkelRoot.) Try grouping this "
                    "prim under a parent group.",
                    path.GetText());
            } else {
                // Show generic error as a last resort if we don't know exactly
                // what went wrong.
                TF_RUNTIME_ERROR(
                    "Could not find an ancestor of the prim <%s> "
                    "that can be converted to a SkelRoot. (USD requires "
                    "that skinned meshes and skeletons be encapsulated "
                    "under a SkelRoot.)",
                    path.GetText());
            }
            return SdfPath();
        }
    }
    return SdfPath();
}

void UsdMaya_SkelBindingsProcessor::MarkBindings(
    const SdfPath& path,
    const SdfPath& skelPath,
    const TfToken& config)
{
    _bindingToSkelMap[path] = _Entry(skelPath, config);
}

void UsdMaya_SkelBindingsProcessor::SetRootPrimPath(const SdfPath& rootPrimPath)
{
    this->_rootPrimPath = rootPrimPath;
}

bool UsdMaya_SkelBindingsProcessor::_VerifyOrMakeSkelRoots(const UsdStagePtr& stage) const
{
    bool success = true;
    for (const auto& pair : _bindingToSkelMap) {
        const _Entry& entry = pair.second;
        SdfPath       skelRootPath
            = _VerifyOrMakeSkelRoot(stage, pair.first, entry.second, _rootPrimPath);
        success = success && !skelRootPath.IsEmpty();
        if (!success) {
            return success;
        }
    }
    return success;
}

bool UsdMaya_SkelBindingsProcessor::UpdateSkelRootsWithExtent(
    const UsdStagePtr&  stage,
    const VtVec3fArray& bbox,
    const UsdTimeCode&  timeSample)
{
    bool success = true;
    for (const auto& pair : _bindingToSkelMap) {
        const _Entry& entry = pair.second;
        SdfPath       skelRootPath
            = _VerifyOrMakeSkelRoot(stage, pair.first, entry.second, _rootPrimPath);
        success = success && !skelRootPath.IsEmpty();
        if (success) {
            UsdSkelRoot skelRoot = UsdSkelRoot::Get(stage, skelRootPath);
            if (skelRoot) {
                UsdAttribute extentsAttr = skelRoot.CreateExtentAttr();
                if (!extentsAttr) {
                    TF_RUNTIME_ERROR(
                        "Could not find/create the extents attribute on the SkelRoot!");
                    return false;
                }
                success = extentsAttr.Set(bbox, timeSample);
                if (!success) {
                    return false;
                }
            }
        }
    }

    return success;
}

bool UsdMaya_SkelBindingsProcessor::PostProcessSkelBindings(const UsdStagePtr& stage) const
{
    bool success = _VerifyOrMakeSkelRoots(stage);
    return success;
}

PXR_NAMESPACE_CLOSE_SCOPE
