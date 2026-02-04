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
#ifndef USDUFE_USDATTRIBUTE_H
#define USDUFE_USDATTRIBUTE_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdAttributeHolder.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/attribute.h>
#ifdef UFE_V4_FEATURES_AVAILABLE
#include <ufe/attributeDef.h>
#endif

// Ufe::Attribute overrides (minus the type method)
#ifdef UFE_HAS_DISPLAY_NAME
#define UFE_V5_ATTRIBUTE_OVERRIDES \
    std::string displayName() const override { return UsdAttribute::_displayName(); }
#else
#define UFE_V5_ATTRIBUTE_OVERRIDES
#endif

#ifdef UFE_V4_FEATURES_AVAILABLE
#define UFE_ATTRIBUTE_OVERRIDES                                                               \
    bool        hasValue() const override { return UsdAttribute::_hasValue(); }               \
    std::string name() const override { return UsdAttribute::_name(); }                       \
    UFE_V5_ATTRIBUTE_OVERRIDES                                                                \
    std::string documentation() const override { return UsdAttribute::_documentation(); }     \
    std::string string() const override                                                       \
    {                                                                                         \
        return UsdAttribute::_string(Ufe::Attribute::sceneItem());                            \
    }                                                                                         \
    Ufe::Value getMetadata(const std::string& key) const override                             \
    {                                                                                         \
        return UsdAttribute::_getMetadata(key);                                               \
    }                                                                                         \
    bool setMetadata(const std::string& key, const Ufe::Value& value) override                \
    {                                                                                         \
        return UsdAttribute::_setMetadata(key, value);                                        \
    }                                                                                         \
    Ufe::UndoableCommand::Ptr setMetadataCmd(const std::string& key, const Ufe::Value& value) \
        override                                                                              \
    {                                                                                         \
        return UsdAttribute::_setMetadataCmd(key, value);                                     \
    }                                                                                         \
    bool clearMetadata(const std::string& key) override                                       \
    {                                                                                         \
        return UsdAttribute::_clearMetadata(key);                                             \
    }                                                                                         \
    bool hasMetadata(const std::string& key) const override                                   \
    {                                                                                         \
        return UsdAttribute::_hasMetadata(key);                                               \
    }
#else
#ifdef UFE_V3_FEATURES_AVAILABLE
#define UFE_ATTRIBUTE_OVERRIDES                                                               \
    bool        hasValue() const override { return UsdAttribute::hasValue(); }                \
    std::string name() const override { return UsdAttribute::name(); }                        \
    std::string documentation() const override { return UsdAttribute::documentation(); }      \
    std::string string() const override                                                       \
    {                                                                                         \
        return UsdAttribute::string(Ufe::Attribute::sceneItem());                             \
    }                                                                                         \
    Ufe::Value getMetadata(const std::string& key) const override                             \
    {                                                                                         \
        return UsdAttribute::getMetadata(key);                                                \
    }                                                                                         \
    bool setMetadata(const std::string& key, const Ufe::Value& value) override                \
    {                                                                                         \
        return UsdAttribute::setMetadata(key, value);                                         \
    }                                                                                         \
    Ufe::UndoableCommand::Ptr setMetadataCmd(const std::string& key, const Ufe::Value& value) \
        override                                                                              \
    {                                                                                         \
        return UsdAttribute::setMetadataCmd(key, value);                                      \
    }                                                                                         \
    bool clearMetadata(const std::string& key) override                                       \
    {                                                                                         \
        return UsdAttribute::clearMetadata(key);                                              \
    }                                                                                         \
    bool hasMetadata(const std::string& key) const override                                   \
    {                                                                                         \
        return UsdAttribute::hasMetadata(key);                                                \
    }
#else
#define UFE_ATTRIBUTE_OVERRIDES                                                          \
    bool        hasValue() const override { return UsdAttribute::hasValue(); }           \
    std::string name() const override { return UsdAttribute::name(); }                   \
    std::string documentation() const override { return UsdAttribute::documentation(); } \
    std::string string() const override                                                  \
    {                                                                                    \
        return UsdAttribute::string(Ufe::Attribute::sceneItem());                        \
    }
#endif
#endif

