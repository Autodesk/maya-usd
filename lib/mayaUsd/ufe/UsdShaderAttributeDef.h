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

#include <pxr/usd/sdr/shaderproperty.h>

#include <ufe/attributeDef.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdShaderAttributeDef interface.
class MAYAUSD_CORE_PUBLIC UsdShaderAttributeDef : public Ufe::AttributeDef
{
public:
    typedef std::shared_ptr<UsdShaderAttributeDef> Ptr;

    UsdShaderAttributeDef(const PXR_NS::SdrShaderPropertyConstPtr& shaderAttributeDef);
    ~UsdShaderAttributeDef();

    // Delete the copy/move constructors assignment operators.
    UsdShaderAttributeDef(const UsdShaderAttributeDef&) = delete;
    UsdShaderAttributeDef& operator=(const UsdShaderAttributeDef&) = delete;
    UsdShaderAttributeDef(UsdShaderAttributeDef&&) = delete;
    UsdShaderAttributeDef& operator=(UsdShaderAttributeDef&&) = delete;

    //! \return The attributeDef's name.
    std::string name() const override;

    //! \return The attributeDef's type.
    std::string type() const override;

    //! \return The string representation of the attributeDef's value.
    std::string defaultValue() const override;

    //! \return The attribute input/output type.
    IOType ioType() const override;

    /*!
        Get the value of the metadata named key.
        \param[in] key The metadata key to query.
        \return The value of the metadata key. If the key does not exist an empty Value is returned.
    */
    Ufe::Value getMetadata(const std::string& key) const override;

    //! Returns true if metadata key has a non-empty value.
    bool hasMetadata(const std::string& key) const override;

private:
    const PXR_NS::SdrShaderPropertyConstPtr fShaderAttributeDef;

}; // UsdShaderAttributeDef

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
