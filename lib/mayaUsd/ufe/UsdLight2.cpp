//
// Copyright 2025 Autodesk
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
#include "UsdLight2.h"

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

#include <usdUfe/ufe/UsdUndoableCommand.h>

#include <pxr/usd/usdLux/lightAPI.h>
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usdLux/shadowAPI.h>

#include <stdexcept>

namespace MAYAUSD_NS_DEF {
namespace ufe {

template <typename ValueTypeIn>
class SetValueUndoableCommandImpl
    : public UsdUfe::UsdUndoableCommand<Ufe::SetValueUndoableCommand<ValueTypeIn>>
{
public:
    using ValueTypeNonRef = typename std::remove_reference<ValueTypeIn>::type;
    using ValueTypeOut = typename std::remove_const<ValueTypeNonRef>::type;

    typedef std::function<ValueTypeOut(const UsdPrim& prim)>      GetterFunc;
    typedef std::function<void(const UsdPrim& prim, ValueTypeIn)> SetterFunc;

    SetValueUndoableCommandImpl(const Ufe::Path& path, const SetterFunc& sf)
        : UsdUfe::UsdUndoableCommand<Ufe::SetValueUndoableCommand<ValueTypeIn>>(path)
        , _setterFunc(sf)
    {
    }

    ~SetValueUndoableCommandImpl() override { }

    bool set(ValueTypeIn v) override
    {
        _value = v;
        return true;
    }

    void executeImplementation() override
    {
        if (auto pItem = downcast(Ufe::BaseUndoableCommand::sceneItem())) {
            _setterFunc(pItem->prim(), _value);
        }
    }

private:
    SetterFunc   _setterFunc;
    ValueTypeOut _value;
};

MAYAUSD_VERIFY_CLASS_SETUP(Ufe::Light2, UsdLight2);

UsdLight2::UsdLight2(const UsdUfe::UsdSceneItem::Ptr& item)
    : Ufe::Light2()
    , _item(item)
{
    Ufe::Light2::Type lightType = type();
    if (lightType == Ufe::Light2::Area) {

        UsdAreaLight2Interface::Ptr areaInterface = std::make_shared<UsdAreaLight2Interface>(item);
        interfaces.push_back(areaInterface);
    }
}

UsdLight2::Ptr UsdLight2::create(const UsdUfe::UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdLight2>(item);
}

//------------------------------------------------------------------------------
// Ufe::Light overrides
//------------------------------------------------------------------------------

const Ufe::Path& UsdLight2::path() const { return _item->path(); }

Ufe::SceneItem::Ptr UsdLight2::sceneItem() const { return _item; }

Ufe::Light2::Type UsdLight2::type() const
{
    const UsdPrim usdPrim = prim();

    if (usdPrim.IsA<UsdLuxRectLight>()) {
        return Ufe::Light2::Area;
    }
    // In case of unknown light type, fallback to point light
    return Ufe::Light2::Invalid;
}

Ufe::Value UsdLight2::getMetadata(const std::string& key) const
{
    try {
        return metaData.at(key);
    } catch (const std::out_of_range&) {
        return Ufe::Value();
    }
}

bool UsdLight2::setMetadata(const std::string& key, const Ufe::Value& value)
{
    metaData[key] = value;
    return true;
}

bool UsdLight2::clearMetadata(const std::string& key)
{
    metaData.erase(key);
    return true;
}

bool UsdLight2::hasMetadata(const std::string& key) const
{
    return metaData.find(key) != metaData.end();
}

float getLight2Intensity(const UsdPrim& prim)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetIntensityAttr();

    float val = 0.f;
    lightAttribute.Get(&val);
    return val;
}

void setLight2Intensity(const UsdPrim& prim, float attrVal)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetIntensityAttr();

    if (!lightAttribute)
        lightSchema.CreateIntensityAttr(VtValue(attrVal));
    else
        lightAttribute.Set(attrVal);
}

Ufe::Light2::IntensityUndoableCommand::Ptr UsdLight2::intensityCmd(float li)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<float>>(path(), setLight2Intensity);

    pCmd->set(li);
    return pCmd;
}

void UsdLight2::intensity(float li) { setLight2Intensity(prim(), li); }

float UsdLight2::intensity() const { return getLight2Intensity(prim()); }

Ufe::Color3f getLight2Color(const UsdPrim& prim)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetColorAttr();

    GfVec3f val(0.f, 0.f, 0.f);
    lightAttribute.Get(&val);
    return Ufe::Color3f(val[0], val[1], val[2]);
}

