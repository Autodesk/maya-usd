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
#include "material.h"

#include "debugCodes.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "render_delegate.h"
#include "tokens.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/render/vp2ShaderFragments/shaderFragments.h>
#include <mayaUsd/utils/hash.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/getenv.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/imaging/hd/sceneDelegate.h>

#ifdef WANT_MATERIALX_BUILD
#include <pxr/imaging/hdMtlx/hdMtlx.h>
#endif
#include <pxr/pxr.h>
#include <pxr/usd/ar/packageUtils.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/usdHydra/tokens.h>
#include <pxr/usdImaging/usdImaging/textureUtils.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/M3dView.h>
#include <maya/MFragmentManager.h>
#include <maya/MGlobal.h>
#include <maya/MProfiler.h>
#include <maya/MShaderManager.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MTextureManager.h>
#include <maya/MUintArray.h>
#include <maya/MViewport2Renderer.h>

#ifdef WANT_MATERIALX_BUILD
#include <mayaUsd/render/MaterialXGenOgsXml/OgsFragment.h>
#include <mayaUsd/render/MaterialXGenOgsXml/OgsXmlGenerator.h>

#include <MaterialXCore/Document.h>
#include <MaterialXFormat/File.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXGenShader/HwShaderGenerator.h>
#include <MaterialXGenShader/ShaderStage.h>
#include <MaterialXGenShader/Util.h>
#include <MaterialXRender/ImageHandler.h>
#endif

#if PXR_VERSION <= 2008
// Needed for GL_HALF_FLOAT.
#include <GL/glew.h>
#endif

#include <ghc/filesystem.hpp>
#include <tbb/parallel_for.h>

#include <iostream>
#include <sstream>
#include <string>

#if PXR_VERSION >= 2102
#include <pxr/imaging/hdSt/udimTextureObject.h>
#else
#include <pxr/imaging/glf/udimTexture.h>
#endif

#if PXR_VERSION >= 2102
#include <pxr/imaging/hio/image.h>
#else
#include <pxr/imaging/glf/image.h>
#endif

#ifdef WANT_MATERIALX_BUILD
namespace mx = MaterialX;
#endif

PXR_NAMESPACE_OPEN_SCOPE

static bool _IsDisabledAsyncTextureLoading()
{
    static const MString kOptionVarName(MayaUsdOptionVars->DisableAsyncTextureLoading.GetText());
    if (MGlobal::optionVarExists(kOptionVarName)) {
        return MGlobal::optionVarIntValue(kOptionVarName);
    }
    return true;
}

// Refresh viewport duration (in milliseconds)
static const std::size_t kRefreshDuration { 1000 };

namespace {

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (file)
    (opacity)
    (useSpecularWorkflow)
    (st)
    (varname)
    (result)
    (cardsUv)
    (sourceColorSpace)
    (sRGB)
    (raw)
    (glslfx)
    (fallback)

    (input)
    (output)

    (diffuseColor)
    (rgb)
    (r)
    (g)
    (b)
    (a)

    (xyz)
    (x)
    (y)
    (z)
    (w)

    (Float4ToFloatX)
    (Float4ToFloatY)
    (Float4ToFloatZ)
    (Float4ToFloatW)
    (Float4ToFloat3)

    (UsdDrawModeCards)
    (FallbackShader)
    (mayaIsBackFacing)
    (isBackfacing)
    ((DrawMode, "drawMode.glslfx"))

    (UsdPrimvarReader_color)
    (UsdPrimvarReader_vector)

    (Unknown)
    (Computed)
);
// clang-format on

#ifdef WANT_MATERIALX_BUILD

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _mtlxTokens,

    (USD_Mtlx_VP2_Material)
    (NG_Maya)
    (image)
    (tiledimage)
    (i_geomprop_)
    (geomprop)
    (uaddressmode)
    (vaddressmode)
    (filtertype)
    (channels)

    // Texcoord reader identifiers:
    (index)
    (UV0)
    (geompropvalue)
    (ST_reader)
    (vector2)
);

const std::set<std::string> _mtlxTopoNodeSet = {
    // Topo affecting nodes due to object/model/world space parameter
    "position",
    "normal",
    "tangent",
    "bitangent",
    // Topo affecting nodes due to channel index. We remap to geomprop in _AddMissingTexcoordReaders
    "texcoord",
    // Color at vertices also affect topo, but we have not locked a naming scheme to go from index
    // based to name based as we did for UV sets. We will mark them as topo-affecting, but there is
    // nothing we can do to link them correctly to a primvar without specifying a naming scheme.
    "geomcolor",
    // Geompropvalue are the best way to reference a primvar by name. The primvar name is
    // topo-affecting. Note that boolean and string are not supported by the GLSL codegen.
    "geompropvalue",
    // Swizzles are inlined into the codegen and affect topology.
    "swizzle",
    // Conversion nodes:
    "convert",
    // Constants: they get inlined in the source.
    "constant"

};

// clang-format on

struct _MaterialXData
{
    _MaterialXData()
    {
        _mtlxLibrary = mx::createDocument();
        _mtlxSearchPath = HdMtlxSearchPaths();

        mx::loadLibraries({}, _mtlxSearchPath, _mtlxLibrary);
    }
    MaterialX::FileSearchPath _mtlxSearchPath; //!< MaterialX library search path
    MaterialX::DocumentPtr    _mtlxLibrary;    //!< MaterialX library
};

_MaterialXData& _GetMaterialXData()
{
    static std::unique_ptr<_MaterialXData> materialXData;
    static std::once_flag                  once;
    std::call_once(once, []() { materialXData.reset(new _MaterialXData()); });

    return *materialXData;
}

//! Return true if that node parameter has topological impact on the generated code.
//
// Swizzle and geompropvalue nodes are known to have an attribute that affects
// shader topology. The "channels" and "geomprop" attributes will have effects at the codegen level,
// not at runtime. Yes, this is forbidden internal knowledge of the MaterialX shader generator and
// we might get other nodes like this one in a future update.
//
// The index input of the texcoord and geomcolor nodes affect which stream to read and is topo
// affecting.
//
// Any geometric input that can specify model/object/world space is also topo affecting.
//
// Things to look out for are parameters of type "string" and parameters with the "uniform"
// metadata. These need to be reviewed against the code used in their registered
// implementations (see registerImplementation calls in the GlslShaderGenerator CTOR). Sadly
// we can not make that a rule because the filename of an image node is both a "string" and
// has the "uniform" metadata, yet is not affecting topology.
bool _IsTopologicalNode(const HdMaterialNode2& inNode)
{
    mx::NodeDefPtr nodeDef
        = _GetMaterialXData()._mtlxLibrary->getNodeDef(inNode.nodeTypeId.GetString());
    if (nodeDef) {
        return _mtlxTopoNodeSet.find(nodeDef->getNodeString()) != _mtlxTopoNodeSet.cend();
    }
    return false;
}

bool _IsMaterialX(const HdMaterialNode& node)
{
    SdrRegistry&    shaderReg = SdrRegistry::GetInstance();
    NdrNodeConstPtr ndrNode = shaderReg.GetNodeByIdentifier(node.identifier);

    return ndrNode && ndrNode->GetSourceType() == HdVP2Tokens->mtlx;
}

//! Helper function to generate a topo hash that can be used to detect if two networks share the
//  same topology.
size_t _GenerateNetwork2TopoHash(const HdMaterialNetwork2& materialNetwork)
{
    // The HdMaterialNetwork2 structure is stable. Everything is alphabetically sorted.
    size_t topoHash = 0;
    for (const auto& c : materialNetwork.terminals) {
        MayaUsd::hash_combine(topoHash, hash_value(c.first));
        MayaUsd::hash_combine(topoHash, hash_value(c.second.upstreamNode));
        MayaUsd::hash_combine(topoHash, hash_value(c.second.upstreamOutputName));
    }
    for (const auto& nodePair : materialNetwork.nodes) {
        MayaUsd::hash_combine(topoHash, hash_value(nodePair.first));

        const auto& node = nodePair.second;
        MayaUsd::hash_combine(topoHash, hash_value(node.nodeTypeId));

        if (_IsTopologicalNode(node)) {
            // We need to capture values that affect topology:
            for (auto const& p : node.parameters) {
                MayaUsd::hash_combine(topoHash, hash_value(p.first));
                MayaUsd::hash_combine(topoHash, hash_value(p.second));
            }
        }
        for (auto const& i : node.inputConnections) {
            MayaUsd::hash_combine(topoHash, hash_value(i.first));
            for (auto const& c : i.second) {
                MayaUsd::hash_combine(topoHash, hash_value(c.upstreamNode));
                MayaUsd::hash_combine(topoHash, hash_value(c.upstreamOutputName));
            }
        }
    }
    return topoHash;
}

//! Helper function to generate a XML string about nodes, relationships and primvars in the
//! specified material network.
std::string _GenerateXMLString(const HdMaterialNetwork2& materialNetwork)
{
    std::ostringstream result;

    if (ARCH_LIKELY(!materialNetwork.nodes.empty())) {

        result << "<terminals>\n";
        for (const auto& c : materialNetwork.terminals) {
            result << "  <terminal name=\"" << c.first << "\" dest=\"" << c.second.upstreamNode
                   << "\"/>\n";
        }
        result << "</terminals>\n";
        result << "<nodes>\n";
        for (const auto& nodePair : materialNetwork.nodes) {
            const auto& node = nodePair.second;
            const bool  hasChildren = !(node.parameters.empty() && node.inputConnections.empty());
            result << "  <node path=\"" << nodePair.first << "\" id=\"" << node.nodeTypeId << "\""
                   << (hasChildren ? ">\n" : "/>\n");
            if (!node.parameters.empty()) {
                result << "    <parameters>\n";
                for (auto const& p : node.parameters) {
                    result << "      <param name=\"" << p.first << "\" value=\"" << p.second
                           << "\"/>\n";
                }
                result << "    </parameters>\n";
            }
            if (!node.inputConnections.empty()) {
                result << "    <inputs>\n";
                for (auto const& i : node.inputConnections) {
                    if (i.second.size() == 1) {
                        result << "      <input name=\"" << i.first << "\" dest=\""
                               << i.second.back().upstreamNode << "."
                               << i.second.back().upstreamOutputName << "\"/>\n";
                    } else {
                        // Extremely rare case seen only with array connections.
                        result << "      <input name=\"" << i.first << "\">\n";
                        result << "      <connections>\n";
                        for (auto const& c : i.second) {
                            result << "        <cnx dest=\"" << c.upstreamNode << "."
                                   << c.upstreamOutputName << "\"/>\n";
                        }
                        result << "      </connections>\n";
                    }
                }
                result << "    </inputs>\n";
            }
            if (hasChildren) {
                result << "  </node>\n";
            }
        }
        result << "</nodes>\n";
        // We do not add primvars. They are found later while traversing the actual effect instance.
    }

    return result.str();
}

// MaterialX FA nodes will "upgrade" the in2 uniform to whatever the vector type it needs for its
// arithmetic operation. So we need to "upgrade" the value we want to set as well.
//
// One example: ND_multiply_vector3FA(vector3 in1, float in2) will generate a float3 in2 uniform.
MStatus _SetFAParameter(
    MHWRender::MShaderInstance* surfaceShader,
    const HdMaterialNode&       node,
    const MString&              paramName,
    float                       val)
{
    auto _endsWith = [](const std::string& s, const std::string& suffix) {
        return s.size() >= suffix.size()
            && s.compare(s.size() - suffix.size(), std::string::npos, suffix) == 0;
    };

    if (_IsMaterialX(node) && _endsWith(paramName.asChar(), "_in2")
        && _endsWith(node.identifier.GetString(), "FA")) {
        // Try as vector
        float vec[4] { val, val, val, val };
        return surfaceShader->setParameter(paramName, &vec[0]);
    }
    return MS::kFailure;
}