namespace USDUFE_NS_DEF {

//! \brief Internal helper class to implement the pure virtual methods from Ufe::Attribute.
class USDUFE_PUBLIC UsdAttribute
{
public:
    UsdAttribute(UsdAttributeHolder::UPtr&& attrHolder);
    ~UsdAttribute() = default;

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdAttribute);

    inline bool isAuthored() const { return _attrHolder->isAuthored(); }
    inline bool isValid() const { return _attrHolder->isValid(); }
    std::string isEditAllowedMsg() const;
    inline bool isEditAllowed() const { return isEditAllowedMsg().empty(); }
    std::string defaultValue() const;
    std::string nativeType() const;
    bool        get(PXR_NS::VtValue& value, PXR_NS::UsdTimeCode time) const;
    bool        set(const PXR_NS::VtValue& value, PXR_NS::UsdTimeCode time);

#ifdef UFE_DEFAULT_VALUE_SUPPORT
    bool _isDefault() const;
    void _reset();
#endif
#ifdef UFE_V4_FEATURES_AVAILABLE
    bool        _hasValue() const;
    std::string _name() const;
    std::string _displayName() const;
    std::string _documentation() const;
    std::string _string(const Ufe::SceneItem::Ptr& item) const;

    Ufe::Value                _getMetadata(const std::string& key) const;
    bool                      _setMetadata(const std::string& key, const Ufe::Value& value);
    Ufe::UndoableCommand::Ptr _setMetadataCmd(const std::string& key, const Ufe::Value& value);
    bool                      _clearMetadata(const std::string& key);
    bool                      _hasMetadata(const std::string& key) const;
#else
    // Ufe::Attribute override methods that we've mimic'd here.
    bool                      hasValue() const;
    std::string               name() const;
    std::string               displayName() const;
    std::string               documentation() const;
    std::string               string(const Ufe::SceneItem::Ptr& item) const;
#ifdef UFE_V3_FEATURES_AVAILABLE
    Ufe::Value                getMetadata(const std::string& key) const;
    bool                      setMetadata(const std::string& key, const Ufe::Value& value);
    Ufe::UndoableCommand::Ptr setMetadataCmd(const std::string& key, const Ufe::Value& value);
    bool                      clearMetadata(const std::string& key);
    bool                      hasMetadata(const std::string& key) const;
#endif
#endif

    PXR_NS::UsdPrim          usdPrim() const { return _attrHolder->usdPrim(); }
    PXR_NS::UsdAttribute     usdAttribute() const { return _attrHolder->usdAttribute(); }
    PXR_NS::SdfValueTypeName usdAttributeType() const;
    Ufe::AttributeEnumString::EnumValues getEnumValues() const
    {
        return _attrHolder->getEnumValues();
    }

protected:
    UsdAttributeHolder::UPtr _attrHolder;
}; // UsdAttribute

//! \brief Interface for USD attributes which don't match any defined type.
class UsdAttributeGeneric
    : public Ufe::AttributeGeneric
#ifdef UFE_V4_FEATURES_AVAILABLE
    , public UsdAttribute
#else
    , private UsdAttribute
#endif
{
public:
    typedef std::shared_ptr<UsdAttributeGeneric> Ptr;

    UsdAttributeGeneric(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);

    //! Create a UsdAttributeGeneric with a scene item and USD attribute
    static UsdAttributeGeneric::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);

    // Ufe::Attribute overrides
    UFE_ATTRIBUTE_OVERRIDES

    // Ufe::AttributeGeneric overrides
    std::string nativeType() const override;

    // Metadata used when creating a dynamic attribute on NodeGraph/Material boundaries that
    // remembers the native type of a generic shader property.
    static const std::string& nativeSdrTypeMetadata();
}; // UsdAttributeGeneric

#ifdef UFE_DEFAULT_VALUE_SUPPORT
#define UFE_DEFAULT_VALUE_OVERRIDES                                        \
    bool isDefault() const override { return UsdAttribute::_isDefault(); } \
    void reset() override { UsdAttribute::_reset(); }
#else
#define UFE_DEFAULT_VALUE_OVERRIDES
#endif

