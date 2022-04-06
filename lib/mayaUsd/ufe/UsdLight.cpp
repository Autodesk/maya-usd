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
#include "UsdLight.h"

#include "private/Utils.h"
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

#include <pxr/usd/usdLux/distantLight.h>
#if PXR_VERSION < 2111
#include <pxr/usd/usdLux/light.h>
#else
#include <pxr/usd/usdLux/lightAPI.h>
#endif
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usdLux/shadowAPI.h>
#include <pxr/usd/usdLux/shapingAPI.h>
#include <pxr/usd/usdLux/sphereLight.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

template <typename ValueTypeIn>
class SetValueUndoableCommandImpl : public Ufe::SetValueUndoableCommand<ValueTypeIn>
{
public:

    using ValueTypeNonRef = typename std::remove_reference<ValueTypeIn>::type;
    using ValueTypeOut = typename std::remove_const<ValueTypeNonRef>::type;

    typedef std::function<ValueTypeOut(const UsdPrim& prim)> GetterFunc;
    typedef std::function<void(const UsdPrim& prim, ValueTypeIn)> SetterFunc;

    SetValueUndoableCommandImpl(const Ufe::Path &path, const GetterFunc& gf, const SetterFunc& sf) 
        : Ufe::SetValueUndoableCommand<ValueTypeIn>(path)
        , getterFunc(gf)
        , setterFunc(sf)
    {
    }

    ~SetValueUndoableCommandImpl() override {}

    bool set(ValueTypeIn v) override { redoValue = v; return true; }

    void execute() override
    {
        if (auto pItem = std::dynamic_pointer_cast<UsdSceneItem>(
            Ufe::BaseUndoableCommand::sceneItem())) {
            undoValue = getterFunc(pItem->prim());
            setterFunc(pItem->prim(), redoValue);        
        }
    }

    void undo() override
    {
        if (auto pItem = std::dynamic_pointer_cast<UsdSceneItem>(
            Ufe::BaseUndoableCommand::sceneItem())) {
                setterFunc(pItem->prim(), undoValue);
        } 
    }

    void redo() override
    {
        if (auto pItem = std::dynamic_pointer_cast<UsdSceneItem>(
            Ufe::BaseUndoableCommand::sceneItem())) {
                setterFunc(pItem->prim(), redoValue);
        }
    }

private:
    GetterFunc getterFunc;
    SetterFunc setterFunc;

    ValueTypeOut redoValue;
    ValueTypeOut undoValue;
};

UsdLight::UsdLight()
    : Light()
{
}

UsdLight::UsdLight(const UsdSceneItem::Ptr& item)
    : Light()
    , fItem(item)
{
}

UsdLight::Ptr UsdLight::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdLight>(item);
}

#if PXR_VERSION < 2111
    using UsdLuxLightCommon = UsdLuxLight;
#else
    using UsdLuxLightCommon = UsdLuxLightAPI;
#endif

//------------------------------------------------------------------------------
// Ufe::Light overrides
//------------------------------------------------------------------------------

const Ufe::Path& UsdLight::path() const { return fItem->path(); }

Ufe::SceneItem::Ptr UsdLight::sceneItem() const { return fItem; }

Ufe::Light::Type UsdLight::type() const
{
    const UsdPrim usdPrim = prim();

    if (usdPrim.IsA<UsdLuxDistantLight>()) {
        return Ufe::Light::Directional;
    } else if (usdPrim.IsA<UsdLuxRectLight>()) {
        return Ufe::Light::Area;
    } else if (usdPrim.IsA<UsdLuxSphereLight>()) {
        const UsdLuxShapingAPI shapingAPI(usdPrim);
        return shapingAPI ? Ufe::Light::Spot : Ufe::Light::Point;
    }
    
    return Ufe::Light::Invalid;
}

float getLightIntensity(const UsdPrim& prim)
{
    const UsdLuxLightCommon lightSchema(prim);
    const UsdAttribute lightAttribute = lightSchema.GetIntensityAttr();

    VtValue val;
    lightAttribute.Get(&val, UsdTimeCode::EarliestTime());
    return val.Get<float>();
}

void setLightIntensity(const UsdPrim& prim, float attrVal)
{
    const UsdLuxLightCommon lightSchema(prim);
    const UsdAttribute lightAttribute = lightSchema.GetIntensityAttr();
    
    lightAttribute.Set(attrVal, UsdTimeCode::Default());
}

