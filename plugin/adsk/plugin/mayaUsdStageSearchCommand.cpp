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

#include "mayaUsdStageSearchCommand.h"

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/MayaUsdHierarchyHandler.h>

#include <maya/MArgParser.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>
#include <ufe/hierarchyHandler.h>
#include <ufe/path.h>
#include <ufe/pathString.h>
#include <ufe/runTimeMgr.h>
#include <ufe/selection.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace MAYAUSD_NS_DEF {

const MString MayaUsdStageSearchCommand::commandName("mayaUsdStageSearch");

namespace {

constexpr auto kPattern = "-p";
constexpr auto kPatternLong = "-pattern";

constexpr auto kCaseSensitive = "-cs";
constexpr auto kCaseSensitiveLong = "-caseSensitive";

std::string toLower(const std::string& s)
{
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

// Linear shell-glob matcher supporting '*' (any run) and '?' (single char).
bool globMatch(const std::string& text, const std::string& pattern)
{
    const char* t = text.c_str();
    const char* p = pattern.c_str();
    const char* star = nullptr;
    const char* tAfterStar = t;

    while (*t != '\0') {
        if (*p == '?' || *p == *t) {
            ++t;
            ++p;
        } else if (*p == '*') {
            star = p++;
            tAfterStar = t;
        } else if (star != nullptr) {
            p = star + 1;
            t = ++tAfterStar;
        } else {
            return false;
        }
    }
    while (*p == '*') {
        ++p;
    }
    return *p == '\0';
}

// A wildcard-free pattern matches as a substring.
UsdUfe::UsdHierarchyHandler::NameMatchFcn
globMatcher(const std::string& pattern, bool caseSensitive)
{
    std::string glob(pattern);
    if (glob.find_first_of("*?") == std::string::npos) {
        glob = "*" + glob + "*";
    }
    if (caseSensitive) {
        return [glob](const std::string& name) { return globMatch(name, glob); };
    }
    return
        [glob = toLower(glob)](const std::string& name) { return globMatch(toLower(name), glob); };
}

} // namespace

MSyntax MayaUsdStageSearchCommand::createSyntax()
{
    MSyntax syntax;
    syntax.enableQuery(false);
    syntax.enableEdit(false);

    syntax.addFlag(kPattern, kPatternLong, MSyntax::kString);
    syntax.addFlag(kCaseSensitive, kCaseSensitiveLong);

    return syntax;
}

MStatus MayaUsdStageSearchCommand::doIt(const MArgList& args)
{
    MStatus    st;
    MArgParser argData(syntax(), args, &st);
    if (!st)
        return st;

    if (!argData.isFlagSet(kPattern)) {
        MGlobal::displayError("mayaUsdStageSearch: -pattern is required.");
        return MS::kFailure;
    }

    MString pattern;
    argData.getFlagArgument(kPattern, 0, pattern);

    const bool caseSensitive = argData.isFlagSet(kCaseSensitive);

    const auto usdHierarchyHandler
        = Ufe::RunTimeMgr::instance().hierarchyHandler(MayaUsd::ufe::getUsdRunTimeId());
    const Ufe::Hierarchy::ChildFilter childFilter
        = usdHierarchyHandler ? usdHierarchyHandler->childFilter() : Ufe::Hierarchy::ChildFilter();

    const Ufe::Selection found = MayaUsd::ufe::MayaUsdHierarchyHandler::findStagePrims(
        Ufe::Path(), globMatcher(pattern.asChar(), caseSensitive), childFilter);

    MStringArray results;
    for (const auto& item : found) {
        results.append(Ufe::PathString::string(item->path()).c_str());
    }

    setResult(results);
    return MS::kSuccess;
}

} // namespace MAYAUSD_NS_DEF