#ifdef UFE_V4_FEATURES_AVAILABLE
//! \brief Interface for USD token attributes.
class UsdAttributeFilename
    : public Ufe::AttributeFilename
    , public UsdAttribute
{
public:
    typedef std::shared_ptr<UsdAttributeFilename> Ptr;

    UsdAttributeFilename(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);

    //! Create a UsdAttributeFilename with a scene item and USD attribute
    static UsdAttributeFilename::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);

    // Ufe::Attribute overrides
    UFE_ATTRIBUTE_OVERRIDES

    // Ufe default value overrides
    UFE_DEFAULT_VALUE_OVERRIDES

    // Ufe::AttributeFilename overrides
    std::string               get() const override;
    void                      set(const std::string& value) override;
    Ufe::UndoableCommand::Ptr setCmd(const std::string& value) override;
}; // UsdAttributeFilename
#endif

//! \brief Interface for enumerated USD string attributes.
class UsdAttributeEnumString
    : public Ufe::AttributeEnumString
#ifdef UFE_V4_FEATURES_AVAILABLE
    , public UsdAttribute
#else
    , private UsdAttribute
#endif
{
public:
    typedef std::shared_ptr<UsdAttributeEnumString> Ptr;

    UsdAttributeEnumString(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);

    //! Create a UsdAttributeEnumString with a scene item and USD attribute
    static UsdAttributeEnumString::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);

    // Ufe::Attribute overrides
    UFE_ATTRIBUTE_OVERRIDES

    // Ufe default value overrides
    UFE_DEFAULT_VALUE_OVERRIDES

    // Ufe::AttributeEnumString overrides
    std::string               get() const override;
    void                      set(const std::string& value) override;
    Ufe::UndoableCommand::Ptr setCmd(const std::string& value) override;
    EnumValues                getEnumValues() const override;
}; // UsdAttributeEnumString

//! \brief Interface for enumerated USD TfToken attributes.
class UsdAttributeEnumToken
    : public Ufe::AttributeEnumString
#ifdef UFE_V4_FEATURES_AVAILABLE
    , public UsdAttribute
#else
    , private UsdAttribute
#endif
{
public:
    typedef std::shared_ptr<UsdAttributeEnumToken> Ptr;

    UsdAttributeEnumToken(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);

    //! Create a UsdAttributeEnumToken with a scene item and USD attribute
    static UsdAttributeEnumToken::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);

    // Ufe::Attribute overrides
    UFE_ATTRIBUTE_OVERRIDES

    // Ufe default value overrides
    UFE_DEFAULT_VALUE_OVERRIDES

    // Ufe::AttributeEnumString overrides
    std::string               get() const override;
    void                      set(const std::string& value) override;
    Ufe::UndoableCommand::Ptr setCmd(const std::string& value) override;
    EnumValues                getEnumValues() const override;
}; // UsdAttributeEnumToken

//! \brief Internal helper template class to implement the get/set methods from Ufe::TypeAttribute.
template <typename T>
class TypedUsdAttribute
    : public Ufe::TypedAttribute<T>
#ifdef UFE_V4_FEATURES_AVAILABLE
    , public UsdAttribute
#else
    , private UsdAttribute
#endif
{
public:
    //! Create a TypedUsdAttribute with a scene item and USD attribute
    TypedUsdAttribute(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);

    // Ufe::Attribute overrides
    UFE_ATTRIBUTE_OVERRIDES

    // Ufe default value overrides
    UFE_DEFAULT_VALUE_OVERRIDES

    // Ufe::TypedAttribute overrides
    T                         get() const override;
    void                      set(const T& value) override;
    Ufe::UndoableCommand::Ptr setCmd(const T& value) override;
}; // TypedUsdAttribute

//! \brief Interface for USD bool attributes.
class UsdAttributeBool : public TypedUsdAttribute<bool>
{
public:
    typedef std::shared_ptr<UsdAttributeBool> Ptr;

    using TypedUsdAttribute<bool>::TypedUsdAttribute;

    //! Create a UsdAttributeBool with a scene item and USD attribute
    static UsdAttributeBool::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);
}; // UsdAttributeBool

//! \brief Interface for USD int attributes.
class UsdAttributeInt : public TypedUsdAttribute<int>
{
public:
    typedef std::shared_ptr<UsdAttributeInt> Ptr;

