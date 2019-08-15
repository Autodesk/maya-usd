// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "material.h"
#include "textureResource.h"
#include "render_delegate.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/thisPlugin.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fileUtils.h"

#include <maya/MProfiler.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MShaderManager.h>
#include <maya/MFragmentManager.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

/* \brief  Finds the path to an XML shader fragment file based on plugin.json.
*/
std::string
_GetShaderFragmentPath(const char* fragmentName)
{
    static PlugPluginPtr plugin = PLUG_THIS_PLUGIN;
    const std::string path =
        PlugFindPluginResource(plugin, TfStringCatPaths("shaderFragments", fragmentName));
    TF_VERIFY(!path.empty(), "Could not find shader fragment: %s\n", fragmentName);

    return path;
}

/* \brief  Creates a new instance of a textured Blinn shader.
 
    When called for the first time, it constructs a textured Blinn shader
    fragment graph from custom shader fragments and caches this instance.
    
    \return A clone of the fragment graph instance.
*/
HdVP2Material::VP2ShaderUniquePtr
_GetTexturedBlinnShader()
{
    MRenderer* const renderer = MRenderer::theRenderer();
    if (!TF_VERIFY(renderer))
        return HdVP2Material::VP2ShaderUniquePtr();

    const MShaderManager* const shaderMgr = renderer->getShaderManager();
    if (!TF_VERIFY(shaderMgr))
        return HdVP2Material::VP2ShaderUniquePtr();

    MHWRender::MFragmentManager* fragMgr = renderer->getFragmentManager();
    if (!TF_VERIFY(fragMgr))
        return HdVP2Material::VP2ShaderUniquePtr();

    auto addFragments = [&fragMgr]()
    {
        auto addFragment = [&fragMgr](const char* name, bool asGraph)
        {
            const std::string fragmentPath = _GetShaderFragmentPath(name);

            const MString fragmentName =
                asGraph ? fragMgr->addFragmentGraphFromFile(fragmentPath.c_str())
                : fragMgr->addShadeFragmentFromFile(fragmentPath.c_str(), false);

            TF_VERIFY(
                fragmentName.length() > 0,
                "Failed to add VP2 shader fragment from %s", fragmentPath.c_str()
            );

            return fragmentName;
        };

        addFragment("customFileTextureOutputColor.xml", false);
        addFragment("customFileTextureOutputTransparency.xml", false);
        return addFragment("customFileTextureBlinnShader.xml", true);
    };

    static const MString fileTextureBlinnFragmentName = addFragments();

    static const MHWRender::MShaderInstance* const shaderInstance =
        shaderMgr->getFragmentShader(
            fileTextureBlinnFragmentName.asChar(), "outSurfaceFinal", true
        );

    return HdVP2Material::VP2ShaderUniquePtr(shaderInstance->clone());
}

/* !\brief Creates a textured shader instance with a texture resource assigned.
*/
HdVP2Material::VP2ShaderUniquePtr
_CreateVP2SurfaceShader(const HdVP2TextureResource& diffuseTextureResource)
{
    MRenderer* const renderer = MRenderer::theRenderer();
    if (!TF_VERIFY(renderer))
        return HdVP2Material::VP2ShaderUniquePtr();

    const MShaderManager* const shaderMgr = renderer->getShaderManager();
    if (!TF_VERIFY(shaderMgr))
        return HdVP2Material::VP2ShaderUniquePtr();

    HdVP2Material::VP2ShaderUniquePtr surfaceShader = _GetTexturedBlinnShader();

    if (TF_VERIFY(surfaceShader)) {
        MHWRender::MTexture* texture =
            diffuseTextureResource.GetTexture();

        TF_VERIFY(texture);
        MHWRender::MTextureAssignment textureAssignment;
        textureAssignment.texture = texture;
        surfaceShader->setParameter("map", textureAssignment);

        const MHWRender::MSamplerState* sampler =
            diffuseTextureResource.GetSampler();

        if (TF_VERIFY(sampler))
            surfaceShader->setParameter("textureSampler", *sampler);
    }

    return surfaceShader;
}

} //anonymous namespace

/* !\brief Releases the reference to the VP2 shader owned by a smart pointer.
*/
void
HdVP2Material::VP2ShaderDeleter::operator () (
    MHWRender::MShaderInstance* shader
) {
    if (!shader)
        return;

    MRenderer* const renderer = MRenderer::theRenderer();
    if (!TF_VERIFY(renderer))
        return;

    const MShaderManager* const shaderMgr = renderer->getShaderManager();
    if (!TF_VERIFY(shaderMgr))
        return;

    shaderMgr->releaseShader(shader);
}

/*! \brief  Constructor
*/
HdVP2Material::HdVP2Material(HdVP2RenderDelegate* renderDelegate, const SdfPath& id)
    : HdMaterial(id)
    , _renderDelegate(renderDelegate)
{
}

