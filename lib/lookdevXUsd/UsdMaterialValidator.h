//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************
#pragma once

#include "Export.h"

#include "pxr/pxr.h"

#include <LookdevXUfe/ValidationLog.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdr/shaderProperty.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/shader.h>

#include <string>
#include <unordered_map>

namespace LookdevXUsd
{

struct UsdConnectionInfo;

//! \brief USD run-time Material handler.
/*!
        Factory object for Material interfaces.
 */
class UsdMaterialValidator
{
public:
    LOOKDEVX_USD_EXPORT explicit UsdMaterialValidator(const PXR_NS::UsdShadeMaterial& prim);

    LOOKDEVX_USD_EXPORT LookdevXUfe::ValidationLog::Ptr validate();

private:
    LOOKDEVX_USD_EXPORT bool visitDestination(const PXR_NS::UsdAttribute& dest);
    LOOKDEVX_USD_EXPORT bool validateShader(const PXR_NS::UsdShadeShader& shader);
    LOOKDEVX_USD_EXPORT void validateGlslfxShader(const PXR_NS::UsdShadeConnectableAPI& connectableAPI,
                                                  PXR_NS::SdrShaderNodeConstPtr shaderNode);
    LOOKDEVX_USD_EXPORT void validateMaterialXShader(const PXR_NS::UsdShadeConnectableAPI& connectableAPI,
                                                     PXR_NS::SdrShaderNodeConstPtr shaderNode);
    LOOKDEVX_USD_EXPORT bool validateMaterial(const PXR_NS::UsdShadeMaterial& material);
    LOOKDEVX_USD_EXPORT bool validateNodeGraph(const PXR_NS::UsdShadeNodeGraph& nodegraph);
    LOOKDEVX_USD_EXPORT bool validatePrim(const PXR_NS::UsdPrim& prim);
    LOOKDEVX_USD_EXPORT void validateConnection();
    LOOKDEVX_USD_EXPORT bool validateAcyclic();
    LOOKDEVX_USD_EXPORT void reportInvalidSources(const PXR_NS::UsdAttribute& dest,
                                                  const PXR_NS::SdfPathVector& invalidSourcePaths);
    LOOKDEVX_USD_EXPORT void traverseConnection();

    LOOKDEVX_USD_EXPORT LookdevXUfe::AttributeComponentInfo remapComponentConnectionAttribute(
        const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& attrName) const;
    LOOKDEVX_USD_EXPORT void validateComponentLocation(const LookdevXUfe::AttributeComponentInfo& attrInfo,
                                                       const std::string& errorDesc) const;
    LOOKDEVX_USD_EXPORT static Ufe::Path toUfe(const PXR_NS::UsdStageWeakPtr& stage, const PXR_NS::SdfPath& path);
    LOOKDEVX_USD_EXPORT static Ufe::Path toUfe(const PXR_NS::UsdPrim& prim);
    LOOKDEVX_USD_EXPORT LookdevXUfe::AttributeComponentInfo toUfe(const PXR_NS::UsdAttribute& attrib) const;
    LOOKDEVX_USD_EXPORT LookdevXUfe::AttributeComponentInfo toUfe(const PXR_NS::UsdPrim& prim,
                                                                  const PXR_NS::TfToken& attrName) const;
    LOOKDEVX_USD_EXPORT LookdevXUfe::Log::ConnectionInfo toUfe(const UsdConnectionInfo& cnx) const;

    const PXR_NS::UsdShadeMaterial& m_material;
    LookdevXUfe::ValidationLog::Ptr m_log;

    // Keep a stack of the current connection chain we are following. We can detect a cycle by taking the source
    // UsdShadeShader prim of the connection we are currently evaluating and traverse up the stack looking at the
    // UsdShadeShader prim of the destinations. If we have a match, then we have a cycle from the current connection up
    // to that one. Please note that we explicitly *ignore* the NodeGraph boundaries as it is possible to create a
    // scenario where there appears to be a loop at the NodeGraph level that would be absent in a flattened of the same
    // graph.
    std::vector<UsdConnectionInfo*> m_connectionStack;

    // This is the set of visited destinations. Once we are done the traversal from the material outputs, we wil
    // traverse a second time all the children of the material in order to detect issues with isolated islands that are
    // not yet connected to the material outputs.
    std::unordered_set<PXR_NS::SdfPath, PXR_NS::SdfPath::Hash> m_visitedDestinations;

    // Do not validate a node more than once:
    std::unordered_map<PXR_NS::SdfPath, bool, PXR_NS::SdfPath::Hash> m_validatedPrims;

    // Current severity level. When evaluating the graph connected to a material output problems are given the Error
    // level, but when we start looking at isolated subgraphs, we report issues as warnings.
    LookdevXUfe::Log::Severity m_currentSeverity = LookdevXUfe::Log::Severity::kError;

    // The current render context we are traversing. Will affect some validation rules.
    PXR_NS::TfToken m_renderContext;

    // we *might* need this map if we resolve a hidden combine node. In this case we
    // need to find out all the potential inputs connected to a combine node. We suspect
    // this will never be more that one
    std::unordered_map<PXR_NS::SdfPath, PXR_NS::UsdAttribute, PXR_NS::SdfPath::Hash> m_seenCombineConnections;

    // Keep a map of broken combine/separate found so we error only once:
    mutable std::set<std::string> m_brokenComponents;
};

} // namespace LookdevXUsd