// MaterialX has a lot of node definitions that will auto-connect to a zero-index texture coordinate
// system. To make these graphs compatible, we will redirect any unconnected input that uses such an
// auto-connection scheme to instead read a texcoord geomprop called "st" which is the canonical
// name for UV at index zero.
void _AddMissingTexcoordReaders(mx::DocumentPtr& mtlxDoc)
{
    // We expect only one node graph, but fixing them all is not an issue:
    for (mx::NodeGraphPtr nodeGraph : mtlxDoc->getNodeGraphs()) {
        if (nodeGraph->hasSourceUri()) {
            continue;
        }
        // This will hold the emergency "ST" reader if one was necessary
        mx::NodePtr stReader;
        // Store nodes to delete when loop iteration is complete
        std::vector<std::string> nodesToDelete;

        for (mx::NodePtr node : nodeGraph->getNodes()) {
            // Check the inputs of the node for UV0 default geom properties
            mx::NodeDefPtr nodeDef = node->getNodeDef();
            // A missing node def is a very bad sign. No need to process further.
            if (!TF_VERIFY(
                    nodeDef,
                    "Could not find MaterialX NodeDef for Node '%s'. Please recheck library paths.",
                    node->getNamePath().c_str())) {
                return;
            }
            for (mx::InputPtr input : nodeDef->getInputs()) {
                if (input->hasDefaultGeomPropString()
                    && input->getDefaultGeomPropString() == _mtlxTokens->UV0.GetString()) {
                    // See if the corresponding input is connected on the node:
                    if (node->getConnectedNodeName(input->getName()).empty()) {
                        // Create emergency ST reader if necessary
                        if (!stReader) {
                            stReader = nodeGraph->addNode(
                                _mtlxTokens->geompropvalue.GetString(),
                                _mtlxTokens->ST_reader.GetString(),
                                _mtlxTokens->vector2.GetString());
                            mx::ValueElementPtr prpInput
                                = stReader->addInputFromNodeDef(_mtlxTokens->geomprop.GetString());
                            prpInput->setValueString(_tokens->st.GetString());
                        }
                        node->addInputFromNodeDef(input->getName());
                        node->setConnectedNodeName(input->getName(), stReader->getName());
                    }
                }
            }
            // Check if it is an explicit texcoord reader:
            if (nodeDef->getNodeString() == "texcoord") {
                // Switch it with a geompropvalue of the same name:
                std::string nodeName = node->getName();
                std::string oldName = nodeName + "_toDelete";
                node->setName(oldName);
                nodesToDelete.push_back(oldName);
                // Find out if there is an explicit stream index:
                int          streamIndex = 0;
                mx::InputPtr indexInput = node->getInput(_mtlxTokens->index.GetString());
                if (indexInput && indexInput->hasValue()) {
                    mx::ValuePtr indexValue = indexInput->getValue();
                    if (indexValue->isA<int>()) {
                        streamIndex = indexValue->asA<int>();
                    }
                }
                // Add replacement geompropvalue node:
                mx::NodePtr doppelNode = nodeGraph->addNode(
                    _mtlxTokens->geompropvalue.GetString(),
                    nodeName,
                    nodeDef->getOutput("out")->getType());
                mx::ValueElementPtr prpInput
                    = doppelNode->addInputFromNodeDef(_mtlxTokens->geomprop.GetString());
                MString primvar = _tokens->st.GetText();
                if (streamIndex) {
                    // If reading at index > 0 we add the index to the primvar name:
                    primvar += streamIndex;
                }
                prpInput->setValueString(primvar.asChar());
            }
        }
        // Delete all obsolete texcoord reader nodes.
        for (const std::string& deadNode : nodesToDelete) {
            nodeGraph->removeNode(deadNode);
        }
    }
}

#endif // WANT_MATERIALX_BUILD

bool _IsUsdDrawModeId(const TfToken& id)
{
    return id == _tokens->DrawMode || id == _tokens->UsdDrawModeCards;
}

bool _IsUsdDrawModeNode(const HdMaterialNode& node) { return _IsUsdDrawModeId(node.identifier); }

//! Helper utility function to test whether a node is a UsdShade primvar reader.
bool _IsUsdPrimvarReader(const HdMaterialNode& node)
{
    const TfToken& id = node.identifier;
    return (
        id == UsdImagingTokens->UsdPrimvarReader_float
        || id == UsdImagingTokens->UsdPrimvarReader_float2
        || id == UsdImagingTokens->UsdPrimvarReader_float3
        || id == UsdImagingTokens->UsdPrimvarReader_float4 || id == _tokens->UsdPrimvarReader_vector
        || id == UsdImagingTokens->UsdPrimvarReader_int);
}

bool _IsUsdFloat2PrimvarReader(const HdMaterialNode& node)
{
    return (node.identifier == UsdImagingTokens->UsdPrimvarReader_float2);
}

//! Helper utility function to test whether a node is a UsdShade UV texture.
bool _IsUsdUVTexture(const HdMaterialNode& node)
{
    if (node.identifier.GetString().rfind(UsdImagingTokens->UsdUVTexture.GetString(), 0) == 0) {
        return true;
    }

#ifdef WANT_MATERIALX_BUILD
    if (_IsMaterialX(node)) {
        mx::NodeDefPtr nodeDef
            = _GetMaterialXData()._mtlxLibrary->getNodeDef(node.identifier.GetString());
        if (nodeDef
            && (nodeDef->getNodeString() == _mtlxTokens->image.GetString()
                || nodeDef->getNodeString() == _mtlxTokens->tiledimage.GetString())) {
            return true;
        }
    }
#endif

    return false;
}

//! Helper function to generate a XML string about nodes, relationships and primvars in the
//! specified material network.
std::string _GenerateXMLString(const HdMaterialNetwork& materialNetwork, bool includeParams = true)
{
    std::string result;

    if (ARCH_LIKELY(!materialNetwork.nodes.empty())) {
        // Reserve enough memory to avoid memory reallocation.
        result.reserve(1024);

        result += "<nodes>\n";

        if (includeParams) {
            for (const HdMaterialNode& node : materialNetwork.nodes) {
                result += "  <node path=\"";
                result += node.path.GetString();
                result += "\" id=\"";
                result += node.identifier;
                result += "\">\n";

                result += "    <params>\n";

                for (auto const& parameter : node.parameters) {
                    result += "      <param name=\"";
                    result += parameter.first;
                    result += "\" value=\"";
                    result += TfStringify(parameter.second);
                    result += "\"/>\n";
                }

                result += "    </params>\n";

                result += "  </node>\n";
            }
        } else {
            for (const HdMaterialNode& node : materialNetwork.nodes) {
                result += "  <node path=\"";
                result += node.path.GetString();
                result += "\" id=\"";
                result += node.identifier;
                result += "\"/>\n";
            }
        }

        result += "</nodes>\n";

        if (!materialNetwork.relationships.empty()) {
            result += "<relationships>\n";

            for (const HdMaterialRelationship& rel : materialNetwork.relationships) {
                result += "  <rel from=\"";
                result += rel.inputId.GetString();
                result += ".";
                result += rel.inputName;
                result += "\" to=\"";
                result += rel.outputId.GetString();
                result += ".";
                result += rel.outputName;
                result += "\"/>\n";
            }

            result += "</relationships>\n";
        }

        if (!materialNetwork.primvars.empty()) {
            result += "<primvars>\n";

            for (TfToken const& primvar : materialNetwork.primvars) {
                result += "  <primvar name=\"";
                result += primvar;
                result += "\"/>\n";
            }

            result += "</primvars>\n";
        }
    }

    return result;
}

//! Return true if the surface shader has its opacity attribute connected to a node which isn't
//! a USD primvar reader.
bool _IsTransparent(const HdMaterialNetwork& network)
{
    const HdMaterialNode& surfaceShader = network.nodes.back();

    for (const HdMaterialRelationship& rel : network.relationships) {
        if (rel.outputName == _tokens->opacity && rel.outputId == surfaceShader.path) {
            for (const HdMaterialNode& node : network.nodes) {
                if (node.path == rel.inputId) {
                    return !_IsUsdPrimvarReader(node);
                }
            }
        }
    }

    return false;
}

//! Helper utility function to convert Hydra texture addressing token to VP2 enum.
MHWRender::MSamplerState::TextureAddress _ConvertToTextureSamplerAddressEnum(const TfToken& token)
{
    MHWRender::MSamplerState::TextureAddress address;

    if (token == UsdHydraTokens->clamp) {
        address = MHWRender::MSamplerState::kTexClamp;
    } else if (token == UsdHydraTokens->mirror) {
        address = MHWRender::MSamplerState::kTexMirror;
    } else if (token == UsdHydraTokens->black) {
        address = MHWRender::MSamplerState::kTexBorder;
    } else {
        address = MHWRender::MSamplerState::kTexWrap;
    }

    return address;
}

//! Get sampler state description as required by the material node.
MHWRender::MSamplerStateDesc _GetSamplerStateDesc(const HdMaterialNode& node)
{
    TF_VERIFY(_IsUsdUVTexture(node));

    MHWRender::MSamplerStateDesc desc;
    desc.filter = MHWRender::MSamplerState::kMinMagMipLinear;

#ifdef WANT_MATERIALX_BUILD
    const bool isMaterialXNode = _IsMaterialX(node);
    auto       it
        = node.parameters.find(isMaterialXNode ? _mtlxTokens->uaddressmode : UsdHydraTokens->wrapS);
#else
    auto it = node.parameters.find(UsdHydraTokens->wrapS);
#endif
    if (it != node.parameters.end()) {
        const VtValue& value = it->second;
        if (value.IsHolding<TfToken>()) {
            const TfToken& token = value.UncheckedGet<TfToken>();
            desc.addressU = _ConvertToTextureSamplerAddressEnum(token);
        }
#ifdef WANT_MATERIALX_BUILD
        if (value.IsHolding<std::string>()) {
            TfToken token(value.UncheckedGet<std::string>().c_str());
            desc.addressU = _ConvertToTextureSamplerAddressEnum(token);
        }
#endif
    }

#ifdef WANT_MATERIALX_BUILD
    it = node.parameters.find(isMaterialXNode ? _mtlxTokens->vaddressmode : UsdHydraTokens->wrapT);
#else
    it = node.parameters.find(UsdHydraTokens->wrapT);
#endif
    if (it != node.parameters.end()) {
        const VtValue& value = it->second;
        if (value.IsHolding<TfToken>()) {
            const TfToken& token = value.UncheckedGet<TfToken>();
            desc.addressV = _ConvertToTextureSamplerAddressEnum(token);
        }
#ifdef WANT_MATERIALX_BUILD
        if (value.IsHolding<std::string>()) {
            TfToken token(value.UncheckedGet<std::string>().c_str());
            desc.addressV = _ConvertToTextureSamplerAddressEnum(token);
        }
#endif
    }

    it = node.parameters.find(_tokens->fallback);
    if (it != node.parameters.end()) {
        const VtValue& value = it->second;
        if (value.IsHolding<GfVec4f>()) {
            const GfVec4f& fallbackValue = value.UncheckedGet<GfVec4f>();
            float const*   value = fallbackValue.data();
            std::copy(value, value + 4, desc.borderColor);
        }
    }

    return desc;
}

