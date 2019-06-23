// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "base/api.h"
#include "importTranslator.h"

#include <maya/MFnPlugin.h>
#include <maya/MStatus.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_PLUGIN_PUBLIC
MStatus initializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "Autodesk", "1.0", "Any");

    MStatus status = plugin.registerFileTranslator(
        "mayaUsdImport",
        "",
        UsdMayaImportTranslator::creator,
        "usdTranslatorImport", // options script name
        const_cast<char*>(UsdMayaImportTranslator::GetDefaultOptions().c_str()),
        false);
    if (!status) {
        status.perror("mayaUsdPlugin: unable to register USD Import translator.");
    }

    return status;
}

MAYAUSD_PLUGIN_PUBLIC
MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);

    MStatus status = plugin.deregisterFileTranslator("mayaUsdImport");
    if (!status) {
        status.perror("mayaUsdPlugin: unable to deregister USD Import translator.");
    }
    return status;
}
