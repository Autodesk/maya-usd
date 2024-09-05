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
#ifndef MAYAUSD_USDLIGHT_H
#define MAYAUSD_USDLIGHT_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/light.h>
#include <ufe/path.h>

#if defined(UFE_VOLUME_LIGHTS_SUPPORT) && (UFE_MAJOR_VERSION == 5)
#define UFE_LIGHT_BASE Ufe::Light_v5_5
#else
#define UFE_LIGHT_BASE Ufe::Light
#endif

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to control lights through USD.
class MAYAUSD_CORE_PUBLIC UsdLight : public UFE_LIGHT_BASE
{
public:
    typedef std::shared_ptr<UsdLight> Ptr;

    UsdLight() = default;
    UsdLight(const UsdUfe::UsdSceneItem::Ptr& item);

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdLight);

    //! Create a UsdLight.
    static UsdLight::Ptr create(const UsdUfe::UsdSceneItem::Ptr& item);

    inline PXR_NS::UsdPrim prim() const
    {
        PXR_NAMESPACE_USING_DIRECTIVE
        if (TF_VERIFY(_item != nullptr))
            return _item->prim();
        else
            return PXR_NS::UsdPrim();
    }

    // Ufe::Light overrides
    const Ufe::Path&    path() const override;
    Ufe::SceneItem::Ptr sceneItem() const override;
    Type                type() const override;

    IntensityUndoableCommand::Ptr intensityCmd(float li) override;
    void                          intensity(float li) override;
    float                         intensity() const override;

    ColorUndoableCommand::Ptr colorCmd(float r, float g, float b) override;
    void                      color(float r, float g, float b) override;
    Ufe::Color3f              color() const override;

    ShadowEnableUndoableCommand::Ptr shadowEnableCmd(bool se) override;
    void                             shadowEnable(bool se) override;
    bool                             shadowEnable() const override;

    ShadowColorUndoableCommand::Ptr shadowColorCmd(float r, float g, float b) override;
    void                            shadowColor(float r, float g, float b) override;
    Ufe::Color3f                    shadowColor() const override;

    DiffuseUndoableCommand::Ptr diffuseCmd(float ld) override;
    void                        diffuse(float ld) override;
    float                       diffuse() const override;

    SpecularUndoableCommand::Ptr specularCmd(float ls) override;
    void                         specular(float ls) override;
    float                        specular() const override;

protected:
    std::shared_ptr<DirectionalInterface> directionalInterfaceImpl() override;
    std::shared_ptr<SphereInterface>      sphereInterfaceImpl() override;
    std::shared_ptr<ConeInterface>        coneInterfaceImpl() override;
    std::shared_ptr<AreaInterface>        areaInterfaceImpl() override;
#ifdef UFE_VOLUME_LIGHTS_SUPPORT
    std::shared_ptr<CylinderInterface> cylinderInterfaceImpl() override;
    std::shared_ptr<DiskInterface>     diskInterfaceImpl() override;
    std::shared_ptr<DomeInterface>     domeInterfaceImpl() override;
#endif

private:
    UsdUfe::UsdSceneItem::Ptr _item;
}; // UsdLight

class UsdDirectionalInterface : public Ufe::Light::DirectionalInterface
{
public:
    UsdDirectionalInterface(const UsdUfe::UsdSceneItem::Ptr& item)
        : _item(item)
    {
    }

    Ufe::Light::AngleUndoableCommand::Ptr angleCmd(float la) override;
    void                                  angle(float la) override;
    float                                 angle() const override;

private:
    UsdUfe::UsdSceneItem::Ptr _item;
};

class UsdSphereInterface : public Ufe::Light::SphereInterface
{
public:
    UsdSphereInterface(const UsdUfe::UsdSceneItem::Ptr& item)
        : _item(item)
    {
    }

    Ufe::Light::SpherePropsUndoableCommand::Ptr spherePropsCmd(float radius, bool asPoint) override;
    void                                        sphereProps(float radius, bool asPoint) override;
    Ufe::Light::SphereProps                     sphereProps() const override;

private:
    UsdUfe::UsdSceneItem::Ptr _item;
};

class UsdConeInterface : public Ufe::Light::ConeInterface
{
public:
    UsdConeInterface(const UsdUfe::UsdSceneItem::Ptr& item)
        : _item(item)
    {
    }

    Ufe::Light::ConePropsUndoableCommand::Ptr
                          conePropsCmd(float focus, float angle, float softness) override;
    void                  coneProps(float focus, float angle, float softness) override;
    Ufe::Light::ConeProps coneProps() const override;

private:
    UsdUfe::UsdSceneItem::Ptr _item;
};

class UsdAreaInterface : public Ufe::Light::AreaInterface
{
public:
    UsdAreaInterface(const UsdUfe::UsdSceneItem::Ptr& item)
        : _item(item)
    {
    }

    Ufe::Light::NormalizeUndoableCommand::Ptr normalizeCmd(bool nl) override;
    void                                      normalize(bool ln) override;
    bool                                      normalize() const override;

private:
    UsdUfe::UsdSceneItem::Ptr _item;
};

#ifdef UFE_VOLUME_LIGHTS_SUPPORT
class UsdCylinderInterface : public UFE_LIGHT_BASE::CylinderInterface
{
public:
    UsdCylinderInterface(const UsdUfe::UsdSceneItem::Ptr& item)
        : _item(item)
    {
    }

    UFE_LIGHT_BASE::VolumePropsUndoableCommand::Ptr
                                volumePropsCmd(float radius, float length) override;
    void                        volumeProps(float radius, float length) override;
    UFE_LIGHT_BASE::VolumeProps volumeProps() const override;

private:
    UsdUfe::UsdSceneItem::Ptr _item;
};

class UsdDiskInterface : public UFE_LIGHT_BASE::DiskInterface
{
public:
    UsdDiskInterface(const UsdUfe::UsdSceneItem::Ptr& item)
        : _item(item)
    {
    }

    UFE_LIGHT_BASE::VolumePropsUndoableCommand::Ptr volumePropsCmd(float radius) override;
    void                                            volumeProps(float radius) override;
    UFE_LIGHT_BASE::VolumeProps                     volumeProps() const override;

private:
    UsdUfe::UsdSceneItem::Ptr _item;
};

class UsdDomeInterface : public UFE_LIGHT_BASE::DomeInterface
{
public:
    UsdDomeInterface(const UsdUfe::UsdSceneItem::Ptr& item)
        : _item(item)
    {
    }

    UFE_LIGHT_BASE::VolumePropsUndoableCommand::Ptr volumePropsCmd(float radius) override;
    void                                            volumeProps(float radius) override;
    UFE_LIGHT_BASE::VolumeProps                     volumeProps() const override;

private:
    UsdUfe::UsdSceneItem::Ptr _item;
};
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_USDLIGHT_H
