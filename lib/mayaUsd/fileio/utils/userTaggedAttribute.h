//
// Copyright 2016 Pixar
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
#ifndef PXRUSDMAYA_USER_TAGGED_ATTRIBUTE_H
#define PXRUSDMAYA_USER_TAGGED_ATTRIBUTE_H

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/iterator.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

#include <maya/MObject.h>
#include <maya/MPlug.h>

#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
#define PXRUSDMAYA_ATTR_TOKENS \
    ((USDAttrTypePrimvar, "primvar")) \
    ((USDAttrTypeUsdRi, "usdRi"))
// clang-format on

TF_DECLARE_PUBLIC_TOKENS(
    UsdMayaUserTaggedAttributeTokens,
    MAYAUSD_CORE_PUBLIC,
    PXRUSDMAYA_ATTR_TOKENS);

/// \brief Represents a single attribute tagged for translation between Maya
/// and USD, and describes how it will be exported from/imported into Maya.
class UsdMayaUserTaggedAttribute
{
private:
    MPlug             _plug;
    const std::string _name;
    const TfToken     _type;
    const TfToken     _interpolation;
    const bool        _translateMayaDoubleToUsdSinglePrecision;

public:
    /// \brief Gets the fallback value for whether attribute types should be
    /// mapped from double precision types in Maya to single precision types in
    /// USD.
    /// By default, the fallback value for this property is false so that
    /// double precision data is preserved in the translation back and forth
    /// between Maya and USD.
    static bool GetFallbackTranslateMayaDoubleToUsdSinglePrecision() { return false; };

    MAYAUSD_CORE_PUBLIC
    UsdMayaUserTaggedAttribute(
        const MPlug&       plug,
        const std::string& name,
        const TfToken&     type,
        const TfToken&     interpolation,
        const bool         translateMayaDoubleToUsdSinglePrecision
        = GetFallbackTranslateMayaDoubleToUsdSinglePrecision());

    /// \brief Gets all of the exported attributes for the given node.
    MAYAUSD_CORE_PUBLIC
    static std::vector<UsdMayaUserTaggedAttribute>
    GetUserTaggedAttributesForNode(const MObject& mayaNode);

    /// \brief Gets the plug for the Maya attribute to be exported.
    MAYAUSD_CORE_PUBLIC
    MPlug GetMayaPlug() const;

    /// \brief Gets the name of the Maya attribute that will be exported;
    /// the name will not contain the name of the node.
    MAYAUSD_CORE_PUBLIC
    std::string GetMayaName() const;

    /// \brief Gets the name of the USD attribute to which the Maya attribute
    /// will be exported.
    MAYAUSD_CORE_PUBLIC
    std::string GetUsdName() const;

    /// \brief Gets the type of the USD attribute to export: whether it is a
    /// regular attribute, primvar, etc.
    MAYAUSD_CORE_PUBLIC
    TfToken GetUsdType() const;

    /// \brief Gets the interpolation for primvars.
    MAYAUSD_CORE_PUBLIC
    TfToken GetUsdInterpolation() const;

    /// \brief Gets whether the attribute type should be mapped from a double
    /// precision type in Maya to a single precision type in USD.
    /// There is not always a direct mapping between Maya-native types and
    /// USD/Sdf types, and often it's desirable to intentionally use a single
    /// precision type when the extra precision is not needed to reduce size,
    /// I/O bandwidth, etc.
    ///
    /// For example, there is no native Maya attribute type to represent an
    /// array of float triples. To get an attribute with a VtVec3fArray type
    /// in USD, you can create a 'vectorArray' data-typed attribute in Maya
    /// (which stores MVectors, which contain doubles) and set
    /// 'translateMayaDoubleToUsdSinglePrecision' to true so that the data is
    /// cast to single-precision on export. It will be up-cast back to double
    /// on re-import.
    MAYAUSD_CORE_PUBLIC
    bool GetTranslateMayaDoubleToUsdSinglePrecision() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