MHWRender::MTexture*
_LoadUdimTexture(const std::string& path, bool& isColorSpaceSRGB, MFloatArray& uvScaleOffset)
{
    /*
        For this method to work path needs to be an absolute file path, not an asset path.
        That means that this function depends on the changes in 4e426565 to materialAdapther.cpp
        to work. As of my writing this 4e426565 is not in the USD that MayaUSD normally builds
        against so this code will fail, because UsdImaging_GetUdimTiles won't file the tiles
        because we don't know where on disk to look for them.

        https://github.com/PixarAnimationStudios/USD/commit/4e42656543f4e3a313ce31a81c27477d4dcb64b9
    */

    // test for a UDIM texture
#if PXR_VERSION >= 2102
    if (!HdStIsSupportedUdimTexture(path))
        return nullptr;
#else
    if (!GlfIsSupportedUdimTexture(path))
        return nullptr;
#endif

    /*
        Maya's tiled texture support is implemented quite differently from Usd's UDIM support.
        In Maya the texture tiles get combined into a single big texture, downscaling each tile
        if necessary, and filling in empty regions of a non-square tile with the undefined color.

        In USD the UDIM textures are stored in a texture array that the shader uses to draw.
    */

    MHWRender::MRenderer* const       renderer = MHWRender::MRenderer::theRenderer();
    MHWRender::MTextureManager* const textureMgr
        = renderer ? renderer->getTextureManager() : nullptr;
    if (!TF_VERIFY(textureMgr)) {
        return nullptr;
    }

    MHWRender::MTexture* texture = textureMgr->findTexture(path.c_str());
    if (texture) {
        return texture;
    }

    // HdSt sets the tile limit to the max number of textures in an array of 2d textures. OpenGL
    // says the minimum number of layers in 2048 so I'll use that.
    int                                   tileLimit = 2048;
    std::vector<std::tuple<int, TfToken>> tiles = UsdImaging_GetUdimTiles(path, tileLimit);
    if (tiles.size() == 0) {
        TF_WARN("Unable to find UDIM tiles for %s", path.c_str());
        return nullptr;
    }

    // I don't think there is a downside to setting a very high limit.
    // Maya will clamp the texture size to the VP2 texture clamp resolution and the hardware's
    // max texture size. And Maya doesn't make the tiled texture unnecessarily large. When I
    // try loading two 1k textures I end up with a tiled texture that is 2k x 1k.
    unsigned int maxWidth = 0;
    unsigned int maxHeight = 0;
    renderer->GPUmaximumOutputTargetSize(maxWidth, maxHeight);

    // Open the first image and get it's resolution. Assuming that all the tiles have the same
    // resolution, warn the user if Maya's tiled texture implementation is going to result in
    // a loss of texture data.
    {
#if PXR_VERSION >= 2102
        HioImageSharedPtr image = HioImage::OpenForReading(std::get<1>(tiles[0]).GetString());
#else
        GlfImageSharedPtr image = GlfImage::OpenForReading(std::get<1>(tiles[0]).GetString());
#endif
        if (!TF_VERIFY(image)) {
            return nullptr;
        }
        isColorSpaceSRGB = image->IsColorSpaceSRGB();
        unsigned int tileWidth = image->GetWidth();
        unsigned int tileHeight = image->GetHeight();

        int maxTileId = std::get<0>(tiles.back());
        int maxU = maxTileId % 10;
        int maxV = (maxTileId - maxU) / 10;
        if ((tileWidth * maxU > maxWidth) || (tileHeight * maxV > maxHeight))
            TF_WARN(
                "UDIM texture %s creates a tiled texture larger than the maximum texture size. Some"
                "resolution will be lost.",
                path.c_str());
    }

    MString textureName(
        path.c_str()); // used for caching, using the string with <UDIM> in it is fine
    MStringArray tilePaths;
    MFloatArray  tilePositions;
    for (auto& tile : tiles) {
        tilePaths.append(MString(std::get<1>(tile).GetText()));

#if PXR_VERSION >= 2102
        HioImageSharedPtr image = HioImage::OpenForReading(std::get<1>(tile).GetString());
#else
        GlfImageSharedPtr image = GlfImage::OpenForReading(std::get<1>(tile).GetString());
#endif
        if (!TF_VERIFY(image)) {
            return nullptr;
        }
        if (isColorSpaceSRGB != image->IsColorSpaceSRGB()) {
            TF_WARN(
                "UDIM texture %s color space doesn't match %s color space",
                std::get<1>(tile).GetText(),
                std::get<1>(tiles[0]).GetText());
        }

        // The image labeled 1001 will have id 0, 1002 will have id 1, 1011 will have id 10.
        // image 1001 starts with UV (0.0f, 0.0f), 1002 is (1.0f, 0.0f) and 1011 is (0.0f, 1.0f)
        int   tileId = std::get<0>(tile);
        float u = (float)(tileId % 10);
        float v = (float)((tileId - u) / 10);
        tilePositions.append(u);
        tilePositions.append(v);
    }

    MColor       undefinedColor(0.0f, 1.0f, 0.0f, 1.0f);
    MStringArray failedTilePaths;
    texture = textureMgr->acquireTiledTexture(
        textureName,
        tilePaths,
        tilePositions,
        undefinedColor,
        maxWidth,
        maxHeight,
        failedTilePaths,
        uvScaleOffset);

    for (unsigned int i = 0; i < failedTilePaths.length(); i++) {
        TF_WARN("Failed to load <UDIM> texture tile %s", failedTilePaths[i].asChar());
    }

    return texture;
}

MHWRender::MTexture* _GenerateFallbackTexture(
    MHWRender::MTextureManager* const textureMgr,
    const std::string&                path,
    const GfVec4f&                    fallbackColor)
{
    MHWRender::MTexture* texture = textureMgr->findTexture(path.c_str());
    if (texture) {
        return texture;
    }

    MHWRender::MTextureDescription desc;
    desc.setToDefault2DTexture();
    desc.fWidth = 1;
    desc.fHeight = 1;
    desc.fFormat = MHWRender::kR8G8B8A8_UNORM;
    desc.fBytesPerRow = 4;
    desc.fBytesPerSlice = desc.fBytesPerRow;

    std::vector<unsigned char> texels(4);
    for (size_t i = 0; i < 4; ++i) {
        float texelValue = GfClamp(fallbackColor[i], 0.0f, 1.0f);
        texels[i] = static_cast<unsigned char>(texelValue * 255.0);
    }
    return textureMgr->acquireTexture(path.c_str(), desc, texels.data());
}

//! Load texture from the specified path
MHWRender::MTexture* _LoadTexture(
    const std::string& path,
    bool               hasFallbackColor,
    const GfVec4f&     fallbackColor,
    bool&              isColorSpaceSRGB,
    MFloatArray&       uvScaleOffset)
{
    MProfilingScope profilingScope(
        HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorD_L2, "LoadTexture", path.c_str());

    // If it is a UDIM texture we need to modify the path before calling OpenForReading
#if PXR_VERSION >= 2102
    if (HdStIsSupportedUdimTexture(path))
        return _LoadUdimTexture(path, isColorSpaceSRGB, uvScaleOffset);
#else
    if (GlfIsSupportedUdimTexture(path))
        return _LoadUdimTexture(path, isColorSpaceSRGB, uvScaleOffset);
#endif

    MHWRender::MRenderer* const       renderer = MHWRender::MRenderer::theRenderer();
    MHWRender::MTextureManager* const textureMgr
        = renderer ? renderer->getTextureManager() : nullptr;
    if (!TF_VERIFY(textureMgr)) {
        return nullptr;
    }

    MHWRender::MTexture* texture = textureMgr->findTexture(path.c_str());
    if (texture) {
        return texture;
    }

#if PXR_VERSION >= 2102
    HioImageSharedPtr image = HioImage::OpenForReading(path);
#else
    GlfImageSharedPtr     image = GlfImage::OpenForReading(path);
#endif

    if (!TF_VERIFY(image, "Unable to create an image from %s", path.c_str())) {
        if (!hasFallbackColor) {
            return nullptr;
        }
        // Create a 1x1 texture of the fallback color, if it was specified:
        return _GenerateFallbackTexture(textureMgr, path, fallbackColor);
    }

    // This image is used for loading pixel data from usdz only and should
    // not trigger any OpenGL call. VP2RenderDelegate will transfer the
    // texels to GPU memory with VP2 API which is 3D API agnostic.
#if PXR_VERSION >= 2102
    HioImage::StorageSpec spec;
#else
    GlfImage::StorageSpec spec;
#endif
    spec.width = image->GetWidth();
    spec.height = image->GetHeight();
    spec.depth = 1;
#if PXR_VERSION >= 2102
    spec.format = image->GetFormat();
#elif PXR_VERSION > 2008
    spec.hioFormat = image->GetHioFormat();
#else
    spec.format = image->GetFormat();
    spec.type = image->GetType();
#endif
    spec.flipped = false;

    const int bpp = image->GetBytesPerPixel();
    const int bytesPerRow = spec.width * bpp;
    const int bytesPerSlice = bytesPerRow * spec.height;

    std::vector<unsigned char> storage(bytesPerSlice);
    spec.data = storage.data();

    if (!image->Read(spec)) {
        return nullptr;
    }

    MHWRender::MTextureDescription desc;
    desc.setToDefault2DTexture();
    desc.fWidth = spec.width;
    desc.fHeight = spec.height;
    desc.fBytesPerRow = bytesPerRow;
    desc.fBytesPerSlice = bytesPerSlice;

#if PXR_VERSION > 2008
#if PXR_VERSION >= 2102
    auto specFormat = spec.format;
#else
    auto specFormat = spec.hioFormat;
#endif
    switch (specFormat) {
    // Single Channel
    case HioFormatFloat32:
        desc.fFormat = MHWRender::kR32_FLOAT;
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    case HioFormatFloat16:
        desc.fFormat = MHWRender::kR16_FLOAT;
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    case HioFormatUNorm8:
        desc.fFormat = MHWRender::kR8_UNORM;
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;

    // Dual channel (quite rare, but seen with mono + alpha files)
    case HioFormatFloat32Vec2:
        desc.fFormat = MHWRender::kR32G32_FLOAT;
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    case HioFormatFloat16Vec2: {
        // R16G16 is not supported by VP2. Converted to R16G16B16A16.
        constexpr int bpp_8 = 8;

        desc.fFormat = MHWRender::kR16G16B16A16_FLOAT;
        desc.fBytesPerRow = spec.width * bpp_8;
        desc.fBytesPerSlice = desc.fBytesPerRow * spec.height;

        std::vector<unsigned char> texels(desc.fBytesPerSlice);

        for (int y = 0; y < spec.height; y++) {
            for (int x = 0; x < spec.width; x++) {
                const int t = spec.width * y + x;
                texels[t * bpp_8 + 0] = storage[t * bpp + 0];
                texels[t * bpp_8 + 1] = storage[t * bpp + 1];
                texels[t * bpp_8 + 2] = storage[t * bpp + 0];
                texels[t * bpp_8 + 3] = storage[t * bpp + 1];
                texels[t * bpp_8 + 4] = storage[t * bpp + 0];
                texels[t * bpp_8 + 5] = storage[t * bpp + 1];
                texels[t * bpp_8 + 6] = storage[t * bpp + 2];
                texels[t * bpp_8 + 7] = storage[t * bpp + 3];
            }
        }

        texture = textureMgr->acquireTexture(path.c_str(), desc, texels.data());
        break;
    }
    case HioFormatUNorm8Vec2:
    case HioFormatUNorm8Vec2srgb: {
        // R8G8 is not supported by VP2. Converted to R8G8B8A8.
        constexpr int bpp_4 = 4;

        desc.fFormat = MHWRender::kR8G8B8A8_UNORM;
        desc.fBytesPerRow = spec.width * bpp_4;
        desc.fBytesPerSlice = desc.fBytesPerRow * spec.height;

        std::vector<unsigned char> texels(desc.fBytesPerSlice);

        for (int y = 0; y < spec.height; y++) {
            for (int x = 0; x < spec.width; x++) {
                const int t = spec.width * y + x;
                texels[t * bpp_4] = storage[t * bpp];
                texels[t * bpp_4 + 1] = storage[t * bpp];
                texels[t * bpp_4 + 2] = storage[t * bpp];
                texels[t * bpp_4 + 3] = storage[t * bpp + 1];
            }
        }

        texture = textureMgr->acquireTexture(path.c_str(), desc, texels.data());
        isColorSpaceSRGB = image->IsColorSpaceSRGB();
        break;
    }

    // 3-Channel
    case HioFormatFloat32Vec3:
        desc.fFormat = MHWRender::kR32G32B32_FLOAT;
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    case HioFormatFloat16Vec3: {
        // R16G16B16 is not supported by VP2. Converted to R16G16B16A16.
        constexpr int bpp_8 = 8;

        desc.fFormat = MHWRender::kR16G16B16A16_FLOAT;
        desc.fBytesPerRow = spec.width * bpp_8;
        desc.fBytesPerSlice = desc.fBytesPerRow * spec.height;

        GfHalf               opaqueAlpha(1.0f);
        const unsigned short alphaBits = opaqueAlpha.bits();
        const unsigned char  lowAlpha = reinterpret_cast<const unsigned char*>(&alphaBits)[0];
        const unsigned char  highAlpha = reinterpret_cast<const unsigned char*>(&alphaBits)[1];

        std::vector<unsigned char> texels(desc.fBytesPerSlice);

        for (int y = 0; y < spec.height; y++) {
            for (int x = 0; x < spec.width; x++) {
                const int t = spec.width * y + x;
                texels[t * bpp_8 + 0] = storage[t * bpp + 0];
                texels[t * bpp_8 + 1] = storage[t * bpp + 1];
                texels[t * bpp_8 + 2] = storage[t * bpp + 2];
                texels[t * bpp_8 + 3] = storage[t * bpp + 3];
                texels[t * bpp_8 + 4] = storage[t * bpp + 4];
                texels[t * bpp_8 + 5] = storage[t * bpp + 5];
                texels[t * bpp_8 + 6] = lowAlpha;
                texels[t * bpp_8 + 7] = highAlpha;
            }
        }

        texture = textureMgr->acquireTexture(path.c_str(), desc, texels.data());
        break;
    }
    case HioFormatFloat16Vec4:
        desc.fFormat = MHWRender::kR16G16B16A16_FLOAT;
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    case HioFormatUNorm8Vec3:
    case HioFormatUNorm8Vec3srgb: {
        // R8G8B8 is not supported by VP2. Converted to R8G8B8A8.
        constexpr int bpp_4 = 4;

        desc.fFormat = MHWRender::kR8G8B8A8_UNORM;
        desc.fBytesPerRow = spec.width * bpp_4;
        desc.fBytesPerSlice = desc.fBytesPerRow * spec.height;

        std::vector<unsigned char> texels(desc.fBytesPerSlice);

        for (int y = 0; y < spec.height; y++) {
            for (int x = 0; x < spec.width; x++) {
                const int t = spec.width * y + x;
                texels[t * bpp_4] = storage[t * bpp];
                texels[t * bpp_4 + 1] = storage[t * bpp + 1];
                texels[t * bpp_4 + 2] = storage[t * bpp + 2];
                texels[t * bpp_4 + 3] = 255;
            }
        }

        texture = textureMgr->acquireTexture(path.c_str(), desc, texels.data());
        isColorSpaceSRGB = image->IsColorSpaceSRGB();
        break;
    }

    // 4-Channel
    case HioFormatFloat32Vec4:
        desc.fFormat = MHWRender::kR32G32B32A32_FLOAT;
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    case HioFormatUNorm8Vec4:
    case HioFormatUNorm8Vec4srgb:
        desc.fFormat = MHWRender::kR8G8B8A8_UNORM;
        isColorSpaceSRGB = image->IsColorSpaceSRGB();
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    default:
        TF_WARN(
            "VP2 renderer delegate: unsupported pixel format (%d) in texture file %s.",
            (int)specFormat,
            path.c_str());
        break;
    }
#else
    switch (spec.format) {
    case GL_RED:
        desc.fFormat = MHWRender::kR8_UNORM;
        if (spec.type == GL_FLOAT)
            desc.fFormat = MHWRender::kR32_FLOAT;
        else if (spec.type == GL_HALF_FLOAT)
            desc.fFormat = MHWRender::kR16_FLOAT;
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    case GL_RGB:
        if (spec.type == GL_FLOAT) {
            desc.fFormat = MHWRender::kR32G32B32_FLOAT;
            texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        } else if (spec.type == GL_HALF_FLOAT) {
            // R16G16B16 is not supported by VP2. Converted to R16G16B16A16.
            constexpr int bpp_8 = 8;

            desc.fFormat = MHWRender::kR16G16B16A16_FLOAT;
            desc.fBytesPerRow = spec.width * bpp_8;
            desc.fBytesPerSlice = desc.fBytesPerRow * spec.height;

            GfHalf               opaqueAlpha(1.0f);
            const unsigned short alphaBits = opaqueAlpha.bits();
            const unsigned char  lowAlpha = reinterpret_cast<const unsigned char*>(&alphaBits)[0];
            const unsigned char  highAlpha = reinterpret_cast<const unsigned char*>(&alphaBits)[1];

            std::vector<unsigned char> texels(desc.fBytesPerSlice);

            for (int y = 0; y < spec.height; y++) {
                for (int x = 0; x < spec.width; x++) {
                    const int t = spec.width * y + x;
                    texels[t * bpp_8 + 0] = storage[t * bpp + 0];
                    texels[t * bpp_8 + 1] = storage[t * bpp + 1];
                    texels[t * bpp_8 + 2] = storage[t * bpp + 2];
                    texels[t * bpp_8 + 3] = storage[t * bpp + 3];
                    texels[t * bpp_8 + 4] = storage[t * bpp + 4];
                    texels[t * bpp_8 + 5] = storage[t * bpp + 5];
                    texels[t * bpp_8 + 6] = lowAlpha;
                    texels[t * bpp_8 + 7] = highAlpha;
                }
            }

            texture = textureMgr->acquireTexture(path.c_str(), desc, texels.data());
        } else {
            // R8G8B8 is not supported by VP2. Converted to R8G8B8A8.
            constexpr int bpp_4 = 4;

            desc.fFormat = MHWRender::kR8G8B8A8_UNORM;
            desc.fBytesPerRow = spec.width * bpp_4;
            desc.fBytesPerSlice = desc.fBytesPerRow * spec.height;

            std::vector<unsigned char> texels(desc.fBytesPerSlice);

            for (int y = 0; y < spec.height; y++) {
                for (int x = 0; x < spec.width; x++) {
                    const int t = spec.width * y + x;
                    texels[t * bpp_4] = storage[t * bpp];
                    texels[t * bpp_4 + 1] = storage[t * bpp + 1];
                    texels[t * bpp_4 + 2] = storage[t * bpp + 2];
                    texels[t * bpp_4 + 3] = 255;
                }
            }

            texture = textureMgr->acquireTexture(path.c_str(), desc, texels.data());
            isColorSpaceSRGB = image->IsColorSpaceSRGB();
        }
        break;
    case GL_RGBA:
        if (spec.type == GL_FLOAT) {
            desc.fFormat = MHWRender::kR32G32B32A32_FLOAT;
        } else if (spec.type == GL_HALF_FLOAT) {
            desc.fFormat = MHWRender::kR16G16B16A16_FLOAT;
        } else {
            desc.fFormat = MHWRender::kR8G8B8A8_UNORM;
            isColorSpaceSRGB = image->IsColorSpaceSRGB();
        }
        texture = textureMgr->acquireTexture(path.c_str(), desc, spec.data);
        break;
    default: break;
    }
#endif

    return texture;
}

