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
#ifndef MAYAUSD_USDLIGHT2_H
#define MAYAUSD_USDLIGHT2_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/light2.h>
#include <ufe/path.h>
#include <ufe/value.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

class MAYAUSD_CORE_PUBLIC UsdAreaLight2Interface : public Ufe::AreaLightInterface
{
public:
    UsdAreaLight2Interface(const UsdUfe::UsdSceneItem::Ptr& item)
        : _item(item)
    {
    }

    Ufe::AreaLightInterface::NormalizeUndoableCommand::Ptr normalizeCmd(bool nl) override;
    void                                                   normalize(bool nl) override;
    bool                                                   normalize() const override;

    virtual Ufe::AreaLightInterface::WidthUndoableCommand::Ptr widthCmd(float w) override;
    virtual void                                               width(float w) override;
    virtual float                                              width() const override;

    virtual Ufe::AreaLightInterface::HeightUndoableCommand::Ptr heightCmd(float h) override;
    virtual void                                                height(float h) override;
    virtual float                                               height() const override;

private:
    UsdUfe::UsdSceneItem::Ptr _item;
};

//! \brief Interface to control lights through USD.
class MAYAUSD_CORE_PUBLIC UsdLight2 : public Ufe::Light2
{
public:
    typedef std::shared_ptr<UsdLight2> Ptr;

    UsdLight2() = default;
    UsdLight2(const UsdUfe::UsdSceneItem::Ptr& item);

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdLight2);

    //! Create a UsdLight2.
    static UsdLight2::Ptr create(const UsdUfe::UsdSceneItem::Ptr& item);

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

    virtual Ufe::Value getMetadata(const std::string& key) const override;

    virtual bool setMetadata(const std::string& key, const Ufe::Value& value) override;

    virtual bool clearMetadata(const std::string& key) override;

    virtual bool hasMetadata(const std::string& key) const override;

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

private:
    UsdUfe::UsdSceneItem::Ptr _item;
    Ufe::ValueDictionary      metaData;
}; // UsdLight2

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_USDLIGHT2_H