    using TypedUsdAttribute<int>::TypedUsdAttribute;

    //! Create a UsdAttributeInt with a scene item and USD attribute
    static UsdAttributeInt::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);
}; // UsdAttributeInt

#ifdef UFE_HAS_UNSIGNED_INT
//! \brief Interface for USD unsigned int attributes.
class UsdAttributeUInt : public TypedUsdAttribute<unsigned int>
{
public:
    typedef std::shared_ptr<UsdAttributeUInt> Ptr;

    using TypedUsdAttribute<unsigned int>::TypedUsdAttribute;

    //! Create a UsdAttributeUInt with a scene item and USD attribute
    static UsdAttributeUInt::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);
}; // UsdAttributeUInt
#endif

//! \brief Interface for USD float attributes.
class UsdAttributeFloat : public TypedUsdAttribute<float>
{
public:
    typedef std::shared_ptr<UsdAttributeFloat> Ptr;

    using TypedUsdAttribute<float>::TypedUsdAttribute;

    //! Create a UsdAttributeFloat with a scene item and USD attribute
    static UsdAttributeFloat::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);
}; // UsdAttributeFloat

//! \brief Interface for USD double attributes.
class UsdAttributeDouble : public TypedUsdAttribute<double>
{
public:
    typedef std::shared_ptr<UsdAttributeDouble> Ptr;

    using TypedUsdAttribute<double>::TypedUsdAttribute;

    //! Create a UsdAttributeDouble with a scene item and USD attribute
    static UsdAttributeDouble::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);

}; // UsdAttributeDouble

//! \brief Interface for USD string attributes.
class UsdAttributeString
    : public Ufe::TypedAttribute<std::string>
#ifdef UFE_V4_FEATURES_AVAILABLE
    , public UsdAttribute
#else
    , private UsdAttribute
#endif
{
public:
    typedef std::shared_ptr<UsdAttributeString> Ptr;

    UsdAttributeString(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);

    //! Create a UsdAttributeString with a scene item and USD attribute
    static UsdAttributeString::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);

    // Ufe::Attribute overrides
    UFE_ATTRIBUTE_OVERRIDES

    // Ufe default value overrides
    UFE_DEFAULT_VALUE_OVERRIDES

    // Ufe::TypedAttribute overrides
    std::string               get() const override;
    void                      set(const std::string& value) override;
    Ufe::UndoableCommand::Ptr setCmd(const std::string& value) override;
}; // UsdAttributeString

//! \brief Interface for USD TfToken attributes.
class UsdAttributeToken
    : public Ufe::TypedAttribute<std::string>
#ifdef UFE_V4_FEATURES_AVAILABLE
    , public UsdAttribute
#else
    , private UsdAttribute
#endif
{
public:
    typedef std::shared_ptr<UsdAttributeToken> Ptr;

    UsdAttributeToken(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);

    //! Create a UsdAttributeToken with a scene item and USD attribute
    static UsdAttributeToken::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);

    // Ufe::Attribute overrides
    UFE_ATTRIBUTE_OVERRIDES

    // Ufe default value overrides
    UFE_DEFAULT_VALUE_OVERRIDES

    // Ufe::TypedAttribute overrides
    std::string               get() const override;
    void                      set(const std::string& value) override;
    Ufe::UndoableCommand::Ptr setCmd(const std::string& value) override;
}; // UsdAttributeToken

//! \brief Interface for USD RGB color (float) attributes.
class UsdAttributeColorFloat3 : public TypedUsdAttribute<Ufe::Color3f>
{
public:
    typedef std::shared_ptr<UsdAttributeColorFloat3> Ptr;

    using TypedUsdAttribute<Ufe::Color3f>::TypedUsdAttribute;

    //! Create a UsdAttributeColorFloat3 with a scene item and USD attribute
    static UsdAttributeColorFloat3::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);
}; // UsdAttributeColorFloat3

#ifdef UFE_V4_FEATURES_AVAILABLE
//! \brief Interface for USD RGB color (float) attributes.
class UsdAttributeColorFloat4 : public TypedUsdAttribute<Ufe::Color4f>
{
public:
    typedef std::shared_ptr<UsdAttributeColorFloat4> Ptr;

