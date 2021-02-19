//
// Copyright 2020 Autodesk
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
#include "utils.h"

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlugArray.h>
#include <maya/MStatus.h>

#if PXR_VERSION < 2011
#include <pxr/base/tf/fileUtils.h>
#include <pxr/imaging/glf/contextCaps.h>
#include <pxr/imaging/glf/image.h>
#include <pxr/imaging/glf/textureHandle.h>
#include <pxr/imaging/glf/textureRegistry.h>
#include <pxr/imaging/glf/udimTexture.h>
#include <pxr/imaging/hdSt/textureResource.h>
#include <pxr/usdImaging/usdImaging/textureUtils.h>

#include <tuple>
#endif

PXR_NAMESPACE_OPEN_SCOPE

#if PXR_VERSION < 2011

namespace {

class UdimTextureFactory : public GlfTextureFactoryBase
{
public:
    virtual GlfTextureRefPtr
    New(TfToken const&                texturePath,
        GlfImage::ImageOriginLocation originLocation = GlfImage::OriginLowerLeft) const override
    {
        const GlfContextCaps& caps = GlfContextCaps::GetInstance();
        return GlfUdimTexture::New(
            texturePath,
            originLocation,
            UsdImaging_GetUdimTiles(texturePath, caps.maxArrayTextureLayers));
    }

    virtual GlfTextureRefPtr
    New(TfTokenVector const&          texturePaths,
        GlfImage::ImageOriginLocation originLocation = GlfImage::OriginLowerLeft) const override
    {
        return nullptr;
    }
};

} // namespace

#endif // PXR_VERSION < 2011

MObject GetConnectedFileNode(const MObject& obj, const TfToken& paramName)
{
    MStatus           status;
    MFnDependencyNode node(obj, &status);
    if (ARCH_UNLIKELY(!status)) {
        return MObject::kNullObj;
    }
    return GetConnectedFileNode(node, paramName);
}

MObject GetConnectedFileNode(const MFnDependencyNode& node, const TfToken& paramName)
{
    MPlugArray conns;
    node.findPlug(paramName.GetText(), true).connectedTo(conns, true, false);
    if (conns.length() == 0) {
        return MObject::kNullObj;
    }
    const auto ret = conns[0].node();
    if (ret.apiType() == MFn::kFileTexture) {
        return ret;
    }
    return MObject::kNullObj;
}

TfToken GetFileTexturePath(const MFnDependencyNode& fileNode)
{
    if (fileNode.findPlug(MayaAttrs::file::uvTilingMode, true).asShort() != 0) {
        const TfToken ret {
            fileNode.findPlug(MayaAttrs::file::fileTextureNamePattern, true).asString().asChar()
        };
        return ret.IsEmpty()
            ? TfToken { fileNode.findPlug(MayaAttrs::file::computedFileTextureNamePattern, true)
                            .asString()
                            .asChar() }
            : ret;
    } else {
        const TfToken ret { MRenderUtil::exactFileTextureName(fileNode.object()).asChar() };
        return ret.IsEmpty() ? TfToken { fileNode.findPlug(MayaAttrs::file::fileTextureName, true)
                                             .asString()
                                             .asChar() }
                             : ret;
    }
}

#if PXR_VERSION < 2011

std::tuple<HdWrap, HdWrap> GetFileTextureWrappingParams(const MObject& fileObj)
{
    const std::tuple<HdWrap, HdWrap> def { HdWrapClamp, HdWrapClamp };
    MStatus                          status;
    MFnDependencyNode                fileNode(fileObj, &status);
    if (!status) {
        return def;
    }

    auto getWrap = [&fileNode](MObject& wrapAttr, MObject& mirrorAttr) {
        if (fileNode.findPlug(wrapAttr, true).asBool()) {
            if (fileNode.findPlug(mirrorAttr, true).asBool()) {
                return HdWrapMirror;
            } else {
                return HdWrapRepeat;
            }
        } else {
            return HdWrapClamp;
        }
    };
    return std::tuple<HdWrap, HdWrap> { getWrap(MayaAttrs::file::wrapU, MayaAttrs::file::mirrorU),
                                        getWrap(MayaAttrs::file::wrapV, MayaAttrs::file::mirrorV) };
}

HdTextureResourceSharedPtr
GetFileTextureResource(const MObject& fileObj, const TfToken& filePath, int maxTextureMemory)
{
    if (filePath.IsEmpty()) {
        return {};
    }
    auto textureType = HdTextureType::Uv;
    if (GlfIsSupportedUdimTexture(filePath)) {
        textureType = HdTextureType::Udim;
    }
    if (textureType != HdTextureType::Udim && !TfPathExists(filePath)) {
        return {};
    }
    // TODO: handle origin
    const auto             origin = GlfImage::OriginLowerLeft;
    GlfTextureHandleRefPtr texture = nullptr;
    if (textureType == HdTextureType::Udim) {
        UdimTextureFactory factory;
        texture = GlfTextureRegistry::GetInstance().GetTextureHandle(filePath, origin, &factory);
    } else {
        texture = GlfTextureRegistry::GetInstance().GetTextureHandle(filePath, origin);
    }

    const auto wrapping = GetFileTextureWrappingParams(fileObj);

    // We can't really mimic texture wrapping and mirroring settings
    // from the uv placement node, so we don't touch those for now.
    return HdTextureResourceSharedPtr(new HdStSimpleTextureResource(
        texture,
        textureType,
        std::get<0>(wrapping),
        std::get<1>(wrapping),
        HdWrapClamp,
        HdMinFilterLinearMipmapLinear,
        HdMagFilterLinear,
        maxTextureMemory));
}

#endif // PXR_VERSION < 2011

PXR_NAMESPACE_CLOSE_SCOPE
