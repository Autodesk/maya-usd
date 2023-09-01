//
// Copyright 2019 Luma Pictures
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
// Copyright 2023 Autodesk, Inc. All rights reserved.
//
#include <mayaHydraLib/adapters/adapterDebugCodes.h>
#include <mayaHydraLib/adapters/adapterRegistry.h>
#include <mayaHydraLib/adapters/lightAdapter.h>
#include <mayaHydraLib/adapters/mayaAttrs.h>
#include <mayaHydraLib/adapters/tokens.h>

#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/usdLux/tokens.h>

#include <maya/MPlugArray.h>
#include <maya/MTextureManager.h>

PXR_NAMESPACE_OPEN_SCOPE

class MayaHydraSceneProducer;

/**
 * \brief MayaHydraAiSkyDomeLightAdapter is used to handle the translation from an Arnold skydome
 * light to hydra.
 */
class MayaHydraAiSkyDomeLightAdapter : public MayaHydraLightAdapter
{
    ///To be able to create a dummy texture (see _dummyTexturePath)
    static MHWRender::MTextureManager*  _pTextureManager;

    ///Temp folder path from the OS
    static std::string                  _tmpFolderPath; 

    /**_dummyTexturePath is the fullpath filename for a dummy 1x1 texture file used when there is no texture connected to the color of the Arnold sky dome light
    * as Hydra always wants a texture and ignores the color if no texture is present.
    */
    std::string                         _dummyTextureFullPathFilename; 
    ///This is only the filename of the dummy texture to be saved
    std::string                         _dummyTextureFilenameOnly; 
    
    ///Is the color attribute of the sky dome light connected to something ?
    bool _colorIsConnected  = false;

    ///When we do a live update of the dummy texture, since the fullpath filename hasn't changed, Hydra doesn't reload it, it wants a new filename...
    ///So we'll use a suffix to alternate between 2 suffixes
    bool _usingSuffix1      = true; 

public:
    MayaHydraAiSkyDomeLightAdapter(MayaHydraSceneProducer* producer, const MDagPath& dag)
        : MayaHydraLightAdapter(producer, dag)
    {
        //Init static variables if needed
        if (! _pTextureManager){
            MHWRender::MRenderer* const renderer = MHWRender::MRenderer::theRenderer();
            _pTextureManager = renderer ? renderer->getTextureManager() : nullptr;
        }

        if (_tmpFolderPath.empty()){
            const char *tmpDir = getenv("TMPDIR");
	        if (tmpDir) {
                _tmpFolderPath = tmpDir;
            }else {
                tmpDir = getenv("TEMP");
                if (tmpDir) {
                    _tmpFolderPath = tmpDir;
                }
            }
        }
        
	    _dummyTextureFilenameOnly = TfStringPrintf("/AiSkyDomeLight__%p", this);
    }

    virtual ~MayaHydraAiSkyDomeLightAdapter()
    {
        //Delete the dummy texture files if they exist
        if (! _dummyTextureFullPathFilename.empty()){
            auto dummyTextureFullPathFilename1 = _tmpFolderPath + _dummyTextureFilenameOnly + std::string("_1.png");
            auto dummyTextureFullPathFilename2 = _tmpFolderPath + _dummyTextureFilenameOnly + std::string("_2.png");
            std::remove(dummyTextureFullPathFilename1.c_str());
            std::remove(dummyTextureFullPathFilename2.c_str());
        }
    }

    const TfToken& LightType() const override { return HdPrimTypeTokens->domeLight; }