TfToken MayaDescriptorToToken(const MVertexBufferDescriptor& descriptor)
{
    // Attempt to match an MVertexBufferDescriptor to the corresponding
    // USD primvar token. The "Computed" token is used for data which
    // can be computed by an an rprim. Unknown is used for unsupported
    // descriptors.

    TfToken token = _tokens->Unknown;
    switch (descriptor.semantic()) {
    case MGeometry::kPosition: token = HdTokens->points; break;
    case MGeometry::kNormal: token = HdTokens->normals; break;
    case MGeometry::kTexture: break;
    case MGeometry::kColor: token = HdTokens->displayColor; break;
    case MGeometry::kTangent: token = _tokens->Computed; break;
    case MGeometry::kBitangent: token = _tokens->Computed; break;
    case MGeometry::kTangentWithSign: token = _tokens->Computed; break;
    default: break;
    }

    return token;
}

} // anonymous namespace

class HdVP2Material::TextureLoadingTask
{
public:
    TextureLoadingTask(
        HdVP2Material*     parent,
        HdSceneDelegate*   sceneDelegate,
        const std::string& path,
        bool               hasFallbackColor,
        const GfVec4f&     fallbackColor)
        : _parent(parent)
        , _sceneDelegate(sceneDelegate)
        , _path(path)
        , _fallbackColor(fallbackColor)
        , _hasFallbackColor(hasFallbackColor)
    {
    }

    ~TextureLoadingTask() = default;

    const HdVP2TextureInfo& GetFallbackTextureInfo()
    {
        if (!_fallbackTextureInfo._texture) {
            // Create a default texture info with fallback color
            MHWRender::MRenderer* const       renderer = MHWRender::MRenderer::theRenderer();
            MHWRender::MTextureManager* const textureMgr
                = renderer ? renderer->getTextureManager() : nullptr;
            if (textureMgr) {
                // Use a relevant but unique name if there is a fallback color
                // Otherwise reuse the same default texture
                _fallbackTextureInfo._texture.reset(_GenerateFallbackTexture(
                    textureMgr,
                    _hasFallbackColor ? _path + ".fallback" : "default_fallback",
                    _fallbackColor));
            }
        }
        return _fallbackTextureInfo;
    }

    bool EnqueueLoadOnIdle()
    {
        if (_started.exchange(true)) {
            return false;
        }
        // Push the texture loading on idle
        auto ret = MGlobal::executeTaskOnIdle(
            [](void* data) {
                auto* task = static_cast<HdVP2Material::TextureLoadingTask*>(data);
                task->_Load();
                // Once it is done, free the memory.
                delete task;
            },
            this);
        return ret == MStatus::kSuccess;
    }

    bool Terminate()
    {
        _terminated = true;
        // Return the started state to caller, the caller will delete this object
        // if this task has not started yet.
        // We will not be able to delete this object within its method.
        return !_started.load();
    }

private:
    void _Load()
    {
        if (_terminated) {
            return;
        }
        bool        isSRGB = false;
        MFloatArray uvScaleOffset;
        auto*       texture
            = _LoadTexture(_path, _hasFallbackColor, _fallbackColor, isSRGB, uvScaleOffset);
        if (_terminated) {
            return;
        }
        _parent->_UpdateLoadedTexture(_sceneDelegate, _path, texture, isSRGB, uvScaleOffset);
    }

    HdVP2TextureInfo  _fallbackTextureInfo;
    HdVP2Material*    _parent;
    HdSceneDelegate*  _sceneDelegate;
    const std::string _path;
    const GfVec4f     _fallbackColor;
    std::atomic_bool  _started { false };
    bool              _terminated { false };
    bool              _hasFallbackColor;
};

std::mutex                            HdVP2Material::_refreshMutex;
std::chrono::steady_clock::time_point HdVP2Material::_startTime;
std::atomic_size_t                    HdVP2Material::_runningTasksCounter;

/*! \brief  Releases the reference to the texture owned by a smart pointer.
 */
void HdVP2TextureDeleter::operator()(MHWRender::MTexture* texture)
{
    MRenderer* const                  renderer = MRenderer::theRenderer();
    MHWRender::MTextureManager* const textureMgr
        = renderer ? renderer->getTextureManager() : nullptr;
    if (TF_VERIFY(textureMgr)) {
        textureMgr->releaseTexture(texture);
    }
}

/*! \brief  Constructor
 */
HdVP2Material::HdVP2Material(HdVP2RenderDelegate* renderDelegate, const SdfPath& id)
    : HdMaterial(id)
    , _renderDelegate(renderDelegate)
{
}

HdVP2Material::~HdVP2Material()
{
    // Tell pending tasks or running tasks (if any) to terminate
    ClearPendingTasks();
}

/*! \brief  Synchronize VP2 state with scene delegate state based on dirty bits
 */
