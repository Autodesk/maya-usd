//
// Copyright 2022 Autodesk
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

#include <mayaUsd/base/api.h>

#include <pxr/usd/sdr/shaderNode.h>

#include <ufe/nodeDef.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdShaderNodeDef interface.
class MAYAUSD_CORE_PUBLIC UsdShaderNodeDef : public Ufe::NodeDef
{
public:
    typedef std::shared_ptr<UsdShaderNodeDef> Ptr;

    static constexpr char kNodeDefCategoryShader[] = "Shader";

    UsdShaderNodeDef(const PXR_NS::SdrShaderNodeConstPtr& shaderNodeDef);
    ~UsdShaderNodeDef();

    // Delete the copy/move constructors assignment operators.
    UsdShaderNodeDef(const UsdShaderNodeDef&) = delete;
    UsdShaderNodeDef& operator=(const UsdShaderNodeDef&) = delete;
    UsdShaderNodeDef(UsdShaderNodeDef&&) = delete;
    UsdShaderNodeDef& operator=(UsdShaderNodeDef&&) = delete;

#if (UFE_PREVIEW_VERSION_NUM < 4010)
    //! \return The type of the shader node definition.
    const std::string& type() const override;

    //! \return The inputs of the shader node definition.
    const Ufe::ConstAttributeDefs& inputs() const override;

    //! \return The outputs of the shader node definition.
    const Ufe::ConstAttributeDefs& outputs() const override;
#else
    //! \return The type of the shader node definition.
    std::string type() const override;

    /*!
        Queries the number of classification levels available for this node.
        This can vary across runtimes. A biology implementation would have
        species as the "type" and genus, family, order, class, phylum, kingdom
        representing the 6 available levels.
        \return The number of classification levels.
    */
    std::size_t nbClassifications() const override;

    /*!
        Gets the classification label applicable to this NodeNef for the
        requested classification level. The most precise classification level
        corresponds to level zero.
        \param level The classification level to query.
        \return The classification label for this node at this level.
    */
    std::string classification(std::size_t level) const override;

    //! \return List of all the input names for this node definition.
    std::vector<std::string> inputNames() const override;

    /*!
        Queries whether an input exists with the given name.
        \param name The input name to check.
        \return True if the object contains an input matching the name.
    */
    bool hasInput(const std::string& name) const override;

    /*!
        Creates an AttributeDef interface for the given input name.
        \param name Name of the input to retrieve.
        \return AttributeDef interface for the given name. Returns a null
        pointer if no input exists for the given name.
    */
    Ufe::AttributeDef::ConstPtr input(const std::string& name) const override;

    //! \return The inputs of the shader node definition.
    Ufe::ConstAttributeDefs inputs() const override;

    //! \return List of all the output names for this node definition.
    std::vector<std::string> outputNames() const override;

    /*!
        Queries whether an output exists with the given name.
        \param name The output name to check.
        \return True if the object contains an output matching the name.
    */
    bool hasOutput(const std::string& name) const override;

    /*!
        Creates an AttributeDef interface for the given output name.
        \param name Name of the output to retrieve.
        \return AttributeDef interface for the given name. Returns a null
        pointer if no output exists for the given name.
    */
    Ufe::AttributeDef::ConstPtr output(const std::string& name) const override;

    //! \return The outputs of the shader node definition.
    Ufe::ConstAttributeDefs outputs() const override;

    /*!
        Get the value of the metadata named key.
        \param[in] key The metadata key to query.
        \return The value of the metadata key. If the key does not exist an empty Value is returned.
    */
    Ufe::Value getMetadata(const std::string& key) const override;

    //! Returns true if metadata key has a non-empty value.
    bool hasMetadata(const std::string& key) const override;

    //! Create a SceneItem using the current node definition as template.
    //! \param parent Item under which the node is to be created.
    //! \param name   Name of the new node.
    //! \return SceneItem for the created node, at its new path.
    Ufe::SceneItem::Ptr
    createNode(const Ufe::SceneItem::Ptr& parent, const Ufe::PathComponent& name) const override;

    //! Create a command to create a SceneItem using the current node definition
    //! as template. The command is not executed.
    //! \param parent Item under which the node is to be created.
    //! \param name   Name of the new node.
    //! \return Command whose execution will create the node.
    Ufe::InsertChildCommand::Ptr
    createNodeCmd(const Ufe::SceneItem::Ptr& parent, const Ufe::PathComponent& name) const override;
#endif

    //! Create a UsdShaderNodeDef.
    static Ptr create(const PXR_NS::SdrShaderNodeConstPtr& shaderNodeDef);

    //! Returns the node definitions that match the provided category.
    static Ufe::NodeDefs definitions(const std::string& category);

private:
#if (UFE_PREVIEW_VERSION_NUM < 4010)
    const std::string fType;
#endif
    const PXR_NS::SdrShaderNodeConstPtr fShaderNodeDef;
#if (UFE_PREVIEW_VERSION_NUM < 4010)
    const Ufe::ConstAttributeDefs fInputs;
    const Ufe::ConstAttributeDefs fOutputs;
#endif

}; // UsdShaderNodeDef

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