Ufe::Light::IntensityUndoableCommand::Ptr UsdLight::intensityCmd(float li)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<float>>(
        path(), getLightIntensity, setLightIntensity);

    pCmd->set(li);
    return pCmd;
}

void UsdLight::intensity(float li)
{
    setLightIntensity(prim(), li);
}

float UsdLight::intensity() const
{
    return getLightIntensity(prim());
}

Ufe::Color3f getLightColor(const UsdPrim& prim)
{
    const UsdLuxLightCommon lightSchema(prim);
    const UsdAttribute lightAttribute = lightSchema.GetColorAttr();

    VtValue val;
    lightAttribute.Get(&val, UsdTimeCode::EarliestTime());
    GfVec3f clr = val.Get<GfVec3f>();
    return Ufe::Color3f(clr[0], clr[1], clr[2]);
}

void setLightColor(const UsdPrim& prim, const Ufe::Color3f& attrVal)
{
    const UsdLuxLightCommon lightSchema(prim);
    const UsdAttribute lightAttribute = lightSchema.GetColorAttr();
    
    lightAttribute.Set(GfVec3f(attrVal.r(), attrVal.g(), attrVal.b()), UsdTimeCode::Default());
}

Ufe::Light::ColorUndoableCommand::Ptr UsdLight::colorCmd(float r, float g, float b)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<const Ufe::Color3f &>>(path(), getLightColor, setLightColor);

    pCmd->set(Ufe::Color3f(r, g, b));
    return pCmd;
}

void UsdLight::color(float r, float g, float b)
{
    setLightColor(prim(), Ufe::Color3f(r, g, b));
}

Ufe::Color3f UsdLight::color() const
{
    return getLightColor(prim());
}

bool getLightShadowEnable(const UsdPrim& prim)
{
    const UsdLuxShadowAPI shadowAPI(prim);
    const UsdAttribute lightAttribute = shadowAPI.GetShadowEnableAttr();

    VtValue val;
    lightAttribute.Get(&val, UsdTimeCode::EarliestTime());
    return val.Get<bool>();  
}

void setLightShadowEnable(const UsdPrim& prim, bool attrVal)
{
    const UsdLuxShadowAPI shadowAPI(prim);
    const UsdAttribute lightAttribute = shadowAPI.GetShadowEnableAttr();
    
    lightAttribute.Set(attrVal, UsdTimeCode::Default());
}

Ufe::Light::ShadowEnableUndoableCommand::Ptr UsdLight::shadowEnableCmd(bool se)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<bool>>(
       path(), getLightShadowEnable, setLightShadowEnable);

    pCmd->set(se);
    return pCmd;
}

void UsdLight::shadowEnable(bool se)
{
    setLightShadowEnable(prim(), se);
}

bool UsdLight::shadowEnable() const
{
    return getLightShadowEnable(prim());
}

Ufe::Color3f getLightShadowColor(const UsdPrim& prim)
{
    const UsdLuxShadowAPI shadowAPI(prim);
    const UsdAttribute lightAttribute = shadowAPI.GetShadowColorAttr();

    VtValue val;
    lightAttribute.Get(&val, UsdTimeCode::EarliestTime());
    GfVec3f clr = val.Get<GfVec3f>();
    return Ufe::Color3f(clr[0], clr[1], clr[2]); 
}

void setLightShadowColor(const UsdPrim& prim, const Ufe::Color3f& attrVal)
{
    const UsdLuxShadowAPI shadowAPI(prim);
    const UsdAttribute lightAttribute = shadowAPI.GetShadowColorAttr();
    
    lightAttribute.Set(GfVec3f(attrVal.r(), attrVal.g(), attrVal.b()), UsdTimeCode::Default());
}

Ufe::Light::ShadowColorUndoableCommand::Ptr UsdLight::shadowColorCmd(float r, float g, float b)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<const Ufe::Color3f &>>(
                        path(), getLightShadowColor, setLightShadowColor);

    pCmd->set(Ufe::Color3f(r, g, b));
    return pCmd;
}

void UsdLight::shadowColor(float r, float g, float b)
{
    setLightShadowColor(prim(), Ufe::Color3f(r, g, b));
}

Ufe::Color3f UsdLight::shadowColor() const
{
    return getLightShadowColor(prim());
}