    using TypedUsdAttribute<Ufe::Color4f>::TypedUsdAttribute;

    //! Create a UsdAttributeColorFloat4 with a scene item and USD attribute
    static UsdAttributeColorFloat4::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);
}; // UsdAttributeColorFloat4
#endif

//! \brief Interface for USD Vector3i (int) attributes.
class UsdAttributeInt3 : public TypedUsdAttribute<Ufe::Vector3i>
{
public:
    typedef std::shared_ptr<UsdAttributeInt3> Ptr;

    using TypedUsdAttribute<Ufe::Vector3i>::TypedUsdAttribute;

    //! Create a UsdAttributeInt3 with a scene item and USD attribute
    static UsdAttributeInt3::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);
}; // UsdAttributeInt3

#ifdef UFE_V4_FEATURES_AVAILABLE
//! \brief Interface for USD Vector2f (float) attributes.
class UsdAttributeFloat2 : public TypedUsdAttribute<Ufe::Vector2f>
{
public:
    typedef std::shared_ptr<UsdAttributeFloat2> Ptr;

    using TypedUsdAttribute<Ufe::Vector2f>::TypedUsdAttribute;

    //! Create a UsdAttributeFloat2 with a scene item and USD attribute
    static UsdAttributeFloat2::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);
}; // UsdAttributeFloat2
#endif

//! \brief Interface for USD Vector3f (float) attributes.
class UsdAttributeFloat3 : public TypedUsdAttribute<Ufe::Vector3f>
{
public:
    typedef std::shared_ptr<UsdAttributeFloat3> Ptr;

    using TypedUsdAttribute<Ufe::Vector3f>::TypedUsdAttribute;

    //! Create a UsdAttributeFloat3 with a scene item and USD attribute
    static UsdAttributeFloat3::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);
}; // UsdAttributeFloat3

#ifdef UFE_V4_FEATURES_AVAILABLE
//! \brief Interface for USD Vector4f (float) attributes.
class UsdAttributeFloat4 : public TypedUsdAttribute<Ufe::Vector4f>
{
public:
    typedef std::shared_ptr<UsdAttributeFloat4> Ptr;

    using TypedUsdAttribute<Ufe::Vector4f>::TypedUsdAttribute;

    //! Create a UsdAttributeFloat4 with a scene item and USD attribute
    static UsdAttributeFloat4::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);
}; // UsdAttributeFloat4
#endif

//! \brief Interface for USD Vector3d (double) attributes.
class UsdAttributeDouble3 : public TypedUsdAttribute<Ufe::Vector3d>
{
public:
    typedef std::shared_ptr<UsdAttributeDouble3> Ptr;

    using TypedUsdAttribute<Ufe::Vector3d>::TypedUsdAttribute;

    //! Create a UsdAttributeDouble3 with a scene item and USD attribute
    static UsdAttributeDouble3::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);
}; // UsdAttributeDouble3

#ifdef UFE_V4_FEATURES_AVAILABLE
//! \brief Interface for USD Matrix3d (double) attributes.
class UsdAttributeMatrix3d : public TypedUsdAttribute<Ufe::Matrix3d>
{
public:
    typedef std::shared_ptr<UsdAttributeMatrix3d> Ptr;

    using TypedUsdAttribute<Ufe::Matrix3d>::TypedUsdAttribute;

    //! Create a UsdAttributeMatrix3d with a scene item and USD attribute
    static UsdAttributeMatrix3d::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);
};

//! \brief Interface for USD Matrix4d (double) attributes.
class UsdAttributeMatrix4d : public TypedUsdAttribute<Ufe::Matrix4d>
{
public:
    typedef std::shared_ptr<UsdAttributeMatrix4d> Ptr;

    using TypedUsdAttribute<Ufe::Matrix4d>::TypedUsdAttribute;

    //! Create a UsdAttributeMatrix4d with a scene item and USD attribute
    static UsdAttributeMatrix4d::Ptr
    create(const UsdSceneItem::Ptr& item, UsdAttributeHolder::UPtr&& attrHolder);
};
#endif

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDATTRIBUTE_H
