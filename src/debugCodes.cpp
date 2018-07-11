#include "debugCodes.h"

#include <pxr/base/tf/registryManager.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
    	HDMAYA_AL_PLUGIN,
        "Print info about the loading of the hdmaya_al plugin");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
    	HDMAYA_AL_POPULATE,
        "Print info about populating the delegate from the stage");
}

PXR_NAMESPACE_CLOSE_SCOPE
