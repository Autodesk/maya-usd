#include "debugCodes.h"

#include <pxr/base/tf/registryManager.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
    	HDMAYA_AL_CALLBACKS,
        "Print info about the various callbacks used by hdmaya_al");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
    	HDMAYA_AL_PLUGIN,
        "Print info about the loading of the hdmaya_al plugin");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
    	HDMAYA_AL_POPULATE,
        "Print info about populating the delegate from the stage");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
    	HDMAYA_AL_SELECTION,
        "Print info about selecting AL objects in the maya-to-hydra viewport");
}

PXR_NAMESPACE_CLOSE_SCOPE
