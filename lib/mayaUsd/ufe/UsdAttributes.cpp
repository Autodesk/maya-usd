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
#include "UsdAttributes.h"

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/usd/schemaRegistry.h>

#include <ufe/ufeAssert.h>

// Note: normally we would use this using directive, but here we cannot because
//		 one of our classes is called UsdAttribute which is exactly the same as
//		the one in USD.
// PXR_NAMESPACE_USING_DIRECTIVE

#ifdef UFE_ENABLE_ASSERTS
static constexpr char kErrorMsgUnknown[] = "Unknown UFE attribute type encountered";
static constexpr char kErrorMsgInvalidAttribute[] = "Invalid USDAttribute!";
#endif

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdAttributes::UsdAttributes(const UsdSceneItem::Ptr& item)
    : Ufe::Attributes()
    , fItem(item)
{
    fPrim = item->prim();
}

UsdAttributes::~UsdAttributes() { }

/*static*/
UsdAttributes::Ptr UsdAttributes::create(const UsdSceneItem::Ptr& item)
{
    auto attrs = std::make_shared<UsdAttributes>(item);
    return attrs;
}

//------------------------------------------------------------------------------
// Ufe::Attributes overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdAttributes::sceneItem() const { return fItem; }

Ufe::Attribute::Type UsdAttributes::attributeType(const std::string& name)
{
    PXR_NS::TfToken      tok(name);
    PXR_NS::UsdAttribute usdAttr = fPrim.GetAttribute(tok);
    return getUfeTypeForAttribute(usdAttr);
}

Ufe::Attribute::Ptr UsdAttributes::attribute(const std::string& name)
{
    // If we've already created an attribute for this name, just return it.
    auto iter = fAttributes.find(name);
    if (iter != std::end(fAttributes))
        return iter->second;

    // No attribute for the input name was found -> create one.
    PXR_NS::TfToken      tok(name);
    PXR_NS::UsdAttribute usdAttr = fPrim.GetAttribute(tok);
    Ufe::Attribute::Type newAttrType = getUfeTypeForAttribute(usdAttr);
    Ufe::Attribute::Ptr  newAttr;

    if (newAttrType == Ufe::Attribute::kBool) {
        newAttr = UsdAttributeBool::create(fItem, usdAttr);
    } else if (newAttrType == Ufe::Attribute::kInt) {
        newAttr = UsdAttributeInt::create(fItem, usdAttr);
    } else if (newAttrType == Ufe::Attribute::kFloat) {
        newAttr = UsdAttributeFloat::create(fItem, usdAttr);
    } else if (newAttrType == Ufe::Attribute::kDouble) {
        newAttr = UsdAttributeDouble::create(fItem, usdAttr);
    } else if (newAttrType == Ufe::Attribute::kString) {
        newAttr = UsdAttributeString::create(fItem, usdAttr);
    } else if (newAttrType == Ufe::Attribute::kColorFloat3) {
        newAttr = UsdAttributeColorFloat3::create(fItem, usdAttr);
    } else if (newAttrType == Ufe::Attribute::kEnumString) {
        newAttr = UsdAttributeEnumString::create(fItem, usdAttr);
    } else if (newAttrType == Ufe::Attribute::kInt3) {
        newAttr = UsdAttributeInt3::create(fItem, usdAttr);
    } else if (newAttrType == Ufe::Attribute::kFloat3) {
        newAttr = UsdAttributeFloat3::create(fItem, usdAttr);
    } else if (newAttrType == Ufe::Attribute::kDouble3) {
        newAttr = UsdAttributeDouble3::create(fItem, usdAttr);
    } else if (newAttrType == Ufe::Attribute::kGeneric) {
        newAttr = UsdAttributeGeneric::create(fItem, usdAttr);
    } else {
        UFE_ASSERT_MSG(false, kErrorMsgUnknown);
    }
    fAttributes[name] = newAttr;
    return newAttr;
}

std::vector<std::string> UsdAttributes::attributeNames() const
{
    auto                     primAttrs = fPrim.GetAttributes();
    std::vector<std::string> names;
    names.reserve(primAttrs.size());
    for (const auto& attr : primAttrs) {
        names.push_back(attr.GetName());
    }
    return names;
}

