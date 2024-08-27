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

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

#include <usdUfe/ufe/UsdUndoableCommand.h>

#include <pxr/usd/usdLux/cylinderLight.h>
#include <pxr/usd/usdLux/diskLight.h>
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/usd/usdLux/domeLight.h>
#include <pxr/usd/usdLux/lightAPI.h>
#include <pxr/usd/usdLux/portalLight.h>
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usdLux/shadowAPI.h>
#include <pxr/usd/usdLux/shapingAPI.h>
#include <pxr/usd/usdLux/sphereLight.h>

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

MAYAUSD_VERIFY_CLASS_SETUP(Ufe::Light, UsdLight);

UsdLight::UsdLight(const UsdUfe::UsdSceneItem::Ptr& item)
    : Light_v5_5()
    , _item(item)
{
}

UsdLight::Ptr UsdLight::create(const UsdUfe::UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdLight>(item);
}

//------------------------------------------------------------------------------
// Ufe::Light overrides
//------------------------------------------------------------------------------

const Ufe::Path& UsdLight::path() const { return _item->path(); }

Ufe::SceneItem::Ptr UsdLight::sceneItem() const { return _item; }

Ufe::Light::Type UsdLight::type() const
{
    const UsdPrim usdPrim = prim();

    if (usdPrim.IsA<UsdLuxDistantLight>()) {
        return Ufe::Light::Directional;
    } else if (usdPrim.IsA<UsdLuxRectLight>() || usdPrim.IsA<UsdLuxPortalLight>()) {
        return Ufe::Light::Area;
    } else if (usdPrim.IsA<UsdLuxSphereLight>()) {
        const UsdLuxShapingAPI shapingAPI(usdPrim);
        return shapingAPI.GetShapingConeAngleAttr().IsValid() ? Ufe::Light::Spot
                                                              : Ufe::Light::Sphere;
    } else if (usdPrim.IsA<UsdLuxCylinderLight>()) {
        return Ufe::Light::Cylinder;
    } else if (usdPrim.IsA<UsdLuxDiskLight>()) {
        return Ufe::Light::Disk;
    } else if (usdPrim.IsA<UsdLuxDomeLight>()) {
        return Ufe::Light::Dome;
    }

    // In case of unknown light type, fallback to point light
    return Ufe::Light::Point;
}

float getLightIntensity(const UsdPrim& prim)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetIntensityAttr();

    float val = 0.f;
    lightAttribute.Get(&val);
    return val;
}

void setLightIntensity(const UsdPrim& prim, float attrVal)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetIntensityAttr();

    lightAttribute.Set(attrVal);
}

Ufe::Light::IntensityUndoableCommand::Ptr UsdLight::intensityCmd(float li)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<float>>(path(), setLightIntensity);

    pCmd->set(li);
    return pCmd;
}

void UsdLight::intensity(float li) { setLightIntensity(prim(), li); }

float UsdLight::intensity() const { return getLightIntensity(prim()); }

Ufe::Color3f getLightColor(const UsdPrim& prim)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetColorAttr();

    GfVec3f val(0.f, 0.f, 0.f);
    lightAttribute.Get(&val);
    return Ufe::Color3f(val[0], val[1], val[2]);
}

void setLightColor(const UsdPrim& prim, const Ufe::Color3f& attrVal)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetColorAttr();

    lightAttribute.Set(GfVec3f(attrVal.r(), attrVal.g(), attrVal.b()));
}

Ufe::Light::ColorUndoableCommand::Ptr UsdLight::colorCmd(float r, float g, float b)
{
    auto pCmd
        = std::make_shared<SetValueUndoableCommandImpl<const Ufe::Color3f&>>(path(), setLightColor);

    pCmd->set(Ufe::Color3f(r, g, b));
    return pCmd;
}

void UsdLight::color(float r, float g, float b) { setLightColor(prim(), Ufe::Color3f(r, g, b)); }

Ufe::Color3f UsdLight::color() const { return getLightColor(prim()); }

bool getLightShadowEnable(const UsdPrim& prim)
{
    const UsdLuxShadowAPI shadowAPI(prim);
    PXR_NS::UsdAttribute  lightAttribute = shadowAPI.GetShadowEnableAttr();

    if (!lightAttribute) {
        // If the shadow enable attribute is not created yet, create one here
        lightAttribute = shadowAPI.CreateShadowEnableAttr(VtValue(true));
        return true;
    }

    bool val = false;
    lightAttribute.Get(&val);
    return val;
}

