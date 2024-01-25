//
// Copyright 2024 Autodesk
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

#include "copyLayerPrims.h"

#include <mayaUsd/utils/traverseLayer.h>

#include <pxr/usd/sdf/copyUtils.h>

#include <maya/MGlobal.h>

using namespace PXR_NS;

namespace {

// Debugging helpers to log message to help diagnose problems.
//#define DEBUG_COPY_LAYER_PRIMS

#ifdef DEBUG_COPY_LAYER_PRIMS

void logDebug(const std::string& msg) { MGlobal::displayInfo(msg.c_str()); }
#define DEBUG_LOG_COPY_LAYER_PRIMS(X) logDebug(X)

#else

#define DEBUG_LOG_COPY_LAYER_PRIMS(X)

#endif

// Wrappers for the optional progress bar, to avoid having to check if it
// is null everywhere in the code.

void addProgressSteps(const MayaUsd::CopyLayerPrimsOptions& options, int steps)
{
    if (!options.progressBar)
        return;

    options.progressBar->addSteps(steps);
}

void advanceProgress(const MayaUsd::CopyLayerPrimsOptions& options)
{
    if (!options.progressBar)
        return;

    options.progressBar->advance();
}

// Replicate missing ancestor prims of the given destination prim
// by mimicking those that were found in the source.

void replicateMissingAncestors(
    const UsdStageRefPtr& srcStage,
    SdfPath               srcPath,
    const UsdStageRefPtr& dstStage,
    SdfPath               dstPath)
{
    // List of prims to be created. The last prim should be created
    // first. (It will be the highest in the hierarchy.)
    std::vector<std::pair<SdfPath, SdfPath>> toBeCreated;

    while (true) {
        // If we reach the top of the hierarchy, stop.
        srcPath = srcPath.GetParentPath();
        if (srcPath.IsEmpty() || srcPath.IsAbsoluteRootPath())
            break;

        // If we reach the top of the hierarchy, stop.
        dstPath = dstPath.GetParentPath();
        if (dstPath.IsEmpty() || dstPath.IsAbsoluteRootPath())
            break;

        // If the destination prim already exists, stop.
        UsdPrim dstPrim = dstStage->GetPrimAtPath(dstPath);
        if (dstPrim) {
            DEBUG_LOG_COPY_LAYER_PRIMS(
                TfStringPrintf("The ancestor %s exists", dstPath.GetAsString().c_str()));
            break;
        }

        DEBUG_LOG_COPY_LAYER_PRIMS(
            TfStringPrintf("The ancestor %s needs to be created", dstPath.GetAsString().c_str()));
        toBeCreated.emplace_back(srcPath, dstPath);
    }

    const auto end = toBeCreated.rend();
    for (auto iter = toBeCreated.rbegin(); iter != end; ++iter) {
        const SdfPath& srcPath = iter->first;
        const SdfPath& dstPath = iter->second;

        // Try to reproduce the same prim type, if we can.
        TfToken primType;
        if (UsdPrim srcPrim = srcStage->GetPrimAtPath(srcPath))
            primType = srcPrim.GetTypeName();

        dstStage->DefinePrim(dstPath, primType);
    }
}

// Verify if the given path needs to be renamed and rename it if needed.
void renamePath(SdfPath& pathToVerify, const MayaUsd::CopyLayerPrimsResult& result)
{
    for (const auto& oldAndNew : result.renamedPaths) {
        // If the relationship was not targeting this renamed path, skip.
        const SdfPath& oldPath = oldAndNew.first;
        if (!pathToVerify.HasPrefix(oldPath))
            continue;

        const SdfPath& newPath = oldAndNew.second;
        SdfPath        renamedPath = pathToVerify.ReplacePrefix(oldPath, newPath);

        DEBUG_LOG_COPY_LAYER_PRIMS(TfStringPrintf(
            "Renaming path %s to %s",
            pathToVerify.GetAsString().c_str(),
            renamedPath.GetAsString().c_str()));

        pathToVerify = renamedPath;

        // Note: each path must only be renamed once. Otherwise, if there
        //       is a chain of renaming 1 -> 2 -> 3, etc, all paths would
        //       get renamed to the end of the chain, instead of their one
        //       true renamed path.
        //
        //       For example:
        //
        //       Let's say we copied a1 and a2 and suppose the destination
        //       already contained a1. Then a1 will become a2 and a2 will
        //       become a3 in the destination.
        //
        //       When verifying the path a1 we want to correctly rename it to
        //       the path a2, but then avoid renaming it again to a3. That is
        //       why we interrupt renaming at the first renaming.
        break;
    }
}

// Prim hierarchy traverser (a function called for every SdfSpec starting
// from a prim to be copied, recursively) that copies each prim encountered
// and optionally adds the targets of relationships to the list of other paths
// to also be copied.
//
// Note: returning false means to prune traversing children.
bool copyTraverser(
    const UsdStageRefPtr&                 srcStage,
    const SdfLayerRefPtr&                 srcLayer,
    const SdfPath&                        srcParentPath,
    const UsdStageRefPtr&                 dstStage,
    const SdfLayerRefPtr&                 dstLayer,
    const SdfPath&                        dstParentPath,
    std::vector<SdfPath>&                 otherPathsToCopy,
    const MayaUsd::CopyLayerPrimsOptions& options,
    const SdfPath&                        pathToCopy,
    MayaUsd::CopyLayerPrimsResult&        result)
{
    // Check if the path is a relationship target path. If so, we optionally copy
    // the target since it is used by the prim containing this relationship.
    if (pathToCopy.IsTargetPath()) {
        if (options.followRelationships) {
            const SdfPath& targetPath = pathToCopy.GetTargetPath();
            if (!targetPath.IsEmpty()) {
                DEBUG_LOG_COPY_LAYER_PRIMS(TfStringPrintf(
                    "Adding %s to be copied due to target in %s",
                    targetPath.GetAsString().c_str(),
                    pathToCopy.GetAsString().c_str()));

                otherPathsToCopy.emplace_back(targetPath);
                addProgressSteps(options, 1);
            }
        }
        return true;
    }

    // We only copy prims, not any other type of specs, like attributes etc.
    // Copying the prim will copy its attributes, etc.
    if (!pathToCopy.IsPrimPath()) {
        DEBUG_LOG_COPY_LAYER_PRIMS(
            TfStringPrintf("Path %s is not a prim path", pathToCopy.GetAsString().c_str()));
        return true;
    }

    for (const auto& srcAndDest : result.copiedPaths) {
        const SdfPath& alreadyDone = srcAndDest.first;
        if (pathToCopy.HasPrefix(alreadyDone)) {
            DEBUG_LOG_COPY_LAYER_PRIMS(TfStringPrintf(
                "Already copied source prim %s, skipping additional copies",
                pathToCopy.GetAsString().c_str()));

            // Note: it may have been copied indirectly, in that case it will not
            //       have been added to the list copied paths, so we want to add
            //       it to the list of copied paths now. It's important that it be
            //       added because the list of copied prims is used to post-process
            //       the copy by the callers. For example, we post-process it to
            //       handle display layers.
            if (result.copiedPaths.count(pathToCopy) == 0) {
                SdfPath dstPath = pathToCopy.ReplacePrefix(srcParentPath, dstParentPath);
                // Verify if the prim that contained this prim was renamed.
                renamePath(dstPath, result);
                result.copiedPaths[pathToCopy] = dstPath;
            }

            // Note: we must not prevent traversing children otherwise we will
            //       not process relationships.
            return true;
        }
    }

    // Make the destination path unique and make sure parent prims
    // exists in the destination.
    const SdfPath origDstPath = pathToCopy.ReplacePrefix(srcParentPath, dstParentPath);
    replicateMissingAncestors(srcStage, pathToCopy, dstStage, origDstPath);
    const SdfPath dstPath = UsdUfe::uniqueChildPath(*dstStage, origDstPath);

    // Record the copy and the potential renaming.
    result.copiedPaths[pathToCopy] = dstPath;
    if (dstPath != origDstPath)
        result.renamedPaths[origDstPath] = dstPath;

    const std::string copyingMsg = TfStringPrintf(
        "Copying source prim %s to destination prim %s",
        pathToCopy.GetAsString().c_str(),
        dstPath.GetAsString().c_str());
    MGlobal::displayInfo(copyingMsg.c_str());

    // Now perform the actual copy.
    if (!SdfCopySpec(srcLayer, pathToCopy, dstLayer, dstPath)) {
        const std::string errMsg
            = TfStringPrintf("could not copy to %s", dstPath.GetAsString().c_str()).c_str();
        throw std::runtime_error(errMsg);
    }

    return true;
}

// Function to create a copy traverser functor that can take
// a single argument: the path to process. Used to create the
// function that can be called by the traverseLayer function.
auto makeCopyTraverser(
    const UsdStageRefPtr&                 srcStage,
    const SdfLayerRefPtr&                 srcLayer,
    const SdfPath&                        srcParentPath,
    const UsdStageRefPtr&                 dstStage,
    const SdfLayerRefPtr&                 dstLayer,
    const SdfPath&                        dstParentPath,
    std::vector<SdfPath>&                 otherPathsToCopy,
    const MayaUsd::CopyLayerPrimsOptions& options,
    MayaUsd::CopyLayerPrimsResult&        result)
{
    auto copyFn = [&srcStage,
                   &srcLayer,
                   &srcParentPath,
                   &dstStage,
                   &dstLayer,
                   &dstParentPath,
                   &otherPathsToCopy,
                   &options,
                   &result](const SdfPath& pathToCopy) -> bool {
        return copyTraverser(
            srcStage,
            srcLayer,
            srcParentPath,
            dstStage,
            dstLayer,
            dstParentPath,
            otherPathsToCopy,
            options,
            pathToCopy,
            result);
    };
    return copyFn;
}

// Prim hierarchy traverser (a function called for every SdfSpec starting
// from a prim to be copied, recursively) that finds every targeting paths.
bool findTargetingPathsTraverser(
    const UsdStageRefPtr& dstStage,
    const SdfPath&        layerSpecPath,
    std::set<SdfPath>&    targetingPaths)
{
    // We're only interested in targeting paths.
    if (!layerSpecPath.IsTargetPath())
        return true;

    SdfPath targetPath = layerSpecPath.GetTargetPath();

    if (targetingPaths.count(layerSpecPath))
        return true;

    DEBUG_LOG_COPY_LAYER_PRIMS(
        TfStringPrintf("Found targeting property %s", layerSpecPath.GetAsString().c_str()));

    targetingPaths.insert(layerSpecPath);

    return true;
}

// Function to create a find-targeting-path traverser functor that can take
// a single argument: the path to process. Used to create the function that
// can be called by the traverseLayer function.
auto makeFindTargetingPathsTraverser(
    const UsdStageRefPtr& dstStage,
    std::set<SdfPath>&    targetingPaths)
{
    auto findTargetingFn = [&dstStage, &targetingPaths](const SdfPath& layerSpecPath) -> bool {
        return findTargetingPathsTraverser(dstStage, layerSpecPath, targetingPaths);
    };
    return findTargetingFn;
}

// Verify if each target needs to be renamed and rename them if needed.
void renameTargets(SdfPathVector& targets, const MayaUsd::CopyLayerPrimsResult& result)
{
    for (auto& target : targets) {
        renamePath(target, result);
    }
}

// Verify if the targets of the given targeting path need to be renamed based on
// the known list of renamed prims and rename them if needed.
void renameTargetingPath(
    const UsdStageRefPtr&                dstStage,
    const SdfPath&                       layerSpecPath,
    const MayaUsd::CopyLayerPrimsResult& result)
{
    // We're only interested in targeting paths.
    if (!layerSpecPath.IsTargetPath())
        return;

#ifdef DEBUG_COPY_LAYER_PRIMS
    SdfPath targetPath = layerSpecPath.GetTargetPath();
    DEBUG_LOG_COPY_LAYER_PRIMS(
        TfStringPrintf("Verifying renaming for %s target path", targetPath.GetAsString().c_str()));
#endif

    // Determine if we have a relationship target or an attribute connection.
    // Note: the parent path of a targeting path is the relationship or connection.
    const SdfPath primPath = layerSpecPath.GetPrimOrPrimVariantSelectionPath();
    UsdPrim       prim = dstStage->GetPrimAtPath(primPath);
    const SdfPath targetingPath = layerSpecPath.GetParentPath();

    DEBUG_LOG_COPY_LAYER_PRIMS(TfStringPrintf(
        "Prim %s containing targeting property %s",
        primPath.GetAsString().c_str(),
        targetingPath.GetAsString().c_str()));

    // Adjust all targets that were referring to prims that were renamed.
    //
    // Note: we could *almost* use a UsdProperty, which is the base class of
    //       both UsdRelationship and UsdAttribute. It has a _GetTargets()
    //       function... but this function is protected! How unfortunate...
    //       So instead we have two almost identical branches.
    UsdRelationship rel = prim.GetRelationshipAtPath(targetingPath);
    if (rel) {
        // Modify all targets that were using the old path to now use the new path.
        SdfPathVector targets;
        rel.GetTargets(&targets);
        renameTargets(targets, result);
        rel.SetTargets(targets);
    } else {
        // Retrieve the attribute so we can modify its targets.
        UsdAttribute attr = prim.GetAttributeAtPath(targetingPath);
        if (attr) {
            // Modify all targets that were using the old path to now use the new path.
            SdfPathVector targets;
            attr.GetConnections(&targets);
            renameTargets(targets, result);
            attr.SetConnections(targets);
        }
    }
}

} // namespace

