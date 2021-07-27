//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYA_READUTIL_H
#define PXRUSDMAYA_READUTIL_H

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/usd/attribute.h>

#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// This struct contains helpers for reading USD (thus writing Maya data).
struct UsdMayaReadUtil
{
    /// \name Helpers for reading USD
    /// \{

    /// Returns whether the environment setting for reading Float2 types as UV
    /// sets is set to true
    MAYAUSD_CORE_PUBLIC
    static bool ReadFloat2AsUV();

    /// Given the \p typeName and \p variability, try to create a Maya attribute
    /// on \p depNode with the name \p attrName.
    /// If the \p typeName isn't supported by this function, raises a runtime
    /// error (this function supports the majority of, but not all, type names).
    /// If the attribute already exists and its type information matches, then
    /// its object is returned. If the attribute already exists but its type
    /// information is conflicting, then returns null and posts a runtime error.
    /// If the attribute doesn't exist yet, then creates it and returns the
    /// attribute object.
    MAYAUSD_CORE_PUBLIC
    static MObject FindOrCreateMayaAttr(
        const SdfValueTypeName& typeName,
        const SdfVariability    variability,
        MFnDependencyNode&      depNode,
        const std::string&      attrName,
        const std::string&      attrNiceName = std::string());

    /// An overload of FindOrCreateMayaAttr that takes an MDGModifier.
    /// \note This function will call doIt() on the MDGModifier; thus the
    /// actions will have been committed when the function returns.
    MAYAUSD_CORE_PUBLIC
    static MObject FindOrCreateMayaAttr(
        const SdfValueTypeName& typeName,
        const SdfVariability    variability,
        MFnDependencyNode&      depNode,
        const std::string&      attrName,
        const std::string&      attrNiceName,
        MDGModifier&            modifier);

    MAYAUSD_CORE_PUBLIC
    static MObject FindOrCreateMayaAttr(
        const TfType&        type,
        const TfToken&       role,
        const SdfVariability variability,
        MFnDependencyNode&   depNode,
        const std::string&   attrName,
        const std::string&   attrNiceName,
        MDGModifier&         modifier);

    /// Sets a Maya plug using the value on a USD attribute at default time.
    /// If the variability of the USD attribute doesn't match the keyable state
    /// of the Maya plug, then the plug's keyable state will also be updated.
    /// Returns true if the plug was set.
    ///
    /// For plugs with color roles, the value will be converted from a linear
    /// color value before being set if \p unlinearizeColors is true.
    MAYAUSD_CORE_PUBLIC
    static bool
    SetMayaAttr(MPlug& attrPlug, const UsdAttribute& usdAttr, const bool unlinearizeColors = true);

    /// Sets a Maya plug using the given VtValue. The plug keyable state won't
    /// be affected.
    /// Returns true if the plug was set.
    ///
    /// For plugs with color roles, the value will be converted from a linear
    /// color value before being set if \p unlinearizeColors is true.
    MAYAUSD_CORE_PUBLIC
    static bool
    SetMayaAttr(MPlug& attrPlug, const VtValue& newValue, const bool unlinearizeColors = true);

    /// An overload of SetMayaAttr that takes an MDGModifier.
    /// \note This function will call doIt() on the MDGModifier; thus the
    /// actions will have been committed when the function returns.
    ///
    /// For plugs with color roles, the value will be converted from a linear
    /// color value before being set if \p unlinearizeColors is true.
    MAYAUSD_CORE_PUBLIC
    static bool SetMayaAttr(
        MPlug&         attrPlug,
        const VtValue& newValue,
        MDGModifier&   modifier,
        const bool     unlinearizeColors = true);

    /// Sets the plug's keyable state based on whether the variability is
    /// varying or uniform.
    MAYAUSD_CORE_PUBLIC
    static void SetMayaAttrKeyableState(MPlug& attrPlug, const SdfVariability variability);

    /// An overload of SetMayaAttrKeyableState that takes an MDGModifier.
    /// \note This function will call doIt() on the MDGModifier; thus the
    /// actions will have been committed when the function returns.
    MAYAUSD_CORE_PUBLIC
    static void SetMayaAttrKeyableState(
        MPlug&               attrPlug,
        const SdfVariability variability,
        MDGModifier&         modifier);

    /// \}

    /// \name Auto-importing API schemas and metadata
    /// \{

    /// Reads the metadata specified in \p includeMetadataKeys from \p prim, and
    /// uses adaptors to write them onto attributes of \p mayaObject.
    /// Returns true if successful (even if there was nothing to import).
    MAYAUSD_CORE_PUBLIC
    static bool ReadMetadataFromPrim(
        const TfToken::Set& includeMetadataKeys,
        const UsdPrim&      prim,
        const MObject&      mayaObject);

    /// Reads the attributes from the non-excluded schemas applied to \p prim,
    /// and uses adaptors to write them onto attributes of \p mayaObject.
    /// Returns true if successful (even if there was nothing to import).
    /// \note If the schema wasn't applied using the schema class's Apply()
    /// function, then this function won't recognize it.
    MAYAUSD_CORE_PUBLIC
    static bool ReadAPISchemaAttributesFromPrim(
        const TfToken::Set& includeAPINames,
        const UsdPrim&      prim,
        const MObject&      mayaObject);

    /// \}

    /// \name Manually importing typed schema data
    /// \{

    template <typename T>
    static size_t ReadSchemaAttributesFromPrim(
        const UsdPrim&              prim,
        const MObject&              mayaObject,
        const std::vector<TfToken>& attributeNames,
        const UsdTimeCode&          usdTime = UsdTimeCode::Default())
    {
        return ReadSchemaAttributesFromPrim(
            prim, mayaObject, TfType::Find<T>(), attributeNames, usdTime);
    }

    /// Reads schema attributes specified by \attributeNames for the schema
    /// with type \p schemaType, storing them as adapted attributes on
    /// \p mayaObject. Attributes that are unauthored in USD (only have their
    /// fallback value) will be skipped.
    /// Values are read from the stage at \p usdTime, and are stored on the
    /// Maya node as unanimated values. If the optional \p valueWriter is
    /// provided, it will be used to write the values.
    /// Returns the number of attributes that were read into Maya.
    static size_t ReadSchemaAttributesFromPrim(
        const UsdPrim&              prim,
        const MObject&              mayaObject,
        const TfType&               schemaType,
        const std::vector<TfToken>& attributeNames,
        const UsdTimeCode&          usdTime = UsdTimeCode::Default());

    /// \}

    /// A cache to store pre-computed file texture hashes on import.
    MAYAUSD_CORE_PUBLIC
    static std::unordered_map<std::string, uint64_t> mapFileHashes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