/*! \brief  Destructor - will release allocated shader instances.
*/
HdVP2Material::~HdVP2Material() {
}

/*! \brief  Looks up a VP2 texture resource in the render index.
*/
HdVP2TextureResourceSharedPtr
HdVP2Material::_GetTextureResource(
    HdSceneDelegate* sceneDelegate,
    HdMaterialParam const& param)
{
    HdResourceRegistrySharedPtr const& resourceRegistry =
        sceneDelegate->GetRenderIndex().GetResourceRegistry();

    HdVP2TextureResourceSharedPtr textureResource;

    SdfPath const& connection = param.GetConnection();
    if (!connection.IsEmpty()) {
        const HdTextureResource::ID textureID =
            sceneDelegate->GetTextureResourceID(connection);

        if (textureID != HdTextureResource::ID(-1)) {
            //
            // Use render index to convert local texture id into global
            // texture key
            HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
            HdResourceRegistry::TextureKey textureKey =
                renderIndex.GetTextureKey(textureID);

            HdInstance<
                HdResourceRegistry::TextureKey,
                HdTextureResourceSharedPtr
            > textureInstance;

            bool textureResourceFound = false;
            std::unique_lock<std::mutex> regLock =
                resourceRegistry->FindTextureResource(
                    textureKey, &textureInstance, &textureResourceFound
                );

            // A bad asset can cause the texture resource to not
            // be found. Hence, issue a warning and continue onto the
            // next param.
            if (!textureResourceFound) {
                TF_WARN("No texture resource found with path %s",
                    param.GetConnection().GetText()
                );
            }
            else {
                return boost::dynamic_pointer_cast<HdVP2TextureResource>(
                    textureInstance.GetValue()
                );
            }
        }
    }

    return HdVP2TextureResourceSharedPtr();
}

/*! \brief  Synchronize VP2 state with scene delegate state based on dirty bits
*/
void
HdVP2Material::Sync(
    HdSceneDelegate* sceneDelegate, HdRenderParam* /*renderParam*/, HdDirtyBits* dirtyBits
) {
    const SdfPath& materialId = GetId();

    if ((*dirtyBits & DirtySurfaceShader)
        || (*dirtyBits & DirtyParams)
    ) {
        // HACK - MAYA-98812: we don't use the results of these calls but as a
        // side effect, they cause material primvars, such as UVs to be
        // evaluated which is what we need. We should find a cleaner way to
        // achieve this.
        sceneDelegate->GetSurfaceShaderSource(materialId);
        sceneDelegate->GetDisplacementShaderSource(materialId);
        sceneDelegate->GetMaterialMetadata(materialId);

        HdVP2TextureResourceSharedPtr diffuseTextureResource;

        const HdMaterialParamVector& params =
            sceneDelegate->GetMaterialParams(materialId);

        for (const HdMaterialParam& param : params) {
            if (param.IsTexture()) {
                HdVP2TextureResourceSharedPtr textureResource =
                    _GetTextureResource(sceneDelegate, param);

                if (!textureResource) {
                    // we were unable to get the requested resource or
                    // fallback resource so skip this param
                    // (Error already posted).
                    continue;
                }

                static const TfToken diffuseToken("diffuseColor");
                TfToken name = param.GetName();

                if (name == diffuseToken
                    && TF_VERIFY(textureResource->GetTextureType() == HdTextureType::Uv)
                ) {
                    diffuseTextureResource = textureResource;

                    const TfTokenVector& uvTokens = param.GetSamplerCoordinates();
                    _uvPrimvarNames = std::set<TfToken>(
                        uvTokens.begin(), uvTokens.end()
                    );
                    break;
                }
            }
        }

        _surfaceShader.reset();
        if (diffuseTextureResource)
            _surfaceShader = _CreateVP2SurfaceShader(*diffuseTextureResource);

        // Mark batches dirty to force batch validation/rebuild.
        //sceneDelegate->GetRenderIndex().GetChangeTracker().MarkBatchesDirty();
    }

    *dirtyBits = HdMaterial::Clean;
}

/*! \brief  Reload the shader
*/
void HdVP2Material::Reload() {
}

/*! \brief  Returns the minimal set of dirty bits to place in the
change tracker for use in the first sync of this prim.
*/
HdDirtyBits
HdVP2Material::GetInitialDirtyBitsMask() const {
    return HdMaterial::AllDirty;
}

/*! \brief  Returns surface shader instance (or fallback if no material provided)
*/
MHWRender::MShaderInstance*
HdVP2Material::GetSurfaceShader() const {
    return _surfaceShader ? _surfaceShader.get()
        : _renderDelegate->GetFallbackShader();
}

/*! \brief  Returns displacement shader instance or nullptr
*/
MHWRender::MShaderInstance*
HdVP2Material::GetDisplacementShader() const {
    return _displacementShader.get();
}

PXR_NAMESPACE_CLOSE_SCOPE