void setLightShadowEnable(const UsdPrim& prim, bool attrVal)
{
    const UsdLuxShadowAPI      shadowAPI(prim);
    const PXR_NS::UsdAttribute lightAttribute = shadowAPI.GetShadowEnableAttr();

    if (lightAttribute) {
        lightAttribute.Set(attrVal);
    }
}

Ufe::Light::ShadowEnableUndoableCommand::Ptr UsdLight::shadowEnableCmd(bool se)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<bool>>(path(), setLightShadowEnable);

    pCmd->set(se);
    return pCmd;
}

void UsdLight::shadowEnable(bool se) { setLightShadowEnable(prim(), se); }

bool UsdLight::shadowEnable() const { return getLightShadowEnable(prim()); }

Ufe::Color3f getLightShadowColor(const UsdPrim& prim)
{
    const UsdLuxShadowAPI shadowAPI(prim);
    PXR_NS::UsdAttribute  lightAttribute = shadowAPI.GetShadowColorAttr();

    if (!lightAttribute) {
        // If the shadow color attribute is not created yet, create one here
        lightAttribute = shadowAPI.CreateShadowColorAttr();
    }

    GfVec3f val(0.f, 0.f, 0.f);
    lightAttribute.Get(&val);
    return Ufe::Color3f(val[0], val[1], val[2]);
}

void setLightShadowColor(const UsdPrim& prim, const Ufe::Color3f& attrVal)
{
    const UsdLuxShadowAPI      shadowAPI(prim);
    const PXR_NS::UsdAttribute lightAttribute = shadowAPI.GetShadowColorAttr();

    lightAttribute.Set(GfVec3f(attrVal.r(), attrVal.g(), attrVal.b()));
}

Ufe::Light::ShadowColorUndoableCommand::Ptr UsdLight::shadowColorCmd(float r, float g, float b)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<const Ufe::Color3f&>>(
        path(), setLightShadowColor);

    pCmd->set(Ufe::Color3f(r, g, b));
    return pCmd;
}

void UsdLight::shadowColor(float r, float g, float b)
{
    setLightShadowColor(prim(), Ufe::Color3f(r, g, b));
}

Ufe::Color3f UsdLight::shadowColor() const { return getLightShadowColor(prim()); }

float getLightDiffuse(const UsdPrim& prim)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetDiffuseAttr();

    float val = 0.f;
    lightAttribute.Get(&val);
    return val;
}

void setLightDiffuse(const UsdPrim& prim, float attrVal)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetDiffuseAttr();

    lightAttribute.Set(attrVal);
}

Ufe::Light::DiffuseUndoableCommand::Ptr UsdLight::diffuseCmd(float ld)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<float>>(path(), setLightDiffuse);

    pCmd->set(ld);
    return pCmd;
}

void UsdLight::diffuse(float ld) { setLightDiffuse(prim(), ld); }

float UsdLight::diffuse() const { return getLightDiffuse(prim()); }

float getLightSpecular(const UsdPrim& prim)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetSpecularAttr();

    float val = 0.f;
    lightAttribute.Get(&val);
    return val;
}

void setLightSpecular(const UsdPrim& prim, float attrVal)
{
    const UsdLuxLightAPI       lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetSpecularAttr();

    lightAttribute.Set(attrVal);
}

Ufe::Light::SpecularUndoableCommand::Ptr UsdLight::specularCmd(float ls)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<float>>(path(), setLightSpecular);

    pCmd->set(ls);
    return pCmd;
}

void UsdLight::specular(float ls) { setLightSpecular(prim(), ls); }

float UsdLight::specular() const { return getLightSpecular(prim()); }

float getLightAngle(const UsdPrim& prim)
{
    const UsdLuxDistantLight   lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetAngleAttr();

    float val = 0.f;
    lightAttribute.Get(&val);
    return val;
}

void setLightAngle(const UsdPrim& prim, float attrVal)
{
    const UsdLuxDistantLight   lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetAngleAttr();

    lightAttribute.Set(attrVal);
}

Ufe::Light::AngleUndoableCommand::Ptr UsdDirectionalInterface::angleCmd(float la)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<float>>(_item->path(), setLightAngle);

    pCmd->set(la);
    return pCmd;
}

