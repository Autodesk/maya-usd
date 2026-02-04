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

#include "mayaUsdInfoCommand.h"

#include <mayaUsd/buildInfo.h>

#include <maya/MArgParser.h>
#include <maya/MSyntax.h>

#define XSTR(x) STR(x)
#define STR(x)  #x

namespace MAYAUSD_NS_DEF {

const MString MayaUsdInfoCommand::commandName("mayaUsdInfo");

namespace {

// Versioning and build information.
constexpr auto kMajorVersion = "-mjv";
constexpr auto kMajorVersionLong = "-majorVersion";

constexpr auto kMinorVersion = "-mnv";
constexpr auto kMinorVersionLong = "-minorVersion";

constexpr auto kPatchVersion = "-pv";
constexpr auto kPatchVersionLong = "-patchVersion";

constexpr auto kVersion = "-v";
constexpr auto kVersionLong = "-version";

constexpr auto kCutId = "-c";
constexpr auto kCutIdLong = "-cutIdentifier";

constexpr auto kBuildNumber = "-bn";
constexpr auto kBuildNumberLong = "-buildNumber";

constexpr auto kGitCommit = "-gc";
constexpr auto kGitCommitLong = "-gitCommit";

constexpr auto kGitBranch = "-gb";
constexpr auto kGitBranchLong = "-gitBranch";

constexpr auto kBuildDate = "-bd";
constexpr auto kBuildDateLong = "-buildDate";

constexpr auto kBuildAR = "-ar";
constexpr auto kBuildARLong = "-buildAR";

} // namespace

MSyntax MayaUsdInfoCommand::createSyntax()
{
    MSyntax syntax;
    syntax.enableQuery(false);
    syntax.enableEdit(false);

    // Versioning and build information flags.
    syntax.addFlag(kMajorVersion, kMajorVersionLong);
    syntax.addFlag(kMinorVersion, kMinorVersionLong);
    syntax.addFlag(kPatchVersion, kPatchVersionLong);
    syntax.addFlag(kVersion, kVersionLong);
    syntax.addFlag(kCutId, kCutIdLong);
    syntax.addFlag(kBuildNumber, kBuildNumberLong);
    syntax.addFlag(kGitCommit, kGitCommitLong);
    syntax.addFlag(kGitBranch, kGitBranchLong);
    syntax.addFlag(kBuildDate, kBuildDateLong);
    syntax.addFlag(kBuildAR, kBuildARLong);

    return syntax;
}

MStatus MayaUsdInfoCommand::doIt(const MArgList& args)
{
    MStatus    st;
    MArgParser argData(syntax(), args, &st);
    if (!st)
        return st;

    if (argData.isFlagSet(kMajorVersion)) {
        setResult(MAYAUSD_MAJOR_VERSION); // int
    } else if (argData.isFlagSet(kMinorVersion)) {
        setResult(MAYAUSD_MINOR_VERSION); // int
    } else if (argData.isFlagSet(kPatchVersion)) {
        setResult(MAYAUSD_PATCH_LEVEL); // int
    } else if (argData.isFlagSet(kVersion)) {
        setResult(XSTR(MAYAUSD_VERSION)); // convert to string
    } else if (argData.isFlagSet(kCutId)) {
        setResult(MayaUsdBuildInfo::cutId());
    } else if (argData.isFlagSet(kBuildNumber)) {
        setResult(MayaUsdBuildInfo::buildNumber());
    } else if (argData.isFlagSet(kGitCommit)) {
        setResult(MayaUsdBuildInfo::gitCommit());
    } else if (argData.isFlagSet(kGitBranch)) {
        setResult(MayaUsdBuildInfo::gitBranch());
    } else if (argData.isFlagSet(kBuildDate)) {
        setResult(MayaUsdBuildInfo::buildDate());
    } else if (argData.isFlagSet(kBuildAR)) {
        setResult(MayaUsdBuildInfo::buildAR());
    }

    return MS::kSuccess;
}

} // namespace MAYAUSD_NS_DEF