void HdVP2Material::Sync(
    HdSceneDelegate* sceneDelegate,
    HdRenderParam* /*renderParam*/,
    HdDirtyBits* dirtyBits)
{
    if (*dirtyBits & (HdMaterial::DirtyResource | HdMaterial::DirtyParams)) {
        const SdfPath& id = GetId();

        MProfilingScope profilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L2,
            "HdVP2Material::Sync",
            id.GetText());

        VtValue vtMatResource = sceneDelegate->GetMaterialResource(id);

        if (vtMatResource.IsHolding<HdMaterialNetworkMap>()) {
            const HdMaterialNetworkMap& networkMap
                = vtMatResource.UncheckedGet<HdMaterialNetworkMap>();

            HdMaterialNetwork bxdfNet, dispNet, vp2BxdfNet;

            TfMapLookup(networkMap.map, HdMaterialTerminalTokens->surface, &bxdfNet);
            TfMapLookup(networkMap.map, HdMaterialTerminalTokens->displacement, &dispNet);

#ifdef WANT_MATERIALX_BUILD
            if (!bxdfNet.nodes.empty()) {
                if (_IsMaterialX(bxdfNet.nodes.back())) {

                    bool               isVolume = false;
                    HdMaterialNetwork2 surfaceNetwork;
                    HdMaterialNetwork2ConvertFromHdMaterialNetworkMap(
                        networkMap, &surfaceNetwork, &isVolume);
                    if (isVolume) {
                        // Not supported.
                        return;
                    }

                    size_t topoHash = _GenerateNetwork2TopoHash(surfaceNetwork);

                    if (!_surfaceShader || topoHash != _topoHash) {
                        _surfaceShader.reset(
                            _CreateMaterialXShaderInstance(GetId(), surfaceNetwork));
                        _topoHash = topoHash;
                    }

                    if (_surfaceShader) {
                        _UpdateShaderInstance(sceneDelegate, bxdfNet);
#ifdef HDVP2_MATERIAL_CONSOLIDATION_UPDATE_WORKAROUND
                        _MaterialChanged(sceneDelegate);
#endif
                        *dirtyBits = HdMaterial::Clean;
                    }
                    return;
                }
            }
#endif

            _ApplyVP2Fixes(vp2BxdfNet, bxdfNet);

            if (!vp2BxdfNet.nodes.empty()) {
                // Generate a XML string from the material network and convert it to a token for
                // faster hashing and comparison.
                const TfToken token(_GenerateXMLString(vp2BxdfNet, false));

                // Skip creating a new shader instance if the token is unchanged. There is no plan
                // to implement fine-grain dirty bit in Hydra for the same purpose:
                // https://groups.google.com/g/usd-interest/c/xytT2azlJec/m/22Tnw4yXAAAJ
                if (_surfaceNetworkToken != token) {
                    MProfilingScope subProfilingScope(
                        HdVP2RenderDelegate::sProfilerCategory,
                        MProfiler::kColorD_L2,
                        "CreateShaderInstance");

                    // Remember the path of the surface shader for special handling: unlike other
                    // fragments, the parameters of the surface shader fragment can't be renamed.
                    _surfaceShaderId = vp2BxdfNet.nodes.back().path;

                    MHWRender::MShaderInstance* shader;

#ifndef HDVP2_DISABLE_SHADER_CACHE
                    // Acquire a shader instance from the shader cache. If a shader instance has
                    // been cached with the same token, a clone of the shader instance will be
                    // returned. Multiple clones of a shader instance will share the same shader
                    // effect, thus reduce compilation overhead and enable material consolidation.
                    shader = _renderDelegate->GetShaderFromCache(token);

                    // If the shader instance is not found in the cache, create one from the
                    // material network and add a clone to the cache for reuse.
                    if (!shader) {
                        shader = _CreateShaderInstance(vp2BxdfNet);

                        if (shader) {
                            _renderDelegate->AddShaderToCache(token, *shader);
                        }
                    }
#else
                    shader = _CreateShaderInstance(vp2BxdfNet);
#endif

                    // The shader instance is owned by the material solely.
                    _surfaceShader.reset(shader);

                    if (TfDebug::IsEnabled(HDVP2_DEBUG_MATERIAL)) {
                        std::cout << "BXDF material network for " << id << ":\n"
                                  << _GenerateXMLString(bxdfNet) << "\n"
                                  << "BXDF (with VP2 fixes) material network for " << id << ":\n"
                                  << _GenerateXMLString(vp2BxdfNet) << "\n"
                                  << "Displacement material network for " << id << ":\n"
                                  << _GenerateXMLString(dispNet) << "\n";

                        if (_surfaceShader) {
                            auto tmpDir = ghc::filesystem::temp_directory_path();
                            tmpDir /= "HdVP2Material_";
                            tmpDir += id.GetName();
                            tmpDir += ".txt";
                            _surfaceShader->writeEffectSourceToFile(tmpDir.c_str());

                            std::cout << "BXDF generated shader code for " << id << ":\n";
                            std::cout << "  " << tmpDir << "\n";
                        }
                    }

                    // Store primvar requirements.
                    _requiredPrimvars = std::move(vp2BxdfNet.primvars);

                    // Verify that _requiredPrivars contains all the requiredVertexBuffers() the
                    // shader instance needs.
                    if (shader) {
                        MVertexBufferDescriptorList requiredVertexBuffers;
                        MStatus status = shader->requiredVertexBuffers(requiredVertexBuffers);
                        if (status) {
                            for (int reqIndex = 0; reqIndex < requiredVertexBuffers.length();
                                 reqIndex++) {
                                MVertexBufferDescriptor desc;
                                requiredVertexBuffers.getDescriptor(reqIndex, desc);
                                TfToken requiredPrimvar = MayaDescriptorToToken(desc);
                                // now make sure something matching requiredPrimvar is in
                                // _requiredPrimvars
                                if (requiredPrimvar != _tokens->Unknown
                                    && requiredPrimvar != _tokens->Computed) {
                                    bool found = false;
                                    for (TfToken const& primvar : _requiredPrimvars) {
                                        if (primvar == requiredPrimvar) {
                                            found = true;
                                            break;
                                        }
                                    }
                                    if (!found) {
                                        _requiredPrimvars.push_back(requiredPrimvar);
                                    }
                                }
                            }
                        }
                    }

                    // The token is saved and will be used to determine whether a new shader
                    // instance is needed during the next sync.
                    _surfaceNetworkToken = token;

                    // If the surface shader has its opacity attribute connected to a node which
                    // isn't a primvar reader, it is set as transparent. If the opacity attr is
                    // connected to a primvar reader, the Rprim side will determine the transparency
                    // state according to the primvars:displayOpacity data. If the opacity attr
                    // isn't connected, the transparency state will be set in
                    // _UpdateShaderInstance() according to the opacity value.
                    if (shader) {
                        shader->setIsTransparent(_IsTransparent(bxdfNet));
                    }
                }

                _UpdateShaderInstance(sceneDelegate, bxdfNet);

#ifdef HDVP2_MATERIAL_CONSOLIDATION_UPDATE_WORKAROUND
                _MaterialChanged(sceneDelegate);
#endif
            }
        } else {
            TF_WARN(
                "Expected material resource for <%s> to hold HdMaterialNetworkMap,"
                "but found %s instead.",
                id.GetText(),
                vtMatResource.GetTypeName().c_str());
        }
    }

    *dirtyBits = HdMaterial::Clean;
}

/*! \brief  Returns the minimal set of dirty bits to place in the
change tracker for use in the first sync of this prim.
*/
HdDirtyBits HdVP2Material::GetInitialDirtyBitsMask() const { return HdMaterial::AllDirty; }

/*! \brief  Applies VP2-specific fixes to the material network.
 */
void HdVP2Material::_ApplyVP2Fixes(HdMaterialNetwork& outNet, const HdMaterialNetwork& inNet)
{
    // To avoid relocation, reserve enough space for possible maximal size. The
    // output network is temporary C++ object that will be released after use.
    const size_t numNodes = inNet.nodes.size();
    const size_t numRelationships = inNet.relationships.size();

    size_t nodeCounter = 0;

    _nodePathMap.clear();
    _nodePathMap.reserve(numNodes);

    HdMaterialNetwork tmpNet;
    tmpNet.nodes.reserve(numNodes);
    tmpNet.relationships.reserve(numRelationships);

    // Some material networks require us to add nodes and connections to the base
    // HdMaterialNetwork. Keep track of the existence of some key nodes to help
    // with performance.
    HdMaterialNode* usdDrawModeCardsNode = nullptr;
    HdMaterialNode* cardsUvPrimvarReader = nullptr;

    // Get the shader registry so I can look up the real names of shading nodes.
    SdrRegistry& shaderReg = SdrRegistry::GetInstance();

    // We might need to query the working color space of Maya if we hit texture nodes. Delay
    // the query until necessary.
    MString mayaWorkingColorSpace;

    // Replace the authored node paths with simplified paths in the form of "node#". By doing so
    // we will be able to reuse shader effects among material networks which have the same node
    // identifiers and relationships but different node paths, reduce shader compilation overhead
    // and enable material consolidation for faster rendering.
    for (const HdMaterialNode& node : inNet.nodes) {
        tmpNet.nodes.push_back(node);

        HdMaterialNode& outNode = tmpNet.nodes.back();

        // For card draw mode the HdMaterialNode will have an identifier which is the hash of the
        // file path to drawMode.glslfx on disk. Using that value I can get the SdrShaderNode, and
        // then get the actual name of the shader "drawMode.glslfx". For other node names the
        // HdMaterialNode identifier and the SdrShaderNode name seem to be the same, so just convert
        // everything to use the SdrShaderNode name.
        SdrShaderNodeConstPtr sdrNode
            = shaderReg.GetShaderNodeByIdentifierAndType(outNode.identifier, _tokens->glslfx);

        if (_IsUsdUVTexture(node)) {
            // We need to rename according to the Maya color working space pref:
            if (!mayaWorkingColorSpace.length()) {
                // Query the user pref:
                mayaWorkingColorSpace = MGlobal::executeCommandStringResult(
                    "colorManagementPrefs -q -renderingSpaceName");
            }
            outNode.identifier = TfToken(
                HdVP2ShaderFragments::getUsdUVTextureFragmentName(mayaWorkingColorSpace).asChar());
        } else {
            if (!sdrNode) {
                TF_WARN("Could not find a shader node for <%s>", node.path.GetText());
                return;
            }
            outNode.identifier = TfToken(sdrNode->GetName());
        }

        if (_IsUsdDrawModeNode(outNode)) {
            // I can't easily name a Maya fragment something with a '.' in it, so pick a different
            // fragment name.
            outNode.identifier = _tokens->UsdDrawModeCards;
            TF_VERIFY(!usdDrawModeCardsNode); // there should only be one.
            usdDrawModeCardsNode = &outNode;
        }

        if (_IsUsdFloat2PrimvarReader(outNode)
            && outNode.parameters[_tokens->varname] == _tokens->cardsUv) {
            TF_VERIFY(!cardsUvPrimvarReader);
            cardsUvPrimvarReader = &outNode;
        }

        outNode.path = SdfPath(outNode.identifier.GetString() + std::to_string(++nodeCounter));

        _nodePathMap[node.path] = outNode.path;
    }

    // Update the relationships to use the new node paths.
    for (const HdMaterialRelationship& rel : inNet.relationships) {
        tmpNet.relationships.push_back(rel);

        HdMaterialRelationship& outRel = tmpNet.relationships.back();
        outRel.inputId = _nodePathMap[outRel.inputId];
        outRel.outputId = _nodePathMap[outRel.outputId];
    }

    outNet.nodes.reserve(numNodes + numRelationships);
    outNet.relationships.reserve(numRelationships * 2);
    outNet.primvars.reserve(numNodes);

    // Add additional nodes necessary for Maya's fragment compiler
    // to work that are logical predecessors of node.
    auto addPredecessorNodes = [&](const HdMaterialNode& node) {
        // If the node is a UsdUVTexture node, verify there is a UsdPrimvarReader_float2 connected
        // to the st input of it. If not, find the basic st reader and/or create it and connect it.
        // Adding the UV reader only works for cards draw mode. We wouldn't know which UV stream to
        // read if another material was missing the primvar reader.
        if (_IsUsdUVTexture(node) && usdDrawModeCardsNode) {
            // the DrawModeCardsFragment has UsdUVtexture nodes without primvar readers.
            // Add a primvar reader to each UsdUVTexture which doesn't already have one.
            if (!cardsUvPrimvarReader) {
                HdMaterialNode stReader;
                stReader.identifier = UsdImagingTokens->UsdPrimvarReader_float2;
                stReader.path
                    = SdfPath(stReader.identifier.GetString() + std::to_string(++nodeCounter));
                stReader.parameters[_tokens->varname] = _tokens->cardsUv;
                outNet.nodes.push_back(stReader);
                cardsUvPrimvarReader = &outNet.nodes.back();
                // Specifically looking for the cardsUv primvar
                outNet.primvars.push_back(_tokens->cardsUv);
            }

            // search for an existing relationship between cardsUvPrimvarReader & node.
            // TODO: if there are multiple UV sets this can fail, it is looking for
            // a connection to a specific UsdPrimvarReader_float2.
            bool hasRelationship = false;
            for (const HdMaterialRelationship& rel : tmpNet.relationships) {
                if (rel.inputId == cardsUvPrimvarReader->path && rel.inputName == _tokens->result
                    && rel.outputId == node.path && rel.outputName == _tokens->st) {
                    hasRelationship = true;
                    break;
                }
            }

            if (!hasRelationship) {
                // The only case I'm currently aware of where we have UsdUVTexture nodes without
                // a corresponding UsdPrimvarReader_float2 to read the UVs is draw mode cards.
                // There could be other cases, and it could be find to add the primvar reader
                // and connection, but we want to know when it is happening.
                TF_VERIFY(usdDrawModeCardsNode);

                HdMaterialRelationship newRel
                    = { cardsUvPrimvarReader->path, _tokens->result, node.path, _tokens->st };
                outNet.relationships.push_back(newRel);
            }
        }

        // If the node is a DrawModeCardsFragment add a MayaIsBackFacing fragment to cull out
        // backfaces.
        if (_IsUsdDrawModeNode(node)) {
            // Add the MayaIsBackFacing fragment
            HdMaterialNode mayaIsBackFacingNode;
            mayaIsBackFacingNode.identifier = _tokens->mayaIsBackFacing;
            mayaIsBackFacingNode.path = SdfPath(
                mayaIsBackFacingNode.identifier.GetString() + std::to_string(++nodeCounter));
            outNet.nodes.push_back(mayaIsBackFacingNode);

            // Connect to the isBackfacing input of the DrawModeCards fragment
            HdMaterialRelationship newRel = { mayaIsBackFacingNode.path,
                                              _tokens->mayaIsBackFacing,
                                              node.path,
                                              _tokens->isBackfacing };
            outNet.relationships.push_back(newRel);
        }
    };

    // Add additional nodes necessary for Maya's fragment compiler
    // to work that are logical successors of node.
    auto addSuccessorNodes = [&](const HdMaterialNode& node, const TfToken& primvarToRead) {
        // If the node is a DrawModeCardsFragment add the fallback material after it to do
        // the lighting etc.
        if (_IsUsdDrawModeNode(node)) {

            // Add the fallback shader node and hook it up. This has to be the last node in
            // outNet.nodes.
            HdMaterialNode fallbackShaderNode;
            fallbackShaderNode.identifier = _tokens->FallbackShader;
            fallbackShaderNode.path = SdfPath(
                fallbackShaderNode.identifier.GetString() + std::to_string(++nodeCounter));
            outNet.nodes.push_back(fallbackShaderNode);

            // The DrawModeCards fragment is basically a texture picker. Connect its output to
            // the diffuseColor input of the fallback shader node.
            HdMaterialRelationship newRel
                = { node.path, _tokens->output, fallbackShaderNode.path, _tokens->diffuseColor };
            outNet.relationships.push_back(newRel);

            // Add the required primvars
            outNet.primvars.push_back(HdTokens->points);
            outNet.primvars.push_back(HdTokens->normals);

            // no passthrough nodes necessary between the draw mode cards node & the fallback
            // shader.
            return;
        }

        // Copy outgoing connections and if needed add passthrough node/connection.
        for (const HdMaterialRelationship& rel : tmpNet.relationships) {
            if (rel.inputId != node.path) {
                continue;
            }

            TfToken passThroughId;
            if (rel.inputName == _tokens->rgb || rel.inputName == _tokens->xyz) {
                passThroughId = _tokens->Float4ToFloat3;
            } else if (rel.inputName == _tokens->r || rel.inputName == _tokens->x) {
                passThroughId = _tokens->Float4ToFloatX;
            } else if (rel.inputName == _tokens->g || rel.inputName == _tokens->y) {
                passThroughId = _tokens->Float4ToFloatY;
            } else if (rel.inputName == _tokens->b || rel.inputName == _tokens->z) {
                passThroughId = _tokens->Float4ToFloatZ;
            } else if (rel.inputName == _tokens->a || rel.inputName == _tokens->w) {
                passThroughId = _tokens->Float4ToFloatW;
            } else if (primvarToRead == HdTokens->displayColor) {
                passThroughId = _tokens->Float4ToFloat3;
            } else if (primvarToRead == HdTokens->displayOpacity) {
                passThroughId = _tokens->Float4ToFloatW;
            } else {
                outNet.relationships.push_back(rel);
                continue;
            }

            const SdfPath passThroughPath(
                passThroughId.GetString() + std::to_string(++nodeCounter));

            const HdMaterialNode passThroughNode = { passThroughPath, passThroughId, {} };
            outNet.nodes.push_back(passThroughNode);

            HdMaterialRelationship newRel
                = { rel.inputId, _tokens->output, passThroughPath, _tokens->input };
            outNet.relationships.push_back(newRel);

            newRel = { passThroughPath, _tokens->output, rel.outputId, rel.outputName };
            outNet.relationships.push_back(newRel);
        }
    };

    // Add nodes necessary for the fragment compiler to produce a shader that works.
    for (const HdMaterialNode& node : tmpNet.nodes) {
        TfToken primvarToRead;

        const bool isUsdPrimvarReader = _IsUsdPrimvarReader(node);
        if (isUsdPrimvarReader) {
            auto it = node.parameters.find(_tokens->varname);
            if (it != node.parameters.end()) {
                primvarToRead = TfToken(TfStringify(it->second));
            }
        }

        addPredecessorNodes(node);
        outNet.nodes.push_back(node);

        // If the primvar reader is reading color or opacity, replace it with
        // UsdPrimvarReader_color which can create COLOR stream requirement
        // instead of generic TEXCOORD stream.
        // Do this before addSuccessorNodes, because changing the identifier may change the
        // input/output types and require another conversion node.
        if (primvarToRead == HdTokens->displayColor || primvarToRead == HdTokens->displayOpacity) {
            HdMaterialNode& nodeToChange = outNet.nodes.back();
            nodeToChange.identifier = _tokens->UsdPrimvarReader_color;
        }
        addSuccessorNodes(node, primvarToRead);

        // Normal map is not supported yet. For now primvars:normals is used for
        // shading, which is also the current behavior of USD/Hydra.
        // https://groups.google.com/d/msg/usd-interest/7epU16C3eyY/X9mLW9VFEwAJ

        // UsdImagingMaterialAdapter doesn't create primvar requirements as
        // expected. Workaround by manually looking up "varname" parameter.
        // https://groups.google.com/forum/#!msg/usd-interest/z-14AgJKOcU/1uJJ1thXBgAJ
        if (isUsdPrimvarReader) {
            if (!primvarToRead.IsEmpty()) {
                outNet.primvars.push_back(primvarToRead);
            }
        }
    }
}