float getLightDiffuse(const UsdPrim& prim)
{
    const UsdLuxLightCommon lightSchema(prim);
    const UsdAttribute lightAttribute = lightSchema.GetDiffuseAttr();

    VtValue val;
    lightAttribute.Get(&val, UsdTimeCode::EarliestTime());
    return val.Get<float>();
}

void setLightDiffuse(const UsdPrim& prim, float attrVal)
{
    const UsdLuxLightCommon lightSchema(prim);
    const UsdAttribute lightAttribute = lightSchema.GetDiffuseAttr();
    
    lightAttribute.Set(attrVal, UsdTimeCode::Default());
}

Ufe::Light::DiffuseUndoableCommand::Ptr UsdLight::diffuseCmd(float ld)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<float>>(
        path(), getLightDiffuse, setLightDiffuse);

    pCmd->set(ld);
    return pCmd;
}

void UsdLight::diffuse(float ld)
{
    setLightDiffuse(prim(), ld);
}

float UsdLight::diffuse() const
{
    return getLightDiffuse(prim());
}

float getLightSpecular(const UsdPrim& prim)
{
    const UsdLuxLightCommon lightSchema(prim);
    const UsdAttribute lightAttribute = lightSchema.GetSpecularAttr();

    VtValue val;
    lightAttribute.Get(&val, UsdTimeCode::EarliestTime());
    return val.Get<float>(); 
}

void setLightSpecular(const UsdPrim& prim, float attrVal)
{
    const UsdLuxLightCommon lightSchema(prim);
    const UsdAttribute lightAttribute = lightSchema.GetSpecularAttr();
    
    lightAttribute.Set(attrVal, UsdTimeCode::Default());
}

Ufe::Light::SpecularUndoableCommand::Ptr UsdLight::specularCmd(float ls)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<float>>(
        path(), getLightSpecular, setLightSpecular);

    pCmd->set(ls);
    return pCmd;
}

void UsdLight::specular(float ls)
{
    setLightSpecular(prim(), ls);
}

float UsdLight::specular() const
{
    return getLightSpecular(prim());
}

float getLightAngle(const UsdPrim& prim)
{
    const UsdLuxDistantLight lightSchema(prim);
    const UsdAttribute lightAttribute = lightSchema.GetAngleAttr();

    VtValue val;
    lightAttribute.Get(&val, UsdTimeCode::EarliestTime());
    return val.Get<float>(); 
}

void setLightAngle(const UsdPrim& prim, float attrVal)
{
    const UsdLuxDistantLight lightSchema(prim);
    const UsdAttribute lightAttribute = lightSchema.GetAngleAttr();
    
    lightAttribute.Set(attrVal, UsdTimeCode::Default());
}

Ufe::Light::AngleUndoableCommand::Ptr UsdDirectionalInterface::angleCmd(float la)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<float>>(
        fItem->path(), getLightAngle, setLightAngle);

    pCmd->set(la);
    return pCmd;
}

void UsdDirectionalInterface::angle(float la)
{
    setLightAngle(fItem->prim(), la);
}

float UsdDirectionalInterface::angle() const
{
    return getLightAngle(fItem->prim());
}

Ufe::Light::SphereProps getLightSphereProps(const UsdPrim& prim)
{
    const UsdLuxSphereLight lightSchema(prim);
    const UsdAttribute lightAttribute = lightSchema.GetRadiusAttr();

    VtValue val;
    lightAttribute.Get(&val, UsdTimeCode::EarliestTime());

    Ufe::Light::SphereProps sp;
    sp.radius = val.Get<float>();
    sp.asPoint = (sp.radius == 0.f);
    return sp;
}

void setLightSphereProps(const UsdPrim& prim, const Ufe::Light::SphereProps& attrVal)
{
    const UsdLuxSphereLight lightSchema(prim);
    const UsdAttribute lightAttribute = lightSchema.GetRadiusAttr();
    
    lightAttribute.Set(attrVal.radius, UsdTimeCode::Default());
}

Ufe::Light::SpherePropsUndoableCommand::Ptr UsdSphereInterface::spherePropsCmd(float radius, bool asPoint)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<const Ufe::Light::SphereProps&>>(
                        fItem->path(), getLightSphereProps, setLightSphereProps);

    pCmd->set(Ufe::Light::SphereProps{radius, asPoint});
    return pCmd;
}

void UsdSphereInterface::sphereProps(float radius, bool asPoint)
{
    setLightSphereProps(fItem->prim(), Ufe::Light::SphereProps {radius, asPoint});
}

