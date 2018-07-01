#include "adapterDebugCodes.h"

#include <pxr/base/tf/registryManager.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_MESH_PLUG_DIRTY,
        "Print information about the mesh plug dirtying handled.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_MESH_UNHANDLED_PLUG_DIRTY,
        "Print information about unhandled mesh plug dirtying.");
}

PXR_NAMESPACE_CLOSE_SCOPE