#ifdef WANT_MATERIALX_BUILD

void HdVP2Material::_ApplyMtlxVP2Fixes(HdMaterialNetwork2& outNet, const HdMaterialNetwork2& inNet)
{

    // The goal here is to strip all local names in the network paths in order to reduce the shader
    // to its topological elements only.

    // We also strip all local values so that the Maya effect gets created with all values set to
    // their MaterialX default values.

    // Once we have that, we can fully re-use any previously encountered effect that has the same
    // MaterialX topology and only update the values that are found in the material network.

    size_t nodeCounter = 0;
    _nodePathMap.clear();

    // Paths will go /NG_Maya/N0, /NG_Maya/N1, /NG_Maya/N2...
    // We need NG_Maya, one level up, as this will be the name assigned to the MaterialX node graph
    // when run thru HdMtlxCreateMtlxDocumentFromHdNetwork (I know, forbidden knowledge again).
    SdfPath ngBase(_mtlxTokens->NG_Maya);

    // We will traverse the network in a depth-first traversal starting at the
    // terminals. This will allow a stable traversal that will not be affected
    // by the ordering of the SdfPaths and make sure we assign the same index to
    // all nodes regardless of the way they are sorted in the network node map.
    std::vector<const SdfPath*> pathsToTraverse;
    for (const auto& terminal : inNet.terminals) {
        const auto& connection = terminal.second;
        pathsToTraverse.push_back(&(connection.upstreamNode));
    }
    while (!pathsToTraverse.empty()) {
        const SdfPath* path = pathsToTraverse.back();
        pathsToTraverse.pop_back();
        if (!_nodePathMap.count(*path)) {
            const HdMaterialNode2& node = inNet.nodes.find(*path)->second;
            // We only need to create the anonymized name at this time:
            _nodePathMap[*path] = ngBase.AppendChild(TfToken("N" + std::to_string(nodeCounter++)));
            for (const auto& input : node.inputConnections) {
                for (const auto& connection : input.second) {
                    pathsToTraverse.push_back(&(connection.upstreamNode));
                }
            }
        }
    }

    // Copy the incoming network using only the anonymized names:
    outNet.primvars = inNet.primvars;
    for (const auto& terminal : inNet.terminals) {
        outNet.terminals.emplace(
            terminal.first,
            HdMaterialConnection2 { _nodePathMap[terminal.second.upstreamNode],
                                    terminal.second.upstreamOutputName });
    }
    for (const auto& nodePair : inNet.nodes) {
        const HdMaterialNode2& inNode = nodePair.second;
        HdMaterialNode2        outNode;
        outNode.nodeTypeId = inNode.nodeTypeId;
        if (_IsTopologicalNode(inNode)) {
            // These parameters affect topology:
            outNode.parameters = inNode.parameters;
        }
        for (const auto& cnxPair : inNode.inputConnections) {
            std::vector<HdMaterialConnection2> outCnx;
            for (const auto& c : cnxPair.second) {
                outCnx.emplace_back(
                    HdMaterialConnection2 { _nodePathMap[c.upstreamNode], c.upstreamOutputName });
            }
            outNode.inputConnections.emplace(cnxPair.first, std::move(outCnx));
        }
        outNet.nodes.emplace(_nodePathMap[nodePair.first], std::move(outNode));
    }
}

/*! \brief  Detects MaterialX networks and rehydrates them.
 */
MHWRender::MShaderInstance* HdVP2Material::_CreateMaterialXShaderInstance(
    SdfPath const&            materialId,
    HdMaterialNetwork2 const& surfaceNetwork)
{
    MHWRender::MShaderInstance* shaderInstance = nullptr;

    auto const& terminalConnIt = surfaceNetwork.terminals.find(HdMaterialTerminalTokens->surface);
    if (terminalConnIt == surfaceNetwork.terminals.end()) {
        // No surface material
        return shaderInstance;
    }

    HdMaterialNetwork2 fixedNetwork;
    _ApplyMtlxVP2Fixes(fixedNetwork, surfaceNetwork);

    SdfPath       terminalPath = terminalConnIt->second.upstreamNode;
    const TfToken shaderCacheID(_GenerateXMLString(fixedNetwork));

    // Acquire a shader instance from the shader cache. If a shader instance has been cached with
    // the same token, a clone of the shader instance will be returned. Multiple clones of a shader
    // instance will share the same shader effect, thus reduce compilation overhead and enable
    // material consolidation.
    shaderInstance = _renderDelegate->GetShaderFromCache(shaderCacheID);
    if (shaderInstance) {
        _surfaceShaderId = terminalPath;
        const TfTokenVector* cachedPrimvars = _renderDelegate->GetPrimvarsFromCache(shaderCacheID);
        if (cachedPrimvars) {
            _requiredPrimvars = *cachedPrimvars;
        }
        return shaderInstance;
    }

    SdfPath fixedPath = fixedNetwork.terminals[HdMaterialTerminalTokens->surface].upstreamNode;
    HdMaterialNode2 const* surfTerminal = &fixedNetwork.nodes[fixedPath];
    if (!surfTerminal) {
        return shaderInstance;
    }

    try {
        // The HdMtlxCreateMtlxDocumentFromHdNetwork function can throw if any MaterialX error is
        // raised.

        // Check if the Terminal is a MaterialX Node
        SdrRegistry&                sdrRegistry = SdrRegistry::GetInstance();
        const SdrShaderNodeConstPtr mtlxSdrNode = sdrRegistry.GetShaderNodeByIdentifierAndType(
            surfTerminal->nodeTypeId, HdVP2Tokens->mtlx);

        mx::DocumentPtr           mtlxDoc;
        const mx::FileSearchPath& crLibrarySearchPath(_GetMaterialXData()._mtlxSearchPath);
        if (mtlxSdrNode) {

            // Create the MaterialX Document from the HdMaterialNetwork
            std::set<SdfPath> hdTextureNodes;
            mx::StringMap     mxHdTextureMap; // Mx-Hd texture name counterparts
            mtlxDoc = HdMtlxCreateMtlxDocumentFromHdNetwork(
                fixedNetwork,
                *surfTerminal, // MaterialX HdNode
                SdfPath(_mtlxTokens->USD_Mtlx_VP2_Material),
                _GetMaterialXData()._mtlxLibrary,
                &hdTextureNodes,
                &mxHdTextureMap);

            if (!mtlxDoc) {
                return shaderInstance;
            }

            // Fix any missing texcoord reader.
            _AddMissingTexcoordReaders(mtlxDoc);

            _surfaceShaderId = terminalPath;

            if (TfDebug::IsEnabled(HDVP2_DEBUG_MATERIAL)) {
                std::cout << "generated shader code for " << materialId.GetText() << ":\n";
                std::cout << "Generated graph\n==============================\n";
                mx::writeToXmlStream(mtlxDoc, std::cout);
                std::cout << "\n==============================\n";
            }
        } else {
            return shaderInstance;
        }

        // This function is very recent and might only exist in a PR at this point in time
        // See https://github.com/autodesk-forks/MaterialX/pull/1197 for current status.
        mx::OgsXmlGenerator::setUseLightAPIV2(true);

        mx::NodePtr materialNode;
        for (const mx::NodePtr& material : mtlxDoc->getMaterialNodes()) {
            if (material->getName() == _mtlxTokens->USD_Mtlx_VP2_Material.GetText()) {
                materialNode = material;
            }
        }

        if (!materialNode) {
            return shaderInstance;
        }

        MaterialXMaya::OgsFragment ogsFragment(materialNode, crLibrarySearchPath);

        // Explore the fragment for primvars:
        mx::ShaderPtr            shader = ogsFragment.getShader();
        const mx::VariableBlock& vertexInputs
            = shader->getStage(mx::Stage::VERTEX).getInputBlock(mx::HW::VERTEX_INPUTS);
        for (size_t i = 0; i < vertexInputs.size(); ++i) {
            const mx::ShaderPort* variable = vertexInputs[i];
            // Position is always assumed.
            // Tangent will be generated in the vertex shader using a utility fragment
            if (variable->getName() == mx::HW::T_IN_NORMAL) {
                _requiredPrimvars.push_back(HdTokens->normals);
            }
        }

        MHWRender::MRenderer* const renderer = MHWRender::MRenderer::theRenderer();
        if (!TF_VERIFY(renderer)) {
            return shaderInstance;
        }

        MHWRender::MFragmentManager* const fragmentManager = renderer->getFragmentManager();
        if (!TF_VERIFY(fragmentManager)) {
            return shaderInstance;
        }

        MString fragmentName(ogsFragment.getFragmentName().c_str());

        if (!fragmentManager->hasFragment(fragmentName)) {
            std::string   fragSrc = ogsFragment.getFragmentSource();
            const MString registeredFragment
                = fragmentManager->addShadeFragmentFromBuffer(fragSrc.c_str(), false);
            if (registeredFragment.length() == 0) {
                TF_WARN("Failed to register shader fragment %s", fragmentName.asChar());
                return shaderInstance;
            }
        }

        const MHWRender::MShaderManager* const shaderMgr = renderer->getShaderManager();
        if (!TF_VERIFY(shaderMgr)) {
            return shaderInstance;
        }

        shaderInstance = shaderMgr->getFragmentShader(fragmentName, "outColor", true);

        // Find named primvar readers:
        MStringArray parameterList;
        shaderInstance->parameterList(parameterList);
        for (unsigned int i = 0; i < parameterList.length(); ++i) {
            static const unsigned int u_geomprop_length
                = static_cast<unsigned int>(_mtlxTokens->i_geomprop_.GetString().length());
            if (parameterList[i].substring(0, u_geomprop_length - 1)
                == _mtlxTokens->i_geomprop_.GetText()) {
                MString varname
                    = parameterList[i].substring(u_geomprop_length, parameterList[i].length());
                shaderInstance->renameParameter(parameterList[i], varname);
                _requiredPrimvars.push_back(TfToken(varname.asChar()));
            }
        }

        // Add automatic tangent generation:
        shaderInstance->addInputFragment("materialXTw", "Tw", "Tw");

        shaderInstance->setIsTransparent(ogsFragment.isTransparent());

    } catch (mx::Exception& e) {
        TF_RUNTIME_ERROR(
            "Caught exception '%s' while processing '%s'", e.what(), materialId.GetText());
        return nullptr;
    }

    if (TfDebug::IsEnabled(HDVP2_DEBUG_MATERIAL)) {
        std::cout << "BXDF material network for " << materialId << ":\n"
                  << _GenerateXMLString(surfaceNetwork) << "\n"
                  << "Topology-only network for " << materialId << ":\n"
                  << shaderCacheID << "\n"
                  << "Required primvars:\n";

        for (TfToken const& primvar : _requiredPrimvars) {
            std::cout << "\t" << primvar << std::endl;
        }

        auto tmpDir = ghc::filesystem::temp_directory_path();
        tmpDir /= "HdVP2Material_";
        tmpDir += materialId.GetName();
        tmpDir += ".txt";
        shaderInstance->writeEffectSourceToFile(tmpDir.c_str());

        std::cout << "BXDF generated shader code for " << materialId << ":\n";
        std::cout << "  " << tmpDir << "\n";
    }

    if (shaderInstance) {
        _renderDelegate->AddShaderToCache(shaderCacheID, *shaderInstance);
        _renderDelegate->AddPrimvarsToCache(shaderCacheID, _requiredPrimvars);
    }

    return shaderInstance;
}