Ufe::Light::SphereProps UsdSphereInterface::sphereProps() const
{
    return getLightSphereProps(fItem->prim());
}

Ufe::Light::ConeProps getLightConeProps(const UsdPrim& prim)
{
    const UsdLuxShapingAPI lightSchema(prim);
    const UsdAttribute focusAttribute = lightSchema.GetShapingFocusAttr();
    const UsdAttribute coneAngleAttribute = lightSchema.GetShapingConeAngleAttr();
    const UsdAttribute coneSoftnessAttribute = lightSchema.GetShapingConeSoftnessAttr();

    VtValue focusVal;
    focusAttribute.Get(&focusVal, UsdTimeCode::EarliestTime());

    VtValue coneAngleVal;
    coneAngleAttribute.Get(&coneAngleVal, UsdTimeCode::EarliestTime());

    VtValue coneSoftnessVal;
    coneSoftnessAttribute.Get(&coneSoftnessVal, UsdTimeCode::EarliestTime());        

    Ufe::Light::ConeProps cp;
    cp.focus = focusVal.Get<float>();
    cp.angle = coneAngleVal.Get<float>();
    cp.softness = coneSoftnessVal.Get<float>();
    return cp;
}

void setLightConeProps(const UsdPrim& prim, const Ufe::Light::ConeProps& attrVal)
{
    const UsdLuxShapingAPI lightSchema(prim);
    const UsdAttribute focusAttribute = lightSchema.GetShapingFocusAttr();
    const UsdAttribute coneAngleAttribute = lightSchema.GetShapingConeAngleAttr();
    const UsdAttribute coneSoftnessAttribute = lightSchema.GetShapingConeSoftnessAttr();
    
    focusAttribute.Set(attrVal.focus, UsdTimeCode::Default());
    coneAngleAttribute.Set(attrVal.angle, UsdTimeCode::Default());
    coneSoftnessAttribute.Set(attrVal.softness, UsdTimeCode::Default());
}

Ufe::Light::ConePropsUndoableCommand::Ptr UsdConeInterface::conePropsCmd(float focus, float angle, float softness)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<const Ufe::Light::ConeProps&>>(
                        fItem->path(), getLightConeProps, setLightConeProps);

    pCmd->set(Ufe::Light::ConeProps{focus, angle, softness});
    return pCmd;
}

void UsdConeInterface::coneProps(float focus, float angle, float softness)
{
    setLightConeProps(fItem->prim(), Ufe::Light::ConeProps{focus, angle, softness});
}

Ufe::Light::ConeProps UsdConeInterface::coneProps() const
{
    return getLightConeProps(fItem->prim());
}

bool getLightNormalize(const UsdPrim& prim)
{
    const UsdLuxRectLight rectLight(prim);
    const UsdAttribute lightAttribute = rectLight.GetNormalizeAttr();

    VtValue val;
    lightAttribute.Get(&val, UsdTimeCode::EarliestTime());
    return val.Get<bool>();
}

void setLightNormalize(const UsdPrim& prim, bool attrVal)
{
    const UsdLuxRectLight rectLight(prim);
    const UsdAttribute lightAttribute = rectLight.GetNormalizeAttr();
    
    lightAttribute.Set(attrVal, UsdTimeCode::Default());
}

Ufe::Light::NormalizeUndoableCommand::Ptr UsdAreaInterface::normalizeCmd(bool nl)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<bool>>(
        fItem->path(), getLightNormalize, setLightNormalize);

    pCmd->set(nl);
    return pCmd;
}

void UsdAreaInterface::normalize(bool ln)
{
    setLightNormalize(fItem->prim(), ln);
}

bool UsdAreaInterface::normalize() const
{
    return getLightNormalize(fItem->prim());
}

std::shared_ptr<Ufe::Light::DirectionalInterface> UsdLight::directionalInterfaceImpl()
{
    return std::make_shared<UsdDirectionalInterface>(fItem);
}

std::shared_ptr<Ufe::Light::SphereInterface> UsdLight::sphereInterfaceImpl()
{
    return std::make_shared<UsdSphereInterface>(fItem);
}

std::shared_ptr<Ufe::Light::ConeInterface> UsdLight::coneInterfaceImpl()
{
    return std::make_shared<UsdConeInterface>(fItem);
}

std::shared_ptr<Ufe::Light::AreaInterface> UsdLight::areaInterfaceImpl()
{
    return std::make_shared<UsdAreaInterface>(fItem);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
