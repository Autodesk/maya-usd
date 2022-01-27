//
// Copyright 2017 Animal Logic
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
#pragma once

#include <AL/usdmaya/Api.h>
#include <AL/usdmaya/StageCache.h>
#include <AL/usdmaya/fileio/ImportParams.h>
#include <AL/usdmaya/nodes/ProxyShape.h>

#include <pxr/usd/usd/stage.h>

#include <maya/MDagModifier.h>
#include <maya/MObjectArray.h>
#include <maya/MPxCommand.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace cmds {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Base class for all proxy shape commands. Sets up some common command syntax, along with
/// a few handy utility
///         methods.
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ProxyShapeCommandBase : public MPxCommand
{
public:
    /// \brief  sets up some core/common command params - enables specification of selectin list as
    /// a command argument \return the syntax object that contains the common command arguments
    static MSyntax setUpCommonSyntax();

    /// \brief  find the proxy shape node
    /// \param  args the valid argument data base
    /// \return the dag path to the proxy shape (extracted from the selected nodes used as the
    /// command args)
    MDagPath getShapePath(const MArgDatabase& args);

    /// \brief  find the proxy shape nodeata base
    /// \param  args the valid argument data base
    /// \return the proxy shape specified in the selected command arguments
    nodes::ProxyShape* getShapeNode(const MArgDatabase& args);

    /// \brief  get the USD stage
    /// \param  args the valid argument data base
    /// \return the stage from the proxy shape specified in the selected command arguments
    UsdStageRefPtr getShapeNodeStage(const MArgDatabase& args);

protected:
    /// utility func to create the arg database
    MArgDatabase makeDatabase(const MArgList& args);
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Imports a proxy shape into maya
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ProxyShapeImport : public MPxCommand
{
    MDagModifier m_modifier;
    MDagModifier m_modifier2;
    MObjectArray m_parentTransforms;
    MObject      m_shape;
    MString      m_proxy_name;
    bool         m_createdParent = true;

public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus undoIt() override;
    MStatus redoIt() override;
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  ProxyShapeFindLoadable
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ProxyShapeFindLoadable : public ProxyShapeCommandBase
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  ProxyShapeImportAllTransforms
///         From a proxy shape, this will import all usdPrims in the stage as AL_usdmaya_Transform
///         nodes.
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ProxyShapeImportAllTransforms : public ProxyShapeCommandBase
{
    MDagModifier m_modifier;
    MDagModifier m_modifier2;

public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus undoIt() override;
    MStatus redoIt() override;
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  ProxyShapeRemoveAllTransforms
///         A command that will remove all AL::usdmaya::nodes::Transform nodes from a given
///         AL_usd_ProxyShape
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ProxyShapeRemoveAllTransforms : public ProxyShapeCommandBase
{
    MDagModifier m_modifier;

public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus undoIt() override;
    MStatus redoIt() override;
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  ProxyShapeImportPrimPathAsMaya
///         A command that will import a portion of a proxyNode as Maya geometry/transforms
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ProxyShapeImportPrimPathAsMaya : public ProxyShapeCommandBase
{
    MDagModifier           m_modifier;
    SdfPath                m_path;
    MDagPath               m_transformPath;
    fileio::ImporterParams m_importParams;
    MObjectArray           m_nodesToKill;
    bool                   m_asProxyShape;

    MObject makePrimTansforms(nodes::ProxyShape* shapeNode, UsdPrim usdPrim);

    MObject makeUsdTransformChain(
        std::map<std::string, MObject>& mapped,
        const UsdPrim&                  usdPrim,
        const MPlug&                    outStage,
        const MPlug&                    outTime,
        const MObject&                  parentXForm);

public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus undoIt() override;
    MStatus redoIt() override;
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  ProxyShapePrintRefCountState
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ProxyShapePrintRefCountState : public ProxyShapeCommandBase
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  ProxyShapeResync
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ProxyShapeResync : public ProxyShapeCommandBase
{
    SdfPath            m_resyncPrimPath;
    UsdPrim            m_resyncPrim;
    nodes::ProxyShape* m_shapeNode;

public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
    MStatus redoIt() override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  TranslatePrim
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class TranslatePrim : public ProxyShapeCommandBase
{
    fileio::translators::TranslatorParameters tp;

    nodes::ProxyShape* m_proxy;
    SdfPathVector      m_importPaths;
    SdfPathVector      m_teardownPaths;
    SdfPathVector      m_updatePaths;
    bool               m_recursive;

public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
    MStatus redoIt() override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  ProxyShapeTestIntersection
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ProxyShapeTestIntersection : public ProxyShapeCommandBase
{
    nodes::ProxyShape* m_proxy;
    double             m_sx;
    double             m_sy;

public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
    MStatus redoIt() override;
};

/// \brief  This function will generate all of the MEL script menus, and the option boxes, for all
/// of the proxy shape
///         commands.
/// \ingroup commands
AL_USDMAYA_PUBLIC
void constructProxyShapeCommandGuis();

/// \brief Construct menu and commands for setting pick modes.
/// \ingroup commands
AL_USDMAYA_PUBLIC
void constructPickModeCommandGuis();

//----------------------------------------------------------------------------------------------------------------------
} // namespace cmds
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
