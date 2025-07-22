//
// Copyright 2021 Autodesk
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
#include <mayaUsd/fileio/primUpdater.h>
#include <mayaUsd/fileio/primUpdaterManager.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/pyContainerConversions.h>
#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/pxr.h>
#include <pxr_python.h>

#include <maya/MFnDagNode.h>
#include <ufe/path.h>
#include <ufe/pathString.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

using NodeWithUserArgs = std::pair<std::string, VtDictionary>;

bool isAnimated(const std::string& nodeName)
{
    MObject obj;
    MStatus status = UsdMayaUtil::GetMObjectByName(nodeName, obj);
    if (status != MStatus::kSuccess)
        return false;

    MDagPath dagPath;
    status = MDagPath::getAPathTo(obj, dagPath);
    if (status != MStatus::kSuccess)
        return false;

    return UsdMayaPrimUpdater::isAnimated(dagPath);
}

std::vector<std::string> mergeNodesToUsd(const std::vector<NodeWithUserArgs>& nodeNameAndUserArgs)
{
    std::vector<PushToUsdArgs> meshArgsVect;
    meshArgsVect.reserve(nodeNameAndUserArgs.size());

    for (const auto& nameArgs : nodeNameAndUserArgs) {
        const MDagPath dagPath = UsdMayaUtil::nameToDagPath(nameArgs.first);
        if (!dagPath.isValid())
            return {};

        auto mergeArgs = PushToUsdArgs::forMerge(dagPath, nameArgs.second);
        if (!mergeArgs)
            return {};

        meshArgsVect.push_back(std::move(mergeArgs));
    }

    const auto merged = PrimUpdaterManager::getInstance().mergeToUsd(meshArgsVect);

    std::vector<std::string> results(merged.size());
    std::transform(merged.begin(), merged.end(), results.begin(), [](const Ufe::Path& p) {
        return Ufe::PathString::string(p);
    });

    return results;
}

// Convenient overload to merge a single object with optional userArgs.
bool mergeToUsd(const std::string& nodeName, const VtDictionary& userArgs = {})
{
    return !mergeNodesToUsd({ { nodeName, userArgs } }).empty();
}

PXR_BOOST_PYTHON_FUNCTION_OVERLOADS(mergeToUsd_overloads, mergeToUsd, 1, 2)

bool editAsMaya(const std::string& ufePathString)
{
    Ufe::Path path = Ufe::PathString::path(ufePathString);

    return PrimUpdaterManager::getInstance().editAsMaya(path);
}

bool canEditAsMaya(const std::string& ufePathString)
{
    return PrimUpdaterManager::getInstance().canEditAsMaya(Ufe::PathString::path(ufePathString));
}

bool discardEdits(const std::string& nodeName)
{
    auto dagPath = UsdMayaUtil::nameToDagPath(nodeName);
    return PrimUpdaterManager::getInstance().discardEdits(dagPath);
}

std::string duplicate(
    const std::string&  srcUfePathString,
    const std::string&  dstUfePathString,
    const VtDictionary& userArgs = VtDictionary())
{
    // Either input string is allowed to be null (but not both).
    Ufe::Path src
        = srcUfePathString.empty() ? Ufe::Path() : Ufe::PathString::path(srcUfePathString);
    Ufe::Path dst
        = dstUfePathString.empty() ? Ufe::Path() : Ufe::PathString::path(dstUfePathString);

    if (src.empty() && dst.empty())
        return {};

    auto dstUfePaths = PrimUpdaterManager::getInstance().duplicate(src, dst, userArgs);
    if (dstUfePaths.size() <= 0)
        return {};

    return Ufe::PathString::string(dstUfePaths[0]);
}

PXR_BOOST_PYTHON_FUNCTION_OVERLOADS(duplicate_overloads, duplicate, 2, 3)

std::string readPullInformationString(const PXR_NS::UsdPrim& prim)
{
    std::string dagPathStr;
    // Ignore boolean return value, empty string is the proper error result.
    MayaUsd::readPullInformation(prim, dagPathStr);
    return dagPathStr;
}

bool isEditedPrimOrphaned(const PXR_NS::UsdPrim& prim)
{
    return MayaUsd::isEditedAsMayaOrphaned(prim);
}

} // namespace

void wrapPrimUpdaterManager()
{
    TfPyContainerConversions::tuple_mapping_pair<NodeWithUserArgs>();

    TfPyContainerConversions::from_python_sequence<
        std::vector<NodeWithUserArgs>,
        TfPyContainerConversions::variable_capacity_policy>();

    PXR_BOOST_PYTHON_NAMESPACE::class_<PrimUpdaterManager, PXR_BOOST_PYTHON_NAMESPACE::noncopyable>(
        "PrimUpdaterManager", PXR_BOOST_PYTHON_NAMESPACE::no_init)
        .def("isAnimated", isAnimated)
        .def("mergeToUsd", mergeToUsd, mergeToUsd_overloads())
        .def("mergeToUsd", mergeNodesToUsd)
        .def("editAsMaya", editAsMaya)
        .def("canEditAsMaya", canEditAsMaya)
        .def("discardEdits", discardEdits)
        .def("duplicate", duplicate, duplicate_overloads())
        .def("isEditedAsMayaOrphaned", isEditedPrimOrphaned)
        .def("readPullInformation", readPullInformationString);
}
