#include <maya/MFnPlugin.h>

#include "viewportRenderer.h"

#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

MStatus initializePlugin(MObject obj) {
    MFnPlugin plugin(obj, "Luma Pictures", "2018", "Any");
    MStatus ret = MS::kSuccess;

    auto* vp = HdViewportRenderer::getInstance();
    if (vp && !vp->registerRenderer()) {
        ret = MS::kFailure;
        ret.perror("Error registering hd viewport renderer!");
        HdViewportRenderer::cleanup();
    }

    return ret;
}


MStatus uninitializePlugin(MObject obj) {
    MFnPlugin plugin(obj, "Luma Pictures", "2018", "Any");
    MStatus ret = MS::kSuccess;

    auto* vp = HdViewportRenderer::getInstance();
    if (vp && vp->deregisterRenderer()) {
        ret = MS::kFailure;
        ret.perror("Error deregistering hd viewport renderer!");
    }
    HdViewportRenderer::cleanup();

    return ret;
}
