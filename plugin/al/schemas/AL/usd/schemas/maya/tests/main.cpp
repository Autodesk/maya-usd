#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/getenv.h>
#include <pxr/base/tf/stackTrace.h>
#include <pxr/base/tf/stringUtils.h>

#include <gtest/gtest.h>

#define AL_USDMAYA_LOCATION_NAME "AL_USDMAYA_LOCATION"

PXR_NAMESPACE_USING_DIRECTIVE
int main(int argc, char** argv)
{
    // Let USD know about the additional plugins
    std::string pluginLocation(
        TfStringCatPaths(TfGetenv(AL_USDMAYA_LOCATION_NAME), "share/usd/plugins"));
    PlugRegistry::GetInstance().RegisterPlugins(pluginLocation);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
