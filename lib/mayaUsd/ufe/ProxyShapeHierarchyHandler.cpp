//
// Copyright 2019 Autodesk
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
#include "ProxyShapeHierarchyHandler.h"

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/ProxyShapeHierarchy.h>
#include <mayaUsd/ufe/Utils.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MSelectionList.h>
#include <ufe/runTimeMgr.h>

namespace {

// Lightweight SceneItem for pure DG nodes that serve as UFE gateways
// (e.g. UsdSceneRenderSettings).  Maya's built-in hierarchy handler only
// creates items for DAG nodes, so we provide this for DG gateway nodes.
class DGGatewaySceneItem : public Ufe::SceneItem
{
public:
    DGGatewaySceneItem(const Ufe::Path& path, const std::string& nodeType)
        : Ufe::SceneItem(path)
        , _nodeType(nodeType)
    {
    }

    std::string              nodeType() const override { return _nodeType; }
    std::vector<std::string> ancestorNodeTypes() const override { return {}; }

    Ufe::Value getMetadata(const std::string& /*key*/) const override { return {}; }

    Ufe::UndoableCommandPtr
    setMetadataCmd(const std::string& /*key*/, const Ufe::Value& /*value*/) override
    {
        return nullptr;
    }

    Ufe::UndoableCommandPtr clearMetadataCmd(const std::string& /*key*/) override
    {
        return nullptr;
    }

    Ufe::Value
    getGroupMetadata(const std::string& /*group*/, const std::string& /*key*/) const override
    {
        return {};
    }

    Ufe::UndoableCommandPtr setGroupMetadataCmd(
        const std::string& /*group*/,
        const std::string& /*key*/,
        const Ufe::Value& /*value*/) override
    {
        return nullptr;
    }

    Ufe::UndoableCommandPtr
    clearGroupMetadataCmd(const std::string& /*group*/, const std::string& /*key*/) override
    {
        return nullptr;
    }

private:
    std::string _nodeType;
};

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(Ufe::HierarchyHandler, ProxyShapeHierarchyHandler);

ProxyShapeHierarchyHandler::ProxyShapeHierarchyHandler(
    const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler)
    : Ufe::HierarchyHandler()
    , _mayaHierarchyHandler(mayaHierarchyHandler)
{
}

/*static*/
ProxyShapeHierarchyHandler::Ptr
ProxyShapeHierarchyHandler::create(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler)
{
    return std::make_shared<ProxyShapeHierarchyHandler>(mayaHierarchyHandler);
}

//------------------------------------------------------------------------------
// Ufe::HierarchyHandler overrides
//------------------------------------------------------------------------------

Ufe::Hierarchy::Ptr ProxyShapeHierarchyHandler::hierarchy(const Ufe::SceneItem::Ptr& item) const
{
    auto nodeType = UsdUfe::getSceneItemNodeType(item);
    if (isAGatewayType(nodeType) && !isReferencedSceneRenderSettingsNode(nodeType, item->path())) {
        return ProxyShapeHierarchy::create(_mayaHierarchyHandler, item);
    } else {
        return _mayaHierarchyHandler->hierarchy(item);
    }
}

Ufe::SceneItem::Ptr ProxyShapeHierarchyHandler::createItem(const Ufe::Path& path) const
{
    auto item = _mayaHierarchyHandler->createItem(path);
    if (item) {
        return item;
    }

    // Maya's handler doesn't create items for DG nodes.  DG gateway nodes
    // use a null separator (e.g. "nodeName").  Extract the component name
    // directly — PathSegment::string() prepends the separator, so .c_str()
    // on a null-separated segment would yield an empty C-string.
    if (path.nbSegments() >= 1
        && path.getSegments()[0].separator() == MayaUsd::ufe::DGPathSeparator) {
        auto          nodeName = path.getSegments()[0].begin()->string();
        MSelectionList sel;
        if (sel.add(MString(nodeName.c_str())) == MS::kSuccess) {
            MObject obj;
            if (sel.getDependNode(0, obj) == MS::kSuccess && !obj.hasFn(MFn::kDagNode)) {
                MFnDependencyNode depFn(obj);
                std::string       nodeType(depFn.typeName().asChar());
                if (isAGatewayType(nodeType)) {
                    return std::make_shared<DGGatewaySceneItem>(path, nodeType);
                }
            }
        }
    }

    return nullptr;
}

Ufe::Hierarchy::ChildFilter ProxyShapeHierarchyHandler::childFilter() const
{
    // Use the same child filter as the USD hierarchy handler.
    auto usdHierHand = Ufe::RunTimeMgr::instance().hierarchyHandler(getUsdRunTimeId());
    return usdHierHand->childFilter();
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
