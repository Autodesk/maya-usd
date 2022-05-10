#include "CommonTranslatorOptions.h"

#include "AL/maya/utils/PluginTranslatorOptions.h"

#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/type.h>
#include <pxr/usd/usdGeom/tokens.h>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

PXR_NAMESPACE_USING_DIRECTIVE

AL::maya::utils::PluginTranslatorOptions* g_exportOptions = 0;
AL::maya::utils::PluginTranslatorOptions* g_importOptions = 0;

//----------------------------------------------------------------------------------------------------------------------
static const char* const g_compactionLevels[] = { "None", "Basic", "Medium", "Extensive", 0 };

//----------------------------------------------------------------------------------------------------------------------
static const char* const g_subdivisionSchemes[] = { "default", // Do not author opinion
                                                    UsdGeomTokens->catmullClark.GetText(),
                                                    UsdGeomTokens->none.GetText(),
                                                    UsdGeomTokens->loop.GetText(),
                                                    UsdGeomTokens->bilinear.GetText(),
                                                    0 };

//----------------------------------------------------------------------------------------------------------------------
void registerCommonTranslatorOptions()
{
    auto context = AL::maya::utils::PluginTranslatorOptionsContextManager::find("ExportTranslator");
    if (context) {
        g_exportOptions = new AL::maya::utils::PluginTranslatorOptions(*context, "Geometry Export");
        g_exportOptions->addBool(GeometryExportOptions::kNurbsCurves, true);
        g_exportOptions->addBool(GeometryExportOptions::kMeshes, true);
        g_exportOptions->addBool(GeometryExportOptions::kMeshConnects, true);
        g_exportOptions->addBool(GeometryExportOptions::kMeshPoints, true);
        g_exportOptions->addBool(GeometryExportOptions::kMeshExtents, true);
        g_exportOptions->addBool(GeometryExportOptions::kMeshNormals, true);
        g_exportOptions->addBool(GeometryExportOptions::kMeshVertexCreases, true);
        g_exportOptions->addBool(GeometryExportOptions::kMeshEdgeCreases, true);
        g_exportOptions->addBool(GeometryExportOptions::kMeshUvs, true);
        g_exportOptions->addBool(GeometryExportOptions::kMeshUvOnly, false);
        g_exportOptions->addBool(GeometryExportOptions::kMeshPointsAsPref, false);
        g_exportOptions->addBool(GeometryExportOptions::kMeshColours, true);
        g_exportOptions->addFloat(GeometryExportOptions::kMeshDefaultColourRGB, 0.18, 3);
        g_exportOptions->addFloat(GeometryExportOptions::kMeshDefaultColourA, 1.0, 3);
        g_exportOptions->addBool(GeometryExportOptions::kCustomColourThreshold, true);
        g_exportOptions->addFloat(
            GeometryExportOptions::kColourThresholdValue,
            0.00001,
            5,
            GeometryExportOptions::kCustomColourThreshold,
            true);
        g_exportOptions->addBool(GeometryExportOptions::kMeshHoles, true);
        g_exportOptions->addBool(GeometryExportOptions::kNormalsAsPrimvars, false);
        g_exportOptions->addBool(GeometryExportOptions::kReverseOppositeNormals, false);
        g_exportOptions->addEnum(
            GeometryExportOptions::kSubdivisionScheme, g_subdivisionSchemes, 0);
        g_exportOptions->addEnum(GeometryExportOptions::kCompactionLevel, g_compactionLevels, 3);
    }

    context = AL::maya::utils::PluginTranslatorOptionsContextManager::find("ImportTranslator");
    if (context) {
        g_importOptions = new AL::maya::utils::PluginTranslatorOptions(*context, "Geometry Import");
        g_importOptions->addBool(GeometryImportOptions::kNurbsCurves, true);
        g_importOptions->addBool(GeometryImportOptions::kMeshes, true);
    }
}

//----------------------------------------------------------------------------------------------------------------------
struct RegistrationHack
{
    RegistrationHack() { registerCommonTranslatorOptions(); }
};

//----------------------------------------------------------------------------------------------------------------------
RegistrationHack g_registration;

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