void setLight2Color(const UsdPrim& prim, const Ufe::Color3f& attrVal)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetColorAttr();
    if (!lightAttribute)
        lightSchema.CreateColorAttr(VtValue(GfVec3f(attrVal.r(), attrVal.g(), attrVal.b())));
    else
        lightAttribute.Set(GfVec3f(attrVal.r(), attrVal.g(), attrVal.b()));
}

Ufe::Light2::ColorUndoableCommand::Ptr UsdLight2::colorCmd(float r, float g, float b)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<const Ufe::Color3f&>>(
        path(), setLight2Color);

    pCmd->set(Ufe::Color3f(r, g, b));
    return pCmd;
}

void UsdLight2::color(float r, float g, float b) { setLight2Color(prim(), Ufe::Color3f(r, g, b)); }

Ufe::Color3f UsdLight2::color() const { return getLight2Color(prim()); }

bool getLight2ShadowEnable(const UsdPrim& prim)
{
    const UsdLuxShadowAPI shadowAPI(prim);
    PXR_NS::UsdAttribute  lightAttribute = shadowAPI.GetShadowEnableAttr();

    bool val = false;
    lightAttribute.Get(&val);
    return val;
}

void setLight2ShadowEnable(const UsdPrim& prim, bool attrVal)
{
    const UsdLuxShadowAPI      shadowAPI(prim);
    const PXR_NS::UsdAttribute lightAttribute = shadowAPI.GetShadowEnableAttr();

    if (lightAttribute)
        lightAttribute.Set(attrVal);
    else
        shadowAPI.CreateShadowEnableAttr(VtValue(attrVal));
}

Ufe::Light2::ShadowEnableUndoableCommand::Ptr UsdLight2::shadowEnableCmd(bool se)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<bool>>(path(), setLight2ShadowEnable);

    pCmd->set(se);
    return pCmd;
}

void UsdLight2::shadowEnable(bool se) { setLight2ShadowEnable(prim(), se); }

bool UsdLight2::shadowEnable() const { return getLight2ShadowEnable(prim()); }

Ufe::Color3f getLight2ShadowColor(const UsdPrim& prim)
{
    const UsdLuxShadowAPI shadowAPI(prim);
    PXR_NS::UsdAttribute  lightAttribute = shadowAPI.GetShadowColorAttr();

    GfVec3f val(0.f, 0.f, 0.f);
    lightAttribute.Get(&val);
    return Ufe::Color3f(val[0], val[1], val[2]);
}

void setLight2ShadowColor(const UsdPrim& prim, const Ufe::Color3f& attrVal)
{
    const UsdLuxShadowAPI      shadowAPI(prim);
    const PXR_NS::UsdAttribute lightAttribute = shadowAPI.GetShadowColorAttr();
    if (!lightAttribute)
        shadowAPI.CreateShadowColorAttr(VtValue(GfVec3f(attrVal.r(), attrVal.g(), attrVal.b())));
    else
        lightAttribute.Set(GfVec3f(attrVal.r(), attrVal.g(), attrVal.b()));
}

Ufe::Light2::ShadowColorUndoableCommand::Ptr UsdLight2::shadowColorCmd(float r, float g, float b)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<const Ufe::Color3f&>>(
        path(), setLight2ShadowColor);

    pCmd->set(Ufe::Color3f(r, g, b));
    return pCmd;
}

void UsdLight2::shadowColor(float r, float g, float b)
{
    setLight2ShadowColor(prim(), Ufe::Color3f(r, g, b));
}

Ufe::Color3f UsdLight2::shadowColor() const { return getLight2ShadowColor(prim()); }

float getLight2Diffuse(const UsdPrim& prim)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetDiffuseAttr();

    float val = 0.f;
    lightAttribute.Get(&val);
    return val;
}

void setLight2Diffuse(const UsdPrim& prim, float attrVal)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetDiffuseAttr();
    if (!lightAttribute)
        lightSchema.CreateDiffuseAttr(VtValue(attrVal));
    else
        lightAttribute.Set(attrVal);
}

Ufe::Light2::DiffuseUndoableCommand::Ptr UsdLight2::diffuseCmd(float ld)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<float>>(path(), setLight2Diffuse);

    pCmd->set(ld);
    return pCmd;
}

