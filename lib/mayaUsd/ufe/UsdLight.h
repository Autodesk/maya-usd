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
#include <mayaUsd/ufe/UsdSceneItem.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/light.h>
#include <ufe/path.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to control lights through USD.
class MAYAUSD_CORE_PUBLIC UsdLight : public Ufe::Light
{
public:
    typedef std::shared_ptr<UsdLight> Ptr;

    UsdLight();
    UsdLight(const UsdSceneItem::Ptr& item);
    ~UsdLight() override = default;

    // Delete the copy/move constructors assignment operators.
    UsdLight(const UsdLight&) = delete;
    UsdLight& operator=(const UsdLight&) = delete;
    UsdLight(UsdLight&&) = delete;
    UsdLight& operator=(UsdLight&&) = delete;

    //! Create a UsdLight.
    static UsdLight::Ptr create(const UsdSceneItem::Ptr& item);

    inline PXR_NS::UsdPrim prim() const
    {
        PXR_NAMESPACE_USING_DIRECTIVE
        TF_AXIOM(fItem != nullptr);
        return fItem->prim();
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

private:
    UsdSceneItem::Ptr fItem;
}; // UsdLight

class UsdDirectionalInterface : public Ufe::Light::DirectionalInterface
{
public:
    UsdDirectionalInterface(const UsdSceneItem::Ptr& item)
        : fItem(item)
    {
    }

    Ufe::Light::AngleUndoableCommand::Ptr angleCmd(float la) override;
    void                                  angle(float la) override;
    float                                 angle() const override;

private:
    UsdSceneItem::Ptr fItem;
};

class UsdSphereInterface : public Ufe::Light::SphereInterface
{
public:
    UsdSphereInterface(const UsdSceneItem::Ptr& item)
        : fItem(item)
    {
    }

    Ufe::Light::SpherePropsUndoableCommand::Ptr spherePropsCmd(float radius, bool asPoint) override;
    void                                        sphereProps(float radius, bool asPoint) override;
    Ufe::Light::SphereProps                     sphereProps() const override;

private:
    UsdSceneItem::Ptr fItem;
};

class UsdConeInterface : public Ufe::Light::ConeInterface
{
public:
    UsdConeInterface(const UsdSceneItem::Ptr& item)
        : fItem(item)
    {
    }

    Ufe::Light::ConePropsUndoableCommand::Ptr
                          conePropsCmd(float focus, float angle, float softness) override;
    void                  coneProps(float focus, float angle, float softness) override;
    Ufe::Light::ConeProps coneProps() const override;

private:
    UsdSceneItem::Ptr fItem;
};

class UsdAreaInterface : public Ufe::Light::AreaInterface
{
public:
    UsdAreaInterface(const UsdSceneItem::Ptr& item)
        : fItem(item)
    {
    }

    Ufe::Light::NormalizeUndoableCommand::Ptr normalizeCmd(bool nl) override;
    void                                      normalize(bool ln) override;
    bool                                      normalize() const override;

private:
    UsdSceneItem::Ptr fItem;
};

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
