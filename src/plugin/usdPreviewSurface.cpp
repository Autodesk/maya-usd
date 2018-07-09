#include "usdPreviewSurface.h"

PXR_NAMESPACE_OPEN_SCOPE

const MString HdMayaUsdPreviewSurface::classification("UsdPreviewSurface");
const MString HdMayaUsdPreviewSurface::name("UsdPreviewSurface");
const MTypeId HdMayaUsdPreviewSurface::typeId(0x0);

MStatus
HdMayaUsdPreviewSurface::Initialize() {
    return MS::kSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