#endif

/*! \brief  Creates a shader instance for the surface shader.
 */
MHWRender::MShaderInstance* HdVP2Material::_CreateShaderInstance(const HdMaterialNetwork& mat)
{
    MHWRender::MRenderer* const renderer = MHWRender::MRenderer::theRenderer();
    if (!TF_VERIFY(renderer)) {
        return nullptr;
    }

    const MHWRender::MShaderManager* const shaderMgr = renderer->getShaderManager();
    if (!TF_VERIFY(shaderMgr)) {
        return nullptr;
    }

    MHWRender::MShaderInstance* shaderInstance = nullptr;

    // MShaderInstance supports multiple connections between shaders on Maya 2018.7, 2019.3, 2020
    // and above.
#if (MAYA_API_VERSION >= 20190300) \
    || ((MAYA_API_VERSION >= 20180700) && (MAYA_API_VERSION < 20190000))

    // UsdImagingMaterialAdapter has walked the shader graph and emitted nodes
    // and relationships in topological order to avoid forward-references, thus
    // we can run a reverse iteration to avoid connecting a fragment before any
    // of its downstream fragments.
    const auto rend = mat.nodes.rend();
    for (auto rit = mat.nodes.rbegin(); rit != rend; rit++) {
        const HdMaterialNode& node = *rit;

        const MString nodeId = node.identifier.GetText();
        const MString nodeName = node.path.GetNameToken().GetText();

        if (shaderInstance == nullptr) {
            shaderInstance = shaderMgr->getFragmentShader(nodeId, "outSurfaceFinal", true);
            if (shaderInstance == nullptr) {
                TF_WARN("Failed to create shader instance for %s", nodeId.asChar());
                break;
            }

            continue;
        }

        MStringArray outputNames, inputNames;

        for (const HdMaterialRelationship& rel : mat.relationships) {
            if (rel.inputId == node.path) {
                MString outputName = rel.inputName.GetText();
                outputNames.append(outputName);

                if (rel.outputId != _surfaceShaderId) {
                    std::string str = rel.outputId.GetName();
                    str += rel.outputName.GetString();
                    inputNames.append(str.c_str());
                } else {
                    inputNames.append(rel.outputName.GetText());
                }
            }
        }

        if (outputNames.length() > 0) {
            MUintArray invalidParamIndices;
            MStatus    status = shaderInstance->addInputFragmentForMultiParams(
                nodeId, nodeName, outputNames, inputNames, &invalidParamIndices);

            if (!status && TfDebug::IsEnabled(HDVP2_DEBUG_MATERIAL)) {
                TF_WARN(
                    "Error %s happened when connecting shader %s",
                    status.errorString().asChar(),
                    node.path.GetText());

                const unsigned int len = invalidParamIndices.length();
                for (unsigned int i = 0; i < len; i++) {
                    unsigned int   index = invalidParamIndices[i];
                    const MString& outputName = outputNames[index];
                    const MString& inputName = inputNames[index];
                    TF_WARN("  %s -> %s", outputName.asChar(), inputName.asChar());
                }
            }

            if (_IsUsdPrimvarReader(node)) {
                auto it = node.parameters.find(_tokens->varname);
                if (it != node.parameters.end()) {
                    const MString paramName = HdTokens->primvar.GetText();
                    const MString varname = TfStringify(it->second).c_str();
                    shaderInstance->renameParameter(paramName, varname);
                }
            }
        } else {
            TF_DEBUG(HDVP2_DEBUG_MATERIAL)
                .Msg("Failed to connect shader %s\n", node.path.GetText());
        }
    }

#elif MAYA_API_VERSION >= 20190000

    // UsdImagingMaterialAdapter has walked the shader graph and emitted nodes
    // and relationships in topological order to avoid forward-references, thus
    // we can run a reverse iteration to avoid connecting a fragment before any
    // of its downstream fragments.
    const auto rend = mat.nodes.rend();
    for (auto rit = mat.nodes.rbegin(); rit != rend; rit++) {
        const HdMaterialNode& node = *rit;

        const MString nodeId = node.identifier.GetText();
        const MString nodeName = node.path.GetNameToken().GetText();

        if (shaderInstance == nullptr) {
            shaderInstance = shaderMgr->getFragmentShader(nodeId, "outSurfaceFinal", true);
            if (shaderInstance == nullptr) {
                TF_WARN("Failed to create shader instance for %s", nodeId.asChar());
                break;
            }

            continue;
        }

        MStringArray outputNames, inputNames;

        std::string primvarname;

        for (const HdMaterialRelationship& rel : mat.relationships) {
            if (rel.inputId == node.path) {
                outputNames.append(rel.inputName.GetText());
                inputNames.append(rel.outputName.GetText());
            }

            if (_IsUsdUVTexture(node)) {
                if (rel.outputId == node.path && rel.outputName == _tokens->st) {
                    for (const HdMaterialNode& n : mat.nodes) {
                        if (n.path == rel.inputId && _IsUsdPrimvarReader(n)) {
                            auto it = n.parameters.find(_tokens->varname);
                            if (it != n.parameters.end()) {
                                primvarname = TfStringify(it->second);
                            }
                            break;
                        }
                    }
                }
            }
        }

        // Without multi-connection support for MShaderInstance, this code path
        // can only support common patterns of UsdShade material network, i.e.
        // a UsdUVTexture is connected to a single input of a USD Preview Surface.
        // More generic fix is coming.
        if (outputNames.length() == 1) {
            MStatus status
                = shaderInstance->addInputFragment(nodeId, outputNames[0], inputNames[0]);

            if (!status) {
                TF_DEBUG(HDVP2_DEBUG_MATERIAL)
                    .Msg(
                        "Error %s happened when connecting shader %s\n",
                        status.errorString().asChar(),
                        node.path.GetText());
            }

            if (_IsUsdUVTexture(node)) {
                const MString paramNames[]
                    = { "file", "fileSampler", "isColorSpaceSRGB", "fallback", "scale", "bias" };

                for (const MString& paramName : paramNames) {
                    const MString resolvedName = nodeName + paramName;
                    shaderInstance->renameParameter(paramName, resolvedName);
                }

                const MString paramName = _tokens->st.GetText();
                shaderInstance->setSemantic(paramName, "uvCoord");
                shaderInstance->setAsVarying(paramName, true);
                shaderInstance->renameParameter(paramName, primvarname.c_str());
            }
        } else {
            TF_DEBUG(HDVP2_DEBUG_MATERIAL)
                .Msg("Failed to connect shader %s\n", node.path.GetText());

            if (outputNames.length() > 1) {
                TF_DEBUG(HDVP2_DEBUG_MATERIAL)
                    .Msg("MShaderInstance doesn't support "
                         "multiple connections between shaders on the current Maya version.\n");
            }
        }
    }

#endif

    return shaderInstance;
}

/*! \brief  Updates parameters for the surface shader.
 */
