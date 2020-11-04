//
// Copyright 2017 Animal Logic
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
#include "AL/usdmaya/fileio/SchemaPrims.h"

#include <pxr/usd/usd/schemaBase.h>

#include <maya/MFnDagNode.h>

namespace AL {
namespace usdmaya {
namespace fileio {

/// the prim typename tokens
const TfToken ALSchemaType("ALType");
const TfToken ALExcludedPrimSchema("ALExcludedPrim");

//----------------------------------------------------------------------------------------------------------------------
/// \brief  hunt for the camera underneath the specified transform
/// \param  cameraNode the returned camera
/// \param  dagPath  path to the cameras transform node
//----------------------------------------------------------------------------------------------------------------------
void huntForParentCamera(MObject& cameraNode, const MDagPath& dagPath)
{
    MDagPath cameraPath = dagPath;
    cameraPath.pop();
    MFnDagNode cameraXform(cameraPath);
    for (uint32_t i = 0; i < cameraXform.childCount(); ++i) {
        if (cameraXform.child(i).hasFn(MFn::kCamera)) {
            cameraNode = cameraXform.child(i);
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
bool importSchemaPrim(
    const UsdPrim&                                   prim,
    MObject&                                         parent,
    MObject&                                         created,
    translators::TranslatorContextPtr                context,
    const translators::TranslatorRefPtr              torBase,
    const fileio::translators::TranslatorParameters& param)
{
    if (torBase) {
        if (param.forceTranslatorImport() || torBase->importableByDefault()) {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg("SchemaPrims::importSchemaPrim import %s\n", prim.GetPath().GetText());
            if (torBase->import(prim, parent, created) != MS::kSuccess) {
                std::cerr << "Failed to import schema prim \"" << prim.GetPath().GetText()
                          << "\"\n";
                return false;
            }
        } else {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg(
                    "SchemaPrims::Skipping import of '%s' since it is not importable by default \n",
                    prim.GetPath().GetText());
            return false;
        }
    } else {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
            .Msg(
                "SchemaPrims::importSchemaPrim Failed to find a translator for %s[%s]\n",
                prim.GetPath().GetText(),
                prim.GetTypeName().GetText());
        return false;
    }

    if (context)
        context->registerItem(prim, created == MObject::kNullObj ? parent : created);
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
SchemaPrimsUtils::SchemaPrimsUtils(fileio::translators::TranslatorManufacture& manufacture)
    : m_manufacture(manufacture)
{
}

//----------------------------------------------------------------------------------------------------------------------
bool SchemaPrimsUtils::needsTransformParent(const UsdPrim& prim)
{
    auto translator = m_manufacture.get(prim);
    return translator->needsTransformParent();
}

//----------------------------------------------------------------------------------------------------------------------
fileio::translators::TranslatorRefPtr SchemaPrimsUtils::isSchemaPrim(const UsdPrim& prim)
{
    return m_manufacture.get(prim);
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
