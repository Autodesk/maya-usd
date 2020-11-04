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

#include <AL/maya/utils/Api.h>
#include <AL/maya/utils/MayaHelperMacros.h>
#include <AL/usdmaya/Api.h>
#include <AL/usdmaya/ForwardDeclares.h>

#include <pxr/usd/usd/stage.h>

#include <maya/MDGModifier.h>
#include <maya/MPxCommand.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace cmds {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Helper class
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class LayerCommandBase : public MPxCommand
{
public:
    /// \brief  sets up some core/common command params for all the layer commands. Just specifies
    /// that selection can be
    ///         used as an argument.
    /// \return the syntax object with the command command flags specified
    static MSyntax setUpCommonSyntax();

    /// \brief  find the proxy shape node from a valid pre-parsed arg database
    /// \param  args the argument database to get the proxy shape from
    /// \return the proxy shape node (if found in the args)
    nodes::ProxyShape* getShapeNode(const MArgDatabase& args);

    /// \brief  get the USD stage
    /// \param  args the argument database to get the stage from
    /// \return the stage from a proxy shape node (if found in the args)
    UsdStageRefPtr getShapeNodeStage(const MArgDatabase& args);

    //  /// \brief  hunt for the first node of the specified type found in the selection list object
    //  args
    //  /// \param  args the pre-parsed argument data base
    //  /// \param  typeId the typeId of the object type that may appear as one of the selected
    //  objects
    //  /// \return the MObject of the node if found
    //  MObject getSelectedNode(const MArgDatabase& args, const MTypeId typeId);
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Given some selected proxy shape node, return the layer names
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class LayerGetLayers : public LayerCommandBase
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Given some selected or passed in proxy shape node, create a layer in maya parented to
/// the layer
///  matching the identifier passed in, else parents to the rootlayer.
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------

class LayerCreateLayer : public LayerCommandBase
{
    UsdStageRefPtr     m_stage;
    SdfLayerHandle     m_rootLayer;
    SdfLayerRefPtr     m_newLayer;
    MString            m_filePath;
    nodes::ProxyShape* m_shape;
    bool               m_addSublayer = false;
    bool               m_layerWasInserted;

public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus undoIt() override;
    MStatus redoIt() override;
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Save the specified layer
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class LayerSave : public LayerCommandBase
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Export a layer to the given filename
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class LayerExport : public LayerCommandBase
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Get / Set the current edit target for the stage
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class LayerCurrentEditTarget : public LayerCommandBase
{
    LayerCurrentEditTarget() { }
    UsdEditTarget                              previous;
    UsdEditTarget                              next;
    UsdStageRefPtr                             stage;
    bool                                       isQuery;
    std::function<std::string(SdfLayerHandle)> getLayerId;

public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool       isUndoable() const override;
    MStatus    doIt(const MArgList& args) override;
    MStatus    undoIt() override;
    MStatus    redoIt() override;
    PcpNodeRef determineEditTargetMapping(
        UsdStageRefPtr      stage,
        const MArgDatabase& args,
        SdfLayerHandle      editTargetLayer) const;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Get / Set whether the layer is currently muted
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class LayerSetMuted : public LayerCommandBase
{
    SdfLayerHandle m_layer;
    bool           m_muted;

public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
    MStatus undoIt() override;
    MStatus redoIt() override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Command to introspect and pull out data from the layer manager
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class LayerManager : public LayerCommandBase
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
    MStatus captureLayerContents(const MString& id, MStringArray& results);
};

/// \brief  function called on startup to generate the menu & option boxes for the layer commands
/// \ingroup commands
AL_USDMAYA_PUBLIC
void constructLayerCommandGuis();

//----------------------------------------------------------------------------------------------------------------------
} // namespace cmds
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
