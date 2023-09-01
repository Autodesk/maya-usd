#include "mayaHydraSceneBrowser.h"

#include <mayaHydraLib/hydraUtils.h>
#include <mayaHydraLib/interfaceImp.h>

#include <sceneIndexDebuggerWidget.h>

#include <maya/MFnPlugin.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace MAYAHYDRA_NS;

HduiSceneIndexDebuggerWidget* widget = nullptr;
MStatus mayaHydraSceneBrowserCmd::doIt( const MArgList& args )
{
    const SceneIndicesVector& sceneIndices = GetMayaHydraLibInterface().GetTerminalSceneIndices();
    if (sceneIndices.empty()) {
        MGlobal::displayError("There are no registered terminal scene indices. The Hydra Scene Browser will not be shown.");
        return MS::kFailure;
    }
    else {
        if (!widget) {
            widget = new HduiSceneIndexDebuggerWidget(MQtUtil::mainWindow());
        }
        widget->setWindowTitle("Hydra Scene Browser");
        widget->setWindowFlags(Qt::Tool); // Ensure the browser stays in front of the main Maya window
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