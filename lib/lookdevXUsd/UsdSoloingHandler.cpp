//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************
#include "UsdSoloingHandler.h"
#include "LookdevXUfe/SceneItemUI.h"
#include "Utils.h"

#include <LookdevXUfe/Notifier.h>
#include <LookdevXUfe/UfeUtils.h>
#include <LookdevXUfe/Utils.h>

#include <ufe/attribute.h>
#include <ufe/attributes.h>
#include <ufe/nodeDef.h>
#include <ufe/pathString.h>
#include <ufe/runTimeMgr.h>
#include <ufe/scene.h>
#include <ufe/sceneNotification.h>
#include <ufe/stringUtils.h>
#include <ufe/undoableCommandMgr.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/sdr/shaderProperty.h>

#include <mayaUsdAPI/undo.h>
#include <mayaUsdAPI/utils.h>

#include <memory>

using namespace LookdevXUfe;
using namespace PXR_NS;

namespace LookdevXUsd
{

namespace
{

// Used for naming nodes and/or layers.
constexpr auto kSoloingTag = "LookdevXSoloing";
// Custom item metadata to mark nodes that are created by soloing.
constexpr auto kSoloingItem = "ldx_isSoloingItem";
// Custom item metadata to store on a soloing node that acts as state info holder.
constexpr auto kHasSoloingInfo = "ldx_hasSoloingInfo";
// -- Info about replaced material output connections.
constexpr auto kReplacedMaterialAttribute = "ldx_replacedMaterialAttribute";
constexpr auto kReplacedShaderAttribute = "ldx_replacedShaderAttribute";
constexpr auto kReplacedShaderName = "ldx_replacedShaderName";
// -- Info about currently soloed item.
constexpr auto kSoloedItemPath = "ldx_soloedItemPath";

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Metadata shortcuts

std::string getMetadata(const Ufe::SceneItem::Ptr& item, const std::string& key)
{
    if (!item)
    {
        return "";
    }
    return LookdevXUfe::getAutodeskMetadata(item, key).get<std::string>();
}

void setMetadata(const Ufe::SceneItem::Ptr& item, const std::string& key, const std::string& value)
{
    if (!item)
    {
        return;
    }
    return LookdevXUfe::setAutodeskMetadataCmd(item, key, Ufe::Value(value))->execute();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utility functions

Ufe::ConnectionHandler::Ptr getConnHandler()
{
    static auto connHandler = Ufe::RunTimeMgr::instance().connectionHandler(MayaUsdAPI::getUsdRunTimeId());
    return connHandler;
}

// Visitor for soloing items. The return value of the supplied function controls early stop of interation (on false).
void processSoloingPrimChildren(const Ufe::SceneItem::Ptr& parent,
                                const std::function<bool(const Ufe::SceneItem::Ptr& child)>& fn)
{
    if (!parent)
    {
        return;
    }

    for (const auto& child : Ufe::Hierarchy::hierarchy(parent)->children())
    {
        if (getMetadata(child, kSoloingItem) == "true")
        {
            if (!fn(child))
            {
                break;
            }
        }
    }
}

Ufe::SceneItem::Ptr getSoloingInfoItem(const Ufe::SceneItem::Ptr& parent)
{
    Ufe::SceneItem::Ptr retval = nullptr;
    processSoloingPrimChildren(parent, [&](const auto& child) {
        if (getMetadata(child, kHasSoloingInfo) == "true")
        {
            retval = child;
            return false;
        }
        return true;
    });
    return retval;
}

Ufe::SceneItem::Ptr getSoloedUsdItem(const Ufe::SceneItem::Ptr& material)
{
    const auto path = Ufe::PathString::path(getMetadata(getSoloingInfoItem(material), kSoloedItemPath));
    return Ufe::Hierarchy::createItem(path);
}

Ufe::SceneItem::Ptr getParentUsdMaterial(const Ufe::SceneItem::Ptr& item)
{
    return UfeUtils::getLookdevContainer(item);
}

class EditTargetGuard
{
public:
    explicit EditTargetGuard(UsdStageWeakPtr stage, const SdfLayerRefPtr& layer)
        : m_stage(std::move(stage)), m_originalEditTarget(m_stage->GetEditTarget())
    {
        m_stage->SetEditTarget(layer);
    }

    EditTargetGuard(const EditTargetGuard&) = delete;
    EditTargetGuard(EditTargetGuard&&) = delete;
    EditTargetGuard& operator=(const EditTargetGuard&) = delete;
    EditTargetGuard&& operator=(EditTargetGuard&&) = delete;

    ~EditTargetGuard()
    {
        m_stage->SetEditTarget(m_originalEditTarget);
    }

private:
    UsdStageWeakPtr m_stage;
    UsdEditTarget m_originalEditTarget;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NodeGraphRegistry

// Soloing is implemented by created hidden node graphs that route the desired result to the material output.
// This class maps node source types and attribute types to node graph creation functions.
class NodeGraphRegistry
{
public:
    using NodeGraph_F = std::function<void(const Ufe::SceneItem::Ptr&, const Ufe::Attribute::Ptr&)>;

    NodeGraphRegistry()
    {
        // It is assumed that as long as a standard surface node exits, the rest of the nodes as well as the
        // expected attributes also exist, and no further fine-grained error checking will happen during node creation.

        // MaterialX and Arnold do not have the same soloing requirements. In the MaterialX case we need to add an explicit
        // node to do the conversion, while Arnold can take any output type and convert it internally. This means we need to
        // differentiate Arnold nodes from MaterialX nodes, and this is done by comparing the top level classification of
        // the NodeDef. As you have noticed, it is currently impossible to Solo a native USD shader.
        const auto* mtlxShaderNodeDef =
            SdrRegistry::GetInstance().GetShaderNodeByIdentifier(TfToken(kMtlxStandardSurface));
        const auto nodeDefHandler = Ufe::RunTimeMgr::instance().nodeDefHandler(MayaUsdAPI::getUsdRunTimeId());
        if (mtlxShaderNodeDef)
        {
            const auto nodeDef = nodeDefHandler ? nodeDefHandler->definition(kMtlxStandardSurface) : nullptr;
            m_materialX = nodeDef ? TfToken(nodeDef->classification(nodeDef->nbClassifications() - 1)) : TfToken();
            registerNodeGraph(m_materialX, Ufe::Attribute::kColorFloat4, mtlxColorFloat4);
            registerNodeGraph(m_materialX, Ufe::Attribute::kFloat4, mtlxFloat4);
            registerNodeGraph(m_materialX, Ufe::Attribute::kColorFloat3, mtlxColorFloat3);
            registerNodeGraph(m_materialX, Ufe::Attribute::kFloat3, mtlxFloat3);
            registerNodeGraph(m_materialX, Ufe::Attribute::kFloat2, mtlxFloat2);
            registerNodeGraph(m_materialX, Ufe::Attribute::kFloat, mtlxFloat);
            registerNodeGraph(m_materialX, Ufe::Attribute::kInt, mtlxInt);
            registerNodeGraph(m_materialX, Ufe::Attribute::kBool, mtlxBool);
            registerNodeGraph(m_materialX, Ufe::Attribute::kGeneric, surfaceShaderDirect);
        }
        const auto* arnoldShaderNodeDef =
            SdrRegistry::GetInstance().GetShaderNodeByIdentifier(TfToken(kArnoldStandardSurface));
        if (arnoldShaderNodeDef)
        {
            const auto nodeDef = nodeDefHandler ? nodeDefHandler->definition(kArnoldStandardSurface) : nullptr;
            m_arnold = TfToken(nodeDef ? nodeDef->classification(nodeDef->nbClassifications() - 1) : TfToken());
            registerNodeGraph(m_arnold, Ufe::Attribute::kColorFloat4, arnoldTypeless);
            registerNodeGraph(m_arnold, Ufe::Attribute::kColorFloat3, arnoldTypeless);
            registerNodeGraph(m_arnold, Ufe::Attribute::kFloat3, arnoldTypeless);
            registerNodeGraph(m_arnold, Ufe::Attribute::kFloat, arnoldTypeless);
            registerNodeGraph(m_arnold, Ufe::Attribute::kInt, arnoldTypeless);
            registerNodeGraph(m_arnold, Ufe::Attribute::kBool, arnoldTypeless);
            registerNodeGraph(m_arnold, Ufe::Attribute::kGeneric, surfaceShaderDirect);
        }
    }

    static NodeGraphRegistry& instance()
    {
        static NodeGraphRegistry instance;
        return instance;
    }

    [[nodiscard]] bool supports(const TfToken& shaderSourceType, const Ufe::Attribute::Ptr& attr) const
    {
        if (attr->type() == Ufe::Attribute::kGeneric)
        {
            const auto deepAttr = UfeUtils::getConnectedSource(attr);
            if (!deepAttr)
            {
                return false;
            }
            if (shaderSourceType == m_materialX)
            {
                const auto genericAttr = std::dynamic_pointer_cast<Ufe::AttributeGeneric>(deepAttr);
                // A MaterialX "surfaceshader" output is a USD "terminal" one:
                if (!genericAttr || genericAttr->nativeType() != SdrPropertyTypes->Terminal)
                {
                    return false;
                }
            }
            const auto nodeDef = UfeUtils::getNodeDef(deepAttr->sceneItem());
            if (shaderSourceType == m_arnold)
            {
                // Since multiple things map to generic, have a hardcoded list of node categories.
                static const std::set<std::string> allowed = {"Surface", "Pbr"};
                if (!nodeDef || nodeDef->nbClassifications() < 2 || !allowed.count(nodeDef->classification(1)))
                {
                    return false;
                }
            }
        }
        return attr && m_nodeGraphs.count(shaderSourceType) && m_nodeGraphs.at(shaderSourceType).count(attr->type());
    }

    void createNodeGraph(const Ufe::SceneItem::Ptr& parent,
                         const TfToken& shaderSourceType,
                         const Ufe::Attribute::Ptr& attr) const
    {
        if (parent && supports(shaderSourceType, attr))
        {
            m_nodeGraphs.at(shaderSourceType).at(attr->type())(parent, attr);
        }
    }

private:
    void registerNodeGraph(const TfToken& shaderSourceType,
                           const Ufe::Attribute::Type& attrType,
                           const NodeGraph_F& nodeGraphF)
    {
        m_nodeGraphs[shaderSourceType][attrType] = nodeGraphF;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////
    // MaterialX

    static void mtlxColorFloat4(const Ufe::SceneItem::Ptr& parent, const Ufe::Attribute::Ptr& attr)
    {
        mtlxSurfaceShader(parent, attr);
    }

    static void mtlxFloat4(const Ufe::SceneItem::Ptr& parent, const Ufe::Attribute::Ptr& attr)
    {
        mtlxColorFloat4(parent, converterNode(parent, TfToken("ND_convert_vector4_color4"), attr));
    }

    static void mtlxColorFloat3(const Ufe::SceneItem::Ptr& parent, const Ufe::Attribute::Ptr& attr)
    {
        mtlxSurfaceShader(parent, attr);
    }

    static void mtlxFloat3(const Ufe::SceneItem::Ptr& parent, const Ufe::Attribute::Ptr& attr)
    {
        mtlxColorFloat3(parent, converterNode(parent, TfToken("ND_convert_vector3_color3"), attr));
    }

    static void mtlxFloat2(const Ufe::SceneItem::Ptr& parent, const Ufe::Attribute::Ptr& attr)
    {
        mtlxFloat3(parent, converterNode(parent, TfToken("ND_convert_vector2_vector3"), attr));
    }

    static void mtlxFloat(const Ufe::SceneItem::Ptr& parent, const Ufe::Attribute::Ptr& attr)
    {
        mtlxColorFloat3(parent, converterNode(parent, TfToken("ND_convert_float_color3"), attr));
    }

    static void mtlxInt(const Ufe::SceneItem::Ptr& parent, const Ufe::Attribute::Ptr& attr)
    {
        mtlxFloat(parent, converterNode(parent, TfToken("ND_convert_integer_float"), attr));
    }

    static void mtlxBool(const Ufe::SceneItem::Ptr& parent, const Ufe::Attribute::Ptr& attr)
    {
        mtlxFloat(parent, converterNode(parent, TfToken("ND_convert_boolean_float"), attr));
    }

    static void mtlxSurfaceShader(const Ufe::SceneItem::Ptr& parent, const Ufe::Attribute::Ptr& color)
    {
        assert((color->type() == Ufe::Attribute::kColorFloat3 || color->type() == Ufe::Attribute::kColorFloat4));

        auto shaderUsdItem = createNode(parent, TfToken(kMtlxStandardSurface));
        auto shaderAttrs = Ufe::Attributes::attributes(shaderUsdItem);

        auto base = shaderAttrs->attribute("inputs:base");
        std::dynamic_pointer_cast<Ufe::AttributeFloat>(base)->set(0.F);
        auto specular = shaderAttrs->attribute("inputs:specular");
        std::dynamic_pointer_cast<Ufe::AttributeFloat>(specular)->set(0.F);
        auto emission = shaderAttrs->attribute("inputs:emission");
        std::dynamic_pointer_cast<Ufe::AttributeFloat>(emission)->set(1.F);

        auto emissionIn = shaderAttrs->attribute("inputs:emission_color");
        auto opacityIn = shaderAttrs->attribute("inputs:opacity");

        if (std::dynamic_pointer_cast<Ufe::AttributeColorFloat3>(color))
        {
            getConnHandler()->connect(color, emissionIn);
        }
        else if (std::dynamic_pointer_cast<Ufe::AttributeColorFloat4>(color))
        {
            getConnHandler()->connect(converterNode(parent, TfToken("ND_convert_color4_color3"), color), emissionIn);
            getConnHandler()->connect(
                converterNode(parent, TfToken("ND_convert_float_color3"),
                              converterNode(parent, TfToken("ND_separate4_color4"), color, "outputs:outa")),
                opacityIn);
        }

        setupMaterialConnection(parent, shaderUsdItem);
    }

    static Ufe::Attribute::Ptr converterNode(const Ufe::SceneItem::Ptr& parent,
                                             const TfToken& nodeId,
                                             const Ufe::Attribute::Ptr& attr,
                                             const std::string& outputName = "outputs:out")
    {
        auto convert = createNode(parent, nodeId);
        auto convertAttrs = Ufe::Attributes::attributes(convert);
        auto convertIn = convertAttrs->attribute("inputs:in");
        auto convertOut = convertAttrs->attribute(outputName);
        getConnHandler()->connect(attr, convertIn);
        return convertOut;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////
    // Arnold

    static void arnoldTypeless(const Ufe::SceneItem::Ptr& parent, const Ufe::Attribute::Ptr& attr)
    {
        arnoldSurfaceShader(parent, attr);
    }

    static void arnoldSurfaceShader(const Ufe::SceneItem::Ptr& parent, const Ufe::Attribute::Ptr& color)
    {
        auto shaderUsdItem = createNode(parent, TfToken(kArnoldStandardSurface));
        auto shaderAttrs = Ufe::Attributes::attributes(shaderUsdItem);

        auto base = shaderAttrs->attribute("inputs:base");
        std::dynamic_pointer_cast<Ufe::AttributeFloat>(base)->set(0.F);
        auto specular = shaderAttrs->attribute("inputs:specular");
        std::dynamic_pointer_cast<Ufe::AttributeFloat>(specular)->set(0.F);
        auto emission = shaderAttrs->attribute("inputs:emission");

        if (color->type() != Ufe::Attribute::kBool)
        {
            std::dynamic_pointer_cast<Ufe::AttributeFloat>(emission)->set(1.F);
            auto emissionIn = shaderAttrs->attribute("inputs:emission_color");
            getConnHandler()->connect(color, emissionIn);

            if (std::dynamic_pointer_cast<Ufe::AttributeColorFloat4>(color))
            {
                auto opacityIn = shaderAttrs->attribute("inputs:opacity");

                auto convert = createNode(parent, TfToken("arnold:rgba_to_float"));
                auto convertAttrs = Ufe::Attributes::attributes(convert);
                auto convertIn = convertAttrs->attribute("inputs:input");
                auto convertOut = convertAttrs->attribute("outputs:out");
                auto mode = convertAttrs->attribute("inputs:mode");
                std::dynamic_pointer_cast<Ufe::AttributeEnumString>(mode)->set("a");

                getConnHandler()->connect(color, convertIn);
                getConnHandler()->connect(convertOut, opacityIn);
            }
        }
        // Boolean cannot connect directly to color and be converted, so it is connected to the emission weight itself.
        else
        {
            getConnHandler()->connect(color, emission);
        }

        setupMaterialConnection(parent, shaderUsdItem);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////
    // Helper functions

    static Ufe::SceneItem::Ptr createNode(const Ufe::SceneItem::Ptr& parent, const TfToken& nodeId)
    {
        Ufe::NotificationGuard guard(Ufe::Scene::instance());

        Ufe::NodeDef::Ptr nodeDef = Ufe::NodeDef::definition(parent->runTimeId(), nodeId);
        auto cmd = nodeDef->createNodeCmd(parent, std::string(kSoloingTag));
        cmd->execute();

        auto usdSceneItem = cmd->insertedChild();
        setMetadata(usdSceneItem, kSoloingItem, "true");
        const auto sceneItemUI = LookdevXUfe::SceneItemUI::sceneItemUI(usdSceneItem);
        sceneItemUI->setHiddenCmd(true)->execute();

        return usdSceneItem;
    }

    static void setupMaterialConnection(const Ufe::SceneItem::Ptr& material,
                                        const Ufe::SceneItem::Ptr& shader)
    {
        // Create a map of from material outputs to source attributes to find out which one is getting replaced.
        // This is because depending on the type of shader connected, the output can have a different name than
        // "outputs::surface", which is not known before the connection is created.
        std::unordered_map<std::string, Ufe::Attribute::Ptr> attrToSrc;
        auto connHandler = Ufe::RunTimeMgr::instance().connectionHandler(material->runTimeId());
        auto materialConns = connHandler->sourceConnections(material)->allConnections();
        for (const auto& conn : materialConns)
        {
            attrToSrc[conn->dst().name()] = conn->src().attribute();
        }

        // Connect shader output to material output
        auto materialOut = Ufe::Attributes::attributes(material)->attribute("outputs:surface");
        auto shaderOut = Ufe::Attributes::attributes(shader)->attribute("outputs:out");
        auto cmd = getConnHandler()->createConnectionCmd(shaderOut, materialOut);
        cmd->execute();

        auto outputName = cmd->connection()->dst().name();
        // Always keep track of the material output that soloing connects to.
        setMetadata(shader, kHasSoloingInfo, "true");
        setMetadata(shader, kReplacedMaterialAttribute, outputName);

        // Fetch the new output name and check if a connection existed before. If so, store the info to recreate it.
        if (attrToSrc.count(outputName))
        {
            auto src = attrToSrc[outputName];

            setMetadata(shader, kReplacedShaderName, src->sceneItem()->nodeName());
            setMetadata(shader, kReplacedShaderAttribute, src->name());
        }
    }

    static void surfaceShaderDirect(const Ufe::SceneItem::Ptr& parent, const Ufe::Attribute::Ptr& attr)
    {
        Ufe::NotificationGuard guard(Ufe::Scene::instance());
        // Use a noop compound as an intermediary node for holding soloing information.
        auto cmd = MayaUsdAPI::createAddNewPrimCommand(parent, std::string(kSoloingTag), "NodeGraph");
        cmd->execute();

        auto compound = cmd->sceneItem();
        setMetadata(compound, kSoloingItem, "true");
        const auto sceneItemUI = LookdevXUfe::SceneItemUI::sceneItemUI(compound);
        sceneItemUI->setHiddenCmd(true)->execute();

        auto compoundIn = Ufe::Attributes::attributes(compound)->addAttribute("inputs:in", attr->type());
        auto compoundOut = Ufe::Attributes::attributes(compound)->addAttribute("outputs:out", attr->type());
        std::string outputName = UfeUtils::getOutputPrefix() + "out";
        if (auto nodeDefHandler = Ufe::RunTimeMgr::instance().nodeDefHandler(attr->sceneItem()->runTimeId()))
        {
            if (auto nodeDef = nodeDefHandler->definition(attr->sceneItem()))
            {
                outputName = UfeUtils::getOutputPrefix() + nodeDef->outputNames().front();
            }
        }
        auto shaderOut = Ufe::Attributes::attributes(attr->sceneItem())->attribute(outputName);
        getConnHandler()->connect(shaderOut, compoundIn);
        getConnHandler()->connect(compoundIn, compoundOut);

        setupMaterialConnection(parent, compound);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////

    std::map<TfToken, std::unordered_map<Ufe::Attribute::Type, NodeGraph_F>> m_nodeGraphs;

    static constexpr auto kMtlxStandardSurface = "ND_standard_surface_surfaceshader";
    static constexpr auto kArnoldStandardSurface = "arnold:standard_surface";
    TfToken m_materialX;
    TfToken m_arnold;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Undoable Commands (Solo/Unsolo)

class UsdUnsoloCommand : public Ufe::UndoableCommand
{
public:
    using Ptr = std::shared_ptr<UsdUnsoloCommand>;

    explicit UsdUnsoloCommand(Ufe::SceneItem::Ptr item) : m_item(std::move(item))
    {
    }

    ~UsdUnsoloCommand() override = default;

    UsdUnsoloCommand(const UsdUnsoloCommand&) = delete;
    UsdUnsoloCommand& operator=(const UsdUnsoloCommand&) = delete;
    UsdUnsoloCommand(UsdUnsoloCommand&&) = delete;
    UsdUnsoloCommand& operator=(UsdUnsoloCommand&&) = delete;

    static Ptr create(const Ufe::SceneItem::Ptr& item)
    {
        return std::make_shared<UsdUnsoloCommand>(item);
    }

    [[nodiscard]] std::string commandString() const override
    {
        return "Unsolo";
    }

    void execute() override
    {
        const MayaUsdAPI::UsdUndoBlock undoBlock(&m_undoableItem);

        auto sessionLayer = m_item ? LookdevXUsdUtils::getSessionLayer(m_item) : nullptr;
        if (!sessionLayer)
        {
            return;
        }

        auto material = getParentUsdMaterial(m_item);
        if (!material)
        {
            return;
        }

        m_soloedItem = getSoloedUsdItem(material);

        auto prim = MayaUsdAPI::getPrimForUsdSceneItem(material);
        auto stage = prim.GetStage();

        {
            EditTargetGuard editTargetGuard(stage, sessionLayer);

            const auto& opsHandler = Ufe::RunTimeMgr::instance().sceneItemOpsHandler(material->runTimeId());
            processSoloingPrimChildren(material, [&opsHandler](const auto& child) {
                opsHandler->sceneItemOps(child)->deleteItem();
                return true;
            });
        }

        notify(false);
    }

    void undo() override
    {
        m_undoableItem.undo();
        notify(true);
    }

    void redo() override
    {
        m_undoableItem.redo();
        notify(false);
    }

private:
    void notify(bool soloEnabled)
    {
        // If the soloed item no longer exists, it means the unsolo command happened
        // in response to deleting it. In this case, the passed item was the parent.
        // If the parent was a compound, it will be useful to process notifications.
        if (m_soloedItem || (m_item && *getParentUsdMaterial(m_item) != *m_item))
        {
            LookdevXUfe::SoloingStateChanged notif(m_soloedItem ? m_soloedItem : m_item, soloEnabled);
            LookdevXUfe::Notifier::instance().notify(notif);
        }
    }

    MayaUsdAPI::UsdUndoableItem m_undoableItem;

    // Input item.
    Ufe::SceneItem::Ptr m_item;
    // Input could be either soloed item or parent material, so keep track of soloed item explicitly.
    Ufe::SceneItem::Ptr m_soloedItem;
};

class UsdSoloCommand : public Ufe::UndoableCommand
{
public:
    using Ptr = std::shared_ptr<UsdSoloCommand>;

    explicit UsdSoloCommand(Ufe::Attribute::Ptr attr) : m_attr(std::move(attr))
    {
        assert(m_attr);
    }

    ~UsdSoloCommand() override = default;

    UsdSoloCommand(const UsdSoloCommand&) = delete;
    UsdSoloCommand& operator=(const UsdSoloCommand&) = delete;
    UsdSoloCommand(UsdSoloCommand&&) = delete;
    UsdSoloCommand& operator=(UsdSoloCommand&&) = delete;

    static Ptr create(const Ufe::Attribute::Ptr& attr)
    {
        return std::make_shared<UsdSoloCommand>(attr);
    }

    [[nodiscard]] std::string commandString() const override
    {
        return "Solo";
    }

    void execute() override
    {
        const MayaUsdAPI::UsdUndoBlock undoBlock(&m_undoableItem);

        auto item = m_attr->sceneItem();
        auto prim = MayaUsdAPI::getPrimForUsdSceneItem(item);
        auto stage = prim.GetStage();
        auto material = getParentUsdMaterial(item);
        auto sessionLayer = LookdevXUsdUtils::getSessionLayer(item);

        if (!sessionLayer || !material)
        {
            return;
        }

        m_previousSoloedItem = getSoloedUsdItem(material);
        // Always unsolo at material level to ensure no stale nodes are left behind.
        UsdUnsoloCommand::create(material)->execute();

        {
            EditTargetGuard guard(stage, sessionLayer);
            NodeGraphRegistry::instance().createNodeGraph(material, LookdevXUsdUtils::getShaderSourceType(m_attr),
                                                          m_attr);

            auto soloingInfoItem = getSoloingInfoItem(material);
            if (soloingInfoItem)
            {
                setMetadata(soloingInfoItem, kSoloedItemPath, Ufe::PathString::string(item->path()));
            }
        }

        if (m_attr->sceneItem())
        {
            LookdevXUfe::SoloingStateChanged notif(m_attr->sceneItem(), true);
            LookdevXUfe::Notifier::instance().notify(notif);
        }
    }

    void undo() override
    {
        m_undoableItem.undo();
        if (m_attr->sceneItem())
        {
            LookdevXUfe::SoloingStateChanged notif(m_attr->sceneItem(), false);
            LookdevXUfe::Notifier::instance().notify(notif);
        }
        if (m_previousSoloedItem)
        {
            LookdevXUfe::SoloingStateChanged notif(m_previousSoloedItem, true);
            LookdevXUfe::Notifier::instance().notify(notif);
        }
    }

    void redo() override
    {
        m_undoableItem.redo();
        if (m_previousSoloedItem)
        {
            LookdevXUfe::SoloingStateChanged notif(m_previousSoloedItem, false);
            LookdevXUfe::Notifier::instance().notify(notif);
        }
        if (m_attr->sceneItem())
        {
            LookdevXUfe::SoloingStateChanged notif(m_attr->sceneItem(), true);
            LookdevXUfe::Notifier::instance().notify(notif);
        }
    }

private:
    MayaUsdAPI::UsdUndoableItem m_undoableItem;

    // Input attribute to solo.
    Ufe::Attribute::Ptr m_attr;
    // Caching potential pre-soloed to notify on undo.
    Ufe::SceneItem::Ptr m_previousSoloedItem;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SoloingObserver

class SoloingObserver : public Ufe::Observer
{
public:
    void operator()(const Ufe::Notification& notification) override
    {
        auto getSoloedPath = [](const auto& item) {
            if (!item)
            {
                return Ufe::Path();
            }
            auto material = getParentUsdMaterial(item);
            auto soloingInfoItem = getSoloingInfoItem(material);
            return Ufe::PathString::path(getMetadata(soloingInfoItem, kSoloedItemPath));
        };

        auto findAffectedItem = [&](const auto& usdItem, const auto& path) {
            auto soloedPath = getSoloedPath(usdItem);

            Ufe::SceneItem::Ptr result = nullptr;

            if (soloedPath == path)
            {
                result = usdItem;
            }
            // The affected item may not necessarily be the same that triggered the notification.
            // A changed path from an ancestor can invalidate connections. In this case, the new path is
            // reconstructed by replacing the old prefix of the path with the new one.
            else if (soloedPath.startsWith(path))
            {
                result = Ufe::Hierarchy::createItem(soloedPath.reparent(path, usdItem->path()));
            }

            return result;
        };

        if (const auto* pathNotif = dynamic_cast<const Ufe::ObjectPathChange*>(&notification))
        {
            auto resoloOnPathChange = [&](const auto& item) {
                auto affected = findAffectedItem(item, pathNotif->changedPath());
                if (affected && UfeUtils::getFirstOutput(affected))
                {
                    UsdSoloCommand::create(UfeUtils::getFirstOutput(affected))->execute();
                }
            };

            if (const auto* renameNotif = dynamic_cast<const Ufe::ObjectRename*>(&notification))
            {
                resoloOnPathChange(renameNotif->item());
            }
            else if (const auto* reparentNotif = dynamic_cast<const Ufe::ObjectReparent*>(&notification))
            {
                resoloOnPathChange(reparentNotif->item());
            }
        }
        else if (const auto* destroyNotif = dynamic_cast<const Ufe::ObjectDestroyed*>(&notification))
        {
            // Actual item no longer exists, but its parent potentially does.
            auto parentPath = destroyNotif->path().pop();
            auto parentItem = Ufe::Hierarchy::createItem(parentPath);
            auto soloedPath = getSoloedPath(parentItem);
            auto deletedPath = destroyNotif->path();

            // Case when deleting ancestor since there are no nested notifications.
            if (parentItem && (soloedPath.startsWith(deletedPath)))
            {
                // If deleting the shader that was connected directly to the material before soloing, then when
                // unsoloing the old connection will be looking for the deleted node and error.
                // To avoid this, delete the attribute since there is nothing to connect to.
                auto soloingInfoItem = getSoloingInfoItem(parentItem);
                auto material = getParentUsdMaterial(parentItem);
                auto shaderName = getMetadata(soloingInfoItem, kReplacedShaderName);
                Ufe::UndoableCommand::Ptr removeAttrCmd = nullptr;
                if (!UfeUtils::findChild(material, shaderName))
                {
                    auto materialOut = getMetadata(soloingInfoItem, kReplacedMaterialAttribute);
                    removeAttrCmd = Ufe::Attributes::attributes(material)->removeAttributeCmd(materialOut);
                }
                // Unsolo can work with any input that can be resolved to parent material.
                UsdUnsoloCommand::create(parentItem)->execute();
                if (removeAttrCmd)
                {
                    removeAttrCmd->execute();
                }
            }
            // Case when deleting descendant. Only unsolo if compound is no longer soloable after internal deletion.
            else if (parentItem && deletedPath.startsWith(soloedPath))
            {
                auto parentMaterial = getParentUsdMaterial(parentItem);
                auto soloedItem = getSoloedUsdItem(parentMaterial);
                if (soloedItem && !LookdevXUfe::SoloingHandler::get(soloedItem->runTimeId())->isSoloable(soloedItem))
                {
                    UsdUnsoloCommand::create(soloedItem)->execute();
                }
            }
        }
        else if (const auto* connNotif = dynamic_cast<const Ufe::AttributeConnectionChanged*>(&notification))
        {
            // The user can attempt to connect other surface shaders to the material output while soloing is active.
            // In that case, the fake connection information needs to be updated.

            const auto item = Ufe::Hierarchy::createItem(connNotif->path());
            const auto prim = MayaUsdAPI::getPrimForUsdSceneItem(item);
            // Only applicable to material nodes.
            if (!item || !prim || prim.GetTypeName() != TfToken("Material"))
            {
                return;
            }

            auto soloingInfoItem = getSoloingInfoItem(item);
            // Only applicable during soloing.
            if (!soloingInfoItem)
            {
                return;
            }

            auto attr = Ufe::Attributes::attributes(item)->attribute(connNotif->name());
            auto materialOut = getMetadata(soloingInfoItem, kReplacedMaterialAttribute);
            // Only applicable if the changed output matches the one soloing is using.
            if (!attr || materialOut != attr->name())
            {
                return;
            }

            auto stack = prim.GetPrimStack();
            // With active soloing, the prim stack of the material output will always contain the session layer edits.
            assert(stack.size() > 1);
            // Find the second strongest layer that has a connection for the material output (beyond the session one).
            // The prim stack is sorted from strong to weak.
            for (size_t i = 1; i < stack.size(); ++i)
            {
                const auto& primSpec = stack[i];
                for (const auto& attribute : primSpec->GetAttributes())
                {
                    if (attribute->GetName() == materialOut && attribute->HasConnectionPaths())
                    {
                        const EditTargetGuard editTargetGuard(prim.GetStage(), prim.GetStage()->GetSessionLayer());

                        const auto& paths = attribute->GetConnectionPathList().GetAddedOrExplicitItems();
                        assert(!paths.empty());
                        // Only one connection should be on a specific material output.
                        const auto& path = paths.front();

                        setMetadata(soloingInfoItem, kReplacedShaderName, path.GetPrimPath().GetName());
                        setMetadata(soloingInfoItem, kReplacedShaderAttribute, path.GetName());
                        break;
                    }
                }
            }
        }
    }
};

Ufe::Observer::Ptr g_soloingObserver;

} // namespace

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SoloingHandler

UsdSoloingHandler::UsdSoloingHandler()
{
    g_soloingObserver = std::make_shared<SoloingObserver>();
    Ufe::Attributes::addObserver(g_soloingObserver);
    Ufe::Scene::instance().addObserver(g_soloingObserver);
}

UsdSoloingHandler::~UsdSoloingHandler()
{
    Ufe::Attributes::removeObserver(g_soloingObserver);
    Ufe::Scene::instance().removeObserver(g_soloingObserver);
    g_soloingObserver.reset();
}

LookdevXUfe::SoloingHandler::Ptr UsdSoloingHandler::create()
{
    return std::make_shared<UsdSoloingHandler>();
}

bool UsdSoloingHandler::isSoloable(const Ufe::SceneItem::Ptr& item) const
{
    return item && isSoloable(UfeUtils::getFirstOutput(item));
}

bool UsdSoloingHandler::isSoloable(const Ufe::Attribute::Ptr& attr) const
{
    if (!attr)
    {
        return false;
    }

    auto shaderSource = LookdevXUsdUtils::getShaderSourceType(attr);

    return NodeGraphRegistry::instance().supports(shaderSource, attr);
}

Ufe::UndoableCommand::Ptr UsdSoloingHandler::soloCmd(const Ufe::SceneItem::Ptr& item) const
{
    return soloCmd(item ? UfeUtils::getFirstOutput(item) : nullptr);
}

Ufe::UndoableCommand::Ptr UsdSoloingHandler::soloCmd(const Ufe::Attribute::Ptr& attr) const
{
    if (!isSoloable(attr))
    {
        return nullptr;
    }

    return UsdSoloCommand::create(attr);
}

Ufe::UndoableCommand::Ptr UsdSoloingHandler::unsoloCmd(const Ufe::SceneItem::Ptr& item) const
{
    if (!item)
    {
        return nullptr;
    }

    return UsdUnsoloCommand::create(item);
}

bool UsdSoloingHandler::isSoloed(const Ufe::SceneItem::Ptr& item) const
{
    return getSoloedAttribute(item) != nullptr;
}

bool UsdSoloingHandler::isSoloed(const Ufe::Attribute::Ptr& attr) const
{
    if (!attr)
    {
        return false;
    }

    auto soloedAttr = getSoloedAttribute(attr->sceneItem());
    return soloedAttr && soloedAttr->name() == attr->name();
}

Ufe::SceneItem::Ptr UsdSoloingHandler::getSoloedItem(const Ufe::SceneItem::Ptr& item) const
{
    if (!item)
    {
        return nullptr;
    }

    // Fetch the actual material if the user has supplied a descendant scene item for convenience.
    auto material = getParentUsdMaterial(item);
    return getSoloedUsdItem(material);
}

bool UsdSoloingHandler::hasSoloedDescendant(const Ufe::SceneItem::Ptr& item) const
{
    if (isSoloed(item))
    {
        return false;
    }

    const auto soloedItem = getSoloedItem(item);
    Ufe::Path soloedPath = soloedItem ? soloedItem->path() : Ufe::Path{};

    return soloedPath.startsWith(item->path());
}

Ufe::Attribute::Ptr UsdSoloingHandler::getSoloedAttribute(const Ufe::SceneItem::Ptr& item) const
{
    if (!MayaUsdAPI::isUsdSceneItem(item))
    {
        return nullptr;
    }
    auto material = getParentUsdMaterial(item);

    Ufe::Attribute::Ptr retval = nullptr;

    // USD does not seem to track outgoing connections. Instead, a search is performed on the
    // incoming connections of soloing nodes until one is found that matches the input item.
    processSoloingPrimChildren(material, [&](const auto& child) {
        auto prim = MayaUsdAPI::getPrimForUsdSceneItem(child);
        const UsdShadeConnectableAPI connectableAttrs(prim);
        for (const auto& input : connectableAttrs.GetInputs(false))
        {
            for (const auto& sourceInfo : input.GetConnectedSources())
            {
                const auto connectedPrim = sourceInfo.source.GetPrim();
                auto usdPrim = MayaUsdAPI::getPrimForUsdSceneItem(item);
                if (connectedPrim == usdPrim)
                {
                    auto attrOut = sourceInfo.source.GetOutput(sourceInfo.sourceName);
                    auto attrs = Ufe::Attributes::attributes(item);
                    retval = attrs->attribute(attrOut.GetFullName());
                    return false;
                }
            }
        }
        return true;
    });

    return retval;
}

bool UsdSoloingHandler::isSoloingItem(const Ufe::SceneItem::Ptr& item) const
{
    return getMetadata(item, kSoloingItem) == "true";
}

Ufe::Connection::Ptr UsdSoloingHandler::replacedConnection(const Ufe::SceneItem::Ptr& item) const
{
    if (!MayaUsdAPI::isUsdSceneItem(item))
    {
        return nullptr;
    }
    auto material = getParentUsdMaterial(item);

    auto soloingInfoItem = getSoloingInfoItem(material);
    if (!soloingInfoItem)
    {
        return nullptr;
    }

    auto materialOut = getMetadata(soloingInfoItem, kReplacedMaterialAttribute);
    auto shaderOut = getMetadata(soloingInfoItem, kReplacedShaderAttribute);
    auto shaderName = getMetadata(soloingInfoItem, kReplacedShaderName);

    auto shaderItem = UfeUtils::findChild(material, shaderName);
    if (shaderItem)
    {
        Ufe::AttributeInfo src(Ufe::Attributes::attributes(shaderItem)->attribute(shaderOut));
        Ufe::AttributeInfo dst(Ufe::Attributes::attributes(material)->attribute(materialOut));

        return std::make_shared<Ufe::Connection>(src, dst);
    }

    return nullptr;
}

} // namespace LookdevXUsd
