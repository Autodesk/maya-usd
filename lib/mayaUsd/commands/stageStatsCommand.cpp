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
#include "stageStatsCommand.h"

#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/stageStatistics.h>
#include <mayaUsd/utils/util.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <maya/MArgParser.h>
#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>
#include <ufe/globalSelection.h>
#include <ufe/hierarchy.h>
#include <ufe/observableSelection.h>
#include <ufe/path.h>
#include <ufe/pathString.h>
#include <ufe/sceneItem.h>

#include <exception>
#include <limits>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

namespace {

constexpr auto kPrimCount = "-pc";
constexpr auto kPrimCountLong = "-primCount";
constexpr auto kMeshCount = "-mc";
constexpr auto kMeshCountLong = "-meshCount";
constexpr auto kVertexCount = "-vc";
constexpr auto kVertexCountLong = "-vertexCount";
constexpr auto kTriangleCount = "-tc";
constexpr auto kTriangleCountLong = "-triangleCount";
constexpr auto kFaceCount = "-fc";
constexpr auto kFaceCountLong = "-faceCount";
constexpr auto kNormalCount = "-nc";
constexpr auto kNormalCountLong = "-normalCount";
constexpr auto kAll = "-ac";
constexpr auto kAllLong = "-allCounts";
constexpr auto kVisibleOnly = "-vo";
constexpr auto kVisibleOnlyLong = "-visibleOnly";

struct Target
{
    PXR_NS::UsdPrim       prim;     // root of the subtree to traverse
    MObject               shapeObj; // the proxy shape
    PXR_NS::UsdTimeCode   time;     // time to sample
    PXR_NS::TfTokenVector purposes; // which purposes count as drawn
};

PXR_NS::TfTokenVector _DrawnPurposes(bool drawRender, bool drawProxy, bool drawGuide)
{
    PXR_NS::TfTokenVector purposes;
    if (drawRender) {
        purposes.push_back(PXR_NS::UsdGeomTokens->render);
    }
    if (drawProxy) {
        purposes.push_back(PXR_NS::UsdGeomTokens->proxy);
    }
    if (drawGuide) {
        purposes.push_back(PXR_NS::UsdGeomTokens->guide);
    }
    return purposes;
}

MayaUsdProxyShapeBase* _ProxyShapeAt(MDagPath& dagPath)
{
    if (dagPath.node().apiType() != MFn::kPluginShape && dagPath.extendToShape() != MS::kSuccess) {
        return nullptr;
    }

    const MObject shapeObj = dagPath.node();
    if (shapeObj.apiType() != MFn::kPluginShape) {
        return nullptr;
    }

    return dynamic_cast<MayaUsdProxyShapeBase*>(MFnDependencyNode(shapeObj).userNode());
}

MayaUsdProxyShapeBase* _ProxyShapeFor(const Ufe::Path& path)
{
    if (path.empty()) {
        return nullptr;
    }

    if (MayaUsdProxyShapeBase* shape = ufe::getProxyShape(path)) {
        return shape;
    }

    const Ufe::SceneItem::Ptr item = Ufe::Hierarchy::createItem(path);
    return item ? ufe::getProxyShapeFromItemOrChildren(item) : nullptr;
}

bool _TargetFromShape(MayaUsdProxyShapeBase* shape, Target* target)
{
    if (!shape) {
        return false;
    }

    PXR_NS::UsdPrim       rootPrim;
    PXR_NS::SdfPathVector excluded;
    int                   complexity = 0;
    PXR_NS::UsdTimeCode   time;
    bool                  drawRender = false;
    bool                  drawProxy = false;
    bool                  drawGuide = false;

    if (!shape->GetAllRenderAttributes(
            &rootPrim, &excluded, &complexity, &time, &drawRender, &drawProxy, &drawGuide)) {
        return false;
    }

    if (!rootPrim || !rootPrim.IsValid()) {
        return false;
    }

    target->prim = rootPrim;
    target->shapeObj = shape->thisMObject();
    target->time = time;
    target->purposes = _DrawnPurposes(drawRender, drawProxy, drawGuide);
    return true;
}

bool _TargetFromUfePath(const Ufe::Path& path, Target* target)
{
    if (!_TargetFromShape(_ProxyShapeFor(path), target)) {
        return false;
    }

    if (path.nbSegments() > 1) {
        const PXR_NS::UsdPrim prim = ufe::ufePathToPrim(path);
        if (!prim || !prim.IsValid()) {
            return false;
        }
        target->prim = prim;
    }

    return true;
}

std::string _NormalizeMayaSegment(const std::string& arg)
{
    const std::string::size_type sep = arg.find(Ufe::PathString::pathSegmentSeparator());
    const std::string            dagPart = arg.substr(0, sep);

    if (dagPart.empty() || dagPart.front() == '|') {
        return arg;
    }

    const MDagPath dagPath = UsdMayaUtil::nameToDagPath(dagPart);
    if (!dagPath.isValid()) {
        return arg;
    }

    const std::string fullPath = dagPath.fullPathName().asChar();
    return sep == std::string::npos ? fullPath : fullPath + arg.substr(sep);
}

bool _TargetFromString(const std::string& arg, Target* target)
{
    Ufe::Path path;
    try {
        path = Ufe::PathString::path(_NormalizeMayaSegment(arg));
    } catch (const std::exception&) {
        return false;
    }

    return _TargetFromUfePath(path, target);
}

std::vector<Target> _TargetsFromSelection()
{
    std::vector<Target> targets;

    const auto globalSelection = Ufe::GlobalSelection::get();
    if (!globalSelection) {
        return targets;
    }

    for (const Ufe::SceneItem::Ptr& item : *globalSelection) {
        if (!item) {
            continue;
        }
        Target target;
        if (_TargetFromUfePath(item->path(), &target)) {
            targets.push_back(target);
        }
    }

    return targets;
}

std::vector<Target> _TargetsFromAllStages()
{
    std::vector<Target> targets;
    for (const std::string& name : ufe::ProxyShapeHandler::getAllNames()) {
        MDagPath dagPath = UsdMayaUtil::nameToDagPath(name);
        if (!dagPath.isValid()) {
            continue;
        }

        Target target;
        if (_TargetFromShape(_ProxyShapeAt(dagPath), &target)) {
            targets.push_back(target);
        }
    }

    return targets;
}

MString _Token(const char* key, std::size_t value)
{
    return MString(key) + "=" + MString(std::to_string(value).c_str());
}

int _AsInt(std::size_t value)
{
    constexpr auto kMaxInt = static_cast<std::size_t>(std::numeric_limits<int>::max());
    if (value > kMaxInt) {
        MGlobal::displayWarning(
            "mayaUsdStageStats: count exceeds the int result range; use -all for the exact "
            "value.");
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(value);
}

} // namespace

const char StageStatsCommand::commandName[] = "mayaUsdStageStats";

void* StageStatsCommand::creator() { return static_cast<MPxCommand*>(new StageStatsCommand()); }

MSyntax StageStatsCommand::createSyntax()
{
    MSyntax syntax;

    syntax.enableQuery(true);
    syntax.enableEdit(false);

    syntax.setObjectType(MSyntax::kStringObjects);
    auto addFlag = [&syntax](const char* shortName, const char* longName) {
        if (!syntax.addFlag(shortName, longName)) {
            MGlobal::displayWarning(
                MString("mayaUsdStageStats: could not register flag ") + longName);
        }
    };

    addFlag(kPrimCount, kPrimCountLong);
    addFlag(kMeshCount, kMeshCountLong);
    addFlag(kVertexCount, kVertexCountLong);
    addFlag(kTriangleCount, kTriangleCountLong);
    addFlag(kFaceCount, kFaceCountLong);
    addFlag(kNormalCount, kNormalCountLong);
    addFlag(kAll, kAllLong);
    syntax.addFlag(kVisibleOnly, kVisibleOnlyLong, MSyntax::kBoolean);

    return syntax;
}

MStatus StageStatsCommand::doIt(const MArgList& argList)
{
    setCommandString(commandName);

    MStatus    status;
    MArgParser argParser(syntax(), argList, &status);
    if (status != MS::kSuccess) {
        return MS::kInvalidParameter;
    }

    bool visibleOnly = true;
    if (argParser.isFlagSet(kVisibleOnly)) {
        argParser.getFlagArgument(kVisibleOnly, 0, visibleOnly);
    }

    std::vector<Target> targets;

    MStringArray objects;
    argParser.getObjects(objects);
    for (unsigned int i = 0; i < objects.length(); ++i) {
        Target target;
        if (_TargetFromString(objects[i].asChar(), &target)) {
            targets.push_back(target);
        } else {
            displayWarning(
                MString("mayaUsdStageStats: ignoring unresolved object \"") + objects[i] + "\"");
        }
    }

    if (objects.length() == 0) {
        targets = _TargetsFromSelection();
        if (targets.empty()) {
            targets = _TargetsFromAllStages();
        }
    }

    StageStats total;
    for (const Target& target : targets) {
        StageStatsOptions options;
        options.visibleOnly = visibleOnly;
        options.drawnPurposes = target.purposes;
        options.time = target.time;

        total += ComputeStageStats(target.prim, options);
    }

    if (argParser.isFlagSet(kAll)) {
        MStringArray results;
        results.append(_Token("prims", total.prims));
        results.append(_Token("meshes", total.meshes));
        results.append(_Token("vertices", total.vertices));
        results.append(_Token("triangles", total.triangles));
        results.append(_Token("faces", total.faces));
        results.append(_Token("normals", total.normals));
        setResult(results);
    } else if (argParser.isFlagSet(kPrimCount)) {
        setResult(_AsInt(total.prims));
    } else if (argParser.isFlagSet(kMeshCount)) {
        setResult(_AsInt(total.meshes));
    } else if (argParser.isFlagSet(kVertexCount)) {
        setResult(_AsInt(total.vertices));
    } else if (argParser.isFlagSet(kTriangleCount)) {
        setResult(_AsInt(total.triangles));
    } else if (argParser.isFlagSet(kFaceCount)) {
        setResult(_AsInt(total.faces));
    } else if (argParser.isFlagSet(kNormalCount)) {
        setResult(_AsInt(total.normals));
    } else {
        displayError("mayaUsdStageStats: specify one count flag, or -all.");
        return MS::kInvalidParameter;
    }

    return MS::kSuccess;
}

} // namespace MAYAUSD_NS_DEF
