#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/scriptModuleLoader.h"
#include "pxr/base/tf/token.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfScriptModuleLoader) {
    std::vector<TfToken> reqs;
    reqs.reserve(5);
    reqs.push_back(TfToken("usd"));
    reqs.push_back(TfToken("usdGeom"));
    TfScriptModuleLoader::GetInstance().
        RegisterLibrary(TfToken(#MFB_PACKAGE_NAME), TfToken("AL.usd.schemas.maya"), reqs);
}

PXR_NAMESPACE_CLOSE_SCOPE