    VtValue GetLightParamValue(const TfToken& paramName) override
    {
        MStatus           status;
        MFnDependencyNode light(GetNode(), &status);
        if (ARCH_UNLIKELY(!status)) {
            return {};
        }

        // We are not using precomputed attributes here, because we don't have
        // a guarantee that mtoa will be loaded before mayaHydra.
        if (paramName == HdLightTokens->color) {
            const auto plug = light.findPlug("color", true);
            MPlugArray conns;
            plug.connectedTo(conns, true, false);
            _colorIsConnected = (conns.length() > 0);
            if (_colorIsConnected){
                return VtValue(GfVec3f(1.0f, 1.0f, 1.0f));//When there is a connection, return a white color.
            }
            
            // If no texture is found then get unconnected plug value and make a 1x1 texture of constant color using it.
            float r = 0.5f;
            float g = 0.5f;
            float b = 0.5f;
            if (!plug.isNull())
            {
               plug.child(0).getValue(r);
               plug.child(1).getValue(g);
               plug.child(2).getValue(b);
            }

            const float rClamped = (r <= 1.f) ? r : 1.0f;
            const float gClamped = (g <= 1.f) ? g : 1.0f;
            const float bClamped = (b <= 1.f) ? b : 1.0f;

            unsigned char texData[4];
            texData[0] = (unsigned char)(255 * rClamped);
            texData[1] = (unsigned char)(255 * gClamped);
            texData[2] = (unsigned char)(255 * bClamped);
            texData[3] = 255;

            //Create a 1 x 1 constant color texture
            MHWRender::MTextureDescription desc;
            desc.setToDefault2DTexture();
            desc.fWidth     = 1;
            desc.fHeight    = 1;
            desc.fFormat    = MHWRender::kR8G8B8A8_UNORM;
            if (_pTextureManager){
                auto pTexture = _pTextureManager->acquireTexture("", desc, texData);//In memory
                if (pTexture && (!_tmpFolderPath.empty()) && (!_dummyTextureFilenameOnly.empty())){

                    pTexture->setHasAlpha(true);

                    //Save the dummy texture to the temp folder as hydra needs a texture file path
                    //When we do a live update of the dummy texture, such as when the color has changed, 
                    //we need to change the filename as when keeping the same filename, Hydra doesn't reload it. It wants a new filename...
                    //So we'll alternate a suffix everytime we save it.
                    _dummyTextureFullPathFilename       = _tmpFolderPath + _dummyTextureFilenameOnly;
                    if (_usingSuffix1){
                        _dummyTextureFullPathFilename   += "_1.png";
                    }else{
                        _dummyTextureFullPathFilename   += "_2.png";
                    }
                    _usingSuffix1 = !_usingSuffix1;//So we use the other suffix for next saving
                    _pTextureManager->saveTexture(pTexture, MString(_dummyTextureFullPathFilename.c_str())); //This texture will be used in the HdLightTokens->textureFile parameter
                }
            }
            return VtValue(GfVec3f(r,g,b));
        } else if (paramName == HdLightTokens->intensity) {
            return VtValue(light.findPlug("intensity", true).asFloat());
        } else if (paramName == HdLightTokens->diffuse) {
            MPlug aiDiffuse = light.findPlug("aiDiffuse", true, &status);
            if (status == MS::kSuccess) {
                return VtValue(aiDiffuse.asFloat());
            }
        } else if (paramName == HdLightTokens->specular) {
            MPlug aiSpecular = light.findPlug("aiSpecular", true, &status);
            if (status == MS::kSuccess) {
                return VtValue(aiSpecular.asFloat());
            }
        } else if (paramName == HdLightTokens->exposure) {
            return VtValue(light.findPlug("aiExposure", true).asFloat());
        } else if (paramName == HdLightTokens->normalize) {
            return VtValue(light.findPlug("aiNormalize", true).asBool());
        } else if (paramName == HdLightTokens->textureFormat) {
            const auto format = light.findPlug("format", true).asShort();
            // mirrored_ball : 0
            // angular : 1
            // latlong : 2
            if (format == 0) {
                return VtValue(UsdLuxTokens->mirroredBall);
            } else if (format == 2) {
                return VtValue(UsdLuxTokens->latlong);
            } else {
                return VtValue(UsdLuxTokens->automatic);
            }
        } else if (paramName == HdLightTokens->textureFile) {
            // Be aware that dome lights in HdStorm always need a texture to work correctly,
            // the color is not used if no texture is present. 
            if (! _colorIsConnected){
                if (! _dummyTextureFullPathFilename.empty()){
                    return VtValue(SdfAssetPath(_dummyTextureFullPathFilename, _dummyTextureFullPathFilename));// SdfAssetPath requires both "path" "resolvedPath"
                } 
                
                return VtValue(SdfAssetPath());//this will produce a warning but hopefully is an edge case as it means we were not able to create a dummy texture
            }

            MPlugArray conns;
            light.findPlug("color", true).connectedTo(conns, true, false);
            if (conns.length() < 1) {
                return VtValue(SdfAssetPath());//Should never happen as it has been tested before with its equivalent : _colorIsConnected
            }
            MFnDependencyNode file(conns[0].node(), &status);
            if (ARCH_UNLIKELY(
                    !status || (file.typeName() != MayaHydraAdapterTokens->file.GetText()))) {
                // Be aware that dome lights in HdStorm always need a texture to work correctly,
                // the color is not used if no texture is present. 
                if (! _dummyTextureFullPathFilename.empty()){
                    return VtValue(SdfAssetPath(_dummyTextureFullPathFilename, _dummyTextureFullPathFilename));// SdfAssetPath requires both "path" "resolvedPath"
                } else {
                    return VtValue(SdfAssetPath());//this will produce a warning but hopefully is an edge case
                }
            }

            const char* fileTextureName
                = file.findPlug(MayaAttrs::file::fileTextureName, true).asString().asChar();
            // SdfAssetPath requires both "path" "resolvedPath"
            return VtValue(SdfAssetPath(fileTextureName, fileTextureName));

        } else if (paramName == HdLightTokens->enableColorTemperature) {
            return VtValue(false);
        }
        return {};
    }
};

//Static variables from MayaHydraAiSkyDomeLightAdapter
MHWRender::MTextureManager* MayaHydraAiSkyDomeLightAdapter::_pTextureManager = nullptr;
std::string                 MayaHydraAiSkyDomeLightAdapter::_tmpFolderPath; 

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<MayaHydraAiSkyDomeLightAdapter, TfType::Bases<MayaHydraLightAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(MayaHydraAdapterRegistry, domeLight)
{
    MayaHydraAdapterRegistry::RegisterLightAdapter(
        TfToken("aiSkyDomeLight"),
        [](MayaHydraSceneProducer* producer, const MDagPath& dag) -> MayaHydraLightAdapterPtr {
            return MayaHydraLightAdapterPtr(new MayaHydraAiSkyDomeLightAdapter(producer, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
