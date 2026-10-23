//
// Copyright 2026 Autodesk
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

#ifndef MAYAUSD_STAGE_SEARCH_CMD_H
#define MAYAUSD_STAGE_SEARCH_CMD_H

#include <mayaUsd/mayaUsd.h>

#include <maya/MPxCommand.h>

namespace MAYAUSD_NS_DEF {

// Finds the prims of all the USD stages of the scene whose name matches a
// pattern, by traversing the stages directly through USD in a single pass. This
// is the same search as the USD runtime's Ufe::HierarchyHandler::find(), see
// MayaUsd::ufe::MayaUsdHierarchyHandler::findStagePrims(), with a
// case-insensitive shell glob ('*', '?') on the prim name as the predicate. A
// wildcard-free pattern matches as a substring.
//
// Flags:
//   -pattern <string>   the search text (required)
//   -caseSensitive      match case-sensitively (default: case-insensitive)
//
// Returns: string[] of matching UFE path strings.
class MayaUsdStageSearchCommand : public MPxCommand
{
public:
    static void*   creator() { return new MayaUsdStageSearchCommand(); }
    static MSyntax createSyntax();

    static const MString commandName;

    MStatus doIt(const MArgList& args) override;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_STAGE_SEARCH_CMD_H