void HdVP2Material::_UpdateShaderInstance(
    HdSceneDelegate*         sceneDelegate,
    const HdMaterialNetwork& mat)
{
    if (!_surfaceShader) {
        return;
    }

    MProfilingScope profilingScope(
        HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorD_L2, "UpdateShaderInstance");

    for (const HdMaterialNode& node : mat.nodes) {
        MString nodeName = "";
#ifdef WANT_MATERIALX_BUILD
        const bool isMaterialXNode = _IsMaterialX(node);
        if (isMaterialXNode) {
            mx::NodeDefPtr nodeDef
                = _GetMaterialXData()._mtlxLibrary->getNodeDef(node.identifier.GetString());
            if (nodeDef
                && _mtlxTopoNodeSet.find(nodeDef->getNodeString()) != _mtlxTopoNodeSet.cend()) {
                // A topo node does not emit editable parameters:
                continue;
            }
            nodeName += _nodePathMap[node.path].GetName().c_str();
            if (node.path == _surfaceShaderId) {
                nodeName = "";
            } else {
                nodeName += "_";
            }
        } else
#endif
        {
            // Find the simplified path for the authored node path from the map which has been
            // created when applying VP2-specific fixes.
            const auto it = _nodePathMap.find(node.path);
            if (it == _nodePathMap.end()) {
                continue;
            }

            // The simplified path has only one token which is the node name.
            const SdfPath& nodePath = it->second;
            if (nodePath != _surfaceShaderId) {
                nodeName = nodePath.GetText();
            }
        }

        MStatus samplerStatus = MStatus::kFailure;

        if (_IsUsdUVTexture(node)) {
            const MHWRender::MSamplerStateDesc desc = _GetSamplerStateDesc(node);
            const MHWRender::MSamplerState*    sampler = _renderDelegate->GetSamplerState(desc);
            if (sampler) {
#ifdef WANT_MATERIALX_BUILD
                if (isMaterialXNode) {
                    const MString paramName = "_" + nodeName + "file_sampler";
                    samplerStatus = _surfaceShader->setParameter(paramName, *sampler);
                } else
#endif
                {
                    const MString paramName = nodeName + "fileSampler";
                    samplerStatus = _surfaceShader->setParameter(paramName, *sampler);
                }
            }
        }

        for (auto const& entry : node.parameters) {
            const TfToken& token = entry.first;
            const VtValue& value = entry.second;

            MString paramName = nodeName + token.GetText();

            MStatus status = MStatus::kFailure;

            if (value.IsHolding<float>()) {
                const float& val = value.UncheckedGet<float>();
                status = _surfaceShader->setParameter(paramName, val);

#ifdef WANT_MATERIALX_BUILD
                if (!status) {
                    status = _SetFAParameter(_surfaceShader.get(), node, paramName, val);
                }
#endif
                // The opacity parameter can be found and updated only when it
                // has no connection. In this case, transparency of the shader
                // is solely determined by the opacity value.
                if (status && nodeName.length() == 0 && token == _tokens->opacity) {
                    _surfaceShader->setIsTransparent(val < 0.999f);
                }
            } else if (value.IsHolding<GfVec2f>()) {
                const float* val = value.UncheckedGet<GfVec2f>().data();
                status = _surfaceShader->setParameter(paramName, val);
            } else if (value.IsHolding<GfVec3f>()) {
                const float* val = value.UncheckedGet<GfVec3f>().data();
                status = _surfaceShader->setParameter(paramName, val);
            } else if (value.IsHolding<GfVec4f>()) {
                const float* val = value.UncheckedGet<GfVec4f>().data();
                status = _surfaceShader->setParameter(paramName, val);
            } else if (value.IsHolding<TfToken>()) {
                if (_IsUsdUVTexture(node)) {
                    if (token == UsdHydraTokens->wrapS || token == UsdHydraTokens->wrapT) {
                        // The two parameters have been converted to sampler state before entering
                        // this loop.
                        status = samplerStatus;
                    } else if (token == _tokens->sourceColorSpace) {
                        status = MStatus::kSuccess;
                    }
                }
            } else if (value.IsHolding<SdfAssetPath>()) {
                const SdfAssetPath& val = value.UncheckedGet<SdfAssetPath>();
                const std::string&  resolvedPath = val.GetResolvedPath();
                const std::string&  assetPath = val.GetAssetPath();
                if (_IsUsdUVTexture(node) && token == _tokens->file) {
                    const HdVP2TextureInfo& info = _AcquireTexture(
                        sceneDelegate, !resolvedPath.empty() ? resolvedPath : assetPath, node);

                    MHWRender::MTextureAssignment assignment;
                    assignment.texture = info._texture.get();
                    status = _surfaceShader->setParameter(paramName, assignment);

#ifdef WANT_MATERIALX_BUILD
                    // TODO: MaterialX image nodes have colorSpace metadata on the file attribute,
                    // and this can be found in the UsdShade version of the MaterialX document. At
                    // this point in time, there is no mechanism in Hydra to transmit metadata so
                    // this information will not reach the render delegate. Follow
                    // https://github.com/PixarAnimationStudios/USD/issues/1523 for future updates
                    // on colorspace handling in MaterialX/Hydra.
                    if (status && !isMaterialXNode) {
#else
                    if (status) {
#endif
                        paramName = nodeName + "isColorSpaceSRGB";
                        bool isSRGB = info._isColorSpaceSRGB;
                        auto scsIt = node.parameters.find(_tokens->sourceColorSpace);
                        if (scsIt != node.parameters.end()) {
                            const VtValue& scsValue = scsIt->second;
                            if (scsValue.IsHolding<TfToken>()) {
                                const TfToken& scsToken = scsValue.UncheckedGet<TfToken>();
                                if (scsToken == _tokens->raw) {
                                    isSRGB = false;
                                } else if (scsToken == _tokens->sRGB) {
                                    isSRGB = true;
                                }
                            }
                        }
                        status = _surfaceShader->setParameter(paramName, isSRGB);
                    }
                    // These parameters allow scaling texcoords into the proper coordinates of the
                    // Maya UDIM texture atlas:
                    if (status) {
#ifdef WANT_MATERIALX_BUILD
                        paramName = nodeName + (isMaterialXNode ? "uv_scale" : "stScale");
#else
                        paramName = nodeName + "stScale";
#endif
                        status = _surfaceShader->setParameter(paramName, info._stScale.data());
                    }
                    if (status) {
#ifdef WANT_MATERIALX_BUILD
                        paramName = nodeName + (isMaterialXNode ? "uv_offset" : "stOffset");
#else
                        paramName = nodeName + "stOffset";
#endif
                        status = _surfaceShader->setParameter(paramName, info._stOffset.data());
                    }
                }
            } else if (value.IsHolding<int>()) {
                const int& val = value.UncheckedGet<int>();
                if (node.identifier == UsdImagingTokens->UsdPreviewSurface
                    && token == _tokens->useSpecularWorkflow) {
                    status = _surfaceShader->setParameter(paramName, val != 0);
                } else {
                    status = _surfaceShader->setParameter(paramName, val);
                }
            } else if (value.IsHolding<bool>()) {
                const bool& val = value.UncheckedGet<bool>();
                status = _surfaceShader->setParameter(paramName, val);
            } else if (value.IsHolding<GfMatrix4d>()) {
                MMatrix matrix;
                value.UncheckedGet<GfMatrix4d>().Get(matrix.matrix);
                status = _surfaceShader->setParameter(paramName, matrix);
            } else if (value.IsHolding<GfMatrix4f>()) {
                MFloatMatrix matrix;
                value.UncheckedGet<GfMatrix4f>().Get(matrix.matrix);
                status = _surfaceShader->setParameter(paramName, matrix);
#ifdef WANT_MATERIALX_BUILD
            } else if (value.IsHolding<std::string>()) {
                // Some MaterialX nodes have a string member that does not translate to a shader
                // parameter.
                if (isMaterialXNode
                    && (token == _mtlxTokens->geomprop || token == _mtlxTokens->uaddressmode
                        || token == _mtlxTokens->vaddressmode || token == _mtlxTokens->filtertype
                        || token == _mtlxTokens->channels)) {
                    status = MS::kSuccess;
                }
#endif
            }

            if (!status) {
                TF_DEBUG(HDVP2_DEBUG_MATERIAL)
                    .Msg("Failed to set shader parameter %s\n", paramName.asChar());
            }
        }
    }
}

/*! \brief  Acquires a texture for the given image path.
 */
const HdVP2TextureInfo& HdVP2Material::_AcquireTexture(
    HdSceneDelegate*      sceneDelegate,
    const std::string&    path,
    const HdMaterialNode& node)
{
    const auto it = _textureMap.find(path);
    if (it != _textureMap.end()) {
        return it->second;
    }

    // Get fallback color if defined
    bool    hasFallbackColor { false };
    GfVec4f fallbackColor { 0.18, 0.18, 0.18, 0 };
    auto    fallbackIt = node.parameters.find(_tokens->fallback);
    if (fallbackIt != node.parameters.end() && fallbackIt->second.IsHolding<GfVec4f>()) {
        fallbackColor = fallbackIt->second.UncheckedGet<GfVec4f>();
        hasFallbackColor = true;
    }

    if (_IsDisabledAsyncTextureLoading()) {
        bool        isSRGB = false;
        MFloatArray uvScaleOffset;

        MHWRender::MTexture* texture
            = _LoadTexture(path, hasFallbackColor, fallbackColor, isSRGB, uvScaleOffset);

        HdVP2TextureInfo& info = _textureMap[path];
        info._texture.reset(texture);
        info._isColorSpaceSRGB = isSRGB;
        if (uvScaleOffset.length() > 0) {
            TF_VERIFY(uvScaleOffset.length() == 4);
            info._stScale.Set(
                uvScaleOffset[0], uvScaleOffset[1]); // The first 2 elements are the scale
            info._stOffset.Set(
                uvScaleOffset[2], uvScaleOffset[3]); // The next two elements are the offset
        }

        return info;
    }

    auto* task = new TextureLoadingTask(this, sceneDelegate, path, hasFallbackColor, fallbackColor);
    _textureLoadingTasks.emplace(path, task);
    return task->GetFallbackTextureInfo();
}

void HdVP2Material::EnqueueLoadTextures()
{
    for (const auto& task : _textureLoadingTasks) {
        if (task.second->EnqueueLoadOnIdle()) {
            ++_runningTasksCounter;
        }
    }
}

void HdVP2Material::ClearPendingTasks()
{
    // Inform tasks that have not started or finished that this material object
    // is no longer valid
    for (auto& task : _textureLoadingTasks) {
        if (task.second->Terminate()) {
            // Delete the pointer: we can only do that outside of the object scope
            delete task.second;
        }
    }

    // Remove the reference of all the tasks
    _textureLoadingTasks.clear();
    // Reset counter, tasks that have started but not finished yet would be
    // terminated and won't trigger any refresh
    _runningTasksCounter = 0;
}

void HdVP2Material::_UpdateLoadedTexture(
    HdSceneDelegate*     sceneDelegate,
    const std::string&   path,
    MHWRender::MTexture* texture,
    bool                 isColorSpaceSRGB,
    const MFloatArray&   uvScaleOffset)
{
    // Decrease the counter if texture finished loading.
    // Please notice that we do not do the same thing for terminated tasks,
    // when termination is requested, the scene delegate is being reset and
    // the counter would be reset to 0 (see `ClearPendingTasks()` method),
    // no need to decrease the counter one by one.
    if (_runningTasksCounter.load() > 0) {
        --_runningTasksCounter;
    }

    // Pop the task object from the container, since this method is
    // called directly from the task object method `loadOnIdle()`,
    // we do not handle the deletion here, we will let the
    // function on idle to delete the task object.
    _textureLoadingTasks.erase(path);

    // Check the local cache again, do not overwrite if same texture has
    // been loaded asynchronously
    if (_textureMap.find(path) != _textureMap.end()) {
        return;
    }

    HdVP2TextureInfo& info = _textureMap[path];
    info._texture.reset(texture);
    info._isColorSpaceSRGB = isColorSpaceSRGB;
    if (uvScaleOffset.length() > 0) {
        TF_VERIFY(uvScaleOffset.length() == 4);
        info._stScale.Set(uvScaleOffset[0], uvScaleOffset[1]); // The first 2 elements are the scale
        info._stOffset.Set(
            uvScaleOffset[2], uvScaleOffset[3]); // The next two elements are the offset
    }

    // Mark sprim dirty
    sceneDelegate->GetRenderIndex().GetChangeTracker().MarkSprimDirty(
        GetId(), HdMaterial::DirtyResource);

    _ScheduleRefresh();
}

/*static*/
void HdVP2Material::_ScheduleRefresh()
{
    // We need this mutex due to the variables used in this method are static
    std::lock_guard<std::mutex> lock(_refreshMutex);

    auto isTimeout = []() {
        auto diff(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - _startTime));
        if (static_cast<std::size_t>(diff.count()) < kRefreshDuration) {
            return false;
        }
        _startTime = std::chrono::steady_clock::now();
        return true;
    };

    // Trigger refresh for the last texture or when it is timeout
    if (!_runningTasksCounter.load() || isTimeout()) {
        M3dView::scheduleRefreshAllViews();
    }
}

#ifdef HDVP2_MATERIAL_CONSOLIDATION_UPDATE_WORKAROUND

void HdVP2Material::SubscribeForMaterialUpdates(const SdfPath& rprimId)
{
    std::lock_guard<std::mutex> lock(_materialSubscriptionsMutex);

    _materialSubscriptions.insert(rprimId);
}

void HdVP2Material::UnsubscribeFromMaterialUpdates(const SdfPath& rprimId)
{
    std::lock_guard<std::mutex> lock(_materialSubscriptionsMutex);

    _materialSubscriptions.erase(rprimId);
}

void HdVP2Material::_MaterialChanged(HdSceneDelegate* sceneDelegate)
{
    std::lock_guard<std::mutex> lock(_materialSubscriptionsMutex);

    HdChangeTracker& changeTracker = sceneDelegate->GetRenderIndex().GetChangeTracker();
    for (const SdfPath& rprimId : _materialSubscriptions) {
        changeTracker.MarkRprimDirty(rprimId, HdChangeTracker::DirtyMaterialId);
    }
}

#endif

PXR_NAMESPACE_CLOSE_SCOPE