void UsdLight2::diffuse(float ld) { setLight2Diffuse(prim(), ld); }

float UsdLight2::diffuse() const { return getLight2Diffuse(prim()); }

float getLight2Specular(const UsdPrim& prim)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetSpecularAttr();

    float val = 0.f;
    lightAttribute.Get(&val);
    return val;
}

void setLight2Specular(const UsdPrim& prim, float attrVal)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetSpecularAttr();
    if (!lightAttribute)
        lightSchema.CreateSpecularAttr(VtValue(attrVal));
    else
        lightAttribute.Set(attrVal);
}

Ufe::Light2::SpecularUndoableCommand::Ptr UsdLight2::specularCmd(float ls)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<float>>(path(), setLight2Specular);

    pCmd->set(ls);
    return pCmd;
}

void UsdLight2::specular(float ls) { setLight2Specular(prim(), ls); }

float UsdLight2::specular() const { return getLight2Specular(prim()); }

bool getLight2Normalize(const UsdPrim& prim)
{
    const UsdLuxRectLight      rectLight(prim);
    const PXR_NS::UsdAttribute lightAttribute = rectLight.GetNormalizeAttr();

    bool val = false;
    lightAttribute.Get(&val);
    return val;
}

void setLight2Normalize(const UsdPrim& prim, bool attrVal)
{
    const UsdLuxRectLight      rectLight(prim);
    const PXR_NS::UsdAttribute lightAttribute = rectLight.GetNormalizeAttr();

    if (!lightAttribute)
        rectLight.CreateNormalizeAttr(VtValue(attrVal));
    else
        lightAttribute.Set(attrVal);
}

Ufe::AreaLightInterface::NormalizeUndoableCommand::Ptr UsdAreaLight2Interface::normalizeCmd(bool nl)
{
    auto pCmd
        = std::make_shared<SetValueUndoableCommandImpl<bool>>(_item->path(), setLight2Normalize);

    pCmd->set(nl);
    return pCmd;
}

void UsdAreaLight2Interface::normalize(bool nl) { setLight2Normalize(_item->prim(), nl); }

bool UsdAreaLight2Interface::normalize() const { return getLight2Normalize(_item->prim()); }

float getLight2Width(const UsdPrim& prim)
{
    const UsdLuxRectLight      rectLight(prim);
    const PXR_NS::UsdAttribute lightAttribute = rectLight.GetWidthAttr();

    float val = 0.0f;
    lightAttribute.Get(&val);
    return val;
}

void setLight2Width(const UsdPrim& prim, float attrVal)
{
    const UsdLuxRectLight      rectLight(prim);
    const PXR_NS::UsdAttribute lightAttribute = rectLight.GetWidthAttr();

    if (!lightAttribute)
        rectLight.CreateWidthAttr(VtValue(attrVal));
    else
        lightAttribute.Set(attrVal);
}

Ufe::AreaLightInterface::WidthUndoableCommand::Ptr UsdAreaLight2Interface::widthCmd(float w)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<float>>(_item->path(), setLight2Width);

    pCmd->set(w);
    return pCmd;
}

void UsdAreaLight2Interface::width(float w) { setLight2Width(_item->prim(), w); }

float UsdAreaLight2Interface::width() const { return getLight2Width(_item->prim()); }

float getLight2Height(const UsdPrim& prim)
{
    const UsdLuxRectLight      rectLight(prim);
    const PXR_NS::UsdAttribute lightAttribute = rectLight.GetHeightAttr();

    float val = 0.0f;
    lightAttribute.Get(&val);
    return val;
}

void setLight2Height(const UsdPrim& prim, float attrVal)
{
    const UsdLuxRectLight      rectLight(prim);
    const PXR_NS::UsdAttribute lightAttribute = rectLight.GetHeightAttr();

    if (!lightAttribute)
        rectLight.CreateHeightAttr(VtValue(attrVal));
    else
        lightAttribute.Set(attrVal);
}

Ufe::AreaLightInterface::HeightUndoableCommand::Ptr UsdAreaLight2Interface::heightCmd(float h)
{
    auto pCmd
        = std::make_shared<SetValueUndoableCommandImpl<float>>(_item->path(), setLight2Height);

    pCmd->set(h);
    return pCmd;
}

void UsdAreaLight2Interface::height(float h) { setLight2Height(_item->prim(), h); }

float UsdAreaLight2Interface::height() const { return getLight2Height(_item->prim()); }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
