#include <maya/MFnPlugin.h>

#include "viewportRenderer.h"

#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
    std::unique_ptr<HdViewportRenderer> _viewportRenderer = nullptr;
}

MStatus initializePlugin(MObject obj) {
    MFnPlugin plugin(obj, "Luma Pictures", "2018", "Any");

    _viewportRenderer.reset(new HdViewportRenderer());
    if (!_viewportRenderer->registerRenderer()) {
        std::cerr << "Error registering hd viewport renderer!" << std::endl;
        _viewportRenderer = nullptr;
        return MS::kFailure;
    }

    return MS::kSuccess;
}


MStatus uninitializePlugin(MObject obj) {
    MFnPlugin plugin(obj, "Luma Pictures", "2018", "Any");

    if (_viewportRenderer != nullptr) {
        if (!_viewportRenderer->deregisterRenderer()) {
            std::cerr << "Error deregistering hd viewport renderer!" << std::endl;
        }
        _viewportRenderer = nullptr;
    }

    return MS::kSuccess;
}
