//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#ifndef MATERIALX_OGSXMLGENERATOR_H
#define MATERIALX_OGSXMLGENERATOR_H

/// @file
/// OGS XML fragments generator
#include <mayaUsd/base/api.h>

#include <MaterialXGenShader/ShaderGenerator.h>

MATERIALX_NAMESPACE_BEGIN

class MAYAUSD_CORE_PUBLIC OgsXmlGenerator
{
public:
    /// Generate OSG XML for the given shader fragments, output to the given stream.
    static string
    generate(const string& shaderName, const Shader& glslShader, const std::string& hlslSource);

    /// Generate light rig graph for the given shader fragments, output to the given stream.
    static string generateLightRig(
        const string& shaderName,
        const string& baseShaderName,
        const Shader& glslShader);

    static bool   isSamplerName(const string&);
    static string textureToSamplerName(const string&);
    static string samplerToTextureName(const string&);

    /// String constants
    static const string OUTPUT_NAME;
    static const string VP_TRANSPARENCY_NAME;

    /// Use Maya's latest external light functions
    static int  useLightAPI();
    static void setUseLightAPI(int);

    /// Replace every texcoord use with this UV set name (optional):
    /// Empty string will let texcoord generate their usual code.
    static const string& getPrimaryUVSetName();
    static void          setPrimaryUVSetName(const string& mainUvSetName);

private:
    static const string SAMPLER_SUFFIX;
    static const string OCIO_SAMPLER_SUFFIX;
    static const string OCIO_SAMPLER_PREFIX;
    static int          sUseLightAPI;
    static string       sPrimaryUVSetName;
};

MATERIALX_NAMESPACE_END

#endif