void UsdDirectionalInterface::angle(float la) { setLightAngle(_item->prim(), la); }

float UsdDirectionalInterface::angle() const { return getLightAngle(_item->prim()); }

Ufe::Light::SphereProps getLightSphereProps(const UsdPrim& prim)
{
    const UsdLuxSphereLight    lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetRadiusAttr();

    Ufe::Light::SphereProps sp;
    lightAttribute.Get(&sp.radius);
    sp.asPoint = (sp.radius == 0.f);
    return sp;
}

void setLightSphereProps(const UsdPrim& prim, const Ufe::Light::SphereProps& attrVal)
{
    const UsdLuxSphereLight    lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetRadiusAttr();

    lightAttribute.Set(attrVal.radius);
}

Ufe::Light::SpherePropsUndoableCommand::Ptr
UsdSphereInterface::spherePropsCmd(float radius, bool asPoint)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<const Ufe::Light::SphereProps&>>(
        _item->path(), setLightSphereProps);

    pCmd->set(Ufe::Light::SphereProps { radius, asPoint });
    return pCmd;
}

void UsdSphereInterface::sphereProps(float radius, bool asPoint)
{
    setLightSphereProps(_item->prim(), Ufe::Light::SphereProps { radius, asPoint });
}

Ufe::Light::SphereProps UsdSphereInterface::sphereProps() const
{
    return getLightSphereProps(_item->prim());
}

Ufe::Light::ConeProps getLightConeProps(const UsdPrim& prim)
{
    const UsdLuxShapingAPI     lightSchema(prim);
    const PXR_NS::UsdAttribute focusAttribute = lightSchema.GetShapingFocusAttr();
    const PXR_NS::UsdAttribute coneAngleAttribute = lightSchema.GetShapingConeAngleAttr();
    const PXR_NS::UsdAttribute coneSoftnessAttribute = lightSchema.GetShapingConeSoftnessAttr();

    Ufe::Light::ConeProps cp;
    focusAttribute.Get(&cp.focus);
    coneAngleAttribute.Get(&cp.angle);
    coneSoftnessAttribute.Get(&cp.softness);
    return cp;
}

void setLightConeProps(const UsdPrim& prim, const Ufe::Light::ConeProps& attrVal)
{
    const UsdLuxShapingAPI     lightSchema(prim);
    const PXR_NS::UsdAttribute focusAttribute = lightSchema.GetShapingFocusAttr();
    const PXR_NS::UsdAttribute coneAngleAttribute = lightSchema.GetShapingConeAngleAttr();
    const PXR_NS::UsdAttribute coneSoftnessAttribute = lightSchema.GetShapingConeSoftnessAttr();

    focusAttribute.Set(attrVal.focus);
    coneAngleAttribute.Set(attrVal.angle);
    coneSoftnessAttribute.Set(attrVal.softness);
}

Ufe::Light::ConePropsUndoableCommand::Ptr
UsdConeInterface::conePropsCmd(float focus, float angle, float softness)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<const Ufe::Light::ConeProps&>>(
        _item->path(), setLightConeProps);

    pCmd->set(Ufe::Light::ConeProps { focus, angle, softness });
    return pCmd;
}

void UsdConeInterface::coneProps(float focus, float angle, float softness)
{
    setLightConeProps(_item->prim(), Ufe::Light::ConeProps { focus, angle, softness });
}

Ufe::Light::ConeProps UsdConeInterface::coneProps() const
{
    return getLightConeProps(_item->prim());
}

bool getLightNormalize(const UsdPrim& prim)
{
    const UsdLuxRectLight      rectLight(prim);
    const PXR_NS::UsdAttribute lightAttribute = rectLight.GetNormalizeAttr();

    bool val = false;
    lightAttribute.Get(&val);
    return val;
}

void setLightNormalize(const UsdPrim& prim, bool attrVal)
{
    const UsdLuxRectLight      rectLight(prim);
    const PXR_NS::UsdAttribute lightAttribute = rectLight.GetNormalizeAttr();

    lightAttribute.Set(attrVal);
}

Ufe::Light::NormalizeUndoableCommand::Ptr UsdAreaInterface::normalizeCmd(bool nl)
{
    auto pCmd
        = std::make_shared<SetValueUndoableCommandImpl<bool>>(_item->path(), setLightNormalize);

    pCmd->set(nl);
    return pCmd;
}

