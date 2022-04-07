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

    //! \return The type of the shader node definition.
    const std::string& type() const override;

    //! \return The inputs of the shader node definition.
    const Ufe::ConstAttributeDefs& inputs() const override;

    //! \return The outputs of the shader node definition.
    const Ufe::ConstAttributeDefs& outputs() const override;

    //! Create a UsdShaderNodeDef.
    static Ptr create(const PXR_NS::SdrShaderNodeConstPtr& shaderNodeDef);

    //! Returns the node definitions that match the provided category.
    static Ufe::NodeDefs definitions(const std::string& category);

private:
    const std::string                   fType;
    const PXR_NS::SdrShaderNodeConstPtr fShaderNodeDef;
    const Ufe::ConstAttributeDefs       fInputs;
    const Ufe::ConstAttributeDefs       fOutputs;

}; // UsdShaderNodeDef

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
