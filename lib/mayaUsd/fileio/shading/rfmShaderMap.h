//
// Copyright 2016 Pixar
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
// This table comes from RenderMan for Maya version 23.5, and is based on the
// config/mayaTranslation.json in the installation.
//
// The software does under the hood transformation implementing some Maya
// native shader nodes as RIS pattern objects. The conversion allows writing
// a fully working shader graph to USD.
// We use this table both for export and for import when RIS mode is active.

#ifndef MAYAUSD_FILEIO_SHADING_RFM_SHADER_MAP_H
#define MAYAUSD_FILEIO_SHADING_RFM_SHADER_MAP_H

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

static const std::vector<std::pair<TfToken, TfToken>> RfmNodesToShaderIds = {
    // These Maya nodes are provided by Maya itself, so we map a Maya node type
    // name to a "Pxr" prefixed RenderMan for Maya shader ID.
    { TfToken("bifrostAeroMaterial", TfToken::Immortal),
      TfToken("PxrBifrostAero", TfToken::Immortal) },
    { TfToken("xgen_hair_physical", TfToken::Immortal),
      TfToken("PxrMayaXgenHairPhysical", TfToken::Immortal) },
    { TfToken("blendColors", TfToken::Immortal), TfToken("PxrMayaBlendColors", TfToken::Immortal) },
    { TfToken("blinn", TfToken::Immortal), TfToken("PxrMayaBlinn", TfToken::Immortal) },
    { TfToken("bulge", TfToken::Immortal), TfToken("PxrMayaBulge", TfToken::Immortal) },
    { TfToken("bump2d", TfToken::Immortal), TfToken("PxrMayaBump2d", TfToken::Immortal) },
    { TfToken("bump3d", TfToken::Immortal), TfToken("PxrMayaBump3d", TfToken::Immortal) },
    { TfToken("brownian", TfToken::Immortal), TfToken("PxrMayaBrownian", TfToken::Immortal) },
    { TfToken("checker", TfToken::Immortal), TfToken("PxrMayaChecker", TfToken::Immortal) },
    { TfToken("clamp", TfToken::Immortal), TfToken("PxrMayaClamp", TfToken::Immortal) },
    { TfToken("cloth", TfToken::Immortal), TfToken("PxrMayaCloth", TfToken::Immortal) },
    { TfToken("cloud", TfToken::Immortal), TfToken("PxrMayaCloud", TfToken::Immortal) },
    { TfToken("condition", TfToken::Immortal), TfToken("PxrCondition", TfToken::Immortal) },
    { TfToken("contrast", TfToken::Immortal), TfToken("PxrMayaContrast", TfToken::Immortal) },
    { TfToken("crater", TfToken::Immortal), TfToken("PxrMayaCrater", TfToken::Immortal) },
    { TfToken("file", TfToken::Immortal), TfToken("PxrMayaFile", TfToken::Immortal) },
    { TfToken("fluidShape", TfToken::Immortal), TfToken("PxrMayaFluidShape", TfToken::Immortal) },
    { TfToken("fractal", TfToken::Immortal), TfToken("PxrMayaFractal", TfToken::Immortal) },
    { TfToken("gammaCorrect", TfToken::Immortal),
      TfToken("PxrMayaGammaCorrect", TfToken::Immortal) },
    { TfToken("granite", TfToken::Immortal), TfToken("PxrMayaGranite", TfToken::Immortal) },
    { TfToken("grid", TfToken::Immortal), TfToken("PxrMayaGrid", TfToken::Immortal) },
    { TfToken("hairSystem", TfToken::Immortal), TfToken("PxrMayaHair", TfToken::Immortal) },
    { TfToken("hsvToRgb", TfToken::Immortal), TfToken("PxrMayaHsvToRgb", TfToken::Immortal) },
    { TfToken("imagePlane", TfToken::Immortal), TfToken("PxrMayaImagePlane", TfToken::Immortal) },
    { TfToken("lambert", TfToken::Immortal), TfToken("PxrMayaLambert", TfToken::Immortal) },
    { TfToken("layeredTexture", TfToken::Immortal),
      TfToken("PxrMayaLayeredTexture", TfToken::Immortal) },
    { TfToken("leather", TfToken::Immortal), TfToken("PxrMayaLeather", TfToken::Immortal) },
    { TfToken("luminance", TfToken::Immortal), TfToken("PxrMayaLuminance", TfToken::Immortal) },
    { TfToken("marble", TfToken::Immortal), TfToken("PxrMayaMarble", TfToken::Immortal) },
    { TfToken("mountain", TfToken::Immortal), TfToken("PxrMayaMountain", TfToken::Immortal) },
    { TfToken("multiplyDivide", TfToken::Immortal),
      TfToken("PxrMultiplyDivide", TfToken::Immortal) },
    { TfToken("noise", TfToken::Immortal), TfToken("PxrMayaNoise", TfToken::Immortal) },
    { TfToken("place2dTexture", TfToken::Immortal),
      TfToken("PxrMayaPlacement2d", TfToken::Immortal) },
    { TfToken("place3dTexture", TfToken::Immortal),
      TfToken("PxrMayaPlacement3d", TfToken::Immortal) },
    { TfToken("plusMinusAverage", TfToken::Immortal),
      TfToken("PxrMayaPlusMinusAverage", TfToken::Immortal) },
    { TfToken("projection", TfToken::Immortal), TfToken("PxrMayaProjection", TfToken::Immortal) },
    { TfToken("ramp", TfToken::Immortal), TfToken("PxrMayaRamp", TfToken::Immortal) },
    { TfToken("remapColor", TfToken::Immortal), TfToken("PxrMayaRemapColor", TfToken::Immortal) },
    { TfToken("remapHsv", TfToken::Immortal), TfToken("PxrMayaRemapHsv", TfToken::Immortal) },
    { TfToken("remapValue", TfToken::Immortal), TfToken("PxrMayaRemapValue", TfToken::Immortal) },
    { TfToken("reverse", TfToken::Immortal), TfToken("PxrMayaReverse", TfToken::Immortal) },
    { TfToken("rgbToHsv", TfToken::Immortal), TfToken("PxrMayaRgbToHsv", TfToken::Immortal) },
    { TfToken("rock", TfToken::Immortal), TfToken("PxrMayaRock", TfToken::Immortal) },
    { TfToken("setRange", TfToken::Immortal), TfToken("PxrMayaSetRange", TfToken::Immortal) },
    { TfToken("snow", TfToken::Immortal), TfToken("PxrMayaSnow", TfToken::Immortal) },
    { TfToken("solidFractal", TfToken::Immortal),
      TfToken("PxrMayaSolidFractal", TfToken::Immortal) },
    { TfToken("stucco", TfToken::Immortal), TfToken("PxrMayaStucco", TfToken::Immortal) },
    { TfToken("uvChooser", TfToken::Immortal), TfToken("PxrMayaUVChooser", TfToken::Immortal) },
    { TfToken("volumeFog", TfToken::Immortal), TfToken("PxrMayaVolumeFog", TfToken::Immortal) },
    { TfToken("volumeNoise", TfToken::Immortal), TfToken("PxrMayaVolumeNoise", TfToken::Immortal) },
    { TfToken("wood", TfToken::Immortal), TfToken("PxrMayaWood", TfToken::Immortal) },
    { TfToken("locator", TfToken::Immortal), TfToken("PxrProcedural", TfToken::Immortal) },

    // These nodes are provided by RenderMan for Maya, so the Maya node type
    // name matches the RenderMan for Maya shader ID exactly.
    // The list can be obtained in Maya with this Python snippet:
    //     cmds.loadPlugin('RenderMan_for_Maya')
    //     allRfmNodes = cmds.pluginInfo('RenderMan_for_Maya', q=True, dependNode=True)
    //     sorted([node for node in allRfmNodes if node.startswith('Pxr')])
    { TfToken("PxrAdjustNormal", TfToken::Immortal),
      TfToken("PxrAdjustNormal", TfToken::Immortal) },
    { TfToken("PxrAovLight", TfToken::Immortal), TfToken("PxrAovLight", TfToken::Immortal) },
    { TfToken("PxrAttribute", TfToken::Immortal), TfToken("PxrAttribute", TfToken::Immortal) },
    { TfToken("PxrBackgroundDisplayFilter", TfToken::Immortal),
      TfToken("PxrBackgroundDisplayFilter", TfToken::Immortal) },
    { TfToken("PxrBackgroundSampleFilter", TfToken::Immortal),
      TfToken("PxrBackgroundSampleFilter", TfToken::Immortal) },
    { TfToken("PxrBakePointCloud", TfToken::Immortal),
      TfToken("PxrBakePointCloud", TfToken::Immortal) },
    { TfToken("PxrBakeTexture", TfToken::Immortal), TfToken("PxrBakeTexture", TfToken::Immortal) },
    { TfToken("PxrBarnLightFilter", TfToken::Immortal),
      TfToken("PxrBarnLightFilter", TfToken::Immortal) },
    { TfToken("PxrBlack", TfToken::Immortal), TfToken("PxrBlack", TfToken::Immortal) },
    { TfToken("PxrBlackBody", TfToken::Immortal), TfToken("PxrBlackBody", TfToken::Immortal) },
    { TfToken("PxrBlend", TfToken::Immortal), TfToken("PxrBlend", TfToken::Immortal) },
    { TfToken("PxrBlockerLightFilter", TfToken::Immortal),
      TfToken("PxrBlockerLightFilter", TfToken::Immortal) },
    { TfToken("PxrBump", TfToken::Immortal), TfToken("PxrBump", TfToken::Immortal) },
    { TfToken("PxrBumpManifold2D", TfToken::Immortal),
      TfToken("PxrBumpManifold2D", TfToken::Immortal) },
    { TfToken("PxrCamera", TfToken::Immortal), TfToken("PxrCamera", TfToken::Immortal) },
    { TfToken("PxrChecker", TfToken::Immortal), TfToken("PxrChecker", TfToken::Immortal) },
    { TfToken("PxrClamp", TfToken::Immortal), TfToken("PxrClamp", TfToken::Immortal) },
    { TfToken("PxrColorCorrect", TfToken::Immortal),
      TfToken("PxrColorCorrect", TfToken::Immortal) },
    { TfToken("PxrConstant", TfToken::Immortal), TfToken("PxrConstant", TfToken::Immortal) },
    { TfToken("PxrCookieLightFilter", TfToken::Immortal),
      TfToken("PxrCookieLightFilter", TfToken::Immortal) },
    { TfToken("PxrCopyAOVDisplayFilter", TfToken::Immortal),
      TfToken("PxrCopyAOVDisplayFilter", TfToken::Immortal) },
    { TfToken("PxrCopyAOVSampleFilter", TfToken::Immortal),
      TfToken("PxrCopyAOVSampleFilter", TfToken::Immortal) },
    { TfToken("PxrCross", TfToken::Immortal), TfToken("PxrCross", TfToken::Immortal) },
    { TfToken("PxrCryptomatte", TfToken::Immortal), TfToken("PxrCryptomatte", TfToken::Immortal) },
    { TfToken("PxrCurvature", TfToken::Immortal), TfToken("PxrCurvature", TfToken::Immortal) },
    { TfToken("PxrCylinderCamera", TfToken::Immortal),
      TfToken("PxrCylinderCamera", TfToken::Immortal) },
    { TfToken("PxrCylinderLight", TfToken::Immortal),
      TfToken("PxrCylinderLight", TfToken::Immortal) },
    { TfToken("PxrDefault", TfToken::Immortal), TfToken("PxrDefault", TfToken::Immortal) },
    { TfToken("PxrDiffuse", TfToken::Immortal), TfToken("PxrDiffuse", TfToken::Immortal) },
    { TfToken("PxrDirectLighting", TfToken::Immortal),
      TfToken("PxrDirectLighting", TfToken::Immortal) },
    { TfToken("PxrDirt", TfToken::Immortal), TfToken("PxrDirt", TfToken::Immortal) },
    { TfToken("PxrDiskLight", TfToken::Immortal), TfToken("PxrDiskLight", TfToken::Immortal) },
    { TfToken("PxrDisney", TfToken::Immortal), TfToken("PxrDisney", TfToken::Immortal) },
    { TfToken("PxrDispScalarLayer", TfToken::Immortal),
      TfToken("PxrDispScalarLayer", TfToken::Immortal) },
    { TfToken("PxrDispTransform", TfToken::Immortal),
      TfToken("PxrDispTransform", TfToken::Immortal) },
    { TfToken("PxrDispVectorLayer", TfToken::Immortal),
      TfToken("PxrDispVectorLayer", TfToken::Immortal) },
    { TfToken("PxrDisplace", TfToken::Immortal), TfToken("PxrDisplace", TfToken::Immortal) },
    { TfToken("PxrDistantLight", TfToken::Immortal),
      TfToken("PxrDistantLight", TfToken::Immortal) },
    { TfToken("PxrDomeLight", TfToken::Immortal), TfToken("PxrDomeLight", TfToken::Immortal) },
    { TfToken("PxrDot", TfToken::Immortal), TfToken("PxrDot", TfToken::Immortal) },
    { TfToken("PxrEdgeDetect", TfToken::Immortal), TfToken("PxrEdgeDetect", TfToken::Immortal) },
    { TfToken("PxrEnvDayLight", TfToken::Immortal), TfToken("PxrEnvDayLight", TfToken::Immortal) },
    { TfToken("PxrExposure", TfToken::Immortal), TfToken("PxrExposure", TfToken::Immortal) },
    { TfToken("PxrFacingRatio", TfToken::Immortal), TfToken("PxrFacingRatio", TfToken::Immortal) },
    { TfToken("PxrFilmicTonemapperDisplayFilter", TfToken::Immortal),
      TfToken("PxrFilmicTonemapperDisplayFilter", TfToken::Immortal) },
    { TfToken("PxrFilmicTonemapperSampleFilter", TfToken::Immortal),
      TfToken("PxrFilmicTonemapperSampleFilter", TfToken::Immortal) },
    { TfToken("PxrFlakes", TfToken::Immortal), TfToken("PxrFlakes", TfToken::Immortal) },
    { TfToken("PxrFractal", TfToken::Immortal), TfToken("PxrFractal", TfToken::Immortal) },
    { TfToken("PxrGamma", TfToken::Immortal), TfToken("PxrGamma", TfToken::Immortal) },
    { TfToken("PxrGoboLightFilter", TfToken::Immortal),
      TfToken("PxrGoboLightFilter", TfToken::Immortal) },
    { TfToken("PxrGradeDisplayFilter", TfToken::Immortal),
      TfToken("PxrGradeDisplayFilter", TfToken::Immortal) },
    { TfToken("PxrGradeSampleFilter", TfToken::Immortal),
      TfToken("PxrGradeSampleFilter", TfToken::Immortal) },
    { TfToken("PxrHSL", TfToken::Immortal), TfToken("PxrHSL", TfToken::Immortal) },
    { TfToken("PxrHairColor", TfToken::Immortal), TfToken("PxrHairColor", TfToken::Immortal) },
    { TfToken("PxrHalfBufferErrorFilter", TfToken::Immortal),
      TfToken("PxrHalfBufferErrorFilter", TfToken::Immortal) },
    { TfToken("PxrImageDisplayFilter", TfToken::Immortal),
      TfToken("PxrImageDisplayFilter", TfToken::Immortal) },
    { TfToken("PxrIntMultLightFilter", TfToken::Immortal),
      TfToken("PxrIntMultLightFilter", TfToken::Immortal) },
    { TfToken("PxrInvert", TfToken::Immortal), TfToken("PxrInvert", TfToken::Immortal) },
    { TfToken("PxrLayer", TfToken::Immortal), TfToken("PxrLayer", TfToken::Immortal) },
    { TfToken("PxrLayerMixer", TfToken::Immortal), TfToken("PxrLayerMixer", TfToken::Immortal) },
    { TfToken("PxrLayerSurface", TfToken::Immortal),
      TfToken("PxrLayerSurface", TfToken::Immortal) },
    { TfToken("PxrLayeredBlend", TfToken::Immortal),
      TfToken("PxrLayeredBlend", TfToken::Immortal) },
    { TfToken("PxrLayeredTexture", TfToken::Immortal),
      TfToken("PxrLayeredTexture", TfToken::Immortal) },
    { TfToken("PxrLightProbe", TfToken::Immortal), TfToken("PxrLightProbe", TfToken::Immortal) },
    { TfToken("PxrLightSaturation", TfToken::Immortal),
      TfToken("PxrLightSaturation", TfToken::Immortal) },
    { TfToken("PxrManifold2D", TfToken::Immortal), TfToken("PxrManifold2D", TfToken::Immortal) },
    { TfToken("PxrManifold3D", TfToken::Immortal), TfToken("PxrManifold3D", TfToken::Immortal) },
    { TfToken("PxrMarschnerHair", TfToken::Immortal),
      TfToken("PxrMarschnerHair", TfToken::Immortal) },
    { TfToken("PxrMatteID", TfToken::Immortal), TfToken("PxrMatteID", TfToken::Immortal) },
    { TfToken("PxrMeshLight", TfToken::Immortal), TfToken("PxrMeshLight", TfToken::Immortal) },
    { TfToken("PxrMix", TfToken::Immortal), TfToken("PxrMix", TfToken::Immortal) },
    { TfToken("PxrMultiTexture", TfToken::Immortal),
      TfToken("PxrMultiTexture", TfToken::Immortal) },
    { TfToken("PxrNormalMap", TfToken::Immortal), TfToken("PxrNormalMap", TfToken::Immortal) },
    { TfToken("PxrOSL", TfToken::Immortal), TfToken("PxrOSL", TfToken::Immortal) },
    { TfToken("PxrOcclusion", TfToken::Immortal), TfToken("PxrOcclusion", TfToken::Immortal) },
    { TfToken("PxrPanini", TfToken::Immortal), TfToken("PxrPanini", TfToken::Immortal) },
    { TfToken("PxrPathTracer", TfToken::Immortal), TfToken("PxrPathTracer", TfToken::Immortal) },
    { TfToken("PxrPortalLight", TfToken::Immortal), TfToken("PxrPortalLight", TfToken::Immortal) },
    { TfToken("PxrPrimvar", TfToken::Immortal), TfToken("PxrPrimvar", TfToken::Immortal) },
    { TfToken("PxrProjectionLayer", TfToken::Immortal),
      TfToken("PxrProjectionLayer", TfToken::Immortal) },
    { TfToken("PxrProjectionStack", TfToken::Immortal),
      TfToken("PxrProjectionStack", TfToken::Immortal) },
    { TfToken("PxrProjector", TfToken::Immortal), TfToken("PxrProjector", TfToken::Immortal) },
    { TfToken("PxrPtexture", TfToken::Immortal), TfToken("PxrPtexture", TfToken::Immortal) },
    { TfToken("PxrRamp", TfToken::Immortal), TfToken("PxrRamp", TfToken::Immortal) },
    { TfToken("PxrRampLightFilter", TfToken::Immortal),
      TfToken("PxrRampLightFilter", TfToken::Immortal) },
    { TfToken("PxrRandomTextureManifold", TfToken::Immortal),
      TfToken("PxrRandomTextureManifold", TfToken::Immortal) },
    { TfToken("PxrRectLight", TfToken::Immortal), TfToken("PxrRectLight", TfToken::Immortal) },
    { TfToken("PxrRemap", TfToken::Immortal), TfToken("PxrRemap", TfToken::Immortal) },
    { TfToken("PxrRodLightFilter", TfToken::Immortal),
      TfToken("PxrRodLightFilter", TfToken::Immortal) },
    { TfToken("PxrRoundCube", TfToken::Immortal), TfToken("PxrRoundCube", TfToken::Immortal) },
    { TfToken("PxrSeExpr", TfToken::Immortal), TfToken("PxrSeExpr", TfToken::Immortal) },
    { TfToken("PxrShadedSide", TfToken::Immortal), TfToken("PxrShadedSide", TfToken::Immortal) },
    { TfToken("PxrShadowDisplayFilter", TfToken::Immortal),
      TfToken("PxrShadowDisplayFilter", TfToken::Immortal) },
    { TfToken("PxrShadowFilter", TfToken::Immortal),
      TfToken("PxrShadowFilter", TfToken::Immortal) },
    { TfToken("PxrSharedSignalLightFilter", TfToken::Immortal),
      TfToken("PxrSharedSignalLightFilter", TfToken::Immortal) },
    { TfToken("PxrSphereCamera", TfToken::Immortal),
      TfToken("PxrSphereCamera", TfToken::Immortal) },
    { TfToken("PxrSphereLight", TfToken::Immortal), TfToken("PxrSphereLight", TfToken::Immortal) },
    { TfToken("PxrSurface", TfToken::Immortal), TfToken("PxrSurface", TfToken::Immortal) },
    { TfToken("PxrSwitch", TfToken::Immortal), TfToken("PxrSwitch", TfToken::Immortal) },
    { TfToken("PxrTangentField", TfToken::Immortal),
      TfToken("PxrTangentField", TfToken::Immortal) },
    { TfToken("PxrTee", TfToken::Immortal), TfToken("PxrTee", TfToken::Immortal) },
    { TfToken("PxrTexture", TfToken::Immortal), TfToken("PxrTexture", TfToken::Immortal) },
    { TfToken("PxrThinFilm", TfToken::Immortal), TfToken("PxrThinFilm", TfToken::Immortal) },
    { TfToken("PxrThreshold", TfToken::Immortal), TfToken("PxrThreshold", TfToken::Immortal) },
    { TfToken("PxrTileManifold", TfToken::Immortal),
      TfToken("PxrTileManifold", TfToken::Immortal) },
    { TfToken("PxrToFloat", TfToken::Immortal), TfToken("PxrToFloat", TfToken::Immortal) },
    { TfToken("PxrToFloat3", TfToken::Immortal), TfToken("PxrToFloat3", TfToken::Immortal) },
    { TfToken("PxrVCM", TfToken::Immortal), TfToken("PxrVCM", TfToken::Immortal) },
    { TfToken("PxrVariable", TfToken::Immortal), TfToken("PxrVariable", TfToken::Immortal) },
    { TfToken("PxrVary", TfToken::Immortal), TfToken("PxrVary", TfToken::Immortal) },
    { TfToken("PxrVisualizer", TfToken::Immortal), TfToken("PxrVisualizer", TfToken::Immortal) },
    { TfToken("PxrVolume", TfToken::Immortal), TfToken("PxrVolume", TfToken::Immortal) },
    { TfToken("PxrVoronoise", TfToken::Immortal), TfToken("PxrVoronoise", TfToken::Immortal) },
    { TfToken("PxrWatermarkFilter", TfToken::Immortal),
      TfToken("PxrWatermarkFilter", TfToken::Immortal) },
    { TfToken("PxrWhitePointDisplayFilter", TfToken::Immortal),
      TfToken("PxrWhitePointDisplayFilter", TfToken::Immortal) },
    { TfToken("PxrWhitePointSampleFilter", TfToken::Immortal),
      TfToken("PxrWhitePointSampleFilter", TfToken::Immortal) },
    { TfToken("PxrWireframe", TfToken::Immortal), TfToken("PxrWireframe", TfToken::Immortal) },
    { TfToken("PxrWorley", TfToken::Immortal), TfToken("PxrWorley", TfToken::Immortal) }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif