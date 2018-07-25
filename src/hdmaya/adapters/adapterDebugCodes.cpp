#include <hdmaya/adapters/adapterDebugCodes.h>

#include <pxr/base/tf/registryManager.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_GET,
        "Print information about 'Get' calls to the adapter.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_GET_LIGHT_PARAM_VALUE,
        "Print information about 'LightParamValue' calls to the light adapters.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
            HDMAYA_ADAPTER_LIGHT_SHADOWS,
        "Print information about shadow rendering.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_MESH_PLUG_DIRTY,
        "Print information about the mesh plug dirtying handled.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_MESH_UNHANDLED_PLUG_DIRTY,
        "Print information about unhandled mesh plug dirtying.");
}

PXR_NAMESPACE_CLOSE_SCOPE