namespace MAYAUSD_NS_DEF {

CopyLayerPrimsResult copyLayerPrims(
    const UsdStageRefPtr&        srcStage,
    const SdfLayerRefPtr&        srcLayer,
    const SdfPath&               srcParentPath,
    const UsdStageRefPtr&        dstStage,
    const SdfLayerRefPtr&        dstLayer,
    const SdfPath&               dstParentPath,
    const std::vector<SdfPath>&  primsToCopy,
    const CopyLayerPrimsOptions& options)
{
    CopyLayerPrimsResult result;

#ifdef DEBUG_COPY_LAYER_PRIMS
    std::string layerContentsMsg;
    if (srcLayer->ExportToString(&layerContentsMsg))
        DEBUG_LOG_COPY_LAYER_PRIMS(layerContentsMsg.c_str());
#endif

    // This contains the list of paths that have to be copied.
    // Initially, it only contains the given source paths,
    // but we optionally add the destination of relationships
    // and connections to the list, to copy the related prims.
    std::vector<SdfPath> otherPathsToCopy = primsToCopy;

    auto copyFn = makeCopyTraverser(
        srcStage,
        srcLayer,
        srcParentPath,
        dstStage,
        dstLayer,
        dstParentPath,
        otherPathsToCopy,
        options,
        result);

    // Traverse the temporary layer starting from the source root path
    // and copy all prims, optionally including the ones targeted by relationships.
    //
    // Note: copyFn can append new items in otherPathsToCopy, so do not optimize
    //       comparing to the size of the container nor use iterators.
    //
    //       For the same reason, the otherPathsToCopy container can be resized
    //       and its value moved to a new memory location, so that is why
    //       the path we pass to the traverseLayer function is taken by value.
    addProgressSteps(options, otherPathsToCopy.size());
    for (size_t i = 0; i < otherPathsToCopy.size(); ++i) {
        const SdfPath srcPath = otherPathsToCopy[i];
        traverseLayer(srcLayer, srcPath, copyFn);
        advanceProgress(options);
    }

    // Traverse again the destination prims to find all targeting properties
    // so that we can rename their targets if necessary.
    std::set<SdfPath> targetingPaths;

    auto findTargetingsFn = makeFindTargetingPathsTraverser(dstStage, targetingPaths);

    addProgressSteps(options, result.copiedPaths.size());
    for (const auto& srcAndDest : result.copiedPaths) {
        const SdfPath& dstPath = srcAndDest.second;
        traverseLayer(dstLayer, dstPath, findTargetingsFn);
        advanceProgress(options);
    }

    DEBUG_LOG_COPY_LAYER_PRIMS(
        TfStringPrintf("Found %d targeting path.", int(targetingPaths.size())));

    // Rename each target of the given list of targeting paths when they need
    // to be renamed based on the known list of renamed prims.
    addProgressSteps(options, targetingPaths.size());
    for (const SdfPath& layerSpecPath : targetingPaths) {
        renameTargetingPath(dstStage, layerSpecPath, result);
        advanceProgress(options);
    }

    return result;
}

} // namespace MAYAUSD_NS_DEF