Ufe::Light_v5_5::VolumeProps getLightVolumeProps(const UsdPrim& prim)
{

    Ufe::Light_v5_5::VolumeProps cp;

    return cp;
}

void setLightVolumeProps(const UsdPrim& prim, const Ufe::Light_v5_5::VolumeProps& attrVal)
{
    const UsdLuxSphereLight    lightSchema(prim);
    const PXR_NS::UsdAttribute lightAttribute = lightSchema.GetRadiusAttr();

    lightAttribute.Set(attrVal.radius);
}

#ifdef UFE_VOLUME_LIGHTS_SUPPORT
void UsdCylinderInterface::volumeProps(float radius, float length)
{
    setLightVolumeProps(_item->prim(), Ufe::Light_v5_5::VolumeProps { radius, length });
}
void UsdDiskInterface::volumeProps(float radius)
{
    setLightVolumeProps(_item->prim(), Ufe::Light_v5_5::VolumeProps { radius });
}

void UsdDomeInterface::volumeProps(float radius)
{
    setLightVolumeProps(_item->prim(), Ufe::Light_v5_5::VolumeProps { radius });
}

// Cylinder Light
Ufe::Light_v5_5::VolumeProps UsdCylinderInterface::volumeProps() const
{
    return getLightVolumeProps(_item->prim());
}

Ufe::Light_v5_5::VolumePropsUndoableCommand::Ptr
UsdCylinderInterface::volumePropsCmd(float radius, float length)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<const Ufe::Light_v5_5::VolumeProps&>>(
        _item->path(), setLightVolumeProps);
    pCmd->set(Ufe::Light_v5_5::VolumeProps { radius, length });
    return pCmd;
}

// Disk Light
Ufe::Light_v5_5::VolumeProps UsdDiskInterface::volumeProps() const
{
    return getLightVolumeProps(_item->prim());
}

Ufe::Light_v5_5::VolumePropsUndoableCommand::Ptr UsdDiskInterface::volumePropsCmd(float radius)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<const Ufe::Light_v5_5::VolumeProps&>>(
        _item->path(), setLightVolumeProps);

    pCmd->set(Ufe::Light_v5_5::VolumeProps { radius, 0 });
    return pCmd;
}

// Dome light
Ufe::Light_v5_5::VolumeProps UsdDomeInterface::volumeProps() const
{
    return getLightVolumeProps(_item->prim());
}

Ufe::Light_v5_5::VolumePropsUndoableCommand::Ptr UsdDomeInterface::volumePropsCmd(float radius)
{
    auto pCmd = std::make_shared<SetValueUndoableCommandImpl<const Ufe::Light_v5_5::VolumeProps&>>(
        _item->path(), setLightVolumeProps);

    pCmd->set(Ufe::Light_v5_5::VolumeProps { radius, 0 });
    return pCmd;
}

void UsdAreaInterface::normalize(bool ln) { setLightNormalize(_item->prim(), ln); }

bool UsdAreaInterface::normalize() const { return getLightNormalize(_item->prim()); }

std::shared_ptr<Ufe::Light::DirectionalInterface> UsdLight::directionalInterfaceImpl()
{
    return std::make_shared<UsdDirectionalInterface>(_item);
}

std::shared_ptr<Ufe::Light::SphereInterface> UsdLight::sphereInterfaceImpl()
{
    return std::make_shared<UsdSphereInterface>(_item);
}

std::shared_ptr<Ufe::Light::ConeInterface> UsdLight::coneInterfaceImpl()
{
    return std::make_shared<UsdConeInterface>(_item);
}

std::shared_ptr<Ufe::Light::AreaInterface> UsdLight::areaInterfaceImpl()
{
    return std::make_shared<UsdAreaInterface>(_item);
}

std::shared_ptr<Ufe::Light_v5_5::CylinderInterface> UsdLight::cylinderInterfaceImpl()
{
    return std::make_shared<UsdCylinderInterface>(_item);
}

std::shared_ptr<Ufe::Light_v5_5::DiskInterface> UsdLight::diskInterfaceImpl()
{
    return std::make_shared<UsdDiskInterface>(_item);
}

std::shared_ptr<Ufe::Light_v5_5::DomeInterface> UsdLight::domeInterfaceImpl()
{
    return std::make_shared<UsdDomeInterface>(_item);
}
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
