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

#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/pxr.h>

#include <maya/MFnDagNode.h>
#include <ufe/path.h>
#include <ufe/pathString.h>

#include <boost/python.hpp>
#include <boost/python/def.hpp>

using namespace std;
using namespace boost::python;
using namespace boost;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
bool push(const std::string& nodeName)
{
    MObject obj;
    MStatus status = UsdMayaUtil::GetMObjectByName(nodeName, obj);
    if (status != MStatus::kSuccess)
        return false;

    MDagPath dagPath;
    status = MDagPath::getAPathTo(obj, dagPath);
    if (status != MStatus::kSuccess)
        return false;

    MFnDagNode dagNode(dagPath, &status);
    if (status != MStatus::kSuccess)
        return false;

    Ufe::Path path;
    if (!PrimUpdaterManager::readPullInformation(dagPath, path))
        return false;

    return PrimUpdaterManager::getInstance().push(dagNode, path);
}

bool pull(const std::string& ufePathString)
{
    Ufe::Path path = Ufe::PathString::path(ufePathString);

    return PrimUpdaterManager::getInstance().pull(path);
}

bool discardEdits(const std::string& nodeName)
{
    auto dagPath = UsdMayaUtil::nameToDagPath(nodeName);

    Ufe::Path path;
    if (!PrimUpdaterManager::readPullInformation(dagPath, path))
        return false;

    return PrimUpdaterManager::getInstance().discardEdits(path);
}

bool duplicate(const std::string& srcUfePathString, const std::string& dstUfePathString)
{
    Ufe::Path src = Ufe::PathString::path(srcUfePathString);
    Ufe::Path dst = Ufe::PathString::path(dstUfePathString);

    if (src.empty() || dst.empty())
        return false;

    return PrimUpdaterManager::getInstance().duplicate(src, dst);
}

} // namespace

void wrapPrimUpdaterManager()
{
    using This = PrimUpdaterManager;
    class_<This>("PrimUpdaterManager", no_init)
        .def("push", push)
        .def("pull", pull)
        .def("pullClear", discardEdits)
        .def("mergeToUsd", push)
        .def("editAsMaya", pull)
        .def("discardEdits", discardEdits)
        .def("duplicate", duplicate);
}