bool UsdAttributes::hasAttribute(const std::string& name) const
{
    TfToken tkName(name);
    return fPrim.HasAttribute(tkName);
}

Ufe::Attribute::Type
UsdAttributes::getUfeTypeForAttribute(const PXR_NS::UsdAttribute& usdAttr) const
{
    // Map the USD type into UFE type.
    static const std::
        unordered_map<size_t, Ufe::Attribute::Type>
            sUsdTypeToUfe {
                { SdfValueTypeNames->Bool.GetHash(), Ufe::Attribute::kBool }, // bool
                //		{SdfValueTypeNames->UChar.GetHash(), Ufe::Attribute::kUnknown},
                //// uint8_t
                { SdfValueTypeNames->Int.GetHash(), Ufe::Attribute::kInt }, // int32_t
                //		{SdfValueTypeNames->UInt.GetHash(), Ufe::Attribute::kUnknown},
                //// uint32_t
                //		{SdfValueTypeNames->Int64.GetHash(), Ufe::Attribute::kInt},
                //// int64_t 		{SdfValueTypeNames->UInt64.GetHash(), Ufe::Attribute::kUnknown},
                //// uint64_t
                //		{SdfValueTypeNames->Half.GetHash(), Ufe::Attribute::kUnknown},
                //// GfHalf
                { SdfValueTypeNames->Float.GetHash(), Ufe::Attribute::kFloat },      // float
                { SdfValueTypeNames->Double.GetHash(), Ufe::Attribute::kDouble },    // double
                { SdfValueTypeNames->String.GetHash(), Ufe::Attribute::kString },    // std::string
                { SdfValueTypeNames->Token.GetHash(), Ufe::Attribute::kEnumString }, // TfToken
                //		{SdfValueTypeNames->Asset.GetHash(), Ufe::Attribute::kUnknown},
                //// SdfAssetPath 		{SdfValueTypeNames->Int2.GetHash(), Ufe::Attribute::kInt2},
                //// GfVec2i 		{SdfValueTypeNames->Half2.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec2h 		{SdfValueTypeNames->Float2.GetHash(), Ufe::Attribute::kFloat2},
                //// GfVec2f 		{SdfValueTypeNames->Double2.GetHash(), Ufe::Attribute::kDouble2},
                //// GfVec2d
                { SdfValueTypeNames->Int3.GetHash(), Ufe::Attribute::kInt3 }, // GfVec3i
                //		{SdfValueTypeNames->Half3.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec3h
                { SdfValueTypeNames->Float3.GetHash(), Ufe::Attribute::kFloat3 },   // GfVec3f
                { SdfValueTypeNames->Double3.GetHash(), Ufe::Attribute::kDouble3 }, // GfVec3d
                //		{SdfValueTypeNames->Int4.GetHash(), Ufe::Attribute::kInt4},
                //// GfVec4i 		{SdfValueTypeNames->Half4.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec4h 		{SdfValueTypeNames->Float4.GetHash(), Ufe::Attribute::kFloat4},
                //// GfVec4f 		{SdfValueTypeNames->Double4.GetHash(), Ufe::Attribute::kDouble4},
                //// GfVec4d 		{SdfValueTypeNames->Point3h.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec3h 		{SdfValueTypeNames->Point3f.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec3f 		{SdfValueTypeNames->Point3d.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec3d 		{SdfValueTypeNames->Vector3h.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec3h 		{SdfValueTypeNames->Vector3f.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec3f 		{SdfValueTypeNames->Vector3d.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec3d 		{SdfValueTypeNames->Normal3h.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec3h 		{SdfValueTypeNames->Normal3f.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec3f 		{SdfValueTypeNames->Normal3d.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec3d 		{SdfValueTypeNames->Color3h.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec3h
                { SdfValueTypeNames->Color3f.GetHash(), Ufe::Attribute::kColorFloat3 }, // GfVec3f
                { SdfValueTypeNames->Color3d.GetHash(), Ufe::Attribute::kColorFloat3 }, // GfVec3d
                //		{SdfValueTypeNames->Color4h.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec4h 		{SdfValueTypeNames->Color4f.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec4f 		{SdfValueTypeNames->Color4d.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec4d 		{SdfValueTypeNames->Quath.GetHash(), Ufe::Attribute::kUnknown},
                //// GfQuath 		{SdfValueTypeNames->Quatf.GetHash(), Ufe::Attribute::kUnknown},
                //// GfQuatf 		{SdfValueTypeNames->Quatd.GetHash(), Ufe::Attribute::kUnknown},
                //// GfQuatd 		{SdfValueTypeNames->Matrix2d.GetHash(), Ufe::Attribute::kUnknown},
                //// GfMatrix2d 		{SdfValueTypeNames->Matrix3d.GetHash(), Ufe::Attribute::kUnknown},
                //// GfMatrix3d 		{SdfValueTypeNames->Matrix4d.GetHash(), Ufe::Attribute::kUnknown},
                //// GfMatrix4d 		{SdfValueTypeNames->Frame4d.GetHash(), Ufe::Attribute::kUnknown},
                //// GfMatrix4d 		{SdfValueTypeNames->TexCoord2f.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec2f 		{SdfValueTypeNames->TexCoord2d.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec2d 		{SdfValueTypeNames->TexCoord2h.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec2h 		{SdfValueTypeNames->TexCoord3f.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec3f 		{SdfValueTypeNames->TexCoord3d.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec3d 		{SdfValueTypeNames->TexCoord3h.GetHash(), Ufe::Attribute::kUnknown},
                //// GfVec3h
                // To add?
                // SdfValueTypeName BoolArray;
                // SdfValueTypeName UCharArray, IntArray, UIntArray, Int64Array, UInt64Array;
                // SdfValueTypeName HalfArray, FloatArray, DoubleArray;
                // SdfValueTypeName StringArray, TokenArray, AssetArray;
                // SdfValueTypeName Int2Array,     Int3Array,     Int4Array;
                // SdfValueTypeName Half2Array,    Half3Array,    Half4Array;
                // SdfValueTypeName Float2Array,   Float3Array,   Float4Array;
                // SdfValueTypeName Double2Array,  Double3Array,  Double4Array;
                // SdfValueTypeName Point3hArray,  Point3fArray,  Point3dArray;
                // SdfValueTypeName Vector3hArray, Vector3fArray, Vector3dArray;
                // SdfValueTypeName Normal3hArray, Normal3fArray, Normal3dArray;
                // SdfValueTypeName Color3hArray,  Color3fArray,  Color3dArray;
                // SdfValueTypeName Color4hArray,  Color4fArray,  Color4dArray;
                // SdfValueTypeName QuathArray,    QuatfArray,    QuatdArray;
                // SdfValueTypeName Matrix2dArray, Matrix3dArray, Matrix4dArray;
                // SdfValueTypeName Frame4dArray;
                // SdfValueTypeName TexCoord2hArray, TexCoord2fArray, TexCoord2dArray;
                // SdfValueTypeName TexCoord3hArray, TexCoord3fArray, TexCoord3dArray;
            };

    if (usdAttr.IsValid()) {
        const PXR_NS::SdfValueTypeName typeName = usdAttr.GetTypeName();
        const auto                     iter = sUsdTypeToUfe.find(typeName.GetHash());

        // ** TEMP - for debugging purposes only
        // std::string cppName = typeName.GetCPPTypeName();

        if (iter != sUsdTypeToUfe.end()) {
            // Special case for TfToken -> Enum. If it doesn't have any allowed
            // tokens, then use String instead.
            if (iter->second == Ufe::Attribute::kEnumString) {
#if USD_VERSION_NUM > 2002
                auto attrDefn = fPrim.GetPrimDefinition().GetSchemaAttributeSpec(usdAttr.GetName());
#else
                auto attrDefn = PXR_NS::UsdSchemaRegistry::GetAttributeDefinition(
                    fPrim.GetTypeName(), usdAttr.GetName());
#endif
                if (!attrDefn || !attrDefn->HasAllowedTokens())
                    return Ufe::Attribute::kString;
            }

            return iter->second;
        }

        // We use the generic type to show a Usd attribute's value and native type.
        return Ufe::Attribute::kGeneric;
    }

    UFE_ASSERT_MSG(false, kErrorMsgInvalidAttribute);
    return Ufe::Attribute::kInvalid;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
