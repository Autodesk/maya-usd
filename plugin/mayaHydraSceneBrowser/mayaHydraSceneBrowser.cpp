#include "mayaHydraSceneBrowser.h"

#include <mayaHydraLib/hydraUtils.h>
#include <mayaHydraLib/interfaceImp.h>

#include <sceneIndexDebuggerWidget.h>

#include <maya/MFnPlugin.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace MAYAHYDRA_NS;

HduiSceneIndexDebuggerWidget* widget = nullptr;
MStatus mayaHydraSceneBrowserCmd::doIt( const MArgList& args )
{
    const SceneIndicesVector& sceneIndices = GetMayaHydraLibInterface().GetTerminalSceneIndices();
    if (sceneIndices.empty()) {
        return MS::kFailure;
    }
    else {
        if (!widget) {
            widget = new HduiSceneIndexDebuggerWidget();
        }
        widget->SetSceneIndex("", sceneIndices.front(), true);
        widget->show();
    }

    return MS::kSuccess;
}

MStatus initializePlugin( MObject obj ) 
{
    MFnPlugin plugin( obj, "Autodesk", "1.0", "Any" );
    plugin.registerCommand( "mayaHydraSceneBrowser", mayaHydraSceneBrowserCmd::creator );
    return MS::kSuccess;
}

MStatus uninitializePlugin( MObject obj ) 
{
    MFnPlugin plugin( obj );
    plugin.deregisterCommand( "mayaHydraSceneBrowser" );
    return MS::kSuccess;
}